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
    Set record lock bit of current record
*/
int INTERNAL_FCN drlbset(DB_TASK *task, int dbn)
{
    FILE_NO  file;
    DB_ULONG slot;
    short    rid;
    int      needReadLock;

    if (!task->curr_rec)
        return (dberr(S_NOCR));

    DECODE_DBA(task->curr_rec, &file, &slot);
    file = (FILE_NO) NUM2INT(file, ft_offset);

    /* shared-mode && ! w-locked && ! x-locked ==> need read lock */
    needReadLock = task->dbopen == 1 && task->app_locks[file] >= 0 &&
                   !task->excl_locks[file];

    if (needReadLock)
    {
        /* request super-read lock on file, might be upgrade from 'r' */
        task->lock_pkt->nfiles = 1;
        task->lock_pkt->locks[0].type = 'R';
        task->lock_pkt->locks[0].fref = task->file_refs[file];
        if (send_lock_pkt(NULL,task) != S_OKAY)
            return (task->db_status);

        STAT_lock(file, task->app_locks[file] ? STAT_LOCK_r2R : STAT_LOCK_R, task);
    }

    if (dio_rrlb(task->curr_rec, &rid, task) != S_OKAY)
        return (task->db_status);

    if (rid & RLBMASK)
        task->rlb_status = S_LOCKED;
    else
    {
        rid |= RLBMASK;
        task->rlb_status = dio_wrlb(task->curr_rec, rid, task);
    }

    if (needReadLock)
    {
        if (task->app_locks[file])
        {
            /* was 'r' locked already, downgrade from 'R' to 'r' */
            task->lock_pkt->nfiles = 1;
            task->lock_pkt->locks[0].type = 'r';
            task->lock_pkt->locks[0].fref = task->file_refs[file];
            if (send_lock_pkt(NULL,task) != S_OKAY)
                return (task->db_status);

            STAT_lock(file, STAT_LOCK_R2r, task);
        }
        else
        {
            /* free 'R' lock */
            task->free_pkt->nfiles = 1;
            task->free_pkt->frefs[0] = task->file_refs[file];
            if (send_free_pkt(task) != S_OKAY)
                return (task->db_status);

            STAT_lock(file, STAT_FREE_R, task);
        }
    }

    return (task->db_status = task->rlb_status);
}


/* ======================================================================
    Clear record lock bit of current record
*/

int INTERNAL_FCN drlbclr(DB_TASK *task, int dbn)
{
    FILE_NO  file;
    short    rid;
    int      needReadLock;
    DB_ULONG slot;

    if (!task->curr_rec)
        return (dberr(S_NOCR));

    DECODE_DBA(task->curr_rec, &file, &slot);
    file = (FILE_NO) NUM2INT(file, ft_offset);

    /* shared_mode && ! w-locked && ! x-locked ==> need read lock */
    needReadLock = task->dbopen == 1 && task->app_locks[file] >= 0 &&
                   !task->excl_locks[file];

    if (needReadLock)
    {
        /* if in a trx, a write lock or exclusive lock is required so
           that the cache is updated but not the disk, thus allowing
           d_trend() to do the actual clearing or allowing d_trabort()
           to leave it set.
        */
        if (task->trans_id[0])
            return (dberr(S_NOTLOCKED));

        /* request super-read lock on file, might be upgrade from 'r' */
        task->lock_pkt->nfiles = 1;
        task->lock_pkt->locks[0].type = 'R';
        task->lock_pkt->locks[0].fref = task->file_refs[file];
        if (send_lock_pkt(NULL,task) != S_OKAY)
            return (task->db_status);

        STAT_lock(file, task->app_locks[file] ? STAT_LOCK_r2R : STAT_LOCK_R, task);
    }

    /* read rlb */
    if (dio_rrlb(task->curr_rec, &rid, task) != S_OKAY)
        return (task->db_status);

    /* clear rlb */
    if (rid & RLBMASK)
    {
        rid &= ~RLBMASK;
        task->rlb_status = dio_wrlb(task->curr_rec, rid, task);
    }
    else
        task->rlb_status = S_UNLOCKED;

    if (needReadLock)
    {
        if (task->app_locks[file])
        {
            /* was 'r' locked already, downgrade 'R' to 'r' */
            task->lock_pkt->nfiles = 1;
            task->lock_pkt->locks[0].type = 'r';
            task->lock_pkt->locks[0].fref = task->file_refs[file];
            if (send_lock_pkt(NULL,task) != S_OKAY)
                return (task->db_status);

            STAT_lock(file, STAT_LOCK_R2r, task);
        }
        else
        {
            /* free the 'R' lock */
            task->free_pkt->nfiles = 1;
            task->free_pkt->frefs[0] = task->file_refs[file];
            if (send_free_pkt(task) != S_OKAY)
                return (task->db_status);

            STAT_lock(file, STAT_FREE_R, task);
        }
    }

    return (task->db_status = task->rlb_status);
}


/* ======================================================================
    Test record lock bit of current record
*/
int INTERNAL_FCN drlbtst(DB_TASK *task, int dbn)
{
    short rid;

    if (!task->curr_rec)
        return (dberr(S_NOCR));

    if (dio_rrlb(task->curr_rec, &rid, task) != S_OKAY)
        return (task->db_status);

    if (rid & RLBMASK)
        task->rlb_status = S_LOCKED;
    else
        task->rlb_status = S_UNLOCKED;

    return (task->db_status = task->rlb_status);
}


