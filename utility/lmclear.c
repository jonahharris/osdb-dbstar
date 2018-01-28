/*-----------------------------------------------------------------------

   lmclear.c -- lockmgr clear utility

   Copyright (c) 1999, Centura Software Corporation.  All Rights Reserved.

-----------------------------------------------------------------------*/

#define MOD lmclear
#include "db.star.h"
#include "version.h"

static void    usage(void);
static int     get_args(int, DB_TCHAR **, DB_BOOLEAN *, DB_BOOLEAN *,
                        DB_TCHAR *, DB_TCHAR *, LMC_AVAIL_FCN **);
static int     Kill_Lockmgr(PSP_LMC);
static int     Clear_User(DB_TCHAR *, PSP_LMC);
static int     DispStats(PSP_LMC);

static PSP_LMC OpenLMC(const DB_TCHAR *, const DB_TCHAR *, LMC_AVAIL_FCN *);
static void    CloseLMC(PSP_LMC lmc);

#define C_FAIL 1
#define C_OKAY 0

/*************************************************************************/

int MAIN(int argc, DB_TCHAR *argv[])
{
    int            stat;
    DB_BOOLEAN     Terminate = FALSE;
    DB_BOOLEAN     Display = FALSE;
    DB_TCHAR       User[80] = DB_TEXT("");
    DB_TCHAR       Lockmgrn[LOCKMGRLEN] = DB_TEXT("");
    DB_TCHAR       Tmp[DB_PATHLEN] = DB_TEXT("");
    DB_TCHAR       typestr[LMCTYPELEN];
    DB_TCHAR      *ptr;
    PSP_INI        ini;
    PSP_LMC        lmc;
    LMC_AVAIL_FCN *avail = NULL;
 
    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Lock Manager Clear")));

    if ((stat = psp_init()) != PSP_OKAY)
        return stat;

    stat = get_args(argc, argv, &Terminate, &Display, User, Lockmgrn, &avail);
    if (stat != C_OKAY)
    {
        psp_term();
        return stat;
    }

    if ((ptr = psp_getenv(DB_TEXT("DBTMP"))) != NULL)
    {
        vtstrncpy(Tmp, ptr, DB_PATHLEN);
        Tmp[DB_PATHLEN - 1] = DB_TEXT('\0');
    }

    if (!Lockmgrn[0])
    {
        if ((ptr = psp_getenv(DB_TEXT("LOCKMGR"))) != NULL)
        {
            vtstrncpy(Lockmgrn, ptr, LOCKMGRLEN);
            Lockmgrn[LOCKMGRLEN - 1] = DB_TEXT('\0');
        }
    }

    if ((ptr = psp_getenv(DB_TEXT("DBINI"))) == NULL)
        ptr = DB_TEXT("./dbstar.ini");

    if ((ini = psp_iniOpen(ptr)) != NULL)
    {
        if (!Lockmgrn[0])
        {
            psp_iniString(ini, DB_TEXT("lockmgr"), DB_TEXT("name"),
                    psp_defLockmgr(), Lockmgrn, sizeof(Lockmgrn));
        }

        if (!Tmp[0])
        {
            psp_iniString(ini, DB_TEXT("db.*"), DB_TEXT("dbtmp"),
                    psp_pathDefTmp(), Tmp, sizeof(Tmp));
        }

        if (!avail)
        {
            psp_iniString(ini, DB_TEXT("lockmgr"), DB_TEXT("type"),
                    DB_TEXT("none"), typestr, sizeof(typestr));
            avail = psp_lmcFind(typestr);
        }

        psp_iniClose(ini);
    }

    if (!Lockmgrn[0])
    {
        vtstrncpy(Lockmgrn, psp_defLockmgr(), sizeof(Lockmgrn));
        Lockmgrn[LOCKMGRLEN - 1] = DB_TEXT('\0');
    }

    if (!Tmp[0])
    {
        vtstrncpy(Tmp, psp_pathDefTmp(), sizeof(Tmp));
        Tmp[DB_PATHLEN - 1] = DB_TEXT('\0');
    }

    if (Tmp[0])
    {
        if (Tmp[vtstrlen(Tmp) - 1] != DIRCHAR)
        {
             Tmp[vtstrlen(Tmp) + 1] = DB_TEXT('\0');
             Tmp[vtstrlen(Tmp)] = DIRCHAR;
        }
    }

    if (!avail)
        avail = psp_lmcFind("TCP");

    if ((lmc = OpenLMC(Lockmgrn, Tmp, avail)) == NULL)
    {
        vtprintf(DB_TEXT("Failed to connect to the lock manager\n"));
        psp_term();
        return C_FAIL;
    }
   
    if (Terminate)
        stat = Kill_Lockmgr(lmc);
    else if (Display)
        stat = DispStats(lmc);
    else
        stat = Clear_User(User, lmc);
 
    CloseLMC(lmc);
    psp_term();

    return stat;
}

/*************************************************************************/

static PSP_LMC OpenLMC(const DB_TCHAR *name, const DB_TCHAR *tmp,
                       LMC_AVAIL_FCN *avail)
{
    DB_TCHAR lmclear_name[10];
    PSP_LMC  lmc;
    int stat, i = 0;

    if ((stat = psp_lmcSetup(&lmc, avail)) != PSP_OKAY)
        return NULL;

    do
    {
        vstprintf(lmclear_name, DB_TEXT("LMCLEAR%d"), i);

        stat = psp_lmcConnect(name, lmclear_name, tmp, lmc);
    } while (stat == PSP_DUPUSERID && ++i < 10);

    if (stat != PSP_OKAY)
    {
        psp_lmcCleanup(lmc);
        return NULL;
    }

    return lmc;
}

/*************************************************************************/

static void CloseLMC(PSP_LMC lmc)
{
    psp_lmcDisconnect(lmc);
    psp_lmcCleanup(lmc);
}

/*************************************************************************/

static int Kill_Lockmgr(PSP_LMC lmc)
{
    int stat;

    stat = psp_lmcTrans(L_TERMINATE, NULL, 0, NULL, NULL, NULL, lmc);
    if (stat != PSP_OKAY)
    {
        vtprintf(DB_TEXT("Unable to terminate lock manager.\n"));
        return stat;
    }

    vtprintf(DB_TEXT("Lock manager terminated.\n"));
    return PSP_OKAY;
}

/*************************************************************************/

static int DispStats(PSP_LMC lmc)
{
    LM_STATUS     *ssp;
    LR_STATUS     *rsp;
    LR_USERINFO   *rup;
    unsigned short i;
    int            stat;

    if ((ssp = (LM_STATUS *) psp_lmcAlloc(sizeof(LM_STATUS))) == NULL)
    {
        vtprintf(DB_TEXT("Out of memory\n"));
        return PSP_NOMEMORY;
    }

    ssp->type_of_status = ST_GENSTAT;
    stat = psp_lmcTrans(L_STATUS, ssp, sizeof(LM_STATUS), (void **) &rsp, NULL,
            NULL, lmc);
    if (stat != PSP_OKAY)
    {
        psp_lmcFree(ssp);
        return stat;
    }

    ssp->type_of_status = ST_USERINFO;
    memcpy(&ssp->tabsize, &rsp->s_tabsize, sizeof(TABSIZE));
    vtprintf(DB_TEXT("  USERS              NETWORK NODE INFORMATION\n"));
    vtprintf(DB_TEXT("  ----------------   ------------------------\n"));

    for (i = 0; i < rsp->s_tabsize.lm_numusers; i++)
    {
        ssp->number = i;
        stat = psp_lmcTrans(L_STATUS, ssp, sizeof(LM_STATUS), (void **) &rup,
                NULL, NULL, lmc);
        if (stat != PSP_OKAY)
        {
             psp_lmcFree(ssp);
             return stat;
        }

        if (rup->ui_status != U_EMPTY && rup->ui_status != U_DEAD)
        {
            vtprintf(DB_TEXT("  %-16s: %s\n"), rup->ui_name, rup->ui_netinfo1);
            if (rup->ui_netinfo2[0])
                vtprintf(DB_TEXT("                    %s\n"), rup->ui_netinfo2);

            if (rup->ui_netinfo3[0])
                vtprintf(DB_TEXT("                    %s\n"), rup->ui_netinfo3);

            vtprintf(DB_TEXT("\n"));
        }

        psp_lmcFree(rup);
    }

    psp_lmcFree(ssp);

    return PSP_OKAY;
}

/*************************************************************************/

static int Clear_User(DB_TCHAR *name, PSP_LMC lmc)
{
    LM_USERID *cu = NULL;
    LM_STATUS *ssp;
    short     *rus;
    int        stat;
 
    if ((ssp = (LM_STATUS *) psp_lmcAlloc(sizeof(LM_STATUS))) == NULL)
    {
        vtprintf(DB_TEXT("Out of memory\n"));
        return PSP_NOMEMORY;
    }

    vtstrcpy(ssp->username, name);
    ssp->type_of_status = ST_USERSTAT;
 
    stat = psp_lmcTrans(L_STATUS, ssp, sizeof(LM_STATUS), (void **) &rus, NULL,
            NULL, lmc);
    psp_lmcFree(ssp);
    if (stat != S_OKAY)
        return stat;

    if (*rus == U_EMPTY)
    {
        vtprintf(DB_TEXT("User %s does not exist in lock manager.\n"), name);
        psp_lmcFree(rus);
        return PSP_FAILED;
    }

    psp_lmcFree(rus);

    if ((cu = (LM_USERID *) psp_lmcAlloc(sizeof(LM_USERID))) == NULL)
    {
        vtprintf(DB_TEXT("Out of memory\n"));
        return PSP_NOMEMORY;
    }

    vtstrcpy(cu->dbuserid, name);

    stat = psp_lmcTrans(L_CLEARUSER, cu, sizeof(LM_USERID), NULL, NULL, NULL,
            lmc);
    psp_lmcFree(cu);
    if (stat != S_OKAY)
    {
        vtprintf(DB_TEXT("Unable to clear user.\n"));
        return stat;
    }

    vtprintf(DB_TEXT("User '%s' has been cleared.\n"), name);

    return stat;
}

/*************************************************************************/

static int get_args(
    int             argc,
    DB_TCHAR       *argv[],
    DB_BOOLEAN     *pTerminate,
    DB_BOOLEAN     *pDisplay,
    DB_TCHAR       *pUser,
    DB_TCHAR       *pLockmgrn,
    LMC_AVAIL_FCN **pAvail)
{
    int i;

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != DB_TEXT('-'))
        {
            usage();
            return C_FAIL;
        }

        switch (vtotupper(argv[i][1]))
        {
            case DB_TEXT('H'):
            case DB_TEXT('?'):
                goto fail;

            case DB_TEXT('U'):
                if ((i + 1 < argc) && (argv[i + 1][0] != DB_TEXT('-')))
                    vtstrcpy(pUser, argv[++i]);
                else
                    goto fail;
                break;
  
            case DB_TEXT('L'):
                if (*pDisplay)
                    goto fail;
                *pTerminate = TRUE;
                break;
  
            case DB_TEXT('A'):
                if ((i + 1 < argc) && (argv[i + 1][0] != DB_TEXT('-')))
                {
                    vtstrncpy(pLockmgrn, argv[++i], LOCKMGRLEN);
                    pLockmgrn[LOCKMGRLEN - 1] = DB_TEXT('\0');
                }
                else
                    goto fail;
                break;
  
            case DB_TEXT('S'):
                if (*pTerminate)
                    goto fail;
                *pDisplay = TRUE;
                break;
  
            case DB_TEXT('M'):
                switch(vtotupper(argv[i][2]) )
                {
                    case DB_TEXT('T'):
                        *pAvail = psp_lmcFind(DB_TEXT("TCP"));
                        break;
  
                    case DB_TEXT('P'):
                        *pAvail = psp_lmcFind(DB_TEXT("IP"));
                        break;

                    default:
                        goto fail;
                }
  
                if (!*pAvail) 
                {
                    vtprintf(DB_TEXT("Invalid lock manager transport type\n"));
                    goto fail;
                }
  
                break;
  
            default:
                goto fail;
        }
    }

    if (!(*pTerminate) && !(*pDisplay))
    {
        if (!pUser[0])
        {
            vtprintf(DB_TEXT("No dbuserid set.\n"));
            vtprintf(DB_TEXT("LMCLEAR does not use the DBUSERID environment variable.\n\n"));
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
    vtprintf(DB_TEXT("\nusage:  lmclear {-l | -s | -u username} [-a lockmgrname] [-m{t|p}]\n\n"));
    vtprintf(DB_TEXT("       where -l  kills the lock manager\n"));
    vtprintf(DB_TEXT("             -s  prints a status report for all users\n"));
    vtprintf(DB_TEXT("             -u  specifies the user name to be cleared\n"));
    vtprintf(DB_TEXT("          (one of the previous three options must be set)\n\n"));
    vtprintf(DB_TEXT("             -a  specifies the lock manager name\n"));
    vtprintf(DB_TEXT("             -mt use the TCP lock manager (default)\n"));
    vtprintf(DB_TEXT("             -mp use the IP lock manager (if available)\n"));
}


