/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbexp utility                                     *
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

    dbexp - Database export tool for db.*

    To execute:
        dbexp [-r] [-m] [-n] [-d] [-e esc_char] [-s separator] database
              [rectypes...]

    Dbexp converts the data within a db.* database into an
    ASCII format that can be moved to another computer, or used
    as input to any other tool which reads ASCII data.

    Data from each record type is written to a file that is named
    after the record type.  The file name suffix is ".txt", which
    would mean that the "employee" data from a database would be
    written to the file named "employee.txt".

    The default usage of dbexp writes all records of all types
    to files.  Every field is printed.  Numeric fields are printed
    as 'printf' prints them, and textual fields are printed with
    quotes (") around them.  All fields are separated by commas.
    Arrays and structured fields are printed sequencially as
    normal fields.

    The default usage of dbexp names the database only:

        dbexp corp

    If not all records are to be exported, the ones that are to
    be exported may be named following the database name:

        dbexp corp employee department

    The "-r" and "-m" options request database addresses to be
    printed as data fields.  The "-r" option causes the database
    address of the record to be printed as the first field.  The
    "-m" option causes the database addresses of the owners of all
    sets of which the record is a member to be printed.  These are
    printed following the record's database address (if requested),
    and before any of the data fields.  This is the same order in
    which they occur in the db.* record.

    The database address fields are normally of no use, but may
    be used to re-create or re-structure the database with the
    'dbimp' database import tool.

    The "-d" option will cause database addresses to be printed
    in decimal format, rather than database address format
    (file:slot).

-----------------------------------------------------------------------*/

#define MOD dbexp
#include "db.star.h"
#include "dbexp.h"
#include "version.h"

/* Maximum number of export (output) files open at once */
#define MAX_OPEN 5

/* ********************** LOCAL FUNCTION DECLARATIONS **************** */
static void usage(void);
static int get_xopts(int, DB_TCHAR **, EXPOPTS *);
static void free_xopts(EXPOPTS *);
static void check_xnames(EXPOPTS *, DB_TASK *);
static void init_xfiles(EXPFILE *, DB_TASK *);
static int export_data_file(FILE_NO, EXPOPTS *, EXPFILE *, DB_TASK *);
static void nospace(void);


int MAIN(int argc, DB_TCHAR *argv[])
{
    int         i;
    int         status;
    FILE_NO     fno;
    EXPOPTS     xopts;
    EXPFILE    *xfiles;
    DB_TASK    *task = NULL;

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Database Export")));

    /* set up the options */
    memset(&xopts, 0, sizeof(EXPOPTS));
    xopts.sep_char = DB_TEXT(',');
    xopts.esc_char = DB_TEXT('\\');

    psp_init();

    if (get_xopts(argc, argv, &xopts) != 0)
    {
        psp_term();
        return 1;
    }

    if ((status = d_opentask(&task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("dbexp: Failed to open task\n"));
        goto exit;
    }

    if ((status = d_set_dberr(dbexp_dberr, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("dbexp: failed to setup error handling\n"));
        goto exit;
    }

    if ((status = d_on_opt(READNAMES, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("dbexp: failed to setup options\n"));
        goto exit;
    }

    status = d_open_sg(xopts.dbname, DB_TEXT("o"), xopts.sg, task);
    if (status != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("dbexp: failed to open database\n"));
        goto exit;
    }

    /* make sure that specified record types exist */
    check_xnames(&xopts, task);

    /* allocate a file pointer for each record type */
    xfiles = (EXPFILE *) psp_cGetMemory(task->size_rt * sizeof(EXPFILE), 0);
    if (!xfiles)
    {
        vftprintf(stderr, DB_TEXT("Unable to allocate memory for output file table\n"));
        d_close(task);
        goto exit;
    }

    init_xfiles(xfiles, task);

    /* for each data file in the database */
    for (fno = 0; fno < task->size_ft; fno++)
    {
        /* make sure it is a data file */
        if (task->file_table[fno].ft_type != DATA)
            continue;

        /* make sure it contains records to be exported */
        if (xopts.n_recs)
        {
            for (i = 0; i < xopts.n_recs; i++)
            {
                if (xopts.rt_index[i] == -1)
                    continue;

                if (fno == task->record_table[xopts.rt_index[i]].rt_file)
                    break;
            }

            if (i == xopts.n_recs)
            {
                if (!xopts.silent)
                    vtprintf(DB_TEXT("Skipping data file '%s'\n"),
                            task->file_table[fno].ft_name);

                continue;
            }
        }

        /* report progress to user */
        if (! xopts.silent)
        {
            vtprintf(DB_TEXT("Exporting contents of data file '%s'\n"),
                    task->file_table[fno].ft_name);
        }

        /* export all the records in this file */
        if ((status = export_data_file(fno, &xopts, xfiles, task)) != S_OKAY)
        {
            if (status == S_NOSPACE)
                nospace();

            break;
        }
    }

    /* termination message */
    if (!xopts.silent)
	vtprintf(DB_TEXT("dbexp completed\n"));

    psp_freeMemory(xfiles, 0);

    d_close(task);

exit:
    if (task)
        d_closetask(task);

    free_xopts(&xopts);
    psp_term();

    return 0;
}

/**************************************************************************/

static int get_xopts(int argc, DB_TCHAR *argv[], EXPOPTS *xo)
{
    int       i;
    int       j;
#if defined(SAFEGARDE)
    DB_TCHAR *cp;
    DB_TCHAR *password;
    int       mode;
#endif

    /* for each command line argument */
    for (i = 1; i < argc; i++)
    {
        /* check for '-' */
        if (argv[i][0] != DB_TEXT('-'))
            break;

        switch (vtotlower(argv[i][1]))
        {
            case DB_TEXT('h'):
            case DB_TEXT('?'):
                usage();
                goto error;

            case DB_TEXT('r'):
                xo->rec_addr = TRUE;
                break;

            case DB_TEXT('m'):
                xo->mem_addr = TRUE;
                break;

            case DB_TEXT('n'):
                xo->silent = TRUE;
                break;

            case DB_TEXT('d'):
                xo->decimal = TRUE;
                break;

            case DB_TEXT('x'):
                xo->extended = TRUE;
                break;

#if defined(UNICODE)
            case DB_TEXT('u'):
                xo->unicode = TRUE;
                break;
#endif

            case DB_TEXT('e'):
                if (argv[i][2])
                    xo->esc_char = argv[i][2];
                else
                {
                    if (++i < argc)
                        xo->esc_char = *argv[i];
                    else
                    {
                        vftprintf(stderr, DB_TEXT("dbexp: character must follow -e option\n"));
                        goto error;
                    }
                }

                break;

            case DB_TEXT('s'):
                if (argv[i][2])
                {
                    if (vtotlower(argv[i][2]) == DB_TEXT('g'))
                    {
                        if (i == argc - 1)
                        {
                            vftprintf(stderr, DB_TEXT("dbexp: Invalid argument: %s\n"), argv[i]);
                            usage();
                            goto error;
                        }

#if defined(SAFEGARDE)
                        if ((cp = vtstrchr(argv[++i], DB_TEXT(','))) != NULL)
                            *cp++ = DB_TEXT('\0');

                        if (cp)
                        {
                            if (vtstricmp(argv[i], DB_TEXT("low")) == 0)
                                mode = LOW_ENC;
                            else if (vtstricmp(argv[i], DB_TEXT("med")) == 0)
                                mode = MED_ENC;
                            else if (vtstricmp(argv[i], DB_TEXT("high")) == 0)
                                mode = HIGH_ENC;
                            else {
                                vftprintf(stderr, DB_TEXT("dbexp: Invalid encryption mode\n"));
                                usage();
                                goto error;
                            }

                            password = cp;
                        }
                        else
                        {
                            mode = MED_ENC;
                            password = argv[i];
                        }

                        if ((xo->sg = sg_create(mode, password)) == NULL)
                        {
                            vftprintf(stderr, DB_TEXT("dbexp: failed to create SafeGarde context\n"));
                            goto error;
                        }
#else
                        vftprintf(stderr, DB_TEXT("dbexp: SafeGarde not available in this version\n"));
                        goto error;
#endif
                    }
                    else
                        xo->sep_char = argv[i][2];
                }
                else
                {
                    if (++i < argc)
                        xo->sep_char = *argv[i];
                    else
                    {
                        vftprintf(stderr, DB_TEXT("dbexp: character must follow -s option\n"));
                        goto error;
                    }
                }

                break;

            default:
                vftprintf(stderr, DB_TEXT("dbexp: invalid option '%s'\n"), argv[i] + 1);
                goto error;
        }
    }

    if (i == argc)
    {
        vftprintf(stderr, DB_TEXT("dbexp: No database name specified\n"));
	usage();
        goto error;
    }

    /* The database name is the first non-option argument */
    xo->dbname = argv[i++];

    if (i < argc)
    {
        xo->n_recs = argc - i;
        xo->recnames = (DB_TCHAR **) psp_cGetMemory(xo->n_recs *
                sizeof(DB_TCHAR *), 0);
        xo->rt_index = (int *) psp_cGetMemory(xo->n_recs * sizeof(int), 0);

        if (!xo->recnames || !xo->rt_index)
        {
            vftprintf(stderr, DB_TEXT("dbexp: out of memory\n"));
            goto error;
        }
    }

    for (j = 0; j < xo->n_recs; j++, i++) {
        /* Copy record name string and convert to upper case */
        if ((xo->recnames[j] = psp_strdup(argv[i], 0)) == NULL)
        {
            vftprintf(stderr, DB_TEXT("dbexp: out of memory\n"));
            goto error;
        }

        vtstrupr(xo->recnames[j]);
    }

    if (vistdigit(xo->sep_char))
    {
        vftprintf(stderr, DB_TEXT("dbexp: Separator character must not be numeric\n"));
        goto error;
    }

    if (vistdigit(xo->esc_char))
    {
        vftprintf(stderr, DB_TEXT("dbexp: Escape character must not be numeric\n"));
        goto error;
    }

    return 0;

error:
    free_xopts(xo);

    return 1;
}

/**************************************************************************/

static void free_xopts(EXPOPTS *xo)
{
    int i;

    if (xo->recnames)
    {
        for (i = 0; i < xo->n_recs; i++)
        {
            if (xo->recnames[i])
                psp_freeMemory(xo->recnames[i], 0);
        }

        psp_zFreeMemory(&xo->recnames, 0);
    }

    if (xo->rt_index)
        psp_zFreeMemory(&xo->rt_index, 0);
}

/**************************************************************************/

static void check_xnames(EXPOPTS *xo, DB_TASK *task)
{
    int i, j;

    /* if specific record types are requested, verify their existence in
        the database dictionary
    */
    if (xo->n_recs == 0)
        return;

    /* for each search record type */
    for (i = 0; i < xo->n_recs; i++)
    {
        xo->rt_index[i] = -1;
        for (j = 0; j < task->size_rt; j++)
        {
            if (vtstrcmp(xo->recnames[i], task->record_names[j]) == 0)
            {
                xo->rt_index[i] = j;
                break;
            }
        }

        if (xo->rt_index[i] == -1)
        {
            vftprintf(stderr,
                DB_TEXT("**WARNING** record type '%s' not in database\n"),
                xo->recnames[i]);
        }
    }
}

/**************************************************************************/

static void init_xfiles(EXPFILE *xfiles, DB_TASK *task)
{
    int i, j;
    DB_BOOLEAN changed;
    EXPFILE *xf;

    for (i = 0, xf = xfiles; i < task->size_rt; i++, xf++)
    {
        xf->fp = NULL;
        xf->open = FALSE;
        xf->removed = FALSE;
        xf->recent = 0;

        if (vtstrcmp(task->record_names[i], DB_TEXT("SYSTEM")) == 0)
        {
            xf->name[0] = 0;
            continue;
        }

        /* create the file name based on the record type */
        vtstrlwr(vtstrcpy(xf->name, task->record_names[i]));

        xf->name[MAXEXPFILELEN-4] = 0;
        vtstrcat(xf->name, DB_TEXT(".txt"));

        /* make sure no duplicate names */
        changed = FALSE;
        for (j = 0; j < i; j++)
        {
            if (vtstrcmp(xf->name, xfiles[j].name) == 0)
            {
                xf->name[vtstrlen(xf->name) - 1]++;
                changed = TRUE;
            }
        }

        if (changed)
        {
            vftprintf(stderr, DB_TEXT("**WARNING** Duplicate text file names.\n"));
            vftprintf(stderr, DB_TEXT("      Record type %s stored in file '%s'.\n"),
                task->record_names[i], xf->name);
        }
    }
}

/**************************************************************************/

static int export_data_file(
    FILE_NO   fno,
    EXPOPTS  *xo,
    EXPFILE  *xfiles,
    DB_TASK  *task)
{
    int       i;
    int       status;
    int       open_xfiles = 0;
    int       recent = 1;
    int       lru, least_recent;
    F_ADDR    rno;
    F_ADDR    records_found = 0;
    F_ADDR    pz_next;
    DB_ADDR   dba;
    short     rec_type;
    EXPFILE  *xf;
    char     *rec;
    DB_TCHAR  sdba[12];

    /* get the number of records in the file */
    if ((status = dio_pzread(fno, task)) != S_OKAY)
        return status;

    pz_next = task->pgzero[fno].pz_next;

    for (rno = 1; rno < pz_next; rno++)
    {
        d_encode_dba(fno, rno, &dba);

        if ((status = dio_read(dba, &rec, NOPGHOLD, task)) != S_OKAY)
            break;

        records_found++;

        /* determine record type */
        memcpy(&rec_type, rec, sizeof(short));
        rec_type &= ~RLBMASK;

        /* skip if record is deleted */
        if (rec_type < 0)
            continue;

        /* skip if system record */
        if (task->record_table[rec_type].rt_fdtot == -1)
            continue;

        /* if this record type is to be exported */
        if (xo->n_recs)
        {
            for (i = 0; i < xo->n_recs; i++)
            {
                if (rec_type == xo->rt_index[i])
                    break;
            }

            /* skip if record type is not to be exported */
            if (i == xo->n_recs)
                continue;
        }

        /* if the output file is not opened yet */
        xf = &xfiles[rec_type];
        if (! xf->open)
        {
            if (open_xfiles == MAX_OPEN)
            {
                least_recent = recent;
                lru = -1;
                for (i = 0; i < task->size_rt; i++)
                {
                    if (xfiles[i].open && xfiles[i].recent < least_recent)
                    {
                        least_recent = xfiles[i].recent;
                        lru = i;
                    }
                }
                if (lru != -1)
                {
                    fclose(xfiles[lru].fp);
                    xfiles[lru].open = FALSE;
                    xfiles[lru].recent = 0;
                    open_xfiles--;
                }
            }

            /* Always remove any existing file */
            if (! xf->removed)
            {
                psp_fileRemove(xf->name);
                xf->removed = TRUE;

                /* also print a message once */
                if (!xo->silent)
                {
                    vtprintf(DB_TEXT("   Record type %s into file name %s\n"),
                        task->record_names[rec_type], xf->name);
                }
            }

            /* Open in append mode because the same file may be opened
               multiple times
            */
            xf->fp = vtfopen(xf->name, xo->unicode ? DB_TEXT("ab") : DB_TEXT("a"));
            if (xf->fp == NULL)
            {
                vftprintf(stderr, DB_TEXT("Unable to open %s\n"), xf->name);
                status = S_NOFILE;
                break;
            }
            open_xfiles++;
            xf->open = TRUE;
        }

        /* This output file has been recently used */
        xf->recent = recent++;

        /* First field in row - no comma before it */
        xo->comma = FALSE;

        /* print database address of record */
        if (xo->rec_addr)
        {
            memcpy(&dba, rec + sizeof(short), sizeof(DB_ADDR));
            cvt_dba(sdba, dba, xo);
            if (vftprintf(xf->fp, DB_TEXT("%s"), sdba) == 0)
            {
                status = S_NOSPACE;
                break;
            }

            if (xo->comma)
            {
                if (vfputtc(xo->sep_char, xf->fp) != xo->sep_char)
                {
                    status = S_NOSPACE;
                    break;
                }
            }
            else
            {
                xo->comma = TRUE;
            }
        }

        /* print owners of sets of which this record is a member */
        if (xo->mem_addr)
        {
            status = pr_mems(xf->fp, rec_type, rec, xo, task);
            if (status != S_OKAY)
                break;
        }

        /* print all fields into `rectype`.txt file */
        status = pr_data(xf->fp, rec_type, rec, xo, task);
        if (status != S_OKAY)
            break;

        if (fflush(xf->fp) == EOF)
        {
            status = S_NOSPACE;
            break;
        }

        /* end the record print */
        if (vfputtc(DB_TEXT('\n'), xf->fp) != DB_TEXT('\n'))
        {
            status = S_NOSPACE;
            break;
        }
    }

    /* close all open output files */
    for (i = 0; i < task->size_rt; i++)
    {
        if (xfiles[i].open)
        {
            fclose(xfiles[i].fp);
            xfiles[i].open = FALSE;
            xfiles[i].recent = 0;
        }
    }

    return(status);
}

/***************************************************************************/

void EXTERNAL_FCN dbexp_dberr(int errnum, DB_TCHAR *errmsg)
{
    /* nop */
}

#if defined(UNICODE)
#define UNICODE_OPT L"[-u] "
#else
#define UNICODE_OPT ""
#endif
/***************************************************************************/

static void usage()
{
    vftprintf(stderr, DB_TEXT("usage: dbexp [options] database [rectype...]\n"));
    vftprintf(stderr, DB_TEXT("options: [-r] [-m] [-n] [-d] [-e esc_char] [-s separator] ") UNICODE_OPT DB_TEXT("[-x]\n"));
    vftprintf(stderr, DB_TEXT("         [-sg [<mode>,]<password>]\n"));
    vftprintf(stderr, DB_TEXT(" where   -r  Causes record's database to be printed as a field\n"));
    vftprintf(stderr, DB_TEXT("         -m  Causes owner database addresses to be printed as fields\n"));
    vftprintf(stderr, DB_TEXT("         -n  Is 'silent' option; no output to stdout\n"));
    vftprintf(stderr, DB_TEXT("         -d  Causes database addresses to be printed in decimal\n"));
    vftprintf(stderr, DB_TEXT("         -e  Selects an alternate escape character (default '\\')\n"));
    vftprintf(stderr, DB_TEXT("         -s  Selects an alternate separator character (default ',')\n"));
#if defined(UNICODE)
    vftprintf(stderr, DB_TEXT("         -u  Causes output to be in Unicode\n"));
#endif
    vftprintf(stderr, DB_TEXT("         -x  Prints extended ascii characters (default octal)\n"));
    vftprintf(stderr, DB_TEXT("         -sg Specifies SafeGarde encryption information\n"));
    vftprintf(stderr, DB_TEXT("              <mode> can be 'low', 'med' (default), or 'high'\n"));
    vftprintf(stderr, DB_TEXT("Default database address format is file:slot.\n"));
}

/***************************************************************************/

static void nospace()
{
    vftprintf(stderr, DB_TEXT("***FATAL ERROR*** unable to write to file.\n"));
    vftprintf(stderr, DB_TEXT("The disk may be full.  errno = %d\n"), errno);
    vftprintf(stderr, DB_TEXT("Terminating export.\n"));
}


VXSTARTUP("dbexp", 0)

