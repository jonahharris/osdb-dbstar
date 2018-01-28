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

/* 16-bit windows prefers these not on the stack due to medium model DS != SS */
static DB_TCHAR dbSection[]      = DB_TEXT("db.*");
static DB_TCHAR lockmgrSection[] = DB_TEXT("lockmgr");
#ifdef DB_DEBUG
static DB_TCHAR debugSection[]   = DB_TEXT("debug");
#endif
static DB_TCHAR defINIPath[3]    = { DB_TEXT('.'), DIRCHAR, DB_TEXT('\0') };

int INTERNAL_FCN initFromIniFile(DB_TASK *task)
{
    /* THIS FUNCTION MUST BE CALLED _BEFORE_ dio_init() GETS CALLED!!!!! */
    int            dbUserPages;
    int            ixUserPages;
    int            pageFlag;
    short          number;
    DB_TCHAR      *ptr;
    DB_TCHAR       string[2 * FILENMLEN];
    PSP_INI        ini;
    LMC_AVAIL_FCN *avail;

    /* get the environment variables first */
    if (!(task->dboptions & IGNOREENV))
    {
        if (initenv(task) != S_OKAY)
            return (task->db_status);
    }

    if ((DB_BYTE)task->iniFile[0] == (DB_BYTE)0xFF)
        return (task->db_status);

    if (!task->iniFile[0])
        d_dbini(defINIPath, task);

    if ((ini = psp_iniOpen(task->iniFile)) == NULL && psp_errno() != ENOENT)
        return dberr(S_NOFILE);

    /* If the values are the the default_vals we can presume that the program
       did not call d_set*() to change them.  Otherwise, leave the changed
       values as the program left them.
    */
    
    if (!task->dbdpath[0])
    {
        psp_iniString(ini, dbSection, DB_TEXT("dbdpath"), DB_TEXT(""), string,
                sizeof(string));
        if (string[0])
            ddbdpath(string, task);
    }

    if (!task->dbfpath[0])
    {
        /* get database files directory */
        psp_iniString(ini, dbSection, DB_TEXT("dbfpath"), DB_TEXT(""), string,
                sizeof(string));
        if (string[0])
            ddbfpath(string, task);
    }

    if (!(task->dboptflag & DCHAINUSE))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("dchainuse"), -1);
        if (number > 0)
            don_opt(DCHAINUSE, task);
        else if (number == 0)
            doff_opt(DCHAINUSE, task);
    }

    if (!task->ctbpath[0])
    {
        /* get country table directory */
        psp_iniString(ini, dbSection, DB_TEXT("ctbpath"), DB_TEXT(""), string,
                        sizeof(string));
        if (string[0])
            dctbpath(string, task);
    }

    if (!(task->dboptflag & IGNORECASE))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("ignorecase"), -1);
        if (number > 0)
            don_opt(IGNORECASE, task);
        else if (number == 0)
            doff_opt(IGNORECASE, task);
    }

    if (!task->dbtmp)
    {
        psp_iniString(ini, dbSection, DB_TEXT("dbtmp"), DB_TEXT(""), string,
                sizeof(string));
        if (string[0])
            ddbtmp(string, task);
        else if ((ptr = psp_pathDefTmp()) != NULL)
            ddbtmp(ptr, task);
    }

    if (!task->dbtaf[0])
    {
        psp_iniString(ini, dbSection, DB_TEXT("dbtaf"), DB_TEXT("db.star.taf"),
                string, sizeof(string));
        ddbtaf(string, task);
    }

    if (!task->dblog[0])
    {
        psp_iniString(ini, dbSection, DB_TEXT("dblog"), DB_TEXT(""), string,
                        sizeof(string));
        if (string[0])
        {
            /* allow directories only */
            size_t len = vtstrlen(string);
           
            if (!psp_pathIsDir(string))
            {
                if (len + 1 >= DB_PATHLEN)
                {
                    string[0] = DB_TEXT('\0');
                    dberr(S_NAMELEN);
                }
                else
                {
                    string[len++] = DIRCHAR;
                    string[len] = DB_TEXT('\0');
                }
            }

            if (string[0])
                ddblog(string, task);
        }
    }

    /* Only read lock manager related info if we're using one */
    if (task->db_lockmgr)
    {
        if (!task->lockmgrn)
        {
            psp_iniString(ini, lockmgrSection, DB_TEXT("name"),
            psp_defLockmgr(), string, sizeof(string));
            d_lockmgr(string, task);
        }

        if (!task->lmc)     /* not picked yet */
        {
            psp_iniString(ini, lockmgrSection, DB_TEXT("Type"), DB_TEXT("NONE"),
                    string, sizeof(string));
            if ((avail = psp_lmcFind(string)) != NULL)
                dlockcomm(avail, task);
            else
                dberr(S_NOLOCKCOMM);
        }

        if (task->db_timeout == DB_TIMEOUT)
        {
            task->db_timeout = psp_iniShort(ini, lockmgrSection,
                    DB_TEXT("timeout"), DB_TIMEOUT);
            /* will be set following the dopen() */
        }
    }

    if (!(task->dboptflag & ARCLOGGING))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("arclogging"), -1);
        if (number > 0)
            don_opt(ARCLOGGING, task);
        else
            doff_opt(ARCLOGGING, task);
    }

    if (!(task->dboptflag & PORTABLE))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("portable"), -1);
        if (number > 0)
            don_opt(PORTABLE, task);
        else if (number == 0)
            doff_opt(PORTABLE, task);
    }

    if (!(task->dboptflag & SYNCFILES))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("syncfiles"), -1);
        if (number > 0)
            don_opt(SYNCFILES, task);
        else if (number == 0)
            doff_opt(SYNCFILES, task);
    }

    if (!(task->dboptflag & READONLY))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("readonly"), -1);
        if (number > 0)
            don_opt(READONLY, task);
        else if (number == 0)
            doff_opt(READONLY, task);
    }

    if (!(task->dboptflag & DELETELOG))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("deletelog"), -1);
        if (number > 0)
            don_opt(DELETELOG, task);
        else if (number == 0)
            doff_opt(DELETELOG, task);
    }

    if (!(task->dboptflag & TRLOGGING))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("trlogging"), -1);
        if (number > 0)
            don_opt(TRLOGGING, task);
        else if (number == 0)
            doff_opt(TRLOGGING, task);
    }

    if (!(task->dboptflag & TXTEST))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("txtest"), -1);
        if (number > 0)
            don_opt(TXTEST, task);
        else if (number == 0)
            doff_opt(TXTEST, task);
    }

    if (!(task->dboptflag & TRUNCATELOG))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("truncatelog"), -1);
        if (number > 0)
            don_opt(TRUNCATELOG, task);
        else if (number == 0)
            doff_opt(TRUNCATELOG, task);
    }

#ifdef MULTI_TAFFILE
    if (!(task->dboptflag & MULTITAF))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("multitaf"), -1);
        if (number > 0)
            don_opt(MULTITAF, task);
        else if (number == 0)
            doff_opt(MULTITAF, task);
    }
#endif /* MULTI_TAFFILE */

    pageFlag = 0;              /* need to call d_setpages() ? */
    ixUserPages = task->cache->ix_pgtab.pgtab_sz;
    dbUserPages = task->cache->db_pgtab.pgtab_sz;
    if (ixUserPages == DEFIXPAGES && dbUserPages == DEFDBPAGES)
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("maxovpages"), -1);
        if (number > 0)
        {
            ixUserPages = number;
            pageFlag = 1;
        }

        number = psp_iniShort(ini, dbSection, DB_TEXT("maxcachepages"), -1);
        if (number > 0)
        {
            dbUserPages = number;
            pageFlag = 1;
        }

        if (pageFlag)
            dsetpages(dbUserPages, ixUserPages, task);
    }

    if (!(task->dboptflag & PREALLOC_CACHE))
    {
        number = psp_iniShort(ini, dbSection, DB_TEXT("prealloc_cache"), -1);
        if (number > 0)
            don_opt(PREALLOC_CACHE, task);
        else if (number == 0)
            doff_opt(PREALLOC_CACHE, task);
    }

#ifdef DB_DEBUG

    if (!(task->db_debug & PZVERIFY))
    {
        number = psp_iniShort(ini, debugSection, DB_TEXT("pzverify"), -1);
        if (number > 0)
            don_opt(PZVERIFY, task);
        else if (number == 0)
            doff_opt(PZVERIFY, task);
    }

    if (!(task->db_debug & LOCK_CHECK))
    {
        number = psp_iniShort(ini, debugSection, DB_TEXT("lock_check"), -1);
        if (number > 0)
            don_opt(LOCK_CHECK, task);
        else if (number == 0)
            doff_opt(LOCK_CHECK, task);
    }

    if (task->db_debug & LOCK_CHECK)
    {
        /* LOCK_CHECK and PAGE_CHECK are mutually exclusive */
        if (task->dboptions & PAGE_CHECK)
            doff_opt(PAGE_CHECK, task);
    }
    else if (!(task->db_debug & PAGE_CHECK))
    {
        number = psp_iniShort(ini, debugSection, DB_TEXT("page_check"), -1);
        if (number > 0)
            don_opt(PAGE_CHECK, task);
        else if (number == 0)
            doff_opt(PAGE_CHECK, task);
    }

    if (!(task->db_debug & CACHE_CHECK))
    {
        number = psp_iniShort(ini, debugSection, DB_TEXT("cache_check"), -1);
        if (number > 0)
            don_opt(CACHE_CHECK, task);
        else if (number == 0)
            doff_opt(CACHE_CHECK, task);
    }

#endif /* DB_DEBUG */

#ifdef DB_TRACE

    if (task->db_trace == 0L || task->db_trace == 0x0FFFFFFF)
    {
        DB_ULONG trace = 0;

        if (psp_iniShort(ini, debugSection, DB_TEXT("trace_dberr"), -1) > 0)
            trace |= TRACE_DBERR;
     
        if (psp_iniShort(ini, debugSection, DB_TEXT("trace_api"), -1) > 0)
            trace |= TRACE_API;
     
        if (psp_iniShort(ini, debugSection, DB_TEXT("trace_locks"), -1) > 0)
            trace |= TRACE_LOCKS;

        if (trace)
        {
            task->db_trace = 0;
            don_opt(trace, task);
        }
    }
#endif /* DB_TRACE */

    psp_iniClose(ini);
    return task->db_status;
}


