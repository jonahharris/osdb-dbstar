/***************************************************************************
 *                                                                         *
 * db.linux                                                                *
 * open source database kernel
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reseved.    *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      *
 *                                                                         *
 **************************************************************************/

/* psp.h - Contains definitions for the PSP */
#if !defined(PSP_H)
#define PSP_H

#include "pspplat.h"

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <setjmp.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILENMLEN   256
#define FILEIDLEN   148
#define LOGFILELEN  48
#define LOCKMGRLEN  48

#define PSP_OKAY         0
#define PSP_TIMEOUT      1
#define PSP_FAILED       2
#define PSP_NOMEMORY     3
#define PSP_PROTOCOL     4
#define PSP_DISCONNECTED 5
#define PSP_NOTAVAIL     6
#define PSP_NOSERVER     7
#define PSP_INVMSG       8
#define PSP_DUPUSERID    9
#define PSP_DUPLICATE   10

int  psp_init(void);
void psp_term(void);

/***************************************************************************
  Memory functions
*/
typedef void *PSP_MEMTAG;
#define NULL_MEMTAG ((PSP_MEMTAG) -1)

void *psp_getMem(size_t, PSP_MEMTAG, int, const DB_TCHAR *, long);
void *psp_extendMem(const void *, size_t, PSP_MEMTAG, int, size_t, long);
void  psp_freeMemory(const void *, PSP_MEMTAG);

#define PSP_TAG_NOSEM   0x0000
#define PSP_TAG_USESEM  0x0001

PSP_MEMTAG psp_createTag(jmp_buf, short);
void       psp_freeTagMemory(PSP_MEMTAG, short);
void       psp_resetTag(jmp_buf, PSP_MEMTAG);

#if !defined(__MOD_ID__)
#define __MOD_ID__ 0
#endif

#define MEM_ALLOC  1
#define MEM_CALLOC 2
#define MEM_STRDUP 3
#define MEM_MEMDUP 4

#define psp_getMemory(sz, tag)                psp_getMem(sz, tag, MEM_ALLOC, NULL, __MOD_ID__)
#define psp_cGetMemory(sz, tag)               psp_getMem(sz, tag, MEM_CALLOC, NULL, __MOD_ID__)
#define psp_strdup(str, tag)                  ((DB_TCHAR *) psp_getMem(0, tag, MEM_STRDUP, str, __MOD_ID__))
#define psp_memdup(sz, mem, tag)              psp_getMem(sz, tag, MEM_MEMDUP, mem, __MOD_ID__)
#define psp_extendMemory(mem, sz, tag)        psp_extendMem(mem, sz, tag, 0, 0, __MOD_ID__)
#define psp_cExtendMemory(mem, nsz, osz, tag) psp_extendMem(mem, nsz, tag, 1, osz, __MOD_ID__)
#define psp_zFreeMemory(pptr, tag)            psp_freeMemory(*pptr, tag), *pptr = NULL

/***************************************************************************
  File functions
*/
typedef void *PSP_FH;

/* extended flags */
#define PSP_FLAG_DENYNO  0x0000
#define PSP_FLAG_DENYRD  0x0001
#define PSP_FLAG_DENYWR  0x0002
#define PSP_FLAG_DENYRW  (PSP_FLAG_DENYRD | PSP_FLAG_DENYWR)
#define PSP_FLAG_STICKY  0x0004
#define PSP_FLAG_SYNC    0x0008
#define PSP_FLAG_FPRINTF 0x0010
#define PSP_FLAG_STREAM  0x0020

void      psp_fileClose(PSP_FH);
char      psp_fileGetc(PSP_FH);
char     *psp_fileGets(PSP_FH, char *, size_t);
long      psp_fileLastAccess(PSP_FH);
size_t    psp_fileLength(PSP_FH);
short     psp_fileLock(PSP_FH);
long      psp_fileModTime(PSP_FH);
PSP_FH    psp_fileOpen(const DB_TCHAR *, unsigned int, unsigned short);
int       psp_filePrintf(PSP_FH, const DB_TCHAR *, ...);
int       psp_fileRead(PSP_FH, void *, size_t);
void      psp_fileSeek(PSP_FH, size_t);
int       psp_fileSeekRead(PSP_FH, size_t, void *, size_t);
int       psp_fileSeekWrite(PSP_FH, size_t, const void *, size_t);
int       psp_fileSetSize(PSP_FH, size_t);
void      psp_fileSync(PSP_FH);
void      psp_fileUnlock(PSP_FH);
int       psp_fileWrite(PSP_FH, const void *, size_t);

int       psp_fileCopy(const DB_TCHAR *, const DB_TCHAR *);
int       psp_fileMove(const DB_TCHAR *, const DB_TCHAR *);
int       psp_fileNameCmp(const DB_TCHAR *, const DB_TCHAR *);
int       psp_fileNamenCmp(const DB_TCHAR *, const DB_TCHAR *, size_t);
int       psp_fileRemove(const DB_TCHAR *);
int       psp_fileRename(const DB_TCHAR *, const DB_TCHAR *);
size_t    psp_fileSize(const DB_TCHAR *);
int       psp_fileValidate(const DB_TCHAR *);

long      psp_fileHandleLimit();
void      psp_fileSetHandleLimit(long);

/***************************************************************************
  Synchronization functions
*/
typedef void *PSP_SEM;
#define NO_PSP_SEM (PSP_SEM) -1
#define PSP_INDEFINITE_WAIT -1

#define PSP_UNDEF_SEM 0
#define PSP_MUTEX_SEM 1
#define PSP_EVENT_SEM 2

PSP_SEM psp_syncCreate(short);
void    psp_syncDelete(PSP_SEM);
void    psp_syncEnterExcl(PSP_SEM);
void    psp_syncExitExcl(PSP_SEM);
void    psp_syncResume(PSP_SEM);
void    psp_syncStart(PSP_SEM);
short   psp_syncWait(PSP_SEM, long);

void    psp_enterCritSec(void);
void    psp_exitCritSec(void);

/***************************************************************************
  INI file functions
*/
typedef void *PSP_INI;

PSP_INI psp_iniOpen(const DB_TCHAR *);
void    psp_iniClose(PSP_INI);
short   psp_iniShort(PSP_INI, const DB_TCHAR *, const DB_TCHAR *, short);
size_t  psp_iniString(PSP_INI, const DB_TCHAR *, const DB_TCHAR *,
        const DB_TCHAR *, DB_TCHAR *, size_t);

/***************************************************************************
  Communication functions
*/
typedef void *PSP_LMC;
typedef void *PSP_CONN;

typedef int   (LMC_SEND_FCN)(const void *, size_t, const void *);
typedef int   (LMC_RECV_FCN)(void **, size_t *, const void *);

typedef int   (LMC_CHECKID_FCN)(const DB_TCHAR *);
typedef int   (LMC_CONNECT_FCN)(const DB_TCHAR *, const DB_TCHAR *,
        const DB_TCHAR *, void **);
typedef void  (LMC_DISCONNECT_FCN)(void *);

typedef int   (LMC_LISTEN_FCN)(const DB_TCHAR *, const DB_TCHAR *, int,
        void **);
typedef void *(LMC_WAITMSG_FCN)(void *, long);
typedef void  (LMC_STOPLISTEN_FCN)(void *);
typedef void  (LMC_DISCONCLIENT_FCN)(void *, void *);
typedef void  (LMC_INFO_FCN)(const void *, DB_TCHAR *, DB_TCHAR *, DB_TCHAR *);

typedef struct {
    DB_TCHAR             *name;
    LMC_SEND_FCN         *send;
    LMC_RECV_FCN         *recv;

    LMC_CHECKID_FCN      *checkid;
    LMC_CONNECT_FCN      *connect;
    LMC_DISCONNECT_FCN   *disconnect;

    LMC_LISTEN_FCN       *listen;
    LMC_WAITMSG_FCN      *waitmsg;
    LMC_STOPLISTEN_FCN   *stoplisten;
    LMC_DISCONCLIENT_FCN *disconclient;

    LMC_INFO_FCN         *info;
} LOCKCOMM_FCNS;

typedef int (LMC_AVAIL_FCN)(int *, LOCKCOMM_FCNS **);

#define PSP_FLAG_NETWORK_ORDER 0x0001
#define PSP_FLAG_NO_TAF_ACCESS 0x0002
#define PSP_FLAG_NO_FILE_LOCKS 0x0004
#define PSP_FLAG_TAF_CLOSE     0x0008

extern LMC_AVAIL_FCN psp_tcp_avail;
extern LMC_AVAIL_FCN psp_ip_avail;
/*extern LMC_AVAIL_FCN psp_gen_avail*/

void          *psp_lmcAlloc(size_t);
LMC_AVAIL_FCN *psp_lmcAvail(PSP_LMC);
int            psp_lmcCheckid(const DB_TCHAR *, const DB_TCHAR *,
        const DB_TCHAR *, PSP_LMC);
void           psp_lmcCleanup(PSP_LMC);
int            psp_lmcConnect(const DB_TCHAR *, const DB_TCHAR *,
        const DB_TCHAR *, PSP_LMC);
void           psp_lmcConnectInfo(PSP_CONN, DB_TCHAR *, DB_TCHAR *, DB_TCHAR *,
        PSP_LMC);
void           psp_lmcDisconClient(PSP_CONN, PSP_LMC);
void           psp_lmcDisconnect(PSP_LMC);
int            psp_lmcErrstr(DB_TCHAR *, int, size_t, PSP_LMC);
LMC_AVAIL_FCN *psp_lmcFind(const DB_TCHAR *);
DB_TCHAR      *psp_lmcFullName(PSP_LMC);
int            psp_lmcFlags(PSP_LMC);
void           psp_lmcFree(void *);
int            psp_lmcListen(const DB_TCHAR *, const DB_TCHAR *, int, PSP_LMC);
DB_TCHAR      *psp_lmcName(PSP_LMC);
int            psp_lmcReply(PSP_CONN, void *, size_t, int, PSP_LMC);
int            psp_lmcSetup(PSP_LMC *, LMC_AVAIL_FCN *);
void           psp_lmcStopListen(PSP_LMC);
int            psp_lmcTrans(int, void *, size_t, void **, size_t *, int *,
        PSP_LMC);
int            psp_lmcWaitMsg(PSP_CONN *, int *, void **, size_t *, long, PSP_LMC);

/***************************************************************************
  Thread and Process functions
*/
unsigned int psp_get_pid(void);
void         psp_sleep(unsigned long);
short        psp_threadBegin(void (*)(void *), unsigned int, void *);
void         psp_threadEnd(void);

void         psp_daemonInit(DB_TCHAR *);
void         psp_daemonTerm(void);

/***************************************************************************
  Path (File and Directory) functions
*/
typedef void *PSP_DIR;

PSP_DIR   psp_pathOpen(const DB_TCHAR *, const DB_TCHAR *);
DB_TCHAR *psp_pathNext(PSP_DIR);
void      psp_pathClose(PSP_DIR);

DB_TCHAR *psp_pathDefTmp(void);
DB_TCHAR *psp_pathGetFile(const DB_TCHAR *);
int       psp_pathIsDir(const DB_TCHAR *);
void      psp_pathSplit(const DB_TCHAR *, DB_TCHAR **, DB_TCHAR **);
DB_TCHAR *psp_pathStripDrive(const DB_TCHAR *);

/**************************************************************************
  SafeGarde definitions
*/
#define NO_ENC   0
#define LOW_ENC  1
#define MED_ENC  2
#define HIGH_ENC 3

typedef void (SG_ENCDEC_FCN)(void *, void *, size_t);
typedef int  (SG_SETCHK_FCN)(void *, void *);

typedef struct {
    void          *data;
    size_t         blocksize;
    SG_ENCDEC_FCN *enc;
    SG_ENCDEC_FCN *dec;
    SG_SETCHK_FCN *set;
    SG_SETCHK_FCN *chk;
} SG;

SG  *sg_create(int, DB_TCHAR *);
void sg_delete(SG *);

/***************************************************************************
  Miscellaneous functions
*/
typedef void (INTERRUPT_FCN)(void);
DB_TCHAR *psp_defLockmgr(void);
int       psp_errno(void);
void      psp_errPrint(const DB_TCHAR *);
DB_TCHAR *psp_getenv(const DB_TCHAR *);
long      psp_seed(void);
long      psp_timeSecs(void);
long      psp_timeMilliSecs(void);
DB_TCHAR *psp_truename(const DB_TCHAR *, PSP_MEMTAG);
int       psp_validLockmgr(const DB_TCHAR *);
int       psp_handleInterrupt(INTERRUPT_FCN *);

void      psp_localeGet(DB_TCHAR *, size_t);
void      psp_localeSet(const DB_TCHAR *);

#if defined(NEED_STRUPR)
char *strlwr(char *);
char *strupr(char *);
#endif

#if defined(NEED_STRICMP)
int stricmp(const char *, const char *);
int strnicmp(const char *, const char *, size_t);
#endif

#if defined(NEED_WCSICMP)
int wcsicmp(const wchar_t *, const wchar_t *);
int wcsnicmp(const wchar_t *, const wchar_t *, size_t);
#endif

/* User table status */
#define U_EMPTY         0
#define U_LIVE          1
#define U_DEAD          2
#define U_BEING_REC     3
#define U_RECOVERING    4
#define U_REC_MYSELF    5
#define U_HOLDING_X     6

#ifdef __cplusplus
}
#endif

#endif /* PSP_H */

