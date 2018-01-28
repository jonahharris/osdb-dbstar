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

#define MOD console
#include "db.star.h"
#include "version.h"

#include <signal.h>
#include "console.h"

static void    usage(void);
static int     get_args(int, char **, int *, DB_BOOLEAN *,
                        char *, char *, char *, LMC_AVAIL_FCN **, char *);

static PSP_LMC open_lmc(const char *, const char *, const char *, LMC_AVAIL_FCN *);
static void    close_lmc();


int            refresh_secs = 2;
char           lockmgrn[LOCKMGRLEN];
char          *title1 = DBSTAR_LABEL;
char           title2[80];
PSP_LMC        lmc;


#define C_FAIL 1
#define C_OKAY 0

/*************************************************************************/

int MAIN(int argc, char *argv[])
{
    int            stat;
    DB_BOOLEAN     terminate = FALSE;
    char           user[80] = DB_TEXT("");
    char           tmp[DB_PATHLEN] = DB_TEXT("");
    char           typestr[LMCTYPELEN];
    char          *ptr;
    PSP_INI        ini;
    LMC_AVAIL_FCN *avail = NULL;

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Lock Manager Console")));

    if ((stat = psp_init()) != PSP_OKAY)
        return stat;

    stat = get_args(argc, argv, &refresh_secs, &terminate, user, lockmgrn, tmp, &avail, typestr);
    if (stat != C_OKAY)
    {
        psp_term();
        return stat;
    }

    if (!tmp[0])
    {
        if ((ptr = psp_getenv(DB_TEXT("DBTMP"))) != NULL)
        {
            vtstrncpy(tmp, ptr, DB_PATHLEN);
            tmp[DB_PATHLEN - 1] = DB_TEXT('\0');
        }
    }

    if (!lockmgrn[0])
    {
        if ((ptr = psp_getenv(DB_TEXT("LOCKMGR"))) != NULL)
        {
            vtstrncpy(lockmgrn, ptr, LOCKMGRLEN);
            lockmgrn[LOCKMGRLEN - 1] = DB_TEXT('\0');
        }
    }

    if ((ptr = psp_getenv(DB_TEXT("DBINI"))) == NULL)
        ptr = DB_TEXT("./rdm.ini");

    if ((ini = psp_iniOpen(ptr)) != NULL)
    {
        if (!lockmgrn[0])
        {
            psp_iniString(ini, DB_TEXT("lockmgr"), DB_TEXT("name"),
                psp_defLockmgr(), lockmgrn, sizeof(lockmgrn));
        }

        if (!tmp[0])
        {
            psp_iniString(ini, DB_TEXT("db.*"), DB_TEXT("dbtmp"),
            psp_pathDefTmp(), tmp, sizeof(tmp));
        }

        if (!avail)
        {
            psp_iniString(ini, DB_TEXT("lockmgr"), DB_TEXT("type"),
                DB_TEXT("none"), typestr, sizeof(typestr));
            avail = psp_lmcFind(typestr);
        }

        psp_iniClose(ini);
    }

    if (!lockmgrn[0])
    {
        vtstrncpy(lockmgrn, psp_defLockmgr(), sizeof(lockmgrn));
        lockmgrn[LOCKMGRLEN - 1] = DB_TEXT('\0');
    }

    if (!tmp[0])
    {
        vtstrncpy(tmp, psp_pathDefTmp(), sizeof(tmp));
        tmp[DB_PATHLEN - 1] = DB_TEXT('\0');
    }

    if (tmp[0])
    {
        if (tmp[vtstrlen(tmp) - 1] != DIRCHAR)
        {
             tmp[vtstrlen(tmp) + 1] = DB_TEXT('\0');
             tmp[vtstrlen(tmp)] = DIRCHAR;
        }
    }

    if (!avail)
    {
        vtstrcpy(typestr, DB_TEXT("TCP"));
        avail = psp_lmcFind(typestr);
    }

    if ((lmc = open_lmc(lockmgrn, user, tmp, avail)) == NULL)
    {
        vtprintf(DB_TEXT("Failed to connect to the lock manager\n"));
        psp_term();
        return C_FAIL;
    }
   
    vstprintf(title2, DB_TEXT("Console for the db.* %s Lock Manager"), typestr);

    signal(SIGQUIT, console_cleanup);
    signal(SIGHUP,  console_cleanup);
    signal(SIGBUS,  console_cleanup);
    signal(SIGINT,  console_cleanup);
    signal(SIGSEGV, console_cleanup);
    signal(SIGTRAP, console_cleanup);
 
    if (terminate)
        stat = kill_lockmgr();
    else
    {
        if (get_display_memory() == S_OKAY)
        {
            /* initialize display */
            vioInit();

            /* main processing function */
            console();

            /* terminate / free memory */
            vioTerm();
            free_table_memory();
            free_display_memory();
        }
        else
            vtprintf(DB_TEXT("Out of memory!\n"));
    }

    close_lmc();
    psp_term();

    return stat;
}

/*************************************************************************/

static PSP_LMC open_lmc(const char *name,
                        const char *userid,
                        const char *tmp,
                        LMC_AVAIL_FCN *avail)
{
    char console_name[80];
    int stat, i = 0;

    if ((stat = psp_lmcSetup(&lmc, avail)) != PSP_OKAY)
        return NULL;

    if (userid[0])
    {
        vtstrncpy(console_name, userid, sizeof(console_name) - 1);
        console_name[sizeof(console_name) - 1] = 0;
    }
    else
    {
        do
        {
            vstprintf(console_name, DB_TEXT("Consolel%d"), i);
            stat = psp_lmcConnect(name, console_name, tmp, lmc);

        } while (stat == PSP_DUPUSERID && ++i < 10);
    }

    if (stat != PSP_OKAY)
    {
        psp_lmcCleanup(lmc);
        return NULL;
    }

    return lmc;
}

/*************************************************************************/

static void close_lmc()
{
    if (lmc)
    {
        psp_lmcDisconnect(lmc);
        psp_lmcCleanup(lmc);
        lmc = NULL;
    }
}

/*************************************************************************/

int kill_lockmgr()
{
    int stat;

    stat = psp_lmcTrans(L_TERMINATE, NULL, 0, NULL, NULL, NULL, lmc);
    if (stat != PSP_OKAY)
    {
        error_msg(DB_TEXT("Unable to terminate lock manager.\n"));
        return C_FAIL;
    }

    error_msg(DB_TEXT("Lock manager terminated.\n"));
    return C_OKAY;
}

/*************************************************************************/

int kill_user(DB_TCHAR *user)
{
    int stat;
    LM_USERID *cu = NULL;

    if ((cu = (LM_USERID *) psp_lmcAlloc(sizeof(LM_USERID))) == NULL)
    {
        error_msg(DB_TEXT("ERROR: Out of memory"));
        return C_FAIL;
    }

    vtstrcpy(cu->dbuserid, user);

    stat = psp_lmcTrans(L_CLEARUSER, cu, sizeof(LM_USERID), NULL, NULL, NULL,
            lmc);
    psp_lmcFree(cu);
    if (stat != S_OKAY)
    {
        error_msg(DB_TEXT("ERROR: Unable to clear user"));
        return C_FAIL;
    }

    return C_OKAY;
}

/*************************************************************************/

static int get_args(
    int             argc,
    char           *argv[],
    int            *pRefresh,
    DB_BOOLEAN     *pTerminate,
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
  
            case DB_TEXT('R'):
                *pRefresh = vttoi(&argv[i][2]);
                if (*pRefresh < 2 || *pRefresh > 59)
                    goto fail;
                break;
  
            case DB_TEXT('S'):
                *pTerminate = TRUE;
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
    vtprintf(DB_TEXT("\nusage:  console [-a lmname] [-m{g|t|p|q}] [-r#] [-s] [-u username] [-z dir]\n\n"));
    vtprintf(DB_TEXT("       where -a  specifies the lock manager name\n"));
    vtprintf(DB_TEXT("             -mg use the general lock manager\n"));
    vtprintf(DB_TEXT("             -mt use the TCP lock manager (default)\n"));
    vtprintf(DB_TEXT("             -mp use the IP lock manager (if available)\n"));
    vtprintf(DB_TEXT("             -mq use the QNX lock manager (if available)\n"));
    vtprintf(DB_TEXT("             -r  specifies seconds (2 to 59) between screen refreshes\n"));
    vtprintf(DB_TEXT("             -s  shuts down the lock manager\n"));
    vtprintf(DB_TEXT("             -u  specifies the console's user id\n"));
    vtprintf(DB_TEXT("             -z  specifies temporary directory\n"));
}

/*************************************************************************/

void console_cleanup(int sig)
{
    vioTerm();
    free_table_memory();
    free_display_memory();
    close_lmc();
    psp_term();
}

