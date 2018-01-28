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
#include "options.h"

static DB_TCHAR DbstarIniFlnm[] = DB_TEXT("dbstar.ini");

/* ======================================================================
    Set Database Dictionary Path
*/
int INTERNAL_FCN ddbdpath(const DB_TCHAR *path, DB_TASK *task)
{
    int       i;
    DB_TCHAR *ptr;

    if (task->dbopen)
        dberr(S_DBOPEN);
    else if (path == NULL)
        dberr(S_INVPTR);
    else if (vtstrlen(path) * sizeof(DB_TCHAR) >= sizeof(task->dbdpath))
        dberr(S_NAMELEN);
    else
    {
        i = 0;
        do
        {
            ptr = get_element(path, i++);
            if (ptr && vtstrlen(ptr) >= DB_PATHLEN - 1)
                return (dberr(S_NAMELEN));
        } while (ptr && vtstrchr(path, DB_TEXT(';')));

        vtstrcpy(task->dbdpath, path);
    }

    return (task->db_status);
}


/* ======================================================================
    Set Database Files Path
*/
int INTERNAL_FCN ddbfpath(const DB_TCHAR *path, DB_TASK *task)
{
    int i;
    DB_TCHAR *ptr;

    if (task->dbopen)
        dberr(S_DBOPEN);
    else if (path == NULL)
        dberr(S_INVPTR);
    else if (vtstrlen(path) * sizeof(DB_TCHAR) >= sizeof(task->dbfpath))
        dberr(S_NAMELEN);
    else
    {
        i = 0;
        do
        {
            ptr = get_element(path, i++);
            if (ptr && vtstrlen(ptr) >= DB_PATHLEN - 1)
                return (dberr(S_NAMELEN));

        } while (ptr && vtstrchr(path, DB_TEXT(';')));

        vtstrcpy(task->dbfpath, path);
    }

    return (task->db_status);
}


int INTERNAL_FCN ddbini(const DB_TCHAR *path, DB_TASK *task)
{
    size_t    len;
    int       need_dir;
    
    if (path == NULL)
    {
        task->iniFile[0] = (DB_BYTE)0xFF;
        return (task->db_status);
    }

    if ((len = vtstrlen(path)) == 0)
    {
        vtstrcpy(task->iniFile, DB_TEXT(""));
        return (task->db_status);
    }

    if ((need_dir = !psp_pathIsDir(path)) != 0)
        ++len;

    if (len > DB_PATHLEN-1)
        return (dberr(S_NAMELEN));

    vtstrcpy(task->iniFile, path);

    /* the string is dir only, add DIRCHAR if necessary */
    if (need_dir)
        task->iniFile[len - 1] = DIRCHAR;
    
    /* since the string passed in is the directory only, add the "dbstar.ini" */
    vtstrcpy(&task->iniFile[len], DbstarIniFlnm);

    return (task->db_status);
}


/* ======================================================================
    Set database log file name/path
*/
int INTERNAL_FCN ddblog(const DB_TCHAR *log, DB_TASK *task)
{
    size_t    len;
    DB_TCHAR *ptr;

    if (log == NULL)
        return (dberr(S_INVPTR));

    if (task->dbopen)
        return (dberr(S_DBOPEN));

    /* if necessary, o_setup() will add the userid to the log directory */
   
    if ((len = vtstrlen(log)) >= FILENMLEN)
        return (dberr(S_NAMELEN));

    if (psp_pathIsDir(log))
    {
        if (len >= DB_PATHLEN - 1)
            return (dberr(S_NAMELEN));
    }
    else
    {
        /* have a [directory and] path */
        ptr = vtstrrchr(log, DIRCHAR);
        if (ptr && (len - vtstrlen(ptr)) >= DB_PATHLEN -  1)
            return (dberr(S_NAMELEN));
    }

    vtstrcpy(task->dblog, log);

    return (task->db_status);
}


/* ======================================================================
    Set database transaction activity file name/path
*/
int INTERNAL_FCN ddbtaf(const DB_TCHAR *taf, DB_TASK *task)
{
    size_t len;
    
    if (task->dbopen)
        return (dberr(S_DBOPEN));

    if (!taf)
        return (dberr(S_INVPTR));

    if ((len = vtstrlen(taf)) > (FILENMLEN - 1))
        return (dberr(S_NAMELEN));

    if (len && psp_pathIsDir(taf))
    {
        /* have directory only, add file name */
        if (len >= (DB_PATHLEN - 1))
            return (dberr(S_NAMELEN));

        vtstrcpy(task->dbtaf, taf);
        vtstrcat(task->dbtaf, DB_TEXT("db.star.taf"));
    }
    else
    {
        /* have [directory and] file name */
        DB_TCHAR *ptr = vtstrrchr(taf, DIRCHAR);

        if (ptr && (len - vtstrlen(ptr)) >= (DB_PATHLEN - 1))
            return (dberr(S_NAMELEN));

        vtstrcpy(task->dbtaf, taf);
    }

    return (task->db_status);
}

/* ======================================================================
    Set database temp file path
*/
int INTERNAL_FCN ddbtmp(const DB_TCHAR *tmp, DB_TASK *task)
{
    size_t len;
    
    if (tmp == NULL)
        return (dberr(S_INVPTR));

    if (task->dbopen)
        return (dberr(S_DBOPEN));

    if ((len = vtstrlen(tmp)) >= (DB_PATHLEN - 1))
        return (dberr(S_NAMELEN));

    if (task->dbtmp)
        psp_freeMemory(task->dbtmp, 0);

    if (tmp[len - 1] != DIRCHAR)
    {
        if (len >= DB_PATHLEN - 2)
        {
            task->dbtmp = NULL;
            return (dberr(S_NAMELEN));
        }

        task->dbtmp = psp_getMemory(len + 2, 0);
        memcpy(task->dbtmp, tmp, len);
        task->dbtmp[len] = DIRCHAR;
        task->dbtmp[len + 1] = 0;
    }
    else
        task->dbtmp = psp_strdup(tmp, 0);

    return (task->db_status);
}


/* ======================================================================
    Set Lockmgr Communication Type
*/
int INTERNAL_FCN dlockcomm(LMC_AVAIL_FCN *avail, DB_TASK *task)
{
    short stat;

    if (task->dbopen)
        return dberr(S_DBOPEN);

    if (!avail)
    {
        if (task->lmc)
        {
            psp_lmcCleanup(task->lmc);
            task->lmc = NULL;
        }

        vtstrcpy(task->lmc_type, psp_lmcName(task->lmc));
        return task->db_status;
    }

    stat = psp_lmcSetup(&task->lmc, avail); 
    vtstrcpy(task->lmc_type, psp_lmcName(task->lmc));

    return (task->db_status = stat);
}


/* ======================================================================
    Send timeouts for read and write locks
*/
int INTERNAL_FCN dlocktimeout(int rs, int ws, DB_TASK *task)
{
    task->readlocksecs = rs;
    task->writelocksecs = ws;

    return (S_OKAY);
}

/* ======================================================================
    Set Lockmgr Identifier
*/
int INTERNAL_FCN dlockmgr(const DB_TCHAR *id, DB_TASK *task)
{
    size_t    len;
    DB_TCHAR *name;
    
    if (task->dbopen)
        return (dberr(S_DBOPEN));

    if (!id || !*id)
        return (dberr(S_INVPTR));

    while (vistspace(*id))
        ++id;

    name = psp_strdup(id, 0);
    while (name[(len = vtstrlen(name)) - 1] == DB_TEXT(' '))
        name[len - 1] = DB_TEXT('\0');

    if (!psp_validLockmgr(name))
    {
        psp_freeMemory(name, 0);
        return dberr(S_BADUSERID);
    }

    if (task->lockmgrn)
       psp_freeMemory(task->lockmgrn, 0);

    task->lockmgrn = name;

    return task->db_status;
}


/* ======================================================================
    Turn on db.* runtime options
*/
int INTERNAL_FCN don_opt(unsigned long options, DB_TASK *task)
{
#ifdef DB_DEBUG
    if (options & DEBUG_OPT)
    {
        options &= ~DEBUG_OPT;
        
        /* some options cannot be changed while the database is open */
        if (task->dbopen && (options & DEBUG_OPEN_OPTS))
            return (dberr(S_DBCLOSE));

        task->db_debug |= options;    /* turn them on */
        if (task->db_debug & LOCK_CHECK)  /* these are mutually exclusive */
            task->db_debug &= ~PAGE_CHECK;

        return (task->db_status);
    }
#endif

#ifdef DB_TRACE
    if (options & TRACE_OPT)
    {
        options &= ~TRACE_OPT;
      
        /* some options cannot be changed while the database is open */
        if (task->dbopen && (options & TRACE_OPEN_OPTS))
            return (dberr(S_DBCLOSE));

        if (task->db_trace == 0x0FFFFFFF)
            task->db_trace = 0L;

        task->db_trace |= options;    /* turn them on */
        return (task->db_status);
    }
#endif

    if (!(options & (unsigned long)0xF0000000))
    {
        /* some options cannot be changed while the database is open */
        if (task->dbopen && (options & OPEN_OPTS))
            return (dberr(S_DBCLOSE));

        if (options & ARCLOGGING)
            task->trlog_flag = 1;

        if (options & IGNORECASE)
        {
            if (ctbl_ignorecase(task) != S_OKAY)
                return (task->db_status);
        }

        task->dboptions |= options;    /* turn them on */
        task->dboptflag |= options;    /* they were explicitly set */
    }

    return (task->db_status);
}


/* ======================================================================
    Turn off db.* runtime options
*/
int INTERNAL_FCN doff_opt(unsigned long options, DB_TASK *task)
{
#ifdef DB_DEBUG
    if (options & DEBUG_OPT)
    {
        options &= ~DEBUG_OPT;
        
        /* some options cannot be changed while the database is open */
        if (task->dbopen && (options & DEBUG_OPEN_OPTS))
            return (dberr(S_DBCLOSE));

        task->db_debug &= ~options;    /* turn them off */
        return (task->db_status);
    }
#endif

#ifdef DB_TRACE
    if (options & TRACE_OPT)
    {
        options &= ~TRACE_OPT;
      
        /* some options cannot be changed while the database is open */
        if (task->dbopen && (options & TRACE_OPEN_OPTS))
            return (dberr(S_DBCLOSE));

        task->db_trace &= ~options;    /* turn them off */
        return (task->db_status);
    }
#endif

    if (!(options & (unsigned long)0xF0000000))
    {
        /* some options cannot be changed while the database is open */
        if (task->dbopen && (options & OPEN_OPTS))
            return (dberr(S_DBCLOSE));

        if (options & ARCLOGGING)
            task->trlog_flag = 0;

        if (options & IGNORECASE)
        {
            if (ctbl_usecase(task) != S_OKAY)
                return (task->db_status);
        }

        task->dboptions &= ~options;    /* turn them off */
        task->dboptflag |= options;     /* they were explicitly set */
    }

    return (task->db_status);
}


/* ======================================================================
    Reset runtime options back to their default values
*/
int INTERNAL_FCN ddef_opt(unsigned long options, DB_TASK *task)
{
    /* explicitly reset each option in case there is any special processing
       required
    */

#ifdef DB_DEBUG
    if (options & DEBUG_OPT)
    {
        options &= ~DEBUG_OPT;
      
        /* some options cannot be changed while the database is open */
        if (task->dbopen && (options & DEBUG_OPEN_OPTS))
            return (dberr(S_DBCLOSE));

        doff_opt(DEBUG_OPT | (options & task->db_debug & ~DEFAULT_DEBUG_OPTS), task);
        don_opt(DEBUG_OPT | (options & ~task->db_debug & DEFAULT_DEBUG_OPTS), task);
        return (task->db_status);
    }
#endif

#ifdef DB_TRACE
    if (options & TRACE_OPT)
    {
        options &= ~TRACE_OPT;
      
        /* some options cannot be changed while the database is open */
        if (task->dbopen && (options & TRACE_OPEN_OPTS))
            return (dberr(S_DBCLOSE));

        doff_opt(TRACE_OPT | (options & task->db_trace & ~DEFAULT_TRACE_OPTS), task);
        don_opt(TRACE_OPT | (options & ~task->db_trace & DEFAULT_TRACE_OPTS), task);
        return (task->db_status);
    }
#endif
    
    if (!(options & (unsigned long)0xF0000000))
    {
        /* some options cannot be changed while the database is open */
        if (task->dbopen && (options & OPEN_OPTS))
            return (dberr(S_DBCLOSE));

        doff_opt(options & task->dboptions & ~DEFAULT_OPTS, task);
        don_opt(options & ~task->dboptions & DEFAULT_OPTS, task);
        task->dboptflag &= ~options;
    }

    return (task->db_status);
}


