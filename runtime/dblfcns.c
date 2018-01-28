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
#include "rntmint.h"

/* Internal function prototypes */
static int INTERNAL_FCN bld_lock_tables(DB_TASK *);
static int INTERNAL_FCN initses(int, DB_TASK *);
static int INTERNAL_FCN termses(int, DB_TASK *);
static int INTERNAL_FCN cmpfiles(DB_TCHAR *, DB_TCHAR *);

static int alloc_lfpkts (DB_TASK *);

/* Constants used by d_open and d_iopen */
#define FIRST_OPEN 0
#define INCR_OPEN  1


/* ======================================================================
    Set the lock request timeout value
*/
int INTERNAL_FCN dtimeout(int secs, DB_TASK *task)
{
    int         stat;
    LM_SETTIME *sto;             /* send timeout packet */

    if (!task->dbopen)
        return (dberr(S_DBOPEN));

    if (task->dbopen == 1)
    {
        if ((sto = psp_lmcAlloc(sizeof(LM_SETTIME))) == NULL)
            return (dberr(S_NOMEMORY));

        sto->queue_timeout = (short) secs;
        sto->lock_timeout = (short)-1;
        stat = msg_trans(L_SETTIME, sto, sizeof(LM_SETTIME), NULL, 0, task);
        if (stat == S_OKAY)
            task->db_timeout = secs;

        psp_lmcFree(sto);
    }

    return (task->db_status);
}


/* ======================================================================
    Perform the actual database open(s)

    The global variables task->type, task->db_lockmgr, and task->dbopen are set according
    to the openmode:

        openmode  task->type  task->db_lockmgr  task->dbopen
        --------  ----------  ----------------  ------------
          "x"      "x"            1               2
          "s"      "s"            1               1
          "o"      "x"            0               2
*/
int INTERNAL_FCN dopen(const DB_TCHAR *dbnames,
                       const DB_TCHAR *openmode,
                       int             opentype,
                       const SG       *sg,
                       DB_TASK        *task)
{
    short    ii;
    DB_TCHAR omode;
    int      dbopen_sv = task->dbopen;
    int      hold_db_status;
    size_t   Lp_size = task->lp_size;
    size_t   Fp_size = task->fp_size;
    short Page_size = task->page_size;
    short Size_ft   = task->size_ft;
    short Size_rt   = task->size_rt;
    short Size_st   = task->size_st;
    short Size_mt   = task->size_mt;
    short Size_srt  = task->size_srt;
    short Size_fd   = task->size_fd;
    short Size_kt   = task->size_kt;


    if (task->dbopen && opentype == FIRST_OPEN)
    {
        diclose(task, ALL_DBS);
        dbopen_sv = 0;
    }

    if (!task->dbopen && opentype == INCR_OPEN)
        return (dberr(S_DBOPEN));

    if (opentype == INCR_OPEN && task->trans_id[0])
        return (dberr(S_TRACTIVE));

    if (openmode)
    {
        if (opentype == FIRST_OPEN)
        {
            /* could be set by inifile or cmdline */
            omode = (DB_TCHAR) (*openmode ? *openmode : task->type[0]);
            omode = (DB_TCHAR) (vistupper(omode) ? vtotlower(omode) : omode);
            switch (omode)
            {
                case DB_TEXT('x'):
                case DB_TEXT('s'):
                    task->db_lockmgr = 1;
                    task->type[0] = omode;
                    break;

                case DB_TEXT('o'):
                    task->db_lockmgr = 0;
                    task->type[0] = DB_TEXT('x');
                    break;

                default:
                    return (dberr(S_BADTYPE));
            }
        }
    }
    else
        task->type[0] = DB_TEXT('x');

    task->type[1] = DB_TEXT('\0');

    if (opentype == FIRST_OPEN)
    {
        /* initialize from ini file */
        if (initFromIniFile(task) != S_OKAY)    /* must be called before dio_init() */
            return task->db_status;

        if (task->db_lockmgr && !task->lmc)
            return (dberr(S_NOLOCKCOMM));

        /* initialize the country table if "db.star.ctb" exists */
        if (ctb_init(task) != S_OKAY)
            return task->db_status;
    }
    else
    {
        /* ensure overflow file is closed as it will be reinitilized */
        dio_close(task->ov_file, task);
    }

    /* initialize multi-db tables */
    if (initdbt(dbnames, task) != S_OKAY)
        goto clean_ret;

    /* read in schema tables */
    if (inittab(task, sg) != S_OKAY)
        goto clean_ret;

    if (renfiles(task) != S_OKAY)
        goto clean_ret;

    if (task->db_lockmgr)
    {                    /* [637] Only alloc task->file_refs for shared open */
        if (alloc_table((void **) &task->file_refs,
                        task->size_ft * sizeof(FILE_NO),
                        task->old_size_ft * sizeof(FILE_NO),
                        task) != S_OKAY)
            goto clean_ret;
    }

    for (ii = task->old_size_ft; ii < task->size_ft; ii++)
        task->sgs[ii] = sg;

    if (*task->type == DB_TEXT('s'))
    {
        /* build application file lock tables */
        if (bld_lock_tables(task) != S_OKAY)
            goto clean_ret;

        task->dbopen = 1;
    }
    else
        task->dbopen = 2;

    if (opentype == FIRST_OPEN && !(task->dboptions & NORECOVER) &&
            taf_login(task) != S_OKAY)
        goto clean_ret_2;

    if (task->db_lockmgr)
        initses(opentype, task);
    else
        recovery_check(task);

    if (task->db_status != S_OKAY)
        goto clean_ret;

    if (o_setup(task) != S_OKAY)
        goto clean_ret;

    if (key_open(task) == S_OKAY)
    {
        if (dio_init(task) == S_OKAY)
            return task->db_status;
    }

clean_ret:     /* an error has occured, clean-up the mess w/ taf_logout() */

    hold_db_status = task->db_status;
    if (dbopen_sv == 0)
    {
        if (!(task->dboptions & NORECOVER))
            taf_logout(task);
    }

    task->db_status = hold_db_status;

clean_ret_2:  /* an error has occured, clean-up the mess w/o taf_logout() */

    /*
        Do not attampt to free tables allocated from heap if there was an
        error while doing incremental open. There's no way of telling which
        tables were successfully allocated, so just leave them all alone -
        they will be freed next time a database is closed.
    */

    hold_db_status = task->db_status;
    task->dbopen = 0;
    if (dbopen_sv == 0 || task->old_no_of_dbs == 0)
    {
        /* not doing incremental open */
        termfree(ALL_DBS, task);
    }

    /* restore original sizes */
    task->dbopen    = dbopen_sv;
    task->lp_size   = Lp_size;
    task->fp_size   = Fp_size;
    task->page_size = Page_size;
    task->size_ft   = Size_ft;
    task->size_rt   = Size_rt;
    task->size_st   = Size_st;
    task->size_mt   = Size_mt;
    task->size_srt  = Size_srt;
    task->size_fd   = Size_fd;
    task->size_kt   = Size_kt;
    task->curr_db_table = &task->db_table[task->curr_db];
    task->curr_rn_table = &task->rn_table[task->curr_db];

    task->no_of_dbs = task->old_no_of_dbs;
    return (task->db_status = hold_db_status);
}


/* ======================================================================
    Initialize multiple database table entries
*/
int INTERNAL_FCN initdbt(const DB_TCHAR *dbnames, DB_TASK *task)
{
    DB_TCHAR        dbfile[FILENMLEN * 2];
    DB_TCHAR       *path;
    DB_TCHAR       *name;
    const DB_TCHAR *cp;
    register int    dbt_lc;                /* loop control */
    register int    i;


    /* compute number of databases to be opened */
    task->old_no_of_dbs = (short) ((!task->db_table) ? 0 : task->no_of_dbs);
    ++task->no_of_dbs;
    for (cp = dbnames; *cp; ++cp)
    {
        if (*cp == DB_TEXT(';'))
            ++task->no_of_dbs;
    }

    /* allocate task->db_table space */
    if (alloc_table((void **) &task->db_table,
                    task->no_of_dbs * sizeof(DB_ENTRY),
                    task->old_no_of_dbs * sizeof(DB_ENTRY),
                    task) != S_OKAY)
        return task->db_status;

    if (alloc_table((void **) &task->rn_table,
                    task->no_of_dbs * sizeof(RN_ENTRY),
                    task->old_no_of_dbs * sizeof(RN_ENTRY),
                    task) != S_OKAY)
        return task->db_status;

    /* initialize task->db_table entries */
    for (dbt_lc = task->no_of_dbs, cp = dbnames,
         task->curr_db_table = &task->db_table[task->old_no_of_dbs];
         --dbt_lc >= task->old_no_of_dbs;
         ++cp, ++task->curr_db_table)
    {
        /* extract database name */
        for (i = 0; *cp && *cp != DB_TEXT(';'); ++cp, ++i)
        {
            if (i >= FILENMLEN)
                return dberr(S_NAMELEN);

            dbfile[i] = *cp;
        }

        dbfile[i] = 0;

        /* make sure database is not already open */
        if (ddbnum(dbfile, task) != S_INVDB)
            return task->db_status;

        task->db_status = S_OKAY;     /* set by ddbnum() */
        psp_pathSplit(dbfile, &path, &name);
        if (path)
        {
            vtstrcpy(DB_REF(db_path), path);
            psp_freeMemory(path, 0);
        }
        else
            vtstrcpy(DB_REF(db_path), DB_TEXT(""));

        if (name)
        {
            vtstrcpy(DB_REF(db_name), name);
            psp_freeMemory(name, 0);
        }
    }

    return task->db_status;
}


static DB_TCHAR defDbtafPath[3] = { DB_TEXT('.'), DIRCHAR, DB_TEXT('\0') };
/* ======================================================================
    Check for possible external recovery
*/
int INTERNAL_FCN recovery_check(DB_TASK *task)
{
#ifdef MULTI_TAFFILE
    PSP_DIR   dir = NULL;
    DB_TCHAR *loc = NULL;
    DB_TCHAR *name;
    DB_TCHAR *original = NULL;
#endif /* MULTI_TAFFILE */

    if (task->dboptions & NORECOVER ||
            psp_lmcFlags(task->lmc) & PSP_FLAG_NO_TAF_ACCESS)
        return S_OKAY;

#ifdef MULTI_TAFFILE
    if (task->dboptions & MULTITAF)
    {
        /* There may be multiple TAF files in the TAF directory. Get each
           TAF file name and copy it into task->dbtaf, then do the recovery
           check for all log files in that TAF file. At the end, replace the
           original TAF file name.  */

        original = psp_strdup(task->dbtaf, 0);
        psp_pathSplit(task->dbtaf, &name, NULL);
        if (name == task->dbtaf)
        {
            /* If there is no subdir data in the dbtaf path, specify the
               current directory */
            psp_freeMemory(name, 0);
            name = psp_strdup(defDbtafPath, 0);
        }

        dir = psp_pathOpen(name, DB_TEXT("*.taf"));
        psp_freeMemory(name, 0);
        if (!dir)
        {
            psp_freeMemory(original, 0);
            return (dberr(S_DBLACCESS));
        }

        loc = psp_pathGetFile(task->dbtaf);
    }

    for ( ; ; )
    {
        if (task->dboptions & MULTITAF)
        {
            if ((name = psp_pathNext(dir)) == NULL)
                break;

            vtstrncpy(loc, name, FILENMLEN - (loc - task->dbtaf));
            task->dbtaf[FILENMLEN - 1] = 0;
            psp_freeMemory(name, 0);
        }
#endif /* MULTI_TAFFILE */

        /* get logfile count */
        if (!(psp_lmcFlags(task->lmc) & PSP_FLAG_NO_TAF_ACCESS))
        {
            if (taf_access(DEL_LOGFILE, task) == S_OKAY)
            {
                taf_release(0, task);
                /* release so 'throw' in dbautorec isn't catastrophic since */
                /* lockmgr holds off other users, tafbuf.cnt will be stable */

                if (tafbuf.cnt)
                {
                    dbautorec(task);   /* notify user of recovery */
                    if (taf_access(DEL_LOGFILE, task) == S_OKAY)
                    {
                         /* perform recovery on each file */
                         while (tafbuf.cnt)
                         {
                             /* taf_del() will readjust tafbuf.files & cnt;
                                0 needing recovery */
                            if (drecover(tafbuf.files[0], task) != S_OKAY)
                            {
                                taf_release(0, task);
                                return (dberr(S_RECFAIL));
                            }

                            taf_del(tafbuf.files[0], task);
                        }

                        taf_release(0, task);
                    }
                }
            }
        }

#ifdef MULTI_TAFFILE
        if (!(task->dboptions & MULTITAF))
            break;
    }

    if (task->dboptions & MULTITAF)
    {
        psp_pathClose(dir);
        vtstrcpy(task->dbtaf, original);
        psp_freeMemory(original, 0);
    }
#endif /* MULTI_TAFFILE */

    return task->db_status;
}

/* ======================================================================
    Initial lock manager session
*/
static int INTERNAL_FCN initses(int opentype, DB_TASK *task)
{
    LM_DBOPEN     *send_pkt = NULL;
    LR_DBOPEN     *recv_pkt = NULL;
    LM_LOGIN      *login_pkt = NULL;
    FILE_NO       *fref_ptr = NULL;
    short         *rcv_fref_ptr = NULL;
    int            ii;
    int            stat;
    int            ft_lc;
    int            status;
    unsigned short send_size;
    FILE_ENTRY    *file_ptr = NULL;
    DB_TCHAR      *tptr = NULL;
    DB_TCHAR     **names;
    int           *excl_ptr;
    int            count;
    unsigned short nfiles = 0;
#if defined(MULTI_TAFFILE)
    DB_TCHAR       taffile[FILENMLEN];
    DB_TCHAR      *p;
#endif

    if (!task->session_active)
    {
        stat = psp_lmcConnect(task->lockmgrn, task->dbuserid, task->dbtmp,
                task->lmc);
        if (stat != PSP_OKAY)
        {
             if (stat == PSP_DUPUSERID)
                 return (dberr(S_DUPUSERID));

             return (dberr(S_NOLOCKMGR));
        }

        if ((login_pkt = psp_lmcAlloc(sizeof(LM_LOGIN))) == NULL)
        {
            dberr(S_NOMEMORY);
            goto ret_err;
        }

        vtstrcpy(login_pkt->dbuserid, task->dbuserid);

#if defined(MULTI_TAFFILE)
        vtstrcpy(taffile, task->dbtaf);
        if (task->dboptions & MULTITAF)
        {
            /* If multiple TAF files are allow, send the TAF directory to the
               lock manager, not the filename, as the lock manager will not
               allow discrepancies in TAF file names */
           if ((p = vtstrrchr(taffile,  DIRCHAR)) == NULL)
               vtstrcpy(taffile, DB_TEXT("."));
           else
               *p = DB_TEXT('\0');
        }

        if ((tptr = psp_truename(taffile, 0)) == NULL)
#else
        if ((tptr = psp_truename(task->dbtaf, 0)) == NULL)
#endif
        {
            dberr(S_NOFILE);
            goto ret_err;
        }

        vtstrcpy(login_pkt->taffile, tptr);
        psp_freeMemory(tptr, 0);

        stat = psp_lmcTrans(L_LOGIN, login_pkt, sizeof(LM_LOGIN), NULL, NULL,
                &status, task->lmc);
        if (stat != PSP_OKAY || status != L_OKAY) {
            psp_lmcDisconnect(task->lmc);
            if (status == L_DUPUSER)
                return dberr(S_DUPUSERID);

            return dberr(S_NOLOCKMGR);
        }
    
        task->session_active = TRUE;
    }

    send_size = 0;
    ft_lc = task->size_ft - task->old_size_ft;
    if ((names = psp_getMemory(ft_lc * sizeof(DB_TCHAR *), 0)) == NULL)
        goto ret_err;

    nfiles = 0;
    file_ptr = &task->file_table[task->old_size_ft];
    for (ii = 0; ii < ft_lc; ii++, file_ptr ++)
    {
        if (!(file_ptr->ft_flags & TEMPORARY))
        {
            if ((names[nfiles] = psp_truename(file_ptr->ft_name, 0)) == NULL)
            {
                while (nfiles)
                    psp_freeMemory(names[--nfiles], 0);

                psp_zFreeMemory(&names, 0);
                goto ret_err;
            }

            /* file name compression - if the first n characters of this
               filename are the same as the first n characters of the last
               filename, they are not sent - they are replace by a single
               character (DB_TCHAR) with n in it (even if n is zero) */

           if (nfiles == 0)
               count = 0;
           else
               count = cmpfiles(names[nfiles], names[nfiles - 1]);

           send_size += (vtstrlen(names[nfiles++]) + 2 - count) *
                   sizeof(DB_TCHAR);
        }
    }

    if (send_size == 0)
        return (task->db_status = S_OKAY); /* nothing to do */

    send_size += sizeof(LM_DBOPEN);
    send_size += WORDPAD(send_size);  /* pad to word boundry */
    if (send_size > MAX_ALLOC)
    {
        dberr(S_NOMEMORY);
        goto ret_err;
    }

    if ((send_pkt = (LM_DBOPEN *) psp_lmcAlloc(send_size)) == NULL)
    {
        dberr(S_NOMEMORY);
        goto ret_err;
    }

    send_pkt->nfiles = nfiles;
    send_pkt->type = (short)task->type[0];

    tptr = send_pkt->fnames;
    for (ii = 0; ii < nfiles; ii++)
    {
        if (ii)
        {
            count = cmpfiles(names[ii], names[ii - 1]);
            psp_freeMemory(names[ii - 1], 0);
        }
        else
            count = 0;

        *tptr++ = (DB_TCHAR) count;
        vtstrcpy(tptr, names[ii] + count);
        tptr += vtstrlen(names[ii] + count) + 1;
    }

    psp_freeMemory(names[ii - 1], 0);
    psp_freeMemory(names, 0);

    msg_trans(L_DBOPEN, send_pkt, send_size, (void **) &recv_pkt, NULL, task);
    if (task->db_status != S_OKAY) {
        psp_lmcFree(send_pkt);
        goto ret_err;
    }

    excl_ptr = &task->excl_locks[task->old_size_ft];
    fref_ptr = &task->file_refs[task->old_size_ft];
    file_ptr = &task->file_table[task->old_size_ft];
    rcv_fref_ptr = recv_pkt->frefs;
    for (ii = 0; ii <  ft_lc; ii++, fref_ptr++, excl_ptr++)
    {
        if (!(file_ptr->ft_flags & TEMPORARY))
            *fref_ptr = *rcv_fref_ptr++;
        else if (task->dbopen == 1)
        {
            *fref_ptr = (FILE_NO) -1;
            ++(*excl_ptr);
        }
    }

    psp_lmcFree(recv_pkt);

    if (opentype == FIRST_OPEN && task->db_timeout != DB_TIMEOUT)
        dtimeout(task->db_timeout, task);

    return task->db_status;

ret_err:
    if (opentype == FIRST_OPEN)
    {
        int s1 = task->db_status;

        psp_lmcDisconnect(task->lmc);
        task->lmc = NULL;
        task->session_active = FALSE;
        task->db_status = s1;
    }

    return task->db_status;
}


/* ======================================================================
    Build application file lock tables
*/
static int INTERNAL_FCN bld_lock_tables(DB_TASK *task)
{
    register int        fd_lc;                 /* loop control */
    register int        st_lc;                 /* loop control */
    int                *file_used;
    int                 rec;
    int                 mem,
                        memtot;
    register FILE_NO    i;
    FILE_NO             fl_cnt;
    struct lock_descr  *ld_ptr;
    RECORD_ENTRY       *rec_ptr;
    FIELD_ENTRY        *fld_ptr;
    SET_ENTRY          *set_ptr;
    MEMBER_ENTRY       *mem_ptr;
    register int       *fu_ptr;
    FILE_NO            *fl_ptr;
    unsigned            new_size;
    unsigned            old;
    int                 old_keyl_cnt;

    new_size = task->size_ft * sizeof(int);
    old = task->old_size_ft * sizeof(int);

    if (alloc_table((void **) &task->app_locks, new_size, old, task) != S_OKAY)
        return task->db_status;

    if (alloc_table((void **) &task->excl_locks, new_size, old, task) != S_OKAY)
        return task->db_status;

    if (alloc_table((void **) &task->kept_locks, new_size, old, task) != S_OKAY)
        return task->db_status;

    if ((file_used = psp_cGetMemory(new_size, 0)) == NULL)
        return (dberr(S_NOMEMORY));

    new_size = task->size_rt * sizeof(struct lock_descr);
    old      = task->old_size_rt * sizeof(struct lock_descr);
    if (alloc_table((void **) &task->rec_locks, new_size, old, task) != S_OKAY)
        return task->db_status;

    if (task->size_st)
    {
        new_size = task->size_st * sizeof(struct lock_descr);
        old = task->old_size_st * sizeof(struct lock_descr);
        if (alloc_table((void **) &task->set_locks, new_size, old,
                        task) != S_OKAY)
            return task->db_status;
    }

    /* build task->rec_locks table */
    for (rec = task->old_size_rt, rec_ptr = &task->record_table[task->old_size_rt],
         ld_ptr = &task->rec_locks[task->old_size_rt];
         rec < task->size_rt;
         ++rec, ++rec_ptr, ++ld_ptr)
    {
        ld_ptr->fl_type = 'f';
        ld_ptr->fl_prev = 'f';           /* [367] init to free */
        ld_ptr->fl_kept = FALSE;

        /* put record's data file in list */
        file_used[rec_ptr->rt_file] = TRUE;

        /* add any key files to list */
        fl_cnt = 1;                      /* count of used files */
        for ( fd_lc = task->size_fd - rec_ptr->rt_fields,
                  fld_ptr = &task->field_table[rec_ptr->rt_fields];
              (--fd_lc >= 0) && (fld_ptr->fd_rec == rec);
              ++fld_ptr)
        {
            if (fld_ptr->fd_key != NOKEY)
            {
                fu_ptr = &file_used[fld_ptr->fd_keyfile];
                if (!*fu_ptr)
                {
                    *fu_ptr = TRUE;
                    ++fl_cnt;
                }
            }
        }

        ld_ptr->fl_cnt = fl_cnt;
        if (ld_ptr->fl_list)
            psp_freeMemory(ld_ptr->fl_list, 0);

        ld_ptr->fl_list = (FILE_NO *) psp_getMemory(fl_cnt * sizeof(FILE_NO), 0);
        if (ld_ptr->fl_list == NULL)
            return (dberr(S_NOMEMORY));

        fl_ptr = ld_ptr->fl_list;
        for (i = 0, fu_ptr = file_used; i < task->size_ft; ++i, ++fu_ptr)
        {
            if (*fu_ptr)
            {
                *fu_ptr = FALSE;
                *fl_ptr++ = i;
            }
        }
    }

    /* build task->set_locks table */
    if (task->size_st)
    {
        for (st_lc = task->size_st - task->old_size_st, set_ptr = &task->set_table[task->old_size_st],
             ld_ptr = &task->set_locks[task->old_size_st];
             --st_lc >= 0; ++set_ptr, ++ld_ptr)
        {
            /* add owner's data file */
            file_used[task->record_table[set_ptr->st_own_rt].rt_file] = TRUE;
            ld_ptr->fl_type = 'f';
            ld_ptr->fl_prev = 'f';        /* [367] init to free */
            ld_ptr->fl_kept = FALSE;

            /* add member record data files to list */
            fl_cnt = 1;                   /* count of used files */
            for ( mem = set_ptr->st_members, memtot = mem + set_ptr->st_memtot,
                      mem_ptr = &task->member_table[mem];
                  mem < memtot;
                  ++mem, ++mem_ptr)
            {
                fu_ptr = &file_used[task->record_table[mem_ptr->mt_record].rt_file];
                if (!*fu_ptr)
                {
                    *fu_ptr = TRUE;
                    ++fl_cnt;
                }
            }

            ld_ptr->fl_cnt = fl_cnt;
            if (ld_ptr->fl_list)
                psp_freeMemory(ld_ptr->fl_list, 0);

            ld_ptr->fl_list = (FILE_NO *) psp_getMemory(fl_cnt * sizeof(FILE_NO), 0);
            if (ld_ptr->fl_list == NULL)
                return (dberr(S_NOMEMORY));

            fl_ptr = ld_ptr->fl_list;
            for (i = 0, fu_ptr = file_used; i < task->size_ft; ++i, ++fu_ptr)
            {
                if (*fu_ptr)
                {
                    *fu_ptr = FALSE;
                    *fl_ptr++ = i;
                }
            }
        }
    }

    /* build task->key_locks table */
    old_keyl_cnt = task->keyl_cnt;
    for ( fd_lc = task->size_fd - task->old_size_fd, fld_ptr = &task->field_table[task->old_size_fd];
          --fd_lc >= 0;
          ++fld_ptr)
    {
        /* count number of keys */
        if (fld_ptr->fd_key != NOKEY)
            ++task->keyl_cnt;
    }

    if (task->keyl_cnt > old_keyl_cnt)
    {
        new_size = task->keyl_cnt * sizeof(struct lock_descr);
        old      = old_keyl_cnt * sizeof(struct lock_descr);
        if (alloc_table((void **) &task->key_locks, new_size, old,
                        task) != S_OKAY)
            return task->db_status;

        for ( fd_lc = task->size_fd - task->old_size_fd,
              fld_ptr = &task->field_table[task->old_size_fd],
              ld_ptr = &task->key_locks[old_keyl_cnt];
              --fd_lc >= 0;
              ++fld_ptr)
        {
            if (fld_ptr->fd_key != NOKEY)
            {
                ld_ptr->fl_type = 'f';
                ld_ptr->fl_prev = 'f';
                ld_ptr->fl_kept = FALSE;
                ld_ptr->fl_cnt = 1;
                if (ld_ptr->fl_list)
                    psp_freeMemory(ld_ptr->fl_list, 0);

                ld_ptr->fl_list = (FILE_NO *) psp_getMemory(ld_ptr->fl_cnt *
                        sizeof(FILE_NO), 0);
                if (ld_ptr->fl_list == NULL)
                    return (dberr(S_NOMEMORY));

                *(ld_ptr->fl_list) = fld_ptr->fd_keyfile;
                ++ld_ptr;
            }
        }
    }

    if (alloc_lfpkts(task) != S_OKAY)
        return (dberr(S_NOMEMORY));

    psp_freeMemory(file_used, 0);

    return (task->db_status);
}




/* ======================================================================
    Incrementally close database
*/
int INTERNAL_FCN diclose(DB_TASK *task, int dbn)
{
    FILE_NO   file;
    DB_ENTRY *db_ptr = NULL;

    if (dbn == CURR_DB)
        dbn = task->curr_db;

    if (! task->dbopen)
        return (task->db_status);

    /* in case they forgot to end the transaction */
    if (task->trans_id[0])
        dtrabort(task);
    else
        dio_flush(task);      /* flush any x-locked files */

    /* release any non x-locks and clear the cache */
    free_dblocks(dbn, task);  /* free locks first so they get cleared */
    dio_clear(dbn, task);
    dio_ixclear(task);

    if (task->no_of_dbs == 1)
        dbn = ALL_DBS;

    if (dbn >= 0)
        db_ptr = &task->db_table[dbn];

    /* close all files */
    for ( file = (FILE_NO) ((dbn == ALL_DBS) ? 0 : db_ptr->ft_offset);
          file < ((dbn == ALL_DBS) ?
          task->size_ft : db_ptr->Size_ft + db_ptr->ft_offset);
          ++file)
        dio_close(file, task);

    dio_close(task->ov_file, task);

    key_close(dbn, task);
    dio_free(dbn, task);  /* must be before o_free() due to task->ov_file reference */
    o_free(dbn, task);

    if (task->db_lockmgr)
        termses(dbn, task);

    termfree(dbn, task);

    /* A d_setkey/d_makenew sequence cannot encompass a d_close */
    sk_free(task);

    if (dbn == ALL_DBS)
    {
        int ret_err = 0;

        /* free the country table */
        if (task->ctbl_activ)
            ctbl_free(task);

        /*  logout of taf before resetting task->db_lockmgr and task->dbopen */
        if (!(task->dboptions & NORECOVER) && taf_logout(task) != S_OKAY)
            ret_err = 1;

        if (task->dboptions & DELETELOG)
        {
            psp_fileRemove(task->dblog);
            if (psp_errno() == EACCES)
                return (dberr(S_EACCESS));
        }

        task->cr_time = 0;
        task->curr_db = VOID_DB;
        task->set_db = VOID_DB;
        task->no_of_dbs = 0;
        task->dbwait_time = 1;
        task->db_lockmgr = 0;
        task->session_active = FALSE;
        task->cache_ovfl = FALSE;
        task->ov_initaddr = 0L;
        task->ov_rootaddr = 0L;
        task->ov_nextaddr = 0L;
        task->size_ft = 0;
        task->size_rt = 0;
        task->size_fd = 0;
        task->size_st = 0;
        task->size_mt = 0;
        task->size_srt = 0;
        task->size_kt = 0;
        task->no_of_keys = 0;
        task->dbopen = 0;
        task->page_size = 0;
        task->curr_rec = NULL_DBA;

        if (ret_err)
            return (task->db_status);
    }
    else
    {
        /* incremental close, adjust current database information */
        task->no_of_dbs--;

        /* if the current database if less than the closed database,
            no adjustment is necessary
        */
        if (task->curr_db >= dbn)
        {
            if (task->curr_db > dbn)
                task->curr_db--;
            else
            {
                /* if task->curr_db was the one closed, set to 0 */
                task->curr_db = 0;
            }

            task->curr_db_table = &task->db_table[task->curr_db];
            task->curr_rn_table = &task->rn_table[task->curr_db];
            task->curr_rec = task->curr_db_table->curr_dbt_rec;
        }

        if (task->set_db >= dbn)
        {
            if (task->set_db > dbn)
                task->set_db--;
            else
            {
                /* if task->set_db was the one closed, set to 0 */
                task->set_db = 0;
            }
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Terminate lock manager session
*/
static int INTERNAL_FCN termses(int dbn, DB_TASK *task)
{
    LM_DBCLOSE       *send_pkt;
    register int      ft_lc;
    FILE_ENTRY       *file_ptr;
    int               total;
    unsigned short    send_size;
    register FILE_NO *fref_ptr;
    register short   *snd_fref_ptr;
    int               low, high;

    if (task->session_active)
    {
        if (dbn == ALL_DBS)
        {
            low  = 0;
            high = task->size_ft;
        }
        else
        {
            low  = task->db_table[dbn].ft_offset;
            high = task->db_table[dbn].Size_ft;
        }

        for (total = 0, ft_lc = high, file_ptr = &task->file_table[low];
             --ft_lc >= 0; ++file_ptr)
        {
            if (!(file_ptr->ft_flags & TEMPORARY))
                ++total;
        }

        send_size = (unsigned short)sizeof(LM_DBCLOSE) + (total - 1) * sizeof(short);
        send_pkt = (LM_DBCLOSE *) psp_lmcAlloc(send_size);
        if (send_pkt == NULL)
            return (dberr(S_NOMEMORY));

        /* copy the file reference numbers into packet */
        for ( ft_lc = high, fref_ptr = &task->file_refs[low],
              snd_fref_ptr = send_pkt->frefs, file_ptr = &task->file_table[low];
              --ft_lc >= 0; ++fref_ptr, ++file_ptr)
        {
            if (!(file_ptr->ft_flags & TEMPORARY))
                *snd_fref_ptr++ = *fref_ptr;
        }

        send_pkt->nfiles = (short) total;

        msg_trans(L_DBCLOSE, send_pkt, send_size, NULL, NULL, task);

        psp_lmcFree(send_pkt);

        /* close the session with the lockmgr only if final close */
        if (dbn == ALL_DBS)
        {
            msg_trans(L_LOGOUT, NULL, 0, NULL, NULL, task);
            psp_lmcDisconnect(task->lmc);
        }
    }

    return (task->db_status);
}

/* ======================================================================
    Free all allocated memory upon termination
*/
void INTERNAL_FCN termfree(int dbn, DB_TASK *task)
{
    int                 low;
    int                 high;
    int                 total;
    register int        i;
    int                 j;
    FILE_NO            *fl_ptr;
    struct lock_descr  *ld_ptr;
    FIELD_ENTRY        *fld_ptr;
    int                 fd_lc;
    int                 extra_file = 0;          /* for ONE_DB */
    int                 temp;
    DB_ENTRY           *db_ptr;
    DB_ENTRY           *close_db_ptr;

    extra_file = 1;

    /* free all allocated memory */
    if (dbn == ALL_DBS)
    {
        low = 0;
        high = task->size_st;
        total = task->size_st;
    }
    else
    {
        low = task->db_table[dbn].st_offset;
        high = low + task->db_table[dbn].Size_st;
        total = task->size_st;
    }

    if (task->curr_mem)
    {
        free_table((void **) &task->curr_mem, low, high, total, sizeof(DB_ADDR),
                task);
    }

    if (task->curr_own)
    {
        free_table((void **) &task->curr_own, low, high, total, sizeof(DB_ADDR),
                task);
    }

    if (task->co_time)
    {
        free_table((void **) &task->co_time, low, high, total, sizeof(DB_ULONG),
                task);
    }

    if (task->cm_time)
    {
        free_table((void **) &task->cm_time, low, high, total, sizeof(DB_ULONG),
                task);
    }

    if (task->cs_time)
    {
        free_table((void **) &task->cs_time, low, high, total, sizeof(DB_ULONG),
                task);
    }

    if (task->set_table)
    {
        if (total > high && high >= 0)
        {
            for (i = high; i < total; i++)
            {
                task->set_table[i].st_own_rt -= task->db_table[dbn].Size_rt;
                task->set_table[i].st_members -= task->db_table[dbn].Size_mt;
            }
        }

        free_table((void **) &task->set_table, low, high, total,
                sizeof(SET_ENTRY), task);
        if (task->set_names)
        {
            free_table((void **) &task->set_names, low, high, total,
                    sizeof(DB_TCHAR *), task);
        }
    }

    if (task->set_locks)
    {
        for (i = (dbn == ALL_DBS) ? 0 : task->db_table[dbn].st_offset,
              ld_ptr = &task->set_locks[i];
              i < ((dbn == ALL_DBS)
                ? task->size_st : task->db_table[dbn].st_offset + task->db_table[dbn].Size_st);
              ++i, ++ld_ptr)
        {
            psp_freeMemory(ld_ptr->fl_list, 0);
            ld_ptr->fl_list = NULL;
        }

        free_table((void **) &task->set_locks, low, high, total,
                sizeof(struct lock_descr), task);
    }

    if (dbn == ALL_DBS)
        task->size_st = 0;
    else
    {
        task->size_st -= task->db_table[dbn].Size_st;
        if (task->size_st < 0)
            task->size_st = 0;

        if (task->set_locks)
        {
            i = task->db_table[dbn].st_offset;
            for (ld_ptr = &task->set_locks[i]; i < task->size_st; i++, ld_ptr++)
            {
                fl_ptr = ld_ptr->fl_list;
                for (j = 0; j < ld_ptr->fl_cnt; j++, fl_ptr++)
                    *fl_ptr = (FILE_NO) ((*fl_ptr) - task->db_table[dbn].Size_ft);
            }
        }
    }

    /* All the above tables - Set_locks, Curr_mem, Curr_own, Co_time, Cm_time,
        and Cs_time - are the same size as the set table. The member table is
        not if there are any multi-member sets.
    */
    if (task->member_table)
    {
        if (dbn == ALL_DBS)
        {
            low = 0;
            high = task->size_mt;
            total = task->size_mt;
        }
        else
        {
            low = task->db_table[dbn].mt_offset;
            high = low + task->db_table[dbn].Size_mt;
            total = task->size_mt;
        }

        if ((total > high) && (high >= 0))
        {
            for (i = high; i < total; i++)
            {
                task->member_table[i].mt_record -= task->db_table[dbn].Size_rt;
                task->member_table[i].mt_sort_fld -= task->db_table[dbn].Size_srt;
            }
        }

        free_table((void **) &task->member_table, low, high, total,
                sizeof(MEMBER_ENTRY), task);
        if (dbn == ALL_DBS)
            task->size_mt = 0;
        else
            task->size_mt -= task->db_table[dbn].Size_mt;
    }

    if (task->key_locks)
    {
        for ( fd_lc = low = high = 0, fld_ptr = task->field_table, ld_ptr = task->key_locks;
                fd_lc < ((dbn == ALL_DBS) ?
                    task->size_fd : task->db_table[dbn].fd_offset + task->db_table[dbn].Size_fd);
                ++fd_lc, ++fld_ptr)
        {
            if (fld_ptr->fd_key != NOKEY)
            {
                if (dbn == ALL_DBS)
                {
                    high++;
                    psp_freeMemory(ld_ptr->fl_list, 0);
                    ld_ptr->fl_list = NULL;
                    ld_ptr++;
                }
                else if (fd_lc < DB_REF(fd_offset))
                {
                    ld_ptr++;
                    low++;
                }
                else
                {
                    psp_freeMemory(ld_ptr->fl_list, 0);
                    ld_ptr->fl_list = NULL;
                    ld_ptr++;
                    high++;
                }
            }
        }

        high += low;
        total = task->keyl_cnt;
        free_table((void **) &task->key_locks, low, high, total,
                sizeof(struct lock_descr), task);
        if (dbn == ALL_DBS)
            task->keyl_cnt = 0;
        else
        {
            task->keyl_cnt -= (high - low);
            for (ld_ptr = &task->key_locks[low], i = low; i < task->keyl_cnt; i++, ld_ptr++)
            {
                *(ld_ptr->fl_list) =
                    (short) ((*(ld_ptr->fl_list)) - task->db_table[dbn].Size_ft);
            }
        }
    }

    if (dbn == ALL_DBS)
    {
        low = 0;
        high = task->size_srt;
        total = task->size_srt;
    }
    else
    {
        low = task->db_table[dbn].srt_offset;
        high = low + task->db_table[dbn].Size_srt;
        total = task->size_srt;
    }

    if (task->sort_table)
    {
        if ((total > high) && (high >= 0))
        {
            for (i = high; i < total; i++)
            {
                task->sort_table[i].se_fld -= task->db_table[dbn].Size_fd;
                task->sort_table[i].se_set -= task->db_table[dbn].Size_st;
            }
        }

        free_table((void **) &task->sort_table, low, high, total,
                sizeof(SORT_ENTRY), task);
        if (dbn == ALL_DBS)
            task->size_srt = 0;
        else
            task->size_srt -= task->db_table[dbn].Size_srt;
    }

    if (dbn == ALL_DBS)
    {
        low = 0;
        high = task->size_fd;
        total = task->size_fd;
    }
    else
    {
        low = task->db_table[dbn].fd_offset;
        high = low + task->db_table[dbn].Size_fd;
        total = task->size_fd;
    }

    if (task->field_table)
    {
        if (total > high && high >= 0)
        {
            int key_count;

            for (key_count = 0, i = low; i < high; i++)
            {
                if (task->field_table[i].fd_key != NOKEY)
                    key_count++;
            }

            for (i = high; i < total; i++)
            {
                task->field_table[i].fd_keyfile -= task->db_table[dbn].Size_ft;
                task->field_table[i].fd_rec -= task->db_table[dbn].Size_rt;
                if (task->field_table[i].fd_key != NOKEY)
                    task->field_table[i].fd_keyno -= (short) key_count;

                if (task->field_table[i].fd_type == COMKEY)
                    task->field_table[i].fd_ptr -= task->db_table[dbn].Size_kt;
            }
        }

        free_table((void **) &task->field_table, low, high, total,
                sizeof(FIELD_ENTRY), task);
        if (task->field_names)
        {
            free_table((void **) &task->field_names, low, high, total,
                    sizeof(DB_TCHAR *), task);
        }

        if (dbn == ALL_DBS)
            task->size_fd = 0;
        else
            task->size_fd -= task->db_table[dbn].Size_fd;
    }

    if (dbn == ALL_DBS)
    {
        low = 0;
        high = task->size_kt;
        total = task->size_kt;
    }
    else
    {
        low = task->db_table[dbn].kt_offset;
        high = low + task->db_table[dbn].Size_kt;
        total = task->size_kt;
    }

    if (task->key_table)
    {
        if ((total > high) && (high >= 0))
        {
            for (i = high; i < total; i++)
            {
                task->key_table[i].kt_field -= task->db_table[dbn].Size_fd;
                task->key_table[i].kt_key -= task->db_table[dbn].Size_fd;
            }
        }

        free_table((void **) &task->key_table, low, high, total,
                 sizeof(KEY_ENTRY), task);
        if (dbn == ALL_DBS)
            task->size_kt = 0;
        else
            task->size_kt -= task->db_table[dbn].Size_kt;
    }

    if (dbn == ALL_DBS)
    {
        low = 0;
        high = task->size_rt;
        total = task->size_rt;
    }
    else
    {
        low = task->db_table[dbn].rt_offset;
        high = low + task->db_table[dbn].Size_rt;
        total = task->size_rt;
    }

    if (task->record_table)
    {
        if ((total > high) && (high >= 0))
        {
            for (i = high; i < total; i++)
            {
                task->record_table[i].rt_file -= task->db_table[dbn].Size_ft;
                task->record_table[i].rt_fields -= task->db_table[dbn].Size_fd;
            }
        }

        free_table((void **) &task->record_table, low, high, total,
                sizeof(RECORD_ENTRY), task);
        if (task->record_names)
        {
            free_table((void **) &task->record_names, low, high, total,
                    sizeof(DB_TCHAR *), task);
        }
    }

    if (task->rec_locks)
    {
        for ( i = (dbn == ALL_DBS) ? 0 : task->db_table[dbn].rt_offset,
                    ld_ptr = &task->rec_locks[i];
                i < ((dbn == ALL_DBS)
                    ? task->size_rt : task->db_table[dbn].rt_offset + task->db_table[dbn].Size_rt);
                ++i, ++ld_ptr)
        {
            psp_freeMemory(ld_ptr->fl_list, 0);
            ld_ptr->fl_list = NULL;
        }

        free_table((void **) &task->rec_locks, low, high, total,
                sizeof(struct lock_descr),
                task);
    }

    if (dbn == ALL_DBS)
    {
        task->size_rt = 0;
        low = 0;
        high = task->size_ft;
        total = task->size_ft;
    }
    else
    {
        task->size_rt -= task->db_table[dbn].Size_rt;
        if (task->rec_locks)
        {
            for (i = low, ld_ptr = &task->rec_locks[low]; i < task->size_rt; i++, ld_ptr++)
            {
                fl_ptr = ld_ptr->fl_list;
                for (j = 0; j < ld_ptr->fl_cnt; j++, fl_ptr++)
                    *fl_ptr = (FILE_NO) ((*fl_ptr) - task->db_table[dbn].Size_ft);
            }
        }

        low = task->db_table[dbn].ft_offset;
        high = low + task->db_table[dbn].Size_ft;
        total = task->size_ft;
    }

    if (task->file_table)
    {
        if (dbn == ALL_DBS)
        {
            free_table((void **) &task->file_table, low, high + extra_file,
                    total + extra_file, sizeof(FILE_ENTRY), task);
            task->size_ft = 0;
        }
        else
        {
            free_table((void **) &task->file_table, low, high, total + extra_file,
                    sizeof(FILE_ENTRY), task);
            task->size_ft -= task->db_table[dbn].Size_ft;
        }
    }

    if (task->db_lockmgr)
    {                       /* [637] Only alloc task->file_refs for shared open */
        if (task->file_refs)
        {
            free_table((void **) &task->file_refs, low, high, total,
                    sizeof(FILE_NO), task);
        }

        free_table((void **) &task->sgs, low, high, total, sizeof(SG *), task);

        if (task->size_ft)
        {
            if (alloc_lfpkts(task) != S_OKAY)
            {
                dberr(S_NOMEMORY);
                return;
            }
        }
        else
        {
            if (task->lock_pkt)
                psp_lmcFree(task->lock_pkt);

            task->lock_pkt = NULL;

            if (task->free_pkt)
                psp_lmcFree(task->free_pkt);

            task->free_pkt = NULL;
        }
    }

#ifdef DBSTAT
    if (task->file_stats)
    {
        free_table((void **) &task->file_stats, low, high, total,
                sizeof(FILE_STATS), task);
    }
#endif

    if (task->app_locks)
    {
        free_table((void **) &task->app_locks, low, high, total, sizeof(int),
                task);
    }

    if (task->excl_locks)
    {
        free_table((void **) &task->excl_locks, low, high, total, sizeof(int),
                task);
    }

    if (task->kept_locks)
    {
        free_table((void **) &task->kept_locks, low, high, total, sizeof(int),
                task);
    }

    if (dbn == ALL_DBS)
    {
        if (task->lock_pkt)
            psp_lmcFree(task->lock_pkt);

        task->lock_pkt = NULL;

        if (task->free_pkt)
            psp_lmcFree(task->free_pkt);

        task->free_pkt = NULL;
    }

    if (task->db_table)
    {
        if (dbn == ALL_DBS)
        {
            if (task->dboptions & READNAMES)
            {
                for (i = 0; i < task->no_of_dbs; i++)
                {
                    if (task->db_table[i].Objnames)
                        psp_freeMemory(task->db_table[i].Objnames, 0);
                }
            }

            psp_freeMemory(task->db_table, 0);
            task->db_table = NULL;
            task->curr_db_table = NULL;
        }
        else
        {
            if (task->no_of_dbs > dbn + 1)
            {
                temp = task->db_table[dbn + 1].key_offset;
                close_db_ptr = &task->db_table[dbn];
                for (i = dbn + 1; i < task->no_of_dbs; i++)
                {
                    db_ptr = &task->db_table[i];
                    db_ptr->ft_offset -= close_db_ptr->Size_ft;
                    db_ptr->rt_offset -= close_db_ptr->Size_rt;
                    db_ptr->st_offset -= close_db_ptr->Size_st;
                    db_ptr->mt_offset -= close_db_ptr->Size_mt;
                    db_ptr->srt_offset -= close_db_ptr->Size_srt;
                    db_ptr->kt_offset -= close_db_ptr->Size_kt;
                    db_ptr->fd_offset -= close_db_ptr->Size_fd;
                    db_ptr->key_offset -= (short) ((temp -
                            close_db_ptr->key_offset));
                }
            }

            if (task->dboptions & READNAMES)
            {
                if (task->db_table[dbn].Objnames)
                    psp_freeMemory(task->db_table[dbn].Objnames, 0);
            }

            free_table((void **) &task->db_table, dbn, dbn + 1, task->no_of_dbs,
                    sizeof(DB_ENTRY), task);
        }
    }

    if (task->rn_table)
    {
        if (dbn == ALL_DBS)
            psp_freeMemory(task->rn_table, 0);
        else
            free_table((void **) &task->rn_table, dbn, dbn + 1, task->no_of_dbs,
                    sizeof(RN_ENTRY), task);
    }
}

/* ======================================================================
    get lock manager status for a user
*/

int INTERNAL_FCN dlmstat(DB_TCHAR *userid, int *user_stat, DB_TASK *task)
{
    LM_STATUS *send_pkt = NULL;
    short     *recv_pkt = NULL;

    if (!task->dbopen)
        return dberr(S_DBOPEN);

    if (task->dbopen >= 2 || !userid[0])
    {
        *user_stat = U_EMPTY;
        return task->db_status;
    }

    send_pkt = (LM_STATUS *)  psp_lmcAlloc(sizeof(LM_STATUS));
    if (!send_pkt)
        return (dberr(S_NOMEMORY));

    vtstrcpy(send_pkt->username, userid);
    send_pkt->type_of_status = ST_USERSTAT;

    msg_trans(L_STATUS, send_pkt, sizeof(LM_STATUS), (void **) &recv_pkt, NULL,
            task);
    if (task->db_status == S_OKAY)
        *user_stat = *recv_pkt;

    psp_lmcFree(send_pkt);
    if (recv_pkt)
        psp_lmcFree(recv_pkt);

    return task->db_status;
}


/* ======================================================================
    free all locked files
*/
int INTERNAL_FCN dfreeall(DB_TASK *task)
{
    if (!task->dbopen)
        return (dberr(S_DBOPEN));

    if (task->dbopen >= 2)                    /* exclusive access needs no locks */
        return (task->db_status);

    if (task->trans_id[0])
        return (dberr(S_TRFREE));

    dio_flush(task);               /* flush any x-locked files */
    free_dblocks(ALL_DBS, task);

    return (task->db_status);
}




/* ======================================================================
*/
int INTERNAL_FCN alloc_table(
    void   **table,
    size_t   newsz,
    size_t   oldsz,
    DB_TASK *task)
{
    void *temp_table;

    if (oldsz == 0)
        temp_table = psp_cGetMemory(newsz, 0);
    else
    {
        temp_table = psp_cExtendMemory(*table, newsz,
                newsz > oldsz ? newsz - oldsz : 0, 0);
    }

    if (temp_table == NULL)
        return (dberr(S_NOMEMORY));

    *table = temp_table;

    return (task->db_status);
}



/* ======================================================================
    Reallocates array, removing elements from low to high-1 inclusive,
    after copying the contents of the remaining elements into a new array.
*/
int INTERNAL_FCN free_table(void **table, int low, int high, int total,
                            size_t size, DB_TASK *task)
{
    void *temp_table;
    int   new_count;

    if (*table == NULL)
    {
        dberr(S_INVPTR);
        goto EXIT;
    }

    if (total == low || high < 1)
        goto EXIT;

    new_count = total - (high - low);
    if (size == 0 || new_count < 0)
    {
        dberr(SYS_BADFREE);
        goto EXIT;
    }

    if (new_count > 0)
    {
        if ((temp_table = psp_getMemory(new_count * size, 0)) == NULL)
        {
            dberr(S_NOMEMORY);
            goto EXIT;
        }

        if (low > 0)
            memcpy(temp_table, *table, low * size);

        if (new_count - low > 0)
        {
            memcpy((char *) temp_table + (low * size), (char *) *table +
                    (high * size), (new_count - low) * size);
        }
    }

    psp_freeMemory(*table, 0);

    *table = (new_count > 0) ? temp_table : NULL;

EXIT:
    return task->db_status;
}


/* ======================================================================
    free the locks associated with a database
*/
int INTERNAL_FCN free_dblocks(int dbn, DB_TASK *task)
{
    register int      i;
    register int     *appl_ptr;
    register FILE_NO *fref_ptr;
    int               low, high;

    /* exclusive access needs no locks */
    if (task->dbopen >= 2)
        return (task->db_status);

    if (dbn == ALL_DBS)
    {
        low = 0;
        high = task->size_ft;
    }
    else
    {
        low = task->db_table[dbn].ft_offset;
        high = low + task->db_table[dbn].Size_ft;
    }

    task->free_pkt->nfiles = 0;
    for ( i = low, fref_ptr = &task->file_refs[low], appl_ptr = &task->app_locks[low];
            i < high; ++i, ++fref_ptr, ++appl_ptr)
    {
        if (*appl_ptr)
        {
            if (!task->excl_locks[i])
            {
#ifdef DBSTAT
                STAT_lock((FILE_NO)i, *appl_ptr > 0 ? STAT_FREE_r : STAT_FREE_w, task);
#endif
                task->free_pkt->frefs[task->free_pkt->nfiles++] = *fref_ptr;
            }
            *appl_ptr = FALSE;
        }
    }

    if (send_free_pkt(task) != S_OKAY)   /* send free files packet */
        return task->db_status;

    if (dbn == ALL_DBS)                 /* reset all lock descriptors */
        reset_locks(task);

    key_reset(task->size_ft, task);      /* reset all key file positions */

    return task->db_status;
}

/* ======================================================================
    Allocate the lock and free pkts
*/
static int alloc_lfpkts (DB_TASK *task)
{
    task->lp_size = (size_t)(sizeof(LM_LOCK) + (task->size_ft - 1) * sizeof(DB_LOCKREQ));
    task->fp_size = (size_t)(sizeof(LM_FREE) + (task->size_ft - 1) * sizeof(short));

    /* if packets were previously allocated then free them */
    if (task->lock_pkt)
        psp_lmcFree(task->lock_pkt);

    if (task->free_pkt)
        psp_lmcFree(task->free_pkt);

    if ((task->lock_pkt = (LM_LOCK *) psp_lmcAlloc(task->lp_size)) == NULL)
        return S_NOMEMORY;

    if ((task->free_pkt = (LM_FREE *) psp_lmcAlloc(task->fp_size)) == NULL)
    {
        psp_lmcFree(task->lock_pkt);
        return S_NOMEMORY;
    }

    return S_OKAY;
}


/* ======================================================================
    Process renamed file table
*/
int INTERNAL_FCN renfiles(DB_TASK *task)
{
    REN_ENTRY *rp;

    int        dbn;
    DB_ENTRY  *db_ptr;

    if (ll_access(&task->ren_list))
    {
        db_ptr = task->curr_db_table;       /* Have to save it because of macros */
        while ((rp = (REN_ENTRY *) ll_next(&task->ren_list)) != NULL)
        {
            dbn = ddbnum(rp->ren_db_name, task);
            if (dbn < 0)
                continue;

            task->curr_db_table = &task->db_table[dbn];
            if ((rp->file_no < 0) || (rp->file_no >= DB_REF(Size_ft)))
            {
                ll_deaccess(&task->ren_list);
                return (dberr(S_RENAME));
            }

            vtstrcpy(task->file_table[NUM2INT(rp->file_no, ft_offset)].ft_name,
                    rp->file_name);
        }

        task->curr_db_table = db_ptr;       /* restore it */
    }

    ll_deaccess(&task->ren_list);

    return (task->db_status);
}


/* ======================================================================
    Count how many characters are same at start of two strings
*/
static int INTERNAL_FCN cmpfiles(DB_TCHAR *s1, DB_TCHAR *s2)
{
    int count = 0;

    while (*s1 && *s2)
    {
        if (*s1++ != *s2++)
            break;

        count++;
    }

    return count;
}


