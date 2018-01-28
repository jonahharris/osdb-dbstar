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
    Initialize database environment variables
*/
int INTERNAL_FCN initenv(DB_TASK *task)
{
    register DB_TCHAR *ptr;

    if (!task->dbtmp)
    {
        /* get database dictionary directory */
        if ((ptr = psp_getenv(DB_TEXT("DBTMP"))) != NULL)
        {
            if (ddbtmp(ptr, task) != S_OKAY)
                return task->db_status;
        }
    }

    if (!task->iniFile[0])
    {
        if ((ptr = psp_getenv(DB_TEXT("DBINI"))) != NULL)
        {
            if (ddbini(ptr, task) != S_OKAY)
                return task->db_status;
        }
    }

    if (!task->dblog[0])
    {
        if ((ptr = psp_getenv(DB_TEXT("DBLOG"))) != NULL)
        {
            if (ddblog(ptr, task) != S_OKAY)
                return task->db_status;
        }
    }

    if (!task->dbtaf[0])
    {
        if ((ptr = psp_getenv(DB_TEXT("DBTAF"))) != NULL)
        {
            if (ddbtaf(ptr, task) != S_OKAY)
                return task->db_status;
        }
    }

    if (task->db_lockmgr && !task->lockmgrn)
    {
        /* get lockmgr name */
        if ((ptr = psp_getenv(DB_TEXT("LOCKMGR"))) != NULL)
        {
            if (dlockmgr(ptr, task) != S_OKAY)
                return task->db_status;
        }
    }

    if (!task->dbdpath[0])
    {
        /* get database dictionary directory */
        if ((ptr = psp_getenv(DB_TEXT("DBDPATH"))) != NULL)
        {
            if (ddbdpath(ptr, task) != S_OKAY)
                return task->db_status;
        }
    }

    if (!task->dbfpath[0])
    {
        /* get database files directory */
        if ((ptr = psp_getenv(DB_TEXT("DBFPATH"))) != NULL)
        {
            if (ddbfpath(ptr, task) != S_OKAY)
                return task->db_status;
        }
    }

    if (!task->ctbpath[0])
    {
        /* get country table directory */
        if ((ptr = psp_getenv(DB_TEXT("CTBPATH"))) != NULL)
        {
            if (dctbpath(ptr, task) != S_OKAY)
                return task->db_status;
        }
    }

    if (task->db_lockmgr && !task->dbuserid[0])
    {
        if ((ptr = psp_getenv(DB_TEXT("DBUSERID"))) != NULL)
        {
            if (ddbuserid(ptr, task) != S_OKAY)
                return task->db_status;
        }
        else
            return (dberr(S_USERID));
    }

    return task->db_status;
}


