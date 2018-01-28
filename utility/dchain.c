/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dchain utility                                    *
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

    dchain.c - db.* Delete Chain Sort Utility

    This utility will reconstruct the delete chain in each file of a
    database (or a subset of files if they are listed on the command
    line).  The delete chain header will point the first physical
    deleted record, which will point to the next, etc.

    The result is a delete chain that is sorted by database address.

-----------------------------------------------------------------------*/

#define MOD dchain
#include "db.star.h"
#include "version.h"

int usage(void);


void EXTERNAL_FCN dchain_dberr(int errnum, DB_TCHAR *errmsg)
{
    vtprintf(DB_TEXT("\n%s (errnum = %d)\n"), errmsg, errnum);
}


/* db.* Delete Chain Sort Utility */

int MAIN(int argc, DB_TCHAR *argv[])
{
    int                  status;
    F_ADDR               delcount;
    F_ADDR               rno;
    F_ADDR               top;
    char                *rec;
    short                rid;
    DB_ADDR              prevdba;
    DB_ADDR              dba;
    F_ADDR               null_addr = 0L;
    register FILE_NO     fno;
    register int         i;
    int                  j;
    DB_TASK             *task = NULL;
    DB_TCHAR            *dbname;
    SG                  *sg = NULL;
#if defined(SAFEGARDE)
    DB_TCHAR            *cp;
    DB_TCHAR            *password;
    int                  mode;
#endif

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Delete Chain Sort")));

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != DB_TEXT('-'))
            break;

        switch(vtotlower(argv[i][1]))
        {
            case DB_TEXT('h'):
            case DB_TEXT('?'):
                return usage();

            case DB_TEXT('s'):
                if (vtotlower(argv[i][2]) != DB_TEXT('g') || i == argc - 1)
                {
                    vftprintf(stderr, DB_TEXT("Invalid argument %s\n"), argv[i]);
                    return usage();
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
                vftprintf(stderr, DB_TEXT("SafeGarde is not available in this version\n"));
                return usage();
#endif

            default:
                vftprintf(stderr, DB_TEXT("Invalid argument %s\n"), argv[i]);
        }
    }

    if (i == argc)
    {
        vftprintf(stderr, DB_TEXT("No database name specified\n"));
        return usage();
    }

    dbname = argv[i++];

    if ((status = d_opentask(&task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to open task (%d)\n"), status);
        return 1;
    }

#if defined(SAFEGARDE)
    if ((sg = sg_create(mode, password)) == NULL)
    {
        vftprintf(stderr, DB_TEXT("Coult not create SafeGarde context\n"));
        goto exit;
    }
#endif

    if ((status = d_set_dberr(dchain_dberr, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to set error handler (%d)\n"), status);
        goto exit;
    }

    if ((status = d_open_sg(dbname, DB_TEXT("o"), sg, task)) != S_OKAY)
    {
        if (status == S_UNAVAIL)
            vftprintf(stderr, DB_TEXT("database %s unavailable\n"), dbname);
        else
            vftprintf(stderr, DB_TEXT("Failed to open database %s (%d)\n"),
                    dbname, status);

        goto exit;
    }

    /* scan each data file */
    for (fno = 0; fno < task->size_ft; ++fno)
    {
        prevdba = NULL_DBA;
        delcount = (F_ADDR)0;

        /* skip non-data files */
        if (task->file_table[fno].ft_type != DATA)
            continue;

        /* if there is a selected list of files, check for this one */
        if (i < argc)
        {
            for (j = i; j < argc; j++)
            {
                if (vtstrcmp(task->file_table[fno].ft_name, argv[j]) == 0)
                    break;
            }

            if (j == argc)
                continue;
        }

        vftprintf(stderr,
            DB_TEXT("processing data file: %s, maximum slot count = %7ld\n"),
            task->file_table[fno].ft_name, top = dio_pznext(fno, task));

        /* read each record in data file */
        for (rno = (F_ADDR) 1; rno < top; ++rno)
        {
            /* read next record */
            d_encode_dba(fno, rno, &dba);
            status = dio_read(dba, &rec, NOPGHOLD, task);
            if (status != S_OKAY)
                break;

            /* get record identification number */
            memcpy(&rid, rec, sizeof(short));

            /* if record is deleted */
            if (rid < 0)
            {
                delcount++;

                /* make this deleted record the current end-of-chain */
                memcpy(rec + sizeof(short), &null_addr, sizeof(F_ADDR));
                status = dio_write(dba, NOPGFREE, task);
                if (status != S_OKAY)
                    break;

                /* if there was a previous deleted record */
                if (prevdba != NULL_DBA)
                {
                    /* point that record to this one */
                    status = dio_read(prevdba, &rec, NOPGHOLD, task);
                    if (status != S_OKAY)
                        break;
                    memcpy(rec + sizeof(short), &rno, sizeof(F_ADDR));
                    status = dio_write(prevdba, NOPGFREE, task);
                    if (status != S_OKAY)
                        break;
                }
                else
                {
                    /* else point page zero to this one */
                    task->pgzero[fno].pz_dchain = rno;
                    task->pgzero[fno].pz_modified = TRUE;
                }

                /* save this deleted record's address */
                prevdba = dba;
            }
        } /* for loop, records */

        if (status != S_OKAY)
            break;

        if (delcount == 0)
        {
            vftprintf(stderr, DB_TEXT("      no deleted record slots\n"));
            if (task->pgzero[fno].pz_dchain != NULL_DBA)
            {
                vftprintf(stderr, DB_TEXT("      dchain was not equal to NULL_DBA\n"));
                task->pgzero[fno].pz_dchain = NULL_DBA;
                task->pgzero[fno].pz_modified = TRUE;
            }
        }
        else
        {
            vftprintf(stderr,
            DB_TEXT("      %ld deleted record slot%s sorted\n"),
                delcount, delcount == 1L ? DB_TEXT("") : DB_TEXT("s"));
        }
    } /* for loop, files */

    vftprintf(stderr, DB_TEXT("\ndelete chain sort completed\n"));
    d_close(task);

exit:
    if (task)
        d_closetask(task);

    return 0;
}

/* print usage message and exit
*/
int usage()
{
    vftprintf(stderr, DB_TEXT("usage: dchain [-sg [<mode>,]<password>] dbname [filename ...]\n"));
    vftprintf(stderr, DB_TEXT("where: -sg Specifies the SafeGarde encryption information for the database\n"));
    vftprintf(stderr, DB_TEXT("         <mode> can be 'low', 'med' (default), or 'high'\n"));

    return 1;
}


VXSTARTUP("dchain", 0)

