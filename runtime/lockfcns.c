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
    Reset lock descriptor tables
*/
void INTERNAL_FCN reset_locks(DB_TASK *task)
{
    int beg, end;
    register int i;
    register struct lock_descr *ld_ptr;

    /* reset record lock descriptors */
    beg = 0;
    end = task->size_rt;
    for (i = beg, ld_ptr = &task->rec_locks[i]; i < end; ++i, ++ld_ptr)
    {
        if (ld_ptr->fl_kept)
        {
            ld_ptr->fl_type = 'r';
            ld_ptr->fl_kept = FALSE;
        }
        else if (ld_ptr->fl_type != 'x')
            ld_ptr->fl_type = 'f';
    }
    
    /* reset set lock descriptors */
    beg = 0;
    end = task->size_st;
    for (i = beg, ld_ptr = &task->set_locks[i]; i < end; ++i, ++ld_ptr)
    {
        if (ld_ptr->fl_kept)
        {
            ld_ptr->fl_type = 'r';
            ld_ptr->fl_kept = FALSE;
        }
        else if (ld_ptr->fl_type != 'x')
            ld_ptr->fl_type = 'f';
    }

    /* reset key lock descriptors */
    beg = 0;
    end = task->keyl_cnt;
    for (i = beg, ld_ptr = &task->key_locks[i]; i < end; ++i, ++ld_ptr)
    {
        if (ld_ptr->fl_kept)
        {
            ld_ptr->fl_type = 'r';
            ld_ptr->fl_kept = FALSE;
        }
        else if (ld_ptr->fl_type != 'x')
            ld_ptr->fl_type = 'f';
    }
}

/* ======================================================================
    Send free files packet
*/
int INTERNAL_FCN send_free_pkt(DB_TASK *task)
{
    int      stat;
    size_t   send_size;
    LM_FREE *free_pkt = task->free_pkt;

    /* send any free packets */
    if (free_pkt->nfiles)
    {
        send_size = sizeof(LM_FREE) + (free_pkt->nfiles - 1) * sizeof(short);
        if (send_size > task->fp_size)
            return (dberr(SYS_LOCKARRAY));

#if 0
        if (task->lockMgrComm == LMC_GENERAL || task->lockMgrComm == LMC_TCP ||
            task->lockMgrComm == LMC_QNX)
        {
            int fno, pno;

            for (fno=0; fno < task->size_ft; ++fno)
            {
                for (pno=0; pno < free_pkt->nfiles; ++pno)
                {
                    if (task->file_refs[fno] == free_pkt->frefs[pno])
                    {
                        dio_close(fno, task);
                        break;
                    }
                }
            }
        }
#endif

        stat = msg_trans(L_FREE, free_pkt, send_size, NULL, NULL, task);
        if (stat != S_OKAY)
            return task->db_status;
    }

#ifdef DB_TRACE
    if (task->db_trace & TRACE_LOCKS)
    {
        short     ii;
        short    *frefs;
        FILE_NO   fno;
        DB_TCHAR *fname;

        for (ii = free_pkt->nfiles, frefs = free_pkt->frefs; ii--; ++frefs)
        {
            for (fno = 0; fno < task->size_ft; fno++)
            {
                if (*frefs == task->file_refs[fno])
                    break;
            }

            fname = psp_truename(task->file_table[fno].ft_name, 0);
            db_printf(task, DB_TEXT("freed lock for file \"%s\"\n"), fname);
            psp_freeMemory(fname, 0);
        }
    }
#endif

    return (task->db_status);
}

/* ======================================================================
    Send lock request
*/
int INTERNAL_FCN send_lock_pkt(int *timestamps, DB_TASK *task)
{
    size_t   send_size;
    size_t   recv_size;
    LR_LOCK *recv_pkt = NULL;
    LM_LOCK *lock_pkt;
    int     *pTo;
    int     *pFrom;
    int      stamps_to_copy;

    lock_pkt = task->lock_pkt;
    if (lock_pkt->nfiles)
    {
        /* send lock request */
        send_size = sizeof(LM_LOCK) + (lock_pkt->nfiles - 1) *
        sizeof(DB_LOCKREQ);

        if (send_size > task->lp_size)
        {
            dberr(SYS_LOCKARRAY);
            goto err_exit;
        }

        lock_pkt->read_lock_secs = task->readlocksecs;
        lock_pkt->write_lock_secs = task->writelocksecs;

        if (msg_trans(L_LOCK, lock_pkt, send_size, (void **) &recv_pkt,
                &recv_size, task) != S_OKAY)
            goto err_exit;

        if (timestamps && lock_pkt->nfiles)
        {
            stamps_to_copy = lock_pkt->nfiles;
            pTo = timestamps;
            pFrom = &recv_pkt->timestamps[0];
            while (stamps_to_copy--)
                *pTo++ = *pFrom++;
        }
    }

#ifdef DB_TRACE
    if (task->db_trace & TRACE_LOCKS)
    {
        FILE_NO     fno;
        short       ii;
        DB_TCHAR   *fname;
        DB_LOCKREQ *p;

        for (ii = lock_pkt->nfiles, p = lock_pkt->locks; ii--; p++)
        {
            for (fno = 0; fno < task->size_ft; fno++)
            {
                if (p->fref == task->file_refs[fno])
                    break;
            }

            fname = psp_truename(task->file_table[fno].ft_name, 0);
            db_printf(task, DB_TEXT("'%c' lock granted for file \"%s\"\n"),
                    p->type, fname);
            psp_freeMemory(fname, 0);
        }
    }
#endif

err_exit:
    psp_lmcFree(recv_pkt);
    return task->db_status;
}

/**************************************************************************/

int INTERNAL_FCN msg_trans(int mtype, void *send_pkt, size_t send_size,
                           void **recv_pkt, size_t *recv_size, DB_TASK *task)
{
    int         stat;
    int         status;
    int         access_taf;
    void       *lmc = task->lmc;
    LR_RECOVER *recv;

    do
    {
        stat = psp_lmcTrans(mtype, send_pkt, send_size, recv_pkt, recv_size,
               &status, lmc);
        if (stat != S_OKAY)
            return (task->db_status = S_LMCERROR);

        switch (status)
        {
            case L_OKAY:
                if (mtype == L_DBOPEN && ((LR_DBOPEN *) *recv_pkt)->nusers == 1)
                {
                    stat = psp_lmcTrans(recovery_check(task) == S_OKAY ?
                            L_RECDONE : L_RECFAIL, NULL, 0, NULL, NULL, NULL,
                            lmc);
                }

                break;

            case L_RECOVER:
                dbautorec(task);   /* notify user of recovery */
                access_taf = !(psp_lmcFlags(lmc) & PSP_FLAG_NO_TAF_ACCESS);

                /* perform auto-recovery */
                if (access_taf)
                {
                    if ((stat = taf_access(DEL_LOGFILE, task)) != S_OKAY)
                        return (task->db_status = stat);
                }

                recv = (LR_RECOVER *) *recv_pkt;
                stat = drecover(recv->logfile, task);
                if (access_taf)
                {
                    if (stat != S_OKAY)
                        taf_del(recv->logfile, task);

                    taf_release(0, task);
                }

                stat = psp_lmcTrans(stat == S_OKAY ? L_RECDONE : L_RECFAIL,
                       NULL, 0, NULL, NULL, NULL, lmc);
                break;

            case L_QUEUEFULL:
                psp_sleep(task->dbwait_time * 1000);
                break;

            case L_BADTAF:
                dberr(stat = S_TAFSYNC);
                break;

            case L_UNAVAIL:
            case L_TIMEOUT:
                stat = S_UNAVAIL;
                break;

            default:    /* unknown error */
/*                lmc->err = recv->status; */
                dberr(stat = S_LMCERROR);
                break;
        }

    } while (status == L_RECOVER || status == L_QUEUEFULL);

    return task->db_status = stat;
}


