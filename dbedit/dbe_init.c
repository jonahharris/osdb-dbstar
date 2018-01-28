/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbedit utility                                    *
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

    dbe_init.c - DBEDIT, program initialization

    The function dbe_init reads the file table, record table etc form
    the DBD file, then reads the object names, then sets the initial
    current record.
    The function dbe_term writes the current record, if changed, and
    closes the current file. Memory allocated in DBEDIT is not
    explicitly released.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_err.h"
#include "dbe_ext.h"
#include "dbe_io.h"

extern void EXTERNAL_FCN dbe_dberr(int, DB_TCHAR *);


/* Initialize - read database dictionary
*/
int dbe_init(int argc, DB_TCHAR **argv, DB_TASK *task)
{
    register int i;
    DB_TCHAR    *database;
    short        slsize;
    int          error, stat;
    FILE_NO      fno = 0;
    DB_ULONG     nextslot;
    SG          *sg = NULL;
#if defined(SAFEGARDE)
    DB_TCHAR    *cp;
    DB_TCHAR    *password;
    int          mode = NO_ENC;
#endif

    error = changed = unicode = 0;
    decimal = 1;
    titles = fields = 1;

    for (i = 1; i < argc && argv[i][0] == DB_TEXT('-'); i++)
    {
        switch (vtotlower(argv[i][1]))
        {
            case DB_TEXT('?'):
            case DB_TEXT('h'):
                return USAGE;

#if defined(UNICODE)
            case DB_TEXT('u'):
                unicode = 1;
                break;
#endif

            case DB_TEXT('s'):
                if (argv[i][2] != DB_TEXT('g') || i == argc - 1)
                    return USAGE;

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
                        return USAGE;

                    password = cp;
                }
                else
                {
                    mode = MED_ENC;
                    password = argv[i];
                }

                break;
#else
                dbe_out(DB_TEXT("SafeGarde is not available in this version\n"),
                        STDERR);
                return USAGE;
#endif
            default:
                return USAGE;
        }
    }

    if (i == argc)
        return USAGE;

    database = argv[i];

#if defined(SAFEGARDE)
    if (mode != NO_ENC && (sg = sg_create(mode, password)) == NULL)
        dbe_out(DB_TEXT("Unable to create SafeGarde context\n"), STDERR);
#endif

    if ((stat = d_set_dberr(dbe_dberr, task)) == S_OKAY)
    {
        if ((stat = d_on_opt(READNAMES, task)) == S_OKAY)
        {
            /* open database in one user mode */
            if ((stat = d_open_sg(database, DB_TEXT("o"), sg, task)) == S_OKAY)
            {
                /* no character translation, even if country table present */
                if (task->ctbl_activ)
                    ctbl_free(task);
            }
        }
    }

    /* Find maximum slot size in database */
    slot.size = 0;
    for (i = 0; i < task->size_ft; ++i)
    {
        if ((slsize = task->file_table[i].ft_slsize) > slot.size)
            slot.size = slsize;
    }

    /* Allocate memory for holding database slot to be edited */
    if ((slot.buffer = malloc(slot.size)) == NULL)
    {
        d_close(task);
        d_closetask(task);
        return (ERR_MEM);
    }

    /* Initialize current record */
    task->curr_rec = NULL_DBA;
    for (i = 0; i < task->size_rt; ++i)
    {
        if (task->record_table[i].rt_fdtot == -1)
        {
            /* found system record */
            fno = (FILE_NO) (FILEMASK & task->record_table[i].rt_file);
            d_encode_dba(fno, 1L, &task->curr_rec);
            break;
        }
    }
    if (task->curr_rec == NULL_DBA)
    {                                   /* No system record - goto [0:1] */
        fno = 0;
        task->curr_rec |= (F_ADDR) 1;
    }
    if (dbe_open(fno, task) || read_nextslot(&nextslot, task)
     || (nextslot < 2L) || dbe_read(task->curr_rec, task))
    {
        dbe_close(task);
        task->curr_rec = NULL_DBA;
    }
    if (error)
    {
        d_close(task);
        d_closetask(task);
    }

    return (error);
}


/* Terminate - write current record if it's been changed
*/
int dbe_term(DB_TCHAR *errstr, DB_TASK *task)
{
    short    fno;
    int      error;
    DB_ULONG slot;

    error = 0;
    if (changed)
    {
        if ((error = dbe_write(task)) == ERR_WRIT)
        {
            d_decode_dba(task->curr_rec, &fno, &slot);
            vtstrcpy(errstr, task->file_table[fno].ft_name);
        }
        changed = 0;
    }
    dbe_close(task);
    in_close();
    d_close(task);
    d_closetask(task);
    return (error);
}


