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

/*** Lock Manager Data Functions ***/

long       GetNumQItems         (void);
long       GetMsgsReceived      (void);
long       GetMsgsSent          (void);
long       GetLocksGranted      (void);
long       GetLocksRejected     (void);
long       GetLocksTimedOut     (void);
long       GetTotalLocks        (void);
long       GetLocksFreed        (void);
int        GetNumUsers          (void);
int        IsUserActive         (int);
DB_TCHAR * GetUserId            (int);
DB_TCHAR * GetUserTAF           (int);
int        GetUserPending       (int);
int        GetUserTimeOut       (int);
int        GetUserTimer         (int);
DB_TCHAR * GetUserStatus        (int);
DB_TCHAR * GetUserRecover       (int);
DB_TCHAR * GetUserLogFile       (int);
int        GetNumFiles          (void);
int        IsFileActive         (int);
DB_TCHAR * GetFileName          (int, short);
char       GetLockStat          (int);
int        GetNumLocks          (int);
DB_TCHAR * GetUserWLock         (int);
DB_TCHAR * GetPendLocks         (int);
int        UserHasOpen          (int, int);
int        UserHasLock          (int, int);
int        UserHasReq           (int, int);
char       UserWaitingLock      (int, int);
int        UserNumOpen          (int);
int        UserNumLock          (int);
int        UserNumReq           (int);
int        FileOpenByUser       (int, int);
int        FileLockedByUser     (int, int);
char       FileRequestedByUser  (int, int);
int        FileNumOpen          (int);
int        FileNumLock          (int);
int        FileNumReq           (int);


/*** Entry Functions ***/

void console_cleanup (int);
void console         (void);

void get_user_info (int);
void get_file_info (int);
void get_tables    (void);
int  kill_user     (DB_TCHAR *);
int  kill_lockmgr  (void);
void error_msg     (DB_TCHAR *);

int  get_display_memory(void);
void free_display_memory(void);
void free_table_memory(void);

void vioInit(void);
void vioTerm(void);


/*** Global Data ***/

extern int     refresh_secs;
extern char    lockmgrn[LOCKMGRLEN];
extern char   *title1;
extern char    title2[80];
extern PSP_LMC lmc;


