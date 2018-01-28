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
    Check to see if current record is a owner of non-empty SET
*/
int INTERNAL_FCN disowner(int set, DB_TASK *task, int dbn)
{
    SET_PTR crsp;                /* current record's set pointer */
    SET_ENTRY *set_ptr;
    int stat;
    int dbopen_sv;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* Make sure we have a current record */
    if (!task->curr_rec)
        return (dberr(S_NOCR));

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* Read current record and check for members */
    if ((dio_read(task->curr_rec, (char **) &task->crloc,
                  NOPGHOLD, task) == S_OKAY) &&
        (r_gset(set, task->crloc, &crsp, task) == S_OKAY) &&
                null_dba(crsp.first))
    {
        /* end-of-set if curr rec not owner */
        stat = S_EOS;
    }
    else
        stat = task->db_status;

    task->dbopen = dbopen_sv;

    if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    return (task->db_status = stat);
}


