/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, keybuild utility                                  *
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

    keybuild.c - db.* key file rebuilding utility

-----------------------------------------------------------------------*/

#define MOD keybuild
#include "db.star.h"
#include "version.h"

static char key[MAXKEYSIZE];


void EXTERNAL_FCN keybuild_dberr(int errnum, DB_TCHAR *msg)
{
    vtprintf(DB_TEXT("\n%s (errnum = %d)\n"), msg, errnum);
}

int usage(void)
{
    vftprintf(stderr, DB_TEXT("usage: keybuild [-p#] [-sg [<mode>,]<password>] <dbname>\n"));
    vftprintf(stderr, DB_TEXT("where: -p       specifies the number of cache pages to use (default = %d).\n"), DEFDBPAGES);
    vftprintf(stderr, DB_TEXT("       -sg      Specifies the SafeGarde encryption information for the database\n"));
    vftprintf(stderr, DB_TEXT("                 <mode> can be 'low', 'med' (default), 'high'\n"));
    vftprintf(stderr, DB_TEXT("       <dbname> is the database name of the key files to be rebuilt.\n"));

    return 1;
}

/* db.* key file build utility */

int MAIN(int argc, DB_TCHAR *argv[])
{
    F_ADDR      rno,
                top;
    char       *rec,
               *fptr;
    short       rid;
    DB_ADDR     dba;
    FILE_NO     fno;
    int         i;
    int         status;
    int         pages = 0;
    int         key_fields;
    int         key_files = 0;
    DB_TCHAR   *dbname;
    DB_TASK    *task = NULL;
    SG         *sg = NULL;
#if defined(SAFEGARDE)
    DB_TCHAR   *cp;
    DB_TCHAR   *password;
    int         mode = NO_ENC;
#endif

    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Key File Build")));

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != DB_TEXT('-'))
            break;

  
        switch (vtotlower(argv[i][1]))
        {
            case DB_TEXT('h'):
            case DB_TEXT('?'):
                return usage();

            case DB_TEXT('s'):
                if (vtotlower(argv[i][2]) != DB_TEXT('g'))
                {
                    vftprintf(stderr, DB_TEXT("Invalid option %s\n"), argv[i]);
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
                        vftprintf(stderr, DB_TEXT("Invalid encryption mode %s\n"), argv[i]);
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
                return 1;
#endif

            case DB_TEXT('p'):
                if ((pages = vttoi(&argv[i][2])) < 0)
                {
                    vftprintf(stderr, DB_TEXT("Invalid number of pages %d\n"),
                            pages);
                    return usage();
                }

                break;

            default:
                vftprintf(stderr, DB_TEXT("Invalid option %s\n"), argv[i]);
                return usage();
        }
    }

    if (i == argc)
    {
        vftprintf(stderr, DB_TEXT("No database name specified\n"));
        return usage();
    }

    dbname = argv[i++];

    while (i < argc)
        vftprintf(stderr, DB_TEXT("Ignoring option '%s' after the database name\n"), argv[i++]);

    if ((status = d_opentask(&task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Unable to open task (%d)\n"), status);
        return 1;
    }

#if defined(SAFEGARDE)
    if (mode != NO_ENC && (sg = sg_create(mode, password)) == NULL)
    {
        vftprintf(stderr, DB_TEXT("Unable to create SafeGarde context\n"));
        goto exit;
    }
#endif

    if ((status = d_set_dberr(keybuild_dberr, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Unable to set error handler (%d)\n"),
                status);
        goto exit;
    }

    if (pages && (status = d_setpages(pages, 1, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Unable to set number of cache pages (%d)\n"),
                status);
        goto exit;
    }

    if ((status = d_open_sg(dbname, DB_TEXT("o"), sg, task)) != S_OKAY)
    {
        if (status == S_UNAVAIL)
            vftprintf(stderr, DB_TEXT("database %s unavailable\n"), dbname);
        else
            vftprintf(stderr, DB_TEXT("Unable to open database %s (%d)\n"),
                    dbname, status);

        goto exit;
    }

    /* initialize all key files */
    for (fno = 0; fno < task->size_ft; ++fno)
    {
        if (task->file_table[fno].ft_type == KEY)
        {
            vftprintf(stderr, DB_TEXT("initializing key file: %s\n"),
                    task->file_table[fno].ft_name);
            if ((status = d_initfile(fno, task, CURR_DB)) != S_OKAY)
            {
                d_close(task);
                goto exit;
            }

            key_files++;
        }
    }

    if (!key_files)
    {
        vftprintf(stderr, DB_TEXT("database %s has no keys\n"), dbname);
        d_close(task);
        goto exit;
    }

    /* scan each data file */
    for (fno = 0; fno < task->size_ft; ++fno)
    {
        if (task->file_table[fno].ft_type != DATA)
            continue;

        key_fields = 0;

        for (rid = 0; rid < task->size_rt; ++rid)
        {
            if ( task->record_table[rid].rt_file == fno )
            {
                int fld;

                for ( fld = task->record_table[rid].rt_fields;
                        fld < task->size_fd &&
                        task->field_table[fld].fd_rec == rid; ++fld)
                {
                    if ( task->field_table[fld].fd_key != NOKEY )
                    {
                        key_fields++;
                        break;
                    }
                }
            }
        }

        if ( !key_fields )
            continue;

        vftprintf(stderr,
                DB_TEXT("\nprocessing data file: %s, total records = %7ld,"),
                task->file_table[fno].ft_name, top = dio_pznext(fno, task) - 1);

        vftprintf(stderr, DB_TEXT(" record:        0"));

        /* read each record in data file */
        for (rno = 1; rno <= top; ++rno)
        {
            if (rno % 10 == 0)
                vftprintf(stderr, DB_TEXT("\b\b\b\b\b\b\b\b%8ld"), rno);

            /* read next record */
            d_encode_dba(fno, rno, &dba);
            if ((status = dio_read(dba, &rec, PGHOLD, task)) != S_OKAY)
            {
                vftprintf(stderr, DB_TEXT("Failure %d during dio_read\n"),
                        status);
                d_close(task);
                goto exit;
            }

            /* get record identification number */
            memcpy(&rid, rec, sizeof(short));

            /* remove the record lock bit */
            rid &= ~RLBMASK;

            if (rid >= task->size_rt)
            {
                dberr(S_INVREC);
                d_close(task);
                goto exit;
            }

            if (rid >= 0)
            {                       /* record not deleted */
                /* for each key field, enter the key value
                   into the key file
                */
                for (i = task->record_table[rid].rt_fields; i < task->size_fd &&
                        task->field_table[i].fd_rec == rid; i++)
                {
                    /* skip if not a key field */
                    if (task->field_table[i].fd_key == NOKEY)
                        continue;

                    /* skip if optional and not stored */
                    if ((task->field_table[i].fd_flags & OPTKEYMASK) &&
                            !r_tstopt(&task->field_table[i], rec, task))
                        continue;

                    /* build key if compound */
                    if (task->field_table[i].fd_type == COMKEY)
                    {
                        key_bldcom(i, rec + task->record_table[rid].rt_data,
                                key, TRUE, task);
                        fptr = key;
                    }
                    else
                        fptr = rec + task->field_table[i].fd_ptr;

                    /* insert the key */
                    if ((status = key_insert(i, fptr, dba, task)) != S_OKAY)
                    {
                        vftprintf(stderr,
                                DB_TEXT("Failure %d during key_insert\n"),
                                status);
                        d_close(task);
                        goto exit;
                    }
                }
            }

            /* free page hold */
            if ((status = dio_release(dba, PGFREE, task)) != S_OKAY)
            {
                vftprintf(stderr, DB_TEXT("Failure %d during dio_release\n"),
                        status);
                goto exit;
            }
        }

        vftprintf(stderr, DB_TEXT("\b\b\b\b\b\b\b\b%8ld"), rno - 1);
    }

    d_close(task);

    vftprintf(stderr, DB_TEXT("\n\nkey file rebuild completed\n"));

exit:
#if defined(SAFEGARDE)
    if (sg)
        sg_delete(sg);
#endif

    if (task)
        d_closetask(task);

    return 0;
}


VXSTARTUP("keybuild", 0)

