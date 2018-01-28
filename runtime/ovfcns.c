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

/*------------------------------------------------------------------------
    contains the following routines:

        o_setup:       Initialize the overflow control data for a database open
        o_init:        Initialize the overflow control data at trx start
        o_fileinit:    Initialize the overflow data that depends on file size
        o_search:      Search the overflow for a specific page
        o_write:       Write a cache page to the overflow
        o_pzwrite:     Write page zero to the overflow file
        o_flush:       Flush the transaction recovery data to the overflow file
        o_update:      Update the database from the data in the overflow file
        o_pages:       Return file's page count at tx start
        o_free:        Free dynamically allocated data
        o_close:       Close the overflow file

------------------------------------------------------------------------*/

#include "db.star.h"

/* Internal function prototypes */
static void INTERNAL_FCN set_ixpbit(int, F_ADDR, DB_TASK *);
static unsigned int INTERNAL_FCN ixp_used(int, F_ADDR, DB_TASK *);
static F_ADDR INTERNAL_FCN oaddr_of_ixp(int, F_ADDR, DB_TASK *);
static void INTERNAL_FCN ov_header_init(DB_TASK *);

/* ======================================================================
    Index page used test
*/
static unsigned int INTERNAL_FCN ixp_used(
    int file,
    F_ADDR ipno,
    DB_TASK *task)
{
    register unsigned int mask;
    RI_ENTRY             *ri_ptr;
    unsigned int          wordpos;

    wordpos = (unsigned int) (ipno / BUI);
    mask = 1 << ((BUI - 1) - (unsigned int) ((ipno - (F_ADDR) wordpos * BUI)));
    ri_ptr = &task->root_ix[file];

    return ri_ptr->ri_bitmap[wordpos] & mask;
}


/* ======================================================================
    set index page used bit
*/
static void INTERNAL_FCN set_ixpbit(int file, F_ADDR ipno, DB_TASK *task)
{
    register unsigned int mask;
    RI_ENTRY             *ri_ptr;
    unsigned int          wordpos;

    wordpos = (unsigned int) (ipno / BUI);
    mask = 1 << ((BUI - 1) - (unsigned int) ((ipno - (F_ADDR) wordpos * BUI)));
    ri_ptr = &task->root_ix[file];
    ri_ptr->ri_bitmap[wordpos] |= mask;
}


/* ======================================================================
    Compute address of index page in overflow file
*/
static F_ADDR INTERNAL_FCN oaddr_of_ixp(
    int file,
    F_ADDR ipno,
    DB_TASK *task)
{
    register RI_ENTRY *ri_ptr;

    ri_ptr = &task->root_ix[file];
    return (ri_ptr->base + RI_BITMAP_SIZE(ri_ptr->pg_cnt) * sizeof(int) +
              ipno * IX_PAGESIZE);
}


/*  ======================================================================
    Initialize the overflow control data for a database open
*/
int EXTERNAL_FCN o_setup(DB_TASK *task)
{
    int           ft_lc;                       /* loop control */
    size_t        new_size;
    size_t        old;
    FILE_ENTRY   *file_ptr;
    RI_ENTRY     *ri_ptr;

    if (!task->ov_setup_done)
    {
        DB_TCHAR *userid = (task->dbuserid[0]) ?  (DB_TCHAR *) task->dbuserid :
                                                  (DB_TCHAR *) DB_TEXT("db.star");

        /* create and initialize log file */
        if (!task->dblog[0])
        {
            vtstrncpy(task->dblog, userid, DBNMLEN);
            task->dblog[DBNMLEN] = 0;
            vtstrcat(task->dblog, DB_TEXT(".log"));
        }
        else
        {
            if (psp_pathIsDir(task->dblog))
            {
                /* directory only, add default file name */
                vtstrncat(task->dblog, userid, DBNMLEN);
                vtstrcat(task->dblog, DB_TEXT(".log"));
            }
        }
    }

    task->ov_file = task->size_ft;

    /* allocate and initialize root index space */
    new_size = task->size_ft * sizeof(RI_ENTRY);
    old =  task->old_size_ft * sizeof(RI_ENTRY);

    if (alloc_table((void **) &task->root_ix, new_size, old, task) != S_OKAY)
        return task->db_status;

    for ( ft_lc = task->size_ft, ri_ptr = &task->root_ix[task->old_size_ft];
          --ft_lc >= task->old_size_ft; ++ri_ptr)
        ri_ptr->ri_bitmap = NULL;

    task->cache_ovfl = FALSE;

    /* initialize the task->file_table entry for the overflow file */
    file_ptr = &task->file_table[task->ov_file];
    vtstrcpy(file_ptr->ft_name, task->dblog);
    file_ptr->ft_status = CLOSED;
    file_ptr->ft_type = DB_OVERFLOW;
    file_ptr->ft_slots = 1;
    file_ptr->ft_slsize = IX_PAGESIZE;
    file_ptr->ft_pgsize = IX_PAGESIZE;
    file_ptr->ft_flags |= NOT_TTS_FILE;
    task->ov_setup_done = TRUE;
    
    /* set the initial useable address in the overflow file */
    ov_header_init(task);

    return (task->db_status);
}


/* ======================================================================
    Initialize overflow/log processing at start of transaction
*/
int INTERNAL_FCN o_init(DB_TASK *task)
{
    PSP_FH               fh;             /* File handle */
    register FILE_NO     ft_lc;            /* loop control */
    register FILE_ENTRY *file_ptr;
             RI_ENTRY   *ri_ptr;
    unsigned long        sav_opt;

    /* Create the log file if necessary.  Truncate it to zero bytes.
       write the needed stuff to it.  Close the file.  The one assumption
       being made here is that the log file is NOT open at this point.
    */

    if (!task->ov_header_written || task->dboptions & TRUNCATELOG)
    {
        /* even with READONLY option set, attempt to creat log file */
        sav_opt = task->dboptions;
        task->dboptions &= ~READONLY;

        /* initialize log file */
        fh = open_b(task->dblog, O_RDWR | O_CREAT | O_TRUNC, 0, task);

        task->dboptions = sav_opt;
        if (fh == NULL)
            return (dberr(S_DBLACCESS));

        STAT_log_open(task);

        /* write file count to overflow file */
        psp_fileSeek(fh, 0);
        if (psp_fileWrite(fh, &task->size_ft, sizeof(short)) < (int) sizeof(short))
            return (dberr(S_BADWRITE));

        STAT_log_write(sizeof(short), task);

        /* write file page sizes and names to overflow file */
        for ( ft_lc = task->size_ft, file_ptr = task->file_table; --ft_lc >= 0; ++file_ptr)
        {
            if (psp_fileWrite(fh, &file_ptr->ft_pgsize,
                    sizeof(short)) < (int) sizeof(short))
            {
                psp_fileClose(fh);
                return (dberr(S_BADWRITE));
            }

            STAT_log_write(sizeof(short), task);

            if (psp_fileWrite(fh, file_ptr->ft_name, FILENMLEN *
                sizeof(DB_TCHAR)) < FILENMLEN * (int) sizeof(DB_TCHAR))
            {
                psp_fileClose(fh);
                return (dberr(S_BADWRITE));
            }

            STAT_log_write(FILENMLEN * sizeof(DB_TCHAR), task);
        }

        psp_fileClose(fh);
        task->ov_header_written = TRUE;    /* set to false by ov_header_init() */
    }

    /* reset start address on log file */
    task->ov_nextaddr = task->ov_initaddr;
    task->cache_ovfl = FALSE;

    /* free any used root index entries */
    for (ft_lc = 0; ft_lc < task->size_ft; ft_lc++)
    {
        ri_ptr = &task->root_ix[ft_lc];

        if (ri_ptr->ri_bitmap)
        {
            psp_freeMemory(ri_ptr->ri_bitmap, 0);
            memset(ri_ptr, '\0', sizeof(*ri_ptr));
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Initialize the overflow data that depends on file size
*/
int INTERNAL_FCN o_fileinit(FILE_NO file, DB_TASK *task)
{
    int       bm_size;    /* Ri_bitmap size in unsigned int units */
    F_ADDR    pz_base;    /* was page zero base address */
    RI_ENTRY *ri_ptr;

    ri_ptr = &task->root_ix[file];
    if (ri_ptr->ri_bitmap == NULL)      /* only initialize once */
    {
        /* fill out the task->root_ix entry for this file */
        ri_ptr->pg_cnt = dio_pages(file, task);   /* Number of pages excluding page 0 */
        if (ri_ptr->pg_cnt <= 0)
            ri_ptr->pg_cnt = (F_ADDR) 1;

        ri_ptr->base = task->ov_nextaddr;
        pz_base = ((F_ADDR) (IX_BASE(file, ri_ptr->pg_cnt) +
                  (IX_SIZE(ri_ptr->pg_cnt) * IX_PAGESIZE)));
        task->ov_nextaddr += (F_ADDR) (pz_base + PGZEROSZ - BM_BASE(file));
        bm_size = RI_BITMAP_SIZE(ri_ptr->pg_cnt);
        ri_ptr->ri_bitmap = (int *) psp_getMemory(bm_size * sizeof(int), 0);
        if (ri_ptr->ri_bitmap == NULL)
            return (dberr(S_NOMEMORY));

        memset(ri_ptr->ri_bitmap, 0, bm_size * sizeof(int));
    }

    return (task->db_status);
}


/* ======================================================================
    Search the overflow for a specific page
*/
int INTERNAL_FCN o_search(
    FILE_NO  file,
    F_ADDR   page,
    F_ADDR  *o_addr,
    DB_TASK *task)
{
    int         ent;        /* index page entry */
    F_ADDR      ipno;       /* Index page of data page */
    F_ADDR      ip_addr;    /* Address of ix page in ovfl */
    F_ADDR     *eptr;       /* Ptr to index page entry */
    PAGE_ENTRY *ixpg_ptr;

    if (page > task->root_ix[file].pg_cnt)
        *o_addr = 0L;                    /* Can't be in overflow */
    else
    {
        /* Search index */
        ipno = (page - 1) / IX_EPP;
        if (ixp_used(file, ipno, task))
        {
            /* Check the index page */
            ip_addr = oaddr_of_ixp(file, ipno, task);

            if (dio_getpg(task->ov_file, ip_addr, task->ix_tag,
                          &task->last_ixpage, &ixpg_ptr, task) == S_OKAY)
            {
                if (ixpg_ptr->buff == NULL)
                    return (dberr(S_INVPTR));

                /* get ovfl address of db page */
                ent = (int) (page - 1 - ipno * IX_EPP);
                eptr = (F_ADDR *) (ixpg_ptr->buff + (sizeof(F_ADDR) * ent));
                *o_addr = *eptr;
                ixpg_ptr->recently_used = TRUE;
            }
        }
        else
            *o_addr = 0L;        /* Not in overflow */
    }

    return task->db_status;
}


/* ======================================================================
    Write a cache page to the overflow
*/
int INTERNAL_FCN o_write(PAGE_ENTRY *pg_ptr, DB_TASK *task)
{
    int         ent;           /* index page entry */
    F_ADDR      ip_addr;       /* Address of index page in ovfl */
    F_ADDR      ipno;          /* Index page number */
    F_ADDR      db_page;       /* Database page # of cache page */
    F_ADDR     *eptr;          /* Ptr to entry in index page */
    FILE_NO     fno;           /* file number */
    PAGE_ENTRY *ixpg_ptr;

    /**********************************************************************

        The cache page will be written in 1 of 3 manners.
        1: if the ovfl_addr of the cache page != 0L, the page is written
            directly to that address in the overflow file.
        2: if the page number to be written is larger than the number of
            pages in the database file, it is written to the end of the data
            base file it belongs to, at the proper address for that page.
        3: otherwise, the page is added to end of the overflow file and
            the index is updated with its address.

    **********************************************************************/
    fno = pg_ptr->file;
    if (pg_ptr->ovfl_addr != 0 || pg_ptr->pageno > task->root_ix[fno].pg_cnt)
    {
        /* write page to end of db file or to overflow file */
        dio_out(pg_ptr, task);
    }
    else
    {
        /* Add page to end of overflow */
        db_page = pg_ptr->pageno;
        ipno = (db_page - 1) / IX_EPP;
        ip_addr = oaddr_of_ixp(fno, ipno, task);

        /* write db page to overflow file and read index page into cache */
        pg_ptr->ovfl_addr = task->ov_nextaddr;
        task->ov_nextaddr += task->file_table[fno].ft_pgsize;
        if (dio_out(pg_ptr, task) != S_OKAY ||
                dio_getpg(task->ov_file, ip_addr, task->ix_tag,
                (PAGE_ENTRY **) &task->last_ixpage, (PAGE_ENTRY **) &ixpg_ptr,
                task) != S_OKAY )
            return task->db_status;

        if (ixpg_ptr->buff == NULL)
            return (dberr(S_INVPTR));

        if (!ixp_used(fno, ipno, task))
        {
            /* initialize new index page */
            ixpg_ptr->ovfl_addr = ip_addr;
            memset(ixpg_ptr->buff, 0, IX_PAGESIZE);
            set_ixpbit(fno, ipno, task);
        }

        /* set index entry for new database page */
        ent = (int) (db_page - 1 - ipno * IX_EPP);
        eptr = (F_ADDR *) (ixpg_ptr->buff + (sizeof(F_ADDR) * ent));
        *eptr = pg_ptr->ovfl_addr;

        /* mark index page as modified */
        ixpg_ptr->modified = TRUE;
        ixpg_ptr->recently_used = TRUE;
    }

    return task->db_status;
}


/* ======================================================================
    Write a page zero to the overflow file
*/
int INTERNAL_FCN o_pzwrite(FILE_NO file, DB_TASK *task)
{
    F_ADDR    pcnt;       /* Number of pages in file */
    F_ADDR    pz_base;    /* page zero base address */
    RI_ENTRY *ri_ptr;
    void     *buff;
    const SG *sg;

    ri_ptr = &task->root_ix[file];
    pcnt = ri_ptr->pg_cnt;
    ri_ptr->pz_modified = TRUE;
    pz_base = ri_ptr->base;
    pz_base += (F_ADDR) (RI_BITMAP_SIZE(pcnt) * sizeof(int));
    pz_base += (F_ADDR) (IX_SIZE(pcnt) * IX_PAGESIZE);

    if ((sg = task->sgs[file]) != NULL)
    {
        buff = task->enc_buff;
        memcpy(buff, &task->pgzero[file], PGZEROSZ);
        (*sg->enc)(sg->data, buff, PGZEROSZ);
    }
    else
        buff = &task->pgzero[file];

    dio_writefile(task->ov_file, pz_base, buff, PGZEROSZ, 0, task);
    task->file_table[file].ft_flags &= ~TS_IS_CURRENT;
    STAT_log_write(PGZEROSZ, task);

    return task->db_status;
}


/* ======================================================================
    Flush the transaction recovery data to the overflow file
*/
int INTERNAL_FCN o_flush(DB_TASK *task)
{
    PSP_FH              desc;       /* Overflow file descriptor */
    int                 bm_size;    /* Size of ri_bitmap */
    register int        i;          /* Loop counter */
    register RI_ENTRY  *ri_ptr;
    PAGE_ENTRY         *ixpg_ptr;

    if (task->dboptions & READONLY)
        return (dberr(S_READONLY));

    if (dio_open(task->ov_file, task) != S_OKAY)
        return task->db_status;

    desc = task->file_table[task->ov_file].ft_desc;

    /* write the root index data out to the overflow file */
    for (i = 0, ri_ptr = task->root_ix; i < task->size_ft; ++i, ++ri_ptr)
    {
        if (psp_fileSeekWrite(desc, task->ov_rootaddr + i * RI_ENTRY_IOSIZE,
                ri_ptr, RI_ENTRY_IOSIZE) < (int) RI_ENTRY_IOSIZE)
            return (dberr(S_BADWRITE));

        STAT_log_write(RI_ENTRY_IOSIZE, task);
    }

    /* write the index bitmaps to the overflow file */
    for (i = 0, ri_ptr = task->root_ix; i < task->size_ft; ++i, ++ri_ptr)
    {
        bm_size = RI_BITMAP_SIZE(ri_ptr->pg_cnt) * sizeof(int);
        if (bm_size)
        {
            if (psp_fileSeekWrite(desc, BM_BASE(i), ri_ptr->ri_bitmap,
                    bm_size) < bm_size)
                return (dberr(S_BADWRITE));

            STAT_log_write(bm_size, task);
        }
    }

    /* flush index page cache */
    for (i = 0, ixpg_ptr = task->cache->ix_pgtab.pg_table; i < task->cache->ix_pgtab.pgtab_sz; ++i, ++ixpg_ptr)
    {
        if (ixpg_ptr->modified)
        {
            dio_out(ixpg_ptr, task);
            ixpg_ptr->modified = FALSE;
        }
    }

    /* commit the overflow file to ensure recoverability */
    commit_file(desc, task);

    return (task->db_status);
}


/* ======================================================================
    Update the database from the data in the overflow file
*/
int INTERNAL_FCN o_update(DB_TASK *task)
{
    register FILE_NO     file;       /* File loop counter */
    register int         ent;        /* Ixp entry loop counter */
    register F_ADDR      ipno;       /* Index page loop counter */
    short                size;       /* Size of page to read */
    F_ADDR               d_page;     /* Page num within db file */
    F_ADDR               ipcnt;      /* Num of index pages for a file */
    F_ADDR               pcnt;       /* Num of pages in file */
    F_ADDR               ip_addr;    /* Address of index page */
    F_ADDR              *eptr;       /* Ptr to ix page entry */
    register RI_ENTRY   *ri_ptr;
    register FILE_ENTRY *file_ptr;     
    PAGE_ENTRY          *ovpg_ptr;
    PAGE_ENTRY          *pg_ptr;
    PAGE_ENTRY           ovpg;       /* overflowed page */
/*    int stat; */

    if (task->dboptions & READONLY)
        return (dberr(S_READONLY));

    memset(&ovpg, '\0', sizeof(PAGE_ENTRY));
    ovpg.buff = task->cache->dbpgbuff;

    for ( file = 0, ri_ptr = task->root_ix, file_ptr = task->file_table;
          (task->db_status == S_OKAY) && (file < task->size_ft);
          ++file, ++ri_ptr, ++file_ptr)
    {
        if (ri_ptr->base == 0L)
            continue;

        size = file_ptr->ft_pgsize;
        pcnt = ri_ptr->pg_cnt;
        ipcnt = IX_SIZE(pcnt);

        /* scan each index page */
        for (ipno = 0; (task->db_status == S_OKAY) && (ipno < ipcnt); ++ipno)
        {
            if (ixp_used(file, ipno, task))
            {
                /* read index page */
                ip_addr = oaddr_of_ixp(file, ipno, task);
                dio_getpg(task->ov_file, ip_addr, task->ix_tag,
                          &task->last_ixpage, &ovpg_ptr, task);
                if (ovpg_ptr->buff == NULL)
                    return (dberr(S_INVPTR));

                /* scan each db page entry on index page */
                for (ent = 0; (task->db_status == S_OKAY) && (ent < IX_EPP); ++ent)
                {
                    eptr = (F_ADDR *) (ovpg_ptr->buff + sizeof(F_ADDR) * ent);
                    if (*eptr)
                    {
                        /* if page is in cache then it's already been
                           output to db
                        */
                        d_page = (ipno * IX_EPP) + ent + 1;
                        if ((dio_findpg(file, d_page, NULL, &pg_ptr, task) == S_OKAY) &&
                            !pg_ptr->modified)
                        {
                            /* An overflowed db page may not have been modified
                               after being reread into the cache from the overflow
                               file. Thus the page needs to be marked as modified so
                               dio_flush will write the page to the database.
                            */
                            pg_ptr->modified = TRUE;
                        }
                        else if (task->db_status == S_NOTFOUND)
                        {
                            task->db_status = S_OKAY;

                            /* update db page from overflow file */
                            ovpg.file = file;
                            ovpg.pageno = d_page;
                            ovpg.buff_size = size;

                            /* read the overflow page */
                            ovpg.ovfl_addr = *eptr;
                            dio_in((PAGE_ENTRY *)&ovpg, task);

                            if (task->db_status == S_OKAY)
                            {
                                /* write the page to the database */
                                ovpg.ovfl_addr = 0L;
                                dio_out((PAGE_ENTRY *)&ovpg, task);

                                if (task->dboptions & TXTEST)
                                {
                                    dberr(S_DEBUG);
                                    flush_dberr(task);
                                }
                            }

                            /* log the page if enabled */
                            if (task->trlog_flag && task->db_status == S_OKAY)
                                dtrlog(file, d_page, task->cache->dbpgbuff, size, task);
                        }
                    }
                }
                dio_clrpage(ovpg_ptr, task);  /* better not use it again! */
            }
        }

        /* If an archive log is being kept, then the pages which were written
           directly to the end of the db file must be read and logged if not
           in the cache.
        */
        if (task->db_status == S_OKAY && task->trlog_flag)
        {
            F_ADDR last_page = dio_pages(file, task);

            ovpg.file = file;
            for ( ovpg.pageno = pcnt;
                  (task->db_status == S_OKAY) && (ovpg.pageno <= last_page);
                  ++ovpg.pageno )
            {
                if (dio_findpg(file, ovpg.pageno, NULL, NULL, task) == S_NOTFOUND)
                {
                    task->db_status = S_OKAY;

                    /* read db page */
                    ovpg.ovfl_addr = 0L;
                    ovpg.buff_size = size;
                    dio_in((PAGE_ENTRY *)&ovpg, task);

                    /* log database page */
                    dtrlog(file, ovpg.pageno, ovpg.buff, size, task);
                }
            }
        }
    }

#ifdef DB_DEBUG
    /* the ix-cache should now be empty */
    for (ipno = 0, pg_ptr = task->cache->ix_pgtab.pg_table;
         ipno < task->cache->ix_pgtab.pgtab_sz;
         ++ipno, ++pg_ptr)
    {
        if (pg_ptr->file != -1)
            return (dberr(SYS_IXNOTCLEAR));
    }
#endif

/*    stat = dio_close(task->ov_file, task);
    if (task->db_status == S_OKAY)
        task->db_status = stat; */
    return (task->db_status);
}


/* ======================================================================
    Return file's page count at tx start
*/
F_ADDR INTERNAL_FCN o_pages(FILE_NO file, DB_TASK *task)
{
    return task->root_ix[file].pg_cnt;
}


/* ======================================================================
    Free dynamically allocated data
*/
void INTERNAL_FCN o_free(int dbn, DB_TASK *task)
{
    register int ft_lc;                 /* loop control */
    register RI_ENTRY *ri_ptr;
    int low, high;

    if (task->root_ix)
    {
        if (dbn == ALL_DBS)
        {
            low      = 0;
            ft_lc    = task->size_ft;
            ri_ptr   = &task->root_ix[0];
        }
        else
        {
            low      = task->db_table[dbn].ft_offset;
            ft_lc    = task->db_table[dbn].ft_offset + task->db_table[dbn].Size_ft;
            ri_ptr   = &task->root_ix[task->db_table[dbn].ft_offset];
        }

        for (high = ft_lc--; ft_lc >= low; --ft_lc, ++ri_ptr)
        {
            if (ri_ptr->ri_bitmap)
                psp_freeMemory(ri_ptr->ri_bitmap, 0);

            task->ov_file--;
        }

        free_table((void **) &task->root_ix, low, high, task->size_ft,
                sizeof(RI_ENTRY), task);
    }

    if (dbn == ALL_DBS)
        task->ov_setup_done = FALSE;

    /* Since FREE_TABLE() keeps everything above the region to be freed while
        it frees the middle, o_setup() does not need to be called because the
        task->ov_file element of the task->file_table[] will just slide down
    */

    /* set the initial useable address in the overflow file */
    ov_header_init(task);
}


/* ======================================================================
    Close the overflow file
*/
int INTERNAL_FCN o_close(DB_TASK *task)
{
    /* close overflow file */
    return dio_close(task->ov_file, task);
}

/**************************************************************************/

static void INTERNAL_FCN ov_header_init(DB_TASK *task)
{
    /* number of files */
    task->ov_initaddr = (F_ADDR) sizeof(short);
    
    /* page size & name of each file */
    task->ov_initaddr += (F_ADDR) ((sizeof(short) + (FILENMLEN * sizeof(DB_TCHAR))) * task->size_ft);
    task->ov_rootaddr = task->ov_initaddr;          /* address of start of root ix data */
    
    /* root index area */
    task->ov_initaddr += (F_ADDR) (task->size_ft * RI_ENTRY_IOSIZE);
    task->ov_nextaddr = task->ov_initaddr;
    task->ov_header_written = FALSE;
}



