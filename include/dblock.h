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

#ifndef DBLOCK_H
#define DBLOCK_H

/* Lock Manager/Runtime function interaction */

/* Function/Status codes */
#define L_BADTAF       -9
#define L_PENDING      -8
#define L_LOCKHANDLE   -7
#define L_DUPUSER      -6
#define L_RECOVER      -5
#define L_QUEUEFULL    -4
#define L_TIMEOUT      -3
#define L_UNAVAIL      -2
#define L_SYSERR       -1

#define L_OKAY          0

/* mtype values for message to lockmgr */
#define L_EMPTY         0
#define L_DBOPEN        1
#define L_DBCLOSE       2
#define L_LOCK          3
#define L_FREE          4
#define L_TRCOMMIT      5
#define L_TREND         6
#define L_SETTIME       7
#define L_RECDONE       8
#define L_RECFAIL       9
#define L_LOGIN        10
#define L_LOGOUT       11
#define L_TERMINATE    12
#define L_OPTIONS      13
#define L_VERIFYUSER   14
#define L_CLEARUSER    15
#define L_STATUS       16
#define L_PING         17

#define L_LAST         17  /* this must be updated when a new message is added */

/* sub-type values for the L_STATUS message */
#define ST_GENSTAT    0
#define ST_TABLES     1
#define ST_USERINFO   2
#define ST_FILEINFO   3
#define ST_USERSTAT   4
#define ST_TAFTAB     5


#define WORDPAD(a) ((sizeof(int)-(a)%sizeof(int))%sizeof(int))

typedef struct
{
    short          fref;
    short          type;
}  DB_LOCKREQ;

typedef struct
{
    short lm_maxusers;
    short lm_maxfiles;
    short lm_maxqueue;
    short lm_numusers;
    short lm_numfiles;
    short lm_maxtafs;
    short lm_numtafs;
} TABSIZE;


/* ==========================================================================
    Message packets, Runtime to Lock Manager
*/

typedef struct
{
    DB_USHORT version;       /* inform lockmgr of runtime version */
    DB_TCHAR  dbuserid[80];
    DB_TCHAR  taffile[FILENMLEN];
}  LM_LOGIN;

typedef struct
{
    DB_ULONG       options;
}  LM_OPTIONS;

typedef struct
{
    short          nfiles;
    short          type;
    DB_TCHAR       fnames[1];
}  LM_DBOPEN;

typedef struct
{
    short          nfiles;
    short          frefs[1];
}  LM_DBCLOSE;

typedef struct
{
    short          nfiles;
    int            read_lock_secs;
    int            write_lock_secs;
    DB_LOCKREQ     locks[1];
}  LM_LOCK;

typedef struct
{
    short          nfiles;
    short          frefs[1];
}  LM_FREE;

typedef struct
{
    DB_TCHAR       logfile[LOGFILELEN];
}  LM_TRCOMMIT;

typedef struct
{
    short          queue_timeout;
    short          lock_timeout;
}  LM_SETTIME;

typedef struct
{
    DB_TCHAR       dbuserid[80];
}  LM_USERID;

typedef struct
{
    short          type_of_status;
    short          number;
    DB_TCHAR       username[16];
    TABSIZE        tabsize;
}  LM_STATUS;


/* ==========================================================================
    Message packets, Lock Manager to Runtime
*/

typedef struct
{
    char           header[LOGFILELEN];
    DB_USHORT      lm_version;
    short          lm_type;
    DB_ULONG       lm_caps;       /* lock manager capabilities */
    short          conn;          /* connection table entry (not on UNIX) */
    short          uid;
}  LR_LOGIN;

typedef struct
{
/*    DB_TCHAR       logfile[LOGFILELEN]; */
    short          nusers;
/*    short          nfiles; */
    short          frefs[1];
}  LR_DBOPEN;

typedef struct
{
    DB_TCHAR logfile[LOGFILELEN];
} LR_RECOVER;

typedef struct
{
    DB_TCHAR       logfile[LOGFILELEN];
    int            ntimestamps;
    int            timestamps[1];
}  LR_LOCK;

typedef struct
{
    TABSIZE        s_tabsize;
}  LR_STATUS;

typedef struct
{
    DB_ULONG       t_msgs_received;
    DB_ULONG       t_msgs_sent;
    DB_ULONG       t_locks_granted;
    DB_ULONG       t_locks_rejected;
    DB_ULONG       t_locks_timedout;
    DB_ULONG       t_locks_freed;
}  LR_TABLES;

typedef struct
{
    DB_TCHAR       ui_name[16];
    short          ui_pending;
    short          ui_timer;
    short          ui_timeout;
    short          ui_status;
    short          ui_recovering;
    DB_TCHAR       ui_taffile[FILENMLEN];
    DB_TCHAR       ui_logfile[LOGFILELEN];
    DB_TCHAR       ui_netinfo1[40];
    DB_TCHAR       ui_netinfo2[40];
    DB_TCHAR       ui_netinfo3[40];
}  LR_USERINFO;

typedef struct
{
    DB_TCHAR       fi_name[FILENMLEN];
    short          fi_lockstat;
    short          fi_queue;
} LR_FILEINFO;

typedef struct
{
    DB_TCHAR       ut_name[16];
    DB_TCHAR       ut_logfile[FILENMLEN];
    short          ut_timer;
    short          ut_pending;
    short          ut_status;
    short          ut_recover;
} LR_USERTAB;

typedef struct
{
    DB_TCHAR       ft_name[FILENMLEN];
    short          ft_lockstat;
    short          ft_numlocks;
    short          ft_user_with_lock;
    short          ft_queue_entry;
} LR_FILETAB;

typedef struct
{
    DB_TCHAR       tt_name[FILENMLEN];
    short          tt_status;
    short          tt_recuser;
    short          tt_nusers;
} LR_TAFTAB;

#endif /* DBLOCK_H */


