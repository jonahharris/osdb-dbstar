/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, initdb utility                                    *
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

    initdb.c - db.* Database Initialization Utility

    This utility initializes all of the files specified in a data
    dictionary.  A data file will have its page zero initialized unless
    it contains the system record, in which case page one is also
    initialized.  A key file will always have both page one and two
    initialized.

-----------------------------------------------------------------------*/

#define MOD initdb
#include "db.star.h"
#include "version.h"

/**************************************************************************/

void EXTERNAL_FCN initdb_dberr(int errnum, DB_TCHAR *msg)
{
    vtprintf(DB_TEXT("\n%s (errnum = %d)\n"), msg, errnum);
}

int usage(const DB_TCHAR *str1, const DB_TCHAR *str2)
{
    vtprintf(DB_TEXT("%s%s\n"), str1, str2 ? str2 : DB_TEXT(""));
    vtprintf(DB_TEXT("\nusage:  initdb [-y] [-sg [<mode>,]<password>] dbname ...\n"));
    vtprintf(DB_TEXT("where: -y  Initialize files without double checking with user\n"));
    vtprintf(DB_TEXT("       -sg Specifies the encryption information (SafeGarde only)\n"));
    vtprintf(DB_TEXT("            <mode> can be 'low', 'med' (default), or 'high'\n"));

    return -1;
}


int MAIN(int argc, DB_TCHAR *argv[])
{
    DB_TCHAR  reply[20];
    int       ii;
    SG       *sg = NULL;
    FILE_NO   fno;
    int       yes = 0;
    int       status = S_OKAY;
    DB_TASK  *task;
#if defined(SAFEGARDE)
    DB_TCHAR *cp;
    DB_TCHAR *password = NULL;
    int       mode = NO_ENC;
#endif

    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );

    psp_init();

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Database Initialization")));

    for (ii = 1; ii < argc; ii++)
    {
        if (*argv[ii] != DB_TEXT('-'))
            break;

        switch (argv[ii][1])
        {
	    case DB_TEXT('h'):
	    case DB_TEXT('?'):
	        return usage("", NULL);

            case DB_TEXT('y'):
                yes = 1;
                break;

            case DB_TEXT('s'):
                if (argv[ii][2] != DB_TEXT('g'))
                    return usage(DB_TEXT("Invalid argument: "), argv[ii]);

#if defined(SAFEGARDE)
                if (ii == argc - 1)
                    return usage(DB_TEXT("No password specified"), NULL);

                if ((cp = vtstrchr(argv[++ii], DB_TEXT(','))) != NULL)
                    *cp++ = DB_TEXT('\0');

                if (cp)
                {
                    if (vtstricmp(argv[ii], "low") == 0)
                        mode = LOW_ENC;
                    else if (vtstricmp(argv[ii], "med") == 0)
                        mode = MED_ENC;
                    else if (vtstricmp(argv[ii], "high") == 0)
                        mode = HIGH_ENC;
                    else
                        return usage(DB_TEXT("Invalid encryption mode"), NULL);

                    password = cp;
                }
                else
                {
                    mode = MED_ENC;
                    password = argv[ii];
                }
#else
                return usage(DB_TEXT("SafeGarde not available in this version"), NULL);
#endif

                break;

            default:
                return usage(DB_TEXT("Invalid argument: "), argv[ii]);
        }
    }

    if (ii == argc)
        return usage(DB_TEXT("Invalid argument: No database specified"), NULL);

#if defined(SAFEGARDE)
    if (mode != NO_ENC && (sg = sg_create(mode, password)) == NULL)
    {
        vtprintf(DB_TEXT("Error creating SafeGarde encryption context\n"));
        return -1;
    }
#endif

    if ((status = d_opentask(&task)) != S_OKAY)
        return status;

    if ((status = d_set_dberr(initdb_dberr, task)) != S_OKAY)
        goto exit;

    /* Initialize each database in argv list */
    for (; ii < argc; ++ii)
    {
        /* open one database at a time */
        if ((status = d_open_sg(argv[ii], DB_TEXT("o"), sg, task)) != S_OKAY)
        {
            switch (status)
            {
                case S_INVDB:
                    vftprintf(stderr,
                        DB_TEXT("Unable to locate database: %s\n"), argv[ii]);
                    break;

                case S_INCOMPAT:
                    vftprintf(stderr,
                        DB_TEXT("Incompatible dictionary file. Re-run ddlp.\n"));
                    break;

                case S_INVENCRYPT:
                    vftprintf(stderr,
                            DB_TEXT("Invalid encryption parameters specified for this database\n"));
                    break;

                default:
                    vftprintf(stderr,
                        DB_TEXT("Error %d opening database, %s not initialized.\n"),
                        status, argv[ii]);
                    break;
            }
            break;
        }

        if (!yes)
        {
            PSP_FH f;

            /* see if any of the files exist */
            for (fno = 0; fno < task->size_ft; ++fno)
            {
                if ((f = psp_fileOpen(task->file_table[fno].ft_name, O_RDONLY, PSP_FLAG_DENYNO)) != NULL)
                {
                    psp_fileClose(f);
                    break;
                }
            }

            if (fno < task->size_ft)
            {
                /* Make sure user wants to destroy any existing data */
                vtprintf(DB_TEXT("Initialization of database: %s\n\n"), argv[ii]);
                vtprintf(DB_TEXT("WARNING: this will destroy contents of the following files:\n\n"));
                for (fno = 0; fno < task->size_ft; ++fno)
                    vtprintf(DB_TEXT("   %s\n"), task->file_table[fno].ft_name);

                vtprintf(DB_TEXT("\ncontinue? (y/n) "));
                vtscanf(DB_TEXT("%s"), reply);
                if (reply[0] != DB_TEXT('y') && reply[0] != DB_TEXT('Y'))
                    break;

                vtprintf(DB_TEXT("\n"));
            }
        }

        /* Delete files, as truncation on open doesn't always work */
        for (fno = 0; fno < task->size_ft; ++fno)
            psp_fileRemove(task->file_table[fno].ft_name);

        /* Initialze the whole database */
        status = d_initialize(task, CURR_DB);
        d_close(task);

        if (status == S_OKAY)
            vtprintf(DB_TEXT("%s initialized\n\n"), argv[ii]);
        else
        {
            vtprintf(DB_TEXT("failed to initialize %s\n\n"), argv[ii]);
            break;
        }
    }

exit:
    d_closetask(task);
    psp_term();
    return(status);
}


VXSTARTUP("initdb", 0)

