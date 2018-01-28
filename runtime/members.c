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
    Get number of members of the current set owner
*/
int INTERNAL_FCN dmembers(
    int set,                        /* Set table entry */
    long *tot,                      /* total members of set */
    DB_TASK *task,
    int dbn)                        /* database number */
{
    SET_PTR setp;
    char *recp;
    int stat;
    SET_ENTRY *set_ptr;
    int dbopen_sv;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* Make sure we have a current owner */
    if (!task->curr_own[set])
        return (dberr(S_NOCO));

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* Read owner record */
    dio_read(task->curr_own[set], (char **) &recp, NOPGHOLD, task);

    task->dbopen = dbopen_sv;
    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* Get set pointer from owner record */
    if ((stat = r_gset(set, recp, &setp, task)) != S_OKAY)
    {
        if (dio_release(task->curr_own[set], NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        return (task->db_status = stat);
    }

    *tot = setp.total;

    if (dio_release(task->curr_own[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    return (task->db_status);
}


