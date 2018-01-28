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
    Check to see if current record is a connected member of SET
*/
int INTERNAL_FCN dismember(int set, DB_TASK *task, int dbn)
{
    MEM_PTR crmp;                /* current record's member pointer */
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

    /* Read current record and check connection */
    if ((dio_read(task->curr_rec, (char **) &task->crloc,
                  NOPGHOLD, task) == S_OKAY) &&
        (r_gmem(set, task->crloc, &crmp, task) == S_OKAY) &&
                null_dba(crmp.owner))
    {
        /* end-of-set if curr rec not owned */
        stat = S_EOS;
    }
    else
        stat = task->db_status;

    task->dbopen = dbopen_sv;

    if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    return (task->db_status = stat);
}


