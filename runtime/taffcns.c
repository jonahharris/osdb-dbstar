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

/*--------------------------------------------------------------------------

    This module contains the functions which access and manipulate
    the Transaction Activity File.   The name of a transaction log
    file is written to the TAF just before a commit and removed
    following the commit in order to provide for external recovery
    in the event that the lock manager fails.

    Access to the TAF must be synchronized so that only one process
    has control of it at a time.  The lock manager cannot be used for
    this synchronization because if it is active then the TAF is not
    needed.  Thus, some outside resource must be used to lock the TAF
    for exclusive access.  Function "locking" (or "lockf" on Unix) is
    called to gain exclusive access to the TAF.  This function is
    available in MS-DOS 3+, and Unix System V and BSD (as "lockf").
    If it is not available for your particular network or operating
    system then you will need to modify the call to locking to that
    which is available on your system.  Alternatively, you can compile
    this module with constant PORTABLE defined.  This will use a
    zero-length file "dblfg" as a semaphore on the TAF.  If the file
    exists then the TAF is not being used.  If it doesn't exist then
    the TAF is being used.  This technique has been used in earlier
    versions of db.*.  It's not ideal but it does work.

    We used function "locking" instead of simply opening the file
    exclusively because on PC-NETWORK a sharing violation occurs
    if a program attempts to exclusively open a file which is
    already exlusively opened by another program.  We could accept
    this if all it did was return a status code but, for some reason,
    the sharing violation is reported to the user (similar to a
    floppy disk error).

--------------------------------------------------------------------------*/

#include "db.star.h"

#ifndef O_SYNC
#define O_SYNC 0x0000
#endif

/* maximum number of attempts to gain access TAF */
#define MAXTRIES 100

/* buffer containing TAF file */
TAFFILE tafbuf;
#define LOCK_LEN   sizeof(tafbuf)

static DB_TCHAR * INTERNAL_FCN dblfg_name(DB_TASK *);
static int  INTERNAL_FCN taf_lock(DB_TASK *);
static void INTERNAL_FCN taf_unlock(DB_TASK *);

/***************************************************************************/

int INTERNAL_FCN taf_login(DB_TASK *task)
{
    int stat;

#ifdef MULTI_TAFFILE
    if (task->dboptions & MULTITAF)
    {
        if (task->dbuserid[0])
        {
            vtstrcpy(psp_pathGetFile(task->dbtaf), task->dbuserid);
            vtstrcat(task->dbtaf, DB_TEXT(".taf"));
        }
        else if (task->dbopen == 1)
            return (dberr(S_DBLACCESS));
    }
#endif /* MULTI_TAFFILE */

#if 0
    /* create the taf if necessary */
    if ((stat = taf_open(task)) != S_OKAY)
        return (task->db_status = stat);

    taf_close(task);     /* taf must be closed before access */
#endif

    /* TAF exists at this point */
    stat = taf_access(DEL_LOGFILE, task);
    if (stat != S_OKAY)
        return (task->db_status = stat);

    if (tafbuf.user_count == 0)
    {
        tafbuf.unicode = DBSTAR_UNICODE_FLAG;
        vtstrcpy(tafbuf.lmc_type, task->lmc_type);
        if (task->lockmgrn)
            vtstrcpy(tafbuf.lockmgrn, task->lockmgrn);
        else
            tafbuf.lockmgrn[0] = DB_TEXT('\0');
    }
    else
    {
        /* The LMC transport type, the lock manager name, and the unicode
           setting must all match between this task and the TAF file. */
        if (vtstrcmp(tafbuf.lmc_type, task->lmc_type) != 0 ||
                (task->lockmgrn && vtstrcmp(tafbuf.lockmgrn, task->lockmgrn)) ||
                (!task->lockmgrn && tafbuf.lockmgrn[0] != DB_TEXT('\0')) ||
                tafbuf.unicode != DBSTAR_UNICODE_FLAG)
            goto ret_err;
    }

    ++tafbuf.user_count;

    stat = taf_release(0, task);

    return (task->db_status);

ret_err:
    taf_unlock(task);
    taf_close(task);
    return (dberr(S_TAFSYNC));
}

/***************************************************************************/

int INTERNAL_FCN taf_logout(DB_TASK *task)
{
    int stat;

#if 0
    if (task->lockMgrComm == LMC_GENERAL && task->lmc_avail(task) != S_OKAY)
    {
        /* assume that if the DBL is not accessable, neither is the TAF */
        taf_close(task);
        return task->db_status;
    }
#endif

    stat = taf_access(DEL_LOGFILE, task);
    if (stat != S_OKAY)
        return (task->db_status = stat);

    if (vtstrcmp(tafbuf.lmc_type, task->lmc_type) != 0 ||
            (task->lockmgrn && vtstrcmp(tafbuf.lockmgrn,task->lockmgrn) != 0) ||
            (!task->lockmgrn && tafbuf.lockmgrn[0] != DB_TEXT('\0')) ||
            tafbuf.unicode != DBSTAR_UNICODE_FLAG)
        goto ret_err;

    if (tafbuf.user_count > 0)
        --tafbuf.user_count;

    taf_release(0, task);
    taf_close(task);

    return (task->db_status);

ret_err:
    taf_unlock(task);
    taf_close(task);
    return (dberr(S_TAFSYNC));
}


/***************************************************************************/

static void INTERNAL_FCN taf_unlock(DB_TASK *task)
{
    PSP_FH lfn;

    /* one user mode does not require locks */
    if (!task->db_lockmgr)
        return;

    /* Free the lock */
    if (task->dboptions & PORTABLE)
    {
        if (psp_lmcFlags(task->lmc) & PSP_FLAG_TAF_CLOSE)
            taf_close(task); /* close file before releasing it */
        else
            commit_file(task->lfn, task);

        /* create lock file guard file */
        lfn = open_b(dblfg_name(task), O_CREAT | O_SYNC, PSP_FLAG_SYNC, task);
        if (lfn != NULL)
            psp_fileClose(lfn);
    }
    else
    {
        commit_file(task->lfn, task);

        /* need the file handle here */
        psp_fileUnlock(task->lfn);

        if (psp_lmcFlags(task->lmc) & PSP_FLAG_TAF_CLOSE)
            taf_close(task); /* close file before releasing it */
    }
}

/***************************************************************************/

static int INTERNAL_FCN taf_lock(DB_TASK *task)
{
    int not_locked;

    /* one user mode does not require locks */
    if (!task->db_lockmgr)
          return 0;

    if (task->dboptions & PORTABLE)
    {
        not_locked = psp_fileRemove(dblfg_name(task));
        if (psp_errno() == EACCES)
            dberr(S_EACCESS);
    }
    else
        not_locked = psp_fileLock(task->lfn);

    return not_locked;
}

/* ======================================================================
    Open Transaction Activity File
*/
int INTERNAL_FCN taf_open(DB_TASK *task)
{
    unsigned long rd_opt, fopt;

    /* even with READONLY option set, attempt to creat log file */
    rd_opt = task->dboptions & READONLY;

    /* Do not re-open an open file */
    if (task->lfn)
        return (task->db_status);

    task->dboptions &= ~READONLY;       /* try to create */

    /* only open with the O_SYNC option when SYNCFILES is on */
    if (task->dboptions & SYNCFILES)
        fopt = O_RDWR | O_SYNC;
    else
        fopt = O_RDWR;

    if ((task->lfn = open_b(task->dbtaf, fopt, PSP_FLAG_SYNC, task)) == NULL)
    {
        if (psp_errno() == ENOENT)
        {
            /* TAF does not exist - try to create it */
            task->lfn = open_b(task->dbtaf, fopt | O_CREAT, PSP_FLAG_SYNC,
                    task);
            if (task->lfn == NULL)
            {
                task->dboptions |= rd_opt;
                return (dberr(S_TAFCREATE));
            }

            STAT_taf_open(task);

            memset(&tafbuf, '\0', sizeof(TAFFILE));
            tafbuf.unicode = DBSTAR_UNICODE_FLAG;
            vtstrcpy(tafbuf.lmc_type, task->lmc_type);
            if (task->lockmgrn)
                vtstrcpy(tafbuf.lockmgrn, task->lockmgrn);
            else
                tafbuf.lockmgrn[0] = DB_TEXT('\0');

            if (psp_fileSeekWrite(task->lfn, 0, &tafbuf,
                    sizeof(tafbuf)) < (int) sizeof(tafbuf))
            {
                psp_fileClose(task->lfn);
                task->lfn = NULL;
                task->dboptions |= rd_opt;
                return (dberr(S_DBLERR));
            }

            STAT_taf_write(sizeof(tafbuf), task);

            commit_file(task->lfn, task);

            if (task->dboptions & PORTABLE)
            {
                PSP_FH lfg;

                /* also create lock file guard file - guard file must be
                    opened with O_SYNC 
                */
                lfg = open_b(dblfg_name(task), O_CREAT|O_SYNC, 0, task);
                if (lfg != NULL)
                    psp_fileClose(lfg);
            }
        }
        else
        {
            task->dboptions |= rd_opt;
            return (dberr(S_TAFLOCK));
        }
    }
    else
        STAT_taf_open(task);

    task->dboptions |= rd_opt;
    return (task->db_status);
}



/* ======================================================================
    Close Transaction Activity File
*/
int INTERNAL_FCN taf_close(DB_TASK *task)
{
    if (task->lfn != NULL)
    {
        psp_fileClose(task->lfn);
        task->lfn = NULL;
    }

    return (task->db_status);
}



/* ======================================================================
    Gain access and read transaction activity file
*/
int INTERNAL_FCN taf_access(int fcn, DB_TASK *task)
{

    register int not_locked;
    int          bytes;
    int          taf_count = 0;

    /* if PORTABLE open after getting lock below */
    if (!(task->dboptions & PORTABLE) && !task->lfn)
    {
        if (taf_open(task) != S_OKAY)
            return task->db_status;
    }

retry:
    errno = taf_count = 0;                /* use as retry count until opened */
    do
    {
#ifdef MULTI_TAFFILE
        if (task->dboptions & MULTITAF && fcn & NO_TAFFILE_LOCK)
            not_locked = 0;
        else
#endif
            not_locked = taf_lock(task);

        if (not_locked)
        {
            if (++taf_count >= MAXTRIES || psp_errno() == EINVAL)
            {
                /* this should never happen, but ... */
                return dberr(S_DBLACCESS);
            }

            naptime(task);
            adjust_naptime(BY_NAP_FAILURE, task);
        }
    } while (not_locked);

    adjust_naptime(BY_NAP_SUCCESS, task);

    if (task->dboptions & PORTABLE && !task->lfn)
    {
        /* open the file only after obtaining exclusive access */
        if (taf_open(task) != S_OKAY)
            return task->db_status;
    }

    bytes = psp_fileSeekRead(task->lfn, 0, &tafbuf, sizeof(tafbuf));
    if (bytes < 0)
    {
#ifdef MULTI_TAFFILE
        if (!(task->dboptions & MULTITAF) || !(fcn & NO_TAFFILE_LOCK))
#endif
            taf_unlock(task);

        return (dberr(S_DBLERR));
    }

    STAT_taf_read(sizeof(tafbuf), task);

    /* If a logfile is to be deleted, this check is not necessary */
    if ((fcn & ADD_LOGFILE) && tafbuf.cnt >= TAFLIMIT)
    {
#ifdef MULTI_TAFFILE
        if (!(task->dboptions & MULTITAF) || !(fcn & NO_TAFFILE_LOCK))
#endif
            taf_unlock(task);

        /* Have to wait until a slot opens up */
        naptime(task);
        adjust_naptime(BY_NAP_FAILURE, task);

        /* possible infinite loop if over TAFLIMIT commits and all processes
           are attempting to do a task->taf_add()
        */
        goto retry;
    }

    adjust_naptime(BY_NAP_SUCCESS, task);

    return (task->db_status);
}



/* ======================================================================
    Write and release transaction activity file
*/
int INTERNAL_FCN taf_release(int fcn, DB_TASK *task)
{
    int stat = S_OKAY;

    if (!(task->dboptions & READONLY))
    {
        if (psp_fileSeekWrite(task->lfn, 0, &tafbuf,
                sizeof(tafbuf)) < (int) sizeof(tafbuf))
        {
            /* need to do taf_unlock before returning */
            stat = dberr(S_DBLERR);
        }
        else
            STAT_taf_write(sizeof(tafbuf), task);
    }

#ifdef MULTI_TAFFILE
    if (!(task->dboptions & MULTITAF) || !(fcn & NO_TAFFILE_LOCK))
#endif
        taf_unlock(task);

#if 0
    taf_close(task);   /* so lots of tasks don't chew up all available file handles */
#endif

    return (task->db_status = stat);
}


/* ======================================================================
    Add log file name to taf
*/
int INTERNAL_FCN taf_add(const DB_TCHAR *tlogfile, DB_TASK *task)
{
    int mode = ADD_LOGFILE;

#ifdef MULTI_TAFFILE
    if (task->dboptions & MULTITAF)
        mode |= NO_TAFFILE_LOCK;
#endif

    if (taf_access(mode, task) == S_OKAY)
    {
        vtstrncpy(tafbuf.files[tafbuf.cnt++], psp_pathGetFile(tlogfile),
                    LOGFILELEN);
        taf_release(mode, task);
    }

    return task->db_status;
}



/* ======================================================================
    Delete log file name from taf
*/
int INTERNAL_FCN taf_del(const DB_TCHAR *tlogfile, DB_TASK *task)
{
    register int     i;
    DB_TCHAR *file_ptr;
    DB_TCHAR *base_logfile = psp_pathGetFile(tlogfile);

    for (i = 0; i < tafbuf.cnt; ++i)
    {
        if (vtstrcmp(file_ptr = tafbuf.files[i], base_logfile) == 0)
        {
            if (i < --tafbuf.cnt)
            {
                vtstrncpy(file_ptr, tafbuf.files[tafbuf.cnt], LOGFILELEN);
                break;
            }
        }
    }

    return task->db_status;
}

/***************************************************************************/

static DB_TCHAR * INTERNAL_FCN dblfg_name(DB_TASK *task)
{
    DB_TCHAR *far_ptr;
    static DB_TCHAR dblfg[FILENMLEN];

    /* lfg file is same name as task->dbtaf, but with .lfg extension */
    vtstrcpy(dblfg, task->dbtaf);
    if ((far_ptr = vtstrrchr(dblfg, DB_TEXT('.'))) != NULL)
        *far_ptr = DB_TEXT('\0');         /* strip off . suffix */

    vtstrcat(dblfg, DB_TEXT(".lfg"));    /* append .lfg suffix */
    return dblfg;
}


