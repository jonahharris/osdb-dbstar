/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, keypack utility                                   *
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

    keypack.c - db.* key file packing utility

-----------------------------------------------------------------------*/

#define MOD keypack
#include "db.star.h"
#include "keypack.h"
#include "version.h"

/* ************************* LOCAL FUNCTIONS ************************* */
static int usage(void);


void EXTERNAL_FCN keypack_dberr(int errnum, DB_TCHAR *msg)
{
    vtprintf(DB_TEXT("\n%s (errnum = %d)\n"), msg, errnum);
}


/* ======================================================================
    Keypack main program
*/
int MAIN(int argc, DB_TCHAR *argv[])
{
    int            unused_slots,    /* empty slots per node in packed file */
                   pages,           /* size of db.* cache */
                   i, stat;
    FILE_NO        fno, start_fno, end_fno;
    DB_TCHAR      *ptr;
    DB_TCHAR      *dbname;
    DB_TCHAR       tmppath[FILENMLEN];
    DB_TCHAR       keyfile[FILENMLEN];      /* name of key file to pack */
    DB_TCHAR       keypath[FILENMLEN];      /* fully qualified key file name */
    DB_TCHAR      *str;
    DB_TCHAR       open_mode[3];
    DB_TASK       *task = NULL;
    LMC_AVAIL_FCN *avail = NULL;
    SG            *sg = NULL;
#if defined(SAFEGARDE)
    DB_TCHAR      *cp;
    DB_TCHAR      *password;
    int            mode = NO_ENC;
#endif

    KEYPACK_OPTS   opts;
    KEYPACK_STATS  stats;

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Key File Packing")));

    /* initialize variables */
    unused_slots = 1;
    memset(&opts, 0, sizeof(KEYPACK_OPTS));
    memset(&stats, 0, sizeof(KEYPACK_STATS));

    /* the defaults are in the dbstar.ini file */
    tmppath[0] = keyfile[0] = keypath[0] = 0;
    vtstrcpy(open_mode, DB_TEXT("o"));
    pages = 0;

    /* process command ddlp_g.line options */
    for (i = 1; (i < argc) && (argv[i][0] == DB_TEXT('-')); ++i)
    {
        /* process selected option */
        switch (vtotlower(argv[i][1]))
        {
            case DB_TEXT('h'):
            case DB_TEXT('?'):
                return usage();

            case DB_TEXT('b'):
                if (! opts.keep)
                {
                    opts.backup = TRUE;
                    vstprintf(opts.backpath, DB_TEXT("%s%c"), argv[++i],
                            DIRCHAR);
                }
                break;

            case DB_TEXT('t'):
                if (! opts.keep)
                    vstprintf(tmppath, DB_TEXT("%s%c"), argv[++i], DIRCHAR);

                break;

            case DB_TEXT('k'):
                opts.backup = FALSE;
                opts.keep = TRUE;
                vstprintf(tmppath, DB_TEXT("%s%c"), argv[++i], DIRCHAR);
                break;

            case DB_TEXT('p'):
                pages = vttoi(&argv[i][2]);
                break;

            case DB_TEXT('u'):
                unused_slots = vttoi(&argv[i][2]);
                break;

            case DB_TEXT('o'):
                vtstrcpy(open_mode, DB_TEXT("o"));
                break;

            case DB_TEXT('x'):
                vtstrcpy(open_mode, DB_TEXT("x"));
                break;

            case DB_TEXT('m'):
                switch(vtotlower(argv[i][2]))
                {
                    case DB_TEXT('n'):
                        str = NULL;
                        break;

                    case DB_TEXT('t'):
                        str = DB_TEXT("TCP");
                        break;

                    case DB_TEXT('p'):
                        str = DB_TEXT("IP");
                        break;

                    default :
                        vtprintf(DB_TEXT("** UNKNOWN lockmgr type: %s\n"),
                                argv[i]);
                        return usage();
                }

                if (str)
                    avail = psp_lmcFind(str);

                break;

            case DB_TEXT('s'):
                if (vtotlower(argv[i][2]) != DB_TEXT('g'))
                {
                    vftprintf(stderr, DB_TEXT("Invalid argument %s\n"),
                            argv[i]);
                    return usage();
                }

                if (i == argc - 1)
                {
                    vftprintf(stderr, DB_TEXT("No password specified\n"));
                    return usage();
                }

#if defined(SAFEGARDE)
                if ((cp = vtstrchr(argv[++i], DB_TEXT(','))) != NULL)
                {
                    *cp++ = DB_TEXT('\0');
                    if (vtstricmp(argv[i], DB_TEXT("low")) == 0)
                        mode = LOW_ENC;
                    else if (vtstricmp(argv[i], DB_TEXT("med")) == 0)
                        mode = MED_ENC;
                    else if (vtstricmp(argv[i], DB_TEXT("high")) == 0)
                        mode = HIGH_ENC;
                    else
                    {
                        vftprintf(stderr, DB_TEXT("Invalid SafeGarde encryption mode\n"));
                        return usage();
                    }

		    password = cp;
                }
                else
                {
                    mode = MED_ENC;
                    password = argv[i];
                }

                break;
#else
                vftprintf(stderr, DB_TEXT("SafeGarde not available in this version\n"));
                return usage();
#endif

            default:
                vtprintf(DB_TEXT("** UNKNOWN Argument: %s\n"), argv[i]);
                return usage();
        }
    }

    /* save database name */
    if (i == argc)
    {
        vftprintf(stderr, DB_TEXT("No database specified\n"));
        return usage();
    }

    dbname = argv[i++];

    if (i < argc)
    {
        /* convert key file name to lower case and save */
        vtstrcpy(keyfile, vtstrlwr(argv[i]));
    }

    if ((stat = d_opentask(&task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to open task (%d)\n"), stat);
        return 1;
    }

#if defined(SAFEGARDE)
    if (mode != NO_ENC && (sg = sg_create(mode, password)) == NULL)
    {
        vftprintf(stderr, DB_TEXT("Failed to create SafeGarde context\n"));
        goto exit;
    }
#endif

    if ((stat = d_set_dberr(keypack_dberr, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to set error handler (%d)\n"), stat);
        goto exit;
    }

    if (pages && (stat = d_setpages(pages, 1, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to set number of cache pages (%d)\n"),
                stat);
        goto exit;
    }

    if ((stat = d_lockcomm(avail, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to set lock manager type (%d)\n"),
                stat);
        goto exit;
    }

    /* open and process database */
    if ((stat = d_open_sg(dbname, open_mode, sg, task)) != S_OKAY)
    {
        if (stat == S_UNAVAIL)
            vftprintf(stderr, DB_TEXT("Database %s is unavailable\n"), dbname);
        else
            vftprintf(stderr, DB_TEXT("Failed opening database %s (%d)\n"),
                    dbname, stat);

        goto exit;
    }

    start_fno = 0;
    end_fno = task->size_ft;

    if (keyfile[0] && keyfile[0] != DB_TEXT('*'))
    {
        /* pack selected key file */
        vstprintf(keypath, DB_TEXT("%s%s"), task->dbfpath, keyfile);
        for (fno = start_fno = end_fno = 0; fno < task->size_ft; fno++)
        {
            /* find task->file_table entry for keyfile */
            if (!vtstrcmp(task->file_table[fno].ft_name, keypath))
            {
                start_fno = fno;
                end_fno = fno + 1;
                break;
            }
        }

        if (fno == task->size_ft)
        {
            vftprintf(stderr,
                    DB_TEXT("unable to find key file %s in database %s\n"),
                    keyfile, dbname);
        }
    }

    for (fno = start_fno; fno < end_fno; fno++)
    {
        if (task->file_table[fno].ft_type == 'k')
        {
            ptr = vtstrrchr(task->file_table[fno].ft_name, DIRCHAR);
            if (ptr == NULL)
                ptr = task->file_table[fno].ft_name;
            else
                ptr++;

            if (opts.keep)
                vstprintf(keypath, DB_TEXT("%s%s"), tmppath, ptr);
            else
                vstprintf(keypath, DB_TEXT("%s%s"), tmppath, DB_TEXT("_dbstar_.key"));

            vtprintf(DB_TEXT("-- packing key file: %s"),
                    task->file_table[fno].ft_name);

            task->db_status = S_OKAY;
            stat = packfile(fno, keypath, unused_slots, &opts, &stats, task);
            if (stat != S_OKAY)
            {
                vftprintf(stderr, DB_TEXT("Failed to pack file %s (%d)\n"),
                        task->file_table[fno].ft_name, stat);
                d_close(task);
                goto exit;
            }
        }
    }

    if (stats.or_total)
    {
        vtprintf(DB_TEXT("\nTotal reduction %.0f bytes or %2.2f%%\n"),
                (double) stats.or_total - stats.pk_total, (1.0 -
                (double) stats.pk_total / stats.or_total) * 100.0);
    }

    d_close(task);

exit:
#if defined(SAFEGARDE)
    if (sg)
        sg_delete(sg);
#endif

    if (task)
        d_closetask(task);

    return 0;
}

/* ======================================================================
    Print keypack usage message
*/
static int usage()
{
    vftprintf(stderr, DB_TEXT("usage:  keypack [options] dbname [file ...] \n"));
    vftprintf(stderr, DB_TEXT("options: [-{o|x}] [-b dir] [-k dir] [-t dir] [-p#] [-u#] [-m{n|t|p}]\n"));
    vftprintf(stderr, DB_TEXT("         [-sg [<mode>,]<password>]\n"));
    vftprintf(stderr, DB_TEXT("         -o     open database in one user mode (default)\n"));
    vftprintf(stderr, DB_TEXT("         -x     open database in exclusive mode\n"));
    vftprintf(stderr, DB_TEXT("         -b     store a backup copy of key files in dir\n"));
    vftprintf(stderr, DB_TEXT("         -k     store packed file in dir keeping original\n"));
    vftprintf(stderr, DB_TEXT("         -t     store packed file in dir replacing original\n"));
    vftprintf(stderr, DB_TEXT("         -p     set pages in db.* cache to # (default=%d)\n"), DEFDBPAGES);
    vftprintf(stderr, DB_TEXT("         -u     set unused slots per page to # (default=1)\n"));
    vftprintf(stderr, DB_TEXT("         -m     set the lock manager type.  Valid types are\n"));
    vftprintf(stderr, DB_TEXT("                 n - None (default, single user only)\n"));
    vftprintf(stderr, DB_TEXT("                 t - TCP/IP\n"));
    vftprintf(stderr, DB_TEXT("                 p - UNIX domain sockets (if available)\n"));
    vftprintf(stderr, DB_TEXT("         -sg    Specify SafeGarde encryption information\n"));
    vftprintf(stderr, DB_TEXT("                 <mode> can be 'low', 'med' (default), or 'high'\n"));
    vftprintf(stderr, DB_TEXT("         dbname name of db.* database\n"));
    vftprintf(stderr, DB_TEXT("         file   key files to pack; if omitted, packs all key files\n\n"));
    vftprintf(stderr, DB_TEXT("Note: The directory names must NOT end '%c'\n"), DIRCHAR);
    vftprintf(stderr, DB_TEXT("Note: The directories must already exist.\n"));
    
    return(1);
}


VXSTARTUP("keypack", 0)

