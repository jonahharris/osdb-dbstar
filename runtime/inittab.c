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

/* Internal function prototypes */
static int INTERNAL_FCN initcurr(DB_TASK *);

/* Compatible dictionary versions */
char *compat_dbd[] = {
    dbd_VERSION_300,
    dbd_VERSION_301,
    dbd_VERSION_500,
#if defined(UNICODE)
    dbd_VERSION_u500
#endif
};
int size_compat = sizeof(compat_dbd) / sizeof(*compat_dbd);

/* Numeric equivalents */
int compat_ver[] = {
    dbd_V300,
    dbd_V301,
    dbd_V500,
#if defined(UNICODE)
    dbd_U500
#endif
};

static void getnames(DB_TCHAR **dest, DB_TCHAR **src, short start, short end)
{
    register short     ii;
    register DB_TCHAR *p = *src;

    for (ii = start; ii < end; ii++)
    {
        dest[ii] = p;
        while (*p && *p != DB_TEXT('\n'))
            p++;

        if (*p)
            *p++ = 0;
    }

    *src = p;
}

static void skipnames(DB_TCHAR **src, short num)
{
    short     ii;
    DB_TCHAR *p = *src;

    for (ii = 0; *p && ii < num; ii++)
    {
        while (*p && *p != DB_TEXT('\n'))
            p++;

        if (*p)
            p++;
    }

    *src = p;
}

/* ======================================================================
    Initialize database tables
*/
int INTERNAL_FCN inittab(DB_TASK *task, const SG *sg)
{
    register int        dbt_lc;
    register short      i;
    int                 stat = S_OKAY;
    int                 size;
    int                 v = 0;
    F_ADDR              n;
    F_ADDR              fpos = 0;
    F_ADDR              flen = 0;
    short               key_offset;
    short               key_count;
    short               Max_Size_ft = 0;
    DB_TCHAR           *p;
    DB_TCHAR            dbfile[FILENMLEN];
    DB_TCHAR            dbname[FILENMLEN]; /* Temporary working space */
    char                dbd_ver[DBD_COMPAT_LEN + 1];
    short               maxsize;
    PSP_FH              dbf;
    int                 is_query_db;
    FILE_ENTRY         *fptr;
    V300_FILE_ENTRY    *v300_fptr = NULL;
    V301_FILE_ENTRY    *v301_fptr = NULL;
    V500_FILE_ENTRY    *v500_fptr = NULL;
    U500_FILE_ENTRY    *u500_fptr = NULL;
    RECORD_ENTRY       *rec_ptr;
    SET_ENTRY          *set_ptr;
    FIELD_ENTRY        *fld_ptr;
    MEMBER_ENTRY       *mem_ptr;
    SORT_ENTRY         *srt_ptr;
    KEY_ENTRY          *key_ptr;
    DB_ENTRY           *curr_dbt = task->curr_db_table;
    struct {
        short Page_size;
        short Size_ft;
        short Size_rt;
        short Size_fd;
        short Size_st;
        short Size_mt;
        short Size_srt;
        short Size_kt;
    } dict_size;

    /* initialize (merged) dictionary values */
    if (task->no_of_dbs == 1)
        task->db_tsrecs = task->db_tssets = FALSE;

    key_offset = 0;

    key_offset = (short) task->no_of_keys;
    /* compute individual dictionary sizes and offsets */
    for (dbt_lc = task->old_no_of_dbs,
            task->curr_db_table = &task->db_table[task->old_no_of_dbs];
            dbt_lc < task->no_of_dbs; ++dbt_lc, ++task->curr_db_table)
    {
        curr_dbt = task->curr_db_table;
        /* form database dictionary name */
        if (curr_dbt->db_path[0])
            vtstrcpy(dbname, curr_dbt->db_path);
        else
            dbname[0] = DB_TEXT('\0');

        if (vtstrlen(dbname) + vtstrlen(curr_dbt->db_name) >= FILENMLEN + 4)
            return (dberr(S_NAMELEN));

        vtstrcat(dbname, curr_dbt->db_name);
        stat = con_dbd(dbfile, dbname, get_element(task->dbdpath, dbt_lc), task);
        if (stat != S_OKAY)
            return (task->db_status);

        if ((dbf = psp_fileOpen(dbfile, O_RDONLY, PSP_FLAG_DENYNO)) == NULL)
            return (dberr(S_INVDB));

        /* Read in and verify the dictionary version */
        psp_fileSeek(dbf, 0);
        if (psp_fileRead(dbf, dbd_ver, DBD_COMPAT_LEN) != DBD_COMPAT_LEN)
        {
            psp_fileClose(dbf);
            return (dberr(S_BADREAD));
        }

        dbd_ver[DBD_COMPAT_LEN] = '\0';
        for (i = 0; i < size_compat; i++)
        {
            if (strcmp(dbd_ver, compat_dbd[i]) == 0)
            {
                v = compat_ver[i];
                curr_dbt->db_ver = (short) v;
                break;
            }
        }

        if (i == size_compat)
        {
            /* Incompatible dictionary file */
            psp_fileClose(dbf);
            return (dberr(S_INCOMPAT));
        }

        /* Read in database page and table sizes */
        if (psp_fileRead(dbf, &dict_size, sizeof(dict_size)) <
                (int) sizeof(dict_size))
            stat = S_BADREAD;

        psp_fileClose(dbf);
        if (stat != S_OKAY)
            return (task->db_status = stat);

        if (dict_size.Size_ft > 255)
            return dberr(S_INVDB); /* probably different byte ordering */

        if (dict_size.Size_ft > Max_Size_ft)
            Max_Size_ft = dict_size.Size_ft;

        curr_dbt->Page_size = dict_size.Page_size;
        curr_dbt->Size_ft   = dict_size.Size_ft;
        curr_dbt->Size_rt   = dict_size.Size_rt;
        curr_dbt->Size_fd   = dict_size.Size_fd;
        curr_dbt->Size_st   = dict_size.Size_st;
        curr_dbt->Size_mt   = dict_size.Size_mt;
        curr_dbt->Size_srt  = dict_size.Size_srt;
        curr_dbt->Size_kt   = dict_size.Size_kt;

        curr_dbt->sysdba = NULL_DBA;

        /* update merged dictionary offsets and sizes */
        if (curr_dbt->Page_size > task->page_size)
            task->page_size = curr_dbt->Page_size;

        curr_dbt->ft_offset = task->size_ft;
        task->size_ft += curr_dbt->Size_ft;
        curr_dbt->rt_offset = task->size_rt;
        task->size_rt += curr_dbt->Size_rt;
        curr_dbt->fd_offset = task->size_fd;
        task->size_fd += curr_dbt->Size_fd;
        curr_dbt->st_offset = task->size_st;
        task->size_st += curr_dbt->Size_st;
        curr_dbt->mt_offset = task->size_mt;
        task->size_mt += curr_dbt->Size_mt;
        curr_dbt->srt_offset = task->size_srt;
        task->size_srt += curr_dbt->Size_srt;
        curr_dbt->kt_offset = task->size_kt;
        task->size_kt += curr_dbt->Size_kt;
    }

    /* allocate dictionary space */
    if (alloc_dict(task, sg) != S_OKAY)
        return task->db_status;

    /* Allocate extra array to convert file table from dbd format to
       internal format. Don't know until we read the DBD version whether
       this is a v3.00 DBD file (normal), v3.01, with file block
       extensions (VxWorks), v5.00, with file block extensions and 256
       char file names or u5.00 (same as v5.00, but wchar_t Unicode file
       names. None of these versions of the file table is bigger than the
       current FILE_ENTRY structure.
    */
    fptr = (FILE_ENTRY *) psp_getMemory(Max_Size_ft * sizeof(FILE_ENTRY), 0);
    if (fptr == NULL)
        return (dberr(S_NOMEMORY));

    /* use the same memory, whatever the dbd version is */
    v300_fptr = (V300_FILE_ENTRY *) fptr;
    v301_fptr = (V301_FILE_ENTRY *) fptr;
    v500_fptr = (V500_FILE_ENTRY *) fptr;
    u500_fptr = (U500_FILE_ENTRY *) fptr;

    /* read in and adjust dictionary entries for each database */
    for ( dbt_lc = task->old_no_of_dbs, task->curr_db_table = &task->db_table[task->old_no_of_dbs];
          dbt_lc < task->no_of_dbs;
          ++dbt_lc, ++task->curr_db_table)
    {
        curr_dbt = task->curr_db_table;
        /* form database dictionary name */
        if (curr_dbt->db_path[0])
            vtstrcpy(dbname, curr_dbt->db_path);
        else
            dbname[0] = DB_TEXT('\0');

        if (vtstrlen(dbname) + vtstrlen(curr_dbt->db_name) >= FILENMLEN + 4)
        {
            psp_freeMemory(v300_fptr, 0);
            return (dberr(S_NAMELEN));
        }

        vtstrcat(dbname, curr_dbt->db_name);

        /* special check for db_query temporary database */
        is_query_db = (vtstricmp(curr_dbt->db_name, DB_TEXT("db_query")) == 0);

        if (con_dbd(dbfile, dbname, get_element(task->dbdpath, dbt_lc), task) != S_OKAY)
        {
            psp_freeMemory(v300_fptr, 0);
            return (task->db_status);
        }

        dbf = psp_fileOpen(dbfile, O_RDONLY, PSP_FLAG_DENYNO);
        flen = psp_fileLength(dbf);

        /* determine dbd version, and read file table into appropriate
           structure:
                V3.00 has 48 character file names
                V3.01 has 48 character file names with file table extensions
                V5.00 has 256 character (ansi) file names with extensions
                U5.00 has 256 character (unicode) file names with extensions
        */
        size = (v == 500) ? sizeof(V500_FILE_ENTRY) :
               (v == -500) ? sizeof(U500_FILE_ENTRY) :
               (v == 301) ? sizeof(V301_FILE_ENTRY) : sizeof(V300_FILE_ENTRY);

        fpos = DBD_COMPAT_LEN + 8L * sizeof(short);
        n = curr_dbt->Size_ft * size;

        psp_fileSeek(dbf, fpos);
        if (psp_fileRead(dbf, v300_fptr, n) < n)
            stat = S_BADREAD;

        fpos += n;

        if (stat == S_OKAY)
        {
            for (i = 0, fptr = &task->file_table[curr_dbt->ft_offset];
                 i < curr_dbt->Size_ft; ++i, ++fptr)
            {
                switch (v)
                {
                    case 300:
                        atot(v300_fptr[i].v3_ft_name, fptr->ft_name, FILENMLEN);
                        fptr->ft_status  = v300_fptr[i].v3_ft_status;
                        fptr->ft_type    = v300_fptr[i].v3_ft_type;
                        fptr->ft_slots   = v300_fptr[i].v3_ft_slots;
                        fptr->ft_slsize  = v300_fptr[i].v3_ft_slsize;
                        fptr->ft_pgsize  = v300_fptr[i].v3_ft_pgsize;
                        fptr->ft_flags   = v300_fptr[i].v3_ft_flags;
                        fptr->ft_pctincrease = 0;
                        fptr->ft_initial = 0;
                        fptr->ft_next    = 0;
                        fptr->ft_index   = -1;
                        fptr->ft_lmtimestamp = 0;
                        fptr->ft_cachetimestamp = 0;
                        break;

                    case 301:
                        atot(v301_fptr[i].v3_ft_name, fptr->ft_name, FILENMLEN);
                        fptr->ft_status  = v301_fptr[i].v3_ft_status;
                        fptr->ft_type    = v301_fptr[i].v3_ft_type;
                        fptr->ft_slots   = v301_fptr[i].v3_ft_slots;
                        fptr->ft_slsize  = v301_fptr[i].v3_ft_slsize;
                        fptr->ft_pgsize  = v301_fptr[i].v3_ft_pgsize;
                        fptr->ft_flags   = v301_fptr[i].v3_ft_flags;
                        fptr->ft_pctincrease = v301_fptr[i].v3_ft_pctincrease;
                        fptr->ft_initial = v301_fptr[i].v3_ft_initial;
                        fptr->ft_next    = v301_fptr[i].v3_ft_next;
                        fptr->ft_index = -1;
                        fptr->ft_lmtimestamp = 0;
                        fptr->ft_cachetimestamp = 0;
                        break;

                    case 500:
                        atot(v500_fptr[i].v5_ft_name, fptr->ft_name, FILENMLEN);
                        fptr->ft_status  = v500_fptr[i].v5_ft_status;
                        fptr->ft_type    = v500_fptr[i].v5_ft_type;
                        fptr->ft_slots   = v500_fptr[i].v5_ft_slots;
                        fptr->ft_slsize  = v500_fptr[i].v5_ft_slsize;
                        fptr->ft_pgsize  = v500_fptr[i].v5_ft_pgsize;
                        fptr->ft_flags   = v500_fptr[i].v5_ft_flags;
                        fptr->ft_pctincrease = v500_fptr[i].v5_ft_pctincrease;
                        fptr->ft_initial = v500_fptr[i].v5_ft_initial;
                        fptr->ft_next    = v500_fptr[i].v5_ft_next;
                        fptr->ft_index = -1;
                        fptr->ft_lmtimestamp = 0;
                        fptr->ft_cachetimestamp = 0;
                        break;
#if defined(UNICODE)
                    case -500:
                        memcpy(fptr, &u500_fptr[i], sizeof(FILE_ENTRY));
                        break;
#endif
                }
            }
        }
        if (stat == S_OKAY)
        {
            n = curr_dbt->Size_rt * sizeof(RECORD_ENTRY);
            if (psp_fileRead(dbf, &task->record_table[curr_dbt->rt_offset], n) < n)
                stat = S_BADREAD;

            fpos += n;
        }

        if (stat == S_OKAY && curr_dbt->Size_fd)
        {
            n = curr_dbt->Size_fd * sizeof(FIELD_ENTRY);
            if (psp_fileRead(dbf, &task->field_table[curr_dbt->fd_offset], n) < n)
                stat = S_BADREAD;

            fpos += n;
        }

        if (stat == S_OKAY && curr_dbt->Size_st)
        {
            n = curr_dbt->Size_st * sizeof(SET_ENTRY);
            if (psp_fileRead(dbf, &task->set_table[curr_dbt->st_offset], n) < n)
                stat = S_BADREAD;

            fpos += n;
        }

        if (stat == S_OKAY && curr_dbt->Size_mt)
        {
            n = curr_dbt->Size_mt * sizeof(MEMBER_ENTRY);
            if (psp_fileRead(dbf, &task->member_table[curr_dbt->mt_offset], n) < n)
                stat = S_BADREAD;

            fpos += n;
        }

        if (stat == S_OKAY && curr_dbt->Size_srt)
        {
            n = curr_dbt->Size_srt * sizeof(SORT_ENTRY);
            if (psp_fileRead(dbf, &task->sort_table[curr_dbt->srt_offset], n) < n)
                stat = S_BADREAD;

            fpos += n;
        }

        if (stat == S_OKAY && curr_dbt->Size_kt)
        {
            n = curr_dbt->Size_kt * sizeof(KEY_ENTRY);
            if (psp_fileRead(dbf, &task->key_table[curr_dbt->kt_offset], n) < n)
                stat = S_BADREAD;

            fpos += n;
        }

        flen = (flen > fpos) ? (flen - fpos) : 0;
        if ((p = (DB_TCHAR *) psp_cGetMemory(flen, 0)) == NULL)
            stat = S_NOMEMORY;
        else
        {
            if (psp_fileRead(dbf, p, flen) < flen)
               stat = S_BADREAD;
            else if (task->dboptions & READNAMES)
            {
#if defined(UNICODE)
                if (v > 0) /* ANSI dbd, convert names to Unicode */
                {
                    char *q = (char *)p;

                    p = (DB_TCHAR *) psp_cGetMemory(flen * sizeof(DB_TCHAR), 0);
                    if (!p)
                        stat = S_NOMEMORY;
                    else
                        atow(q, p, flen);

                    psp_freeMemory(q, 0);
                }
#endif
                if (stat == S_OKAY)
                {
                    curr_dbt->Objnames = p;
                    getnames(task->record_names, &p, curr_dbt->rt_offset,
                            (short) (curr_dbt->rt_offset + curr_dbt->Size_rt));
                    getnames(task->field_names, &p, curr_dbt->fd_offset,
                            (short) (curr_dbt->fd_offset + curr_dbt->Size_fd));
                    getnames(task->set_names, &p, curr_dbt->st_offset,
                            (short) (curr_dbt->st_offset + curr_dbt->Size_st));
                    flen -= p - curr_dbt->Objnames;
                }
            }
            else
            {
                DB_TCHAR *tmp = p;

                skipnames(&p, (short) (curr_dbt->Size_rt + curr_dbt->Size_fd +
                        curr_dbt->Size_st));
                flen -= p - tmp;
            }

            if (stat == S_OKAY)
            {
                if (flen == 16)
                {
                    if (!sg)
                        stat = S_INVENCRYPT;
                    else if (!(*sg->chk)(sg->data, p))
                        stat = S_INVENCRYPT;
                }
                else if (sg)
                    stat = S_INVENCRYPT;
            }
        }

        psp_fileClose(dbf);
        if (stat != S_OKAY)
        {
            psp_freeMemory(v300_fptr, 0);
            return dberr(stat);
        }

        maxsize = 0;
        fptr = &task->file_table[curr_dbt->ft_offset];
        for (i = 0; i < curr_dbt->Size_ft; i++, fptr++)
        {
             if (fptr->ft_pgsize > maxsize)
                 maxsize = fptr->ft_pgsize;
        }

        if (maxsize > task->enc_size)
        {
            task->enc_buff = psp_extendMemory(task->enc_buff, maxsize, 0);
            task->enc_size = maxsize;
        }

        curr_dbt->key_offset = key_offset;

        /* update file table path entries */
        if (curr_dbt->db_path[0] || task->dbfpath[0])
        {
            for ( i = 0, fptr = &task->file_table[curr_dbt->ft_offset];
                  i < curr_dbt->Size_ft; ++i, ++fptr)
            {
                /* Construct the data/key file name */
                if (curr_dbt->db_path[0])
                    vtstrcpy(dbname, curr_dbt->db_path);
                else
                    dbname[0] = DB_TEXT('\0');

                if (vtstrlen(dbname) + vtstrlen(curr_dbt->db_name) >= FILENMLEN + 4)
                {
                    psp_freeMemory(v300_fptr, 0);
                    return (dberr(S_NAMELEN));
                }

                vtstrcat(dbname, curr_dbt->db_name);
                if (con_dbf(dbfile, fptr->ft_name, dbname,
                        get_element(task->dbfpath, dbt_lc), task) != S_OKAY)
                {
                    psp_freeMemory(v300_fptr, 0);
                    return (task->db_status);
                }

                /* Save new name in dictionary */
                vtstrcpy(fptr->ft_name, dbfile);
            }
        }

        /* make db_query's files temporary so they don't get sent
            to the lock manager.
        */
        if (is_query_db)
        {
            for ( i = 0, fptr = &task->file_table[curr_dbt->ft_offset];
                  i < curr_dbt->Size_ft; ++i, ++fptr)
                fptr->ft_flags |= (TEMPORARY | NOT_TTS_FILE);
        }   

        /* adjust record table entries */
        for ( i = curr_dbt->rt_offset, rec_ptr = &task->record_table[curr_dbt->rt_offset];
              i < curr_dbt->rt_offset + curr_dbt->Size_rt;
              ++i, ++rec_ptr)
        {
            rec_ptr->rt_file += curr_dbt->ft_offset;
            rec_ptr->rt_fields += curr_dbt->fd_offset;
            if (rec_ptr->rt_flags & TIMESTAMPED)
                task->db_tsrecs = TRUE;
        }

        /* adjust field table entries */
        for ( key_count = 0, i = curr_dbt->fd_offset,
              fld_ptr = &task->field_table[curr_dbt->fd_offset];
              i < curr_dbt->fd_offset + curr_dbt->Size_fd; ++i, ++fld_ptr)
        {
            fld_ptr->fd_rec += curr_dbt->rt_offset;
            if (fld_ptr->fd_key != NOKEY)
            {
                fld_ptr->fd_keyno += key_offset;
                ++key_count;
                fld_ptr->fd_keyfile += curr_dbt->ft_offset;
                if (fld_ptr->fd_type == 'k')
                    fld_ptr->fd_ptr += curr_dbt->kt_offset;
            }
        }

        key_offset += key_count;

        /* adjust set table entries */
        for ( i = curr_dbt->st_offset, set_ptr = &task->set_table[curr_dbt->st_offset];
              i < curr_dbt->st_offset + curr_dbt->Size_st;
              ++i, ++set_ptr)
        {
            set_ptr->st_own_rt += curr_dbt->rt_offset;
            set_ptr->st_members += curr_dbt->mt_offset;

            if (set_ptr->st_flags & TIMESTAMPED)
                task->db_tssets = TRUE;
        }

        /* adjust member table entries */
        for ( i = curr_dbt->mt_offset,
              mem_ptr = &task->member_table[curr_dbt->mt_offset];
              i < curr_dbt->mt_offset + curr_dbt->Size_mt;
              ++i, ++mem_ptr)
        {
            mem_ptr->mt_record += curr_dbt->rt_offset;
            mem_ptr->mt_sort_fld += curr_dbt->srt_offset;
        }

        /* adjust sort table entries */
        for ( i = curr_dbt->srt_offset,
              srt_ptr = &task->sort_table[curr_dbt->srt_offset];
              i < curr_dbt->srt_offset + curr_dbt->Size_srt;
              ++i, ++srt_ptr)
        {
            srt_ptr->se_fld += curr_dbt->fd_offset;
            srt_ptr->se_set += curr_dbt->st_offset;
        }

        /* adjust key table entries */
        for ( i = curr_dbt->kt_offset,
              key_ptr = &task->key_table[curr_dbt->kt_offset];
              i < curr_dbt->kt_offset + curr_dbt->Size_kt;
              ++i, ++key_ptr)
        {
            key_ptr->kt_key += curr_dbt->fd_offset;
            key_ptr->kt_field += curr_dbt->fd_offset;
        }
    }

    psp_freeMemory(v300_fptr, 0);
    initcurr(task);

    return task->db_status;
}



/* ======================================================================
    Allocate space for dictionary
*/
int EXTERNAL_FCN alloc_dict(DB_TASK *task, const SG *sg)
{
    int   stat;
    short ii;
    short old_size_ft;

    DB_ENTRY *db_ptr;

    /* allocate and initialize task->file_table */

    if (task->old_no_of_dbs == 0)
    {
        task->old_size_ft = 0;
        task->old_size_fd = 0;
        task->old_size_st = 0;
        task->old_size_mt = 0;
        task->old_size_srt = 0;
        task->old_size_kt = 0;
        task->old_size_rt = 0;
    }
    else
    {
        /* Refers to the last of the old databases */
        db_ptr = &task->db_table[task->old_no_of_dbs - 1];
        task->old_size_ft  = (short) (db_ptr->Size_ft + db_ptr->ft_offset);
        task->old_size_fd  = (short) (db_ptr->Size_fd + db_ptr->fd_offset);
        task->old_size_st  = (short) (db_ptr->Size_st + db_ptr->st_offset);
        task->old_size_mt  = (short) (db_ptr->Size_mt + db_ptr->mt_offset);
        task->old_size_srt = (short) (db_ptr->Size_srt + db_ptr->srt_offset);
        task->old_size_kt  = (short) (db_ptr->Size_kt + db_ptr->kt_offset);
        task->old_size_rt  = (short) (db_ptr->Size_rt + db_ptr->rt_offset);
    }

    /* we must take into account the log file in this computation */
    if (task->old_size_ft != 0)
        old_size_ft = (task->old_size_ft + 1);
    else
        old_size_ft = 0;

    stat = alloc_table((void **) &task->file_table, (task->size_ft + 1) *
            sizeof(FILE_ENTRY), old_size_ft * sizeof(FILE_ENTRY), task);
    if (stat != S_OKAY)
        return stat;

    stat = alloc_table((void **) &task->sgs, (task->size_ft + 1) * sizeof(SG *),
            old_size_ft * sizeof(SG *), task);
    if (stat != S_OKAY)
        return stat;

    for (ii = old_size_ft; ii < task->size_ft + 1; ii++)
        task->sgs[ii] = sg;

#ifdef DBSTAT
    stat = alloc_table((void **) &task->file_stats, task->size_ft *
            sizeof(FILE_STATS), task->old_size_ft * sizeof(FILE_STATS), task);
    if (stat != S_OKAY)
        return stat;
#endif

    /* allocate task->record_table */
    stat = alloc_table((void **) &task->record_table, task->size_rt *
            sizeof(RECORD_ENTRY), task->old_size_rt * sizeof(RECORD_ENTRY), task);
    if (stat != S_OKAY)
        return stat;

    /* allocate task->field_table */
    stat = alloc_table((void **) &task->field_table, task->size_fd *
            sizeof(FIELD_ENTRY), task->old_size_fd * sizeof(FIELD_ENTRY), task);
    if (stat != S_OKAY)
        return stat;

    /* allocate set table */
    if (task->size_st)
    {
        if (task->size_st > task->old_size_st)
        {
            stat = alloc_table((void **) &task->set_table, task->size_st *
                    sizeof(SET_ENTRY), task->old_size_st * sizeof(SET_ENTRY), task);
            if (stat != S_OKAY)
                return stat;
        }
    }
    else
        task->set_table = NULL;

    /* allocate task->member_table */
    if (task->size_mt)
    {
        if (task->size_mt > task->old_size_mt)
        {
            stat = alloc_table((void **) &task->member_table, task->size_mt *
                    sizeof(MEMBER_ENTRY), task->old_size_mt * sizeof(MEMBER_ENTRY),
                    task);
            if (stat != S_OKAY)
                return stat;
        }
    }
    else
        task->member_table = NULL;

    /* allocate task->sort_table */
    if (task->size_srt)
    {
        if (task->size_srt > task->old_size_srt)
        {
            stat = alloc_table((void **) &task->sort_table, task->size_srt *
                    sizeof(SORT_ENTRY), task->old_size_srt * sizeof(SORT_ENTRY),
                    task);
            if (stat != S_OKAY)
                return stat;
        }
    }
    else
        task->sort_table = NULL;

    /* allocate task->key_table */
    if (task->size_kt)
    {
        if (task->size_kt > task->old_size_kt)
        {
            stat = alloc_table((void **) &task->key_table, task->size_kt *
                    sizeof(KEY_ENTRY), task->old_size_kt * sizeof(KEY_ENTRY), task);
            if (stat != S_OKAY)
                return stat;
        }
    }
    else
        task->key_table = NULL;

    if (task->dboptions & READNAMES)
    {
        stat = alloc_table((void **) &task->record_names, task->size_rt *
                sizeof(DB_TCHAR *), task->old_size_rt * sizeof(DB_TCHAR *), task);
        if (stat != S_OKAY)
            return stat;

        stat = alloc_table((void **) &task->field_names, task->size_fd *
                sizeof(DB_TCHAR *), task->old_size_fd * sizeof(DB_TCHAR *), task);
        if (stat != S_OKAY)
            return stat;

        if (task->size_st)
        {
            if (task->size_st > task->old_size_st)
            {
                stat = alloc_table((void **) &task->set_names, task->size_st *
                        sizeof(DB_TCHAR *), task->old_size_st * sizeof(DB_TCHAR *),
                        task);
                if (stat != S_OKAY)
                    return stat;
            }
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Initialize currency tables
*/
static int INTERNAL_FCN initcurr(DB_TASK *task)
{
    register int     dbt_lc;                /* loop control */
    register int     rec,
                     i;
    RECORD_ENTRY    *rec_ptr;
    SET_ENTRY       *set_ptr;
    DB_ADDR         *co_ptr;
    DB_ENTRY        *curr_dbt;
    int              old_size;
    int              new_size;

    /* Initialize current record and type */
    for ( dbt_lc = task->no_of_dbs, task->curr_db_table = &task->db_table[task->old_no_of_dbs],
          task->curr_rn_table = &task->rn_table[task->old_no_of_dbs];
          --dbt_lc >= task->old_no_of_dbs;
          ++task->curr_db_table, ++task->curr_rn_table)
    {
        task->curr_db_table->curr_dbt_rec = NULL_DBA;
        task->curr_rn_table->rn_dba = NULL_DBA;
        task->curr_rn_table->rn_type = -1;
    }

    if (task->size_st)
    {
        if (task->size_st > task->old_size_st)
        {
            new_size = task->size_st * sizeof(DB_ADDR);
            old_size = task->old_size_st * sizeof(DB_ADDR);
            if (alloc_table((void **) &task->curr_own, new_size, old_size,
                    task) != S_OKAY)
                return task->db_status;

            if (alloc_table((void **) &task->curr_mem, new_size, old_size,
                    task) != S_OKAY)
                return task->db_status;

            new_size = task->size_st * sizeof(DB_ULONG);
            if (task->db_tsrecs)
            {
                if (task->co_time == NULL)
                    old_size = 0;
                else
                    old_size = task->old_size_st * sizeof(DB_ULONG);

                if (alloc_table((void **) &task->co_time, new_size, old_size,
                        task) != S_OKAY)
                    return task->db_status;

                if (alloc_table((void **) &task->cm_time, new_size, old_size,
                        task) != S_OKAY)
                    return task->db_status;
            }

            if (task->db_tssets)
            {
                if (task->cs_time == NULL)
                    old_size = 0;
                else
                    old_size = task->old_size_st * sizeof(DB_ULONG);

                if (alloc_table((void **) &task->cs_time, new_size, old_size,
                        task) != S_OKAY)
                    return task->db_status;
            }

            /* for each db make system record as task->curr_own of its sets */
            task->curr_db_table = &task->db_table[task->old_no_of_dbs];
            for ( dbt_lc = task->no_of_dbs;
                  --dbt_lc >= task->old_no_of_dbs;
                  ++task->curr_db_table)
            {
                curr_dbt = task->curr_db_table;
                for ( rec = curr_dbt->rt_offset,
                      rec_ptr = &task->record_table[curr_dbt->rt_offset];
                      rec < curr_dbt->rt_offset + curr_dbt->Size_rt;
                      ++rec, ++rec_ptr)
                {
                    if (rec_ptr->rt_fdtot == -1)
                    {
                        DB_ADDR sys_dba;
    
                        /* found system record */
                        ENCODE_DBA(NUM2EXT(rec_ptr->rt_file, ft_offset), 1L,
                            &sys_dba);
    
                        /* make system record current of sets it owns */
                        for ( i = curr_dbt->st_offset,
                              set_ptr = &task->set_table[curr_dbt->st_offset],
                              co_ptr = &task->curr_own[curr_dbt->st_offset];
                              i < curr_dbt->st_offset + curr_dbt->Size_st;
                              ++i, ++set_ptr, ++co_ptr)
                        {
                            if (set_ptr->st_own_rt == rec)
                                *co_ptr = sys_dba;
                        }

                        curr_dbt->sysdba = sys_dba;
                        curr_dbt->curr_dbt_rec = sys_dba;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        task->curr_own = NULL;
        task->curr_mem = NULL;
    }

    /* save the old  current record */
    if (task->old_no_of_dbs > 0)
        task->db_table[task->curr_db].curr_dbt_rec = task->curr_rec;

    task->curr_db = task->old_no_of_dbs;
    task->set_db = task->old_no_of_dbs;
    task->curr_db_table = &task->db_table[task->curr_db];
    task->curr_rn_table = &task->rn_table[task->curr_db];
    task->curr_rec = task->curr_db_table->curr_dbt_rec;

    return (task->db_status);
}


