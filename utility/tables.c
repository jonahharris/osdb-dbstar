/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, lock manager console utility                      *
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
#include "console.h"


#define is_bit_set(m, b) (((DB_BYTE *) m)[b / BITS_PER_BYTE] & (1 << ((BITS_PER_BYTE - 1) - (b % BITS_PER_BYTE))) & 0xff)
static int count_bits(DB_BYTE *, int);


typedef struct USER_INFO_S
{
    DB_TCHAR       name[16];
    unsigned short pending;
    unsigned short timeout;
    unsigned short timer;
    unsigned short status;
    int            recover;
    DB_TCHAR       taffile[LOGFILELEN];
    DB_TCHAR       logfile[LOGFILELEN];
} USER_INFO;

typedef struct FILE_INFO_S
{
    DB_TCHAR       name[FILENMLEN];
    unsigned short lockstat;
    unsigned short numlocks;
    int            user_with_lock;
    int            qentry;
} FILE_INFO;

typedef struct
{
    short q_locktype;
    short q_user;
    short q_next;
} QUEUETAB;

static LM_STATUS *ssp = NULL;

static long msgsr = 0;
static long msgss = 0;
static long granted = 0;
static long rejected = 0;
static long timedout = 0;
static long freed = 0;

static TABSIZE cur_sizes;
static TABSIZE max_sizes;
static size_t  max_userbmbytes;
static size_t  max_filebmbytes;

static USER_INFO *users = NULL;
static FILE_INFO *files = NULL;
static DB_TCHAR filename[FILENMLEN];         /* for file detail */
static QUEUETAB *queue = NULL;
static DB_BYTE *user_o = NULL;
static DB_BYTE *user_l = NULL;
static DB_BYTE *user_r = NULL;
static DB_BYTE *file_o = NULL;
static DB_BYTE *file_l = NULL;

static DB_TCHAR *status[] =
{
    DB_TEXT("Empty   "),
    DB_TEXT("Live    "),
    DB_TEXT("Dead    "),
    DB_TEXT("RecOther"),
    DB_TEXT("Recover "),
    DB_TEXT("RecMe   "),
    DB_TEXT("ExLock  ")
};


void free_table_memory()
{
    if (users)
    {
        psp_freeMemory(users, 0);
        users = NULL;
    }
    if (files)
    {
        psp_freeMemory(files, 0);
        files = NULL;
    }
    if (queue)
    {
        psp_freeMemory(queue, 0);
        queue = NULL;
    }
    if (user_o)
    {
        psp_freeMemory(user_o, 0);
        user_o = NULL;
    }
    if (user_l)
    {
        psp_freeMemory(user_l, 0);
        user_l = NULL;
    }
    if (user_r)
    {
        psp_freeMemory(user_r, 0);
        user_r = NULL;
    }
    if (file_o)
    {
        psp_freeMemory(file_o, 0);
        file_o = NULL;
    }
    if (file_l)
    {
        psp_freeMemory(file_l, 0);
        file_l = NULL;
    }
    if (ssp)
    {
        psp_lmcFree(ssp);
        ssp = NULL;
    }
}

static int get_general_status()
{
    int        stat;
    size_t     userbmbytes = 0;
    size_t     filebmbytes = 0;
    LR_STATUS *rsp = NULL;

    if (!ssp)
    {
        if ((ssp = (LM_STATUS *) psp_lmcAlloc(sizeof(LM_STATUS))) == NULL)
        {
            error_msg(DB_TEXT("Out of memory"));
            return -1;
        }
    }

    ssp->type_of_status = ST_GENSTAT;
    stat = psp_lmcTrans(L_STATUS, ssp, sizeof(LM_STATUS), (void **) &rsp,
            NULL, NULL, lmc);

    if (stat != PSP_OKAY)
    {
        error_msg(DB_TEXT("Transmission error"));
        if (rsp)
            psp_lmcFree(rsp);
        return -1;
    }

    memcpy(&cur_sizes, &rsp->s_tabsize, sizeof(TABSIZE));

    if (rsp->s_tabsize.lm_maxusers > max_sizes.lm_maxusers)
    {
        max_sizes.lm_maxusers = 0;
        if (users)
            psp_freeMemory(users, 0);
        if ((users = (USER_INFO *) psp_cGetMemory(
                rsp->s_tabsize.lm_maxusers * sizeof(USER_INFO), 0)) == NULL)
            goto nomem;
        
        max_sizes.lm_maxusers = rsp->s_tabsize.lm_maxusers;
        userbmbytes = (max_sizes.lm_maxusers - 1) / BITS_PER_BYTE + 1;
    }

    if (rsp->s_tabsize.lm_maxfiles > max_sizes.lm_maxfiles)
    {
        max_sizes.lm_maxfiles = 0;
        if (files)
            psp_freeMemory(files, 0);
        if ((files = (FILE_INFO *) psp_cGetMemory(
                rsp->s_tabsize.lm_maxfiles * sizeof(FILE_INFO), 0)) == NULL)
            goto nomem;
        
        max_sizes.lm_maxfiles = rsp->s_tabsize.lm_maxfiles;
        filebmbytes = (max_sizes.lm_maxfiles - 1) / BITS_PER_BYTE + 1;
    }

    if (userbmbytes > max_userbmbytes)
    {
        max_userbmbytes = 0;
        if (file_o)
            psp_freeMemory(file_o, 0);
        if ((file_o = (DB_BYTE *) psp_cGetMemory(userbmbytes, 0)) == NULL)
            goto nomem;

        if (file_l)
            psp_freeMemory(file_l, 0);
        if ((file_l = (DB_BYTE *) psp_cGetMemory(userbmbytes, 0)) == NULL)
            goto nomem;

        max_userbmbytes = userbmbytes;
    }

    if (filebmbytes > max_filebmbytes)
    {
        max_filebmbytes = 0;
        if (user_o)
            psp_freeMemory(user_o, 0);
        if ((user_o = (DB_BYTE *) psp_cGetMemory(filebmbytes, 0)) == NULL)
            goto nomem;

        if (user_l)
            psp_freeMemory(user_l, 0);
        if ((user_l = (DB_BYTE *) psp_cGetMemory(filebmbytes, 0)) == NULL)
            goto nomem;

        if (user_r)
            psp_freeMemory(user_r, 0);
        if ((user_r = (DB_BYTE *) psp_cGetMemory(filebmbytes, 0)) == NULL)
            goto nomem;

        max_filebmbytes = filebmbytes;
    }

    if (rsp->s_tabsize.lm_maxqueue > max_sizes.lm_maxqueue)
    {
        max_sizes.lm_maxqueue = 0;
        if (queue)
            psp_freeMemory(queue, 0);
        if ((queue = (QUEUETAB *) psp_cGetMemory(
                rsp->s_tabsize.lm_maxqueue * sizeof(QUEUETAB), 0)) == NULL)
            goto nomem;

        max_sizes.lm_maxqueue = rsp->s_tabsize.lm_maxqueue;
    }

    psp_lmcFree(rsp);
    return 0;

nomem:
    error_msg(DB_TEXT("Out of memory"));
    psp_lmcFree(rsp);
    return -1;
}

void get_tables()
{
    int i, stat;
    LR_TABLES *rtp;
    LR_USERTAB *utp;
    LR_FILETAB *ftp;

    if (get_general_status())
        return;

    ssp->type_of_status = ST_TABLES;
    memcpy(&ssp->tabsize, &cur_sizes, sizeof(TABSIZE));
    
    stat = psp_lmcTrans(L_STATUS, ssp, sizeof(LM_STATUS), (void **) &rtp,
            NULL, NULL, lmc);
    if (stat != PSP_OKAY)
        return;

    msgsr    = rtp->t_msgs_received;
    msgss    = rtp->t_msgs_sent;
    granted  = rtp->t_locks_granted;
    rejected = rtp->t_locks_rejected;
    timedout = rtp->t_locks_timedout;
    freed    = rtp->t_locks_freed;

    utp = (LR_USERTAB *) (rtp + 1);

    for (i = 0; i < cur_sizes.lm_numusers; i++)
    {
        users[i].timer   = utp->ut_timer;
        users[i].pending = utp->ut_pending;
        users[i].status  = utp->ut_status;
        users[i].recover = utp->ut_recover;
        vtstrcpy(users[i].name, utp->ut_name);
        vtstrcpy(users[i].logfile, utp->ut_logfile);
        utp++;
    }

    ftp = (LR_FILETAB *) utp;

    for (i = 0; i < cur_sizes.lm_numfiles; i++)
    {
        vtstrcpy(files[i].name, ftp->ft_name);
        files[i].lockstat       = ftp->ft_lockstat;
        files[i].numlocks       = ftp->ft_numlocks;
        files[i].user_with_lock = ftp->ft_user_with_lock;
        files[i].qentry         = ftp->ft_queue_entry;
        ftp++;
    }

    if (cur_sizes.lm_maxqueue > 0)
        memcpy(queue, ftp, sizeof(QUEUETAB) * cur_sizes.lm_maxqueue);

    psp_lmcFree(rtp);
}

void get_user_info(int user)
{
    int          i, len, stat;
    LR_USERINFO *rup;
    DB_BYTE     *bm;
    DB_TCHAR    *pname;
    LR_FILEINFO *ftp;

    if (get_general_status())
        return;

    ssp->type_of_status = ST_USERINFO;
    ssp->number = user;
    memcpy(&ssp->tabsize, &cur_sizes, sizeof(TABSIZE));

    stat = psp_lmcTrans(L_STATUS, ssp, sizeof(LM_STATUS), (void **) &rup,
            NULL, NULL, lmc);
    if (stat != PSP_OKAY)
        return;

    vtstrcpy(users[user].name, rup->ui_name);
    users[user].pending = rup->ui_pending;
    users[user].timer   = rup->ui_timer;
    users[user].timeout = rup->ui_timeout;
    users[user].status  = rup->ui_status;
    users[user].recover = rup->ui_recovering;
    
    /* If TAF and LOG file names are too long, copy without path */
    pname = rup->ui_taffile;
    len = vtstrlen(pname);
    if (len > LOGFILELEN - 1)
    {
        for (pname += len; *pname != DIRCHAR && len; len--)
            pname--;
    }
    vtstrncpy(users[user].taffile, pname, LOGFILELEN - 1);
    users[user].taffile[LOGFILELEN-1] = 0;
    
    pname = rup->ui_logfile;
    len = vtstrlen(pname);
    if (len > LOGFILELEN - 1)
    {
        for (pname += len; *pname != DIRCHAR && len; len--)
            pname--;
    }
    vtstrncpy(users[user].logfile, pname, LOGFILELEN - 1);
    users[user].logfile[LOGFILELEN-1] = 0;

    bm = (DB_BYTE *) (rup + 1);

    memcpy(user_o, bm, max_filebmbytes);
    bm += max_filebmbytes;
    memcpy(user_l, bm, max_filebmbytes);
    bm += max_filebmbytes;
    memcpy(user_r, bm, max_filebmbytes);
    bm += max_filebmbytes;

    ftp = (LR_FILEINFO *) bm;

    for (i = 0; i < cur_sizes.lm_numfiles; i++)
    {
        vtstrcpy(files[i].name, ftp->fi_name);
        files[i].lockstat = ftp->fi_lockstat;
        files[i].qentry = ftp->fi_queue;
        ftp++;
    }

    if (cur_sizes.lm_maxqueue > 0)
        memcpy(queue, ftp, sizeof(QUEUETAB) * cur_sizes.lm_maxqueue);

    psp_lmcFree(rup);
}

void get_file_info(int file)
{
    int          i, stat;
    DB_BYTE     *bm;
    DB_TCHAR    *pusername;
    LR_FILEINFO *rfp;

    if (get_general_status())
        return;

    ssp->type_of_status = ST_FILEINFO;
    ssp->number = file;
    memcpy(&ssp->tabsize, &cur_sizes, sizeof(TABSIZE));

    stat = psp_lmcTrans(L_STATUS, ssp, sizeof(LM_STATUS), (void **) &rfp,
            NULL, NULL, lmc);
    if (stat != PSP_OKAY)
        return;

    vtstrcpy(filename, rfp->fi_name);
    files[file].lockstat = rfp->fi_lockstat;
    files[file].qentry = rfp->fi_queue;

    bm = (DB_BYTE *) (rfp + 1);

    memcpy(file_o, bm, max_userbmbytes);
    bm += max_userbmbytes;

    memcpy(file_l, bm, max_userbmbytes);
    bm += max_userbmbytes;

    for (i = 0, pusername = bm; i < cur_sizes.lm_numusers; i++, pusername += 16)
        vtstrcpy(users[i].name, pusername);

    if (cur_sizes.lm_maxqueue > 0)
        memcpy(queue, pusername, sizeof(QUEUETAB) * cur_sizes.lm_maxqueue);

    psp_lmcFree(rfp);
}

long GetNumQItems()
{
    int i, q_ent;
    long total = 0;

    for (i = 0, total = 0; i < cur_sizes.lm_numfiles; i++)
    {
        if (files[i].name[0])
        {
            q_ent = files[i].qentry;
            while (q_ent != -1)
            {
                total++;
                q_ent = queue[q_ent].q_next;
            }
        }
    }
    return total;
}

long GetMsgsReceived()
{
    return msgsr;
}

long GetMsgsSent()
{
    return msgss;
}

long GetLocksGranted()
{
    return granted;
}

long GetLocksRejected()
{
    return rejected;
}

long GetLocksTimedOut()
{
    return timedout;
}

long GetTotalLocks()
{
    return granted + rejected + timedout;
}

long GetLocksFreed()
{
    return freed;
}

int GetNumUsers()
{
    return cur_sizes.lm_numusers;
}

int IsUserActive(int user)
{
    if (users[user].status == U_DEAD || users[user].status == U_EMPTY)
        return 0;
    else
        return 1;
}

DB_TCHAR *GetUserId(int user)
{
    return (users[user].status ? users[user].name : DB_TEXT(""));
}

DB_TCHAR *GetUserTAF(int user)
{
    return (users[user].status ? users[user].taffile : DB_TEXT(""));
}

int GetUserPending(int user)
{
    return (users[user].status ? users[user].pending : 0);
}

int GetUserTimeOut(int user)
{
    return (users[user].status ? users[user].timeout : 0);
}

int GetUserTimer(int user)
{
    return (users[user].status ? users[user].timer : 0);
}

DB_TCHAR *GetUserStatus(int user)
{
    return (status[users[user].status]);
}

DB_TCHAR *GetUserRecover(int user)
{
    static DB_TCHAR temp_yes[4] = DB_TEXT("Yes");
    static DB_TCHAR temp_no[4]  = DB_TEXT("No");

    return (users[user].status ? ((users[user].recover == -1) ? temp_no : temp_yes) : DB_TEXT(""));
}

DB_TCHAR *GetUserLogFile(int user)
{
    return (users[user].status ? users[user].logfile : DB_TEXT(""));
}

int GetNumFiles()
{
    return cur_sizes.lm_numfiles;
}

int IsFileActive(int file)
{
    return (files[file].name[0] != 0);
}

DB_TCHAR *GetFileName(int file, short path_flag)
{
    if (path_flag)
        return filename;
    else
        return files[file].name;
}

char GetLockStat(int file)
{
    return (char) (files[file].name[0] ? files[file].lockstat : ' ');
}

int GetNumLocks(int file)
{
    return (files[file].name[0] ? files[file].numlocks : 0);
}

DB_TCHAR *GetUserWLock(int file)
{
    static DB_TCHAR temp[20];

    temp[0] = 0;
    if (files[file].name[0] && files[file].lockstat != (int) 'f')
        vtstrcpy(temp, users[files[file].user_with_lock].name);
    return temp;
}

DB_TCHAR *GetPendLocks(int file)
{
    int q_ent;
    static DB_TCHAR temp[FILENMLEN];
    DB_TCHAR *p = temp;
    int left = FILENMLEN, plen;

    *p = 0;
    if (files[file].name[0])
    {
        if (files[file].lockstat == (int) 'f')
            return (temp);
        else
        {
            q_ent = files[file].qentry;
            while (q_ent != -1)
            {
                vtstrncpy(p, users[queue[q_ent].q_user].name, left);
                p[left-1] = 0;
                plen = vtstrlen(p);
                left -= plen; p += plen;
                if (left <= 1)
                    break;

                vtstrncpy(p, DB_TEXT("  "), left);
                p[left-1] = 0;
                plen = vtstrlen(p);
                left -= plen; p += plen;
                if (left <= 1)
                    break;
                q_ent = queue[q_ent].q_next;
            }
        }
    }
    return (temp);
}

int UserHasOpen(int user, int file)
{
    return (is_bit_set(user_o, file));
}

int UserHasLock(int user, int file)
{
    return (is_bit_set(user_l, file));
}

int UserHasReq(int user, int file)
{
    return (is_bit_set(user_r, file));
}

char UserWaitingLock(int user, int file)
{
    int q_entry;

    q_entry = files[file].qentry;
    while (q_entry != -1)
    {
        if (queue[q_entry].q_user == user)
            return (char) (queue[q_entry].q_locktype);
        q_entry = queue[q_entry].q_next;
    }
    return ' ';
}

int UserNumOpen(int user)
{
    return (count_bits(user_o, cur_sizes.lm_numfiles));
}

int UserNumLock(int user)
{
    return (count_bits(user_l, cur_sizes.lm_numfiles));
}

int UserNumReq(int user)
{
    return (count_bits(user_r, cur_sizes.lm_numfiles));
}

int FileOpenByUser(int file, int user)
{
    return (is_bit_set(file_o, user));
}

int FileLockedByUser(int file, int user)
{
    return (is_bit_set(file_l, user));
}

char FileRequestedByUser(int file, int user)
{
    int qent = files[file].qentry;

    while (qent != -1)
    {
        if (queue[qent].q_user == user)
            return (char) queue[qent].q_locktype;
        qent = queue[qent].q_next;
    }
    return ' ';
}

int FileNumOpen(int file)
{
    return (count_bits(file_o, cur_sizes.lm_numusers));
}

int FileNumLock(int file)
{
    return (count_bits(file_l, cur_sizes.lm_numusers));
}

int FileNumReq(int file)
{
    int total = 0;
    int qent = files[file].qentry;

    while (qent != -1)
    {
        total++;
        qent = queue[qent].q_next;
    }
    return total;
}

int count_bits(DB_BYTE *map, int max)
{
    int i, temp;

    for (i = 0, temp = 0; i < max; i++)
    {
        if (is_bit_set(map, i))
            temp++;
    }

    return temp;
}

