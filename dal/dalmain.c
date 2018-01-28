/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dal utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/*-----------------------------------------------------------------------

    dalmain.c - DAL main program.

    This function will process command line options and commence with
    the processing of input.  Once yyparse() is called, control will
    remain in the parser until the exit or abort commands are given.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "daldef.h"
#include "version.h"

/* ********************** GLOBAL VARIABLE DECLARATIONS *************** */
struct fcnlist fcns[] = {
    { DB_TEXT("op"),        (D_API_FCN) d_open_sg,     L_L_SG_TSK},
    { DB_TEXT("setp"),      (D_API_FCN) d_setpages,    I_I_TSK},
    { DB_TEXT("clo"),       (D_API_FCN) d_close,       N_TSK},
    { DB_TEXT("in"),        (D_API_FCN) d_initialize,  N_TSK_DBN},
    { DB_TEXT("des"),       (D_API_FCN) d_destroy,     L_TSK},
    { DB_TEXT("disd"),      (D_API_FCN) d_disdel,      N_TSK_DBN},
    { DB_TEXT("dbd"),       (D_API_FCN) d_dbdpath,     L_TSK},
    { DB_TEXT("dbf"),       (D_API_FCN) d_dbfpath,     L_TSK},
    { DB_TEXT("ctb"),       (D_API_FCN) d_ctbpath,     L_TSK},
    { DB_TEXT("fr"),        (D_API_FCN) d_freeall,     N_TSK},
    { DB_TEXT("keyfre"),    (D_API_FCN) d_keyfree,     F_TSK_DBN},
    { DB_TEXT("keylo"),     (D_API_FCN) d_keylock,     F_L_TSK_DBN},
    { DB_TEXT("keyls"),     (D_API_FCN) d_keylstat,    U},
    { DB_TEXT("lock"),      (D_API_FCN) d_lock,        I_LP_TSK_DBN},
    { DB_TEXT("recfre"),    (D_API_FCN) d_recfree,     R_TSK_DBN},
    { DB_TEXT("reclo"),     (D_API_FCN) d_reclock,     R_L_TSK_DBN},
    { DB_TEXT("recls"),     (D_API_FCN) d_reclstat,    U},
    { DB_TEXT("setls"),     (D_API_FCN) d_setlstat,    U},
    { DB_TEXT("rlbc"),      (D_API_FCN) d_rlbclr,      N_TSK_DBN},
    { DB_TEXT("rlbs"),      (D_API_FCN) d_rlbset,      N_TSK_DBN},
    { DB_TEXT("rlbt"),      (D_API_FCN) d_rlbtst,      N_TSK_DBN},
    { DB_TEXT("setfr"),     (D_API_FCN) d_setfree,     S_TSK_DBN},
    { DB_TEXT("setlo"),     (D_API_FCN) d_setlock,     S_L_TSK_DBN},
    { DB_TEXT("ti"),        (D_API_FCN) d_timeout,     I_TSK},
    { DB_TEXT("dbl"),       (D_API_FCN) d_dblog,       L_TSK},
    { DB_TEXT("dbt"),       (D_API_FCN) d_dbtaf,       L_TSK},
    { DB_TEXT("dbu"),       (D_API_FCN) d_dbuserid,    L_TSK},
    { DB_TEXT("tra"),       (D_API_FCN) d_trabort,     N_TSK},
    { DB_TEXT("trb"),       (D_API_FCN) d_trbegin,     L_TSK},
    { DB_TEXT("tre"),       (D_API_FCN) d_trend,       N_TSK},
    { DB_TEXT("crr"),       (D_API_FCN) d_crread,      F_FP_TSK_DBN},
    { DB_TEXT("crw"),       (D_API_FCN) d_crwrite,     F_FP_TSK_DBN},
    { DB_TEXT("csmr"),      (D_API_FCN) d_csmread,     S_F_FP_TSK_DBN},
    { DB_TEXT("csmw"),      (D_API_FCN) d_csmwrite,    S_F_FP_TSK_DBN},
    { DB_TEXT("csor"),      (D_API_FCN) d_csoread,     S_F_FP_TSK_DBN},
    { DB_TEXT("csow"),      (D_API_FCN) d_csowrite,    S_F_FP_TSK_DBN},
    { DB_TEXT("recr"),      (D_API_FCN) d_recread,     RP_TSK_DBN},
    { DB_TEXT("recw"),      (D_API_FCN) d_recwrite,    RP_TSK_DBN},
    { DB_TEXT("fil"),       (D_API_FCN) d_fillnew,     R_RP_TSK_DBN},
    { DB_TEXT("ma"),        (D_API_FCN) d_makenew,     R_TSK_DBN},
    { DB_TEXT("setk"),      (D_API_FCN) d_setkey,      F_FP_TSK_DBN},
    { DB_TEXT("del"),       (D_API_FCN) d_delete,      N_TSK_DBN},
    { DB_TEXT("con"),       (D_API_FCN) d_connect,     S_TSK_DBN},
    { DB_TEXT("di"),        (D_API_FCN) d_discon,      S_TSK_DBN},
    { DB_TEXT("me"),        (D_API_FCN) d_members,     S_IP_TSK_DBN},
    { DB_TEXT("iso"),       (D_API_FCN) d_isowner,     S_TSK_DBN},
    { DB_TEXT("ism"),       (D_API_FCN) d_ismember,    S_TSK_DBN},
    { DB_TEXT("findc"),     (D_API_FCN) d_findco,      S_TSK_DBN},
    { DB_TEXT("findf"),     (D_API_FCN) d_findfm,      S_TSK_DBN},
    { DB_TEXT("findl"),     (D_API_FCN) d_findlm,      S_TSK_DBN},
    { DB_TEXT("findn"),     (D_API_FCN) d_findnm,      S_TSK_DBN},
    { DB_TEXT("findp"),     (D_API_FCN) d_findpm,      S_TSK_DBN},
    { DB_TEXT("keyfi"),     (D_API_FCN) d_keyfind,     F_FP_TSK_DBN},
    { DB_TEXT("keyn"),      (D_API_FCN) d_keynext,     F_TSK_DBN},
    { DB_TEXT("keyp"),      (D_API_FCN) d_keyprev,     F_TSK_DBN},
    { DB_TEXT("keyfrs"),    (D_API_FCN) d_keyfrst,     F_TSK_DBN},
    { DB_TEXT("keyla"),     (D_API_FCN) d_keylast,     F_TSK_DBN},
    { DB_TEXT("keyr"),      (D_API_FCN) d_keyread,     FP_TSK},
    { DB_TEXT("setro"),     (D_API_FCN) d_setro,       S_TSK_DBN},
    { DB_TEXT("setrm"),     (D_API_FCN) d_setrm,       S_TSK_DBN},
    { DB_TEXT("setor"),     (D_API_FCN) d_setor,       S_TSK_DBN},
    { DB_TEXT("setom"),     (D_API_FCN) d_setom,       S_S_TSK_DBN},
    { DB_TEXT("setoo"),     (D_API_FCN) d_setoo,       S_S_TSK_DBN},
    { DB_TEXT("setmr"),     (D_API_FCN) d_setmr,       S_TSK_DBN},
    { DB_TEXT("setmo"),     (D_API_FCN) d_setmo,       S_S_TSK_DBN},
    { DB_TEXT("setmm"),     (D_API_FCN) d_setmm,       S_S_TSK_DBN},
    { DB_TEXT("crg"),       (D_API_FCN) d_crget,       DP_TSK_DBN},
    { DB_TEXT("crse"),      (D_API_FCN) d_crset,       DP_TSK_DBN},
    { DB_TEXT("csmg"),      (D_API_FCN) d_csmget,      S_DP_TSK_DBN},
    { DB_TEXT("csms"),      (D_API_FCN) d_csmset,      S_DP_TSK_DBN},
    { DB_TEXT("csog"),      (D_API_FCN) d_csoget,      S_DP_TSK_DBN},
    { DB_TEXT("csos"),      (D_API_FCN) d_csoset,      S_DP_TSK_DBN},
    { DB_TEXT("crt"),       (D_API_FCN) d_crtype,      IP_TSK_DBN},
    { DB_TEXT("cmt"),       (D_API_FCN) d_cmtype,      S_IP_TSK_DBN},
    { DB_TEXT("cot"),       (D_API_FCN) d_cotype,      S_IP_TSK_DBN},
    { DB_TEXT("cms"),       (D_API_FCN) d_cmstat,      U},
    { DB_TEXT("cos"),       (D_API_FCN) d_costat,      U},
    { DB_TEXT("crst"),      (D_API_FCN) d_crstat,      U},
    { DB_TEXT("css"),       (D_API_FCN) d_csstat,      U},
    { DB_TEXT("ctscm"),     (D_API_FCN) d_ctscm,       U},
    { DB_TEXT("ctsco"),     (D_API_FCN) d_ctsco,       U},
    { DB_TEXT("ctscr"),     (D_API_FCN) d_ctscr,       U},
    { DB_TEXT("gtscm"),     (D_API_FCN) d_gtscm,       U},
    { DB_TEXT("gtsco"),     (D_API_FCN) d_gtsco,       U},
    { DB_TEXT("gtscr"),     (D_API_FCN) d_gtscr,       U},
    { DB_TEXT("gtscs"),     (D_API_FCN) d_gtscs,       U},
    { DB_TEXT("recst"),     (D_API_FCN) d_recstat,     U},
    { DB_TEXT("stscm"),     (D_API_FCN) d_stscm,       U},
    { DB_TEXT("stsco"),     (D_API_FCN) d_stsco,       U},
    { DB_TEXT("stscr"),     (D_API_FCN) d_stscr,       U},
    { DB_TEXT("stscs"),     (D_API_FCN) d_stscs,       U},
    { DB_TEXT("utscm"),     (D_API_FCN) d_utscm,       U},
    { DB_TEXT("utsco"),     (D_API_FCN) d_utsco,       U},
    { DB_TEXT("utscr"),     (D_API_FCN) d_utscr,       U},
    { DB_TEXT("utscs"),     (D_API_FCN) d_utscs,       U},
    { DB_TEXT("initf"),     (D_API_FCN) d_initfile,    I_TSK_DBN},
    { DB_TEXT("keyd"),      (D_API_FCN) d_keydel,      F_TSK_DBN},
    { DB_TEXT("keye"),      (D_API_FCN) d_keyexist,    F_TSK_DBN},
    { DB_TEXT("keys"),      (D_API_FCN) d_keystore,    F_TSK_DBN},
    { DB_TEXT("of"),        (D_API_FCN) d_off_opt,     I_TSK},
    { DB_TEXT("on"),        (D_API_FCN) d_on_opt,      I_TSK},
    { DB_TEXT("rd"),        (D_API_FCN) d_rdcurr,      CP_IP_TSK_DBN},
    { DB_TEXT("recfrs"),    (D_API_FCN) d_recfrst,     R_TSK_DBN},
    { DB_TEXT("recla"),     (D_API_FCN) d_reclast,     R_TSK_DBN},
    { DB_TEXT("recn"),      (D_API_FCN) d_recnext,     N_TSK_DBN},
    { DB_TEXT("recp"),      (D_API_FCN) d_recprev,     N_TSK_DBN},
    { DB_TEXT("recse"),     (D_API_FCN) d_recset,      R_TSK_DBN},
    { DB_TEXT("ren"),       (D_API_FCN) d_renfile,     REN_TSK},
    { DB_TEXT("setd"),      (D_API_FCN) d_setdb,       U},
    { DB_TEXT("setfi"),     (D_API_FCN) d_setfiles,    I_TSK},
    { DB_TEXT("wr"),        (D_API_FCN) d_wrcurr,      C_TSK_DBN},
    { DB_TEXT("def_rec"),   (D_API_FCN) def_rec,        R_RP_TSK_DBN},
    { DB_TEXT("def_fld"),   (D_API_FCN) def_fld,        F_FP_TSK_DBN},
    { DB_TEXT("get_clock"), (D_API_FCN) get_clock,      CT},
    { DB_TEXT("cmp_clock"), (D_API_FCN) cmp_clock,      CT_CT_CT}
};
int nfcns = sizeof(fcns) / sizeof(struct fcnlist);

INST                *loopstack[20],
                    *curinst,
                    *previnst;
int                  loop_lvl,
                     nparam,
                     batch;

struct printfield   *curprint;

FILE                *fdal;
extern int           tot_errs;
int                  dal_unicode = 0;
DB_TASK             *DalDbTask;
SG                  *dal_sg;

int usage(void);

/* ------------------------------------------------------------------------ */

static void init_dalmain()
{
    /* for VxWorks - initialize globals explicitly */
    memset(loopstack, 0, sizeof(loopstack));
    curinst = previnst = NULL;
    loop_lvl = nparam = batch = 0;
    curprint = NULL;
    fdal = NULL;
    dal_unicode = 0;
    dal_sg = NULL;
    DalDbTask = NULL;
}


void EXTERNAL_FCN dal_dberr(int errnum, DB_TCHAR *errmsg)
{
    vtprintf(DB_TEXT("\n%s (errnum = %d)\n"), errmsg, errnum);
}


/* db.* Access Language **************************************
*/
int MAIN(int argc, DB_TCHAR **argv)
{
    int       stat, i;
    DB_TCHAR *str = NULL;
    DB_TCHAR *bfile = NULL;
#if defined(SAFEGARDE)
    DB_TCHAR *cp;
    DB_TCHAR *password;
    int       mode = NO_ENC;
#endif

    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Database Access Language")));

    /* initialize all global / static variables */
    init_dalmain();
    init_dallex();
    init_dalvar();

    for (i = 1; i < argc && argv[i][0] == DB_TEXT('-'); i++)
    {
        switch (vtotlower(argv[i][1]))
        {
            case DB_TEXT('?'):
            case DB_TEXT('h'):
                return usage();

            case DB_TEXT('m'):
                switch (vtotlower(argv[i][2]))
                {
                    case DB_TEXT('n'):
                        str = DB_TEXT("None");
                        break;

                    case DB_TEXT('t'):
                        str = DB_TEXT("TCP");
                        break;

                    case DB_TEXT('p'):
                        str = DB_TEXT("IP");
                        break;

                    default:
                        vftprintf(stderr, DB_TEXT("Invalid option: %s\n"),
                                argv[i]);
                        return usage();
                }

                break;

#if defined(UNICODE)
            case DB_TEXT('u'):
                dal_unicode = 1;
                break;
#endif

            case DB_TEXT('s'):
                if (argv[i][2] != DB_TEXT('g'))
                {
                    vftprintf(stderr, DB_TEXT("Invalid option: %s\n"), argv[i]);
                    return usage();
                }

                if (i == argc - 1)
                {
                    vftprintf(stderr, DB_TEXT("No password specified\n"));
                    return usage();
                }

#if defined(SAFEGARDE)
                if ((cp = vtstrchr(argv[++i], DB_TEXT(','))) != NULL)
                {
                    *cp++ = DB_TEXT('\0');
                    if (vtstricmp(argv[i], DB_TEXT("low")) == 0)
                        mode = LOW_ENC;
                    else if (vtstricmp(argv[i], DB_TEXT("med")) == 0)
                        mode = MED_ENC;
                    else if (vtstricmp(argv[i], DB_TEXT("high")) == 0)
                        mode = HIGH_ENC;
                    else
                    {
                        vftprintf(stderr, DB_TEXT("Invalid SafeGarde encryption mode\n"));
                        return usage();
                    }

                    password = cp;
                }
                else
                {
                    mode = MED_ENC;
                    password = argv[i];
                }

                break;
#else
                vftprintf(stderr, DB_TEXT("SafeGarde is not available in this version\n"));
                return 1;
#endif

            default:
                vftprintf(stderr, DB_TEXT("Invalid option: %s\n"), argv[i]);
                return usage();
        }
    }

    if (i < argc)
        bfile = argv[i++];

    while (i < argc)
        vftprintf(stderr, DB_TEXT("Ignoring option: %s\n"), argv[i++]);

    if ((stat = d_opentask(&DalDbTask)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to open task\n"));
        return 1;
    }

#if defined(SAFEGARDE)
    if (mode != NO_ENC && (dal_sg = sg_create(mode, password)) == NULL)
    {
        vftprintf(stderr, DB_TEXT("Failed to create SafeGarde contnext\n"));
        goto exit;
    }
#endif

    if ((stat = d_set_dberr(dal_dberr, DalDbTask)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to set error handler\n"));
        goto exit;
    }

    if (str && (stat = d_lockcomm(psp_lmcFind(str), DalDbTask)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Failed to set lock manager type\n"));
        goto exit;
    }

    if (bfile)
    {
        fdal = vtfopen(bfile, dal_unicode ? DB_TEXT("rb") : DB_TEXT("r"));
        if (fdal == NULL)
        {
            vftprintf(stderr,
                    DB_TEXT("unable to open file '%s'.  Errno = %d\n"),
                    bfile, errno);
                goto exit;
        }

        batch = 1;
    }
    else
    {
	fdal = stdin;
	vtprintf(DB_TEXT("d_"));
    }

    if ((stat = yyparse()) != 0)
	vftprintf(stderr, DB_TEXT("%d errors detected\n"), tot_errs);

    fclose(fdal);

exit:
#if defined(SAFEGARDE)
    if (dal_sg)
        sg_delete(dal_sg);
#endif

    if (DalDbTask)
        d_closetask(DalDbTask);

    return stat;
}

/* ------------------------------------------------------------------------ */

#if defined(UNICODE)
#define UNICODE_OPT L"[-u] "
#else
#define UNICODE_OPT ""
#endif

int usage()
{
    vftprintf(stderr, DB_TEXT("usage: dal [-m{n|t|p}] [-sg [<mode>,]<password>] [") UNICODE_OPT
            DB_TEXT("file]\n"));
    vftprintf(stderr, DB_TEXT("where: -m   Specifies the lock manager type.  Valid values are\n"));
    vftprintf(stderr, DB_TEXT("             n - None (default, single user only)\n"));
    vftprintf(stderr, DB_TEXT("             t - TCP/IP\n"));
    vftprintf(stderr, DB_TEXT("             p - UNIX Domain sockets\n"));
    vftprintf(stderr, DB_TEXT("       -sg  Specifies the SafeGarde encryption information for the database\n"));
    vftprintf(stderr, DB_TEXT("             <mode> can be 'low', 'med' (default), and 'high'\n"));
#if defined(UNICODE)
    vftprintf(stderr, DB_TEXT("       -u   Specifies the batch file to be a Unicode file\n"));
#endif
    vftprintf(stderr, DB_TEXT("       file Specifies a file containing batch commands to execute\n"));

    return 1;
}


VXSTARTUP("dal", 0)

