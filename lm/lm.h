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

#include "psp.h"
#include "dblock.h"

#define MAX_USERS  20
#define MAX_FILES  256
#define MAX_QUEUES 128

#define DEFAULT_TIMEOUT 10

#define LM_OKAY         0
#define LM_FAILED       1
#define LM_DISCONNECTED 2
#define LM_NOMEMORY     3
#define LM_INVPARM      4
#define LM_INVUSER      5
#define LM_INVTYPE      6
#define LM_TERMINATE    7

#define REC_OKAY       0
#define REC_EXTERNAL   1
#define REC_NEEDED     2
#define REC_PENDING    3

typedef void *LM;
typedef void *FIDTAB;

typedef int  (LM_REPLY_FCN)(void *, void *, size_t, int);
typedef void (LM_DISCON_FCN)(void *);
typedef void (LM_INFO_FCN)(void *, DB_TCHAR *, DB_TCHAR *, DB_TCHAR *);

typedef struct {
    void          *u_conn;
    DB_TCHAR       u_name[16];
    DB_TCHAR       u_logname[LOGFILELEN];
    unsigned long  u_options;
    int            u_readlocksecs;
    int            u_writelocksecs;
    unsigned char *u_open;
    unsigned char *u_lock;
    unsigned char *u_req;
    short          u_pending;
    short          u_waiting;
    short          u_timeout;
    short          u_status;
    short          u_recuser;
    short          u_taf;
    long           u_logfile;
    long           u_timer;
} USERTAB;

typedef struct {
    unsigned char *f_open;
    unsigned char *f_lock;
    long           f_index;
    int            f_fileid;
    int            f_timestamp;
    short          f_taf;
    short          f_lockstat;
    short          f_queue;
} FILETAB;

typedef struct {
    long  t_index;
    int   t_fileid;
    short t_status;
    short t_recuser;
    short t_nusers;
} TAFTAB;

typedef struct {
    short q_locktype;
    short q_user;
    short q_next;
} QUEUETAB;

typedef struct {
    USERTAB       *usertab;
    FILETAB       *filetab;
    TAFTAB        *taftab;
    QUEUETAB      *queuetab;
    short          qHead;
    DB_BYTE       *bitmaps;
    short          maxusers;
    short          maxfiles;
    short          maxqueues;
    short          numusers;
    short          numfiles;
    short          numtafs;
    unsigned short filebmbytes;
    unsigned short userbmbytes;
    unsigned long  locks_rejected;
    unsigned long  locks_freed;
    unsigned long  locks_granted;
    unsigned long  locks_timedout;
    unsigned long  requests;
    unsigned long  replys;
    FIDTAB         fidtab;
    LM_REPLY_FCN  *send;
    LM_DISCON_FCN *discon;
    LM_INFO_FCN   *info;
} LMCB;

typedef void (LM_FCN)(LMCB *, int, void *);

int  lm_init(LM *, short, short, short, LM_REPLY_FCN *, LM_DISCON_FCN *,
        LM_INFO_FCN *);
void lm_term(LM);
int  lm_crank(LM);
int  lm_process(LM, int, int, int, void *);
int  lm_dead_user(LM, int);
int  lm_find_user(void *, void *);

int       fileid_init(FIDTAB *, long, unsigned long);
void      fileid_deinit(FIDTAB);
long      fileid_add(FIDTAB, DB_TCHAR *);
long      fileid_fnd(FIDTAB, DB_TCHAR *);
DB_TCHAR *fileid_get(FIDTAB, long);
void      fileid_del(FIDTAB, long);

void adj_files(LMCB *);
void adj_tafs(LMCB *);
void adj_users(LMCB *);
void clear_user(LMCB *, int);
void close_file(LMCB *, int, int);
void close_user(LMCB *, int);
void dead_user(LMCB *, int, int);
int  free_partial(LMCB *, int);
int  free_pending(LMCB *, int);
void freelock(LMCB *, int, int);
void grant_lock(LMCB *, int, int, short);
void lock_all(LMCB *);
void lock_reply(LMCB *);
int  usercount(LMCB *);

int  room_q(LMCB *, int);
int  get_q(LMCB *);
void free_q(LMCB *, int);

void ins_head(LMCB *, int, int);
void ins_tail(LMCB *, int, int);
void ins_Record(LMCB *, int, int);

void reply_user(LMCB *, int, void *, size_t, int);

int is_map_zero(DB_BYTE *, size_t);

#define bit_clr(m, b) (((DB_BYTE *) m)[b / BITS_PER_BYTE]) &=\
        ~(1 << ((BITS_PER_BYTE - 1) - (b % BITS_PER_BYTE)))
#define bit_set(m, b) (((DB_BYTE *) m)[b / BITS_PER_BYTE]) |=\
        1 << ((BITS_PER_BYTE - 1) - (b % BITS_PER_BYTE))
#define is_bit_set(m, b) (((DB_BYTE *) m)[b / BITS_PER_BYTE] &\
        (1 << ((BITS_PER_BYTE - 1) - (b % BITS_PER_BYTE))) & 0xff)
int count_bits(DB_BYTE *, int);
int first_bit_set(DB_BYTE *, int);

#if defined(DB_DEBUG)
extern FILE *logfh;
#define DEBUG1(a)             { vftprintf(logfh,  a); fflush(logfh); }
#define DEBUG2(a, b)          { vftprintf(logfh,  a, b); fflush(logfh); }
#define DEBUG3(a, b, c)       { vftprintf(logfh,  a, b, c); fflush(logfh); }
#define DEBUG4(a, b, c, d)    { vftprintf(logfh,  a, b, c, d); fflush(logfh); }
#define DEBUG5(a, b, c, d, e) { vftprintf(logfh,  a, b, c, d, e); fflush(logfh); }
#else
#define DEBUG1(a)
#define DEBUG2(a, b)
#define DEBUG3(a, b, c)
#define DEBUG4(a, b, c, d)
#define DEBUG5(a, b, c, d, e)
#endif


