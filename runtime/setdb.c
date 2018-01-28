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
    Set current database
*/
int INTERNAL_FCN dsetdb(int dbn, DB_TASK *task)
{
    if (dbn < 0 || dbn >= task->no_of_dbs)
        return (dberr(S_INVDB));

    task->set_db = dbn;
    if (dbn != task->curr_db)
    {
        task->db_table[task->curr_db].curr_dbt_rec = task->curr_rec;
        task->db_table[task->curr_db].curr_dbt_ts = task->cr_time;

        task->curr_db = dbn;
        task->curr_db_table = &task->db_table[task->curr_db];
        task->curr_rn_table = &task->rn_table[task->curr_db];
        task->curr_rec = task->curr_db_table->curr_dbt_rec;
        task->cr_time  = task->curr_db_table->curr_dbt_ts;
    }

    return (task->db_status);
}


