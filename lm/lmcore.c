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

#include "lm.h"

static void l_dbopen(LMCB *, int, LM_DBOPEN *);
static void l_dbclose(LMCB *, int, LM_DBCLOSE *);
static void l_lock(LMCB *, int, LM_LOCK *);
static void l_free(LMCB *, int, LM_FREE *);
static void l_trcommit(LMCB *, int, LM_TRCOMMIT *);
static void l_trend(LMCB *, int, void *);
static void l_settime(LMCB *, int, LM_SETTIME *);
static void l_recdone(LMCB *, int, void *);
static void l_recfail(LMCB *, int, void *);
static void l_login(LMCB *, int, LM_LOGIN *);
static void l_logout(LMCB *, int, void *);
static void l_terminate(LMCB *, int, void *);
static void l_options(LMCB *, int, LM_OPTIONS *);
static void l_verifyuser(LMCB *, int, void *);
static void l_clearuser(LMCB *, int, LM_USERID *);
static void l_status(LMCB *, int, LM_STATUS *);

typedef struct {
    LM_FCN *fcn;
    int     user_check;
} LM_FCNS;

static LM_FCNS fcns[] = {
    { (LM_FCN *) NULL,           0 },
    { (LM_FCN *) &l_dbopen,      1 },
    { (LM_FCN *) &l_dbclose,     1 },
    { (LM_FCN *) &l_lock,        1 },
    { (LM_FCN *) &l_free,        1 },
    { (LM_FCN *) &l_trcommit,    1 },
    { (LM_FCN *) &l_trend,       1 },
    { (LM_FCN *) &l_settime,     1 },
    { (LM_FCN *) &l_recdone,     1 },
    { (LM_FCN *) &l_recfail,     1 },
    { (LM_FCN *) &l_login,       0 },
    { (LM_FCN *) &l_logout,      1 },
    { (LM_FCN *) &l_terminate,   0 },
    { (LM_FCN *) &l_options,     1 },
    { (LM_FCN *) &l_verifyuser,  0 },
    { (LM_FCN *) &l_clearuser,   0 },
    { (LM_FCN *) &l_status,      0 }
};

static DB_TCHAR *get_filename(LMCB *, int, short);
static void      l_status_genstat(LMCB *, int);
static void      l_status_tables(LMCB *, int, TABSIZE *);
static void      l_status_userinfo(LMCB *, int, TABSIZE *, int);
static void      l_status_fileinfo(LMCB *, int, TABSIZE *, int);
static void      l_status_userstat(LMCB *, int, DB_TCHAR *);
static void      l_status_taftab(LMCB *, int, TABSIZE *);


static int term_flag = 0;

int lm_init(
    LM            *lm,
    short          maxusers,
    short          maxfiles,
    short          maxqueues,
    LM_REPLY_FCN  *send,
    LM_DISCON_FCN *discon,
    LM_INFO_FCN   *info)
{
    short     ii;
    int       maxids;
    size_t    userbmbytes;
    size_t    filebmbytes;
    size_t    bitmapsize;
    LMCB     *lmcb;
    USERTAB  *ut;
    FILETAB  *ft;
    TAFTAB   *tt;
    QUEUETAB *qt;
    DB_BYTE  *b;

    if (!maxusers || !maxfiles || !maxqueues || !send || !discon || !info)
        return LM_INVPARM;

    userbmbytes = (maxusers - 1) / BITS_PER_BYTE + 1;
    filebmbytes = (maxfiles - 1) / BITS_PER_BYTE + 1;
    bitmapsize = userbmbytes * 2 * maxfiles + filebmbytes * 3 * maxusers;
    if ((lmcb = psp_cGetMemory(sizeof(LMCB), 0)) == NULL)
        return LM_NOMEMORY;

    lmcb->maxusers  = maxusers;
    lmcb->maxfiles  = maxfiles;
    lmcb->maxqueues = maxqueues;

    lmcb->filebmbytes = filebmbytes;
    lmcb->userbmbytes = userbmbytes;

    lmcb->usertab  = psp_cGetMemory(sizeof(USERTAB) * maxusers, 0);
    lmcb->filetab  = psp_cGetMemory(sizeof(FILETAB) * maxfiles, 0);
    lmcb->taftab   = psp_cGetMemory(sizeof(TAFTAB) * maxusers, 0);
    lmcb->queuetab = psp_cGetMemory(sizeof(QUEUETAB) * maxqueues, 0);
    lmcb->bitmaps  = psp_cGetMemory(bitmapsize, 0);

    maxids = maxusers + maxfiles + maxqueues;
    if (fileid_init(&lmcb->fidtab, maxids, 20 * maxids) != L_OKAY ||
            !lmcb->usertab || !lmcb->filetab || !lmcb->taftab ||
            !lmcb->queuetab || !lmcb->bitmaps) {
        if (lmcb->usertab)
            psp_freeMemory(lmcb->usertab, 0);

        if (lmcb->filetab)
            psp_freeMemory(lmcb->filetab, 0);

        if (lmcb->taftab)
            psp_freeMemory(lmcb->taftab, 0);

        if (lmcb->queuetab)
            psp_freeMemory(lmcb->queuetab, 0);

        if (lmcb->bitmaps)
            psp_freeMemory(lmcb->bitmaps, 0);

        psp_freeMemory(lmcb, 0);
        return LM_NOMEMORY;
    }

    lmcb->send = send;
    lmcb->discon = discon;
    lmcb->info = info;

    ut = lmcb->usertab;
    tt = lmcb->taftab;
    b = lmcb->bitmaps;
    for (ii = 0; ii < maxusers; ii++, ut++, tt++) {
        ut->u_taf = -1;
        ut->u_open = b;
        ut->u_lock = b + filebmbytes;
        ut->u_req  = b + 2 * filebmbytes;
        ut->u_timeout = DEFAULT_TIMEOUT;
        ut->u_recuser = -1;
        ut->u_logfile = -1;
        b += 3 * filebmbytes;

        tt->t_index = -1;
        tt->t_recuser = -1;
    }

    for (ft = lmcb->filetab, ii = 0; ii < maxfiles; ii++, ft++) {
        ft->f_index = -1;
        ft->f_lockstat = 'f';
        ft->f_open = b;
        ft->f_lock = b + userbmbytes;
        ft->f_queue = -1;
        b += 2 * userbmbytes;
    }

    for (qt = lmcb->queuetab, ii = 0; ii < maxqueues - 1; ii++, qt++) {
        qt->q_locktype = 'f';
        qt->q_user = -1;
        qt->q_next = ii + 1;
    }

    qt->q_next = -1;

    *lm = (LM) lmcb;
    
    return LM_OKAY;
}

void lm_term(
    LM lm)
{
    LMCB *lmcb = (LMCB *) lm;

    fileid_deinit(lmcb->fidtab);

    psp_freeMemory(lmcb->queuetab, 0);
    psp_freeMemory(lmcb->taftab, 0);
    psp_freeMemory(lmcb->filetab, 0);
    psp_freeMemory(lmcb->usertab, 0);
    psp_freeMemory(lmcb->bitmaps, 0);
    psp_freeMemory(lmcb, 0);
}

int lm_crank(
    LM lm)
{
    short    ii;
    LMCB    *lmcb = (LMCB *) lm;
    USERTAB *ut;
    long     curr = psp_timeMilliSecs();

    for (ut = lmcb->usertab, ii = 0; ii < lmcb->numusers; ii++, ut++) {
        if (!ut->u_name[0])
            continue;

        if (ut->u_pending && ut->u_timeout > 0 && ut->u_timer != 0) {
            if ((curr - ut->u_timer) / 1000 >= ut->u_timeout)
                ut->u_timer = 0; /* timeout */
        }
    }

    lock_reply(lmcb);

    return LM_OKAY;
}

int lm_process(
    LM     lm,
    int    user,
    int    conn,
    int    type,
    void  *msg)
{
    LMCB *lmcb = (LMCB *) lm;

    if (type < 0 || type > L_LAST) {
        psp_lmcFree(msg);
        return LM_INVTYPE;
    }

    if (fcns[type].user_check && (user < 0 || user > lmcb->maxusers ||
            lmcb->usertab[user].u_status == U_EMPTY)) {
        psp_lmcFree(msg);
        return LM_INVUSER;
    }

    if (type)
        (*fcns[type].fcn)(lmcb, fcns[type].user_check ? user : conn, msg);

    if (term_flag)
        return LM_TERMINATE;

    return lm_crank(lm);
}

int lm_dead_user(
    LM  lm,
    int user)
{
    LMCB *lmcb = (LMCB *) lm;

    dead_user(lm, user, 0);

    return LM_OKAY;
}

int lm_find_user(
    LM    lm,
    void *conn)
{
    short    ii;
    LMCB    *lmcb = (LMCB *) lm;
    USERTAB *ut;

    for (ut = lmcb->usertab, ii = 0; ii < lmcb->numusers; ii++, ut++) {
        if (ut->u_conn == conn)
            return ii;
    }

    return -1;
}


void reply_user(
    LMCB  *lmcb,
    int    user,
    void  *msg,
    size_t size,
    int    status)
{
    USERTAB *ut = &lmcb->usertab[user];

    if ((*lmcb->send)(ut->u_conn, msg, size, status) != LM_OKAY)
        dead_user(lmcb, user, 0);

    if (msg)
        psp_lmcFree(msg);
}

void reply_non_user(
    LMCB  *lmcb,
    int    conn,
    void  *msg,
    size_t size,
    int    status)
{
    (*lmcb->send)((void *) conn, msg, size, status);
    if (msg)
        psp_lmcFree(msg);
}

static void l_login(
    LMCB     *lmcb,
    int       user,
    LM_LOGIN *msg)
{
    short     ii;
    short     taf;
    int       fid;
    DB_TCHAR *userid;
    USERTAB  *ut;
    TAFTAB   *tt = NULL;

#if 0  /* TBD */
    if (msg->version != (MSGVER | UNICODE_FLAG)) {
        DEBUG1(DB_TEXT("\tInvalid message version\n"))
        goto unavail;
    }
#endif

    DEBUG3(DB_TEXT("l_login(dbuserid = %s, taffile = %s)\n"), msg->dbuserid, msg->taffile)
    taf = -1;
    if ((fid = fileid_fnd(lmcb->fidtab, msg->taffile)) != -1) {
        tt = lmcb->taftab;
        for (ii = 0; ii < lmcb->numtafs; ii++, tt++) {
            if (fid == tt->t_index) {
                taf = ii;
                break;
            }
        }
    }

    if (taf != -1 && tt->t_status == REC_EXTERNAL) {
        DEBUG1(DB_TEXT("\tUnavailable due to external recover in progress\n"))
        goto unavail;
    }

    userid = msg->dbuserid;
    ut = lmcb->usertab;
    for (ii = 0; ii < lmcb->numusers; ii++, ut++) {
        if (vtstrcmp(ut->u_name, userid) == 0) {
            psp_lmcFree(msg);
            switch (ut->u_status) {
                /* This happens when the same userid is used by two different
                   clients.  It is an error and the second user is denied
                   access. */
                case U_LIVE:
                    reply_non_user(lmcb, user, NULL, 0, L_DUPUSER);
                    break;

                /* The user died during a previous session, so we check for a
                   recovery in progress.  If not, we assign the recovery to
                   this user for him to do. */
                case U_DEAD:
                    ut->u_conn = (void *) user;
                    reply_user(lmcb, user, NULL, 0, L_OKAY);
                    if (ut->u_logfile != -1) {
                        ut->u_status = U_REC_MYSELF;
                        ut->u_recuser = ii;
                    }

                    if (ut->u_taf != -1) {
                        lmcb->taftab[ut->u_taf].t_status = REC_NEEDED;
                        lmcb->taftab[ut->u_taf].t_recuser = ii;
                    }

                    break;

                /* since this user's files are being recovered by another
                   user this login must be denied.  If this user held
                   exclusive locks then he must be able to regain the same
                   usertab entry, and cannot until the recover is complete.
                   If this user did not hold exclusive locks, he must wait
                   for the recovery to complete because he might destroy the
                   logfile while it is being used for recovery. */
                case U_BEING_REC:
                    DEBUG1(DB_TEXT("\tUnavailable because user is being recovered\n"))
                    goto unavail;

                /* The same user is logging in after having died holding
                   exclusive locks; give him the same entry */
                case U_HOLDING_X:
                    ut->u_conn = (void *) user;
                    reply_user(lmcb, user, NULL, 0, L_OKAY);
                    ut->u_status = U_LIVE;
                    break;

                default:
                    break;
            }

            return;
        }     
    }

    /* The user name does not already exist in the user table so add him */
    ut = lmcb->usertab;
    for (ii = 0; ii < lmcb->numusers; ii++, ut++) {
        if (*ut->u_name == '\0')
            break;
    }

    if (ii == lmcb->numusers) {
        if (lmcb->numusers == lmcb->maxusers) {
            DEBUG1(DB_TEXT("!\tuser table is full\n"))
            goto unavail;
        }

        ii = lmcb->numusers++;
    }

    if (taf == -1) {
        if (lmcb->numtafs == lmcb->maxusers) {
            DEBUG1(DB_TEXT("!\ttaf table is full\n"))

            if (ii == lmcb->numusers - 1) /* did we just increment numusers */
                lmcb->numusers--;

            goto unavail;
        }

        taf = lmcb->numtafs++;
        tt = &lmcb->taftab[taf];
    }

    if (tt->t_nusers++ == 0) {
        if ((fid = fileid_add(lmcb->fidtab, msg->taffile)) == -1) {
            tt->t_nusers = 0;
            if (taf == lmcb->numtafs - 1)
                lmcb->numtafs--;

            if (ii == lmcb->numusers - 1)
                lmcb->numusers--;

            DEBUG1(DB_TEXT("\tout of memory for fileid table\n"))
            goto unavail;
        }

        tt->t_index = fid;
        tt->t_status = REC_EXTERNAL;
        tt->t_recuser = ii;
    }

    vtstrcpy(ut->u_name, msg->dbuserid);
    ut->u_taf = taf;
    ut->u_conn = (void *) user;
    memset(ut->u_open, 0, lmcb->filebmbytes);
    memset(ut->u_lock, 0, lmcb->filebmbytes);
    memset(ut->u_req, 0, lmcb->filebmbytes);
    ut->u_pending = 0;
    ut->u_waiting = 0;
    ut->u_timeout = DEFAULT_TIMEOUT;
    ut->u_timer   = 0;
    ut->u_status  = U_LIVE;
    ut->u_recuser = -1;
    ut->u_logfile = -1;

    psp_lmcFree(msg);

    reply_user(lmcb, ii, NULL, 0, L_OKAY);
    DEBUG5(DB_TEXT("Adding user:\n\tuser name: %s\n")
            DB_TEXT("\tuser table entry %hd\n\ttaf table entry: %hd\n")
            DB_TEXT("\tconnection: %08X\n"), ut->u_name, ii, taf, user)


    return;

unavail:
    psp_lmcFree(msg);
    reply_non_user(lmcb, user, NULL, 0, L_UNAVAIL);
}

static void l_logout(
    LMCB  *lmcb,
    int    user,
    void  *msg)
{
    USERTAB *ut = &lmcb->usertab[user];

    reply_user(lmcb, user, NULL, 0, L_OKAY);
    close_user(lmcb, user);
}

static void l_terminate(
    LMCB  *lmcb,
    int    user,
    void  *msg)
{
    DEBUG2(DB_TEXT("l_terminate(user = %d)\n"), user);

    reply_non_user(lmcb, user, NULL, 0, L_OKAY);
    term_flag = 1;
}

static void l_clearuser(
    LMCB      *lmcb,
    int        user,
    LM_USERID *msg)
{
    short    ii;
    USERTAB *ut = lmcb->usertab;

    DEBUG2(DB_TEXT("l_clearuser(user = %d)\n"), user)

    for (ii = 0; ii < lmcb->numusers; ii++, ut++) {
        if (vtstrcmp(ut->u_name, msg->dbuserid) == 0) {
            dead_user(lmcb, ii, 1);
            break;
        }
    }

    reply_non_user(lmcb, user, NULL, 0, L_OKAY);
}

static int chk_recover(
    LMCB *lmcb,
    int   user,
    int   type)
{
    int         taf;
    short       recstat;
    USERTAB    *ut = &lmcb->usertab[user];
    USERTAB    *rt;
    TAFTAB     *tt;
    LR_RECOVER *recover;

    if ((taf = ut->u_taf) == -1) {
        reply_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return 1;
    }

    /* Check to see if recover needs to be done for somebody or if recover is
       already in progress */
    tt = &lmcb->taftab[taf];
    if ((recstat = tt->t_status) == REC_NEEDED) {
        tt->t_status = REC_PENDING;
        rt = &lmcb->usertab[ut->u_recuser = tt->t_recuser];
        rt->u_status = U_BEING_REC;

        recover = psp_lmcAlloc(sizeof(LR_RECOVER));
        vtstrcpy(recover->logfile, fileid_get(lmcb->fidtab, rt->u_logfile));
        reply_user(lmcb, user, recover, sizeof(LR_RECOVER), L_RECOVER);
        return 1;
    }

    if (recstat == REC_PENDING || (recstat == REC_EXTERNAL &&
            (type != L_DBOPEN || tt->t_recuser != user))) {
        reply_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return 1;
    }

    return 0;
}

static void l_dbopen(
    LMCB      *lmcb,
    int        user,
    LM_DBOPEN *msg)
{
    short       ii;
    short       jj;
    long        kk;
    short       new = 0;
    short       avail;
    short       count;
    short       open_slots;
    size_t      resp_size;
    USERTAB    *ut = &lmcb->usertab[user];
    FILETAB    *ft;
    LR_DBOPEN  *response;
    DB_TCHAR   *names;
    DB_TCHAR    filename[FILENMLEN];
    DB_TCHAR    lastname[FILENMLEN];

    DEBUG4(DB_TEXT("l_dbopen(user = %d, type = %c, nfiles = %d)\n"), user,
            msg->type, msg->nfiles)
    DEBUG2(DB_TEXT("\tnumfiles = %d\n"), lmcb->numfiles)

    if (msg->nfiles > lmcb->maxfiles)
        goto unavail;

    if (chk_recover(lmcb, user, L_DBOPEN)) {
        psp_lmcFree(msg);
        return;
    }

    /* first, verify that 1) if this is an exclusive open then none of the
       files are already open and 2) if this is a shared open the none of the
       files are already exclusively opened. */
    filename[0] = lastname[0] = 0;
    for (names = msg->fnames, ii = 0; ii < msg->nfiles; ii++) {
        count = (short) *names++;
        vtstrcpy(filename, lastname);
        vtstrcpy(filename + count, names);
        vtstrcpy(lastname, filename);
        names += vtstrlen(names) + 1;

        if (vtstrlen(filename) == 0)
            goto unavail;

        if ((kk = fileid_fnd(lmcb->fidtab, filename)) == -1) {
            new++;
            continue;
        }

        for (ft = lmcb->filetab, jj = 0; jj < lmcb->numfiles; jj++, ft++) {
            if (kk != ft->f_index)
                continue;

            if (msg->type == 'x' || ft->f_lockstat == 'X')
                goto unavail;

            if (ft->f_taf != ut->u_taf) {
                psp_lmcFree(msg);
                reply_user(lmcb, user, NULL, 0, L_BADTAF);
                return;
            }

            break;
        }
    }

    DEBUG2(DB_TEXT("\tnew files = %hd\n"), new);

    ft = lmcb->filetab;
    for (open_slots = 0, ii = 0; ii < lmcb->numfiles; ii++, ft++) {
        if (ft->f_index == -1)
            open_slots++;
    }

    open_slots += lmcb->maxfiles - lmcb->numfiles;
    if (new > open_slots)
        goto unavail;

    resp_size = sizeof(LR_DBOPEN) + (msg->nfiles - 1) * sizeof(short);
    response = psp_lmcAlloc(resp_size);

    /* We now know that all of the files can be added, go ahead and do it. */
    filename[0] = lastname[0] = 0;
    for (names = msg->fnames, ii = 0; ii < msg->nfiles; ii++) {
        count = (short) *names++;
        vtstrcpy(filename, lastname);
        vtstrcpy(filename + count, names);
        vtstrcpy(lastname, filename);
        names += vtstrlen(names) + 1;

        if ((kk = fileid_add(lmcb->fidtab, filename)) == -1)
            goto unavail;

        avail = -1;
        for (ft = lmcb->filetab, jj = 0; jj < lmcb->numfiles; jj++, ft++) {
            if (kk == ft->f_index)
                break;
            else if (avail == -1 && ft->f_index == -1)
                avail = jj;
        }

        if (jj == lmcb->numfiles) {
            if (avail != -1)
                jj = avail;
            else
                lmcb->numfiles++;

            ft = &lmcb->filetab[jj];
            ft->f_index = kk;
            ft->f_taf = ut->u_taf;

            ft->f_lockstat = msg->type == 'x' ? 'X' : 'f';
            memset(ft->f_open, 0, lmcb->userbmbytes);
            memset(ft->f_lock, 0, lmcb->userbmbytes);
            DEBUG3(DB_TEXT("\tnew entry # %d, %s\n"), jj, filename)
        }
        else
            DEBUG3(DB_TEXT("\told entry # %d, %s\n"), jj, filename);

        response->frefs[ii] = jj;
        bit_set(ft->f_open, user);
        bit_set(ut->u_open, jj);
    }

    psp_lmcFree(msg);
    response->nusers = lmcb->taftab[ut->u_taf].t_nusers;
    reply_user(lmcb, user, response, resp_size, L_OKAY);

    return;

unavail:
    psp_lmcFree(msg);
    reply_user(lmcb, user, NULL, 0, L_UNAVAIL);
}

static void l_dbclose(
    LMCB       *lmcb,
    int         user,
    LM_DBCLOSE *msg)
{
    short    ii;
    short   *fref;
    USERTAB *ut = &lmcb->usertab[user];
    FILETAB *ft = lmcb->filetab;

    DEBUG3(DB_TEXT("l_dbclose(user = %d, nfiles = %hd\n"), user, msg->nfiles)

    fref = msg->frefs;
    for (ii = 0; ii < msg->nfiles; ii++, fref++) {
        if (is_bit_set(ut->u_lock, *fref) && ft[*fref].f_lockstat == 'x')
            continue;

        freelock(lmcb, user, *fref);
        close_file(lmcb, user, *fref);
    }

    psp_lmcFree(msg);
    reply_user(lmcb, user, NULL, 0, L_OKAY);
}

static void l_lock(
    LMCB    *lmcb,
    int      user,
    LM_LOCK *msg)
{
    int         qptr;
    short       ii;
    short       locktype;
    DB_LOCKREQ *reqs;
    USERTAB    *ut = &lmcb->usertab[user];
    FILETAB    *ft;
    QUEUETAB   *qt;

    DEBUG3(DB_TEXT("l_lock(user = %d, nfiles = %hd\n"), user, msg->nfiles)

    if (chk_recover(lmcb, user, L_LOCK)) {
        psp_lmcFree(msg);
        return;
    }

    /* make sure there is room for these requests on the queue */
    if (!room_q(lmcb, msg->nfiles)) {
        lmcb->locks_rejected += msg->nfiles;
        psp_lmcFree(msg);
        reply_user(lmcb, user, NULL, 0, L_QUEUEFULL);
        return;
    }

    /* Make sure all the file reference numbers are valid and that the request
       won't be rejected because of an upgrade conflict */
    reqs = msg->locks;
    for (ii = 0; ii < msg->nfiles; ii++, reqs++) {
        if (reqs->fref < 0 || reqs->fref > lmcb->numfiles) {
            lmcb->locks_rejected += msg->nfiles;
            psp_lmcFree(msg);
            reply_user(lmcb, user, NULL, 0, L_SYSERR);
            return;
        }

        /* is this file already locked by this user? */
        ft = &lmcb->filetab[reqs->fref];
        if (!is_bit_set(ft->f_lock, user))
            continue;

        /* is this an upgrade request? */
        if (ft->f_lockstat != 'r' || reqs->type == 'r' || reqs->type == 'R')
            continue;

        /* This is an upgrade request.  Has an upgrade request already been
           queued up by someone else?  (Upgrade requests are capital letters */
        if (ft->f_queue == -1)
            continue;

        locktype = lmcb->queuetab[ft->f_queue].q_locktype;
        if (locktype != 'W' && locktype != 'X')
            continue;

        /* Our worst fears.  This upgrade request cannot be granted.  This
           whole lock request must be rejected. */
        DEBUG1(DB_TEXT("\tCannot grant upgrade request\n"))
        lmcb->locks_rejected += msg->nfiles;
        psp_lmcFree(msg);
        reply_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return;
    }

    /* set the # of files that must be locked before the user can proceed */
    ut->u_pending = msg->nfiles;

    reqs = msg->locks;
    for (ii = 0; ii < msg->nfiles; ii++, reqs++) {
        ft = &lmcb->filetab[reqs->fref];
        DEBUG3(DB_TEXT("\treference # %d for '%c'"), reqs->fref, reqs->type)
        
        /* set the 'requested' bit */
        bit_set(ut->u_req, reqs->fref);

        /* If this is an upgrade request */
        if (is_bit_set(ft->f_lock, user) && ft->f_lockstat == 'r' &&
                reqs->type != 'r' && reqs->type != 'R') {
            /* place request at head of the queue */
            qt = &lmcb->queuetab[qptr = get_q(lmcb)];
            qt->q_locktype = (short) toupper(reqs->type);
            qt->q_user = user;
            ins_head(lmcb, reqs->fref, qptr);
            DEBUG1(DB_TEXT("\tupgrade request queued\n"))
        }
        /* else if this is a downgrade request */
        else if (is_bit_set(ft->f_lock, user) && (ft->f_lockstat == 'w' ||
                ft->f_lockstat == 'R') && reqs->type == 'r') {
            /* it can be granted immediately */
            ft->f_lockstat = 'r';
            bit_set(ut->u_lock, reqs->fref);
            bit_set(ft->f_lock, user);

            --ut->u_pending;
            ++ut->u_waiting;
            DEBUG1(DB_TEXT("\tdowngrade request granted immediately\n"))
        }
        else {
            /* get a queue entry ready */
            qt = &lmcb->queuetab[qptr = get_q(lmcb)];
            qt->q_locktype = reqs->type;
            qt->q_user = user;
            /* if this is a Record lock request */
            if (reqs->type == 'R') {
                /* place this request at the head of the queue, but
                   following any existing 'R' requests */
                ins_Record(lmcb, reqs->fref, qptr);
                DEBUG1(DB_TEXT("\tRecord lock request queued\n"))
            }
            else {
                /* else place this request at the end of the file's queue */
                ins_tail(lmcb, reqs->fref, qptr);
                DEBUG1(DB_TEXT("\tlock request queued\n"))
            }
        }
    }

    psp_lmcFree(msg);

    /* lock whatever is lockable */
    lock_all(lmcb);

    /* start the clock */
    if (ut->u_timeout > 0) {
        while ((ut->u_timer = psp_timeMilliSecs()) == 0)
            ;
    }

    lock_reply(lmcb);
}

static void l_free(
    LMCB    *lmcb,
    int      user,
    LM_FREE *msg)
{
    short  ii;
    short *file;

    DEBUG3(DB_TEXT("l_free(user = %d, nfiles = %d)\n"), user, msg->nfiles)

    for (file = msg->frefs, ii = 0; ii < msg->nfiles; ii++, file++)
        freelock(lmcb, user, *file);

    psp_lmcFree(msg);

    /* lock any lockable files */
    lock_all(lmcb);

    reply_user(lmcb, user, NULL, 0, L_OKAY);

     /* grant any locks that are now available */
    lock_reply(lmcb);
}

static void l_trcommit(
    LMCB        *lmcb,
    int          user,
    LM_TRCOMMIT *msg)
{
    long     fid;
    USERTAB *ut = &lmcb->usertab[user];

    DEBUG3(DB_TEXT("l_trcommit(user=%d, logfile=%s\n"), user, msg->logfile)

    fid = fileid_add(lmcb->fidtab, msg->logfile);
    psp_lmcFree(msg);
    if (fid == -1) {
        psp_lmcFree(msg);
        reply_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return;
    }

    ut->u_logfile = fid;

    /* If the lock manager times out a user while file writes are taking place
       it can grant simultaneous rights to write to the file to another user.
       Do not allow timeouts to take place once transaction commit begins. */
    ut->u_readlocksecs = 0;
    ut->u_writelocksecs = 0;

    reply_user(lmcb, user, NULL, 0, L_OKAY);
}

static void l_trend(
    LMCB  *lmcb,
    int    user,
    void  *msg)
{
    short    ii;
    USERTAB *ut = &lmcb->usertab[user];
    FILETAB *ft;

    DEBUG2(DB_TEXT("l_trend(user=%d\n"), user)

    /* Increment the timestamp for every file with an outstanding write
       lock for this user */
    for (ft = lmcb->filetab, ii = 0; ii < lmcb->numfiles; ii++, ft++) {
        if (is_bit_set(ft->f_lock, user)) {
            switch (ft->f_lockstat) {
                case 'w':
                case 'W':
                case 'x':
                case 'X':
                    ft->f_timestamp++;
                    break;

                default:
                    break;
            }
        }
    }

    fileid_del(lmcb->fidtab, ut->u_logfile);
    ut->u_logfile = -1;
    reply_user(lmcb, user, NULL, 0, L_OKAY);
}

static void l_settime(
    LMCB       *lmcb,
    int         user,
    LM_SETTIME *msg)
{
    USERTAB *ut = &lmcb->usertab[user];

    DEBUG2(DB_TEXT("l_settime(user = %d)\n"), user)
    lmcb->usertab[user].u_timeout = msg->queue_timeout;
    psp_lmcFree(msg);
    reply_user(lmcb, user, NULL, 0, L_OKAY);
}

static void l_recdone(
    LMCB  *lmcb,
    int    user,
    void  *msg)
{
    short    ii;
    short    excl;
    short    dead_id;
    USERTAB *ut = &lmcb->usertab[user];
    TAFTAB  *tt = &lmcb->taftab[ut->u_taf];
    FILETAB *ft;

    DEBUG2(DB_TEXT("l_recdone(user = %d)\n"), user)

    if (tt->t_status != REC_EXTERNAL) {
        /* This user has just completed recovering the files of a dead user.
           Now the files held by the dead user may be freed and closed. */
        if ((dead_id = ut->u_recuser) != -1) {
            ft = lmcb->filetab;
            for (excl = 0, ii = 0; ii < lmcb->numfiles; ii++, ft++) {
                if (!is_bit_set(ut->u_open, ii))
                    continue;

                if (ft->f_lockstat == 'x')
                    excl = 1;
                else {
                    if (ft->f_lockstat != 'f')
                        freelock(lmcb, dead_id, ii);

                    close_file(lmcb, dead_id, ii);
                }
            }

            /* change this entry to a normal status */
            ut->u_status = U_LIVE;
            ut->u_recuser = -1;
            ut->u_logfile = -1;
        }

        /* close out the dead user entry */
        if (dead_id != -1 && dead_id != user) {
            if (excl) {
                lmcb->usertab[dead_id].u_status = U_HOLDING_X;
                lmcb->usertab[dead_id].u_logfile = -1;
            }
            else
                close_user(lmcb, dead_id);
        }
    }

    tt->t_status = REC_OKAY;
    reply_user(lmcb, user, NULL, 0, L_OKAY);
}

static void l_recfail(
    LMCB  *lmcb,
    int    user,
    void  *msg)
{
    USERTAB *ut = &lmcb->usertab[user];

    DEBUG2(DB_TEXT("l_recfail(user = %d)\n"), user)

    lmcb->taftab[ut->u_taf].t_status = REC_NEEDED;
    reply_user(lmcb, user, NULL, 0, L_OKAY);
}

static void l_options(
    LMCB       *lmcb,
    int         user,
    LM_OPTIONS *msg)
{
}

static void l_verifyuser(
    LMCB  *lmcb,
    int    user,
    void  *msg)
{
    DEBUG2(DB_TEXT("l_verifyuser(user = %d)\n"), user)

    reply_non_user(lmcb, user, NULL, 0, L_OKAY);
}

static void l_status(
    LMCB      *lmcb,
    int        user,
    LM_STATUS *msg)
{
    DEBUG3(DB_TEXT("l_status(user = %d, type = %d)\n"), user,
            msg->type_of_status)

    switch (msg->type_of_status) {
        case ST_GENSTAT:
            l_status_genstat(lmcb, user);
            break;

        case ST_TABLES:
            l_status_tables(lmcb, user, &msg->tabsize);
            break;

        case ST_USERINFO:
            l_status_userinfo(lmcb, user, &msg->tabsize, msg->number);
            break;

        case ST_FILEINFO:
            l_status_fileinfo(lmcb, user, &msg->tabsize, msg->number);
            break;

        case ST_USERSTAT:
            l_status_userstat(lmcb, user, msg->username);
            break;

        case ST_TAFTAB:
            l_status_taftab(lmcb, user, &msg->tabsize);
            break;

        default:
            reply_non_user(lmcb, user, NULL, 0, L_UNAVAIL);
            break;
    }

    psp_lmcFree(msg);
}

static void l_status_genstat(
    LMCB *lmcb,
    int   user)
{
    LR_STATUS *msg;

    msg = psp_lmcAlloc(sizeof(LR_STATUS));
    if (msg == NULL) {
        reply_non_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return;
    }
    
    msg->s_tabsize.lm_numusers = lmcb->numusers;
    msg->s_tabsize.lm_numfiles = lmcb->numfiles;
    msg->s_tabsize.lm_maxusers = lmcb->maxusers;
    msg->s_tabsize.lm_maxfiles = lmcb->maxfiles;
    msg->s_tabsize.lm_maxqueue = lmcb->maxqueues;
    msg->s_tabsize.lm_maxtafs  = lmcb->maxusers;
    msg->s_tabsize.lm_numtafs  = lmcb->numtafs;

    reply_non_user(lmcb, user, msg, sizeof(LR_STATUS), L_OKAY);
}

static void l_status_tables(
    LMCB    *lmcb,
    int      user,
    TABSIZE *tabsize)
{
    short       ii;
    short       maxusers;
    long        curr;
    size_t      size;
    LR_TABLES  *msg;
    LR_USERTAB *users;
    LR_FILETAB *files;
    USERTAB    *ut;
    FILETAB    *ft;

    size = sizeof(LR_TABLES) + tabsize->lm_numusers * sizeof(LR_USERTAB) +
            tabsize->lm_numfiles * sizeof(LR_FILETAB) + tabsize->lm_maxqueue *
            sizeof(QUEUETAB);

    if ((msg = psp_lmcAlloc(size)) == NULL) {
        reply_non_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return;
    }

    memset(msg, 0, size);
    msg->t_msgs_received  = lmcb->requests;
    msg->t_msgs_sent      = lmcb->replys;
    msg->t_locks_granted  = lmcb->locks_granted;
    msg->t_locks_rejected = lmcb->locks_rejected;
    msg->t_locks_timedout = lmcb->locks_timedout;
    msg->t_locks_freed    = lmcb->locks_freed;

    maxusers = lmcb->maxusers;

    curr = psp_timeMilliSecs();
    users = (LR_USERTAB *) (msg + 1);
    ut = lmcb->usertab;
    for (ii = 0; ii < tabsize->lm_numusers; ii++, users++) {
        if (ii < lmcb->numusers) {
            if (ut->u_timer)
                users->ut_timer    = (curr - ut->u_timer) / 1000;
            else
                users->ut_timer    = 0;

            users->ut_pending = ut->u_pending;
            users->ut_status  = ut->u_status;
            users->ut_recover = ut->u_recuser;
            vtstrcpy(users->ut_name, ut->u_name);
            vtstrcpy(users->ut_logfile, get_filename(lmcb, ut->u_logfile, 0));
            ut++;
        }
        else {
            users->ut_status  = U_EMPTY;
            users->ut_recover = -1;
            vtstrcpy(users->ut_name, DB_TEXT("empty"));
        }
    }

    files = (LR_FILETAB *) users;
    ft = lmcb->filetab;
    for (ii = 0; ii < tabsize->lm_numfiles; ii++, files++) {
        if (ii < lmcb->numfiles) {
            files->ft_lockstat       = ft->f_lockstat;
            files->ft_numlocks       = count_bits(ft->f_lock, maxusers);
            files->ft_user_with_lock = first_bit_set(ft->f_lock, maxusers);
            files->ft_queue_entry    = ft->f_queue;
            vtstrcpy(files->ft_name, get_filename(lmcb, ft->f_index, 0));
            ft++;
        }
        else {
            files->ft_lockstat       = DB_TEXT('f');
            files->ft_queue_entry    = -1;
            vtstrcpy(files->ft_name, DB_TEXT("empty"));
        }
    }

    if (tabsize->lm_maxqueue)
        memcpy(files, lmcb->queuetab, sizeof(QUEUETAB) * tabsize->lm_maxqueue);

    reply_non_user(lmcb, user, msg, size, L_OKAY);
}

static void l_status_userinfo(
    LMCB    *lmcb,
    int      user,
    TABSIZE *tabsize,
    int      user_no)
{
    short        ii;
    long         curr = psp_timeMilliSecs();
    size_t       size;
    DB_BYTE     *bm;
    LR_FILEINFO *files;
    LR_USERINFO *msg;
    USERTAB     *ut = &lmcb->usertab[user_no];
    FILETAB     *ft;

    size = sizeof(LR_USERINFO) + lmcb->filebmbytes * 3 + tabsize->lm_numfiles *
            sizeof(LR_FILEINFO) + tabsize->lm_maxqueue * sizeof(QUEUETAB);
    if ((msg = psp_lmcAlloc(size)) == NULL) {
        reply_non_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return;
    }

    memset(msg, 0, size);
    vtstrcpy(msg->ui_name, ut->u_name);
    msg->ui_pending    = ut->u_pending;
    msg->ui_timeout    = ut->u_timeout;
    msg->ui_status     = ut->u_status;
    msg->ui_recovering = ut->u_recuser;
    if (ut->u_timer)
        msg->ui_timer = (curr - ut->u_timer) / 1000;
    else
        msg->ui_timer = 0;

    if (ut->u_taf != -1) {
        vtstrcpy(msg->ui_taffile, get_filename(lmcb,
                lmcb->taftab[ut->u_taf].t_index, 1));
    }

    vtstrcpy(msg->ui_logfile, get_filename(lmcb, ut->u_logfile, 1));
    (*lmcb->info)(ut->u_conn, msg->ui_netinfo1, msg->ui_netinfo2,
            msg->ui_netinfo3);

    bm = (DB_BYTE *) (msg + 1);
    memcpy(bm, ut->u_open, lmcb->filebmbytes * 3);

    files = (LR_FILEINFO *) (bm + 3 * lmcb->filebmbytes);
    ft = lmcb->filetab;
    for (ii = 0; ii < tabsize->lm_numfiles; ii++, files++) { 
        if (ii < lmcb->numfiles) {
            vtstrcpy(files->fi_name, get_filename(lmcb, ft->f_index, 0));
            files->fi_lockstat = ft->f_lockstat;
            files->fi_queue    = ft->f_queue;
            ft++;
        }
        else
            vtstrcpy(files->fi_name, DB_TEXT("empty"));
    }

    if (tabsize->lm_maxqueue)
        memcpy(files, lmcb->queuetab, sizeof(QUEUETAB) * tabsize->lm_maxqueue);

    reply_non_user(lmcb, user, msg, size, L_OKAY);
}

static void l_status_fileinfo(
    LMCB    *lmcb,
    int      user,
    TABSIZE *tabsize,
    int      file_no)
{
    short        ii;
    size_t       size;
    DB_BYTE     *bm;
    DB_TCHAR    *users;
    LR_FILEINFO *msg;
    FILETAB     *ft = &lmcb->filetab[file_no];
    USERTAB     *ut;

    size = sizeof(LR_FILEINFO) + lmcb->userbmbytes * 2 + tabsize->lm_numusers *
            16 + tabsize->lm_maxqueue * sizeof(QUEUETAB);
    if ((msg = psp_lmcAlloc(size)) == NULL) {
        reply_non_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return;
    }

    vtstrcpy(msg->fi_name, get_filename(lmcb, ft->f_index, 1));
    msg->fi_lockstat = ft->f_lockstat;
    msg->fi_queue    = ft->f_queue;

    bm = (DB_BYTE *) (msg + 1);
    memcpy(bm, ft->f_open, lmcb->userbmbytes * 2);

    users = (DB_TCHAR *) (bm + 2 * lmcb->userbmbytes);
    ut = lmcb->usertab;
    for (ii = 0; ii < tabsize->lm_numusers; ii++, users += 16) {
        if (ii < lmcb->numusers)
            vtstrncpy(users, (ut++)->u_name, 16);
        else
            vtstrcpy(users, DB_TEXT("empty"));
    }

    if (tabsize->lm_maxqueue)
        memcpy(users, lmcb->queuetab, sizeof(QUEUETAB) * tabsize->lm_maxqueue);

    reply_non_user(lmcb, user, msg, size, L_OKAY);
}

static void l_status_userstat(
    LMCB     *lmcb,
    int       user,
    DB_TCHAR *name)
{
    short    ii;
    short   *msg;
    USERTAB *ut;

    if ((msg = psp_lmcAlloc(sizeof(short))) == NULL) {
        reply_non_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return;
    }

    for (ut = lmcb->usertab, ii = 0; ii < lmcb->numusers; ii++, ut++) {
        if (!vtstrcmp(ut->u_name, name)) {
            *msg = ut->u_status;
            break;
        }
    }


    if (ii == lmcb->numusers)
        *msg = U_EMPTY;

    reply_non_user(lmcb, user, msg, sizeof(short), L_OKAY);
}

static void l_status_taftab(
    LMCB    *lmcb,
    int      user,
    TABSIZE *tabsize)
{
    short      ii;
    size_t     size;
    LR_TAFTAB *msg;
    LR_TAFTAB *tafs;
    TAFTAB    *tt;

    size = sizeof(LR_TAFTAB) * tabsize->lm_numtafs;
    if ((msg = psp_lmcAlloc(size)) == NULL) {
        reply_non_user(lmcb, user, NULL, 0, L_UNAVAIL);
        return;
    }

    tt = lmcb->taftab;
    for (tafs = msg, ii = 0; ii < tabsize->lm_numtafs; ii++, tafs++) {
        if (ii < lmcb->numtafs) {
            vtstrcpy(tafs->tt_name, get_filename(lmcb, tt->t_index, 1));
            tafs->tt_status  = tt->t_status;
            tafs->tt_recuser = tt->t_recuser;
            tafs->tt_nusers  = tt->t_nusers;
            tt++;
        }
        else {
            vtstrcpy(tafs->tt_name, DB_TEXT("empty"));
            tafs->tt_status  = U_EMPTY;
            tafs->tt_recuser = -1;
            tafs->tt_nusers  = 0;
        }
    }

    reply_non_user(lmcb, user, msg, size, L_OKAY);
}

static DB_TCHAR *get_filename(
    LMCB *lmcb,
    int   fileid,
    short with_path)
{
    DB_TCHAR *s1;
    DB_TCHAR *s2;

    if (fileid == -1)
        return DB_TEXT("");

    s1 = fileid_get(lmcb->fidtab, fileid);
    if (with_path || (s2 = vtstrrchr(s1, DIRCHAR)) == NULL)
        return s1;

    return s2 + 1;
}
