/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, lock manager ping utility                         *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "db.star.h"
#include "version.h"
#include "psp.h"
#include "dblock.h"

static void usage(void);
static int  get_args(int argc, char *argv[], int *count, int *pktsize,
                     char *pUser, char *pLockmgrn, char *pTmp,
                     LMC_AVAIL_FCN **pAvail, char *pType);

#define C_FAIL 1
#define C_OKAY 0

int main(int argc, char *argv[])
{
    int     ii;
    int     count = 5;
    int     pktsize = 0;
    PSP_LMC mylmc;
    int     err;
    size_t  i;
    char   *send_pkt;
    size_t  send_size;
    char   *recv_pkt;
    size_t  recv_size;
    int     flags;
    int     status;
    char    name[80] = DB_TEXT("");
    char    typestr[LMCTYPELEN];
    char    lockmgrn[LOCKMGRLEN];
    char    tmp[DB_PATHLEN] = DB_TEXT("");
    char   *ptr;
    PSP_INI ini;
    LOCKCOMM_FCNS *fcns;
    LMC_AVAIL_FCN *avail = NULL;

    srand(time(NULL));

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("lm Ping")));

    if ((err=psp_init()) != PSP_OKAY) {
         printf("Error %d in psp_lmcSetup\n", err);
         return -1;
    }

    status = get_args(argc, argv, &count, &pktsize, name, lockmgrn, tmp,
                      &avail, typestr);
    if (status != C_OKAY)
    {
        psp_term();
        return status;
    }

    if (!name[0])
        strcpy(name, "PingUser");

    if (!tmp[0]) {
        if ((ptr = psp_getenv(DB_TEXT("DBTMP"))) != NULL) {
            vtstrncpy(tmp, ptr, DB_PATHLEN);
            tmp[DB_PATHLEN - 1] = DB_TEXT('\0');
        }
    }

    if (!lockmgrn[0]) {
        if ((ptr = psp_getenv(DB_TEXT("LOCKMGR"))) != NULL)
        {
            vtstrncpy(lockmgrn, ptr, LOCKMGRLEN);
            lockmgrn[LOCKMGRLEN - 1] = DB_TEXT('\0');
        }
    }

    if ((ptr = psp_getenv(DB_TEXT("DBINI"))) == NULL)
        ptr = DB_TEXT("./rdm.ini");

    if ((ini = psp_iniOpen(ptr)) != NULL) {
        if (!lockmgrn[0]) {
            psp_iniString(ini, DB_TEXT("lockmgr"), DB_TEXT("name"),
                psp_defLockmgr(), lockmgrn, sizeof(lockmgrn));
        }

        if (!tmp[0]) {
            psp_iniString(ini, DB_TEXT("db.*"), DB_TEXT("dbtmp"),
            psp_pathDefTmp(), tmp, sizeof(tmp));
        }

        if (!avail) {
            psp_iniString(ini, DB_TEXT("lockmgr"), DB_TEXT("type"),
                DB_TEXT("none"), typestr, sizeof(typestr));
            avail = psp_lmcFind(typestr);
        }

        psp_iniClose(ini);
    }

    if (!lockmgrn[0]) {
        vtstrncpy(lockmgrn, psp_defLockmgr(), sizeof(lockmgrn));
        lockmgrn[LOCKMGRLEN - 1] = DB_TEXT('\0');
    }

    if (!tmp[0]) {
        vtstrncpy(tmp, psp_pathDefTmp(), sizeof(tmp));
        tmp[DB_PATHLEN - 1] = DB_TEXT('\0');
    }

    if (tmp[0]) {
        if (tmp[vtstrlen(tmp) - 1] != DIRCHAR) {
             tmp[vtstrlen(tmp) + 1] = DB_TEXT('\0');
             tmp[vtstrlen(tmp)] = DIRCHAR;
        }
    }

    if (!avail) {
        vtstrcpy(typestr, DB_TEXT("TCP"));
        avail = psp_lmcFind(typestr);
    }

    if ((err = psp_lmcSetup(&mylmc, avail)) != PSP_OKAY) {
         printf("Failed to setup lmc structure\n");
         psp_term();
         return -1;
    }

    if ((err = psp_lmcConnect(lockmgrn, name, tmp, (void **)mylmc))
        != PSP_OKAY) {
        printf("Failed to connect, status=%d\n", err);
        psp_lmcCleanup(mylmc);
        return err;
    }

    /* Main loop sending packets to LM. */
    for (ii = 0; ii < count; ii++) {
        if (pktsize)
            send_size = pktsize;
        else
            send_size = rand() % 1000;
        send_pkt = psp_lmcAlloc(send_size);
        for (i=0; i<send_size; i++)
             send_pkt[i] = (char)(send_size-i);
        if ((err = psp_lmcTrans(L_PING, send_pkt, send_size,
                                (void **)&recv_pkt, &recv_size, &status,
                                (void**)mylmc)) != PSP_OKAY) {
            printf("Failed transaction, status=%d\n",err);
            return err;
        }

        if (send_size != recv_size || memcmp(send_pkt, recv_pkt, send_size))
             printf("Bad ping: %d == %d?\n", send_size, recv_size);
        else
             printf("%d ", recv_size);
        if ( (ii+1)%10 == 0 )
            printf("\n");
        fflush(stdout);
        psp_lmcFree(recv_pkt);
    }

    printf("\nGood transmits = %d\n",ii);

    psp_lmcDisconnect((void**)mylmc);
    psp_lmcCleanup((void**)mylmc);
    psp_term();
    
    return 0;
}

/*************************************************************************/

static int get_args(
    int             argc,
    char           *argv[],
    int            *count,
    int            *pktSize,
    char           *pUser,
    char           *pLockmgrn,
    char           *pTmp,
    LMC_AVAIL_FCN **pAvail,
    char           *pType)
{
    int i;
    DB_TCHAR *p = DB_TEXT("");

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != DB_TEXT('-'))
        {
            usage();
            return C_FAIL;
        }

        switch (vtotupper(argv[i][1]))
        {
            case DB_TEXT('A'):
                if ((i + 1 < argc) && (argv[i + 1][0] != DB_TEXT('-')))
                {
                    vtstrncpy(pLockmgrn, argv[++i], LOCKMGRLEN);
                    pLockmgrn[LOCKMGRLEN - 1] = DB_TEXT('\0');
                }
                else
                    goto fail;
                break;
  
            case DB_TEXT('C'):
                *count = atoi(argv[++i]);
                if (*count <= 0 || *count > 30000)
                    /* use a sane number */
                    *count = 5;
                break;

            case DB_TEXT('M'):
                switch(vtotupper(argv[i][2]) )
                {
                    case DB_TEXT('G'):
                        *pAvail = psp_lmcFind(p = DB_TEXT("General"));
                        break;
  
                    case DB_TEXT('T'):
                        *pAvail = psp_lmcFind(p = DB_TEXT("TCP"));
                        break;
  
                    case DB_TEXT('P'):
                        *pAvail = psp_lmcFind(p = DB_TEXT("IP"));
                        break;

                    case DB_TEXT('Q'):
                        *pAvail = psp_lmcFind(p = DB_TEXT("QNX"));
                        break;
  
                    default:
                        goto fail;
                }
  
                if (*pAvail)
                {
                    vtstrcpy(pType, p);
                } 
                else
                {
                    vtprintf(DB_TEXT("Invalid lock manager transport type\n"));
                    goto fail;
                }
  
                break;
  
            case DB_TEXT('S'):
                *pktSize = atoi(argv[++i]);
                if (*pktSize <= 0 || *pktSize > 30000)
                    /* use a sane number */
                    *pktSize = 200;
                break;

            case DB_TEXT('U'):
                if ((i + 1 < argc) && (argv[i + 1][0] != DB_TEXT('-')))
                    vtstrcpy(pUser, argv[++i]);
                else
                    goto fail;
                break;

            case DB_TEXT('Z'):
                if ((i + 1 < argc) && (argv[i + 1][0] != DB_TEXT('-')))
                {
                    vtstrncpy(pTmp, argv[++i], DB_PATHLEN);
                    pTmp[DB_PATHLEN - 1] = DB_TEXT('\0');
                }
                else
                    goto fail;
                break;
  
            default:
                goto fail;
        }
    }
    return C_OKAY;

fail:
    usage();
    return C_FAIL;
}

/*************************************************************************/

static void usage()
{
    vtprintf(DB_TEXT("\nusage:  lmping [-a lmname] [-c count] [-s packetsize] [-m{t|p}] [-u username] [-z dir]\n\n"));
    vtprintf(DB_TEXT("       where -a  specifies the lock manager name\n"));
    vtprintf(DB_TEXT("             -c  count of transactions\n"));
    vtprintf(DB_TEXT("             -mt use the TCP lock manager (default)\n"));
    vtprintf(DB_TEXT("             -mp use the IP lock manager (if available)\n"));
    vtprintf(DB_TEXT("             -s  size of packet\n"));
    vtprintf(DB_TEXT("             -u  specifies the console's user id\n"));
    vtprintf(DB_TEXT("             -z  specifies temporary directory\n"));
}
