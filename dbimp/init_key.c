/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbimp utility                                     *
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

    init_key.c - add a temporary key file to the dictionary tables

    This function will patch up the dictionary tables to include a new
    file named "dbimp.kfl".  Then the file will be created and
    initialized.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "impdef.h"
#include "impvar.h"

/* ********************** TYPE DEFINITIONS *************************** */

typedef union
{
    struct
    {
        F_ADDR dchain;                   /* delete chain pointer (init -1) */
        F_ADDR next;                     /* next page or record slot */
        DB_ULONG timestamp;              /* file's timestamp value */
        time_t cdate;                    /* creation date,time */
        time_t bdate;                    /* date/time of last backup */
        char vdb_id[LENVDBID];           /* db.* id mark */
    } pg0;
    struct
    {
        time_t chg_date;                 /* date of last page change */
        /* char arrays are used to avoid alignment problems */
        char init_int[SHORT_SIZE];       /* # filled slots on key file;
                                            System record # on data file */
        char init_addr[DB_ADDR_SIZE];    /* NONE node pointer on key file;
                                            System record db_addr on data file */
        char init_crts[LONG_SIZE];       /* if system record is timestamped */
        char init_upts[LONG_SIZE];       /* if system record is timestamped */
    } pg1;
} INIT_PAGE;

/* ******************************************************************* */

/* set up the db.* tables to use an extra key file
*/
int init_key()
{
    int i, ftx, fdx, rtx, status;
    short num_keys, len;
    FILE_ENTRY *ft_ptr;
    FIELD_ENTRY *fd_ptr;
    RECORD_ENTRY *rt_ptr;
    DB_TASK *task = imp_g.dbtask;

    /* add one new entry in file, field and record tables */
    ftx = task->size_ft;
    fdx = task->size_fd;
    rtx = task->size_rt;

    task->size_ft++;
    task->size_fd++;
    task->size_rt++;

    task->old_no_of_dbs++;

    if ((status = alloc_dict(task, NULL)) != S_OKAY)
        return status;

    ft_ptr = &task->file_table[ftx];
    fd_ptr = &task->field_table[fdx];
    rt_ptr = &task->record_table[rtx];

    /* initialize new entry in file table */
    imp_g.slsize = (short) (imp_g.keylen + sizeof(short) + sizeof(F_ADDR) +
            sizeof(short) + sizeof(DB_ADDR));
    vtstrcpy(ft_ptr->ft_name, imp_g.keyfile);
    ft_ptr->ft_pgsize = task->page_size;
    ft_ptr->ft_type = KEY;
    ft_ptr->ft_slots = (short)
            ((task->page_size - PGHDRSIZE - sizeof(short) - sizeof(F_ADDR)) / imp_g.slsize);
    if (ft_ptr->ft_slots < 4)
    {
        vftprintf(stderr, DB_TEXT("dbimp: key length of %d is too large\n"), imp_g.keylen);
        vftprintf(stderr, DB_TEXT("    Decrease the key length, or increase the database\n"));
        vftprintf(stderr, DB_TEXT("    page length, which is currently %d\n"), task->page_size);
        return S_SYSERR;
    }
    ft_ptr->ft_slsize  = imp_g.slsize;
    ft_ptr->ft_status  = CLOSED;
    ft_ptr->ft_flags   = NOT_TTS_FILE | TEMPORARY;
    ft_ptr->ft_pctincrease = 0;
    ft_ptr->ft_initial = 0;
    ft_ptr->ft_next    = 0;

    /* initialize new entry in field table */
    len = (short) (imp_g.keylen + sizeof(short));
    fd_ptr->fd_key = UNIQUE;
    fd_ptr->fd_len = len;
    fd_ptr->fd_dim[0] = (short) (len / 2);
    fd_ptr->fd_dim[1] = 0;
    fd_ptr->fd_dim[2] = 0;
    fd_ptr->fd_keyfile = (short) ftx;
    fd_ptr->fd_type = SHORTINT;
    fd_ptr->fd_flags = 0;
    imp_g.keyfld = fdx;

    /* count total number of key fields */
    for (i = num_keys = 0; i < fdx; ++i)
    {
        if (task->field_table[i].fd_key != NOKEY)
            ++num_keys;
    }
    fd_ptr->fd_keyno = num_keys;

    /*
        The record table must be expanded to avoid an assumption made by
        d_recwrite().  Since compound keys follow the normal fields in a
        record, and are not counted in rt_fdtot, the starting point of the
        next record's fields is used to locate the compound key definitions.
        Since the field we are adding here is not included in a record, it
        is assumed to be a compound key in the last record definition.  This
        null record definition will prevent this from occurring.
    */
    rt_ptr->rt_file = 0;
    rt_ptr->rt_len = 0;
    rt_ptr->rt_data = 0;
    rt_ptr->rt_flags = 0;
    rt_ptr->rt_fields = (short) fdx;
    rt_ptr->rt_fdtot = 1;

    /*
        Re-initialize the log file, key field arrays, and page zero array,
        now that the file table and field table have been extended.
    */
    if (o_setup(task) == S_OKAY)
        if (key_open(task) == S_OKAY)
            if (dio_pzinit(task) == S_OKAY)
                d_initfile((FILE_NO) ftx, task, 0);
                    /* create the key file */

    return task->db_status;
}


