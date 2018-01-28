/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database lock manager                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

#include "lm.h"

/* ======================================================================
   Lock all lockable files.
*/
void lock_all(
    LMCB *lmcb)
{
    int       user;
    int       ii;
    short     locktype;
    short     lockstat;
    int       qind;
    int       qfree;
    int       qn;
    USERTAB  *ut;
    FILETAB  *ft;
    QUEUETAB *qt;
    QUEUETAB *qprev;
    QUEUETAB *qnext;

    /* This function enforces an ordering on the lock requests.  A group
       lock request is always granted from lowest file reference number to
       highest.  For each group request, the lowest numbered un-granted
       request must be granted before the next one.  This will allow
       deadlock-free locking if each application follows the convention of
       making one lock request for all files used within a transaction.
     
       Deadlocks will still be possible if multiple group lock requests are
       made within the same transaction.  */
    DEBUG1(DB_TEXT("lock_all()\n"));

    /* for each user */
    for (ut = lmcb->usertab, user = 0; user < lmcb->numusers; user++, ut++) {
        /* user not deleted? */
        if (!ut->u_name[0])
            continue;

        /* pending requests? */
        if (ut->u_pending == 0)
            continue;

        /* for each pending request */
        for (ft = lmcb->filetab, ii = 0; ii < lmcb->numfiles; ii++, ft++) {
            if (!(ii % BITS_PER_BYTE) && !ut->u_req[ii / BITS_PER_BYTE]) {
                ii += BITS_PER_BYTE - 1;
                continue;
            }

            if (!is_bit_set(ut->u_req, ii))
                continue;

            /* This file is requested by this user.  Is this user at the
               head of the file's queue?  */
            if ((qind = ft->f_queue) == -1)
                continue;               /* already granted */

            qt = &lmcb->queuetab[qind];
            if (qt->q_user != user) {
                /* if this user does not already have the lock on this
                   file, this is the end of the road */
                if (!is_bit_set(ut->u_lock, ii))
                    goto nextuser;

                /* this is a request bit for a file that is already locked */
                continue;
            }
            else {
                /* this user is the first on the file's queue */
                locktype = qt->q_locktype;
                lockstat = ft->f_lockstat;

                /* was this an upgrade request? */
                if (locktype == 'W' || locktype == 'X')
                    locktype = (short) tolower(locktype);

                /* if this request is for a lock that is already held only
                   by this user */
                if ((lockstat == 'w' || lockstat == 'x') &&
                        is_bit_set(ut->u_lock, ii)) {
                    /* Give the user the lock he already has the access to.
                       This may be a downgrade to read.  */
                    grant_lock(lmcb, user, ii, locktype);
                }
                /* else if the the file is free, read locked, or Record
                   locked, and the first queued request is a read lock */
                else if ((lockstat == 'f' || lockstat == 'r' ||
                        lockstat == 'R') && locktype == 'r') {
                    /* give this lock to the waiting user */
                    grant_lock(lmcb, user, ii, 'r');

                    /* Scan the LRQ for other 'r' lock req [591] */
                    qprev = qt;
                    qn = qt->q_next;
                    while (qn != -1) {
                        qnext = &lmcb->queuetab[qn];
                        /* Stop when we encounter a non-read lock req */
                        if (qt->q_locktype != 'r')
                            break;

                        if (qt->q_user > user) {
                            qprev = qnext;
                            qn = qnext->q_next;
                            continue;
                        }

                        /* Here's another read lock we can grant */
                        grant_lock(lmcb, qnext->q_user, ii, 'r');

                        /* Remove this request from queue */
                        qprev->q_next = qnext->q_next;
                        qfree = qn;
                        qn = qnext->q_next;
                        free_q(lmcb, qfree);
                    }
                }
                /* else if the file is free or read locked, and the first
                   queued request is a Record lock */
                else if ((lockstat == 'f' || lockstat == 'r') &&
                        locktype == 'R') {
                    /* grant the Record lock */
                    grant_lock(lmcb, user, ii, locktype);
                }
                /* else if the file is free, and the first queued request
                   is a write or exclusive lock */
                else if (lockstat == 'f' && (locktype == 'w' ||
                        locktype == 'x')) {
                    /* grant the write lock */
                    grant_lock(lmcb, user, ii, locktype);
                }
                /* else if the file is read locked, and the first queued
                   request is a write or exclusive lock */
                else if (lockstat == 'r' && (locktype == 'w' ||
                        locktype == 'x')) {
                    int set;

                    /* see if this is the only holder of the read lock */
                    set = is_bit_set(ft->f_lock, user);
                    if (set)
                        bit_clr(ft->f_lock, user);

                    if (is_map_zero(ft->f_lock, lmcb->userbmbytes))
                        grant_lock(lmcb, user, ii, locktype);
                    else {
                        /* not lockable */
                        if (set)
                            bit_set(ft->f_lock, user);

                        goto nextuser;
                    }
                }
                /* else not lockable */
                else
                    goto nextuser;

                /* discard this queue entry */
                ft->f_queue = qt->q_next;
                free_q(lmcb, qind);
            }
        }
nextuser:
        continue;
    }
}


/* ======================================================================
   Grant a lock request to one user
*/
void grant_lock(
    LMCB *lmcb,
    int   user,
    int   file,
    short type)
{
    USERTAB *ut = &lmcb->usertab[user];
    FILETAB *ft = &lmcb->filetab[file];

    DEBUG4(DB_TEXT("\tgrant_lock(user=%d, file=%d, type=%c)\n"), user, file,
            type)
    /* set the corresponding bits */
    bit_set(ft->f_lock, user);
    bit_set(ut->u_lock, file);

    /* set the lock status */
    if (ft->f_lockstat != 'R')
        ft->f_lockstat = type;

    /* update the lock request status for this user:
       u_pending == 0 && u_waiting > 0 indicates that this user is now
       ready to get its reply
    */
    ut->u_pending--;
    ut->u_waiting++;
}


/* ======================================================================
   Free one file lock
*/
void freelock(
    LMCB *lmcb,
    int   user,
    int   file)
{
    USERTAB *ut = &lmcb->usertab[user];
    FILETAB *ft = &lmcb->filetab[file];

    DEBUG3(DB_TEXT("\tfreelock(user=%d, file=%d)\n"), user, file)

    /* remove the file and user lock bits */
    bit_clr(ft->f_lock, user);
    bit_clr(ut->u_lock, file);

    /* if this is a Record locked file */
    if (ft->f_lockstat == 'R')
    {
        /* if there are now no read locks, unlock this file */
        if (is_map_zero(ft->f_lock, lmcb->userbmbytes))
        {
            lmcb->locks_freed++;
            ft->f_lockstat = 'f';
        }
        else
            ft->f_lockstat = 'r';
    }
    else
    {
        /* if there are now no locks, unlock this file */
        if (is_map_zero(ft->f_lock, lmcb->userbmbytes))
        {
            lmcb->locks_freed++;
            ft->f_lockstat = 'f';
        }
    }
}


/* ======================================================================
   Close one file
*/
void close_file(
    LMCB *lmcb,
    int   user,
    int   file)
{
    USERTAB *ut = &lmcb->usertab[user];
    FILETAB *ft = &lmcb->filetab[file];

    DEBUG3(DB_TEXT("\tclose_file(user=%d, file=%d)\n"), user, file)

    /* remove the bits between file and user */
    bit_clr(ft->f_open, user);
    bit_clr(ut->u_open, file);

    /* if this was the last user holding the file open, remove the entry */
    if (is_map_zero(ft->f_open, lmcb->userbmbytes))
    {
        fileid_del(lmcb->fidtab, ft->f_index);
        ft->f_index = -1;
        memset(ft->f_open, 0, lmcb->userbmbytes);
        memset(ft->f_lock, 0, lmcb->userbmbytes);

        adj_files(lmcb);

        DEBUG2(DB_TEXT("\t\talso removing %d from filetab\n"), file)
    }
}


/* ======================================================================
   Close a user entry
*/
void close_user(
    LMCB *lmcb,
    int   user)
{
    int      taf;
    USERTAB *ut = &lmcb->usertab[user];
    TAFTAB  *tt;

    DEBUG2(DB_TEXT("close_user(user=%d)\n"), user)

    if (ut->u_conn != NULL) {
        (*lmcb->discon)(ut->u_conn);
        ut->u_conn = NULL;
    }

    /* If the user still has files open, then the name is to remain in the
       table, because he is supposed to come back later, re-open the files
       and free them then. */
    if (!is_map_zero(ut->u_open, lmcb->filebmbytes)) {
        ut->u_status = U_HOLDING_X;
        return;
    }

    DEBUG2(DB_TEXT("\tremoving user %d\n"), user);

    ut->u_status = U_EMPTY;
    if (ut->u_pending)
        free_pending(lmcb, user);

    if ((taf = ut->u_taf) != -1) {
        tt = &lmcb->taftab[taf];
        if (--tt->t_nusers == 0) {
            fileid_del(lmcb->fidtab, tt->t_index);
            tt->t_index = -1;
            tt->t_status = REC_OKAY;
            tt->t_recuser = -1;
        }
    }

    ut->u_name[0] = 0;
    ut->u_taf = -1;

    /* If this is the last user, decrease the list size */
    adj_users(lmcb);
    adj_tafs(lmcb);
}

/* ======================================================================
   Mark a user entry as dead
*/
void dead_user(
    LMCB *lmcb,
    int   user,
    int   kill)
{
    int      ii;
    int      excl;
    int      dead_user;
    USERTAB *ut = &lmcb->usertab[user];
    USERTAB *dt;
    FILETAB *ft;
    TAFTAB  *tt = NULL;

    if (user == -1) {
        DEBUG1(DB_TEXT("dead_user(-1)\n"))
        return;
    }

    DEBUG2(DB_TEXT("dead_user(user=%s)\n"), ut->u_name)
    DEBUG2(DB_TEXT("\tuser = %d\n"), user)

    excl = 0;
    for (ft = lmcb->filetab, ii = 0; ii < lmcb->numfiles; ii++, ft++) {
        if (is_bit_set(ut->u_lock, ii)) {
            if (toupper(ft->f_lockstat) == 'R')
                freelock(lmcb, user, ii);

            if (ft->f_lockstat == 'x') {
                if (kill)
                    freelock(lmcb, user, ii);
                else
                    excl = 1;
            }

            if (ft->f_lockstat == 'f')
                close_file(lmcb, user, ii);
        }
    }

    free_pending(lmcb, user);

    DEBUG2(DB_TEXT("\tafter lock checks, excl = %d\n"), excl);
    DEBUG3(DB_TEXT("\t\tlogfile = '%s', ucount = %d\n"),
            fileid_get(lmcb->fidtab, ut->u_logfile), usercount(lmcb));

    if (ut->u_conn != NULL) {
        (*lmcb->discon)(ut->u_conn);
        ut->u_conn = NULL;
    }

    /* if there is a log file, and other users to do recovery */
    if (ut->u_taf != -1)
        tt = &lmcb->taftab[ut->u_taf];

    if (ut->u_logfile != -1 && (usercount(lmcb) > 1 || excl)) {
        ut->u_status = U_DEAD;
        if (ut->u_taf != -1) {
            tt->t_status = REC_NEEDED;
            tt->t_recuser = user;
        }

        DEBUG1(DB_TEXT("\tUser died during commit, marked as needing recovery\n"));
    }
    else {
        if (ut->u_status == U_RECOVERING) {
            DEBUG3(DB_TEXT("\tUser died during recovery: user = %d, recuser = %d\n"),
                    user, ut->u_recuser);

            /* transfer the log file back to the original dead user */
            if ((dead_user = ut->u_recuser) != user) {
                ft = lmcb->filetab;
                for (ii = 0; ii < lmcb->numfiles; ii++, ft++) {
                    if (!is_bit_set(ut->u_open, ii))
                        continue;

                    if (is_bit_set(ut->u_lock, ii)) {
                        if (ft->f_lockstat != 'x') {
                            freelock(lmcb, user, ii);
                            close_file(lmcb, user, ii);
                        }
                    }
                    else
                        close_file(lmcb, user, ii);
                }
                close_user(lmcb, user);
            }

            dt = &lmcb->usertab[dead_user];
            dt->u_status = U_DEAD;
            if (dt->u_taf != -1) {
                lmcb->taftab[dt->u_taf].t_status = REC_NEEDED;
                lmcb->taftab[dt->u_taf].t_recuser = dead_user;
            }
        }
        else {
            DEBUG2(DB_TEXT("\tUser died in non-critical state=%d\n"),
                    ut->u_status);
            clear_user(lmcb, user);
            /* If this is an exclusive file lock holder who is quitting for
               the day, mark it so that it can be picked up again */
            if (excl)
                ut->u_status = U_HOLDING_X;

            DEBUG2(DB_TEXT("\tUsers new state = %d\n"), ut->u_status);

            /* Having cleared out a dead-user, see if locks can be granted */
            lock_all(lmcb);
        }
    }
}


#if 0
/* ======================================================================
   Mark as dead the user entry corresponding to specified connection
*/
void dead_connection(LMCB *lmcb, int conn)
{
   int i;

#ifdef DB_DEBUG
   lm_printf(DB_TEXT("\nDead User detected, connection %d\n"), conn);
#endif

   for (i = 0; i < maxusers; i++)
   {
      if (usertab[i].u_conn == conn)
      {
         dead_user(i, 0);
         return;
      }
   }
}
#endif


/* ======================================================================
   Clear out a dead user's write-locked files
*/
void clear_user(
    LMCB *lmcb,
    int   user)
{
    int      ii;
    USERTAB *ut = &lmcb->usertab[user];
    FILETAB *ft;

    DEBUG2(DB_TEXT("clear_user(user = %d)\n"), user);

    /* free all the write locks and close the files */
    for (ft = lmcb->filetab, ii = 0; ii < lmcb->numfiles; ii++, ft++) {
        if (!is_bit_set(ut->u_open, ii))
            continue;

        if (is_bit_set(ut->u_lock, ii)) {
            if (ft->f_lockstat != 'x') {
                freelock(lmcb, user, ii);
                close_file(lmcb, user, ii);
            }
        }
        else
            close_file(lmcb, user, ii);
    }
 
    close_user(lmcb, user);
}


/* ======================================================================
   Decrease the size of the user table as far as possible
*/
void adj_users(
    LMCB *lmcb)
{
    /* while the last entry is empty, keep removing it */
    while (lmcb->numusers > 0 && !lmcb->usertab[lmcb->numusers - 1].u_name[0])
        lmcb->numusers--;

    /* If there is only one user, and he is dead, eliminate the entry */
    if (usercount(lmcb) == 0 && lmcb->numusers > 0)
        clear_user(lmcb, lmcb->numusers - 1);
}


/* ======================================================================
   Decrease the size of the taf table as far as possible
*/
void adj_tafs(
    LMCB *lmcb)
{
    /* while the last entry is empty */
    while(lmcb->numtafs > 0 && lmcb->taftab[lmcb->numtafs - 1].t_index == -1)
        lmcb->numtafs--;
}


/* ======================================================================
   Count the number of LIVE users
*/
int usercount(
    LMCB *lmcb)
{
    int      ii;
    int      count;
    USERTAB *ut;

    /* An entry that is recovering itself is considered live because the
       first user to login may need to recover itself (when exclusive locks
       exist, and recovery is required).  This will allow external recovery
       to be performed on the first user, even though the entry has been
       preserved in the user table.  */
    ut = lmcb->usertab;
    for (ii = 0, count = 0; ii < lmcb->numusers; ii++, ut++) {
        if (*ut->u_name && (ut->u_status == U_LIVE ||
                ut->u_status == U_REC_MYSELF || ut->u_status == U_RECOVERING))
            count++;
    }

    return count;
}


/* ======================================================================
   Decrease the size of the file table as far as possible
*/
void adj_files(
    LMCB *lmcb)
{
    while (lmcb->numfiles > 0 &&
           lmcb->filetab[lmcb->numfiles - 1].f_index == -1)
       lmcb->numfiles--;
}


void send_lock(
    LMCB *lmcb,
    int   user,
    int   status)
{
    int      ii;
    int     *p;
    size_t   size;
    size_t   locked_files = 0;
    LR_LOCK  *msg;
    FILETAB  *ft;

    for (ft = lmcb->filetab, ii = 0; ii < lmcb->numfiles; ii++) {
        if (is_bit_set(ft->f_lock, user))
            locked_files++;
    }

    if (!locked_files)
        reply_user(lmcb, user, NULL, 0, status);
    else {
        size = sizeof(LR_LOCK) + sizeof(int) * (locked_files - 1);
        msg = psp_lmcAlloc(size);
        msg->ntimestamps = locked_files;
        p = msg->timestamps;
        for (ft = lmcb->filetab, ii = 0; ii < lmcb->numfiles; ii++, ft++) {
            if (is_bit_set(ft->f_lock, user))
                *p++ = ft->f_timestamp;
        }

        reply_user(lmcb, user, msg, size, status);
    }
}

/* ======================================================================
   lock_reply()
 
   Reply to all current lock requests that have timedout or a free
   has occurred granting the lock requests in the queue.
*/
void lock_reply(
    LMCB *lmcb)
{
    int      ii;
    int      repeat = 0;
    USERTAB *ut;

    do {
        for (ut = lmcb->usertab, ii = 0; ii < lmcb->numusers; ii++, ut++) {
            /* if this user is not alive, don't check anything */
            if (!ut->u_name[0])
                continue;

            /* if this user is ready */
            if (!ut->u_pending && ut->u_waiting) {
                DEBUG2(DB_TEXT("\treplying to user # %d\n"), ii);

                memset(ut->u_req, 0, lmcb->filebmbytes);
                lmcb->locks_granted += ut->u_waiting;
                ut->u_timer = 0;
                ut->u_waiting = 0;
                send_lock(lmcb, ii, L_OKAY);
                continue;
            }

            if (ut->u_pending && ut->u_timeout >= 0 && ut->u_timer == 0) {
                lmcb->locks_timedout += free_partial(lmcb, ii);
                lmcb->locks_timedout += free_pending(lmcb, ii);
                memset(ut->u_req, 0, lmcb->filebmbytes);
                reply_user(lmcb, ii, NULL, 0, L_TIMEOUT);
                repeat++;
                continue;
            }
        }

        if (repeat) {
            DEBUG2(DB_TEXT("\trepeat = %d\n"), repeat);
            lock_all(lmcb);
        }
    }
    while (repeat--);
}


/* ======================================================================
   Free all file locks that are part of a partial lock request
*/
int free_partial(
    LMCB *lmcb,
    int   user)
{
    int      ii;
    int      qind;
    int      freed = 0;
    short    locktype;
    USERTAB *ut = &lmcb->usertab[user];
    FILETAB *ft;

    /* for each file in the file table */
    for (ft = lmcb->filetab, ii = 0; ii < lmcb->numfiles; ii++, ft++) {
        /* if the file was both requested and locked */
        if (is_bit_set(ut->u_lock, ii) && is_bit_set(ut->u_req, ii)) {
            /* Make sure that the queued request is not for an upgrade by
               searching through the request queue (if any).  If this user
               has a pending request that is an upgrade request, then the
               original lock is pre-existing, and not to be freed at this
               time.  */
            qind = ft->f_queue;
            while (qind != -1 && lmcb->queuetab[qind].q_user != user)
                qind = lmcb->queuetab[qind].q_next;

            if (qind != -1)
                locktype = lmcb->queuetab[qind].q_locktype;
            else
                locktype = 'a';            /* fool it */
   
            /* if not an upgrade request */
            if (locktype != 'W' && locktype != 'X')
            {
                /* free this file */
                freelock(lmcb, user, ii);
   
                freed++;
                ut->u_waiting--;
            }
        }
    }

    return freed;
}


/* ======================================================================
   Find and delete request queue entries for this user
*/
int free_pending(
    LMCB *lmcb,
    int   user)
{
    int       ii;
    int       qind;
    int       qprev;
    int       freed = 0;
    USERTAB  *ut = &lmcb->usertab[user];
    FILETAB  *ft;
    QUEUETAB *qt = lmcb->queuetab;

    /* for all files, while there are more pending requests */
    for (ft = lmcb->filetab, ii = 0; ii < lmcb->numfiles; ii++, ft++) {
        /* if the file is requested */
        if (is_bit_set(ut->u_req, ii))
        {
            /* search through the request queue for this user's entry */
            qind = ft->f_queue;
            qprev = -1;
            while (qind != -1) {
                if (qt[qind].q_user == user) {
                    /* this is the one to delete */

                    /* if is the first one on the queue */
                    if (qprev == -1) {
                        /* it was the first on the queue */
                        ft->f_queue = qt[qind].q_next;
                    }
                    else
                    {
                        /* point the previous to the next */
                        qt[qprev].q_next = qt[qind].q_next;
                    }
     
                    /* free this entry */
                    free_q(lmcb, qind);
                 
                    freed++;
                    if (--ut->u_pending == 0)
                        return freed;

                    break;
                }

                qprev = qind;
                qind = qt[qind].q_next;
            }
        }
    }

    return freed;
}


/* ====================================================================== */

/* return TRUE if map is all zeros */
int is_map_zero(
    DB_BYTE *map,
    size_t   len)
{
    while (len--) {
        if (*map++ != '\0')
            return 0;
    }

    return 1;
}

int count_bits(
    DB_BYTE *map,
    int      max)
{
    int ii;
    int temp;

    for (ii = 0, temp = 0; ii < max; ii++) {
        if (is_bit_set(map, ii))
            temp++;
    }

    return temp;
}

int first_bit_set(
    DB_BYTE *map,
    int      max)
{
    int ii = 0;

    while (ii < max && !is_bit_set(map, ii))
        ii++;

    return ii;
}


/* ======================================================================
   Get an available request queue entry
*/
int get_q(
    LMCB *lmcb)
{
    int qind;

    if (lmcb->qHead == -1)
        return -1;

    qind = lmcb->qHead;
    lmcb->qHead = lmcb->queuetab[qind].q_next;

    return qind;
}


/* ======================================================================
   Return a queue entry to the empty list
*/
void free_q(
    LMCB *lmcb,
    int   entry)
{
    lmcb->queuetab[entry].q_next = lmcb->qHead;
    lmcb->qHead = entry;
}


/* ======================================================================
   Check for room in the request queue
*/
int room_q(
    LMCB *lmcb,
    int   req)
{
    int       entry = lmcb->qHead;
    QUEUETAB *qt = lmcb->queuetab;

    while (req--) {
        if (entry == -1)
            return 0;

        entry = qt[entry].q_next;
    }

    return 1;
}


/* ======================================================================
   Place a queue entry at the head of a file's queue
*/
void ins_head(
    LMCB *lmcb,
    int   file,
    int   qind)
{
    FILETAB *ft = &lmcb->filetab[file];

    lmcb->queuetab[qind].q_next = ft->f_queue;
    ft->f_queue = qind;
}


/* ======================================================================
   Place a queue entry at the end of a file's queue
*/
void ins_tail(
    LMCB *lmcb,
    int   file,
    int   qind)
{
    int      q;
    FILETAB  *ft = &lmcb->filetab[file];
    QUEUETAB *qt = lmcb->queuetab;

    /* if this file does not have an existing queue */
    if (ft->f_queue == -1)
    {
        /* put this entry at the head */
        ft->f_queue = qind;
    }
    else
    {
        /* find the end of the queue */
        q = ft->f_queue;
        while (qt[q].q_next != -1)
            q = qt[q].q_next;

        /* point this last one to the new one */
        qt[q].q_next = qind;
    }

    qt[qind].q_next = -1;
}


/* ======================================================================
   Place a Record lock entry at the head of a file's queue
*/
void ins_Record(
    LMCB *lmcb,
    int   file,
    int   qind)
{
    int       q;
    int       prev = 0;
    FILETAB  *ft = &lmcb->filetab[file];
    QUEUETAB *qt = lmcb->queuetab;

    /* if this file does not have an existing queue */
    if ((q = ft->f_queue) == -1) {
        /* put this entry at the head */
        ft->f_queue = qind;
        qt[qind].q_next = -1;
    }
    else
    {
         if (qt[q].q_locktype == 'R') {
              /* find the last 'R' request in the queue */
              while (q != -1 && qt[q].q_locktype == 'R') {
                  prev = q;
                  q = qt[q].q_next;
              }

              if (q == -1) {
                  qt[prev].q_next = qind;
                  qt[qind].q_next = -1;
              }
              else {
                  qt[qind].q_next = qt[prev].q_next;
                  qt[prev].q_next = qind;
              }
         }
         else {
              qt[qind].q_next = ft->f_queue;
              ft->f_queue = qind;
         }
    }
}

