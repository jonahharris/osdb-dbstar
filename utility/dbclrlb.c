/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbclrlb utility                                   *
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

    dbclrlb - db.* Clear Record Lock Bit Utility

    To execute:
        dbclrlb dbname [dbfile]

    where:
        dbname  = name of database to be checked
        dbfile  = if specified check only this file

-----------------------------------------------------------------------*/

#define MOD dbclrlb
#include "db.star.h"
#include "version.h"

/* ****************** EXTERNAL VARIABLE DECLARATIONS ***************** */
static void usage(void);
static int  data_file(FILE_NO, DB_TASK *);


void EXTERNAL_FCN dbclrlb_dberr(int errnum, DB_TCHAR *errmsg)
{
    vtprintf(DB_TEXT("\n%s (errnum = %d)\n"), errmsg, errnum);
}


/* ------------------------------------------------------------------------
    db.* Clear Record Lock Bit Utility
*/
int MAIN(int argc, DB_TCHAR *argv[])
{
    int         i;
    int         status;
    FILE_NO     fno;
    DB_TCHAR   *dbname;
    DB_TASK    *task = NULL;
    SG         *sg = NULL;
#if defined(SAFEGARDE)
    DB_TCHAR   *cp;
    DB_TCHAR   *password;
    int         mode;
#endif

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Clear Record Lock Bit")));

    psp_init();

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != DB_TEXT('-'))
            break;

        switch (tolower(argv[i][1]))
        {
            case DB_TEXT('?'):
            case DB_TEXT('h'):
	        usage();
		goto exit;

            case DB_TEXT('s'):
                if (tolower(argv[i][2]) != DB_TEXT('g') || i == argc - 1)
                {
                    vtprintf(DB_TEXT("Invalid argument %s\n"), argv[i]);
		    usage();
		    goto exit;
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
                    else
                    {
                        vtprintf(DB_TEXT("Invalid encryption mode\n"));
			usage();
			goto exit;
                    }

                    password = cp;
                }
                else
                {
                    mode = MED_ENC;
                    cp = argv[i];
                }

                if ((sg = sg_create(mode, password)) == NULL)
                {
                    vtprintf(DB_TEXT("Unable to create SafeGarde context\n"));
		    goto exit;
                }

                break;
#else
                vtprintf(DB_TEXT("SafeGarde encryption is not available in this version\n"));
		goto exit;
#endif

            default:
                vtprintf(DB_TEXT("Invalid argument %s\n"), argv[i]);
		usage();
		goto exit;
        }
    }

    if (i == argc)
    {
        vtprintf(DB_TEXT("Database name not specified\n"));
	usage();
	goto exit;
    }

    dbname = argv[i++];
    if ((status = d_opentask(&task)) != S_OKAY)
    {
        vtprintf(DB_TEXT("Failed to open task (%d)\n"), status);
        goto exit;
    }

    if ((status = d_set_dberr(dbclrlb_dberr, task)) != S_OKAY)
    {
        vtprintf(DB_TEXT("Failed to set error handler (%d)\n"), status);
        goto exit;
    }

    if ((status = d_open_sg(dbname, DB_TEXT("o"), sg, task)) != S_OKAY)
    {
        if (status == S_UNAVAIL)
            vtprintf(DB_TEXT("database %s unavailable\n"), dbname);
        else
            vtprintf(DB_TEXT("Failed to open database %s (%d)\n"), dbname, status);

        goto exit;
    }

    if (i < argc)
    {
        /* files were specified */
        for ( ; i < argc; i++)
        {
            /* scan each database file */
            for (fno = 0; fno < task->size_ft; fno++)
            {
                if (vtstrcmp(task->file_table[fno].ft_name, argv[i]) == 0)
                    break;
            }

            if (fno == task->size_ft)
            {
                vtprintf(DB_TEXT("file %s not found\n"), argv[i]);
                continue;
            }

            if (task->file_table[fno].ft_type == DATA)
            {
                if (data_file(fno, task) != S_OKAY)
                    break;
            }
            else if (task->file_table[fno].ft_type == KEY)
            {
                vtprintf(DB_TEXT("\nError - can not clear lock bit from key file %s"),
                        task->file_table[fno].ft_name);
            }
        }
    }
    else
    {
        /* no files specified, do all files */
        for (fno = 0; fno < task->size_ft; fno++)
        {
            if (task->file_table[fno].ft_type == DATA)
            {
                if (data_file(fno, task) != S_OKAY)
                    break;
            }
        }
    }

    d_close(task);

exit:
#if defined(SAFEGARDE)
    if (sg)
        sg_delete(sg);
#endif

    if (task)
        d_closetask(task);

    psp_term();
    return 0;
}


/* ------------------------------------------------------------------------
    Clear Lock Bits
*/
static int data_file(FILE_NO fno, DB_TASK *task)
{
    int         status = S_OKAY;
    F_ADDR      rno,
                top;
    short       rid;
    char       *recptr;
    DB_ADDR     dba;

    vtprintf(DB_TEXT("\n\n------------------------------------"));
    vtprintf(DB_TEXT("------------------------------------\n\n"));
    vtprintf(DB_TEXT("PROCESSING DATA FILE: %s, TOTAL OF %ld SLOTS\n"),
             task->file_table[fno].ft_name,
             top = dio_pznext(fno, task) - (F_ADDR)1);

    /* read each record in data file */
    for (rno = 1; rno <= top && status == S_OKAY; ++rno)
    {
        /* read next record */
        d_encode_dba(fno, rno, &dba);

        /* read rlb */
        status = dio_read(dba, (char **) &recptr, PGHOLD, task);
        if (status != S_OKAY)
            return (status);

        memcpy(&rid, recptr, sizeof(short));

        if (rid < 0)
        {
            rid = (short) ~rid;
            if (rid & RLBMASK)
            {
                rid &= ~RLBMASK;
                rid = (short) ~rid;
                memcpy(recptr, &rid, sizeof(short));
                status = dio_write(dba, PGFREE, task);
            }
            else
            {
                status = dio_release(dba, PGFREE, task);
            }
        }
        else if (rid & RLBMASK)
        {
            rid &= ~RLBMASK;
            memcpy(recptr, &rid, sizeof(short));
            status = dio_write(dba, PGFREE, task);
        }
        else
        {
            status = dio_release(dba, PGFREE, task);
        }
    }
    return (status);
}

/* ------------------------------------------------------------------------ */

static void usage()
{
    vtprintf(DB_TEXT("usage: dbclrlb [-sg [<mode>,]<password>] dbname [dbfile ...]\n"));
    vtprintf(DB_TEXT("where: -sg Specifies the SafeGarde encryption information for the database\n"));
}


VXSTARTUP("dbclrlb", 0)

