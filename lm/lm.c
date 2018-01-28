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

#define MOD lm
#include "lm.h"
#include "version.h"

PSP_LMC        lmc;
LM             lm;
int            daemon_mode = 1;
int            terminate = 0;
short          maxusers = MAX_USERS;
short          maxfiles = MAX_FILES;
short          maxqueues = MAX_QUEUES;
DB_TCHAR      *lockmgrn = NULL;
DB_TCHAR      *tmpdir = NULL;
DB_TCHAR      *lm_type = "TCP";
LMC_AVAIL_FCN *avail = NULL;
#if defined(DB_DEBUG)
DB_TCHAR      *lm_log = NULL;
FILE          *logfh;
#endif

int  reply(PSP_CONN, void *, size_t, int);
void discon(PSP_CONN);
void info(PSP_CONN, DB_TCHAR *, DB_TCHAR *, DB_TCHAR *);
void get_opts(int, DB_TCHAR **);
void usage(DB_TCHAR *);

void ctrlc(
    void)
{
    static int calls = 0;

    if (++calls == 4)
        exit(-1);

    terminate = 1;
}

int MAIN(
    int        argc,
    DB_TCHAR **argv)
{
    int      stat;
    int      mtype;
    size_t   size;
    void    *msg;
    int      user;
    PSP_CONN conn;

    vtprintf(DBSTAR_LM_DESC);

    if ((stat = psp_init()) != PSP_OKAY) {
        printf("Error %d in psp_init()\n", stat);
        return -1;
    }

    get_opts(argc, argv);
    if (!lockmgrn)
        lockmgrn = psp_defLockmgr();

    if (!tmpdir)
        tmpdir = psp_pathDefTmp();

#if defined(DB_DEBUG)
    if (lm_log)
    {
        if ((logfh = fopen(lm_log, "w")) == NULL)
        {
            vftprintf(stderr, DB_TEXT("Unable to open file %s, logging is set to stderr\n"), lm_log);
            logfh = stderr;
        }
    }
    else
        logfh = stderr;
#endif

    psp_handleInterrupt(ctrlc);

    stat = lm_init(&lm, maxusers, maxfiles, maxqueues, &reply, &discon, &info);
    if (stat != LM_OKAY) {
        psp_term();
        return -1;
    }

    if ((stat = psp_lmcSetup(&lmc, avail)) != PSP_OKAY) {
        printf("Error %d in psp_lmcSetup\n", stat);
        lm_term(lm);
        psp_term();
        return -1;
    }

    vtprintf(DB_TEXT("%s Lock manager\n"), psp_lmcFullName(lmc));

    stat = psp_lmcListen(lockmgrn, tmpdir, maxusers, lmc);
    if (stat != PSP_OKAY) {
        if (stat == PSP_DUPLICATE)
            printf("Another lock manager is already running with the specified name\n");
        else
            printf("Error %d in psp_lmcListen\n", stat);
        psp_lmcCleanup(lmc);
        lm_term(lm);
        psp_term();
        return -1;
    }

    if (daemon_mode)
        psp_daemonInit(lockmgrn);
    else
        vtprintf(DB_TEXT("Hit <CTRL>-C to shut down\n"));

    vtprintf(DB_TEXT("\n"));

    while (!terminate) {
        stat = psp_lmcWaitMsg(&conn, &mtype, &msg, &size, 100, lmc);
        if (stat == PSP_FAILED) {
            printf("Error %d in psp_lmcWaitMsg, shutting down\n", stat);
            break;
        }

        if (stat == PSP_TIMEOUT)
            stat = lm_crank(lm);
        else if (stat == PSP_OKAY && mtype == L_PING)
        {
            stat = psp_lmcReply(conn, msg, size, 0, lmc);
            psp_lmcFree(msg);
        }
        else {
            user = lm_find_user(lm, conn);
            if (stat == PSP_DISCONNECTED)
                stat = lm_dead_user(lm, user);
            else
                stat = lm_process(lm, user, (int) conn, mtype, msg);
        }

        if (stat == LM_TERMINATE)
            terminate = 1;
        else if (stat != LM_OKAY)
            vtprintf(DB_TEXT("Lock Manager error %d\n"), stat);
    }

    vtprintf(DB_TEXT("\nShutting down\n"));

#if defined(DB_DEBUG)
    if (logfh != stderr)
        fclose(logfh);
#endif

    lm_term(lm);
    psp_lmcStopListen(lmc);
    psp_lmcCleanup(lmc);
    psp_term();
    if (daemon_mode)
        psp_daemonTerm();

    return 0;
}

void get_opts(
    int        argc,
    DB_TCHAR **argv)
{
    int       ii;
    DB_TCHAR *appname;

    if ((appname = vtstrrchr(argv[0], DIRCHAR)) != NULL)
        appname++;
    else
        appname = argv[0];

    for (ii = 1; ii < argc; ii++) {
        if (argv[ii][0] != DB_TEXT('-'))
            goto error;

        switch (vtotlower(argv[ii][1])) {
            case DB_TEXT('?'):
            case DB_TEXT('h'):
                usage(appname);
                break;

            case DB_TEXT('u'):
                if (ii + 1 == argc)
                    goto error;

                maxusers = vttoi(argv[++ii]);
                break;

            case DB_TEXT('f'):
                if (ii + 1 == argc)
                    goto error;

                maxfiles = vttoi(argv[++ii]);
                break;

            case DB_TEXT('q'):
                if (ii + 1 == argc)
                    goto error;

                maxqueues = vttoi(argv[++ii]);
                break;

#if defined(DB_DEBUG)
            case DB_TEXT('l'):
                if (ii + 1 == argc)
                    goto error;

                lm_log = argv[++ii];
                break;
#endif

            case DB_TEXT('z'):
                if (ii + 1 == argc)
                    goto error;

                tmpdir = argv[++ii];
                break;

            case DB_TEXT('a'):
                if (ii + 1 == argc)
                    goto error;

                if (vtstrlen(argv[++ii]) > LOCKMGRLEN - 1) {
                    vftprintf(stderr, "\nLock manager name too long\n");
                    goto error;
                }

                lockmgrn = argv[ii];
                break;

            case DB_TEXT('m'):
                switch (argv[ii][2]) {
                    case DB_TEXT('t'): lm_type = DB_TEXT("TCP"); break;
                    case DB_TEXT('p'): lm_type = DB_TEXT("IP");  break;
                    default:           usage(appname);           break;
                }

                break;

            case DB_TEXT('n'):
                daemon_mode = 0;
                break;

            default:
                goto error;
        }
    }

    if ((avail = psp_lmcFind(lm_type)) == NULL) {
        vftprintf(stderr, DB_TEXT("Invalid lock manager type\n"));
        usage(appname);
    }

    return;

error:
    vftprintf(stderr, DB_TEXT("\nInvalid or incomplete command line option: %s\n"), argv[ii]);
    usage(appname);
}

#if defined(DB_DEBUG)
#define LOG_OPT DB_TEXT(" [-l log]")
#else
#define LOG_OPT
#endif

void usage(
    DB_TCHAR *name)
{
    vftprintf(stderr, DB_TEXT("\nusage: %s [-h] [-n] [-a name] [-u #] [-f #] [-q #] [-m{t|p}] [-z tmp]") LOG_OPT DB_TEXT("\n"), name);
    vftprintf(stderr, DB_TEXT("   -h = Display this usage information\n"));
    vftprintf(stderr, DB_TEXT("   -n = Do not daemonize the lock manager\n"));
    vftprintf(stderr, DB_TEXT("   -a = Set lock manager name to 'name' (default is '%s')\n"), psp_defLockmgr());
    vftprintf(stderr, DB_TEXT("   -u = Set maximum users to # (default is %d)\n"), maxusers);
    vftprintf(stderr, DB_TEXT("   -f = Set maximum files to # (default is %d)\n"), maxfiles);
    vftprintf(stderr, DB_TEXT("   -q = Set maximum queues to # (default is %d)\n"), maxqueues);
    vftprintf(stderr, DB_TEXT("   -m = Set the lock manager type\n"));
    vftprintf(stderr, DB_TEXT("         t = TCP/IP (default)\n"));
    vftprintf(stderr, DB_TEXT("         p = Unix Domain sockets (UNIX only)\n"));
    vftprintf(stderr, DB_TEXT("   -z = Set temporary directory to 'tmp' (default is '%s')\n"), psp_pathDefTmp());
#if defined(DB_DEBUG)
    vftprintf(stderr, DB_TEXT("   -l = Enable verbose logging to 'log' (default is stderr)\n\n"));
#endif

    exit(1);
}

int reply(
    PSP_CONN conn,
    void    *msg,
    size_t   size,
    int      status)
{
    int stat;

    stat = psp_lmcReply(conn, msg, size, status, lmc);
    if (stat != PSP_OKAY) {
        if (stat == PSP_DISCONNECTED)
            return LM_DISCONNECTED;

        printf("Error %d in psp_lmcReply\n", stat);
        return LM_FAILED;
    }

    return LM_OKAY;
}

void discon(
    PSP_CONN conn)
{
    psp_lmcDisconClient(conn, lmc);
}

void info(
    PSP_CONN conn,
    DB_TCHAR *line1,
    DB_TCHAR *line2,
    DB_TCHAR *line3)
{
    psp_lmcConnectInfo(conn, line1, line2, line3, lmc);
}

