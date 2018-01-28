/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, prdbd utility                                     *
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

    prdbd.c - db.* Database Dictionary Report Utility

    This program produces a formatted report of the database dictionary.

-----------------------------------------------------------------------*/

#define MOD prdbd
#include "db.star.h"
#include "version.h"

/* ********************** LOCAL FUNCTION DECLARATIONS **************** */
static int usage(void);


void EXTERNAL_FCN prdbd_dberr(int errnum, DB_TCHAR *msg)
{
    vtprintf(DB_TEXT("\n%s (errnum = %d)\n"), msg, errnum);
}


/* Print db.* Database Schema Tables
*/
int MAIN(int argc, DB_TCHAR *argv[])
{
    DB_TCHAR *dbname = NULL;
    DB_TCHAR  version[80];
    DB_TCHAR *pv = DB_TEXT("");
    long      m;
    int       i, j, v;
    int       stat, symbolic = 1;
    DB_TASK  *task = NULL;
    SG       *sg = NULL;
#if defined(SAFEGARDE)
    DB_TCHAR *cp;
    DB_TCHAR *password;
    int       mode = NO_ENC;
#endif

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Database Dictionary Print")));

    for (i = 1; i < argc && argv[i][0] == DB_TEXT('-'); i++)
    {
        switch (vtotlower(argv[i][1]))
        {
            case DB_TEXT('?'):
            case DB_TEXT('h'):
                return usage();

            case DB_TEXT('c'):
                symbolic = 0;
                break;

            case DB_TEXT('s'):
                if (vtotlower(argv[i][2]) != DB_TEXT('g'))
                {
                    vftprintf(stderr, DB_TEXT("Invalid option: %s\n"), argv[i]);
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
                        vftprintf(stderr, DB_TEXT("Invalid encryption mode\n"));
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

            default:
                vftprintf(stderr, DB_TEXT("Invalid option: %s\n"), argv[i]);
                return usage();
        }
    }

    if (i == argc)
    {
        vftprintf(stderr, DB_TEXT("Database name not supplied\n"));
        return usage();
    }

    dbname = argv[i++];

    while (i == argc)
        vftprintf(stderr, DB_TEXT("Ignoring option %s\n"), argv[i++]);

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

    if ((stat = d_set_dberr(prdbd_dberr, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to set error handler (%d)\n"), stat);
        goto exit;
    }

    if ((stat = d_on_opt(READNAMES, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to set options (%d)\n"), stat);
        goto exit;
    }

    if ((stat = d_open_sg(dbname, DB_TEXT("o"), sg, task)) != S_OKAY)
    {
        if (stat == S_INVDB)
            vftprintf(stderr, DB_TEXT("Unable to locate database: %s\n"),
                    dbname);
        else if (stat == S_INCOMPAT)
            vftprintf(stderr,
                    DB_TEXT("Incompatible dictionary file. Re-run ddlp.\n"));
        else
            vftprintf(stderr,
                    DB_TEXT("Error %d opening database, %s not printed.\n"),
                    stat, dbname);

        goto exit;
    }
        
    /* get dbd version */
    v = (int) DB_REF(db_ver);
    if (v < 0)
    {
        v *= -1;
        pv = DB_TEXT(" Unicode");
    }

    /* compute memory reqts for dictionary tables */
    m =  (long) task->size_ft  *  (long) sizeof(FILE_ENTRY);
    m += (long) task->size_rt  *  (long) sizeof(RECORD_ENTRY);
    m += (long) task->size_fd  *  (long) sizeof(FIELD_ENTRY);
    m += (long) task->size_st  *  (long) sizeof(SET_ENTRY);
    m += (long) task->size_mt  *  (long) sizeof(MEMBER_ENTRY);
    m += (long) task->size_srt *  (long) sizeof(SORT_ENTRY);
    m += (long) task->size_kt  *  (long) sizeof(KEY_ENTRY);

    d_dbver(DB_TEXT("%n Version %v"), version, sizeof(version) / sizeof(DB_TCHAR));
    vtprintf(DB_TEXT("%s\n"), version);
    vtprintf(DB_TEXT("Database Dictionary Tables for Database: %s\n"), dbname);
    vtprintf(DB_TEXT("DBD Version: %d.%02d%s\n\n"), v / 100, v % 100, pv);
    vtprintf(DB_TEXT("REQUIRED MEMORY: %ld BYTES\n\n"), m);
    vtprintf(DB_TEXT("\n\n------------------------------------------------------------------\n"));
    vtprintf(DB_TEXT("FILE TABLE:\n\n"));
    vtprintf(DB_TEXT("file sta type slots sl sz pg sz  initial     next pct flgs name\n"));
    vtprintf(DB_TEXT("---- --- ---- ----- ----- ----- -------- -------- --- ---- ----\n"));
    for (i = 0; i < task->size_ft; ++i)
    {
        vtprintf(DB_TEXT("%3d  "), i);
        vtprintf(DB_TEXT("  %c  "), task->file_table[i].ft_status);
        vtprintf(DB_TEXT("  %c  "), task->file_table[i].ft_type);
        vtprintf(DB_TEXT("%4d  "), task->file_table[i].ft_slots);
        vtprintf(DB_TEXT("%4d  "), task->file_table[i].ft_slsize);
        vtprintf(DB_TEXT("%4d "), task->file_table[i].ft_pgsize);
        vtprintf(DB_TEXT("%8ld "), task->file_table[i].ft_initial);
        vtprintf(DB_TEXT("%8ld "), task->file_table[i].ft_next);
        vtprintf(DB_TEXT("%3d "), task->file_table[i].ft_pctincrease);
        vtprintf(DB_TEXT("%04x "), task->file_table[i].ft_flags);
        vtprintf(DB_TEXT("%-24s\n"), task->file_table[i].ft_name);
    }

    vtprintf(DB_TEXT("\n\n------------------------------------------------------------------\n"));
    vtprintf(DB_TEXT("RECORD TABLE:\n\n"));
    if (!symbolic)
    {
        vtprintf(DB_TEXT("                          1st  tot\n"));
        vtprintf(DB_TEXT("rec #  file #  len  data  fld  flds  flags\n"));
        vtprintf(DB_TEXT("-----  ------ ----  ----  ---  ----  -----\n"));
        for (i = 0; i < task->size_rt; ++i)
        {
            vtprintf(DB_TEXT("%4d   %4d   "), i, task->record_table[i].rt_file);
            vtprintf(DB_TEXT("%4d  "), task->record_table[i].rt_len);
            vtprintf(DB_TEXT("%4d  "), task->record_table[i].rt_data);
            vtprintf(DB_TEXT("%3d  "), task->record_table[i].rt_fields);
            vtprintf(DB_TEXT("%4d   "), task->record_table[i].rt_fdtot);
            vtprintf(DB_TEXT("%04x\n"), task->record_table[i].rt_flags);
        }
    }
    else
    {
        vtprintf(DB_TEXT("                                       [#]first          tot\n"));
        vtprintf(DB_TEXT("   [#]record      file    len  data      field      flds flags\n"));
        vtprintf(DB_TEXT("-------------- ---------- ---- ---- --------------- ---- -----\n"));
        for (i = 0; i < task->size_rt; ++i)
        {
            vtprintf(DB_TEXT("[%2d]%-10.10s %-10.10s "),
                i, task->record_names[i],
                task->file_table[task->record_table[i].rt_file].ft_name);
            vtprintf(DB_TEXT("%4d "), task->record_table[i].rt_len);
            vtprintf(DB_TEXT("%4d "), task->record_table[i].rt_data);
            if (task->record_table[i].rt_fdtot != -1
             && task->record_table[i].rt_fdtot != 0)
            {
                vtprintf(DB_TEXT("[%3d]%-10.10s "),
                    task->record_table[i].rt_fields,
                    task->field_names[task->record_table[i].rt_fields]);
            }
            else
            {
                vtprintf(DB_TEXT("                "));
            }
            vtprintf(DB_TEXT("%4d "), task->record_table[i].rt_fdtot);
            vtprintf(DB_TEXT("%04x\n"), task->record_table[i].rt_flags);
        }
    }

    vtprintf(DB_TEXT("\n\n------------------------------------------------------------------\n"));
    vtprintf(DB_TEXT("FIELD TABLE:\n\n"));
    if (!symbolic)
    {
        vtprintf(DB_TEXT("                       key   key  record\n"));
        vtprintf(DB_TEXT("fld #  key  type  len  file  num  offset  rec #  flags  dims\n"));
        vtprintf(DB_TEXT("-----  ---  ----  ---  ----  ---  ------  -----  -----  ----\n"));
        for (i = 0; i < task->size_fd; ++i)
        {
            vtprintf(DB_TEXT("%4d    %c   "), i, task->field_table[i].fd_key);
            vtprintf(DB_TEXT("  %c   "), task->field_table[i].fd_type);
            vtprintf(DB_TEXT("%3d  "), task->field_table[i].fd_len);
            vtprintf(DB_TEXT("%4d  "), task->field_table[i].fd_keyfile);
            vtprintf(DB_TEXT("%3d  "), task->field_table[i].fd_keyno);
            vtprintf(DB_TEXT("%4d    "), task->field_table[i].fd_ptr);
            vtprintf(DB_TEXT("%3d     "), task->field_table[i].fd_rec);
            vtprintf(DB_TEXT("%04x  "), task->field_table[i].fd_flags);
            for (j = 0; j < MAXDIMS && task->field_table[i].fd_dim[j]; ++j)
                vtprintf(DB_TEXT("[%d]"), task->field_table[i].fd_dim[j]);
            vtprintf(DB_TEXT("\n"));
        }
    }
    else
    {
        vtprintf(DB_TEXT("                                key     key record\n"));
        vtprintf(DB_TEXT("   [#]field     key type len    file    num offset  [#]record     flags dims\n"));
        vtprintf(DB_TEXT("--------------- --- ---- --- ---------- --- ------ -------------- ----- ----\n"));
        for (i = 0; i < task->size_fd; ++i)
        {
            vtprintf(DB_TEXT("[%3d]%-10.10s   %c "),
                i, task->field_names[i], task->field_table[i].fd_key);
            vtprintf(DB_TEXT("   %c "), task->field_table[i].fd_type);
            vtprintf(DB_TEXT("%3d "), task->field_table[i].fd_len);
            if (task->field_table[i].fd_key != NOKEY)
            {
                vtprintf(DB_TEXT("%-10.10s "),
                    task->file_table[task->field_table[i].fd_keyfile].ft_name);
            }
            else
            {
                vtprintf(DB_TEXT("           "));
            }
            vtprintf(DB_TEXT("%3d "), task->field_table[i].fd_keyno);
            vtprintf(DB_TEXT("  %4d "), task->field_table[i].fd_ptr);
            vtprintf(DB_TEXT("[%2d]%-10.10s "),
                task->field_table[i].fd_rec,
                task->record_names[task->field_table[i].fd_rec]);
            vtprintf(DB_TEXT("%04x  "), task->field_table[i].fd_flags);
            for (j = 0; j < MAXDIMS && task->field_table[i].fd_dim[j]; ++j)
                vtprintf(DB_TEXT("[%d]"), task->field_table[i].fd_dim[j]);
            vtprintf(DB_TEXT("\n"));
        }
    }

    vtprintf(DB_TEXT("\n\n------------------------------------------------------------------\n"));
    if (task->size_st)
    {
        vtprintf(DB_TEXT("SET TABLE:\n\n"));
        if (!symbolic)
        {
            vtprintf(DB_TEXT("              owner  own ptr  # of 1st    total\n"));
            vtprintf(DB_TEXT("set #  order  rec #   offset    member  members  flags\n"));
            vtprintf(DB_TEXT("-----  -----  -----  -------  --------  -------  -----\n"));
            for (i = 0; i < task->size_st; ++i)
            {
                vtprintf(DB_TEXT("%4d     %c    "), i, task->set_table[i].st_order);
                vtprintf(DB_TEXT("%4d   "), task->set_table[i].st_own_rt);
                vtprintf(DB_TEXT(" %4d    "), task->set_table[i].st_own_ptr);
                vtprintf(DB_TEXT("  %4d    "), task->set_table[i].st_members);
                vtprintf(DB_TEXT(" %3d     "), task->set_table[i].st_memtot);
                vtprintf(DB_TEXT("%04x\n"), task->set_table[i].st_flags);
            }
        }
        else
        {
            vtprintf(DB_TEXT("                         [#]owner    own ptr first  total\n"));
            vtprintf(DB_TEXT("    [#]set      order     record     offset  member members flags\n"));
            vtprintf(DB_TEXT("--------------- ----- -------------- ------- ------ ------- -----\n"));
            for (i = 0; i < task->size_st; ++i)
            {
                vtprintf(DB_TEXT("[%3d]%-10.10s     %c "),
                    i, task->set_names[i], task->set_table[i].st_order);
                vtprintf(DB_TEXT("[%2d]%-10.10s "),
                    task->set_table[i].st_own_rt,
                    task->record_names[task->set_table[i].st_own_rt]);
                vtprintf(DB_TEXT("%7d "), task->set_table[i].st_own_ptr);
                vtprintf(DB_TEXT("%6d "), task->set_table[i].st_members);
                vtprintf(DB_TEXT("%7d "), task->set_table[i].st_memtot);
                vtprintf(DB_TEXT("%04x\n"), task->set_table[i].st_flags);
            }
        }
        vtprintf(DB_TEXT("\n\n------------------------------------------------------------------\n"));
    }

    if (task->size_mt)
    {
        vtprintf(DB_TEXT("MEMBER TABLE:\n\n"));
        if (!symbolic)
        {
            vtprintf(DB_TEXT("              mem ptr  # of 1st  total\n"));
            vtprintf(DB_TEXT("mem #  rec #   offset  sort fld  sort flds\n"));
            vtprintf(DB_TEXT("-----  -----  -------  --------  ---------\n"));
            for (i = 0; i < task->size_mt; ++i)
            {
                vtprintf(DB_TEXT("%4d   %4d   "), i, task->member_table[i].mt_record);
                vtprintf(DB_TEXT(" %4d    "), task->member_table[i].mt_mem_ptr);
                vtprintf(DB_TEXT("  %3d     "), task->member_table[i].mt_sort_fld);
                vtprintf(DB_TEXT("  %3d\n"), task->member_table[i].mt_totsf);
            }
        }
        else
        {
            vtprintf(DB_TEXT("                     mem ptr  # of 1st  total\n"));
            vtprintf(DB_TEXT("mem #   [#]record    offset   sort fld  sort flds\n"));
            vtprintf(DB_TEXT("----- -------------- -------  --------  ---------\n"));
            for (i = 0; i < task->size_mt; ++i)
            {
                vtprintf(DB_TEXT("%4d  [%2d]%-10.10s   "),
                    i,
                    task->member_table[i].mt_record,
                    task->record_names[task->member_table[i].mt_record]);
                vtprintf(DB_TEXT("%5d    "), task->member_table[i].mt_mem_ptr);
                vtprintf(DB_TEXT(" %5d     "), task->member_table[i].mt_sort_fld);
                vtprintf(DB_TEXT(" %5d\n"), task->member_table[i].mt_totsf);
            }
        }
        vtprintf(DB_TEXT("\n\n------------------------------------------------------------------\n"));
    }

    if (task->size_srt)
    {
        vtprintf(DB_TEXT("SORT TABLE:\n\n"));
        if (!symbolic)
        {
            vtprintf(DB_TEXT("sort #  field #  set #\n"));
            vtprintf(DB_TEXT("------  -------  -----\n"));
            for (i = 0; i < task->size_srt; ++i)
            {
                vtprintf(DB_TEXT("%4d    %4d    %4d\n"),
                    i, task->sort_table[i].se_fld, task->sort_table[i].se_set);
            }
        }
        else
        {
            vtprintf(DB_TEXT("sort #     [#]field          [#]set\n"));
            vtprintf(DB_TEXT("------  ---------------  ---------------\n"));
            for (i = 0; i < task->size_srt; ++i)
            {
                vtprintf(DB_TEXT("%6d  [%3d]%-10.10s  [%3d]%-10.10s\n"),
                    i,
                    task->sort_table[i].se_fld,
                    task->field_names[task->sort_table[i].se_fld],
                    task->sort_table[i].se_set,
                    task->set_names[task->sort_table[i].se_set]);
            }
        }
        vtprintf(DB_TEXT("\n\n------------------------------------------------------------------\n"));
    }

    if (task->size_kt)
    {
        vtprintf(DB_TEXT("COMPOUND KEY TABLE:\n\n"));
        if (!symbolic)
        {
            vtprintf(DB_TEXT("       key    rec\n"));
            vtprintf(DB_TEXT("key #  fld #  fld #  ptr  order\n"));
            vtprintf(DB_TEXT("-----  -----  -----  ---  -----\n"));
            for (i = 0; i < task->size_kt; ++i)
            {
                vtprintf(DB_TEXT("%4d   %4d   %4d  %3d    %c\n"), i,
                    task->key_table[i].kt_key,
                    task->key_table[i].kt_field,
                    task->key_table[i].kt_ptr,
                    task->key_table[i].kt_sort);
            }
        }
        else
        {
            vtprintf(DB_TEXT("         [#]compound    [#]component\n"));
            vtprintf(DB_TEXT("key #     key field         field        ptr  order\n"));
            vtprintf(DB_TEXT("-----  ---------------  ---------------  ---  -----\n"));
            for (i = 0; i < task->size_kt; ++i)
            {
                vtprintf(DB_TEXT(" %4d  [%3d]%-10.10s  [%3d]%-10.10s  %3d      %c\n"), i,
                    task->key_table[i].kt_key,
                    task->field_names[task->key_table[i].kt_key],
                    task->key_table[i].kt_field,
                    task->field_names[task->key_table[i].kt_field],
                    task->key_table[i].kt_ptr,
                    task->key_table[i].kt_sort);
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

    return 0;
}

static int usage()
{
    vftprintf(stderr, DB_TEXT("usage: prdbd [-c] [-sg [<mode>,]<password>] dbname\n"));
    vftprintf(stderr, DB_TEXT("where: -c  Disables using record/set/key names\n"));
    vftprintf(stderr, DB_TEXT("       -sg Specifies the SafeGarde encryption information for the database\n"));

    return(1);
}


VXSTARTUP("prdbd", 0)

