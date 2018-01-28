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

static int INTERNAL_FCN process_lock(struct lock_descr *, DB_TCHAR, DB_TASK *);
static int INTERNAL_FCN keep_locks(struct lock_descr *, DB_TASK *);
static int INTERNAL_FCN free_files(struct lock_descr *, DB_TASK *);
static int INTERNAL_FCN lock_files(int, LOCK_REQUEST *, DB_TASK *);

#define KEYMARK ((unsigned) 30000)


/* ======================================================================
    Establish record file locks
*/
int INTERNAL_FCN dreclock(int rec, DB_TCHAR *lock_type, DB_TASK *task, int dbn)
{
    LOCK_REQUEST  lr;
    RECORD_ENTRY *rec_ptr;
    int           nrec;

    if (nrec_check(rec, &nrec, &rec_ptr, task) != S_OKAY)
        return (task->db_status);

    lr.item = rec;
    lr.type = *lock_type;

    return (dlock(1, &lr, task, dbn));
}


/* ======================================================================
    Establish set file locks
*/
int INTERNAL_FCN dsetlock(int set, DB_TCHAR *lock_type, DB_TASK *task, int dbn)
{
    LOCK_REQUEST lr;
    SET_ENTRY   *set_ptr;
    int          nset;

    if (nset_check(set, &nset, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    lr.item = set;
    lr.type = *lock_type;

    return (dlock(1, &lr, task, dbn));
}


/* ======================================================================
    Lock key file
*/
int INTERNAL_FCN dkeylock(long key, DB_TCHAR *lock_type, DB_TASK *task, int dbn)
{
    int           fld,
                  rec;
    LOCK_REQUEST  lr;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY  *fld_ptr;

    if (nfld_check(key, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    if (fld_ptr->fd_key == NOKEY)
        return (dberr(S_NOTKEY));

    lr.item = (unsigned int)fld + KEYMARK;
    lr.type = *lock_type;

    return (dlock(1, &lr, task, dbn));
}


/* ======================================================================
    Lock a group of records and/or sets
*/
int INTERNAL_FCN dlock(int count, LOCK_REQUEST *lrpkt, DB_TASK *task, int dbn)
{
    register unsigned int  item;
    register int           i;
    register LOCK_REQUEST *lrpkt_ptr;
    struct lock_descr     *ld_ptr = NULL;

    if (task->dbopen >= 2)
        return (task->db_status);

    task->lock_pkt->nfiles = 0;
    for (i = 0, lrpkt_ptr = lrpkt; (task->db_status == S_OKAY) && (i < count);
         ++i, ++lrpkt_ptr)
    {
        if (lrpkt_ptr->item >= KEYMARK)
        {
            item = task->field_table[lrpkt_ptr->item - KEYMARK].fd_keyno;
            ld_ptr = &task->key_locks[item];
        }
        else if (lrpkt_ptr->item >= SETMARK)
        {
            item = NUM2INT(lrpkt_ptr->item - SETMARK, st_offset);
            if (item < (unsigned) ORIGIN(st_offset) ||
                item >= (unsigned) (DB_REF(Size_st) + ORIGIN(st_offset)))
                dberr(S_INVSET);
            else
                ld_ptr = &task->set_locks[item];
        }
        else if (lrpkt_ptr->item >= RECMARK)
        {
            item = NUM2INT(lrpkt_ptr->item - RECMARK, rt_offset);
            if (item < (unsigned) ORIGIN(rt_offset) ||
                item >= (unsigned) (DB_REF(Size_rt) + ORIGIN(rt_offset)))
                dberr(S_INVREC);
            else if (task->record_table[item].rt_flags & STATIC)
                dberr(S_STATIC);
            else
                ld_ptr = &task->rec_locks[item];
        }
        else
            dberr(S_INVNUM);

        if (task->db_status == S_OKAY)
        {
            process_lock(ld_ptr, (DB_TCHAR) lrpkt_ptr->type, task);
            /* if process_lock() fails, break out of loop after i is incremented */
        }
        else
            break;
    }

    if (task->db_status == S_OKAY)
        lock_files(count, lrpkt, task);
    else
        count = i;  /* only reset as many as were processed */

    if (task->db_status != S_OKAY)
    {
        /* reset lock descriptor tables to previous state */
        for (i = 0, lrpkt_ptr = lrpkt; i < count; ++i, ++lrpkt_ptr)
        {
            if (lrpkt_ptr->item >= KEYMARK)
            {
                item = task->field_table[lrpkt_ptr->item - KEYMARK].fd_keyno;
                ld_ptr = &task->key_locks[item];
            }
            else if (lrpkt_ptr->item >= SETMARK)
            {
                item = NUM2INT(lrpkt_ptr->item - (unsigned) SETMARK, st_offset);
                ld_ptr = &task->set_locks[item];
            }
            else if (lrpkt_ptr->item >= RECMARK)
            {
                item = NUM2INT(lrpkt_ptr->item - (unsigned) RECMARK, rt_offset);
                ld_ptr = &task->rec_locks[item];
            }
            else
                continue;

            ld_ptr->fl_type = ld_ptr->fl_prev;
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Process set/record lock
*/
static int INTERNAL_FCN process_lock(struct lock_descr *ld_ptr, DB_TCHAR type,
                                     DB_TASK *task)
{
    register int            fl_lc;                 /* loop control */
    FILE_NO                 fno;
    register unsigned short i;
    register DB_LOCKREQ    *lockreq_ptr;
    FILE_NO                *fl_ptr;
    FILE_NO                 fref;

    ld_ptr->fl_prev = ld_ptr->fl_type;
    switch (type)
    {
        case DB_TEXT('k'):
            if (!task->trans_id[0])
                dberr(S_NOTRANS);
            else if (ld_ptr->fl_prev == DB_TEXT('f'))
                dberr(S_NOTLOCKED);
            else if (ld_ptr->fl_prev != DB_TEXT('x'))
                return (keep_locks(ld_ptr, task));

            break;

        case DB_TEXT('r'):
            if (ld_ptr->fl_prev != DB_TEXT('f'))
                dberr(S_NOTFREE);
            else
                ld_ptr->fl_type = DB_TEXT('r');

            break;

        case DB_TEXT('w'):
            if (!task->trans_id[0])
                dberr(S_NOTRANS);
            else if (ld_ptr->fl_prev != DB_TEXT('f') &&
                    ld_ptr->fl_prev != DB_TEXT('r'))
                dberr(S_NOTFREE);
            else
                ld_ptr->fl_type = DB_TEXT('w');

            break;

        case DB_TEXT('x'):
            if (ld_ptr->fl_prev != DB_TEXT('f') &&
                    ld_ptr->fl_prev != DB_TEXT('r'))
                dberr(S_NOTFREE);
            else
                ld_ptr->fl_type = DB_TEXT('x');

            break;

        default:
            dberr(S_BADTYPE);
    }

    if (task->db_status == S_OKAY)
    {
        /* build lock request packet */
        fl_ptr = (FILE_NO *) ld_ptr->fl_list;
        for (fl_lc = ld_ptr->fl_cnt; --fl_lc >= 0; ++fl_ptr)
        {
            fref = task->file_refs[fno = *fl_ptr];
            if (fref == -1)
                continue;      /* file is TEMPORARY */

            for ( i = 0, lockreq_ptr = task->lock_pkt->locks;
                  (i < task->lock_pkt->nfiles) && (lockreq_ptr->fref != fref);
                  ++i, ++lockreq_ptr)
                ; /* null statement */

            if (i < task->lock_pkt->nfiles)
            {
                /* file already is in lock request packet */
                if (lockreq_ptr->type == DB_TEXT('r') ||
                        ld_ptr->fl_type == DB_TEXT('x'))
                    lockreq_ptr->type = ld_ptr->fl_type;
            }
            else if (!task->excl_locks[fno] && (!task->app_locks[fno] ||
                    (ld_ptr->fl_type == DB_TEXT('w') &&
                    task->app_locks[fno] > 0) ||
                    (ld_ptr->fl_type == DB_TEXT('x'))))
            {
                /* add to lock request packet */
                ++task->lock_pkt->nfiles;
                lockreq_ptr->fref = fref;
                lockreq_ptr->type = ld_ptr->fl_type;
            }
        }
    }

    return task->db_status;
}


/* ======================================================================
    Lock database files
*/
static int INTERNAL_FCN lock_files(
    int           count,
    LOCK_REQUEST *lrpkt,
    DB_TASK      *task)
{
    register int          fl_lc;                 /* loop control */
    struct lock_descr    *ld_ptr;
    FILE_NO               fno;
    register unsigned int item;
    int                   l;
    LOCK_REQUEST         *lrpkt_ptr;
    int                  *appl_ptr;
    int                  *excl_ptr;
    FILE_NO              *fl_ptr;
    DB_BOOLEAN            clearStatic;

    if (send_lock_pkt(NULL, task) == S_OKAY)
    {
        /* update task->app_locks and excl_lock tables */
        for (l = 0, lrpkt_ptr = lrpkt; l < count; ++l, ++lrpkt_ptr)
        {
            if (lrpkt_ptr->type == DB_TEXT('k'))
                continue;               /* skip keep lock requests */

            /* process each record/set lock */
            if (lrpkt_ptr->item >= KEYMARK)
            {
                clearStatic = 0;
                item = task->field_table[lrpkt_ptr->item - KEYMARK].fd_keyno;
                ld_ptr = &task->key_locks[item];
            }
            else if (lrpkt_ptr->item >= SETMARK)
            {
                /* Since all the records in the file are static, the file will
                    be flagged as static.  However, the set pointers CAN be
                    updated in a static record, thus requiring the pages to be
                    read in again.
                */
                clearStatic = 1;
                item = NUM2INT(lrpkt_ptr->item - (unsigned) SETMARK, st_offset);
                ld_ptr = &task->set_locks[item];
            }
            else
            {
                clearStatic = 0;
                item = NUM2INT(lrpkt_ptr->item - (unsigned) RECMARK, rt_offset);
                ld_ptr = &task->rec_locks[item];
            }
            
            fl_ptr = (FILE_NO *) ld_ptr->fl_list;
            for (fl_lc = ld_ptr->fl_cnt; --fl_lc >= 0; ++fl_ptr)
            {
                /* process each file for each record/set lock */
                fno = *fl_ptr;
                if (task->file_table[fno].ft_flags & TEMPORARY)
                    continue;

                appl_ptr = &task->app_locks[fno];
                excl_ptr = &task->excl_locks[fno];
                if ((! *appl_ptr) && (! *excl_ptr))
                {
                    if (clearStatic || !(task->file_table[fno].ft_flags & STATIC))
                    {
                        /*
                        Don't clear the cache if the timestamps match.
                          ft_lmtimestamp     this is the value obtained last from
                                                    the lock mgr
                          ft_cachetimestamp  this was the timestamp value during
                                                    previous communication
                        If the cache does get cleared, update the stored timestamp.
                        */
#if 0
                        if (task->file_table[fno].ft_lmtimestamp != task->file_table[fno].ft_cachetimestamp)
                        {
#endif
                            dio_clrfile(fno, task); /* clear file's pages from cache */
#if 0
                            task->file_table[fno].ft_cachetimestamp = task->file_table[fno].ft_lmtimestamp;
                        }
#endif
                    }
                }

                if (ld_ptr->fl_type == DB_TEXT('r'))
                {
                    if (*appl_ptr >= 0)
                    {
                        if (*appl_ptr == 0)
                            STAT_lock(fno, STAT_LOCK_r, task);

                        ++*appl_ptr;  /* increment if file free or read-locked */
                    }
                }
                else
                {
                    if (ld_ptr->fl_type == DB_TEXT('w'))
                    {
                        *appl_ptr = -1;
                        if (ld_ptr->fl_prev == DB_TEXT('r'))
                            STAT_lock(fno, STAT_LOCK_r2w, task);
                        else
                            STAT_lock(fno, STAT_LOCK_w, task);
                    }
                    else if (ld_ptr->fl_type == DB_TEXT('x'))
                    {
                        ++*excl_ptr;
                        if (ld_ptr->fl_prev == DB_TEXT('r'))
                        {
                            /* read to excl lock upgrade */
                            --*appl_ptr;
                            STAT_lock(fno, STAT_LOCK_r2x, task);
                        }
                        else
                            STAT_lock(fno, STAT_LOCK_x, task);
                    }
                }
            }
        }
    }

    return task->db_status;
}


/* ======================================================================
    Setup table to keep locks after transaction end
*/
static int INTERNAL_FCN keep_locks(
    struct lock_descr *ld_ptr,
    DB_TASK           *task)
{
    register int      fl_lc;                 /* loop control */
    register FILE_NO *fl_ptr;

    /* Mark lock as kept */
    ld_ptr->fl_kept = TRUE;

    fl_ptr = (FILE_NO *) ld_ptr->fl_list;
    for (fl_lc = ld_ptr->fl_cnt; --fl_lc >= 0; ++fl_ptr)
    {
        if (!(task->file_table[*fl_ptr].ft_flags & TEMPORARY))
            ++task->kept_locks[*fl_ptr];
    }

    return (task->db_status);
}


/* ======================================================================
    Free record lock
*/
int INTERNAL_FCN drecfree(int rec, DB_TASK *task, int dbn)
{
    RECORD_ENTRY      *rec_ptr;
    struct lock_descr *ld_ptr;

    if (nrec_check(rec, &rec, &rec_ptr, task) != S_OKAY)
        return (task->db_status);

    if (task->dbopen >= 2)                /* exclusive access needs no locks */
        return (task->db_status);

    ld_ptr = &task->rec_locks[rec];

    if (task->trans_id[0])
        return (dberr(S_TRFREE));

    if (ld_ptr->fl_type == DB_TEXT('f'))
        return (dberr(S_NOTLOCKED));

    free_files(ld_ptr, task);
    ld_ptr->fl_type = DB_TEXT('f');

    return task->db_status;
}


/* ======================================================================
    Free set lock
*/
int INTERNAL_FCN dsetfree(int set, DB_TASK *task, int dbn)
{
    SET_ENTRY         *set_ptr;
    struct lock_descr *ld_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    if (task->dbopen >= 2)              /* exclusive access needs no locks */
        return (task->db_status);

    ld_ptr = &task->set_locks[set];

    if (task->trans_id[0])
        return (dberr(S_TRFREE));

    if (ld_ptr->fl_type == DB_TEXT('f'))
        return (dberr(S_NOTLOCKED));

    free_files(ld_ptr, task);
    ld_ptr->fl_type = DB_TEXT('f');

    return task->db_status;
}


/* ======================================================================
    Free key lock
*/
int INTERNAL_FCN dkeyfree(long key, DB_TASK *task, int dbn)
{
    int                 fld,
                        rec;
    RECORD_ENTRY       *rec_ptr;
    FIELD_ENTRY        *fld_ptr;
    struct lock_descr  *ld_ptr;

    if (nfld_check(key, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    if (fld_ptr->fd_key == NOKEY)
        return (dberr(S_NOTKEY));

    if (task->dbopen >= 2)              /* exclusive access needs no locks */
        return (task->db_status);

    ld_ptr = &task->key_locks[fld_ptr->fd_keyno];
    if (task->trans_id[0])
        return (dberr(S_TRFREE));

    if (ld_ptr->fl_type == DB_TEXT('f'))
        return (dberr(S_NOTLOCKED));

    free_files(ld_ptr, task);
    ld_ptr->fl_type = DB_TEXT('f');

    return task->db_status;
}


/* ======================================================================
    Free read-locked files associated with record or set
*/
static int INTERNAL_FCN free_files(
    struct lock_descr *ld_ptr,
    DB_TASK           *task)
{
    register int      fl_lc;                 /* loop control */
    FILE_NO           fno;
    DB_LOCKREQ       *lockreq_ptr;
    int              *appl_ptr;
    FILE_NO           fref;
    register FILE_NO *fl_ptr;
    short             flushed = FALSE;


    /* fill free packet */
    task->lock_pkt->nfiles = task->free_pkt->nfiles = 0;
    fl_ptr = (FILE_NO *) ld_ptr->fl_list;
    for (fl_lc = ld_ptr->fl_cnt; --fl_lc >= 0; ++fl_ptr)
    {
        fno = *fl_ptr;
        appl_ptr = &task->app_locks[fno];
        fref = task->file_refs[fno];
        if (fref == -1)
            continue;      /* file is TEMPORARY */

        if (ld_ptr->fl_type == DB_TEXT('r') && *appl_ptr > 0)
        {
            /* free read lock */
            if (--*appl_ptr == 0 && task->excl_locks[fno] == 0)
            {
                task->free_pkt->frefs[task->free_pkt->nfiles++] = fref;
                STAT_lock(fno, STAT_FREE_r, task);

                /* reset key scan position */
                if (task->file_table[fno].ft_type == DB_TEXT('k'))
                    key_reset(fno, task);
            }
        }
        else if (--task->excl_locks[fno] == 0)
        {
            if (!flushed)
            {
                flushed = TRUE;
                dio_flush(task);
            }

            /* free exclusive access lock */
            if (*appl_ptr > 0)
            {
                /* downgrade to read-lock */
                lockreq_ptr = &task->lock_pkt->locks[task->lock_pkt->nfiles++];
                lockreq_ptr->type = DB_TEXT('r');
                lockreq_ptr->fref = fref;
                STAT_lock(fno, STAT_LOCK_x2r, task);
            }
            else
            {
                /* free excl-lock */
                task->free_pkt->frefs[task->free_pkt->nfiles++] = fref;
                STAT_lock(fno, STAT_FREE_x, task);
            
                /* reset key scan position */
                if (task->file_table[fno].ft_type == DB_TEXT('k'))
                    key_reset(fno, task);
            }
        }

        if (ld_ptr->fl_kept)
        {
            /* Remove hold on lock */
            if (--task->kept_locks[fno] < 0)
                return (dberr(S_BADLOCKS));
            ld_ptr->fl_kept = FALSE;
        }
    }

#if 0
    if (task->lockMgrComm == LMC_GENERAL || task->lockMgrComm == LMC_TCP ||
            task->lockMgrComm == LMC_QNX)
    {
        /* close files for downgrades */

        FILE_NO        fno;
        unsigned short pno;

        if (task->lock_pkt->nfiles > 0)
        {
            for (fno = 0; fno < task->size_ft; ++fno)
            {
                for (pno = 0; pno < task->lock_pkt->nfiles; ++pno)
                {
                    if (task->file_refs[fno] == task->lock_pkt->locks[pno].fref)
                    {
                        dio_close(fno, task);
                        break;
                    }
                }
            }
        }
    }
#endif

    /* send any downgrades */
    if (send_lock_pkt(NULL,task) == S_OKAY)
    {
        /* free any files */
        send_free_pkt(task);
    }

    return task->db_status;
}


