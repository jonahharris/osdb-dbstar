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

/* psptcpux.c - Contains the TCP/IP and Unix Domain sockets functions for
                UNIX */

#if defined(QNX) || defined(SCO3)
#define NO_UNIX_DOMAIN
#endif

#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(QNX)
#include <sys/select.h>
#endif
#if !defined(NO_UNIX_DOMAIN)
#include <sys/un.h>
#endif

#if !defined(INADDR_NONE)
#define INADDR_NONE ((unsigned long) -1)
#endif

#include "psp.h"
#include "pspint.h"

typedef union {
    struct sockaddr_in in;
    struct sockaddr_un un;
} saddr;

typedef struct {
    int    sock;           /* connection socket */
/*    void (*pipefcn)(int);*/  /* location to hold current SIGPIPE function */
    char  *tokfile;        /* token file for local connect through IP */
    saddr  addrinfo;
} LMC_CLIENT_DATA;

typedef struct {
    fd_set            FDS;
    fd_set            holdFDS;
    int               count;
    int               sock1;
    int               sock2;
    char             *tokfile;
    int               numclients;
    int               maxclients;
    LMC_CLIENT_DATA **clients;
} LMC_SERVER_DATA;

static int   psp_socket_checkid(const char *);
static void  psp_socket_disconnect(void *);
static void *psp_socket_waitmsg(void *, long);
static void  psp_socket_stoplisten(void *);
static void  psp_socket_disconclient(void *, void *);
static void  psp_socket_info(const void *, char *, char *, char *);

static int   psp_tcp_connect(const char *, const char *, const char *, void **);
static int   psp_tcp_listen(const char *, const char *, int, void **);
static int   psp_tcp_send(const void *, size_t, const void *);
static int   psp_tcp_receive(void **, size_t *, const void *);

LOCKCOMM_FCNS tcp_lockcomm = {
    "TCP/IP sockets",
    psp_tcp_send,
    psp_tcp_receive,

    psp_socket_checkid,
    psp_tcp_connect,
    psp_socket_disconnect,

    psp_tcp_listen,
    psp_socket_waitmsg,
    psp_socket_stoplisten,
    psp_socket_disconclient,

    psp_socket_info
};

static int tcp_connect(const char *, const char *, int);
static int tcp_listen(const char *);

#if !defined(NO_UNIX_DOMAIN)
static int   psp_ip_connect(const char *, const char *, const char *, void **);
static int   psp_ip_listen(const char *, const char *, int, void **);
static int   psp_ip_send(const void *, size_t, const void *);
static int   psp_ip_receive(void **, size_t *, const void *);

LOCKCOMM_FCNS ip_lockcomm = {
    "Unix Domain sockets",
    psp_ip_send,
    psp_ip_receive,

    psp_socket_checkid,
    psp_ip_connect,
    psp_socket_disconnect,

    psp_ip_listen,
    psp_socket_waitmsg,
    psp_socket_stoplisten,
    psp_socket_disconclient,

    psp_socket_info
};

static int ip_connect(const char *, const char *, char *, int);
static int ip_listen(const char *, const char *, char *);
#endif

static int socket_send(const MSG_HDR *, const void *, int, int);
static int socket_receive(void *, int, int);

/* ==========================================================================
   Check to see if TCP/IP is available
*/
int psp_tcp_avail(
    int            *flags,
    LOCKCOMM_FCNS **fcns)
{
    *flags = PSP_FLAG_NETWORK_ORDER /* | PSP_FLAG_TAF_CLOSE */;
    *fcns = &tcp_lockcomm;

    return PSP_OKAY;
}

/* ==========================================================================
   Verify that the id is valid for this transport
*/
static int psp_socket_checkid(
    const char *id)
{
    return PSP_OKAY;
}

/* ==========================================================================
   TCP/IP specific connection
*/
static int tcp_connect(
    const char *name,
    const char *port,
    int         timeout)
{
    int                ii;
    int                sock;
    int                addr_len;
    unsigned long      addr;
    struct hostent    *host;
    struct servent    *service;
    struct protoent   *protocol;
    struct sockaddr   *sockaddr = NULL;
    struct sockaddr_in sockaddr_in;
    sockaddr = (struct sockaddr *) &sockaddr_in;
    addr_len = sizeof(struct sockaddr_in);

    host = gethostbyname(name);
    endhostent();

    if (host!= NULL) {
        sockaddr_in.sin_family = host->h_addrtype;
        memcpy(&sockaddr_in.sin_addr, host->h_addr, host->h_length);
    }
    else if ((addr = inet_addr(name)) != INADDR_NONE) {
        sockaddr_in.sin_family = AF_INET;
        sockaddr_in.sin_addr.s_addr = addr;
    }
    else {
        /* TBD: Log out error */
/*        printf("cannot create valid sockaddr: check lock manager name\n"); */
        return -1;
    }

    /* We have found the TCP/IP host that the lock manager is on, now
       figure out the port number. */
    protocol = getprotobyname("tcp");
    endprotoent();

    sockaddr_in.sin_port = 0;
    if (port)
        sockaddr_in.sin_port = htons(atoi(port));

    if (sockaddr_in.sin_port == 0) {
        if (port && (service = getservbyname(port, "tcp")) != NULL)
            sockaddr_in.sin_port = service->s_port;
        else if ((service = getservbyname("db.star", "tcp")) != NULL)
            sockaddr_in.sin_port = service->s_port;

        endservent();

        /* If no port was found, try the default */
        if (sockaddr_in.sin_port == 0)
            sockaddr_in.sin_port = htons(1523);
    }

    sock = socket(sockaddr->sa_family, SOCK_STREAM, protocol->p_proto);
    if (sock == -1)
        return -1;

    for (ii = 0; ii < timeout; ii++) {
        if (connect(sock, sockaddr, addr_len) != -1)
            break;

        /* TBD: Log out error */
/*        printf("ip connect fail: %s (%d)\n", strerror(errno), errno); */

        psp_sleep(1);
    }

    if (ii == timeout) {
        close(sock);
        return -1;
    }

    return sock;
}

/* ==========================================================================
   Open a TCP/IP connection to the lock manager
*/
static int psp_tcp_connect(
    const char *lockmgr,
    const char *tmp,
    const char *id,
    void      **data)
{
    int              sock = -1;
    char            *name;
    char            *portpos;
    char             tokfile[80] = "";
    LMC_CLIENT_DATA *lmcdata;

    /* TBD: Need to get timeout value (net_timeout) from rdm.ini. */
    int                timeout = 5;

#if defined(QNX)
    name = psp_strdup(lockmgr + ((lockmgr[0] == '/') ? 1 : 0), 0);
#else
    name = psp_strdup(lockmgr, 0);
#endif

    if ((portpos = strchr(name, ':')) != NULL)
        *portpos++ = '\0';

#if !defined(NO_UNIX_DOMAIN)
    /* If there is no ':' in the lock manager name then it is probably
       running locally.  Try to connect to it using a UNIX domain socket,
       as this is faster than TCP/IP. */
    if (!portpos)
        sock = ip_connect(name, tmp, tokfile, timeout);
#endif

    /* If the attempt to connect to a local lock manager failed, or if the
       lock manager name contains a ':' then it must be running on a
       remote machine - use a TCP/IP socket. */
    if (sock == -1)
        sock = tcp_connect(name, portpos, timeout);

    psp_freeMemory(name, 0);
    if (sock == -1)
        return PSP_FAILED;

    /* Allocate control data buffer */
    lmcdata = (LMC_CLIENT_DATA *) psp_getMemory(sizeof(LMC_CLIENT_DATA), 0);
    if (lmcdata == NULL) {
        shutdown(sock, 2);
        close(sock);
        return PSP_NOMEMORY;
    }

    lmcdata->sock = sock;
/*    lmcdata->pipefcn = signal(SIGPIPE, SIG_IGN); */
    if (tokfile[0])
        lmcdata->tokfile = psp_strdup(tokfile, 0);
    else
        lmcdata->tokfile = NULL;

    *data = lmcdata;

    return PSP_OKAY;
}

/* ==========================================================================
   TCP/IP specific listen
*/
static int tcp_listen(
    const char *name)
{
    int                sock;
    int                opt;
    char               hostname[128];
    struct hostent    *host;
    struct servent    *service;
    struct sockaddr_in sockaddr_in;

    sockaddr_in.sin_port = 0;
    if ((name = strchr(name, ':')) != NULL)
        sockaddr_in.sin_port = htons(atoi(name + 1));

    if (sockaddr_in.sin_port == 0 && name &&
             (service = getservbyname(name + 1, "tcp")) != NULL)
        sockaddr_in.sin_port = service->s_port; /* Port from /etc/services */

    if (sockaddr_in.sin_port == 0 &&
            (service = getservbyname("db.star", "tcp")) != NULL)
        sockaddr_in.sin_port = service->s_port; /* Port from /etc/services */

    endservent();

    if (sockaddr_in.sin_port == 0)
        sockaddr_in.sin_port = htons(1523);  /* Default port number */

    if (gethostname(hostname, sizeof(hostname)) != -1 &&
            (host = gethostbyname(hostname)) != NULL) {
        sockaddr_in.sin_family = host->h_addrtype;
        memcpy(&sockaddr_in.sin_addr, host->h_addr, host->h_length);
    }
    else {
        sockaddr_in.sin_family = AF_INET;
        sockaddr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }

    endhostent();

    if (sockaddr_in.sin_addr.s_addr != 0) {
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
            return -1;

        if (connect(sock, (struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in)) != -1) {
            close(sock);
            return -2;
        }

        close(sock);
    }

    sockaddr_in.sin_addr.s_addr = 0;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    if (bind(sock, (struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in)) == -1) {
        close(sock);
        return -1;
    }

    if (listen(sock, 5) == -1) {
        close(sock);
        return -1;
    }

    return sock;
}

#if !defined(NO_UNIX_DOMAIN)
/* ==========================================================================
   Unix Domain sockets specific listen
*/
static int ip_listen(
    const char *name,
    const char *tmp,
    char *tokfile)
{
    int                sock;
    char              *p;
    struct sockaddr_un sockaddr_un;

    sprintf(tokfile, "%s%s", tmp, name);
    if ((p = strchr(tokfile, ':')) != NULL)
        *p = '\0';

    sockaddr_un.sun_family = AF_UNIX; 
    strcpy(sockaddr_un.sun_path, tokfile);
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        return -1;

    if (connect(sock, (struct sockaddr *) &sockaddr_un, sizeof(sockaddr_un)) != -1) {
        close(sock);
        return -2;
    }

    if (bind(sock, (struct sockaddr *) &sockaddr_un, sizeof(sockaddr_un)) == -1) {
       /* Since we could not connect, but the bind failed, there may be an
          old socket file left over - shut it down, delete the file, and
          start again. */
       close(sock);
       unlink(tokfile);

       if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
           return -1;

       if (bind(sock, (struct sockaddr *) &sockaddr_un, sizeof(sockaddr_un)) == -1) {
           close(sock);
           return -1;
       }
    }

    if (listen(sock, 5) == -1) {
        close(sock);
        return -1;
    }

    return sock;
}
#endif

/* ==========================================================================
   Listen for TCP/IP connections
*/
static int psp_tcp_listen(
    const char *name,
    const char *tmp,
    int         maxclients,
    void      **data)
{
    int              sock1;
    int              sock2;
    char            *p;
    char             tokfile[80] = "";
    LMC_SERVER_DATA *lmcdata;

    if ((sock1 = tcp_listen(name)) == -2)
        return PSP_DUPLICATE;

    if (sock1 == -1)
        return PSP_FAILED;

    sock2 = ip_listen(name, tmp, tokfile);

    if ((lmcdata = psp_getMemory(sizeof(LMC_SERVER_DATA), 0)) == NULL) {
        close(sock1);
        if (sock2 >= 0) {
           close(sock2);
           unlink(tokfile);
        }

        return PSP_NOMEMORY;
    }

    lmcdata->clients = psp_cGetMemory(maxclients * sizeof(LMC_CLIENT_DATA), 0);
    if (lmcdata->clients == NULL) {
        psp_freeMemory(lmcdata, 0);
        close(sock1);
        if (sock2 >= 0) {
            close(sock2);
            unlink(tokfile);
        }

        return PSP_NOMEMORY;
    }

    lmcdata->count = 0;
    FD_ZERO(&lmcdata->holdFDS);
    FD_SET(sock1, &lmcdata->holdFDS);
    if (sock2 >= 0)
        FD_SET(sock2, &lmcdata->holdFDS);

    lmcdata->sock1 = sock1;
    lmcdata->sock2 = sock2;
    if (tokfile[0])
        lmcdata->tokfile = psp_strdup(tokfile, 0);
    else
        lmcdata->tokfile = NULL;

    lmcdata->numclients = 0;
    lmcdata->maxclients = maxclients;

    *data = lmcdata;

    return PSP_OKAY;
}

#if !defined(NO_UNIX_DOMAIN)
/* ==========================================================================
   Listen for Unix Domain socket connections
*/
static int psp_ip_listen(
    const char *name,
    const char *tmp,
    int         maxclients,
    void      **data)
{
    int              sock;
    char            *p;
    char             tokfile[80] = "";
    LMC_SERVER_DATA *lmcdata;

    if ((sock = ip_listen(name, tmp, tokfile)) == -2)
        return PSP_DUPLICATE;

    if (sock == -1)
        return PSP_FAILED;

    if ((lmcdata = psp_getMemory(sizeof(LMC_SERVER_DATA), 0)) == NULL) {
        close(sock);
        unlink(tokfile);
        return PSP_NOMEMORY;
    }

    lmcdata->clients = psp_getMemory(maxclients * sizeof(LMC_CLIENT_DATA), 0);
    if (lmcdata->clients == NULL) {
        psp_freeMemory(lmcdata, 0);
        close(sock);
        unlink(tokfile);
        return PSP_NOMEMORY;
    }

    lmcdata->count = 0;
    FD_ZERO(&lmcdata->holdFDS);
    FD_SET(sock, &lmcdata->holdFDS);

    lmcdata->sock1 = sock;
    lmcdata->sock2 = -1;
    if (tokfile[0])
        lmcdata->tokfile = psp_strdup(tokfile, 0);
    else
        lmcdata->tokfile = NULL;

    lmcdata->numclients = 0;
    lmcdata->maxclients = maxclients;

    *data = lmcdata;

    return PSP_OKAY;
}
#endif

/* ==========================================================================
   Handle a new connection
*/
static void new_connection(
    int               listensock,
    LMC_SERVER_DATA  *lmcdata)
{
    int              ii;
    int              len;
    int              sock;
    LMC_CLIENT_DATA *client;

    FD_CLR(listensock, &lmcdata->FDS);
    for (ii = 0; ii < lmcdata->maxclients; ii++) {
        if (!lmcdata->clients[ii])
            break;
    }

    if (ii == lmcdata->maxclients)
        return;

    if  ((client = psp_getMemory(sizeof(LMC_CLIENT_DATA), 0)) == NULL)
        return;

    lmcdata->count--;
    do {
        len = sizeof(saddr);
        sock = accept(listensock, (struct sockaddr *) &client->addrinfo, &len);
        if (sock == -1 && errno != EINTR) {
            psp_freeMemory(client, 0);
            return;
        }
    } while (sock == -1);

    client->sock = sock;
    FD_SET(sock, &lmcdata->holdFDS);
    if (++lmcdata->numclients == lmcdata->maxclients) {
        listen(lmcdata->sock1, 0);
        if (lmcdata->sock2 >= 0)
            listen(lmcdata->sock2, 0);
    }

    lmcdata->clients[ii] = client;
}

/* ==========================================================================
   Check for a message or new connection
*/
static void *handle_data(
    LMC_SERVER_DATA *lmcdata)
{
    int               ii;
    LMC_CLIENT_DATA **clients;

    if (lmcdata->sock1 != -1 && FD_ISSET(lmcdata->sock1, &lmcdata->FDS))
        new_connection(lmcdata->sock1, lmcdata);

    if (lmcdata->sock2 >= 0 && FD_ISSET(lmcdata->sock2, &lmcdata->FDS))
        new_connection(lmcdata->sock2, lmcdata);

    if (lmcdata->count) {
        clients = lmcdata->clients;
        for (ii = 0; ii  < lmcdata->maxclients; ii++) {
            if (clients[ii] && FD_ISSET(clients[ii]->sock, &lmcdata->FDS)) {
                FD_CLR(lmcdata->clients[ii]->sock, &lmcdata->FDS);
                lmcdata->count--;
                return clients[ii];
            }
        }
    }

    lmcdata->count = 0;  /* just to be sure */

    return NULL; /* timeout */
}

/* ==========================================================================
   Wait for a message
*/
static void *psp_socket_waitmsg(
    void   *data,
    long    timeout)
{
    struct timeval   tmout;
    LMC_SERVER_DATA *lmcdata = (LMC_SERVER_DATA *) data;

    if (!data)
        return NULL;

    if (lmcdata->count)
        return handle_data(lmcdata);

    tmout.tv_sec = timeout / 1000;
    tmout.tv_usec = (timeout % 1000) * 1000;
    memcpy(&lmcdata->FDS, &lmcdata->holdFDS, sizeof(fd_set));
    lmcdata->count = select(FD_SETSIZE, &lmcdata->FDS, NULL, NULL, &tmout);
    if (lmcdata->count == -1)
        return NULL;

    if (lmcdata->count)
        return handle_data(lmcdata);

    return NULL; /* timeout */
}

/* ==========================================================================
   Shut down the listen
*/
static void psp_socket_stoplisten(
    void *data)
{
    int               ii;
    LMC_SERVER_DATA  *lmcdata;
    LMC_CLIENT_DATA **clients;

    if (!data)
        return;

    lmcdata = (LMC_SERVER_DATA *) data;
    if (lmcdata->numclients > 0) {
        clients = lmcdata->clients;
        for (ii = 0; ii < lmcdata->maxclients; ii++) {
            if (clients[ii]) {
               close(clients[ii]->sock);
               psp_freeMemory(clients[ii], 0);
            }
        }
    }

    close(lmcdata->sock1);
    if (lmcdata->sock2 >= 0)
        close(lmcdata->sock2);

    if (lmcdata->tokfile) {
        unlink(lmcdata->tokfile);
        psp_freeMemory(lmcdata->tokfile, 0);
    }

    psp_freeMemory(lmcdata->clients, 0);
    psp_freeMemory(lmcdata, 0);
}

/* ==========================================================================
   Close the client connection
*/
static void psp_socket_disconclient(
    void  *data,
    void  *cdata)
{
    int ii;
    LMC_SERVER_DATA *lmcdata;
    LMC_CLIENT_DATA *client;

    if (!data || !cdata)
        return;

    lmcdata = (LMC_SERVER_DATA *) data;
    client = (LMC_CLIENT_DATA *) cdata;

    close(client->sock);
    FD_CLR(client->sock, &lmcdata->holdFDS);
    if (lmcdata->numclients-- == lmcdata->maxclients) {
        listen(lmcdata->sock1, 5);
        if (lmcdata->sock2 >= 0)
            listen(lmcdata->sock2, 5);
    }

    if (FD_ISSET(client->sock, &lmcdata->FDS)) {
        FD_CLR(client->sock, &lmcdata->FDS);
        lmcdata->count--;
    }

    for (ii = 0; ii < lmcdata->maxclients; ii++) {
        if (client == lmcdata->clients[ii]) {
            psp_freeMemory(client, 0);
            lmcdata->clients[ii] = NULL;
        }
    }
}

/* ==========================================================================
   Close the connection
*/
static void psp_socket_disconnect(
    void *data)
{
    LMC_CLIENT_DATA *lmcdata;

    if (!data)
        return;

    lmcdata = (LMC_CLIENT_DATA *) data;

    shutdown(lmcdata->sock, 2);
    close(lmcdata->sock);
/*    signal(SIGPIPE, lmcdata->pipefcn); */

    if (lmcdata->tokfile)
        psp_freeMemory(lmcdata->tokfile, 0);

    psp_freeMemory(lmcdata, 0);
}

/* ==========================================================================
   Send a message on a socket
*/
static int socket_send(
    const MSG_HDR  *hdr,
    const void     *msg,
    int             len,
    int             sock)
{
    int         bytes;
    const char *cp;

    while (send(sock, (const char *) hdr, sizeof(MSG_HDR), 0) == -1) {
        if (errno != EINTR)
            return PSP_DISCONNECTED;
    }

    cp = msg;
    while (len) {
        if ((bytes = send(sock, cp, len, 0)) == -1) {
            if (errno == EINTR)
                continue;

            return PSP_DISCONNECTED;
        }

        len -= bytes;
        cp += bytes;
    }

    return PSP_OKAY;
}

/* ==========================================================================
   TCP/IP specific send function
*/
static int psp_tcp_send(
    const void *msg,
    size_t      len,
    const void *data)
{
    MSG_HDR hdr;

    hdr.db_star_id = htonl(DB_STAR_ID);
    hdr.msglen     = htonl(len);
   
    return socket_send(&hdr, msg, (int) len, ((LMC_CLIENT_DATA *) data)->sock);
}

/* ==========================================================================
   Receive data from a socket
*/
static int socket_receive(
    void *msg,
    int   len,
    int   sock)
{
    char *cp = msg;
    int   bytes;

    do
    {
        if ((bytes = recv(sock, cp, len, 0)) <= 0) {
            if (errno == EINTR)
                continue;

            return PSP_DISCONNECTED;
        }

        len -= bytes;
        cp += bytes;
    } while (len);

    return PSP_OKAY;
}

/* ==========================================================================
   TCP/IP specific receive function
*/
static int psp_tcp_receive(
    void      **msg,
    size_t     *len,
    const void *data)
{
    int     stat;
    int     sock = ((LMC_CLIENT_DATA *) data)->sock;
    MSG_HDR hdr;

    if ((stat = socket_receive(&hdr, sizeof(hdr), sock)) != PSP_OKAY)
        return stat;

    if (ntohl(hdr.db_star_id) != DB_STAR_ID)
        return PSP_INVMSG;

    *len = ntohl(hdr.msglen);
    if ((*msg = psp_getMemory(*len, 0)) == NULL)
        return PSP_NOMEMORY;

    return socket_receive(*msg, (int) *len, sock);
}

static void psp_socket_info(
    const void *data,
    char       *line1,
    char       *line2,
    char       *line3)
{
    saddr *sa = &((LMC_CLIENT_DATA *) data)->addrinfo;

    if (sa->in.sin_family == AF_INET) {
        strcpy(line1, "TCP/IP socket connection");
        sprintf(line2, "Address: %s", inet_ntoa(sa->in.sin_addr));
    }
    else {
        strcpy(line1, "Unix domain socket connection");
        line2[0] = '\0';
    }

    line3[0] = '\0';
} 

#if !defined(NO_UNIX_DOMAIN) 
/* ==========================================================================
   Check to see if IP (Unix Domain sockets) is available
*/
int psp_ip_avail(
    int            *flags,
    LOCKCOMM_FCNS **fcns)
{
    *fcns = &ip_lockcomm;
    return PSP_OKAY;
}

/* ==========================================================================
   IP (Unix Domain sockets) specific connect
*/
static int ip_connect(
    const char *name,
    const char *tmp,
    char       *tokfile,
    int         timeout)
{
    int                ii;
    int                sock;
    int                addr_len;
    struct protoent   *protocol;
    struct sockaddr   *sockaddr = NULL;
    struct sockaddr_un sockaddr_un;

    /* Set up the token file for local IP attempt */
    strcpy(tokfile, tmp);
    strcat(tokfile, name);

    protocol = getprotobyname("ip");
    sockaddr = (struct sockaddr *) &sockaddr_un;
    addr_len = sizeof(sockaddr_un.sun_family) + strlen(tokfile) + 1;

    sockaddr_un.sun_family = AF_UNIX;
    strncpy(sockaddr_un.sun_path, tokfile, sizeof(sockaddr_un.sun_path));
    sockaddr_un.sun_path[sizeof(sockaddr_un.sun_path) - 1] = '\0';

    /* The first connect may fail if there is a lot of activity on
       the lockmgr. */
    sock = socket(sockaddr_un.sun_family, SOCK_STREAM, protocol->p_proto);
    if (sock != -1) {
        for (ii = 0; ii < timeout; ii++) {
            if (connect(sock, sockaddr, addr_len) != -1)
                break;

            /* TBD: Log out error */
/*            printf("ip connect fail: %s (%d)\n", strerror(errno),
                    errno); */

            psp_sleep(1);
        }
            
        if (ii == timeout) {
            close(sock);
            return -1;
        }
    }

    return sock;
}

/* ==========================================================================
   Open a connection using IP (Unix Domain sockets)
*/
static int psp_ip_connect(
    const char *lockmgr,
    const char *tmp,
    const char *id,
    void      **data)
{
    int              ii;
    int              sock = -1;
    char             tokfile[80];
    LMC_CLIENT_DATA *lmcdata;

    /* TBD: Need to get timeout value (net_timeout) from rdm.ini. */
    int              timeout = 5;

    if ((sock = ip_connect(lockmgr, tmp, tokfile, timeout)) == -1)
        return PSP_FAILED;

    /* Allocate control data buffer */
    lmcdata = (LMC_CLIENT_DATA *) psp_cGetMemory(sizeof(LMC_CLIENT_DATA), 0);
    if (lmcdata == NULL)
        return PSP_NOMEMORY;

    lmcdata->sock = sock;
/*    lmcdata->pipefcn = signal(SIGPIPE, SIG_IGN); */
    lmcdata->tokfile = psp_strdup(tokfile, 0);
    *data = lmcdata;

    return PSP_OKAY;
}

/* ==========================================================================
   IP (Unix Domain sockets) specific send
*/
static int psp_ip_send(
    const void *msg,
    size_t      len,
    const void *data)
{
    MSG_HDR hdr;

    hdr.db_star_id = DB_STAR_ID;
    hdr.msglen     = len;
   
    return socket_send(&hdr, msg, (int) len, ((LMC_CLIENT_DATA *) data)->sock);
}

/* ==========================================================================
   IP (Unix Domain sockets) specific send
*/
static int psp_ip_receive(
    void      **msg,
    size_t     *len,
    const void *data)
{
    int     stat;
    int     sock = ((LMC_CLIENT_DATA *) data)->sock;
    MSG_HDR hdr;
  
    if ((stat = socket_receive(&hdr, sizeof(hdr), sock)) != PSP_OKAY)
        return stat;

    if (hdr.db_star_id != DB_STAR_ID)
        return PSP_INVMSG;

    *len = hdr.msglen;
    if ((*msg = psp_getMemory(*len, 0)) == NULL)
        return PSP_NOMEMORY;

    return socket_receive(*msg, (int) *len, sock);
}
#endif
