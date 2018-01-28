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
    Disconnect from all sets (owner and member) and delete record
*/
int INTERNAL_FCN ddisdel(DB_TASK *task, int dbn)
{
    int rectype, nset, cset;
    register int set, mem;
    DB_ADDR dba, odba, mdba;
    DB_ADDR *co_ptr, *cm_ptr;
    register SET_ENTRY *set_ptr;
    register MEMBER_ENTRY *mem_ptr;
    int memtot;

    if (dcrtype(&rectype, task, dbn) != S_OKAY)
        return (task->db_status);

    rectype += NUM2INT(-RECMARK, rt_offset);

    dcrget(&dba, task, dbn);
    for (set = 0, set_ptr = &task->set_table[ORIGIN(st_offset)];
         set < TABLE_SIZE(Size_st);
         ++set, ++set_ptr)
    {
        nset = set + SETMARK;
        cset = set + ORIGIN(st_offset);
        co_ptr = &task->curr_own[cset];
        cm_ptr = &task->curr_mem[cset];
        odba = *co_ptr;
        mdba = *cm_ptr;
        if (set_ptr->st_own_rt == rectype)
        {
            /* disconnect all member records from set s */
            dsetor(nset, task, dbn);
            while (dfindfm(nset, task, dbn) == S_OKAY)
            {
                if (ddiscon(nset, task, dbn) < S_OKAY)
                    return (task->db_status);
            }

            /* either findfm or discon may return S_EOS */
            if (task->db_status == S_EOS)
                task->db_status = S_OKAY;

            dsetro(nset, task, dbn);
        }

        for (mem = set_ptr->st_members, memtot = mem + set_ptr->st_memtot,
             mem_ptr = &task->member_table[mem];
             mem < memtot;
             ++mem, ++mem_ptr)
        {
            if (mem_ptr->mt_record == rectype)
            {
                /* disconnect current record from set */
                if (dismember(nset, task, dbn) == S_OKAY)
                {
                    dcsmset(nset, &dba, task, dbn);
                    if (ddiscon(nset, task, dbn) < S_OKAY)
                        return (task->db_status);
                }

                /* either ismember or discon may return S_EOS */
                if (task->db_status == S_EOS)
                    task->db_status = S_OKAY;
            }
        }

        task->curr_rec = dba;
        if (dba == odba)
        {
            *co_ptr = NULL_DBA;
            *cm_ptr = NULL_DBA;
        }
        else
        {
            *co_ptr = odba;
            if (dba == mdba)
                *cm_ptr = NULL_DBA;
            else
                *cm_ptr = mdba;
        }
    }

    return (ddelete(task, dbn));
}


