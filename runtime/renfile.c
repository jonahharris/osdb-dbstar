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
    Rename database file
*/
int INTERNAL_FCN drenfile(
    const DB_TCHAR *dbn,  /* database containing file to be renamed */
    FILE_NO         fno,  /* file id number for file to be renamed */
    const DB_TCHAR *fnm,  /* new file name */
    DB_TASK        *task)
{
    REN_ENTRY *rp;

    /* make sure this database is not opened yet */
    if (ddbnum(dbn, task) != S_INVDB)
        return (dberr(S_DBOPEN));

    task->db_status = S_OKAY;     /* set by ddbnum() */
    ll_access(&task->ren_list);
    while ((rp = (REN_ENTRY *) ll_next(&task->ren_list)) != NULL)
    {
        if (vtstrcmp(rp->ren_db_name, dbn) == 0 && rp->file_no == fno)
        {
            psp_freeMemory(rp->file_name, 0);
            rp->file_name = psp_getMemory((vtstrlen(fnm) + 1) * sizeof(DB_TCHAR), 0);
            if (rp->file_name == NULL)
                return (dberr(S_NOMEMORY));

            vtstrcpy(rp->file_name, fnm);
            ll_deaccess(&task->ren_list);
            return (task->db_status);
        }
    }

    rp = (REN_ENTRY *) psp_getMemory(sizeof(REN_ENTRY), 0);
    if (rp == NULL)
        return (dberr(S_NOMEMORY));

    if (ll_append(&task->ren_list, (char *) rp, task) != S_OKAY)
        return (task->db_status);

    rp->ren_db_name = psp_getMemory((vtstrlen(dbn) + 1) * sizeof(DB_TCHAR), 0);
    rp->file_name = psp_getMemory((vtstrlen(fnm) + 1) * sizeof(DB_TCHAR), 0);
    if (!rp->ren_db_name || !rp->file_name)
        return (dberr(S_NOMEMORY));

    vtstrcpy(rp->ren_db_name, dbn);
    vtstrcpy(rp->file_name, fnm);
    rp->file_no = fno;

    ll_deaccess(&task->ren_list);

    return (task->db_status);
}


/* ======================================================================
    Clean up renamed file table
*/
int INTERNAL_FCN drenclean(DB_TASK *task)
{
    REN_ENTRY *rp;

    if (ll_access(&task->ren_list))
    {
        while ((rp = (REN_ENTRY *) ll_next(&task->ren_list)) != NULL)
        {
            psp_freeMemory(rp->ren_db_name, 0);
            psp_freeMemory(rp->file_name, 0);
            psp_freeMemory(rp, 0);
        }
    }

    ll_deaccess(&task->ren_list);
    ll_free(&task->ren_list, task);

    return (task->db_status);
}


