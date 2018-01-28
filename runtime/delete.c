/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database kernel                                             *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

#include "db.star.h"

static DB_ADDR zap_dba = NULL_DBA;


/* ======================================================================
    Delete the current record, with error checking
*/
int INTERNAL_FCN ddelete(DB_TASK *task, int dbn)
{
    register int st_lc, mt_lc;          /* loop control */
    short i, rt;
    char *ptr;
    SET_ENTRY *set_ptr;
    MEMBER_ENTRY *mem_ptr;
    DB_ADDR *co_ptr, *cm_ptr;
    DB_ULONG *cots_ptr, *cmts_ptr, *sts_ptr;

    if (!task->curr_rec)
        return (dberr(S_NOCR));

    /* get the record type of the current record */
    if (dio_read(task->curr_rec, (char **) &ptr, NOPGHOLD, task) != S_OKAY)
        return (task->db_status);

    memcpy(&rt, ptr, sizeof(short));
    if (rt < 0)
    {
        if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        return (task->db_status = S_DELETED);
    }

    rt &= ~RLBMASK;                  /* mask off rlb */
    rt += task->curr_db_table->rt_offset;

    /* make sure this is not the system record */
    if (task->record_table[rt].rt_fdtot == -1)
    {
        if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        return (dberr(S_DELSYS));
    }

    /* scan the set list for sets that this record owns to confirm
       it is empty
    */
    for (st_lc = task->size_st, set_ptr = task->set_table; --st_lc >= 0; ++set_ptr)
    {
        if ((set_ptr->st_own_rt == rt) &&
            (memcmp(ptr + set_ptr->st_own_ptr + SP_FIRST, &zap_dba, DB_ADDR_SIZE) != 0))
        {
            if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
                return (task->db_status);

            return (dberr(S_HASMEM));
        }
    }

    /* scan the member list for sets which own this record */
    for (mt_lc = task->size_mt, mem_ptr = task->member_table; --mt_lc >= 0; ++mem_ptr)
    {
        if ((mem_ptr->mt_record == rt) &&
            (memcmp(ptr + mem_ptr->mt_mem_ptr + MP_OWNER, &zap_dba, DB_ADDR_SIZE) != 0))
        {
            if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
                return (task->db_status);

            return (dberr(S_ISMEM));
        }
    }

    if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    /* delete record */
    if (r_delrec(rt, task->curr_rec, task) == S_OKAY)
    {
        /* nullify any currency containing deleted record */
        for (i = 0, co_ptr = &task->curr_own[ORIGIN(st_offset)],
             cm_ptr = &task->curr_mem[ORIGIN(st_offset)],
             cots_ptr = &task->co_time[ORIGIN(st_offset)],
             cmts_ptr = &task->cm_time[ORIGIN(st_offset)],
             sts_ptr = &task->cs_time[ORIGIN(st_offset)];
             i < TABLE_SIZE(Size_st);
             ++i, ++co_ptr, ++cm_ptr, ++cots_ptr, ++cmts_ptr, ++sts_ptr)
        {
            if (ADDRcmp(&task->curr_rec, co_ptr) == 0)
            {
                *co_ptr = NULL_DBA;
                if (task->db_tsrecs)
                    *cots_ptr = 0L;

                if (task->db_tssets)
                    *sts_ptr = 0L;
            }

            if (ADDRcmp(&task->curr_rec, cm_ptr) == 0)
            {
                *cm_ptr = NULL_DBA;
                if (task->db_tsrecs)
                    *cmts_ptr = 0L;
            }
        }

        task->curr_rec = NULL_DBA;
        task->cr_time = 0L;
    }

    return (task->db_status);
}


