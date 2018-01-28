/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/*--------------------------------------------------------------------------
    IDA - Miscellaneous Information Functions
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "internal.h"
#include "ddlnms.h"
#include "ida.h"

/* ========================================================================
    Display database file status
*/
void mffcn()
{
    char                 slots[20];
    int                  cnt;
    register short       i;
    extern char          dbname[];
    PGZERO               pgz;

    tprintf("@m0400@E@SDatabase name:@s %s\n", dbname);
    tprintf("@SAccess type  :@s %s\n",
          task->dbopen == 1 ? "shared" : (task->dbopen == 2 ? "exclusive" : "closed"));

    if (task->trans_id[0])
        tprintf("@m0600@STransaction  :@s %s", task->trans_id);

    tprintf("@m0800@SFILE  SLOTS TYPE STATUS        @s");
    if (task->dbopen == 1)
        tprintf("@SRW EX@s ");
    tprintf("@SNAME@s\n");
    for (cnt = i = 0; (i < task->size_ft) && (task->file_table[i].ft_type != 'o'); ++i)
    {
        int rn, cn;

        rn = cnt + 9;
        cn = 0;
        tprintf("@M@e%3d  ", &rn, &cn, i);
        d_internals(task, TOPIC_PGZERO_TABLE, 0, i, &pgz, sizeof(pgz));
        sprintf(slots, "%6ld", pgz.pz_next - 1);
        tprintf("%s %s %s-%s ", slots,
                  (task->file_table[i].ft_type == DATA) ? "data" :
                  ((task->file_table[i].ft_type == KEY) ? "key " : "stat"),
                  (task->file_table[i].ft_status == OPEN) ? "open  " : "closed",
                  (task->dbopen == 2 || task->app_locks[i] || task->excl_locks[i]) ? "locked" : "free  ");
        if (task->dbopen == 1)
        {
            tprintf("%2d ", task->app_locks[i]);
            tprintf("%2d ", task->excl_locks[i]);
        }
        tprintf("%s", task->file_table[i].ft_name);
        if (cnt++ == 12 && i < task->size_ft - 1)
        {
            usererr("more");
            tprintf("@m0900@E");
            cnt = 0;
        }
    }
}

/* ========================================================================
    Display set/record lock status
*/
void mlfcn()
{
    register int i, cnt;
    int rn, cn;

    if (task->dbopen == 2)
    {
        usererr("Exclusive database access");
    }
    else
    {
        tprintf("@m0400@E@SRECORD NAME             LOCK STATUS@s\n");
        for (cnt = i = 0; i < task->size_rt; ++i)
        {
            rn = cnt + 5;
            cn = 0;
            tprintf("@M@e%s", &rn, &cn, task->record_names[i]);
            cn = 23;
            tprintf("@M@e ", &rn, &cn);
            switch (task->rec_locks[i].fl_type)
            {
                case 'r':   tprintf("read  ");   break;
                case 'w':   tprintf("write ");   break;
                case 'f':   tprintf("free  ");   break;
                case 'x':   tprintf("excl  ");   break;
                default:    break;
            }
            if (task->rec_locks[i].fl_kept)
                tprintf("and keep");
            if (cnt++ == 17 && i < task->size_rt - 1)
            {
                usererr("more");
                tprintf("@m0500@E");
                cnt = 0;
            }
        }
        if (task->size_st)
        {
            usererr("more");
            tprintf("@m0400@E@SSET NAME                LOCK STATUS@s\n");
            for (cnt = i = 0; i < task->size_st; ++i)
            {
                rn = cnt + 5;
                cn = 0;
                tprintf("@M@e%s", &rn, &cn, task->set_names[i]);
                cn = 23;
                tprintf("@M@e ", &rn, &cn);
                switch (task->set_locks[i].fl_type)
                {
                    case 'r':   tprintf("read  ");   break;
                    case 'w':   tprintf("write ");   break;
                    case 'f':   tprintf("free  ");   break;
                    case 'x':   tprintf("excl  ");   break;
                }
                if (task->set_locks[i].fl_kept)
                    tprintf("and keep");
                if (cnt++ == 17 && i < task->size_st - 1)
                {
                    usererr("more");
                    tprintf("@m0500@E");
                    cnt = 0;
                }
            }
        }
        if (tot_keys)
        {
            usererr("more");
            tprintf("@m0400@E@SKEY NAME@s\n");
            tprintf("@m0434@SLOCK STATUS@s\n");
            for (cnt = i = 0; i < tot_keys; ++i)
            {
                rn = cnt + 5;
                cn = 0;
                tprintf("@M@e%s", &rn, &cn, keynames[i]);
                cn = 33;
                tprintf("@M@e ", &rn, &cn);
                switch (task->key_locks[i].fl_type)
                {
                    case 'r':   tprintf("read  ");   break;
                    case 'w':   tprintf("write ");   break;
                    case 'f':   tprintf("free  ");   break;
                    case 'x':   tprintf("excl  ");   break;
                }
                if (task->key_locks[i].fl_kept)
                    tprintf("and keep");
                if (cnt++ == 17 && i < task->size_st - 1)
                {
                    usererr("more");
                    tprintf("@m0500@E");
                    cnt = 0;
                }
            }
        }
    }
}



