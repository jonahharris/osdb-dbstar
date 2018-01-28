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

/*-----------------------------------------------------------------------------
   lmc.c -- db.* Communications function module

   Copyright (c) 1999, Centura Software Corporation, All Rights Reserved
-----------------------------------------------------------------------------*/

#include "psp.h"

typedef struct
{
    LMC_AVAIL_FCN *avail;
    LOCKCOMM_FCNS *fcns;
    void          *data;
    int            connected;
    int            flags;
} LMCCB;

/* ==========================================================================
   Allocate a message (including a header)
*/
void *psp_lmcAlloc(
    size_t size)
{
    int *ptr;

    ptr = (int *) psp_getMemory(size + sizeof(int), 0);
    if (ptr == NULL)
        return NULL;

    return ptr + 1;
}

/* ==========================================================================
   Free a message (including the header)
*/
void psp_lmcFree(
    void *ptr)
{
    if (ptr)
        psp_freeMemory((int *) ptr - 1, 0);
}

/* ==========================================================================
   Setup the lock manager communication data
*/
int psp_lmcSetup(
    PSP_LMC       *lmc,
    LMC_AVAIL_FCN *avail)
{
    LMCCB         *p;
    LOCKCOMM_FCNS *fcns;
    int            flags;

    if (!avail)
        return PSP_FAILED;

    if ((*avail)(&flags, &fcns) != PSP_OKAY)
        return PSP_NOTAVAIL;

    p = (LMCCB *) (*lmc = psp_cGetMemory(sizeof(LMCCB), 0));
    if (!p)
        return PSP_NOMEMORY;

    p->avail = avail;
    p->fcns  = fcns;
    p->flags = flags;

    *lmc = (PSP_LMC) p;
 
    return PSP_OKAY;
}

/* ==========================================================================
   Open up communication to the lockmanager
*/
int psp_lmcConnect(
    const DB_TCHAR *lockmgr,
    const DB_TCHAR *id,
    const DB_TCHAR *tmp,
    PSP_LMC         lmc)
{
    int    stat;
    LMCCB *p = (LMCCB *) lmc;

    if (!p)
        return PSP_FAILED;

    if (p->connected)
        return PSP_OKAY;

    if ((stat = (*p->fcns->connect)(lockmgr, tmp, id, &p->data)) != PSP_OKAY)
        return stat;
   
    p->connected = 1;
 
    return PSP_OKAY;
}

/* ==========================================================================
   Close communication to the lock manager
*/
void psp_lmcDisconnect(
    PSP_LMC lmc)
{
    LMCCB *p = (LMCCB *) lmc;

    if (!p || !p->connected)
        return;

    (*p->fcns->disconnect)(p->data);
    p->connected = 0;
}

/* ==========================================================================
   Setup listening
*/
int psp_lmcListen(
    const DB_TCHAR *name,
    const DB_TCHAR *tmp,
    int             maxusers,
    PSP_LMC         lmc)
{
    LMCCB *p;

    if (!lmc)
        return PSP_FAILED;

    p = (LMCCB *) lmc;
    return (*p->fcns->listen)(name, tmp, maxusers, &p->data);
}

/* ==========================================================================
   Wait for a message from the clients
*/
int psp_lmcWaitMsg(
    PSP_CONN *user,
    int      *mtype,
    void    **msg,
    size_t   *size,
    long      timeout,
    PSP_LMC   lmc)
{
    int    stat;
    int   *buf;
    void  *conn;
    LMCCB *p;

    if (!lmc)
        return PSP_FAILED;

    p = (LMCCB *) lmc;
    if ((conn = (*p->fcns->waitmsg)(p->data, timeout)) == NULL)
        return PSP_TIMEOUT;

    *user = conn;
    if ((stat = (*p->fcns->recv)((void **) &buf, size, conn)) != PSP_OKAY) {
        (*p->fcns->disconclient)(p->data, conn);
        return stat;
    }

    *mtype = *buf;
    if ((*size -= sizeof(int)) == 0)
    {
        *msg = NULL;
        psp_freeMemory(buf, 0);
    }
    else
        *msg = buf + 1;

    return PSP_OKAY;
}

/* ==========================================================================
   Reply to a connection
*/
int psp_lmcReply(
    PSP_CONN  conn,
    void     *msg,
    size_t    size,
    int       status,
    PSP_LMC   lmc)
{
    int    stat;
    int   *ptr;
    LMCCB *p;

    if (!lmc)
        return PSP_FAILED;

    if (msg) {
        ptr = (int *) msg - 1;
        *ptr = status;
    }
    else
        ptr = &status;

    p = (LMCCB *)lmc;
    if ((stat = (*p->fcns->send)(ptr, size + sizeof(int), conn)) != PSP_OKAY)
        (*p->fcns->disconclient)(lmc, conn);

    return stat;
}

/* ==========================================================================
   Shut down listening
*/
void psp_lmcStopListen(
    PSP_LMC lmc)
{
    LMCCB *p;

    if (!lmc)
        return;

    p = (LMCCB *) lmc;
    (*p->fcns->stoplisten)(p->data);
}

/* ==========================================================================
   Disconnect a client
*/
void psp_lmcDisconClient(
    PSP_CONN conn,
    PSP_LMC  lmc)
{
    LMCCB *p;

    if (!lmc)
        return;

    p = (LMCCB *) lmc;
    (*p->fcns->disconclient)(p->data, conn);
}

/* ==========================================================================
   Get network location information about the client
*/
void psp_lmcConnectInfo(
    PSP_CONN  conn,
    DB_TCHAR *line1,
    DB_TCHAR *line2,
    DB_TCHAR *line3,
    PSP_LMC   lmc)
{
    if (!lmc)
        return;

    (*((LMCCB *) lmc)->fcns->info)(conn, line1, line2, line3);
}

/* ==========================================================================
   Cleanup up lock manager communication data
*/
void psp_lmcCleanup(
    PSP_LMC lmc)
{
    if (lmc)
        psp_freeMemory(lmc, 0);
}

/* ==========================================================================
   Check a userid for correctness and uniqueness
*/
int psp_lmcCheckid(
    const DB_TCHAR *lockmgr,
    const DB_TCHAR *id,
    const DB_TCHAR *tmp,
    PSP_LMC         lmc)
{
    int stat;

    if (!lockmgr || !*lockmgr)
        return PSP_NOSERVER;

    if ((stat = (*((LMCCB *) lmc)->fcns->checkid)(id)) != PSP_OKAY)
        return PSP_FAILED;

    if ((stat = psp_lmcConnect(lockmgr, id, tmp, lmc)) == PSP_OKAY)
        psp_lmcDisconnect(lmc);

    return stat;
}

/* ==========================================================================
   Return the lock manager communication flags
*/
int psp_lmcFlags(
    PSP_LMC lmc)
{
    if (!lmc)
        return 0;

    return ((LMCCB *) lmc)->flags;
}

/* ==========================================================================
   Return the full transport name
*/
DB_TCHAR *psp_lmcFullName(
    PSP_LMC lmc)
{
    if (!lmc)
        return DB_TEXT("None");

    return ((LMCCB *) lmc)->fcns->name;
}

/* ==========================================================================
   Return a pointer to the lock communication function
*/
LMC_AVAIL_FCN *psp_lmcAvail(
    PSP_LMC lmc)
{
    if (!lmc)
        return NULL;

    return ((LMCCB *) lmc)->avail;
}

/* ==========================================================================
   Generate a transport specific error string
*/
int psp_lmcErrstr(
    DB_TCHAR *buf,
    int       err,
    size_t    len,
    PSP_LMC   lmc)
{
    return PSP_OKAY;
}

/* ==========================================================================
   Send a message to the lock manager and receive a response
*/
int psp_lmcTrans(
    int     mtype,
    void   *send,
    size_t  sendsize,
    void  **recv,
    size_t *recvsize,
    int    *status,
    PSP_LMC lmc)
{
    int           stat;
    int          *pkt;
    size_t        size;
    LMCCB        *p = (LMCCB *) lmc;
    LMC_SEND_FCN *sendfcn = p->fcns->send;
    LMC_RECV_FCN *recvfcn = p->fcns->recv;

    if (send) {
        pkt = (int *) send - 1;
        *pkt = mtype;
    }
    else
        pkt = &mtype;

    if ((stat = (*sendfcn)(pkt, sendsize + sizeof(int), p->data)) != PSP_OKAY)
        return stat;

    if ((stat = (*recvfcn)((void **) &pkt, &size, p->data)) != PSP_OKAY)
        return stat;

    if (status)
        *status = *pkt;

    if ((size -= sizeof(int)) == 0) {
        if (recv)
            *recv = NULL;

        psp_freeMemory(pkt, 0);
    }
    else {
        if (recv)
            *recv = ++pkt;
        else
            psp_freeMemory(pkt, 0);
    }

    if (recvsize)
        *recvsize = size;

    return PSP_OKAY;
}

typedef struct {
    DB_TCHAR      *name;
    LMC_AVAIL_FCN *avail;
} TRANSPORTS;

static TRANSPORTS transports[] = {
    { DB_TEXT("TCP"), psp_tcp_avail },
#if defined(UNIX) && !defined(QNX)
    { DB_TEXT("IP"),  psp_ip_avail },
#endif
    { NULL,  NULL },
    { NULL,  NULL },
    { NULL,  NULL }
};

#define NUM_TRANSPORTS (sizeof(transports) / sizeof(TRANSPORTS))

/* ==========================================================================
   Return a pointer to the transport name
*/
DB_TCHAR *psp_lmcName(
    PSP_LMC lmc)
{
    size_t         ii;
    LMC_AVAIL_FCN *avail;

    if (!lmc)
        return DB_TEXT("None");

    avail = ((LMCCB *) lmc)->avail;
    for (ii = 0; ii < NUM_TRANSPORTS; ii++) {
        if (transports[ii].avail == avail)
            return transports[ii].name;
    }

    return DB_TEXT("None");
}

/* ===========================================================================
   Find the lock manager communications available function
*/
LMC_AVAIL_FCN *psp_lmcFind(
    const DB_TCHAR *name)
{
    size_t ii;

    for (ii = 0; ii < NUM_TRANSPORTS; ii++) {
        if (transports[ii].name && vtstricmp(name, transports[ii].name) == 0)
            return transports[ii].avail;
    }

    return NULL;
}
