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

#include "dblock.h"


/* ======================================================================
    Begin transaction
*/
int INTERNAL_FCN dtrbegin(const DB_TCHAR *tid, DB_TASK *task)
{
    if (!task->dbopen)
        return (dberr(S_DBOPEN));

    if (task->dboptions & READONLY)
        return (dberr(S_READONLY));

    if (!tid || !*tid)
        return (dberr(S_TRANSID));

    if (task->trans_id[0])
        return (dberr(S_TRACTIVE));

    /* changes were possible outside a transaction */
    dio_flush(task);

    if (task->dboptions & TRLOGGING)
    {
        if (o_init(task) != S_OKAY)
            return (task->db_status);
    }

    vtstrncpy(task->trans_id, tid, TRANS_ID_LEN - 1);

    STAT_trbegin(task);

    return (task->db_status);
}

/* ======================================================================
    End transaction
*/
int INTERNAL_FCN dtrend(DB_TASK *task)
{
    DB_LOCKREQ       *lockreq_ptr;
    register FILE_NO *fref_ptr;
    register int     *appl_ptr;
    register int     *keptl_ptr;
    register int     *excl_ptr;
    register FILE_NO  fno;
    DB_TCHAR         *ptr;
    int               mode = 0;

#ifdef MULTI_TAFFILE
    if (task->dboptions & MULTITAF)
        mode = NO_TAFFILE_LOCK;
#endif

    if (!task->trans_id[0])
        return (dberr(S_TRNOTACT));

    if (task->dboptions & TRLOGGING)            /* end of if's condition */
    {
        if (task->trlog_flag)
            dtrmark(task);      /* mark start of trx in archive log file */

        /* flush data to database or overflow */
        if (dio_flush(task) != S_OKAY)
            return (task->db_status);

        /* flush recovery data to overflow file */
        if (o_flush(task) != S_OKAY)
            return (task->db_status);

        if (task->db_lockmgr)
        {
            LM_TRCOMMIT *send_pkt;

            if ((send_pkt = psp_lmcAlloc(sizeof(LM_TRCOMMIT))) == NULL)
                return (dberr(S_NOMEMORY));

            /* send only the logfile name, without path, to lockmgr */
            /* path is stripped off during recovery anyway */
            ptr = vtstrrchr(task->dblog, DIRCHAR);
            if (ptr)
                ++ptr;
            else
                ptr = task->dblog;

            vtstrncpy(send_pkt->logfile, ptr, LOGFILELEN);
            msg_trans(L_TRCOMMIT, send_pkt, sizeof(LM_TRCOMMIT), NULL, NULL,
                    task);
            psp_lmcFree(send_pkt);

            if (task->db_status != S_OKAY)
                return task->db_status;
        }

        task->trcommit = TRUE;
        if (!(psp_lmcFlags(task->lmc) & PSP_FLAG_NO_TAF_ACCESS))
        {
            if (taf_add(task->dblog, task) != S_OKAY)
                return task->db_status;
        }

        /* allow for user interrupt to test recovery */
        if (task->dboptions & TXTEST)
        {
            dberr(S_DEBUG);
            flush_dberr(task);
        }

        if (task->cache_ovfl)
        {
            /* update db from overflow file */
            if (o_update(task) != S_OKAY)
                return (task->db_status);
        }

        /* flush modified cache data to database */
        if (dio_flush(task) != S_OKAY)
            return (task->db_status);

        if (!(psp_lmcFlags(task->lmc) & PSP_FLAG_NO_TAF_ACCESS))
        {
            if (taf_access(DEL_LOGFILE | mode, task) == S_OKAY)
            {
                if (taf_del(task->dblog, task) != S_OKAY)
                     return task->db_status;

                taf_release(mode, task);
            }
            else
                return task->db_status;
        }

        if (task->db_lockmgr)
        {
            if (msg_trans(L_TREND, NULL, 0, NULL, NULL, task) != S_OKAY)
                return task->db_status;
        }

        if (task->trlog_flag)
            dtrbound(task);     /* mark end of trx in archive log file */
    }
    else
    {
        /* End trx without db.*'s logging and/or with NOVELL's */
        if (task->cache_ovfl)   /* not possible if NOVELLTRX */
        {
            /* update db from overflow file */
            if (o_update(task) != S_OKAY)
                return (task->db_status);
        }

        /* flush data to database */
        if (dio_flush(task) != S_OKAY)
            return (task->db_status);

    }

    task->trcommit = FALSE;

    if (task->dbopen == 1)
    {
        /* free unkept, non-exclusive file locks */
        task->lock_pkt->nfiles = task->free_pkt->nfiles = 0;
        for ( fno = 0, fref_ptr = task->file_refs, appl_ptr = task->app_locks,
              keptl_ptr = task->kept_locks, excl_ptr = task->excl_locks;
              fno < task->size_ft;
              ++fno, ++fref_ptr, ++appl_ptr, ++keptl_ptr, ++excl_ptr)
        {
            if (*excl_ptr)
            {
                *appl_ptr = *keptl_ptr;
                STAT_lock(fno, *appl_ptr ? STAT_LOCK_x2r : STAT_FREE_x, task);
            }
            else if (*appl_ptr == -1)
            {
                if ((*appl_ptr = *keptl_ptr) > 0)
                {
                    lockreq_ptr = &task->lock_pkt->locks[task->lock_pkt->nfiles++];
                    lockreq_ptr->type = 'r';
                    lockreq_ptr->fref = *fref_ptr;
                    STAT_lock(fno, STAT_LOCK_w2r, task);
                }
                else
                {
                    task->free_pkt->frefs[task->free_pkt->nfiles++] = *fref_ptr;
                    STAT_lock(fno, STAT_FREE_w, task);
                }
            }
            else if (*appl_ptr && (*appl_ptr = *keptl_ptr) == 0)
            {
                task->free_pkt->frefs[task->free_pkt->nfiles++] = *fref_ptr;
                STAT_lock(fno, STAT_FREE_r, task);
            }

            *keptl_ptr = 0;
        }

        /* send lock downgrade request */
        if (send_lock_pkt(NULL,task) != S_OKAY || send_free_pkt(task) != S_OKAY)
            return (task->db_status);

        reset_locks(task);         /* clear lock descriptors */
        key_reset(task->size_ft, task);   /* reset all key file positions */
    }

    STAT_trend(task);

    memset(task->trans_id, '\0', TRANS_ID_LEN * sizeof(DB_TCHAR));
    task->cache_ovfl = FALSE;
    dio_ixclear(task);
/*    dio_close(task->ov_file, task);   */  /* d_trbegin() requires it be closed */

    return (task->db_status);
}



/* ======================================================================
    Abort transaction
*/
int INTERNAL_FCN dtrabort(DB_TASK *task)
{
    if (!task->trans_id[0])
        return (dberr(S_TRNOTACT));

    if (task->dbopen == 1)
    {
        int i;

        /* Revert any kept locks to unkept status */
        for (i = 0; i < task->size_ft; i++)
            task->kept_locks[i] = 0;
        
        for (i = 0; i < task->size_rt; i++)
            task->rec_locks[i].fl_kept = FALSE;

        for (i = 0; i < task->size_st; i++)
            task->set_locks[i].fl_kept = FALSE;

        for (i = 0; i < task->keyl_cnt; i++)
            task->key_locks[i].fl_kept = FALSE;
    }

    dio_pzclr(task);
    memset(task->trans_id, '\0', TRANS_ID_LEN);
    task->cache_ovfl = FALSE;
    
    if (task->dbopen == 1)
        free_dblocks(ALL_DBS, task);

    /* clear any changed pages */
    dio_clear(ALL_DBS, task);
    dio_ixclear(task);
    dio_close(task->ov_file, task);      /* d_trbegin() requires it be closed */

    STAT_trabort(task);

    return (task->db_status);
}


