/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbcheck utility                                   *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/*-----------------------------------------------------------------------

    dbcheck - db.* Database Consistency Check Utility

    To execute:
        dbcheck [-options] dbname [dbfile(s)]
    Options:
        [-s] [-k] [-dk] [-kd] [-ts] [-a] [-r#] [p#] [-f#] [-t] [-c]
        [-sg [<mode>,]<password>]

    Where:
         -s        = Perform complete set consistency check
         -k        = Perform key file structure consistency check
         -dk       = Perform key access consistency check from data files
         -kd       = Perform key data consisteny check from key files
         -ts       = Perform time stamp checks for records and sets
         -a        = Does -s -dk -kd -ts
         -nk       = Disables -k, -dk, -kd, and causes key files to be ignored
         -r#       = Report every # percent to stderr
         -p#       = Number of pages for db.* to allocate to its page cache
         -f#       = Number of open files db.* is allowed to have
         -t        = Print a traceback of the b-tree at the first sign of disorder
         -c        = Print counts of objects scanned in the check
         -sg       = Specifies the SafeGarde encryption information
         dbname    = Name of database to be checked
         dbfile(s) = If specified check only these files

    If no "-" options are specified, dbcheck merely checks the delete
    chains and verifies that the ids and addresses in each slot are sane.
    The "-" options perform better integrity checks, but can add significant
    run time.

    The -r option is useful when you have a huge database, and you want
    to keep tabs in how far dbcheck has progressed.  The -f and -p options
    can improve speed, at the cost of extra memory.  The -t option is used
    to quickly inspect the tree to find out why it is disordered.  The -c
    option can be useful to determine if the database has application level
    integrity.

-----------------------------------------------------------------------*/

/*********************** INCLUDE FILES ******************************/

#define MOD dbcheck
#include "db.star.h"
#include "dbcache.h"
#include "version.h"

#define DBCHECK
#include "cnvfld.h"

#define ERR_DEFINE
#include "dbcheck.h"

#define MAXTEXT 1600

/* ********************** EXTERNAL VARIABLE DECLARATIONS ************* */
extern char    etext,
                    edata,
                    end;

/* ********************** LOCAL FUNCTION DECLARATIONS **************** */

static void flush(void);

void EXTERNAL_FCN dbcheck_dberr(int errnum, DB_TCHAR *errmsg)
{
    vtprintf(DB_TEXT("\n%s (errnum = %d)\n"), errmsg, errnum);
}

/***************************************************************************/
/***************************************************************************/
/*                             Main Processing                             */
/***************************************************************************/
/***************************************************************************/

int MAIN(int argc, DB_TCHAR *argv[])
{
    int status = S_OKAY;
    long tot_err_cnt = 0;
    char *pgbuf = NULL;
    CHKOPTS *opts = NULL;
    DBINFO *dbi = NULL;
    DB_TASK *task = NULL;  /* dberr and other macros reference
                              a pointer called 'task' */

    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Database Consistency Check")));

    psp_init();

    if ((status = parse_args(argc, argv, &opts)) != S_OKAY) {
        psp_term();
        return 1;
    }

    fflush(stdout);

    psp_handleInterrupt(flush);

    if ((status = d_opentask(&task)) == S_OKAY)
    {
        if ((status = d_set_dberr(dbcheck_dberr, task)) == S_OKAY)
        {
            if ((status = setup(&pgbuf, opts, &dbi, task)) == S_OKAY)
                status = scan(pgbuf, dbi, task);

            fflush(stdout);
            pr_stat(status, dbi);
            if (status == S_OKAY)
                pr_counts(dbi, task);

            if (dbi)                   /* allocated in setup_db */
            {
                if (dbi->errinfo)
                    tot_err_cnt += dbi->errinfo->tot_err_cnt;
                free_dbinfo(dbi);
            }

            if (pgbuf)                 /* allocated in setup_db */
                psp_freeMemory(pgbuf, 0);

            d_close(task);            /* database opened in setup() */
        }
        d_closetask(task);
    }

    psp_handleInterrupt(NULL);
    if (status != S_OKAY)
        tot_err_cnt++;

    if (opts)                        /* allocated in parse_args */
    {
        if (opts->fnames)
            psp_freeMemory(opts->fnames, 0);

        psp_freeMemory(opts, 0);
    }

    psp_term();
    return(tot_err_cnt != 0);
}


/*************************************************************************
    Parse the argument vector
*/   
static int parse_args(int argc, DB_TCHAR **argv, CHKOPTS **ppopts)
{
    register int i;
    CHKOPTS *opts;
#if defined(SAFEGARDE)
    DB_TCHAR *cp;
    DB_TCHAR *password;
    int       mode;
#endif

    if (argc < 2)
    {
        usage();
        return(S_UNAVAIL);
    }

    opts = (CHKOPTS *) psp_cGetMemory(sizeof(CHKOPTS), 0);
    if (!opts)
        return(S_NOMEMORY);

    opts->report = -1;
    opts->numpages = 64;

    opts->fnames = (DB_TCHAR **) psp_cGetMemory(argc * sizeof(DB_TCHAR *), 0);
    if (opts->fnames == NULL)
    {
        psp_freeMemory(opts, 0);
        return(S_NOMEMORY);
    }

    /* process the options */
    for (i = 1; i < argc; i++)
    {
        if (vtstricmp(argv[i], DB_TEXT("-s")) == 0)
            opts->setscan = TRUE;
        else if (vtstricmp(argv[i], DB_TEXT("-k")) == 0)
            opts->keyscan = TRUE;
        else if (vtstricmp(argv[i], DB_TEXT("-dk")) == 0)
            opts->datkey = TRUE;
        else if (vtstricmp(argv[i], DB_TEXT("-kd")) == 0)
            opts->keydat = TRUE;
        else if (vtstricmp(argv[i], DB_TEXT("-ts")) == 0)
            opts->timestmp = TRUE;
        else if (vtstricmp(argv[i], DB_TEXT("-nk")) == 0)
            opts->ignorekey = TRUE;
        else if (vtstrnicmp(argv[i], DB_TEXT("-r"), 2) == 0)
            opts->report = (short) vttoi(argv[i] + 2);
        else if (vtstrnicmp(argv[i], DB_TEXT("-p"), 2) == 0)
            opts->numpages = (short) vttoi(argv[i] + 2);
        else if (vtstrnicmp(argv[i], DB_TEXT("-f"), 2) == 0)
            opts->numfiles = (short) vttoi(argv[i] + 2);
        else if (vtstricmp(argv[i], DB_TEXT("-t")) == 0)
            opts->treetrace = TRUE;
        else if (vtstricmp(argv[i], DB_TEXT("-c")) == 0)
            opts->counts = TRUE;
        else if (vtstricmp(argv[i], DB_TEXT("-a")) == 0)
        {
            opts->keyscan = opts->setscan = opts->datkey =
            opts->keydat = opts->timestmp = TRUE;
        }
        else if (vtstricmp(argv[i], DB_TEXT("-sg")) == 0)
        {
            if (i == argc - 1)
            {
                vftprintf(stderr, DB_TEXT("dbcheck: no password specified\n"));
                usage();
                return S_UNAVAIL;
            }

#if defined(SAFEGARDE)
            if ((cp = vtstrchr(argv[++i], DB_TEXT(','))) != NULL)
                *cp++ = DB_TEXT('\0');

            if (cp)
            {
                if (vtstricmp(argv[i], "low") == 0)
                    mode = LOW_ENC;
                else if (vtstricmp(argv[i], "med") == 0)
                    mode = MED_ENC;
                else if (vtstricmp(argv[i], "high") == 0)
                    mode = HIGH_ENC;
                else
                {
                    vftprintf(stderr, DB_TEXT("dbcheck: invalid encryption mode\n"));
                    usage();
                    return S_UNAVAIL;
                }

                password = cp;
            }
            else
            {
                mode = MED_ENC;
                password = argv[i];
            }

            if ((opts->sg = sg_create(mode, password)) == NULL)
            {
                vftprintf(stderr, DB_TEXT("dbcheck: Could not create SafeGarde context\n"));
                return S_UNAVAIL;
            }
#else
            vftprintf(stderr, DB_TEXT("dbcheck: SafeGarde is not available in this version\n"));
            return S_UNAVAIL;
#endif
        }
        else if (argv[i][0] == DB_TEXT('-'))
        {
            vftprintf(stderr, DB_TEXT("dbcheck: invalid option: %s\n"), argv[i]);
            usage();
            return(S_UNAVAIL);
        }
        else if (opts->dbname == NULL)
            opts->dbname = argv[i];    /* first non-option string is db name */
        else
            opts->fnames[opts->nfnames++] = argv[i];
    }

    if (opts->dbname == NULL)
    {
        usage();
        return(S_UNAVAIL);
    }

    if (opts->ignorekey == TRUE)
    {
        opts->keyscan = opts->datkey = opts->keydat = FALSE;
    }

    *ppopts = opts;

    return(S_OKAY);
}


/***************************************************************************
    Print a usage message
*/
static void usage()
{
    vtprintf(DB_TEXT("usage:  dbcheck [-options] dbname [dbfile(s)]\n"));
    vtprintf(DB_TEXT("options: [-s] [-k] [-dk] [-kd] [-a] [-nk] [-ts] [-r#] [-p#] [-f#] [-t] [-c]\n"));
    vtprintf(DB_TEXT("         [-sg [<mode>,]password]\n\n"));
    vtprintf(DB_TEXT("  -s  = Perform complete set consistency check\n"));
    vtprintf(DB_TEXT("  -k  = Perform key file structure consistency check\n"));
    vtprintf(DB_TEXT("  -dk = Perform key access consistency check from data to key files\n"));
    vtprintf(DB_TEXT("  -kd = Perform key data consistency check from key to data files\n"));
    vtprintf(DB_TEXT("  -a  = Does -s -dk -kd -ts\n"));
    vtprintf(DB_TEXT("  -nk = Disables -k, -dk, -kd, and causes key files to be ignored\n"));
    vtprintf(DB_TEXT("  -ts = Perform time stamp checks for records and sets\n"));
    vtprintf(DB_TEXT("  -r# = Report every # percent to stderr\n"));
    vtprintf(DB_TEXT("  -p# = Max number of pages for its page cache\n"));
    vtprintf(DB_TEXT("  -f# = Max number of open files allowed to have open at once\n"));
    vtprintf(DB_TEXT("  -t  = Print a traceback of the b-tree at the first sign of disorder\n"));
    vtprintf(DB_TEXT("  -c  = Print counts of objects scanned in the check\n"));
    vtprintf(DB_TEXT("  -sg = Specifies SafeGarde encryption information\n"));
    vtprintf(DB_TEXT("         <mode> can be 'low', 'med' (default), or 'high'\n"));
    vtprintf(DB_TEXT("\n"));
}


/***************************************************************************
    Flush the stdout buffer in case only a few errors were found.
*/
static void flush(void)
{
    DB_TCHAR ch;

    fflush(stdout);
    vtprintf(DB_TEXT("[INTERRUPT] Terminate? (y/n) "));

    do {
        ch = vgettchar();
        if (ch == DB_TEXT('y') || ch == DB_TEXT('Y'))
            exit(1);
    } while (ch != DB_TEXT('\n') && ch != DB_TEOF);

    vtprintf(DB_TEXT("Continuing execution\n"));
}


/**************************************************************************
    returns dbd_ver of DBD
*/
static int get_dbd_ver(DB_TCHAR *dbname, char *dbd_ver, DB_TASK *task)
{
    DB_TCHAR dbfile[FILENMLEN] = DB_TEXT("");
    PSP_FH   dbf;
    int      stat = S_OKAY;

    /* form database dictionary name */

    if (vtstrlen(dbname) + vtstrlen(DB_REF(db_name)) >= FILENMLEN + 4)
        return (dberr(S_NAMELEN));

    vtstrcpy(dbfile, dbname);
    vtstrcat(dbfile, DB_TEXT(".dbd"));

    if ((dbf = psp_fileOpen(dbfile, O_RDONLY, PSP_FLAG_DENYNO)) == NULL)
        return (dberr(S_INVDB));

    if (psp_fileRead(dbf, (void *) dbd_ver, DBD_COMPAT_LEN) != DBD_COMPAT_LEN)
        stat = S_OKAY;

    psp_fileClose(dbf);
 
    if (stat != S_OKAY)
        dberr(stat);

    return(stat);
}

/**************************************************************************
    Setup structures used later
*/
static int setup(
    char **ppbuf,
    CHKOPTS *opts,
    DBINFO **ppdb,
    DB_TASK *task)
{
    int status = S_OKAY;

    if ((status = setup_db(ppbuf, opts, ppdb, task)) == S_OKAY)
    {
        if ((status = setup_keys(*ppdb, task)) == S_OKAY)
        {
            if ((status = setup_sets(*ppdb, task)) == S_OKAY)
            {
                if ((status = setup_tots(*ppdb, task)) == S_OKAY)
                {
                    if ((status = setup_cnts(*ppdb, task)) == S_OKAY)
                        status = setup_temp_files(*ppdb, task);
                }
            }
        }
    }
    return status;
}


/***************************************************************************
    Open the database and read the data dictionary
*/
static int setup_db(
    char **ppbuf,
    CHKOPTS *opts,
    DBINFO **ppdb,
    DB_TASK *task)
{
    int      i, j, status = S_OKAY;
    DB_TCHAR dbfile[FILENMLEN];
    PSP_FH   dbf;
    DB_TCHAR filnam[FILENMLEN];
    DBINFO  *dbi;

    /* allocate structure for general database structure information */
    *ppdb = dbi = (DBINFO *) psp_cGetMemory(sizeof(DBINFO), 0);
    if (!dbi)
        return (dberr(S_NOMEMORY));

    dbi->opts = opts;

    if ((status = d_on_opt(NORECOVER, task)) == S_OKAY)
    {
        /* set the number of virtual memory pages for database cache */
        if ((status = d_setpages(opts->numpages, 3, task)) == S_OKAY)
        {
            /* set the number of open files */
            if (opts->numfiles)
            {
                status = d_setfiles(opts->numfiles, task);
            }
            if (status == S_OKAY)
            {
                if ((status = d_on_opt(READNAMES, task)) == S_OKAY)
                {
                    /* open database in one user mode */
                    if ((status = d_open_sg(opts->dbname, DB_TEXT("o"), opts->sg, task)) == S_OKAY)
                    {
                        /* check for existence of all db files */
                        for (i = 0; i < task->size_ft; i++)
                        {
                            /* if name list was specified, search for name */
                            if (opts->nfnames)
                            {
                                for (j = 0; j < opts->nfnames; j++)
                                {
                                    if (!psp_fileNameCmp(task->file_table[i].ft_name, opts->fnames[j]))
                                        break;

                                    con_dbf(filnam, opts->fnames[j], opts->dbname, task->dbfpath, task);
                                    if (!psp_fileNameCmp(task->file_table[i].ft_name, filnam))
                                        break;
                                }
                                if (j == opts->nfnames)
                                    continue;
                            }

                            if (opts->ignorekey && task->file_table[i].ft_type == KEY)
                                continue;

                            vtstrcpy(dbfile, task->file_table[i].ft_name);
                            if ((dbf = psp_fileOpen(dbfile, O_RDONLY, 0)) == NULL)
                            {
                                vftprintf(stderr, DB_TEXT("database file '%s' not found\n"), dbfile);
                                return (dberr(S_NOFILE));
                            }
                            else
                                psp_fileClose(dbf);
                        }

                        /* allocate record/page buffer */
                        *ppbuf = (char *) psp_cGetMemory(task->page_size, 0);
                        if (*ppbuf == NULL)
                            return (dberr(S_NOMEMORY));

                        /* allocate structure for error totals */
                        dbi->errinfo = (ERRINFO *) psp_cGetMemory(sizeof(ERRINFO), 0);
                        if (dbi->errinfo == NULL)
                            return (dberr(S_NOMEMORY));
                    }
                }
            }
        }
    }
    return status;
}


/***************************************************************************
    Get the key information in easy-to-access tables
*/
static int setup_keys(DBINFO *dbi, DB_TASK *task)
{
    int           i, elements, size;
    short         fd, r;
    REC_KEY_INFO *kip;
    FIELD_ENTRY  *kfp;
    ERR_FSIZE     errsz;

    /* allocate record type indicies */
    dbi->keys = (REC_KEY_INFO *) psp_cGetMemory(task->size_rt *
            sizeof(REC_KEY_INFO), 0);
    if (!dbi->keys)
        return (dberr(S_NOMEMORY));

    dbi->num_recs = task->size_rt;  /* needed for freeing arrays after the database
                                       is closed, when task->size_rt is no longer valid */

    /* extract key information */
    for (fd = 0; fd < task->size_fd; fd++)
    {
        kfp = task->field_table + fd;
        if (kfp->fd_key != NOKEY)
        {
            ++(dbi->num_keys);
            dbi->keys[kfp->fd_rec].cnt++;
        }

        /* compute number of array elements */
        for (i = 0, elements = 1; i < MAXDIMS && kfp->fd_dim[i]; i++)
            elements *= kfp->fd_dim[i];
        size = kfp->fd_len / elements;
        errsz.fsize = 0;

        /* check field size */
        switch (kfp->fd_type)
        {
            case CHARACTER:
                if (size != sizeof(char))
                    errsz.fsize = size;
                break;
            case DBADDR:
                if (size != sizeof(DB_ADDR))
                    errsz.fsize = size;
                break;
            case LONGINT:
                if (size != sizeof(long))
                    errsz.fsize = size;
                break;
            case SHORTINT:
                if (size != sizeof(short))
                    errsz.fsize = size;
                break;
            case REGINT:
                if (size != sizeof(int))
                {
                    errsz.fsize = size;
                    if (size == sizeof(short))
                    {
                        /* This database may have been imported from a 16-bit
                           platform - treat the field as if it was a short.
                        */
                        kfp->fd_type = SHORTINT;
                    }
                }
                break;

            case WIDECHAR:
                if (size != sizeof(wchar_t))
                    errsz.fsize = size;
                break;

            case FLOAT:
                if (size != sizeof(float))
                    errsz.fsize = size;
                break;
            case DOUBLE:
                if (size != sizeof(double))
                    errsz.fsize = size;
                break;
        }
        if (errsz.fsize)
        {
            errsz.fname = task->field_names[fd];
            pr_err(BAD_SIZE, &errsz, dbi, task);
        }
    }

    dbi->key_len = (short *) psp_cGetMemory(dbi->num_keys * sizeof(short), 0);
    dbi->key_fld = (short *) psp_cGetMemory(dbi->num_keys * sizeof(short), 0);
    if (! dbi->key_len || ! dbi->key_fld)
        return (dberr(S_NOMEMORY));

    for (r = 0; r < task->size_rt; r++)
    {
        kip = dbi->keys + r;

        if (kip->cnt == 0)
            kip->lst = (short *) psp_cGetMemory(sizeof(short), 0);
        else
            kip->lst = (short *) psp_cGetMemory(kip->cnt * sizeof(short), 0);

        if (! kip->lst)
            return (dberr(S_NOMEMORY));

        kip->lst += kip->cnt;
        kip->byt = (short) BITS_TO_BYTES(kip->cnt);
    }

    for (fd = 0; fd < task->size_fd; fd++)
    {
        kfp = task->field_table + fd;
        if (kfp->fd_key != NOKEY)
        {
            dbi->key_len[kfp->fd_keyno] = kfp->fd_len;
            dbi->key_fld[kfp->fd_keyno] = fd;
        }
    }

    for (fd = (short) (dbi->num_keys - 1); fd >= 0; fd--)
    {
        r = task->field_table[dbi->key_fld[fd]].fd_rec;
        *(--(dbi->keys[r].lst)) = dbi->key_fld[fd];
    }

    return (S_OKAY);
}


/*************************************************************************
    Get the set information in easy-to-access tables
*/
static int setup_sets(DBINFO *dbi, DB_TASK *task)
{
    register short          st,
                            r,
                            i,
                            t;
    register REC_SET_INFO  *oip,
                           *mip;

    /* allocate record type indices */
    dbi->owners = (REC_SET_INFO *) psp_cGetMemory(task->size_rt *
            sizeof(REC_SET_INFO), 0);
    dbi->members = (REC_SET_INFO *) psp_cGetMemory(task->size_rt *
            sizeof(REC_SET_INFO), 0);
    if (!dbi->owners || !dbi->members)
        return (dberr(S_NOMEMORY));

    /* extract set information */
    for (st = 0; st < task->size_st; st++)
    {
        dbi->owners[task->set_table[st].st_own_rt].cnt++;

        for (i = task->set_table[st].st_members;
             i < task->set_table[st].st_members + task->set_table[st].st_memtot; i++)
        {
            dbi->members[task->member_table[i].mt_record].cnt++;
        }
    }

    for (r = 0; r < task->size_rt; r++)
    {
        oip = dbi->owners + r;
        mip = dbi->members + r;

        if (oip->cnt == 0)
            oip->lst = (SET_INFO *) psp_cGetMemory(sizeof(SET_INFO), 0);
        else
            oip->lst = (SET_INFO *) psp_cGetMemory(oip->cnt * sizeof(SET_INFO), 0);

        if (mip->cnt == 0)
            mip->lst = (SET_INFO *) psp_cGetMemory(sizeof(SET_INFO), 0);
        else
            mip->lst = (SET_INFO *) psp_cGetMemory(mip->cnt * sizeof(SET_INFO), 0);

        if (!oip->lst || !mip->lst)
            return (dberr(S_NOMEMORY));

        t = (short) (REC_KEY(0) + (2 * dbi->keys[r].byt));
        for (i = 0; i < oip->cnt; i++, t += (3 * sizeof(long)))
            oip->lst[i].sbfoff = t;
        for (i = 0; i < mip->cnt; i++, t += (3 * sizeof(long)))
            mip->lst[i].mbfoff = t;

        oip->lst += oip->cnt;
        mip->lst += mip->cnt;
    }

    for (st = (short) (task->size_st - 1); st >= 0; st--)
    {
        oip = dbi->owners + task->set_table[st].st_own_rt;
        (--oip->lst)->setno = st;
        oip->lst->setoff = task->set_table[st].st_own_ptr;
        oip->lst->ownrid = task->set_table[st].st_own_rt;
        oip->lst->stampd = (short) (task->set_table[st].st_flags & TIMESTAMPED);

        for (i = task->set_table[st].st_members;
              i < task->set_table[st].st_members + task->set_table[st].st_memtot; i++)
        {
            mip = dbi->members + task->member_table[i].mt_record;
            (--mip->lst)->setno = st;
            mip->lst->setoff = task->set_table[st].st_own_ptr;
            mip->lst->sbfoff = oip->lst->sbfoff;
            mip->lst->ownrid = task->set_table[st].st_own_rt;
            mip->lst->memoff = task->member_table[i].mt_mem_ptr;
        }
    }

    return (S_OKAY);
}


/***************************************************************************
    Summarize the easy-to-access tables into still more easy-to-access tables.
*/
static int setup_tots(DBINFO *dbi, DB_TASK *task)
{
    int            status = S_OKAY;
    short          i;
    F_ADDR         fileSize,
                   pageCount,        /* max pages in file based on size */
                   firstSlot,        /* first slot on last page */
                   lastSlot;         /* last slot on last page */
    ERR_PGZERO     pgz;
    FILE_ENTRY    *filePtr;
    char           dbd_ver[DBD_COMPAT_LEN + 1];

    get_dbd_ver(dbi->opts->dbname, dbd_ver, task);

    dbi->tops = (F_ADDR *) psp_cGetMemory(task->size_ft * sizeof(F_ADDR), 0);
    if (! dbi->tops)
        return (dberr(S_NOMEMORY));

    for (i = 0; i < task->size_ft; i++)
    {
        filePtr = &task->file_table[i];
        if ((filePtr->ft_type != DATA) && (filePtr->ft_type != KEY))
            return (dberr(S_SYSERR));

        if (dbi->opts->ignorekey == TRUE && filePtr->ft_type == KEY)
            continue;

        /* make sure the nextslot values are reasonable */
        dbi->tops[i] = dio_pznext(i, task); /* opens file as side effect! */

        /* make sure file is actually open */
        if (filePtr->ft_status != OPEN)
        {
            if ((status = dio_open(i, task)) != S_OKAY)
                return (dberr(status));
        }

        fileSize = psp_fileLength(filePtr->ft_desc);
        if (fileSize == (F_ADDR) -1L)
            continue;      /* os_filesize failed, assume pageZero is good */

        if (filePtr->ft_type == KEY)
        {
            /* page zero tracks pages not slots for key files */
            pageCount = fileSize / filePtr->ft_pgsize - 1;
            lastSlot  = pageCount + 1;
            firstSlot = lastSlot;
        }
        else
        {
            /* page zero tracks slots for data files */
            pageCount = fileSize / filePtr->ft_pgsize - 1;
            lastSlot = pageCount * filePtr->ft_slots + 1;
            firstSlot = pageCount ? lastSlot - filePtr->ft_slots : 0L;
        }
        if ((dbi->tops[i] < firstSlot) || (dbi->tops[i] > lastSlot))
        {
            pgz.file = i;
            pgz.slot = dbi->tops[i];
            if (!strcmp(dbd_ver, dbd_VERSION_300))
                pr_err(BAD_NEXTSLOT, &pgz, dbi, task); /* not for v3.01 */
            
            /* use a best guess */
            dbi->tops[i] = min(dbi->tops[i], lastSlot);
        }

        /* this must be done AFTER the respective dio_pznext() */
        lastSlot = task->pgzero[i].pz_dchain;
        if (filePtr->ft_type == KEY && lastSlot == DCH_NONE)
            lastSlot = 0L;
        if (lastSlot >= dbi->tops[i])
        {
            pgz.file = i;
            pgz.slot = lastSlot;
            pr_err(BAD_DCHAIN, &pgz, dbi, task);
        }
    }

    return (S_OKAY);
}


/*************************************************************************
    Get the set information in easy to access tables.
*/
static int setup_cnts(DBINFO *dbi, DB_TASK *task)
{
    if (!dbi->opts->counts)
        return (S_OKAY);

    dbi->rec_cnt = (F_ADDR *) psp_cGetMemory(task->size_rt * sizeof(F_ADDR), 0);
    dbi->set_cnt = (F_ADDR *) psp_cGetMemory(task->size_st * sizeof(F_ADDR), 0);
    dbi->key_cnt = (F_ADDR *) psp_cGetMemory(dbi->num_keys * sizeof(F_ADDR), 0);
    if (!dbi->rec_cnt || !dbi->set_cnt || !dbi->key_cnt)
    {
        dbi->opts->counts = FALSE;
        vftprintf(stderr,
                  DB_TEXT("\n\n*** Insufficient memory for aggregating counts\n"));
        return (dberr(S_NOMEMORY));
    }
    return (S_OKAY);
}


/***************************************************************************
    Setup file names for the temporary files.
*/
static int setup_temp_files(DBINFO *dbi, DB_TASK *task)
{
    DB_TCHAR path[256];
    DB_TCHAR *p;
    int len;
    
    if ((dbi->pcache = cache_alloc()) == NULL)
        return(dberr(S_NOMEMORY));

    path[0] = 0;

    p = psp_getenv(DB_TEXT("TMP"));
    if (!p)
        p = psp_getenv(DB_TEXT("TEMP"));

    if (p)
    {
        vtstrcpy(path, p);
        if (!psp_pathIsDir(path))
        {
            len = vtstrlen(path);
            path[len++] = DIRCHAR;
            path[len] = 0;
        }
    }

    vtstrcpy(dbi->pcache->dchain->name, path);
    vtstrcat(dbi->pcache->dchain->name, DB_TEXT("dbcheck.dch"));

    vtstrcpy(dbi->pcache->used->name, path);
    vtstrcat(dbi->pcache->used->name, DB_TEXT("dbcheck.use"));

    return (S_OKAY);
}


/*************************************************************************
    Free heap memory allocated during setup for database info.
*/
static void free_dbinfo(DBINFO *dbi)
{
    int i;

    if (!dbi)
        return;

    if (dbi->pcache)                    /* allocated in setup_temp_files */
        cache_free(dbi->pcache);

    if (dbi->owners)                    /* allocated in setup_sets */
    {
        for (i = 0; i < dbi->num_recs; i++)
        {
            if (dbi->owners[i].lst)
                psp_freeMemory(dbi->owners[i].lst, 0);
        }

        psp_freeMemory(dbi->owners, 0);
    }

    if (dbi->members)                   /* allocated in setup_sets */
    {
        for (i = 0; i < dbi->num_recs; i++)
        {
            if (dbi->members[i].lst)
                psp_freeMemory(dbi->members[i].lst, 0);
        }

        psp_freeMemory(dbi->members, 0);
    }

    if (dbi->keys)                      /* allocated in setup_keys */
    {
        for (i = 0; i < dbi->num_recs; i++)
        {
            if (dbi->keys[i].lst)
                psp_freeMemory(dbi->keys[i].lst, 0);
        }

        psp_freeMemory(dbi->keys, 0);
    }

    if (dbi->key_fld)                   /* allocated in setup_keys */
        psp_freeMemory(dbi->key_fld, 0);

    if (dbi->key_len)                   /* allocated in setup_keys */
        psp_freeMemory(dbi->key_len, 0);

    if (dbi->rec_cnt)                   /* allocated in setup_cnts */
        psp_freeMemory(dbi->rec_cnt, 0);

    if (dbi->set_cnt)
        psp_freeMemory(dbi->set_cnt, 0);

    if (dbi->key_cnt)
        psp_freeMemory(dbi->key_cnt, 0);

    if (dbi->tops)                      /* allocated in setup_tots */
        psp_freeMemory(dbi->tops, 0);

    if (dbi->errinfo)                   /* allocated in setup_db */
        psp_freeMemory(dbi->errinfo, 0);

    psp_freeMemory(dbi, 0);
}


/***************************************************************************
    Scan the data and key files
*/
static int scan(
    char *pgbuf,
    DBINFO *dbi,
    DB_TASK *task)
{
    DB_TCHAR             filnam[FILENMLEN];
    register int         pass;
    register int         i;
    register FILE_NO     fno;
    int                  status = S_OKAY;
    DB_TCHAR            *dbname  = dbi->opts->dbname;
    DB_TCHAR           **fnames  = dbi->opts->fnames;
    short                nfnames = dbi->opts->nfnames;

    for (pass = (dbi->opts->ignorekey ? 1:0); pass < 2; pass++)
    {                                   /* First key, then data files */
        for (fno = 0; fno < task->size_ft; fno++)
        {                                /* Scan each database file */
            if (nfnames)
            {                             /* If name list, search for name */
                for (i = 0; i < nfnames; i++)
                {
                    if (vtstrcmp(task->file_table[fno].ft_name, fnames[i]) == 0)
                        break;

                    con_dbf(filnam, fnames[i], dbname, task->dbfpath, task);
                    if (vtstrcmp(task->file_table[fno].ft_name, filnam) == 0)
                        break;
                }
                if (i == nfnames)
                    continue;
            }

            /* set file number to be specified in error reports */
            dbi->errinfo->fno = fno;

            /* reset number of deleted records / nodes in current file */
            dbi->num_del = 0;

            if ((task->file_table[fno].ft_type == DATA) && (pass == 1))
            {
                if ((status = data_file(fno, pgbuf, dbi, task)) != S_OKAY)
                    return (status);
            }
            else if ((task->file_table[fno].ft_type == KEY) && (pass == 0))
            {
                if ((status = key_file(fno, dbi, task)) != S_OKAY)
                    return (status);
            }
            cache_clear(dbi->pcache);
            d_closeall(task);
        }
    }

    return (S_OKAY);
}


/*************************************************************************/
/*************************************************************************/
/*                         Data File Checks                              */
/*************************************************************************/
/*************************************************************************/

/*************************************************************************
    Perform data file consistency check
*/
static int data_file(
    FILE_NO fno,
    char *pgbuf,
    DBINFO *dbi,
    DB_TASK *task)
{
    int               status = S_OKAY;
    F_ADDR            rno;
    F_ADDR            top;
    DB_ADDR           dba;
    static CHKREC     chk = {  0, 0, BAD_LOCK, BAD_RID, BAD_RDBA, 0,
                                        NULL, 0, 0, 0, 0  };

    stat_report(fno, 0, dbi, task);
    
    if ((status = read_del_chain(fno, dbi, task)) != S_OKAY)
        return (status);

    top = dbi->tops[fno] - 1L;

    /* read each record in data file */
    for (rno = 1; rno <= top; rno++)
    {
        /* set record number to be specified in error reports */
        dbi->errinfo->addr = rno;

        /* report progress */
        stat_report(fno, rno, dbi, task);

        d_encode_dba(fno, rno, &dba);

        if ((status = chk_rec(dba, pgbuf, NULL, &chk, dbi, task)) != S_OKAY)
            return (status);

        if (chk.del)
        {
            if ((status = chk_del_chain(rno, FALSE, dbi, task)) < S_OKAY)
                dberr(status);
        }
        else if (chk.good)
        {
            if (dbi->opts->counts)
                dbi->rec_cnt[chk.rid]++;

            if (dbi->opts->setscan)
            {                             /* check the set connections */
                if ((status = chk_own(pgbuf, chk.rid, dba, dbi, task)) != S_OKAY)
                    return (status);

                if ((status = chk_mem(pgbuf, chk.rid, dba, dbi, task)) != S_OKAY)
                    return (status);

            }
            if (dbi->opts->datkey)        /* check the keys */
            {
                if ((status = chk_key(pgbuf, chk.rid, dba, dbi, task)) != S_OKAY)
                    return (status);
            }
            if (dbi->opts->timestmp)      /* check the timestamps */
                chk_stmp(pgbuf, chk.rid, fno, TRUE, dbi, task);
        }
    }

    delete_temp_files(dbi->pcache);

    return (S_OKAY);
}


/***************************************************************************
    Read a data record, and figure out if it has any errors
*/
static int chk_rec(
    DB_ADDR     dba,     /* database address to check */
    char       *pgptr,
    char       *errblk,
    CHKREC     *chk,
    DBINFO     *dbi,
    DB_TASK    *task)
{
    int                  status = S_OKAY;
    register DB_ADDR    *rdba;
    register DB_BOOLEAN  bad_rid = FALSE;
             FILE_NO     fno;
             DB_ADDR     rno;

    d_decode_dba(dba, &fno, &rno);

    chk->good = FALSE;

    if (chk_dba(dba, dbi, task) != S_OKAY)
    {
        if (chk->dba_msg)
            pr_err(chk->dba_msg, errblk, dbi, task);
        return (S_OKAY);
    }

    /* read next record */
    if ((status = tio_read(dba, &chk->pgptr, dbi, task)) != S_OKAY)
        return (status);

    get_rid_info(chk->pgptr, (RECINFO *) & chk->rid);

    if (pgptr != NULL)
        memcpy(pgptr, chk->pgptr, task->file_table[fno].ft_slsize);

    chk->good = TRUE;

    /* check if deleted */
    if (chk->del_msg && chk->del)
        pr_err(chk->del_msg, errblk, dbi, task);

    /* check the lock bit */
    if (chk->lock_msg && chk->lck)
        pr_err(chk->lock_msg, errblk, dbi, task);

    /* check the record id */
    if (  (chk->rid < 0) || (chk->rid >= task->size_rt) ||
            (task->record_table[chk->rid].rt_file != fno) )
    {
        bad_rid = TRUE;
        chk->good = FALSE;
        if (chk->rid_msg)
        {
            pr_err(chk->rid_msg, (errblk == NULL) ?
                     (char *) &chk->rid : errblk, dbi, task);
        }
    }

    /* check the database address */
    rdba = (DB_ADDR *) (chk->pgptr + REC_DBA);
    if ((! chk->del) && (ADDRcmp(&dba, rdba) != 0))
    {
        chk->good = FALSE;
        if (chk->idba_msg && ((chk->idba_msg != chk->rid_msg) || (! bad_rid)))
        {
            pr_err(chk->idba_msg, (errblk == NULL) ? 
                (void *)rdba : (void *) errblk, dbi, task);
        }
    }

    /* make sure the system record is in slot [x:0001] */
    if (! bad_rid)
    {
        if ((task->record_table[chk->rid].rt_fdtot == -1) && (rno != (DB_ADDR)1))
        {
            /* The slot number of the record being checked by this function
                (i.e. rno) may not be the same as the slot number to be used
                in error reports (i.e. dbi->errinfo->addr).

                For this error, we need to report the problem as being in slot
                rno - so temporarily set the current error-report slot to rno,
                then replace whatever was in there before.
            */
            F_ADDR addr = dbi->errinfo->addr;
            dbi->errinfo->addr = rno;

            pr_err(BAD_SYSR, NULL, dbi, task);

            dbi->errinfo->addr = addr;
            chk->good = FALSE;
        }
    }

    return (S_OKAY);
}


/*************************************************************************
    For each key field, check the key value in the key file
*/
static int chk_key(
    char *pgptr,
    int rid,
    DB_ADDR dba,
    DBINFO *dbi,
    DB_TASK *task)
{
    int                     status = S_OKAY;
    short                   fd;
    register int            i;
    register char          *fptr;
    register FIELD_ENTRY   *kf_ptr;
    register REC_KEY_INFO  *kip;
    char                    key[MAXKEYSIZE];

    kip = dbi->keys + rid;
    for (i = 0; i < kip->cnt; i++)
    {

        fd = kip->lst[i];
        kf_ptr = task->field_table + fd;

        /* try to find the key, and prepare for an error */

        /* make the next key to check */
        if ((status = key_init(fd, task)) != S_OKAY)
            return (status);

        if (kf_ptr->fd_type == COMKEY)
        {
            key_bldcom(fd, pgptr + task->record_table[rid].rt_data, key, TRUE, task);
            fptr = key;
        }
        else
            fptr = pgptr + kf_ptr->fd_ptr;

        status = key_locpos(fptr, &dba, task);

        /* if the key is optional */
        if (kf_ptr->fd_flags & OPTKEYMASK)
        {
            /* if the "stored" bit is set, but key not found */
            if (r_tstopt(kf_ptr, pgptr, task))
            {
                if (status != S_OKAY)
                    pr_err(BAD_MOKEY, &fd, dbi, task);
            }
            /* else if the bit is not set, but the key was found */
            else if (status == S_OKAY)
                pr_err(BAD_OKEY, &fd, dbi, task);
        }
        else if (status != S_OKAY)
        {
            /* a non-optional key must be found */
            pr_err(BAD_MKEY, &fd, dbi, task);
        }
    }

    return (S_OKAY);
}


/***************************************************************************
    For the record and each set this record owns, check the timestamp
*/
static void chk_stmp(
    register char *rec,
    int rid,
    FILE_NO fno,
    DB_BOOLEAN print,
    DBINFO *dbi,
    DB_TASK *task)
{
    register int            i;
    register REC_SET_INFO  *oip;
    register SET_INFO      *sip;
    register DB_ULONG       ts_file;
    DB_ULONG                ts_creat,
                            ts_modif;

    ts_file = task->pgzero[fno].pz_timestamp;

    if (task->record_table[rid].rt_flags & TIMESTAMPED)
    {
        memcpy((char *) &ts_creat, rec + REC_STMP, sizeof(ts_creat));
        memcpy((char *) &ts_modif, rec + REC_STMP + sizeof(ts_creat),
                  sizeof(ts_modif));

        if ((ts_creat > ts_file) || (ts_modif > ts_file))
        {
            if (print)
                pr_err(BAD_RSTP, NULL, dbi, task);
        }
    }

    oip = dbi->owners + rid;
    for (i = 0; i < oip->cnt; i++)
    {                                   /* For sets this rid owns */
        sip = oip->lst + i;
        if (sip->stampd)
        {
            memcpy((char *) &ts_modif, rec + sip->setoff + 3 * sizeof(F_ADDR),
                   sizeof(ts_modif));

            if (ts_modif > ts_file)
            {
                if (print)
                    pr_err(BAD_SSTP, &oip->lst[i].setno, dbi, task);
            }
        }
    }
}


/**************************************************************************/
/**************************************************************************/
/*                             Set Checks                                 */
/**************************************************************************/
/**************************************************************************/

/****************************************************************************
    Check consistency of sets which this record owns
*/
static int chk_own(
    char *orec,
    short orid,
    DB_ADDR odba,
    DBINFO *dbi,
    DB_TASK *task)
{
    int                     status = S_OKAY;
    SET                     set;
    MEMBER                  memptr;
    F_ADDR                  mcnt;
    int                     i;
    ERR_PDBA                pdba;
    ERR_STYP                styp;
    ERR_SCNT                scnt;
    register DB_ADDR        mdba,
                            prev_mdba;
    register REC_SET_INFO  *oip;
    register SET_INFO      *sip;
    register char          *bp;


    static CHKREC chk = {BAD_OMEM, BAD_OMDL, 0, BAD_OMRC, BAD_OMRC, EOF,
                         NULL, 0, 0, 0, 0};

    oip = dbi->owners + orid;
    for (i = 0; i < oip->cnt; i++)
    {                                   /* For sets this rid owns */
        sip = oip->lst + i;

        /* copy the set structure from the record */
        memcpy((char *) &set, orec + sip->setoff, sizeof(SET));
        pdba.setno = styp.setno = scnt.setno = sip->setno;

        if (dbi->opts->counts)
            dbi->set_cnt[sip->setno] += set.members;

        /* check the validity of the first and last pointers */
        if (chk_dba(set.last, dbi, task) != S_OKAY)
        {
            pdba.dba = set.last;
            pr_err(BAD_OLST, &pdba, dbi, task);
        }

        if (chk_dba(set.first, dbi, task) != S_OKAY)
        {
            pdba.dba = set.first;
            pr_err(BAD_OFST, &pdba, dbi, task);
            continue;                     /* Must have good first pointer */
        }

        /* get the DB_ADDR of the first member */
        mcnt = 0;
        mdba = set.first;
        prev_mdba = NULL_DBA;

        /* scan all members in this set */
        while (mdba != NULL_DBA)
        {
            pdba.dba = styp.dba = mdba;

            if (++mcnt > set.members)
            {
                pr_err(BAD_LOOP, &pdba, dbi, task);
                break;
            }
            if ((status = chk_rec(mdba, NULL, (char *) &pdba, &chk, dbi, task)) != S_OKAY)
                return (status);

            if (!chk.good)
            {
                if (prev_mdba != NULL_DBA)
                {
                    pdba.dba = prev_mdba;
                    vtprintf(DB_TEXT("    (last good member-dba=%s)\n"), pr_dba(&pdba.dba));
                }
                break;
            }

            if ((sip = val_mtype(sip->setno, chk.rid, dbi->members, task)) == NULL)
            {
                styp.rid = chk.rid;
                pr_err(BAD_OMTP, &styp, dbi, task);
                break;
            }

            /* obtain the member pointers */
            bp = chk.pgptr + sip->memoff;

            memcpy((char *) &memptr, bp, sizeof(MEMBER));

            /* make sure that the owner pointers match up */
            if (memptr.owner != odba)
                pr_err(BAD_OMOW, &pdba, dbi, task);

            /* if this is the first member, the PREV pointer must be null */
            if ((mdba == set.first) && (memptr.prev != NULL_DBA))
            {
                pr_err(BAD_MFPN, &pdba, dbi, task);
            }
            else if (memptr.prev != prev_mdba)
            {
                /* the previous pointer should point the the previous member */
                pr_err(BAD_MNPW, &pdba, dbi, task);
                pdba.dba = prev_mdba;
                vtprintf(DB_TEXT("    (last good member-dba=%s)\n"), pr_dba(&pdba.dba));
            }

            /* if this is the last member, the NEXT pointer must be null */
            if ((mdba == set.last) && (memptr.next != NULL_DBA))
            {
                pr_err(BAD_MLNN, &pdba, dbi, task);
                break;
            }

            /* point to the next member */
            prev_mdba = mdba;
            mdba = memptr.next;
        }                                /* end scan all members in this set */

        /* verify the membership count */
        if (mcnt != set.members)
        {
            scnt.owncnt = set.members;
            scnt.memcnt = mcnt;
            pr_err(BAD_OCNT, &scnt, dbi, task);
        }
    }                 /* end scan sets which are owned by this record type */

    return (S_OKAY);
}


/***************************************************************************
    Check consistency of sets which this record is a member of
*/
static int chk_mem(
    char *mrec,
    short mrid,
    DB_ADDR mdba,
    DBINFO *dbi,
    DB_TASK *task)
{
    int                     status = S_OKAY;
    MEMBER                  memptr;
    MEMBER                  prev_memptr;
    MEMBER                  next_memptr;
    SET                     set;
    ERR_PDBA                pdba;
    ERR_STYP                styp;
    register int            i;
    register REC_SET_INFO  *mip;
    register SET_INFO      *sip, *tsip;
    register char          *bp;


    static CHKREC chk = {BAD_MOPB, BAD_MODL, 0, BAD_MORC, BAD_MORC, EOF,
                         NULL, 0, 0, 0, 0};

    static CHKREC ptrchk = {0, 0, 0, 0, 0, EOF, NULL, 0, 0, 0, 0};
            /* used to check next and prev pointers w/i set.  Don't report
               errors here because they will be duplicates of an error
               reported elsewhere. */

    mip = dbi->members + mrid;
    for (i = 0; i < mip->cnt; i++)
    {                                   /* Sets of which rid is a member */
        sip = mip->lst + i;

        /* copy member set into structure */
        memcpy((char *) &memptr, mrec + sip->memoff, sizeof(MEMBER));

        pdba.setno = styp.setno = sip->setno;
        pdba.dba = styp.dba = memptr.owner;

        if (memptr.owner == NULL_DBA)
        {                                /* if the owner pointer is null */
            /* if either the next or prev pointers are non-null */
            if (memptr.prev != NULL_DBA || memptr.next != NULL_DBA)
                pr_err(BAD_MONL, &pdba, dbi, task);
        }
        else
        {
            if ((status = chk_rec(memptr.owner, NULL, (char *) &pdba, &chk, dbi, task)) != S_OKAY)
                return (status);

            if (!chk.good)
                continue;

            if (chk.rid != sip->ownrid)
            {
                styp.rid = chk.rid;
                pr_err(BAD_MOIN, &styp, dbi, task);
                continue;
            }

            /* get the set pointers */
            bp = chk.pgptr + sip->setoff;

            memcpy((char *) &set, bp, sizeof(SET));

            /* make sure the first pointer points to this record */
            if (memptr.prev == NULL_DBA)
            {
                if (set.first != mdba)
                     pr_err(BAD_MNPP, &sip->setno, dbi, task);
            }
            else
            {
                /* read the .prev record */
                if ((status = chk_rec(memptr.prev, NULL, (char *) &pdba, &ptrchk, dbi, task)) != S_OKAY)
                     return (status);

                /* check for valid record type */
                if ( (tsip = val_mtype(sip->setno, ptrchk.rid, dbi->members, task)) == NULL)
                {
                     pdba.dba = memptr.prev;
                     pr_err(BAD_OMPP, &pdba, dbi, task);
                     pdba.dba = memptr.owner;
                }
                else
                {
                     /* make sure that prevrecord.next points to this record */
                     memcpy((char *) &prev_memptr, (ptrchk.pgptr + tsip->memoff), sizeof(MEMBER));
                     if (prev_memptr.next != mdba)
                          pr_err(BAD_SETP, &pdba, dbi, task);
                }
            }
            
            /* make sure the last pointer points to this record */
            if (memptr.next == NULL_DBA)
            {
                if (set.last != mdba)
                     pr_err(BAD_MNNP, &sip->setno, dbi, task);
            }
            else
            {
                /* read the .next record */
                if ((status = chk_rec(memptr.next, NULL, (char *) &pdba, &ptrchk, dbi, task)) != S_OKAY)
                     return (status);

                /* check for valid record type */
                if ( (tsip = val_mtype(sip->setno, ptrchk.rid, dbi->members, task)) == NULL)
                {
                     pdba.dba = memptr.next;
                     pr_err(BAD_OMEM, &pdba, dbi, task);
                     pdba.dba = memptr.owner;
                }
                else
                {
                     /* make sure that nextrecord.prev points to this record */
                     memcpy((char *) &next_memptr, (ptrchk.pgptr + tsip->memoff), sizeof(MEMBER));
                     if (next_memptr.prev != mdba)
                          pr_err(BAD_SETN, &pdba, dbi, task);
                }
            }
        }
    }

    return (S_OKAY);
}


/***************************************************************************
    Locate the member table entry for this record type
*/
static SET_INFO *val_mtype(
    int st,
    short mrid,
    REC_SET_INFO *meminfo,
    DB_TASK *task)
{
    register int            i;
    register REC_SET_INFO  *mip;
    register SET_INFO      *sip;

    if (mrid < 0 || mrid >= task->size_rt)
        return (NULL);

    mip = meminfo + mrid;
    for (i = 0; i < mip->cnt; i++)
    {
        sip = mip->lst + i;
        if (sip->setno == st)
            return (sip);
    }

    return (NULL);
}


/***************************************************************************/
/***************************************************************************/
/*                            Key File Checks                              */
/***************************************************************************/
/***************************************************************************/

/**************************************************************************
    Perform key file consistency check
*/
static int key_file(
    FILE_NO fno,
    DBINFO *dbi,
    DB_TASK *task)
{
    int               status;
    register long     m;
    register int      slots;
    F_ADDR            top;
    KEYINFO           keyinfo;

    stat_report(fno, 0, dbi, task);

    if ((status = read_del_chain(fno, dbi, task)) != S_OKAY)
        return (status);

    memset(&keyinfo, 0, sizeof(KEYINFO));
    keyinfo.oldno = -1;
    keyinfo.slot_len = task->file_table[fno].ft_slsize;
    keyinfo.kdup.frst_slot = TRUE;

    top = dbi->tops[fno] - 1L;
    slots = task->file_table[fno].ft_slots;

    for (m = slots / 2, keyinfo.max_levl = 1; m < top * slots; m *= slots / 2)
        keyinfo.max_levl++;

    /* set node number to be specified in error reports */
    dbi->errinfo->addr = 1;

    status = dbi->opts->keyscan ?
        scan_key(fno, ROOT_ADDR, 1, &keyinfo, dbi, task) :
        walk_key(fno, &keyinfo, dbi, task);

    if (status != S_OKAY)
        return (status);

    if (keyinfo.kdup.in_series == TRUE)
    {
        vtprintf(DB_TEXT("   (total of %ld occurrences of above duplicated value)\n\n"), 
        keyinfo.kdup.count);
    }

    if (dbi->opts->keyscan)
    {
        if ((status = chk_pglst(fno, dbi, task)) != S_OKAY)
            dberr(status);
    }

    delete_temp_files(dbi->pcache);

    return (S_OKAY);
}


/***************************************************************************
    Check a key file in page sequential order
    (Note: it doesn't check the tree structure at all)
*/
static int walk_key(
    FILE_NO fno,
    KEYINFO *ki,
    DBINFO *dbi,
    DB_TASK *task)
{
    int status = S_OKAY;
    register F_ADDR page;
    F_ADDR top = dbi->tops[fno] - 1L;


    /* This function only gets called if the -k (keyscan) option was not
       specified. Walk through all the nodes in the file in physical order,
       without recursive calls to check b-tree structure.
    */
    for (page = ROOT_ADDR; page <= top; page++)
    {
        /* set node number to be specified in error reports */
        dbi->errinfo->addr = page;

        /* check this node only - no recursion, as keyscan option is off */
        if ((status = chk_node(fno, page, (short) 0, ki, dbi, task)) != S_OKAY)
            return (status);
    }

    return (S_OKAY);
}


/**************************************************************************
    Check the key file in ascending tree sequential order
*/
static int scan_key(
    FILE_NO fno,
    F_ADDR page,
    short levl,
    KEYINFO *ki,
    DBINFO *dbi,
    DB_TASK *task)
{
    int status = S_OKAY;
    ERR_LEVL linf;

    if (levl > ki->max_levl)
    {
        linf.levl = levl;
        linf.mlvl = ki->max_levl;
        pr_err(BAD_TRLV, &linf, dbi, task);
    }

    /* This function only gets called if the -k (keyscan) option was
       specified, for recursive checking of b-tree structure. Test
       whether we have already encountered this node:
    */
    if ((status = reg_page(page, dbi->pcache, USAGE_ID)) != S_OKAY)
    {
        /* We must abort on this error because there might be a loop in
           the b-tree pointers.
        */
        if (status < S_OKAY)
            dberr(status);
        else
            status = S_KEYERR;
        pr_err(BAD_KBTL, NULL, dbi, task);
        dbi->opts->treetrace = TRUE;
        return (status);
    }

    /* chk_node will check the node and all its children recursively, as
       keyscan option is on.
    */
    return (chk_node(fno, page, levl, ki, dbi, task));
}


/**************************************************************************
    Process a single key node (page)
*/
static int chk_node(
    FILE_NO fno,
    F_ADDR page,
    short levl,
    KEYINFO *ki,
    DBINFO *dbi,
    DB_TASK *task)
{
    SLINFO              slinfo;
    register short      slot, last_slot;
    int                 status = S_OKAY;

    stat_report(fno, ++(ki->btree_pages), dbi, task);

    if ((status = tio_get(fno, page, (char **) &slinfo.node, NOPGHOLD, dbi, task)) != S_OKAY)
        return (status);

    last_slot = slinfo.node->used_slots;

    if ((last_slot < 0) || (last_slot >= task->file_table[fno].ft_slots))
    {
        pr_err(BAD_SLCT, &slinfo.node->used_slots, dbi, task);
        if (dbi->opts->treetrace)
            return (S_KEYERR);
    }
    else if ((last_slot == 0) && (page != ROOT_ADDR))
    {
        /* deleted node, make sure it is found on delete chain */
        if ((status = chk_del_chain(page, FALSE, dbi, task)) < S_OKAY)
            dberr(status);

        if (dbi->opts->keyscan)
        {
            pr_err(BAD_KWDL, NULL, dbi, task);     /* Deleted nodes don't  */
            if (dbi->opts->treetrace)              /* belong on the b-tree */
                return (S_KEYERR);
        }
    }
    else
    {
        slinfo.all_null = 0;
        slinfo.levl = levl;

        for (slot = 0; slot <= last_slot; slot++)
        {
            /* lock page in cache */
            if ((status = tio_get(fno, page, (char **) &slinfo.node, PGHOLD, dbi, task)) != S_OKAY)
                return (status);

            slinfo.slot = slot;
            slinfo.last = (DB_BOOLEAN) (slot == last_slot);
            slinfo.sl_addr = slinfo.node->slots + (slot * ki->slot_len);
            memcpy(&slinfo.child, slinfo.sl_addr + SLOT_CHILD, sizeof(F_ADDR));
            memcpy(&slinfo.keyno, slinfo.sl_addr + SLOT_KEYNO, sizeof(short));

            status = chk_slot(fno, ki, &slinfo, dbi, task);
            tio_unget(fno, page, PGFREE, dbi, task);
            if (status != S_OKAY)
                return (status);

            if (dbi->opts->keyscan)
            {
                /* recursive b-tree check - scan_child rereads page if necessary */
                if ((status = scan_child(fno, page, ki, &slinfo, dbi, task)) != S_OKAY)
                    return (status);
            }
        }
    }

    return (S_OKAY);
}


/***************************************************************************
    Process a single slot on a given key node (page)
*/
static int chk_slot(
    FILE_NO fno,
    KEYINFO *ki,
    SLINFO *si,
    DBINFO *dbi,
    DB_TASK *task)
{
    int                      status = S_OKAY;
    register FIELD_ENTRY    *kf_ptr;
    register char           *fptr;
    char                     key[MAXKEYSIZE];

    static CHKREC chk = {BAD_KDBA, BAD_KDAD, 0, BAD_KREC, BAD_KREC, TRUE,
                         NULL, 0, 0, 0, 0};

    /* Check to verify that all null pointers are on the same level */
    if (ki->null_levl == 0)
    {
        if (si->child == NULL_NODE)
            ki->null_levl = si->levl;
    }
    else if ((si->child == NULL_NODE) &&
                (ki->null_levl > 0) && (si->levl != ki->null_levl))
    {
        pr_err(BAD_CHSL, NULL, dbi, task);
        ki->null_levl = EOF;
    }

    /* Check to verify that all pointers on page are either
       null or non-null
    */
    if (si->all_null != EOF)
    {
        if (si->slot == 0)
            si->all_null = (DB_BOOLEAN) (si->child == NULL_NODE);
        else if (si->all_null != (si->child == NULL_NODE))
        {
            pr_err(BAD_CHSM, NULL, dbi, task);
            si->all_null = EOF;
        }
    }

    if (si->last)
        return (S_OKAY);

    /* extract key prefix */
    if ((si->keyno < 0) || (si->keyno >= dbi->num_keys))
    {
        pr_err(BAD_KEYP, si, dbi, task);
        si->keyno = EOF;
        return (S_OKAY);
    }
    si->keyfd = dbi->key_fld[si->keyno];
    si->keyln = dbi->key_len[si->keyno];

    si->kf_ptr = kf_ptr = &(task->field_table[si->keyfd]);
    if (kf_ptr->fd_keyfile != fno)
    {
        pr_err(BAD_KPFL, si, dbi, task);
        return (S_OKAY);
    }

    if (dbi->opts->counts)
        dbi->key_cnt[si->keyno]++;

    /* extract database address */
    memcpy((char *) &si->dba, si->sl_addr + SLOT_DBA(si->keyln),
              sizeof(DB_ADDR));

    if (!dbi->opts->keydat)
        return (S_OKAY);

    if ((status = chk_rec(si->dba, NULL, (char *) si, &chk, dbi, task)) != S_OKAY)
        return (status);

    if (!chk.good)
        return (S_OKAY);

    if (kf_ptr->fd_type == COMKEY)
    {                                   /* build a compound key */
        key_bldcom(si->keyfd, chk.pgptr + task->record_table[chk.rid].rt_data,
                      key, TRUE, task);
        fptr = key;
    }
    else
        fptr = chk.pgptr + kf_ptr->fd_ptr;        /* copy a simple key */

    if (fldcmp(kf_ptr, si->sl_addr + SLOT_KEY, fptr, task) != 0)
        pr_err(BAD_KDAT, si, dbi, task);

    return (S_OKAY);
}


/***************************************************************************
    Traverses the child pointer, then checks to see that all the child keys
    were less than the current key.
*/
static int scan_child(
    FILE_NO fno,
    F_ADDR page,
    KEYINFO *ki,
    SLINFO *si,
    DBINFO *dbi,
    DB_TASK *task)
{
    int status = S_OKAY;

    if (si->child != NULL_NODE)
    {
        if ((si->child < 0L) || (si->child > dbi->tops[fno] - 1))
            pr_err(BAD_CHPT, si, dbi, task);
        else
        {
            /* error reports refer to child node from now on */
            dbi->errinfo->addr = si->child;

            status = scan_key(fno, si->child, (short) (si->levl + 1), ki, dbi, task);

            /* recursed back up - error reports now refer to parent again */
            dbi->errinfo->addr = page;

            if (status != S_OKAY)
            {
                pr_err(BAD_BTRC, si, dbi, task);
                return (status);
            }
            if (!si->last)
            {
                if ((status = tio_get(fno, page, (char **) &si->node, NOPGHOLD, dbi, task)) != S_OKAY)
                    return (status);

                si->sl_addr = si->node->slots + (si->slot * ki->slot_len);
            }
        }
    }

    if (si->last)
        return (S_OKAY);

    if (si->keyno == EOF)
        return (S_OKAY);

    if ((si->keyno < ki->oldno) ||
        ((si->keyno == ki->oldno) &&
         (fldcmp(si->kf_ptr, si->sl_addr + SLOT_KEY, ki->oldkey, task) < 0)))
    {
        pr_err(BAD_BTUO, si, dbi, task);
        if (dbi->opts->treetrace)
            return (S_KEYERR);

        /* Throw away the old key, start again */
        ki->oldno = -1;
    }
    else if ((si->keyno == ki->oldno) && (si->dba == ki->olddba))
    {
        pr_err(BAD_BTDD, si, dbi, task);
    }
    else if ((si->kf_ptr->fd_key == UNIQUE) &&
             (si->keyno == ki->oldno) &&
             (fldcmp(si->kf_ptr, si->sl_addr + SLOT_KEY, ki->oldkey, task) == 0))
    {
        ki->kdup.dba = si->dba;
        ki->kdup.frstdba = ki->olddba;
        ki->kdup.fldno = dbi->key_fld[si->keyno]; /* index into field table */
        ki->kdup.buf = ki->oldkey;

        pr_err(BAD_UKNU, &(ki->kdup), dbi, task);

        ki->kdup.in_series = TRUE;
    }
    else
    {
        if (ki->kdup.in_series == TRUE)
        {
            vtprintf(DB_TEXT("   (total of %ld occurrences of above duplicated value)\n\n"), 
                  ki->kdup.count);

            ki->kdup.count = 0;
            ki->kdup.in_series = FALSE;
        }

        ki->oldno = si->keyno;
        ki->olddba = si->dba;
        memcpy(ki->oldkey, si->sl_addr + SLOT_KEY, si->keyln);
    }

    ki->kdup.frst_slot = FALSE;

    return (S_OKAY);
}

/*****************************************************************************/
/*****************************************************************************/
/*                       Key File Completeness Checks                        */
/*****************************************************************************/
/*****************************************************************************/

static int reg_page(F_ADDR page, PAGE_CACHE *pcache, long id)
{
    int               rc;
    long              byte;
    long              bit;
    long              page_id;       /* this should be < 32K */
    long              page_offset;   /* this will be < 8 */
    unsigned char    *pglst;

    byte = ((long) page - 1L) / 8L;
    bit = ((long) page - 1L) % 8L;
    page_id = byte / CACHE_PG_SIZE;
    page_offset = byte % CACHE_PG_SIZE;
    pglst = (unsigned char *) cache_find(page_id + id, pcache);

    if (pglst == NULL)
    {
        vtprintf(DB_TEXT("Failed to find page %ld in temporary file %ld (reg_page).\n"), 
            page_id, id);
        return (S_SYSERR);
    }

    rc = (*(pglst + page_offset) & (1 << bit));
    if (rc)
        return (S_DUPLICATE);

    *(pglst + page_offset) |= (1 << bit);

    return (S_OKAY);
}

/**************************************************************************/

static int chk_pglst(
    FILE_NO fno,
    DBINFO *dbi,
    DB_TASK *task)
{
    unsigned char       *lsta,
                        *lstb;
    register F_ADDR      page;
    int                  rc1,
                         rc2;
    long                 byte;
    long                 bit;
    long                 page_id;
    long                 page_offset;


    /* Check that all nodes in the file appear either in the b-tree (i.e.
       a bit is set in the USAGE_ID bitmap) of in the delete chain (i.e. a
       bit is set in the DCHAIN_ID bitmap).
    */
    for (page = ROOT_ADDR; page < dbi->tops[fno]; page++)
    {
        byte = ((long) page - 1L) / 8L;
        bit = ((long) page - 1L) % 8L;
        page_id = byte / CACHE_PG_SIZE;          /* this should be < 32K */
        page_offset = byte % CACHE_PG_SIZE;      /* this will be < 8 */
        lsta = (unsigned char *)
                cache_find(page_id + DCHAIN_ID, dbi->pcache);
        lstb = (unsigned char *)
                cache_find(page_id + USAGE_ID, dbi->pcache);

        if ((lsta == NULL) || (lstb == NULL))
        {
            if (lsta == NULL)
                vtprintf(DB_TEXT("Failed to find page %ld in temporary file %ld(chk_pglst)\n"), page_id, DCHAIN_ID);
            if (lstb == NULL)
                vtprintf(DB_TEXT("Failed to find page %ld in temporary file %ld(chk_pglst)\n"), page_id, USAGE_ID);
            return (S_SYSERR);
        }

        rc1 = (*(lsta + page_offset) & (1 << bit));
        rc2 = (*(lstb + page_offset) & (1 << bit));

        if (!rc1 && !rc2)
        {
            dbi->errinfo->addr = page;
            pr_err(BAD_KPST, NULL, dbi, task);
        }
    }
    return (S_OKAY);
}


/***************************************************************************/
/***************************************************************************/
/*             Delete Chain Checks (Both Data and Key Files)               */
/***************************************************************************/
/***************************************************************************/

/***************************************************************************
    Read the delete chain for the current file
*/
static int read_del_chain(
    FILE_NO fno,
    DBINFO *dbi,
    DB_TASK *task)
{
    int               status = S_OKAY;
    DB_ADDR           dba;
    RECINFO           ri;
    char             *pgptr;
    F_ADDR            next;
    F_ADDR            cur_itm,
                      nxt_itm;
    DB_BOOLEAN        keyfile;

    /* prepare to build a delete chain in memory */
    dbi->num_del = 0L;
    cur_itm = 0L;
    next = nxt_itm = task->pgzero[fno].pz_dchain;
    keyfile = (task->file_table[fno].ft_type == KEY) ? TRUE : FALSE;

    /* run through the delete chain */
    while (nxt_itm != ((keyfile) ? NULL_NODE : NULL_DBA))
    {
        dbi->num_del++;
        dbi->errinfo->addr = cur_itm;

        if (nxt_itm < ((keyfile) ? 2L : 1L) || nxt_itm > dbi->tops[fno] - 1)
        {
            pr_err(BAD_DCHN, &next, dbi, task);
            break;
        }

        if ((status = reg_page(nxt_itm, dbi->pcache, DCHAIN_ID)) != S_OKAY)
        {                                /* Check for loops */
            if (status < S_OKAY)
                dberr(status);
            dbi->errinfo->addr = nxt_itm;
            pr_err((keyfile) ? BAD_NCLP : BAD_RCLP, NULL, dbi, task);
            break;
        }

        /* calculate the address and read in the deleted record */
        if (keyfile)
        {
            if ((status = tio_get(fno, dbi->errinfo->addr = nxt_itm,
                                  &pgptr, NOPGHOLD, dbi, task)) != S_OKAY)
                return (status);
        }
        else
        {
            d_encode_dba(fno, nxt_itm, &dba);
            dbi->errinfo->addr = cur_itm;
            if ((status = tio_read(dba, &pgptr, dbi, task)) != S_OKAY)
                return (status);
        }

        /* Check available assertions to guarantee that the item is deleted */
        if (keyfile)
        {
            if (((NODE *) pgptr)->used_slots != 0)
            {
                dbi->errinfo->addr = nxt_itm;
                pr_err(BAD_KCNDL, NULL, dbi, task);
            }
        }
        else
        {
            get_rid_info(pgptr, &ri);
            if (!ri.del)
            {
                dbi->errinfo->addr = nxt_itm;
                pr_err(BAD_DCNDL, NULL, dbi, task);
            }
        }

        /* get the address of the next deleted record */
        memcpy(&next, pgptr + ((keyfile) ? sizeof(short) + sizeof(long) :
                                           sizeof(short)), sizeof(F_ADDR));
        cur_itm = nxt_itm;
        nxt_itm = next;
    }

    return (S_OKAY);
}


/***************************************************************************
    Check to see that a deleted item is actually on the delete chain
*/
static int chk_del_chain(
    F_ADDR rno,                         /* data file slot or key file node */
    DB_BOOLEAN isfnd,
    DBINFO *dbi,
    DB_TASK *task)
{
    DB_BOOLEAN        found;
    long              byte;
    long              bit;
    long              page_id;          /* this should be < 32K */
    long              page_offset;      /* this will be < 8 */
    unsigned char    *pg_list;

    byte = ((long) rno - 1L) / 8L;
    bit = ((long) rno - 1L) % 8L;
    page_id = byte / CACHE_PG_SIZE;
    page_offset = byte % CACHE_PG_SIZE;
    pg_list = (unsigned char *) cache_find(page_id + DCHAIN_ID, dbi->pcache);
    dbi->errinfo->addr = rno;

    /* search through the delete chain for this record number */

    if (pg_list == NULL)
    {
        vtprintf(DB_TEXT("Failed to find page %ld in temporary file %ld (chk_del_chain).\n"),
            page_id, DCHAIN_ID);
        return (S_SYSERR);
    }

    found = (DB_BOOLEAN) (*(pg_list + page_offset) & (1 << bit));

    if (isfnd == found)
    {
        pr_err((task->file_table[dbi->errinfo->fno].ft_type == KEY) ? BAD_KDLNC : BAD_DDLNC,
               NULL, dbi, task);
        return (S_OKAY);
    }

    return (S_NOTFOUND);
}


/***************************************************************************
    Same as dio_get except that it reports the address of any error.
*/
static int tio_get(
    FILE_NO fno,
    F_ADDR addr,
    char **pgptr,
    int hold,
    DBINFO *dbi,
    DB_TASK *task)
{
    int status;

    if ((status = dio_get(fno, addr, pgptr, hold, task)) != S_OKAY)
        pr_err(BAD_DIOG, &addr, dbi, task);

    return (status);
}

/***************************************************************************
    Same as dio_unget except that it reports the address of any error.
*/
static int tio_unget(
    FILE_NO fno,
    F_ADDR addr,
    int hold,
    DBINFO *dbi,
    DB_TASK *task)
{
    int status;

    if ((status = dio_unget(fno, addr, hold, task)) != S_OKAY)
        pr_err(BAD_DIOG, &addr, dbi, task);

    return (status);
}


/***************************************************************************
    Same as dio_read except that it reports the address of any error.
*/
static int tio_read(
    DB_ADDR dba,        /* database address to read */
    char **pgptr,       /* buffer to read it into */
    DBINFO *dbi,
    DB_TASK *task)
{
    int status;

    if ((status = dio_read(dba, pgptr, NOPGHOLD, task)) != S_OKAY)
        pr_err(BAD_DIOR, &dba, dbi, task);

    return (status);
}



/***************************************************************************
    Check for valid DB_ADDR
    (copy of runtime's check_dba(), but changed to be consistent with the
    rest of dbcheck)
*/
static int chk_dba(register DB_ADDR dba, DBINFO *dbi, DB_TASK *task)
{
    short      fn;
    DB_ADDR    rn;

    if (dba == NULL_DBA)
        return (S_OKAY);

    d_decode_dba(dba, &fn, &rn);

    if (  ((fn < 0) || (fn >= task->size_ft))    ||
          (task->file_table[fn].ft_type != DATA) ||
          (rn >= (DB_ADDR) dbi->tops[fn])   )
    {
        return (S_INVADDR);
    }

    return (S_OKAY);
}


/***************************************************************************
    Given a record slot, determine its record id, delete flag, and lock flag
*/
static int get_rid_info(char *ptr, RECINFO *ri)
{
    /* get record identification number */
    memcpy((char *) &ri->rid, ptr + REC_RID, sizeof(short));

    if (ri->rid < 0)
    {
        ri->rid = (short) ~ri->rid;
        ri->del = TRUE;
    }
    else
    {
        ri->del = FALSE;
    }

    if (ri->rid & RLBMASK)
    {
        ri->rid &= ~RLBMASK;
        ri->lck = TRUE;
    }
    else
    {
        ri->lck = FALSE;
    }

    return (0);
}

/***************************************************************************/
/***************************************************************************/
/*                     Miscellaneous Print Routines                        */
/***************************************************************************/
/***************************************************************************/

/***************************************************************************
    Prints a complete error message including the address of where the
    problem occured.  Also keeps track of the error counters.
*/
static void pr_err(
    int err,
    void *what,
    DBINFO *dbi,
    DB_TASK *task)
{
    DB_ADDR    *dba;
    short       setno,
                keyno,
                fldno;
    DB_TCHAR   *fmt   = chk_errs[--err].text;
    int         type  = chk_errs[err].type;
    int         sev   = chk_errs[err].severity;
    ERRINFO    *ei    = dbi->errinfo;

    if (sev != ERS_WARN)
        ei->tot_err_cnt++;

    ei->addr &= ADDRMASK;    /* get rid of file number, if it's there */

    if (ei->err_addr != ei->addr)
    {
        ei->err_addr = ei->addr;
        if (sev != ERS_WARN)
            ei->tot_rec_err++;
        if (task->file_table[ei->fno].ft_type == KEY)
            vtprintf(DB_TEXT("\nProblems at node %lu:\n"), ei->addr);
        else
            vtprintf(DB_TEXT("\nProblems at record %lu:\n"), ei->addr);
    }

    fputs("  * ", stdout);

    switch (type)
    {
        case ERT_NULL:
            vtprintf(fmt);
            break;

        case ERT_SHORT:
            vtprintf(fmt, *((short *) what));
            break;

        case ERT_FADR:
            vtprintf(fmt, *((F_ADDR *) what));
            break;

        case ERT_DBA:
            vtprintf(fmt, pr_dba((DB_ADDR *) what));
            break;

        case ERT_SETS:
            setno = *((short *) what);
            vtprintf(DB_TEXT("set %s(%d) error:\n    %s"), task->set_names[setno], setno, fmt);
            break;

        case ERT_SPTR:
            setno = ((ERR_PDBA *) what)->setno;
            dba = &((ERR_PDBA *) what)->dba;
            vtprintf(DB_TEXT("set %s(%d) error:\n"), task->set_names[setno], setno);
            vtprintf(DB_TEXT("    %s:\n    pointer-dba=%s\n"), fmt, pr_dba(dba));
            break;

        case ERT_SMEM:
            setno = ((ERR_PDBA *) what)->setno;
            dba = &((ERR_PDBA *) what)->dba;
            vtprintf(DB_TEXT("set %s(%d) error:\n"), task->set_names[setno], setno);
            vtprintf(DB_TEXT("    %s:\n    member-dba=%s\n"), fmt, pr_dba(dba));
            break;

        case ERT_SMTP:
            setno = ((ERR_STYP *) what)->setno;
            dba = &((ERR_STYP *) what)->dba;
            vtprintf(DB_TEXT("set %s(%d) error:\n    "), task->set_names[setno], setno);
            vtprintf(fmt, ((ERR_STYP *) what)->rid);
            vtprintf(DB_TEXT(":\n    member-dba=%s\n"), pr_dba(dba));
            break;

        case ERT_SCNT:
            setno = ((ERR_SCNT *) what)->setno;
            vtprintf(DB_TEXT("set %s(%d) error:\n    "), task->set_names[setno], setno);
            vtprintf(fmt, ((ERR_SCNT *) what)->owncnt, ((ERR_SCNT *) what)->memcnt);
            break;

        case ERT_SOWN:
            setno = ((ERR_PDBA *) what)->setno;
            dba = &((ERR_PDBA *) what)->dba;
            vtprintf(DB_TEXT("set %s(%d) error:\n"), task->set_names[setno], setno);
            vtprintf(DB_TEXT("    %s:\n    owner-dba=%s\n"), fmt, pr_dba(dba));
            break;

        case ERT_SOTP:
            setno = ((ERR_STYP *) what)->setno;
            dba = &((ERR_STYP *) what)->dba;
            vtprintf(DB_TEXT("set %s(%d) error:\n    "), task->set_names[setno], setno);
            vtprintf(fmt, ((ERR_STYP *) what)->rid);
            vtprintf(DB_TEXT(":\n    owner-dba=%s\n"), pr_dba(dba));
            break;

        case ERT_KINF:
            fldno = *((short *) what);
            vtprintf(DB_TEXT("key field %s(%d) error: %s"), task->field_names[fldno], fldno, fmt);
            break;

        case ERT_SINM:
            keyno = ((SLINFO *) what)->keyno;
            if ((keyno >= 0) && (keyno < dbi->num_keys))
            {
                 fldno = dbi->key_fld[keyno];
                 vtprintf(DB_TEXT("key field %s(%d) error:\n    "), task->field_names[fldno], fldno);
            }
            vtprintf(DB_TEXT("slot %d's "), ((SLINFO *) what)->slot);
            vtprintf(fmt, keyno);
            break;

        case ERT_SDBA:
            keyno = ((SLINFO *) what)->keyno;
            fldno = dbi->key_fld[keyno];
            dba = &((SLINFO *) what)->dba;
            vtprintf(DB_TEXT("key field %s(%d) error:\n    "), task->field_names[fldno], fldno);
            vtprintf(DB_TEXT("slot %d's "), ((SLINFO *) what)->slot);
            vtprintf(fmt, pr_dba(dba));
            break;

        case ERT_SLCH:
            vtprintf(DB_TEXT("slot %d's "), ((SLINFO *) what)->slot);
            vtprintf(fmt, ((SLINFO *) what)->child);
            break;

        case ERT_LEVL:
            vtprintf(fmt, ((ERR_LEVL *) what)->levl, ((ERR_LEVL *) what)->mlvl);
            break;

        case ERT_SLIN:
            vtprintf(DB_TEXT("slot %d: "), ((SLINFO *) what)->slot);
            vtprintf(fmt, ((SLINFO *) what)->levl);
            break;

        case ERT_SYSTEM:
            vtprintf(fmt, ei->addr);
            break;

        case ERT_PGZERO:
            vtprintf(fmt, ((ERR_PGZERO *) what)->slot,
                ((ERR_PGZERO *) what)->file);
            break;

        case ERT_UKNU:
            {
                ERR_KEYDUPES *kdup = (ERR_KEYDUPES *) what;
                DB_TCHAR kval[MAXTEXT];

                /* if there are really two duplicate key errors here */
                /* print error for last key slot */

                if (kdup->in_series == FALSE && kdup->frst_slot == FALSE)
                {
                    ei->tot_err_cnt++;   /* increment the error count an extra time */

                    /* This is the first in a series of duplicates, so */
                    /*  print the value of the key field here */

                    fldtotxt(kdup->fldno, kdup->buf, kval, 0, task);

                    kdup->count = 1; /* this is first in series of duplications */
                    vtprintf(DB_TEXT("Duplicated key value: %s\n"), kval);

                    fputs("  * ", stdout);
                    vtprintf(fmt, task->field_names[kdup->fldno], pr_dba(&(kdup->frstdba)));

                    fputs("  * ", stdout);
                }
                kdup->count++;

                /* print error for current key slot */
                vtprintf(fmt, task->field_names[kdup->fldno], pr_dba(&(kdup->dba)));
            }
            break;

        case ERT_SIZE:
            vtprintf(fmt, ((ERR_FSIZE *) what)->fname,
                ((ERR_FSIZE *) what)->fsize);
            break;

        default:
            vtprintf(DB_TEXT("dbcheck internal error:  invalid error type %d\n"), type);
            break;
    }
}


/***************************************************************************
    Format a database address
*/
static DB_TCHAR *pr_dba(DB_ADDR *db_addr)
{
    DB_ADDR           dba;
    static DB_TCHAR   prbuf[30];
    short             file;
    DB_ULONG          slot;

    memcpy(&dba, db_addr, DB_ADDR_SIZE);
    d_decode_dba(dba, &file, &slot);
    vstprintf(prbuf, DB_TEXT("[%d:%lu]"), file, slot);

    return (prbuf);
}


/***************************************************************************
    Prints a status report at fixed intervals to appease eager users
*/
static int stat_report(
    FILE_NO fno,
    F_ADDR rno,
    DBINFO *dbi,
    DB_TASK *task)
{
    static int     prev;
    int            pcnt;
    DB_TCHAR      *filtyp, *wlktyp;
    F_ADDR         top, realtop;
    DB_TCHAR       filnam[FILENMLEN],
                  *filptr;

    top = dbi->tops[fno] - 1L;

    if (task->file_table[fno].ft_type == KEY)
    {
        filtyp = DB_TEXT("key");
        wlktyp = DB_TEXT("node");
        realtop = (dbi->opts->keyscan && (top - dbi->num_del > 0)) ?
                  top - dbi->num_del : top;
    }
    else
    {
        filtyp = DB_TEXT("data");
        wlktyp = DB_TEXT("record");
        realtop = top;
    }

    if (rno == 0L)
    {
        vtstrcpy(filnam, task->file_table[fno].ft_name);
        if ((filptr = vtstrrchr(filnam, DIRCHAR)) != NULL)
            filptr++;
        else
            filptr = filnam;
        vtprintf(DB_TEXT("\n\n------------------------------------"));
        vtprintf(DB_TEXT("------------------------------------\n\n"));
        vtprintf(DB_TEXT("Processing %s file: %s(%d), total of %ld %s%s\n"),
                filtyp, filptr, fno, top, wlktyp, PLULET(top));
        fflush(stdout);
        if (dbi->opts->report != -1)
        {
            vftprintf(stderr, DB_TEXT("Processing %s file: %s(%d), total of %ld %s%s\n"),
                filtyp, filptr, fno, top, wlktyp, PLULET(top));
            fflush(stderr);
        }
    }

    dbi->errinfo->err_addr = -1L;
    if ((dbi->opts->report == -1) || (top == 0L))
        return (0);

    if (rno == 0L)
    {
        vftprintf(stderr, DB_TEXT("\n\nProcessing delete chain:  "));
        return (0);
    }
    else if (rno == 1L)
    {
        prev = 0;
        vftprintf(stderr, DB_TEXT("%ld %s%s on delete chain.\n"),
                  dbi->num_del, wlktyp, PLULET(dbi->num_del));
        vftprintf(stderr, DB_TEXT("Processing %ss:\n"), wlktyp);
    }

    if (dbi->opts->report == 0)
    {
        vfputtc(DB_TEXT('.'), stderr);
    }
    else
    {
        pcnt = (int) ((rno * 100L) / realtop);
        if (pcnt != prev)
        {
            prev = pcnt;
            if ((pcnt % dbi->opts->report) == 0)
            {
                if ((pcnt % 10) == 0)
                    vftprintf(stderr, (pcnt == 100) ? DB_TEXT("%d%%\n\n") : DB_TEXT("%d%%"), pcnt);
                else
                    vfputtc(DB_TEXT('+'), stderr);
            }
        }
    }

    return (0);
}


/***************************************************************************
    Print out execution statistics
*/
static int pr_stat(int status, DBINFO *dbi)
{
    if (status == S_OKAY)
    {
        vftprintf(stdout, DB_TEXT("\n\nDatabase consistency check completed\n"));
    }
    else
    {
        vftprintf(stdout, DB_TEXT("\n\nDatabase consistency check prematurely terminated"));
        vftprintf(stdout, DB_TEXT("\n   Last error = %d\n\n"), status);
    }
    if (dbi)
    {
        ERRINFO *ei = dbi->errinfo;

        if (ei)
        {
            vftprintf(stdout,
              DB_TEXT("\n%ld error%s %s encountered in %ld record%s/node%s\n"),
              ei->tot_err_cnt,
              PLULET(ei->tot_err_cnt),
              PLURAL(ei->tot_err_cnt, DB_TEXT("was"), DB_TEXT("were")),
              ei->tot_rec_err,
              PLULET(ei->tot_rec_err),
              PLULET(ei->tot_rec_err));
        }
    }
    return (0);
}

/****************************************************************************
    Print out count statistics
*/
static int pr_counts(DBINFO *dbi, DB_TASK *task)
{
    int i;

    if (!dbi || !dbi->opts->counts)
        return (0);

    vtprintf(DB_TEXT("\n\nRecord occurrence counts:\n"));

    for (i = 0; i < task->size_rt; i++)
        vtprintf(DB_TEXT("%8ld   %s(%03d)\n"), dbi->rec_cnt[i], task->record_names[i], i);

    vtprintf(DB_TEXT("\n\nSet member counts:\n"));

    for (i = 0; i < task->size_st; i++)
        vtprintf(DB_TEXT("%8ld   %s(%03d)\n"), dbi->set_cnt[i], task->set_names[i], i);

    vtprintf(DB_TEXT("\n\nKey occurrence counts:\n"));

    for (i = 0; i < dbi->num_keys; i++)
        vtprintf(DB_TEXT("%8ld   %s(%03d)\n"), dbi->key_cnt[i], task->field_names[dbi->key_fld[i]], i);

    fflush(stdout);

    return (0);
}


VXSTARTUP("dbcheck", 0)

