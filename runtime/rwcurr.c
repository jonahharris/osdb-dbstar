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

/* ======================================================================
    Read currency table
*/
int INTERNAL_FCN drdcurr(
    DB_ADDR **currbuff,
    int      *currsize,
    DB_TASK  *task,
    int       dbn)
{
    short size_st;

    size_st = task->db_table[task->curr_db].Size_st;

    *currsize = (2 * size_st + 1) * sizeof(DB_ADDR);

    if (task->db_tsrecs)
        *currsize += (2 * size_st + 1) * sizeof(DB_ULONG);

    if (task->db_tssets)
        *currsize += size_st * sizeof(DB_ULONG);

    *currbuff = (DB_ADDR *) psp_cGetMemory(*currsize, 0);
    if (*currbuff == NULL)
        return (dberr(S_NOMEMORY));

    return (drerdcurr(currbuff, task, dbn));
}

/* ======================================================================
    Reread currency table
*/
int INTERNAL_FCN drerdcurr(
    DB_ADDR **currbuff,
    DB_TASK *task,
    int      dbn)
{
    DB_ADDR  *cb_ptr;
    DB_ULONG *ul_ptr;
    short     size_st;
    short     offset;

    size_st = task->db_table[task->curr_db].Size_st;
    offset  = task->db_table[task->curr_db].st_offset;

    *(cb_ptr = *currbuff) = task->curr_rec;
    cb_ptr++;

    memcpy(cb_ptr, &task->curr_own[offset], size_st * sizeof(*cb_ptr));
    cb_ptr += size_st;

    memcpy(cb_ptr, &task->curr_mem[offset], size_st * sizeof(*cb_ptr));
    cb_ptr += size_st;

    ul_ptr = (DB_ULONG *) cb_ptr;
    if (task->db_tsrecs)
    {
        *ul_ptr = task->cr_time;
        ul_ptr++;

        memcpy(ul_ptr, &task->co_time[offset], size_st * sizeof(*ul_ptr));
        ul_ptr += size_st;

        memcpy(ul_ptr, &task->cm_time[offset], size_st * sizeof(*ul_ptr));
        ul_ptr += size_st;
    }
    
    if (task->db_tssets)
        memcpy(cb_ptr, &task->cs_time[offset], size_st * sizeof(*ul_ptr));

    return (task->db_status);
}



/* ======================================================================
    Write currency table
*/
int INTERNAL_FCN dwrcurr(DB_ADDR *currbuff, DB_TASK *task, int dbn)
{
    DB_ADDR   *cb_ptr;
    DB_ULONG  *ul_ptr;
    short      size_st;
    short      offset;

    size_st = task->db_table[task->curr_db].Size_st;
    offset  = task->db_table[task->curr_db].st_offset;

    if ((cb_ptr = currbuff) != NULL)
    {
        task->curr_rec = *cb_ptr;
        cb_ptr++;

        memcpy(&task->curr_own[offset], cb_ptr, size_st * sizeof(*cb_ptr));
        cb_ptr += size_st;

        memcpy(&task->curr_mem[offset], cb_ptr, size_st * sizeof(*cb_ptr));
        cb_ptr += size_st;

        ul_ptr = (DB_ULONG *) cb_ptr;
        if (task->db_tsrecs)
        {
            task->cr_time = *ul_ptr;
            ul_ptr++;

            memcpy(&task->co_time[offset], ul_ptr, size_st * sizeof(*ul_ptr));
            ul_ptr += size_st;

            memcpy(&task->cm_time[offset], ul_ptr, size_st * sizeof(*ul_ptr));
            ul_ptr += size_st;
        }

        if (task->db_tssets)
            memcpy(&task->cs_time[offset], ul_ptr, size_st * sizeof(*ul_ptr));
    }

    psp_freeMemory(currbuff, 0);

    return (task->db_status);
}


