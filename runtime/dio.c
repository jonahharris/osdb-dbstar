/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database kernel                                             *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

#include "db.star.h"

#define MAX_SHORT_PRIME    32749    /* max prime that fits in a short */

static int  INTERNAL_FCN cache_init(PAGE_TABLE *, DB_TASK *);
static void INTERNAL_FCN cache_free(PAGE_TABLE *);
static int  INTERNAL_FCN pgtag_alloc(short *, DB_TASK *);
static void INTERNAL_FCN pgtag_free(short *, DB_TASK *);
static int  INTERNAL_FCN clear_cache(FILE_NO, FILE_NO, DB_TASK *);
static int  INTERNAL_FCN dio_findlru(FILE_NO, F_ADDR, short, PAGE_ENTRY * *, DB_TASK *);
static void INTERNAL_FCN dio_unhash(PAGE_ENTRY *, DB_TASK *);
static void INTERNAL_FCN dio_rehash(PAGE_ENTRY *, DB_TASK *);
static void INTERNAL_FCN dio_pzfree(int, DB_TASK *);
static int  INTERNAL_FCN dio_pzflush(DB_TASK *);
static int  INTERNAL_FCN isprime(long);

#define EXTENDFILES  0x00000010L    /* only data and key files are extended */

#define LU_HASH(f,p,sz)  ((short)(((f)+(p)) % (sz)))

/* ======================================================================
    Set the maximum number of open db.* files
*/
int INTERNAL_FCN dsetfiles(int num, DB_TASK *task)
{
    if (task->dbopen)
        dcloseall(task);

    if (num > 0 && num < (int) (FILEMASK + 1L))
        psp_fileSetHandleLimit(num);
    else
        dberr(S_INVNUM);

    return task->db_status;
}

/* ======================================================================
    Set number of virtual memory pages
*/
int INTERNAL_FCN dsetpages(
    int      dbpgs,             /* # of db cache pages */
    int      ixpgs,             /* # of index cache pages */
    DB_TASK *task)
{
    if ((dbpgs > MAX_SHORT_PRIME) || (ixpgs > MAX_SHORT_PRIME))
    {
        /* we can not allow numbers bigger than this because all the tables
           for the caches are using signed shorts as indices!
        */
        return (dberr(S_INVNUM));
    }

    if (task->cache->db_pgtab.pg_table)
    {
        /* There is already a cache - can't change size */
        return (dberr(S_SETPAGES));
    }

    task->cache->db_pgtab.pgtab_sz = (short) max(dbpgs, MINDBPAGES);
    task->cache->ix_pgtab.pgtab_sz = (short) max(ixpgs, MINIXPAGES);
    
    return (task->db_status);
}

void block_parms(off_t addr, size_t size, off_t *new_addr, size_t *new_size,
                 off_t *off, const SG *sg)
{
    size_t blocks;
    size_t blocksize = sg->blocksize;

    *off = addr % blocksize;
    *new_addr = addr - *off;
    blocks = (size - 1) / blocksize;
    if (*off >= (off_t) (blocksize - size % blocksize + 1))
        blocks++;

    *new_size = blocksize * blocks;
}

/* ======================================================================
    Open a database file
*/
int EXTERNAL_FCN dio_open(FILE_NO fno, DB_TASK *task)
{
    FILE_ENTRY *file_ptr = &task->file_table[fno];

    if (file_ptr->ft_status == OPEN)
        return (task->db_status);

    file_ptr->ft_desc = NULL;
    do
    {
        file_ptr->ft_desc = open_b(file_ptr->ft_name, O_RDWR, PSP_FLAG_SYNC,
                                   task);
        if (file_ptr->ft_desc == NULL)
        {
            int err = psp_errno();
            if (err == EACCES || err == EPIPE)
                return (dberr(S_EACCESS));    /* Sharing violation */

            return (dberr(S_NOFILE));
        }
    } while (file_ptr->ft_desc == NULL);

    file_ptr->ft_status = OPEN;
    ++task->cnt_open_files;

#ifdef DB_DEBUG
    if ((task->db_debug & LOCK_CHECK) && (file_ptr->ft_type != 'o'))
        task->pgzero[fno].file_mtime = psp_fileModTime(file_ptr->ft_desc);
#endif
    if (fno != task->ov_file)
        STAT_file_open(fno, task);
    else
        STAT_log_open(task);
    STAT_max_open(task->cnt_open_files, task);
    
    return (task->db_status);
}


/* ======================================================================
    Close a database file
    returns status, does not set task->db_status
*/
int EXTERNAL_FCN dio_close(FILE_NO fno, DB_TASK *task)
{
    FILE_ENTRY *file_ptr = &task->file_table[fno];

    if (file_ptr->ft_status == CLOSED)
        return (S_OKAY);  

    /* Commit file before closing only if needed */
    if (file_ptr->ft_flags & NEEDS_COMMIT)
    {
        /* Don't want commit_file to call dio_close recursively, so don't
           pass a file number ...
        */
        commit_file(file_ptr->ft_desc, task);
        file_ptr->ft_flags &= ~NEEDS_COMMIT;
    }

    if (file_ptr->ft_desc != NULL)
        psp_fileClose(file_ptr->ft_desc);

    file_ptr->ft_desc = NULL;
    file_ptr->ft_status = CLOSED;
    --task->cnt_open_files;

    return (task->db_status);
}

/* ======================================================================
    Initialize database I/O
*/
int INTERNAL_FCN dio_init(DB_TASK *task)
{
    /* init task on first open */
    if (!task->old_size_ft)
    {
        /* init file handling functions */
        task->last_file = 0;

        /* most recently used pages */
        task->last_datapage = NULL;
        task->last_keypage = NULL;
        task->last_ixpage = NULL;

        if (pgtag_alloc(&task->db_tag, task) != S_OKAY ||
            pgtag_alloc(&task->key_tag, task) != S_OKAY ||
            pgtag_alloc(&task->ix_tag, task) != S_OKAY)
            goto clean_exit;
    }

    if (task->cache->db_pgtab.pg_table)   /* re-init database cache */
    {
        if (dio_pzinit(task) != S_OKAY)
            goto clean_exit;

        if (task->page_size > task->cache->largest_page)
        {
            char *tempbuff;
            
            if (task->cache->prealloc)
            {
                /* must reallocate all cache pages */
                dio_clear(ALL_DBS, task);
                cache_free(&task->cache->db_pgtab);
                if (cache_init(&task->cache->db_pgtab, task) != S_OKAY)
                    return (task->db_status);
            }

            tempbuff = psp_cGetMemory(task->page_size, 0);
            if (tempbuff == NULL)
            {
                /* the cache has already been allocated, do not free it */
                return (dberr(S_NOMEMORY));
            }

            psp_freeMemory(task->cache->dbpgbuff, 0);
            task->cache->dbpgbuff = tempbuff;
            task->cache->largest_page = task->page_size;
        }

        return (S_OKAY);
    }

    /* the first task for a cache sets the rules */
    task->cache->prealloc = (task->dboptions & PREALLOC_CACHE) != 0;

    /* initialize database cache */
    if (cache_init(&task->cache->db_pgtab, task) != S_OKAY ||
        cache_init(&task->cache->ix_pgtab, task) != S_OKAY)
        goto clean_exit;

    /* allocate o_update buffer */
    task->cache->largest_page = task->page_size;
    task->cache->dbpgbuff = psp_cGetMemory(task->cache->largest_page, 0);
    if (task->cache->dbpgbuff == NULL)
    {
        dberr(S_NOMEMORY);
        goto clean_exit;
    }

    /* initialize the page zero table and return */
    dio_pzinit(task);

clean_exit:
    if (task->db_status != S_OKAY)
    {
        int stat = task->db_status;

        dio_free(ALL_DBS, task);
        task->db_status = stat;
    }
    return (task->db_status);
}

/* ======================================================================
*/
static int INTERNAL_FCN cache_init(PAGE_TABLE *pgtab, DB_TASK *task)
{
    short       pg_no;
    PAGE_ENTRY *pg_ptr = pgtab->pg_table;

    /* lookup_sz is prime number >= pgtab_sz */
    pgtab->lookup_sz = pgtab->pgtab_sz;
    pgtab->lookup_sz |= 1;
    while (! isprime(pgtab->lookup_sz))
        pgtab->lookup_sz += 2;

    pgtab->lookup = (LOOKUP_ENTRY *)psp_cGetMemory(pgtab->lookup_sz * sizeof(LOOKUP_ENTRY), 0);
    pgtab->pg_table = (PAGE_ENTRY *)psp_cGetMemory(pgtab->pgtab_sz * sizeof(PAGE_ENTRY), 0);
    if (!pgtab->lookup || !pgtab->pg_table)
        return (dberr(S_NOMEMORY));
    memset(pgtab->pg_table, '\0', pgtab->pgtab_sz * sizeof(PAGE_ENTRY));

    /* this code assumes that the number of pages <= size of lookup pgtab */
    for (pg_no = 0, pg_ptr = pgtab->pg_table;
         pg_no < pgtab->pgtab_sz; ++pg_no, ++pg_ptr)
    {
        pg_ptr->file = -1;         /* page not in use */

        if (task->cache->prealloc)
        {
            /* allocate the cache pages now using the largest page size to make
               sure that there is enough memory.
            */
            pg_ptr->buff = psp_cGetMemory(task->page_size, 0);
            if (!pg_ptr->buff)
                return (dberr(S_NOMEMORY));

            pg_ptr->buff_size = (short) task->page_size;
#ifdef DBSTAT
            STAT_mem_alloc(pgtab, task->page_size);
#endif
        }
    }

    return (task->db_status);
}

/* ======================================================================
    Free the memory allocated for pages
*/
void INTERNAL_FCN dio_free(int dbn, DB_TASK *task)
{
    task->last_datapage = NULL;
    task->last_keypage = NULL;
    task->last_ixpage = NULL;

    dio_pzfree(dbn, task);

    if (dbn != ALL_DBS)
    {
        /* Files being free'd have already had their cache pages cleared,
           so just update the file reference numbers of files being moved
           down in file table.  The ix-cache should be empty at this point.
        */
        int i, high;
        PAGE_ENTRY *pg_ptr;

        high = task->db_table[dbn].ft_offset + task->db_table[dbn].Size_ft - 1;
        for (i = 0, pg_ptr = task->cache->db_pgtab.pg_table; i < task->cache->db_pgtab.pgtab_sz; ++i, ++pg_ptr)
        {
            if (pg_ptr->file > high)
            {
                dio_unhash(pg_ptr, task);
                pg_ptr->file -= task->db_table[dbn].Size_ft;
                dio_rehash(pg_ptr, task);
            }
        }
    }
    else
    {
        pgtag_free(&task->db_tag, task);
        pgtag_free(&task->key_tag, task);
        pgtag_free(&task->ix_tag, task);

#ifdef DBSTAT
        if (task->cache->db_pgtab.pg_table)
        {
            sync_MEM_STATS(&task->gen_stats.dbmem_stats, &task->cache->db_pgtab.mem_stats);
            sync_CACHE_STATS(&task->gen_stats.db_stats, &task->cache->db_pgtab.cache_stats);
        }
        if (task->cache->ix_pgtab.pg_table)
        {
            sync_MEM_STATS(&task->gen_stats.ixmem_stats, &task->cache->ix_pgtab.mem_stats);
            sync_CACHE_STATS(&task->gen_stats.ix_stats, &task->cache->ix_pgtab.cache_stats);
        }
#endif
        cache_free(&task->cache->db_pgtab);
        cache_free(&task->cache->ix_pgtab);

        if (task->cache->dbpgbuff)
            psp_freeMemory(task->cache->dbpgbuff, 0);

        task->cache->dbpgbuff = NULL;
    }
}

/* ======================================================================
*/
static void INTERNAL_FCN cache_free(PAGE_TABLE *pgtab)
{
    int i;
    PAGE_ENTRY *pg_ptr;

    if (pgtab->pg_table)   /* null if dio_init() failed */
    {
        for (i = 0, pg_ptr = pgtab->pg_table; i < pgtab->pgtab_sz; ++i, ++pg_ptr)
        {
            if (pg_ptr->buff)
                psp_freeMemory(pg_ptr->buff, 0);
        }

        psp_freeMemory(pgtab->pg_table, 0);
        pgtab->pg_table = NULL;
    }

    if (pgtab->lookup)
    {
        psp_freeMemory(pgtab->lookup, 0);
        pgtab->lookup = NULL;
    }

#ifdef DBSTAT
    memset(&pgtab->mem_stats,   '\0', sizeof(MEM_STATS));
    memset(&pgtab->cache_stats, '\0', sizeof(CACHE_STATS));
#endif
}

/* ======================================================================
    Allocate a page tag
*/
static int INTERNAL_FCN pgtag_alloc(short *tag, DB_TASK *task)
{
    short tag_idx;
    PAGE_TAG *tag_ptr;

    if (!task->cache->tag_table)
        task->cache->num_tags = task->cache->max_tags = 0;

    for (tag_idx = 0, tag_ptr = task->cache->tag_table; tag_idx < task->cache->max_tags; ++tag_idx, ++tag_ptr)
    {
        if (tag_ptr->lru_page == -1)
            break;
    }
    *tag = tag_idx;

    if (tag_idx >= task->cache->max_tags)
    {
        if (alloc_table((void **) &task->cache->tag_table,
                        (task->cache->max_tags + 3) * sizeof(PAGE_TAG),
                        task->cache->max_tags * sizeof(PAGE_TAG),
                        task) != S_OKAY)
            return (task->db_status);

        task->cache->max_tags += 3;    /* arbitrary small value */
        for (tag_ptr = &task->cache->tag_table[tag_idx]; tag_idx < task->cache->max_tags; ++tag_idx, ++tag_ptr)
            tag_ptr->lru_page = -1;
    }

    tag_ptr = &task->cache->tag_table[*tag];
    tag_ptr->lru_page = 0;
    tag_ptr->num_pages = 0;
    tag_ptr->lookups = tag_ptr->hits = 0L;
    ++task->cache->num_tags;

    return (task->db_status);
}

/* ======================================================================
    Free a page tag
*/
static void INTERNAL_FCN pgtag_free(short *tag, DB_TASK *task)
{
    if (task->cache->tag_table)
    {
        if (*tag >= 0 && *tag < task->cache->max_tags)
        {
            task->cache->tag_table[*tag].lru_page = -1;
            if (--task->cache->num_tags == 0)
            {
                psp_freeMemory(task->cache->tag_table, 0);
                task->cache->tag_table = NULL;
            }

            return;
        }
    }

    dberr(SYS_INVPGTAG);
}

/* ======================================================================
    Clear database page cache
*/
int INTERNAL_FCN dio_clrfile(FILE_NO fno, DB_TASK *task)
{
    return clear_cache(fno, (FILE_NO)(fno+1), task);
}

int EXTERNAL_FCN dio_clear(int dbn, DB_TASK *task)
{
    FILE_NO from, to;

    if (dbn == ALL_DBS)
    {
        from = 0;
        to   = task->size_ft;
    }
    else
    {
        if (dbn == CURR_DB)
            dbn = task->curr_db;
        from = task->db_table[dbn].ft_offset;
        to   = from + task->db_table[dbn].Size_ft;
    }

    return clear_cache(from, to, task);
}

/*
    called from:  action:
     d_iclose      clear all pages for db
     d_destroy     clear all pages for db
     d_initfile    clear all pages for file
     d_trabort     clear all non-temporary modified pages
     d_lock        clear all non-temporary, non-modifyable static pages
*/
static int INTERNAL_FCN clear_cache(
    FILE_NO fr_file,        /* clear from file "fr_file" */
    FILE_NO to_file,        /* ... to (not thru) file "to_file" */
    DB_TASK *task)
{
    FILE_NO            fno;
    PAGE_ENTRY *pg_ptr;
    PAGE_ENTRY *nxt_ptr;
    PGZERO     *pgzero_ptr;
    FILE_ENTRY *file_ptr;

    for (fno = fr_file, pgzero_ptr = &task->pgzero[fno]; fno < to_file; fno++, pgzero_ptr++)
    {
        for (pg_ptr = pgzero_ptr->pg_ptr; pg_ptr; pg_ptr = nxt_ptr)
        {
            nxt_ptr = pg_ptr->n_pg;

            if (dio_clrpage(pg_ptr, task) != S_OKAY)
                return (task->db_status);
        }
    }

    /* clear page zeroes */
    for (fno = fr_file, pgzero_ptr = &task->pgzero[fno], file_ptr = &task->file_table[fno];
         fno < to_file;
         ++fno, ++pgzero_ptr, ++file_ptr)
    {
        pgzero_ptr->pz_next = (F_ADDR) 0;  /* force it to be re-read */
        if (pgzero_ptr->pz_modified && !(file_ptr->ft_flags & TEMPORARY))
        {
            if (file_ptr->ft_status == OPEN)
                commit_file(file_ptr->ft_desc, task);
        }

        pgzero_ptr->pz_modified = FALSE;
        file_ptr->ft_flags &= ~NEEDS_COMMIT;
    }

    return (task->db_status);
}

/* ======================================================================
*/
void INTERNAL_FCN dio_ixclear(DB_TASK *task)
{
    PAGE_ENTRY *ixpg_ptr;
    int                x;

    for (x = task->cache->ix_pgtab.pgtab_sz, ixpg_ptr = task->cache->ix_pgtab.pg_table; x; --x, ++ixpg_ptr)
    {
        if (ixpg_ptr->file != -1)
            dio_clrpage(ixpg_ptr, task);
    }
}

/* ======================================================================
    Flush database I/O buffer
*/
int EXTERNAL_FCN dio_flush(DB_TASK *task)
{
    FILE_NO            fno;
    FILE_ENTRY *file_ptr;
    PAGE_ENTRY *pg_ptr;
    PAGE_ENTRY *nxt_ptr;

    if (task->cache->db_pgtab.pg_table == NULL)
        return (task->db_status);

    for (fno = 0, file_ptr = task->file_table; fno < task->size_ft; ++fno, ++file_ptr)
    {
        if (file_ptr->ft_flags & TEMPORARY)
            continue;

        for (pg_ptr = task->pgzero[fno].pg_ptr; pg_ptr; pg_ptr = nxt_ptr)
        {
            nxt_ptr = pg_ptr->n_pg;
            if (! pg_ptr->modified)
            {
                pg_ptr->ovfl_addr = 0L;
                continue;
            }

            if ((task->dboptions & TRLOGGING) && task->trans_id[0] && (! task->trcommit))
            {
                /* flush to overflow/log file -- before tr commit */
                if (o_write(pg_ptr, task) != S_OKAY)
                    return (task->db_status);

                if (pg_ptr->pageno > o_pages(pg_ptr->file, task))
                {
                    file_ptr->ft_flags |= NEEDS_COMMIT;
                    /* no need to rewrite this page at task->trcommit time */
                }
                else
                    continue; /* skip call to trlog and reinit of holdcnt, modified */
            }
            else
            {
                /* write directly to database */
                pg_ptr->ovfl_addr = 0L;
                if (dio_out(pg_ptr, task) != S_OKAY)
                    return (task->db_status);
                file_ptr->ft_flags |= NEEDS_COMMIT;
            }

            pg_ptr->holdcnt = 0;
            pg_ptr->modified = FALSE;
            if (task->trlog_flag)
            {
                dtrlog(fno, pg_ptr->pageno, pg_ptr->buff, file_ptr->ft_pgsize, task);
            }
        }
    }

    /* store the page zero values in the data file and return */
    if (dio_pzflush(task) != S_OKAY)
        return (task->db_status);

    for (fno = 0, file_ptr = task->file_table; fno < task->size_ft; ++fno, ++file_ptr)
    {
        if (file_ptr->ft_flags & TEMPORARY)
            continue;

        if (file_ptr->ft_flags & NEEDS_COMMIT)
        {
            /* Commit the file even it was closed */
            if ((file_ptr->ft_status != OPEN)
              && dio_open(fno, task) != S_OKAY)
                 return (task->db_status);

            commit_file(file_ptr->ft_desc, task);
            file_ptr->ft_flags &= ~NEEDS_COMMIT;
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Database I/O page get for a key file.
*/
int EXTERNAL_FCN dio_get(
    FILE_NO  fno,
    F_ADDR   page_no,
    char   **page_ptr,
    int      hold,
    DB_TASK *task)
{
    FILE_ENTRY *file_ptr = &task->file_table[fno];
    PAGE_ENTRY *pg_ptr = NULL;

    if (task->dbopen == 1)
    {
        if (!task->app_locks[fno] && !task->excl_locks[fno]
          && !(file_ptr->ft_flags & STATIC))
        {
            return (dberr(S_NOTLOCKED));
        }
    }

    if ((task->pgzero[fno].pz_next == (F_ADDR) 0
      &&  dio_pzread(fno, task) != S_OKAY)
#ifdef DB_DEBUG
      || dio_pzcheck(fno, task) != S_OKAY
#endif
      )
        return (task->db_status);

    if (dio_getpg(fno, page_no, task->key_tag,
                  &task->last_keypage, &pg_ptr, task) == S_OKAY)
    {
        if (pg_ptr == NULL || pg_ptr->buff == NULL)
            return (dberr(S_INVPTR));

        *page_ptr = pg_ptr->buff;
        pg_ptr->recently_used = 1;

        if (hold)
            ++pg_ptr->holdcnt;
    }
    return (task->db_status);
}


/* ======================================================================
    Set modified flag for a page for the key file
*/
int INTERNAL_FCN dio_touch(
    FILE_NO fno,
    F_ADDR page_no,
    int release,
    DB_TASK *task)
{
    FILE_ENTRY *file_ptr = &task->file_table[fno];
    PAGE_ENTRY *pg_ptr = NULL;

    if (task->trans_id[0] && o_fileinit(fno, task) != S_OKAY)
        return (task->db_status);

    if (task->dbopen == 1)
    {
        /* check shared access privileges */
        if (!task->trans_id[0] && !task->excl_locks[fno])
            return (dberr(S_NOTRANS));
        if (task->app_locks[fno] >= 0 && !task->excl_locks[fno])
            return (dberr(S_NOTLOCKED));
    }

    if (dio_findpg(fno, page_no, &task->last_keypage, &pg_ptr, task) != S_OKAY)
    {
        /* changed page was lost if not previously marked modified */
        dberr(S_FAULT);
    }
    else
    {
        pg_ptr->recently_used = 1;

        pg_ptr->modified = TRUE;
        if (release)
        {
            if (--(pg_ptr->holdcnt) < 0)
                dberr(SYS_INVHOLD);
        }
    }
    return (task->db_status);
}

/* ======================================================================
    Release Hold obtained with dio_get on this key file page
*/
int EXTERNAL_FCN dio_unget(
    FILE_NO fno,
    F_ADDR page_no,
    int release,
    DB_TASK *task)
{
    PAGE_ENTRY *pg_ptr = NULL;

    if (dio_findpg(fno, page_no, &task->last_keypage, &pg_ptr, task) != S_OKAY)
    {
        dberr(S_FAULT);
    }
    else
    {
        if (release)
        {
            if (--(pg_ptr->holdcnt) < 0)
                dberr(SYS_INVHOLD);
        }
    }
    return (task->db_status);
}




/* ======================================================================
    Database I/O read
*/
int EXTERNAL_FCN dio_read(
    DB_ADDR  dba,
    char   **recptr,
    int      hold,
    DB_TASK *task)
{
    short       file;
    int         offset;
    DB_ULONG    us1, us2;
    FILE_ENTRY *file_ptr;
    PAGE_ENTRY *pg_ptr = NULL;

    DECODE_DBA(dba, &file, &us1);
    file = NUM2INT(file, ft_offset);
    file_ptr = &task->file_table[file];

    if ((task->pgzero[file].pz_next == (F_ADDR) 0
        && dio_pzread(file, task) != S_OKAY)
#ifdef DB_DEBUG
        || dio_pzcheck(file, task) != S_OKAY
#endif
      )
        return (task->db_status);

    if (task->dbopen == 1)
    {
        /* check shared access privileges */
        if (!task->app_locks[file] && !task->excl_locks[file] &&
            !(file_ptr->ft_flags & STATIC))
        {
            return (dberr(S_NOTLOCKED));
        }
    }

    us2 = (us1 - 1) / file_ptr->ft_slots;
    if (dio_getpg(file, us2 + 1, task->db_tag,
                  &task->last_datapage, &pg_ptr, task) == S_OKAY)
    {
        if (pg_ptr->buff == NULL)
            return (dberr(S_INVPTR));

        pg_ptr->recently_used = 1;

        offset = file_ptr->ft_slsize * (int) (us1 - 1 - us2 *
                        file_ptr->ft_slots) + PGHDRSIZE;
        *recptr = &pg_ptr->buff[offset];
        if (hold)
            ++(pg_ptr->holdcnt);
    }
    return (task->db_status);
}


/* ======================================================================
    Database I/O write
*/
int EXTERNAL_FCN dio_write(DB_ADDR dba, int release, DB_TASK *task)
{
    short       file;
    DB_ULONG    us1, us2;
    FILE_ENTRY *file_ptr;
    PAGE_ENTRY *pg_ptr = NULL;

    DECODE_DBA(dba, &file, &us1);
    file = (FILE_NO) NUM2INT(file, ft_offset);
    file_ptr = &task->file_table[file];

    if (task->trans_id[0] && o_fileinit(file, task) != S_OKAY)
        return (task->db_status);

    if (task->dbopen == 1)
    {
        if (!task->trans_id[0] && !task->excl_locks[file])
            return (dberr(S_NOTRANS));

        /* check shared access priviledges */
        if (task->app_locks[file] >= 0 && !task->excl_locks[file])
            return (dberr(S_NOTLOCKED));
    }

    us2 = (us1 - 1) / file_ptr->ft_slots;
    if (dio_findpg(file, us2 + 1, &task->last_datapage, &pg_ptr, task) != S_OKAY)
    {
        /* changed page was lost if not previously marked modified */
        dberr(S_FAULT);
    }
    else
    {
        pg_ptr->recently_used = 1;

        pg_ptr->modified = TRUE;
        if (release)
        {
            if (--(pg_ptr->holdcnt) < 0)
                dberr(SYS_INVHOLD);
        }
    }
    return (task->db_status);
}


/* ======================================================================
    Release database page hold
*/
int EXTERNAL_FCN dio_release(DB_ADDR dba, int release, DB_TASK *task)
{
    short       file;
    DB_ULONG    us1, us2;
    PAGE_ENTRY *pg_ptr = NULL;

    DECODE_DBA(dba, &file, &us1);
    file = (FILE_NO) NUM2INT(file, ft_offset);

    us2 = (us1 - 1) / task->file_table[file].ft_slots;
    if (dio_findpg(file, us2 + 1, &task->last_datapage, &pg_ptr, task) != S_OKAY)
    {
        dberr(S_FAULT);
    }
    else if (release)
    {
        if (--(pg_ptr->holdcnt) < 0)
            dberr(SYS_INVHOLD);
    }
    return (task->db_status);
}


/* ======================================================================
    Read record lock bit
*/
int INTERNAL_FCN dio_rrlb(
    DB_ADDR  dba,
    short   *rid,
    DB_TASK *task)
{
    FILE_NO     file;           /* file number */
    F_ADDR      page;           /* page number */
    F_ADDR      sno;            /* slot number */
    short       spp;            /* slots per page */
    off_t       offset;         /* lseek offset from start of file */
    FILE_ENTRY *file_ptr;
    PAGE_ENTRY *pg_ptr = NULL;
    const SG   *sg;

    DECODE_DBA(dba, &file, &sno);
    file = (FILE_NO) NUM2INT(file, ft_offset);
    file_ptr = &task->file_table[file];
    spp = file_ptr->ft_slots;
    page = ((sno - 1) / spp) + 1;
    offset = PGHDRSIZE + (off_t)(sno - 1 - (page - 1) * spp) * file_ptr->ft_slsize;

    /* This fcn always goes to disk on a read-locked file because multiple
       users could update the same page.  If the file is write- or exclusely-
       locked, then the cache can be used because the cache and disk are in
       sync since no one else can update the file.  Also, if d_rlbclr() is
       called in a trx, the disk does not get updated until d_trend().
    */
    if ((task->dbopen > 1) || task->excl_locks[file] || (task->app_locks[file] < 0))
    {
        if (dio_getpg(file, page, task->db_tag, &task->last_datapage,
                      &pg_ptr, task) != S_OKAY)
        {
            /* this failure was a system level failure, not S_NOTFOUND! */
            return (task->db_status);
        }
        if (pg_ptr->buff == NULL)
            return (dberr(S_INVPTR));
        /* record in cache - update only rlb in rid */
        memcpy(rid, &pg_ptr->buff[offset], sizeof(short));
    }
    else
    {
        offset += page * file_ptr->ft_pgsize;
        if ((sg = task->sgs[file]) != NULL)
        {
            off_t  begin;
            off_t  off;
            size_t size;

            block_parms(offset, sizeof(short), &begin, &size, &off, sg);
            if (dio_readfile(file, begin, task->enc_buff, size, 0,
                    NULL, task) != S_OKAY)
                return task->db_status;

            (*sg->dec)(sg->data, task->enc_buff, size);
            memcpy(&rid, (char *) task->enc_buff + off, sizeof(short));
        }
        else
        {
            if (dio_readfile(file, offset, rid, sizeof(short), 0, NULL,
                    task) != S_OKAY)
                return (task->db_status);
        }

        STAT_rlb_read(file, sizeof(short), task);
    }

    return (task->db_status);
}

/* ======================================================================
    Write record lock bit
*/
/*
    Setting the RLB requires that both the cache and the disk be updated
    at the same time if the file is locked, else just the disk when not
    locked.

    Clearing the RLB requires that both the cache and disk be updated when
    locked, too.  However, if clearing in a trx, then update only the cache
    because the d_trend() will write the cache to disk thus clearing the
    RLB on disk.  When clearing the RLB in a trx, we need it to remain set
    on disk until the end of the trx or to be not cleared if d_trabort()
    is called.
*/
int INTERNAL_FCN dio_wrlb(
    DB_ADDR  dba,
    short    rid,
    DB_TASK *task)
{
    FILE_NO     file;       /* file number */
    F_ADDR      page;       /* page number */
    F_ADDR      sno;        /* slot number */
    short       spp;        /* slots per page */
    off_t       offset;     /* offset from start of page or file */
    int         clr_in_trx; /* true if called from d_rlbclr in trx */
    short       trid;       /* temp rid */
    FILE_ENTRY *file_ptr;
    PAGE_ENTRY *pg_ptr = NULL;
    const SG   *sg;

    if (task->dboptions & READONLY)
        return (dberr(S_READONLY));

    DECODE_DBA(dba, &file, &sno);
    file = (FILE_NO) NUM2INT(file, ft_offset);
    file_ptr = &task->file_table[file];
    spp = file_ptr->ft_slots;
    page = ((sno - 1) / spp) + 1;
    offset = PGHDRSIZE + (off_t)(sno - 1 - (page - 1) * spp) * file_ptr->ft_slsize;

    /* d_rlbclr() inside a trx requires a w-/x-lock, caught by d_rlbclr() */
    clr_in_trx = !(rid & RLBMASK) && task->trans_id[0];

    /* one-user or (x-locked or (not not-locked)) */
    if (task->dbopen > 1 || task->excl_locks[file] || task->app_locks[file])
    {
        /* file is locked -- update the cache */
        if (dio_getpg(file, page, task->db_tag, &task->last_datapage, &pg_ptr,
                      task) != S_OKAY)
        {
            /* this failure was a system level failure, not S_NOTFOUND! */
            return (task->db_status);
        }

        if (pg_ptr->buff == NULL)
            return (dberr(S_INVPTR));

        /* record in cache - update only rlb in rid */
        memcpy(&trid, &pg_ptr->buff[offset], sizeof(short));
        rid = (short) ((trid & ~((short) RLBMASK)) | (rid & RLBMASK));
        memcpy(&pg_ptr->buff[offset], &rid, sizeof(short));
        if (clr_in_trx)
            pg_ptr->modified = TRUE;
    }

    if (! clr_in_trx)
    {
        /* update directly to disk (rlb part of record only) */
        offset += page * file_ptr->ft_pgsize;
        if ((sg = task->sgs[file]) != NULL)
        {
            /* If we have an encrypted database, we can't just update the 
               record lock bit field directly.  Instead we have to grab the
               data around it on boundaries based on the block size of the
               encryption algorithm and unencrypt it.  Then we can get the
               record lock bit out, update it and copy it back, at which
               time we can re-encrypt and re-write the block(s)*/
            off_t  begin;
            off_t  off;
            size_t size;

            block_parms(offset, sizeof(short), &begin, &size, &off, sg);
            if (dio_readfile(file, begin, task->enc_buff, size, 0, NULL,
                    task) != S_OKAY)
                return task->db_status;

            (*sg->dec)(sg->data, task->enc_buff, size);
            memcpy(&trid, (char *) task->enc_buff + off, sizeof(short));
            rid = (short) ((trid & ~((short) RLBMASK)) | (rid & RLBMASK));
            /* write original rid out with modified rlb */
            memcpy((char *) task->enc_buff + off, &rid, sizeof(short));
            (*sg->enc)(sg->data, task->enc_buff, size);

            if (dio_writefile(file, begin, task->enc_buff, size, 0,
                    task) != S_OKAY)
                return task->db_status;
        }
        else
        {
            if (dio_readfile(file, offset, &trid, sizeof(short), 0, NULL,
                    task) != S_OKAY)
                return (task->db_status);

            rid = (short) ((trid & ~((short) RLBMASK)) | (rid & RLBMASK));
            /* write original rid out with modified rlb */
            if (dio_writefile(file, offset, &rid, sizeof(short), 0,
                    task) != S_OKAY)
                return (task->db_status);
        }

        STAT_rlb_read(file, sizeof(short), task);
        STAT_rlb_write(file, sizeof(short), task);
        commit_file(file_ptr->ft_desc, task);
    }

    return (task->db_status);
}


/* ======================================================================
    Search a cache for page
*/
int INTERNAL_FCN dio_findpg(
    FILE_NO file,               /* file table index */
    F_ADDR page,                /* page in file */
    PAGE_ENTRY **last_lu,       /* the last looked-up page */
    PAGE_ENTRY **xpg_ptr,
    DB_TASK *task)
{
    PAGE_TABLE *pgtab = (file == task->ov_file) ? &task->cache->ix_pgtab : &task->cache->db_pgtab;
    register PAGE_ENTRY *pg_ptr;
    short bucket;

    /* check if desired page was last one */
    if (last_lu && *last_lu)
    {
        pg_ptr = *last_lu;
        /* sanity check */
        if (pg_ptr >= pgtab->pg_table
         && pg_ptr <  pgtab->pg_table + pgtab->pgtab_sz)
        {
            if ((file == pg_ptr->file) && (page == pg_ptr->pageno))
            {
                if (xpg_ptr) *xpg_ptr = pg_ptr;
                return (task->db_status);
            }
        }
    }

    /* perform hash lookup */
    bucket = LU_HASH(file, page, pgtab->lookup_sz);
    pg_ptr = pgtab->lookup[bucket];
    while (pg_ptr && ((pg_ptr->file != file) || (pg_ptr->pageno != page)))
    {
        /* search the chain for this bucket */
#ifdef DB_DEBUG
        if (pg_ptr->next == pg_ptr)
            return (dberr(SYS_HASHCYCLE));
#endif
        pg_ptr = pg_ptr->next;
    }

    if (last_lu) *last_lu = pg_ptr;
    if (xpg_ptr) *xpg_ptr = pg_ptr;
    if (!pg_ptr)
        task->db_status = S_NOTFOUND;
    return (task->db_status);
}

/* ======================================================================
    Find page to replace in the single unified cache.
    Returns S_FAULT if there are no available pages
*/
static int INTERNAL_FCN dio_findlru(
    FILE_NO file,       /* file table index */
    F_ADDR page,        /* page in file */
    short pg_tag,
    PAGE_ENTRY **rlu_ptr,
    DB_TASK *task)
{
    PAGE_TABLE *pgtab = (file == task->ov_file) ? &task->cache->ix_pgtab : &task->cache->db_pgtab;
    short      *lru_page = &(task->cache->tag_table[pg_tag].lru_page);
    int         pass, x;
    PAGE_ENTRY *pg_ptr;

#ifdef DB_DEBUG
    if (*lru_page < 0 || *lru_page >= pgtab->pgtab_sz)
        return (dberr(SYS_INVLRU));
#endif

    pg_ptr = &pgtab->pg_table[*lru_page]; *rlu_ptr = NULL;
    for (pass = 2; pass > 0; --pass)
    {
        for (x = pgtab->pgtab_sz; x > 0; --x)
        {
            if (pg_ptr->recently_used)
            {
                if (pg_ptr->holdcnt == 0)
                    pg_ptr->recently_used = 0;
            }
            else if (!pg_ptr->holdcnt)
            {
                *rlu_ptr = pg_ptr;
                break;
            }

            if (++*lru_page >= pgtab->pgtab_sz)
            {
                *lru_page = 0;
                pg_ptr = pgtab->pg_table;
            }
            else
                ++pg_ptr;
        }
        if (*rlu_ptr)
        {
            if (++*lru_page >= pgtab->pgtab_sz)
                *lru_page = 0;
            return (task->db_status);
        }
    }
    if (!(*rlu_ptr))
        dberr(S_FAULT);
    return (task->db_status);
}

/* ======================================================================
*/
static void INTERNAL_FCN dio_unhash(PAGE_ENTRY *pg_ptr, DB_TASK *task)
{
    FILE_NO file = pg_ptr->file;
    PAGE_TABLE *pgtab = (file == task->ov_file) ? &task->cache->ix_pgtab : &task->cache->db_pgtab;
    short bucket;

#ifdef DB_DEBUG
    if (pg_ptr <  pgtab->pg_table
     || pg_ptr >= pgtab->pg_table + pgtab->pgtab_sz)
    {
        dberr(SYS_INVPAGE);
        return;
    }
#endif

    if (pg_ptr->next)
        pg_ptr->next->prev = pg_ptr->prev;

    if (pg_ptr->prev)
        pg_ptr->prev->next = pg_ptr->next;
    else
    {
        bucket = LU_HASH(file, pg_ptr->pageno, pgtab->lookup_sz);
        pgtab->lookup[bucket] = pg_ptr->next;
    }
    pg_ptr->next = pg_ptr->prev = NULL;
}

static void INTERNAL_FCN dio_rehash(PAGE_ENTRY *pg_ptr, DB_TASK *task)
{
    FILE_NO file = pg_ptr->file;
    PAGE_TABLE *pgtab = (file == task->ov_file) ? &task->cache->ix_pgtab : &task->cache->db_pgtab;
    LOOKUP_ENTRY *lu_ptr;
    short bucket;

#ifdef DB_DEBUG
    if (pg_ptr <  pgtab->pg_table
     || pg_ptr >= pgtab->pg_table + pgtab->pgtab_sz)
    {
        dberr(SYS_INVPAGE);
        return;
    }
#endif

    bucket = LU_HASH(file, pg_ptr->pageno, pgtab->lookup_sz);
    lu_ptr = &pgtab->lookup[bucket];
    pg_ptr->next   = *lu_ptr;
    *lu_ptr        = pg_ptr;
    if (pg_ptr->next)
        pg_ptr->next->prev = pg_ptr;
}

/* ======================================================================
    Clear a page from the cache
*/
int INTERNAL_FCN dio_clrpage(PAGE_ENTRY *pg_ptr, DB_TASK *task)
{
    dio_unhash(pg_ptr, task);
    if (pg_ptr->file != task->ov_file)
    {
        if (pg_ptr->n_pg)
            pg_ptr->n_pg->p_pg = pg_ptr->p_pg;

        if (pg_ptr->p_pg)
            pg_ptr->p_pg->n_pg = pg_ptr->n_pg;
        else
            task->pgzero[pg_ptr->file].pg_ptr = pg_ptr->n_pg;
        pg_ptr->n_pg = pg_ptr->p_pg = NULL;
    }
    STAT_pages(pg_ptr->file, -1, task);

    pg_ptr->file = -1;
    pg_ptr->modified = 0;
    pg_ptr->holdcnt = 0;
    pg_ptr->ovfl_addr = 0L;
    pg_ptr->recently_used = 0;

    if (task->cache->tag_table[pg_ptr->tag].num_pages < 1)
        return (dberr(SYS_INVPGCOUNT));
    --task->cache->tag_table[pg_ptr->tag].num_pages;
    return (task->db_status);
}

/* ======================================================================
    Return a pointer to the buffer of a requested page
*/
int EXTERNAL_FCN dio_getpg(
    FILE_NO         file,       /* file table index */
    F_ADDR          page,       /* page in file */
    short           pg_tag,
    PAGE_ENTRY    **last_lu,    /* the last looked-up page */
    PAGE_ENTRY    **xpg_ptr,
    DB_TASK *task)
{
    PAGE_TABLE *pgtab = (file == task->ov_file) ? &task->cache->ix_pgtab : &task->cache->db_pgtab;
    PAGE_TAG   *tag_ptr;
    PAGE_ENTRY *pg_ptr;
    short       pgsize;
    int         retcode;
    F_ADDR      ovfl_addr;
    
    retcode = task->db_status;

    tag_ptr = &task->cache->tag_table[pg_tag];
    if (tag_ptr->lookups > LONG_MAX)
    {
        /* prevent roll-over while keeping proportions */
        tag_ptr->lookups >>= 1;
        tag_ptr->hits    >>= 1;
    }
    tag_ptr->lookups++;
    STAT_lookups(file, task);

#ifdef DB_DEBUG
    if (task->db_debug & LOCK_CHECK)
    {
        /* make sure that a locked file has not been modified by someone else
        */
        if (file != task->ov_file && !(task->file_table[file].ft_flags & STATIC) &&
            task->dbopen == 1 && (task->app_locks[file] || task->excl_locks[file]))
        {
            if (task->pgzero[file].pz_next)
            {
                if (dio_open(file, task) != S_OKAY)
                    return (task->db_status);
                if (task->pgzero[file].file_mtime != psp_fileModTime(task->file_table[file].ft_desc))
                    retcode = dberr(SYS_FILEMODIFIED);

                if (retcode)
                    return (task->db_status = retcode);
            }
        }
    }
#endif /* DB_DEBUG */

    /* look in the cache */
    if (dio_findpg(file, page, last_lu, xpg_ptr, task) == S_OKAY)
    {
        /* found the page in the cache */
#ifdef DB_DEBUG
        if (task->db_debug & LOCK_CHECK)
        {
            /* make sure that a locked page gotten from the cache has not been
               modified by someone else in the meantime
            */
            if (file != task->ov_file && !(task->file_table[file].ft_flags & STATIC) &&
                task->dbopen == 1 && (task->app_locks[file] || task->excl_locks[file]))
            {
                const SG *sg;
                long      ptime = 0L;
                off_t     addr = task->file_table[file].ft_pgsize * (off_t)page;

                if ((sg = task->sgs[file]) != NULL)
                {
                    off_t  begin;
                    off_t  off;
                    size_t size;

                    block_parms(addr, sizeof(long), &begin, &size, &off, sg);
                    dio_readfile(file, begin, task->enc_buff, size, 0,
                            NULL, task);
                    (*sg->dec)(sg->data, task->enc_buff, size);
                    memcpy(&ptime, (char *) task->enc_buff + off, sizeof(long));
                }
                else
                    dio_readfile(file, addr, &ptime, 4, 0, NULL, task);

                if (memcmp((*xpg_ptr)->buff, (char *)&ptime, 4) != 0)
                    return (dberr(SYS_FILEMODIFIED));
            }
        }
#endif /* DB_DEBUG */
        tag_ptr->hits++;
        STAT_hits(file, task);
        return (task->db_status);
    }

    if (task->db_status == S_NOTFOUND)
        task->db_status = S_OKAY;

    /* page not found -- start of process to read into cache */

    /* #1 -- search the page table for a page not recently used */
    if (dio_findlru(file, page, pg_tag, &pg_ptr, task) != S_OKAY)
        return (task->db_status);
#ifdef DB_DEBUG
    if (pg_ptr <  pgtab->pg_table
     || pg_ptr >= pgtab->pg_table + pgtab->pgtab_sz)
        return (dberr(SYS_INVPAGE));
#endif

    /* #2 -- write page if dirty */
    if (pg_ptr->modified)
    {
        FILE_ENTRY *file_ptr = &task->file_table[pg_ptr->file];
        if (pg_ptr->file == task->ov_file
          || file_ptr->ft_flags & TEMPORARY
          || (!task->trans_id[0] && (task->dbopen >= 2 || task->excl_locks[pg_ptr->file]))
          || !(task->dboptions & TRLOGGING))
        {
            /* ix page swapping occurs here; also data/key pages when
               (in one-user/exclusive mode or has exclusive locks) and
               (without transactions).
            */
            retcode = dio_out(pg_ptr, task);
            if (pg_ptr->file != task->ov_file)
            {
                /* this will cause the file to be committed at d_close */
                file_ptr->ft_flags |= NEEDS_COMMIT;
            }
        }
        else
        {
            retcode = o_write(pg_ptr, task);
            if (retcode == S_OKAY)
                task->cache_ovfl = TRUE;
        }

        if (retcode == S_OKAY)
            pg_ptr->modified = FALSE;
    }

    /* #3 -- remove the found page from the hash_table old bucket */
    if (retcode == S_OKAY && pg_ptr->file != -1)
        retcode = dio_clrpage(pg_ptr, task);

    if (retcode != S_OKAY)
        goto ret_dbstatus;

    /* old page has now been swapped out, bring in new page */

    /* #4 -- resize the page buffer if necessary */
    pgsize = task->file_table[file].ft_pgsize;
    if (!task->cache->prealloc)
    {
        if ((pgsize != pg_ptr->buff_size) || (pg_ptr->buff == NULL))
        {
            /* The realloc for a smaller page allows for the cache to shrink
               when the db needing the larger pages is d_iclose()-ed.  If this
               is not done, the d_iopen() would make the cache grow without the
               possibility of it ever shrinking.
            */
            char *tempbuff = psp_cGetMemory(pgsize, 0);
            if (! tempbuff)
                return (dberr(S_NOMEMORY));

            if (pg_ptr->buff)
                psp_freeMemory(pg_ptr->buff, 0);

            pg_ptr->buff = tempbuff;
#ifdef DBSTAT
            STAT_mem_alloc(pgtab, pgsize - pg_ptr->buff_size);
#endif
        }
    }
    pg_ptr->buff_size = pgsize;

    /* #5 -- check to see if page is in overflow file */
    ovfl_addr = 0L;
    if (file == task->ov_file)
    {
        ovfl_addr = page;
    }
    else if (task->cache_ovfl)
    {
        retcode = o_search(file, page, &ovfl_addr, task);
        if (retcode != S_OKAY)
            goto ret_dbstatus;    /* o_search failed, exit function */
    }

    /* #6 -- this is now the one we are looking for, setup for dio_in() */
    pg_ptr->file = file;
    pg_ptr->pageno = page;
    pg_ptr->ovfl_addr = ovfl_addr;

    /* #7 -- read the page from disk */
    if ((retcode = dio_in(pg_ptr, task)) == S_OKAY)
    {
        /* #8 -- add pg_ptr into hash_table in new bucket, front of the chain */
        dio_rehash(pg_ptr, task);
        pg_ptr->tag = pg_tag;

        if (file != task->ov_file)
        {
            pg_ptr->n_pg = task->pgzero[file].pg_ptr;
            task->pgzero[file].pg_ptr = pg_ptr;
            if (pg_ptr->n_pg)
                pg_ptr->n_pg->p_pg = pg_ptr;
        }
        ++task->cache->tag_table[pg_tag].num_pages;

        STAT_pages(file, 1, task);

        if (last_lu) *last_lu = pg_ptr;
        if (xpg_ptr) *xpg_ptr = pg_ptr;
    }

#ifdef DB_DEBUG
    if (task->db_debug & CACHE_CHECK)
    {
        if (check_cache(DB_TEXT("after dio_getpg"), task) != S_OKAY)
            retcode = task->db_status;
    }
#endif

ret_dbstatus:
    return (task->db_status = retcode);
}

/* ======================================================================
*/
int INTERNAL_FCN dio_out(
    PAGE_ENTRY *pg_ptr,       /* page table entry to be output */
    DB_TASK    *task)
{
    FILE_NO   fno;                /* file number */
    short     pgsize;             /* size of page */
    off_t     addr;               /* file address */
    long      curr_time;
    void     *buff;
    const SG *sg;
#ifdef DB_DEBUG
    DB_ADDR   page_id;
#endif

    if (task->dboptions & READONLY)
        return (dberr(S_READONLY));

    if (pg_ptr->buff == NULL)
        return (dberr(S_INVPTR));

    fno = pg_ptr->file;
    pgsize = task->file_table[fno].ft_pgsize;
#ifdef DB_DEBUG
    if (pg_ptr->buff_size != pgsize)
        return (dberr(SYS_INVPGSIZE));
#endif

    if (pg_ptr->ovfl_addr == 0L)
    {
        /* write to database file */
        if (pg_ptr->pageno == 0)
            return (dberr(SYS_PZACCESS));

#ifdef DB_DEBUG
        if (task->db_debug & PAGE_CHECK)
        {
            ENCODE_DBA(min(fno, 255), pg_ptr->pageno, &page_id);
            if (memcmp(pg_ptr->buff, &page_id, sizeof(DB_ADDR)) != 0)
                return (dberr(SYS_BADPAGE));
        }
        else if (task->db_debug & LOCK_CHECK)
        {
            /* make sure that a locked page has not been modified by someone
               else in the meantime
            */
            if (fno != task->ov_file && !(task->file_table[fno].ft_flags & STATIC) &&
                task->dbopen == 1 && (task->app_locks[fno] || task->excl_locks[fno]))
            {
                long ptime = 0L;
                addr = pgsize * (off_t)pg_ptr->pageno;
                if ((sg = task->sgs[fno]) != NULL)
                {
                    off_t  begin;
                    off_t  off;
                    size_t size;

                    block_parms(addr, sizeof(long), &begin, &size, &off, sg);
                    dio_readfile(fno, begin, task->enc_buff, size, 0,
                            NULL, task);
                    (*sg->dec)(sg->data, task->enc_buff, size);
                    memcpy(&ptime, (char *) task->enc_buff, sizeof(long));
                }
                else
                {
                    dio_readfile(fno, addr, &ptime, sizeof(long), 0,
                            NULL, task);
                }

                if (memcmp(pg_ptr->buff, &ptime, 4) != 0)
                    return (dberr(SYS_FILEMODIFIED));
            }
        }
#endif /* DB_DEBUG */

        curr_time = psp_timeSecs();             /* get the current time */
        memcpy(pg_ptr->buff, &curr_time, sizeof(time_t));
        addr = pgsize * (off_t)pg_ptr->pageno;
    }
    else
    {
        /* write to overflow file */
        fno = task->ov_file;
        addr = pg_ptr->ovfl_addr;
    }

    if ((sg = task->sgs[pg_ptr->file]) != NULL)
    {
        buff = task->enc_buff;
        memcpy(buff, pg_ptr->buff, pgsize);
        (*sg->enc)(sg->data, buff, pgsize);
    }
    else
        buff = pg_ptr->buff;

    dio_writefile(fno, addr, buff, pgsize, 0, task);

#ifdef DB_DEBUG
    if (task->db_debug & PAGE_CHECK)
    {
        if (fno != task->ov_file)
        {
            /* put page id back in header */
            memcpy(pg_ptr->buff, &page_id, sizeof(DB_ADDR));
        }
    }
#endif

    if (task->db_status == S_OKAY)
    {
        if (fno != task->ov_file)
            STAT_pg_write(fno, pgsize, task);
        else
            STAT_log_write(pgsize, task);
    }

    return (task->db_status);
}


/* ======================================================================
    Read in a page to the buffer
*/
int INTERNAL_FCN dio_in(
    PAGE_ENTRY *pg_ptr,
    DB_TASK *task)
{
    FILE_NO   fno;           /* file number */
    short     pgsize;        /* page size */
    off_t     addr;          /* file address */
    int       extended;
    DB_ULONG  extend = EXTENDFILES;
    const SG *sg;

    if (pg_ptr->buff == NULL)
        return (dberr(S_INVPTR));

    fno = pg_ptr->file;
    pgsize = task->file_table[fno].ft_pgsize;

    if (pg_ptr->buff_size != pgsize)
        return (dberr(SYS_INVPGSIZE));
    
    if (pg_ptr->ovfl_addr == 0L)
    {
        /* read from database file */
        if (pg_ptr->pageno == 0)
            return (dberr(SYS_PZACCESS));

        addr = pgsize * (off_t)pg_ptr->pageno;

        /* suppress file extend logic if called from dbCheck */
        if (task->dboptions & NORECOVER)
            extend = 0;
#ifdef DB_DEBUG
        else
        {
            /* suppress file extend logic if not at end of file */
            if (pg_ptr->pageno < dio_pages(fno, task))
                extend = 0;
        }
#endif
    }
    else
    {
        /* read from overflow file */
        fno = task->ov_file;
        addr = pg_ptr->ovfl_addr;
    }

    if ((sg = task->sgs[fno]) != NULL)
    {
        dio_readfile(fno, addr, task->enc_buff, pgsize, extend,
                &extended, task);
        (*sg->dec)(sg->data, task->enc_buff, pgsize);
        memcpy(pg_ptr->buff, task->enc_buff, pgsize);
    }
    else
    {
        dio_readfile(fno, addr, pg_ptr->buff, pgsize, extend,
                &extended, task);
    }

#ifdef DB_DEBUG
    if (extended && !extend && !(task->dboptions & NORECOVER))
    {
        /* report the error if not dbCheck */
        return (dberr(SYS_INVEXTEND));
    }

    if (task->db_status == S_OKAY && (task->db_debug & PAGE_CHECK))
    {
        if (fno != task->ov_file)
        {
            DB_ADDR page_id;
            ENCODE_DBA(min(fno, 255), pg_ptr->pageno, &page_id);
            memcpy(pg_ptr->buff, &page_id, sizeof(DB_ADDR));
        }
    }
#endif

    if (task->db_status == S_OKAY)
    {
        if (fno != task->ov_file)
        {
            if (extended)
            {
                STAT_pg_write(fno, pgsize, task);
                STAT_new_page(fno, task);
            }
            else
                STAT_pg_read(fno, pgsize, task);
        }
        else
            STAT_log_read(pgsize, task);
    }

    return (task->db_status);
}


/*---------------------------------------------------------------------/
/----------------------------------------------------------------------/
    Page zero handling functions for data and key files
/----------------------------------------------------------------------/
/---------------------------------------------------------------------*/

/* ======================================================================
    Initialize page zero table
*/
int EXTERNAL_FCN dio_pzinit(DB_TASK *task)
{
    register FILE_NO i;
    PGZERO          *pgzero_ptr;
    size_t           size;
    size_t           old;

    size = task->size_ft * sizeof(PGZERO);
    old  = task->old_size_ft * sizeof(PGZERO);

    if (alloc_table((void **) &task->pgzero, size, old, task) != S_OKAY)
        return (task->db_status);

    /* read in page zeros */
    for (i = task->old_size_ft, pgzero_ptr = &task->pgzero[task->old_size_ft];
         i < task->size_ft; ++i, ++pgzero_ptr)
        memset(pgzero_ptr, '\0', sizeof(PGZERO));

    return (task->db_status);
}

/* ======================================================================
    Free the memory allocated for page zeros
*/
static void INTERNAL_FCN dio_pzfree(int dbn, DB_TASK *task)
{
    int low, high, total;

    if (dbn == ALL_DBS)
    {
        low = 0;
        high = task->size_ft;
        total = task->size_ft;
    }
    else
    {
        low = task->db_table[dbn].ft_offset;
        high = low + task->db_table[dbn].Size_ft;
        total = task->size_ft;
    }

    if (task->pgzero) {
        free_table((void **) &task->pgzero, low, high, total, sizeof(PGZERO),
                task);
    }
}


/* ======================================================================
    Increment and return file timestamp
*/
DB_ULONG INTERNAL_FCN dio_pzsetts(FILE_NO fno, DB_TASK *task)
{
    DB_ULONG    ts;
    PGZERO     *pgzero_ptr;

    if (task->db_tsrecs || task->db_tssets)
    {
        if ((task->pgzero[fno].pz_next == (F_ADDR) 0
          &&  dio_pzread(fno, task) != S_OKAY)
#ifdef DB_DEBUG
          || dio_pzcheck(fno, task) != S_OKAY
#endif
          )
            return 0L;

        pgzero_ptr = &task->pgzero[fno];
        if (++pgzero_ptr->pz_timestamp == 0L)
            pgzero_ptr->pz_timestamp = 1L;
        pgzero_ptr->pz_modified = TRUE;
        task->file_table[fno].ft_flags |= TS_IS_CURRENT;

        ts = pgzero_ptr->pz_timestamp;
    }
    else
    {
        ts = 0L;
    }

    return (ts);
}


/* ======================================================================
    Return file timestamp
*/
DB_ULONG INTERNAL_FCN dio_pzgetts(FILE_NO fno, DB_TASK *task)
{
    PGZERO    *pgzero_ptr;

    pgzero_ptr = &task->pgzero[fno];
    
    if ((task->pgzero[fno].pz_next == (F_ADDR) 0
      &&  dio_pzread(fno, task) != S_OKAY)
#ifdef DB_DEBUG
      || dio_pzcheck(fno, task) != S_OKAY
#endif
      )
        return 0L;

    if (task->trans_id[0])
        o_fileinit(fno, task);

    if (! (task->file_table[fno].ft_flags & TS_IS_CURRENT))
        dio_pzsetts(fno, task);
    return (pgzero_ptr->pz_timestamp);
}


/* ======================================================================
    Flush page zero table
*/
static int INTERNAL_FCN dio_pzflush(DB_TASK *task)
{
    register FILE_NO       i;
    register PGZERO       *pgzero_ptr;
    register FILE_ENTRY   *file_ptr;
    void                  *buff;
    const SG              *sg;

    if ((task->dboptions & TRLOGGING) && task->trans_id[0] && (! task->trcommit))
    {
        /* flush to overflow/log file -- before tx commit */
        for (i = 0, pgzero_ptr = task->pgzero, file_ptr = task->file_table;
             i < task->size_ft;
             ++i, ++pgzero_ptr, ++file_ptr)
        {
            if (pgzero_ptr->pz_modified && !(file_ptr->ft_flags & TEMPORARY))
            {
#ifdef DB_DEBUG
                if (dio_pzcheck(i, task) != S_OKAY)
                    return (task->db_status);
#endif
                if (o_pzwrite(i, task) != S_OKAY)
                    return (task->db_status);
            }
        }
    }
    else
    {
        /* flush modified page zeroes to database files */
        for (i = 0, pgzero_ptr = task->pgzero, file_ptr = task->file_table;
                i < task->size_ft; ++i, ++pgzero_ptr, ++file_ptr)
        {
            if (pgzero_ptr->pz_modified && !(file_ptr->ft_flags & TEMPORARY))
            {
#ifdef DB_DEBUG
                if (dio_pzcheck(i, task) != S_OKAY)
                    return (task->db_status);
#endif
                if ((sg = task->sgs[i]) != NULL)
                {
                    buff = task->enc_buff;
                    memcpy(buff, pgzero_ptr, PGZEROSZ);
                    (*sg->enc)(sg->data, buff, PGZEROSZ);
                }
                else
                    buff = pgzero_ptr;

                if (dio_writefile(i, 0L, buff, PGZEROSZ, 0, task) != S_OKAY)
                    return (task->db_status);

                STAT_pz_write(i, PGZEROSZ, task);
                
                pgzero_ptr->pz_modified = FALSE;
                if (task->trlog_flag)
                    dtrlog(i, 0L, (char *) pgzero_ptr, PGZEROSZ, task);

                /* Since the file is getting committed immediately,
                    there is no need to set the NEEDS_COMMIT flag.
                */
                commit_file(task->file_table[i].ft_desc, task);
                task->file_table[i].ft_flags &= ~(NEEDS_COMMIT | TS_IS_CURRENT);
            }
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Read a file's page zero
*/
int EXTERNAL_FCN dio_pzread(FILE_NO fno, DB_TASK *task)
{
    PGZERO   *pgzero_ptr = &task->pgzero[fno];
    const SG *sg;
    PSP_FH    desc;

    if (dio_open(fno, task) != S_OKAY)
        return task->db_status;
        
    desc = task->file_table[fno].ft_desc;

    if (psp_fileSeekRead(desc, 0, pgzero_ptr, PGZEROSZ) < (int) PGZEROSZ)
    {
        dberr(S_BADREAD);
        pgzero_ptr->pz_dchain = (F_ADDR) 0;
        pgzero_ptr->pz_next = (F_ADDR) 0;
        pgzero_ptr->pz_timestamp = 0L;
        pgzero_ptr->pz_modified = FALSE;
        task->file_table[fno].ft_flags &= (~TS_IS_CURRENT);
        goto exit;
    }

    if ((sg = task->sgs[fno]) != NULL)
        (*sg->dec)(sg->data, pgzero_ptr, PGZEROSZ);

    STAT_pz_read(fno, PGZEROSZ, task);

#ifdef DB_DEBUG
    if (task->db_debug & PZVERIFY)
    {
        pgzero_ptr->file_size = psp_fileLength(task->file_table[fno].ft_desc);
        if (pgzero_ptr->file_size != -1L)
        {
            long pz_file_size;
            FILE_ENTRY *file_ptr = &task->file_table[fno];

            /* round the size down to the nearest page boundary */
            pgzero_ptr->file_size -= (pgzero_ptr->file_size % file_ptr->ft_pgsize);

            /* calculate the size that we'd expect from pz_next */
            pz_file_size  = pgzero_ptr->pz_next - 1;  /* number of slots */
            pz_file_size += file_ptr->ft_slots - 1;   /* round up to next page */
            pz_file_size /= file_ptr->ft_slots;       /* number of pages */
            pz_file_size++;                           /* add 1 for page 0 */
            pz_file_size *= file_ptr->ft_pgsize;      /* number of bytes */

            /*
                The actual file size may be larger than we would expect from 
                pz_next, as there may be some newly created records that were
                written to the end of the file before a transaction was aborted.
                The actual size should not be less than the value calculated
                from pz_next, but it seems that psp_fileLength sometimes returns
                a value that is not current, so allow some error.
            */
            if (pz_file_size > pgzero_ptr->file_size)
            {
                if ((pz_file_size - pgzero_ptr->file_size)
                      > (32 * file_ptr->ft_pgsize))
                {
                    /* more than 32 pages - pznext probably corrupted */
                    return (dberr(SYS_PZNEXT));
                }

                /* assume value returned by psp_fileLength is wrong */
                pgzero_ptr->file_size = pz_file_size;
            }
        }
    }
#endif

exit:
#ifdef DB_DEBUG
    if (task->db_debug & LOCK_CHECK)
        pgzero_ptr->file_mtime = psp_fileModTime(desc);
#endif

    return (task->db_status);
}



/* ======================================================================
    Allocate new record slot or key node from page zero
*/
int EXTERNAL_FCN dio_pzalloc(
    FILE_NO  fno,              /* file number */
    F_ADDR  *loc,              /* pointer to allocated location */
    DB_TASK *task)
{
    DB_ADDR    dba;
    DB_ULONG   pg;
    char      *ptr;
    PGZERO    *pgzero_ptr;

    /* check shared access privileges */
    if ((task->dbopen == 1) && !task->trans_id[0] && !task->excl_locks[fno])
        return (dberr(S_NOTRANS));

    pgzero_ptr = &task->pgzero[fno];
    if ((pgzero_ptr->pz_next == (F_ADDR) 0
        && dio_pzread(fno, task) != S_OKAY)
#ifdef DB_DEBUG
        || dio_pzcheck(fno, task) != S_OKAY
#endif
      )
        return (task->db_status);
    
    if (task->trans_id[0] && o_fileinit(fno, task) != S_OKAY)
        return (task->db_status);
    
    if (task->file_table[fno].ft_type == KEY)
    {
        if ((pgzero_ptr->pz_dchain == DCH_NONE) || (! (task->dboptions & DCHAINUSE)))
        {
            if (pgzero_ptr->pz_next >= MAXRECORDS - 1)
                return (dberr(S_RECLIMIT));
            pg = pgzero_ptr->pz_next++;
        }
        else
        {
            pg = pgzero_ptr->pz_dchain;
            if (dio_get(fno, pg, &ptr, NOPGHOLD, task) != S_OKAY)
                return (task->db_status);
            memcpy(&pgzero_ptr->pz_dchain,
                ptr + sizeof(long) + sizeof(short), sizeof(F_ADDR));
            if (dio_unget(fno, pg, NOPGFREE, task) != S_OKAY)
                return (task->db_status);
        }
    }
    else
    {
        if ((! pgzero_ptr->pz_dchain) || (! (task->dboptions & DCHAINUSE)))
        {
            if (pgzero_ptr->pz_next >= MAXRECORDS)
                return (dberr(S_RECLIMIT));
            pg = pgzero_ptr->pz_next++;
        }
        else
        {
            pg = pgzero_ptr->pz_dchain;
            ENCODE_DBA(NUM2EXT(fno, ft_offset), pg, &dba);
            if (dio_read(dba, (char **) &ptr,
                         NOPGHOLD, task) != S_OKAY)
                return (task->db_status);
            memcpy(&pgzero_ptr->pz_dchain,
                ptr + sizeof(short), sizeof(F_ADDR));
            if (dio_release(dba, NOPGFREE, task) != S_OKAY)
                return (task->db_status);
        }
    }
    *loc = pg;
    pgzero_ptr->pz_modified = TRUE;
    return (task->db_status);
}


/* ======================================================================
    Delete record slot or key node from page zero
*/
int EXTERNAL_FCN dio_pzdel(
    FILE_NO fno,                           /* file number */
    F_ADDR  loc,                            /* location to be freed */
    DB_TASK *task)
{
    DB_ADDR       dba;
    short         recnum;
    char         *ptr;
    PGZERO       *pgzero_ptr;

    /* check shared access privileges */
    if ((task->dbopen == 1) && !task->trans_id[0] && !task->excl_locks[fno])
        return (dberr(S_NOTRANS));

    pgzero_ptr = &task->pgzero[fno];
    if ((pgzero_ptr->pz_next == (F_ADDR) 0
        && dio_pzread(fno, task) != S_OKAY)
#ifdef DB_DEBUG
        || dio_pzcheck(fno, task) != S_OKAY
#endif
      )
        return (task->db_status);

    if (task->file_table[fno].ft_type == KEY)
    {
        if (dio_get(fno, loc, &ptr, PGHOLD, task) != S_OKAY)
            return (task->db_status);
        memcpy(ptr + sizeof(long) + sizeof(short),
            &pgzero_ptr->pz_dchain, sizeof(F_ADDR));
        pgzero_ptr->pz_dchain = loc;
        if (dio_touch(fno, loc, PGFREE, task) != S_OKAY)
            return (task->db_status);
    }
    else
    {
        ENCODE_DBA(NUM2EXT(fno, ft_offset), loc, &dba);
        if (dio_read(dba, &ptr, PGHOLD, task) != S_OKAY)
            return (task->db_status);
        memcpy(&recnum, ptr, sizeof(short));
        recnum &= ~RLBMASK;              /* in case the RLB is set [624] */
        recnum = (short) ~recnum;     /* indicates deleted record */
        memcpy(ptr, &recnum, sizeof(short));
        memcpy(ptr + sizeof(short), &pgzero_ptr->pz_dchain, sizeof(F_ADDR));
        pgzero_ptr->pz_dchain = loc;
        if (dio_write(dba, PGFREE, task) != S_OKAY)
            return (task->db_status);
    }
    pgzero_ptr->pz_modified = TRUE;
    return (task->db_status);
}

/* ======================================================================
    Return pz_next for file fno
*/
F_ADDR EXTERNAL_FCN dio_pznext(FILE_NO fno, DB_TASK *task)
{
    if ((task->pgzero[fno].pz_next == (F_ADDR) 0
        && dio_pzread(fno, task) != S_OKAY)
#ifdef DB_DEBUG
        || dio_pzcheck(fno, task) != S_OKAY
#endif
      )
        return (F_ADDR) 0;
    
    return (task->pgzero[fno].pz_next);
}

/* ======================================================================
    Return last page for file fno
*/
F_ADDR INTERNAL_FCN dio_pages(FILE_NO fno, DB_TASK *task)
{
    F_ADDR last_page;  /* Last page used in file */
    F_ADDR last_slot;  /* Last slot allocated in a file */

    if (task->pgzero[fno].pz_next == (F_ADDR) 0)
    {
        if (dio_pzread(fno, task) != S_OKAY)
            return 0;
    }
    last_slot = task->pgzero[fno].pz_next - 1L;

    if (task->file_table[fno].ft_type == DATA)
    {
        if (last_slot <= 0)
            last_page = (F_ADDR) 0;
        else
            last_page = ((last_slot - 1L) / task->file_table[fno].ft_slots) + 1L;
    }
    else
    {
        last_page = last_slot;
    }
    return last_page;
}

/* ======================================================================
    Clear page zero cache
*/
void INTERNAL_FCN dio_pzclr(DB_TASK *task)
{
    register FILE_NO           i;
    register PGZERO    *pgzero_ptr;

    for (i = 0, pgzero_ptr = task->pgzero; i < task->size_ft; i++, pgzero_ptr++)
    {
        if (pgzero_ptr->pz_modified)
        {
            pgzero_ptr->pz_next = (F_ADDR) 0;
            pgzero_ptr->pz_modified = FALSE;
        }
    }
    return;
}

#ifdef DB_DEBUG

/* ======================================================================
    Validate page zero values
*/
int INTERNAL_FCN dio_pzcheck(FILE_NO fno, DB_TASK *task)
{
    if (task->db_debug & PZVERIFY)
    {
        PGZERO *pgzero_ptr = &task->pgzero[fno];
        FILE_ENTRY *file_ptr = &task->file_table[fno];
        off_t size;
        F_ADDR pages, last_page;

        if (pgzero_ptr->pz_next == (F_ADDR) 0)
            return (task->db_status);

        size = dio_filesize(fno, task);
        if (size == -1)
            return S_OKAY;
        if (task->db_status != S_OKAY)
            return (task->db_status);
        pages = (F_ADDR)(size / file_ptr->ft_pgsize - 1);

        /* validate pz_next */
        last_page = dio_pages(fno, task);
            /* note that we can't check for last_page < pages until d_trabort
               truncates new pages */
        if (/* last_page < pages || */ last_page > pages+1)
            return (dberr(SYS_PZNEXT));

        /* validate dchain */
        if (pgzero_ptr->pz_dchain != DCH_NONE &&
            pgzero_ptr->pz_dchain >= pgzero_ptr->pz_next)
            return (dberr(SYS_DCHAIN));
    }

    return (task->db_status);
}

/* ======================================================================
    Return actual length of file in pages
*/
off_t INTERNAL_FCN dio_filesize(FILE_NO fno, DB_TASK *task)
{
    PGZERO *pgzero_ptr = &task->pgzero[fno];

    if (pgzero_ptr->pz_next == (F_ADDR) 0)
    {
        if (dio_pzread(fno, task) != S_OKAY)
            return 0;
    }

    return pgzero_ptr->file_size;
}

#endif

/* ======================================================================
    Read from a file
*/
int EXTERNAL_FCN dio_readfile(
    FILE_NO  fno,
    off_t    addr,
    void    *buf,
    size_t   len,
    DB_ULONG flags,     /* EXTENDFILES */
    int     *extend,
    DB_TASK *task)
{
    FILE_ENTRY *file_ptr = &task->file_table[fno];
    PSP_FH      desc;
    int         bytes;
    int         stat = S_OKAY;

    if (extend)
        *extend = 0;

    if (dio_open(fno, task) != S_OKAY)
        return (task->db_status);

    desc = file_ptr->ft_desc;

    if (stat == S_OKAY)
    {
        if ((bytes = psp_fileSeekRead(desc, addr, buf, len)) < (int) len)
        {
            if (bytes == 0)
            {
                /* We are extending the file */
                if (task->dboptions & READONLY)
                    stat = dberr(S_READONLY);

                if (extend)
                    *extend = 0;

                if (flags & EXTENDFILES)
                {
                    memset(buf, 0, len);
                    if (psp_fileSeekWrite(desc, addr, buf, len) < (int) len)
                    {
                        if (file_ptr->ft_type != 'o')
                        {
                            /* clean up and return out of space */
                            PGZERO *pgzero_ptr = &task->pgzero[fno];
                            pgzero_ptr->pz_next--;
                            pgzero_ptr->pz_modified = TRUE;
                            dio_pzflush(task);
                        }

                        dberr(S_BADWRITE);
                    }
#ifdef DB_DEBUG
                    else if (task->db_debug & PZVERIFY && file_ptr->ft_type != 'o')
                        task->pgzero[fno].file_size = addr + len;

                    if (task->db_debug & LOCK_CHECK && file_ptr->ft_type != 'o')
                        task->pgzero[fno].file_mtime = psp_fileModTime(desc);
#endif
                }
                else
                {
                    /* dbCheck wants this to not be reported as an error */
                    stat = S_OKAY;
                }
            }
            else
            {
                if (file_ptr->ft_type == 'o')
                    dberr(S_LOGIO);
                else
                    dberr(S_BADREAD);
            }
        }
    }

    return (task->db_status = stat);
}


/* ======================================================================
    Write to a file
*/
int EXTERNAL_FCN dio_writefile(
    FILE_NO  fno,
    off_t    addr,
    void    *buf,
    size_t   len,
    DB_ULONG flags,
    DB_TASK *task)
{
    FILE_ENTRY *file_ptr = &task->file_table[fno];
    PSP_FH      desc;
    int         stat = S_OKAY;
#ifdef DB_DEBUG
    int         is_db_file = (file_ptr->ft_type != 'o');
    off_t       size;
#endif

    if (task->dboptions & READONLY)
        return (dberr(S_READONLY));

    if (dio_open(fno, task) != S_OKAY)
        return (task->db_status);
    desc = file_ptr->ft_desc;

#ifdef DB_DEBUG
    if ((task->db_debug & PZVERIFY) && is_db_file)
    {
        size = dio_filesize(fno, task);
        if (size != -1 && addr + (off_t)len > size)
            stat = dberr(SYS_EOF);
    }
#endif

    if (stat == S_OKAY)
    {
        if (psp_fileSeekWrite(desc, addr, buf, len) < (int) len)
        {
            if (file_ptr->ft_type == 'o')
                dberr(S_LOGIO);
            else
                dberr(S_BADWRITE);
        }
    }

#ifdef DB_DEBUG
    if (task->db_debug & LOCK_CHECK && is_db_file)
        task->pgzero[fno].file_mtime = psp_fileModTime(desc);
#endif

    return (task->db_status = stat);
}


/**************************************************************************/

static int INTERNAL_FCN isprime(long x)
{
    long  prime;
    long  divisor;

    /* even numbers are not prime */
    prime = (x > 1L) && ((x == 2L) || (x & 1L));
    
    divisor = 3;
    if (prime)
    {
        /* It might be quicker to replace the "(divisor * divisor <= x)"
            with a single call to "sroot = sqrt() + 1" and use
            "(divisor < sroot)" in the while condition.  However, the
            call to sqrt() will bring in the floating point libs; and we
            do not need them for this.  And since MAX_PRIME_SHORT is set
            to 32749, 'divisor' will only go from 3 to 181 by twos for a
            maximum of 89 times thru the loop.
        */
        while (prime && (divisor * divisor <= x))
        {
            prime = x % divisor;
            divisor += 2L;       /* check only the odd numbers */
        }
    }

    prime = ! prime;           /* get around any screwy optimization */
    return ((int) (! prime));
}



#ifdef DB_DEBUG

int check_pages(PAGE_TABLE *pgtab, DB_TCHAR *msg, DB_TASK *task)
{
    short bucket, pg;
    LOOKUP_ENTRY *lu_ptr;
    PAGE_ENTRY *pg_ptr;
    FILE_NO fno;
    int data = 0, key = 0, ix = 0;
    DB_TCHAR *what;
    
    what = (pgtab == &task->cache->db_pgtab ? DB_TEXT("db_cache") : DB_TEXT("ix_cache"));

    if (!pgtab || !pgtab->pg_table)    /* not yet initialized */
        return (task->db_status);

    /* validate the hash lookup table */
    for (bucket = 0, lu_ptr = pgtab->lookup; bucket < pgtab->lookup_sz; ++bucket, ++lu_ptr)
    {
        if (*lu_ptr)
        {
            pg_ptr = *lu_ptr;
            if (pg_ptr <  pgtab->pg_table ||
                pg_ptr >= pgtab->pg_table + pgtab->pgtab_sz)
                goto syserr;

            /* validate hash */
            if (bucket != LU_HASH(pg_ptr->file, pg_ptr->pageno, pgtab->lookup_sz))
                goto syserr;

            if (pg_ptr->prev != NULL && pg_ptr->prev == pg_ptr ||
                pg_ptr->prev->next != pg_ptr)
                goto syserr;

            if (pg_ptr->next != NULL && pg_ptr->next == pg_ptr ||
                pg_ptr->next->prev != pg_ptr)
                goto syserr;
        }
    }

    /* validate the cache pages */
    for (pg = 0, pg_ptr = pgtab->pg_table; pg < pgtab->pgtab_sz; ++pg, ++pg_ptr)
    {
        fno = pg_ptr->file;
        if (fno == -1)
            continue;

        if (fno < 0 || fno > task->size_ft)
            goto syserr;

        /* ix-cache should be empty if no transaction active */
        if (fno == task->ov_file && !task->trans_id[0])
            goto syserr;

        if (!pg_ptr->buff || pg_ptr->buff_size != task->file_table[fno].ft_pgsize)
            goto syserr;

        if (fno == task->ov_file)
        {
            if (pg_ptr->pageno != pg_ptr->ovfl_addr)
                goto syserr;

            ++ix;
        }
        else if (task->file_table[fno].ft_type == 'd')
            ++data;
        else
            ++key;

        if (pg_ptr->prev == NULL)
        {
            bucket = LU_HASH(pg_ptr->file, pg_ptr->pageno, pgtab->lookup_sz);
            if (pgtab->lookup[bucket] != pg_ptr)
                goto syserr;
        }
        else
            if (pg_ptr->prev->next != pg_ptr)
                goto syserr;

        if (pg_ptr->next != NULL)
        {
            if (pg_ptr->next->prev != pg_ptr)
                goto syserr;
        }

        if (fno != task->ov_file)
        {
            if (pg_ptr->p_pg == NULL)
            {
                if (task->pgzero[fno].pg_ptr != pg_ptr)
                    goto syserr;
            }
            else
            {
                if (pg_ptr->p_pg->n_pg != pg_ptr)
                    goto syserr;
            }
        }
    }

    /* validate file pages */
    for (fno = 0; fno < task->size_ft; fno++)
    {
        for (pg_ptr = task->pgzero[fno].pg_ptr; pg_ptr; pg_ptr = pg_ptr->n_pg)
        {
            if (pg_ptr->file != fno)
                goto syserr;
        }
    }

    return (task->db_status);

err:
    flush_dberr(task);    /* show the error right away */
#ifdef DB_TRACE
    if (task->db_trace)
        db_printf(task, DB_TEXT("%s: %s\n"), what, msg);

#endif
    return (task->db_status);

syserr:
    dberr(S_SYSERR);
    goto err;
}

int INTERNAL_FCN check_cache(DB_TCHAR *msg, DB_TASK *task)
{
    if (check_pages(&task->cache->db_pgtab, msg, task) != S_OKAY ||
        check_pages(&task->cache->ix_pgtab, msg, task) != S_OKAY)
        return (task->db_status);

    return S_OKAY;
}

#endif   /* DB_DEBUG */


