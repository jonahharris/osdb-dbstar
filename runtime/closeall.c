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
#include "rntmint.h"

/* ======================================================================
    Close all open Database files
*/
int INTERNAL_FCN dcloseall(DB_TASK *task)
{
    FILE_NO file;

    if (! task->dbopen)
        return (task->db_status);

    for (file = 0; file < task->size_ft; file++)
        dio_close(file, task);

    o_close(task);

    if (!(psp_lmcFlags(task->lmc) & PSP_FLAG_NO_TAF_ACCESS))
        taf_close(task);

    return (task->db_status);
}


