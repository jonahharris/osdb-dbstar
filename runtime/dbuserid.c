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


extern PSP_SEM task_sem;

/* ======================================================================
    Set Database User Identifier
*/
int INTERNAL_FCN ddbuserid(const DB_TCHAR *id, DB_TASK *task)
{
    DB_TCHAR        c;
    const DB_TCHAR *chk_id;

    if (task->dbopen)
        return (dberr(S_DBOPEN));

    if (id == NULL)
        return (dberr(S_INVPTR));

    if (vtstrlen(id) > USERIDLEN-1)
        return (dberr(S_NAMELEN));

    for (chk_id = id; *chk_id; chk_id++)
    {
        c = *chk_id;
        if (!(vistalnum(c) || (c == DB_TEXT('_'))))
            return (dberr(S_BADUSERID));
    }

    vtstrcpy(task->dbuserid, id);

    return (task->db_status);
}

/* ======================================================================
    Check Database User Identifier Value
    Possible return values:  (dberr is not called)
        S_OKAY:        valid user ID and ID is not in use.
        S_NAMELEN:     User ID string too long.
        S_BADUSERID:   invalid user ID.
        S_DUPLICATE:   user ID already in use.
        S_NOLOCKCOMM:  don't know which lockmgr type to ask.  (dberr() called)
*/
int INTERNAL_FCN dcheckid(DB_TCHAR *id, DB_TASK *task)
{
    const DB_TCHAR *chk_id;
    DB_TCHAR        c;
    int             i;
    int             stat = S_OKAY;
    extern int      Db_task_count; /* numberof tasks */
    extern TASK   **Db_task_table; /* database task table */

    if (task->dbopen)
        return (dberr(S_DBOPEN));

    if (id == NULL)
        return (dberr(S_INVPTR));

    for (chk_id = id; *chk_id; chk_id++)
    {
        c = *chk_id;
        if (! (vistalnum(c) || (c == DB_TEXT('_'))))
            return (task->db_status = S_BADUSERID);
    }

    psp_syncEnterExcl(task_sem);

    for (i = 0; i < Db_task_count; i++)
    {
        if (Db_task_table[i] == task)
            continue;      /* don't count me */

        if (Db_task_table[i]->dbopen &&
            vtstrcmp(id, Db_task_table[i]->dbuserid) == 0)
            return (task->db_status = S_DUPLICATE);
    }

    psp_syncExitExcl(task_sem);


    if (!task->dbuserid[0])
        task->dbuserid[0] = (DB_TCHAR) '\xFF';

    if (!task->dbopen)
    {
        task->db_lockmgr = 1;
        if (initFromIniFile(task) != S_OKAY)
            goto exit;
    }

    if (task->db_lockmgr && task->lmc && (stat = psp_lmcCheckid(task->lockmgrn,
            id, task->dbtmp, task->lmc)) != S_OKAY)
    {
        if (stat == S_DUPUSERID)
            stat = S_DUPLICATE;
        else
            stat = S_OKAY;  /* If we can't connect to the lock manger, then
                               we can't assume the id is bad */
    }

    task->db_status = stat;

exit:
    if (task->dbuserid[0] == (DB_TCHAR) '\xFF')
        task->dbuserid[0] = 0;

    return task->db_status;
}


