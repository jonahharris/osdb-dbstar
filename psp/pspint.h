/***************************************************************************
 *                                                                         *
 * db.linux                                                                *
 * open source platform support package for Linux (tm)                     *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reseved.    *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      *
 *                                                                         *
 **************************************************************************/

/* pspint.h - Internal definitions for the PSP */

#if defined(UNIX)
#define NEED_STRNICMP
#define PSP_FDESC    int
#if defined(SCO) || defined(QNX)
#define psp_thread_t int
#else
#include <pthread.h>
#define psp_thread_t pthread_t
#endif
#endif

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define PSP_FDESC    HANDLE
#define psp_thread_t long
#endif

extern int psp_inited;
extern int psp_terminating;

int    psp_syncInit(void);
void   psp_syncShutdown(void);
void   psp_syncTerm(void);

int    psp_memInit(void);
void   psp_memTerm(void);

int    psp_fileInit(void);
void   psp_fileTerm(void);

int    psp_osInit(void);
void   psp_osTerm(void);

void   psp_flClose(PSP_FDESC);
long   psp_flLastAccess(PSP_FDESC);
short  psp_flLock(PSP_FDESC);
long   psp_flModTime(PSP_FDESC);
short  psp_flOpen(const DB_TCHAR *, unsigned int, unsigned short, PSP_FDESC *);
size_t psp_flRead(PSP_FDESC, void *, size_t);
void   psp_flSeek(PSP_FDESC, size_t, int);
size_t psp_flSeekRead(PSP_FDESC, size_t, void *, size_t);
size_t psp_flSeekWrite(PSP_FDESC, size_t, const void *, size_t);
int    psp_flSetSize(PSP_FDESC, size_t);
void   psp_flSync(PSP_FDESC);
size_t psp_flSize(PSP_FDESC);
void   psp_flUnlock(PSP_FDESC);
size_t psp_flWrite(PSP_FDESC, const void *, size_t);

psp_thread_t psp_threadId(void);

/* Network definitions */
typedef struct {
    long db_star_id;
    long msglen;
} MSG_HDR;

#define DB_STAR_ID 0x239A03CF
