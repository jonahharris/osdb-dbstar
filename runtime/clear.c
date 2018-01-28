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


/*************************************************************************/

int INTERNAL_FCN dlmclear(
    const DB_TCHAR *username,
    const DB_TCHAR *lockmgr,
    LMC_AVAIL_FCN  *avail,
    DB_TASK        *task)
{
    int        i;
    int        stat = PSP_OKAY;
    DB_TCHAR   userid[10];
    DB_BOOLEAN newsession = FALSE;
    LM_USERID *cu;
    PSP_LMC    lmc = task->lmc;

    /* Must specify user to be cleared */

    if (!username[0])
        return (dberr(S_USERID));

    if (task->dbopen)
    {
        /* A lockmgr session has already been started
           - only use it if the name and type are ok
        */
        if ((lockmgr && vtstrcmp(lockmgr, task->lockmgrn)) ||
                (avail && avail != psp_lmcAvail(lmc)))
            newsession = TRUE;

        /* Do not allow users to clear themselves
           - this will crash the lock manager
        */
        if (!vtstrcmp(username, task->dbuserid))
            return (dberr(S_DUPUSERID));
    }
    else
        newsession = TRUE;

    if (newsession)
    {
        /* Use lock manager type, if specified */
        if (!avail)
            avail = psp_lmcAvail(task->lmc);

        if ((stat = psp_lmcSetup(&lmc, avail)) != PSP_OKAY)
            return (dberr(S_LMCERROR));

        /* Lock manager name defaults to "lockmgr", but
           there is no default lock manager type
        */
        if (!lockmgr)
            lockmgr = task->lockmgrn;

        if (task->dbuserid[0])
            stat = psp_lmcConnect(lockmgr, task->dbuserid, task->dbtmp, lmc);
        else
        {
            vtstrcpy(userid, DB_TEXT("LMCLEAR0"));

            for (i = 0; i < 10; i++)
            {
                userid[7] = (DB_TCHAR)(i + DB_TEXT('0'));
                stat = psp_lmcConnect(lockmgr, userid, task->dbtmp, lmc);
                if (stat != PSP_DUPUSERID)
                    break;
            }
        }

        if (stat != PSP_OKAY)
        {
            psp_lmcCleanup(lmc);
            if (stat == PSP_DUPUSERID)
                return (dberr(S_DUPUSERID));

            return (dberr(S_NOLOCKMGR));
        }
    }

    if ((cu = (LM_USERID *) psp_lmcAlloc(sizeof(LM_USERID))) == NULL)
        return (dberr(S_NOMEMORY));

    vtstrcpy(cu->dbuserid, username);
    stat = psp_lmcTrans(L_CLEARUSER, cu, sizeof(LM_USERID), NULL, NULL, NULL,
            lmc);

    if (stat != PSP_OKAY)
        stat = S_LMCERROR;

    psp_lmcFree(cu);

    if (newsession)
    {
        psp_lmcDisconnect(lmc);
        psp_lmcCleanup(lmc);
    }

    return (dberr(stat));
}


