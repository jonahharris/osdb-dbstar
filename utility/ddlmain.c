/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ddlp utility                                      *
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

    ddlmain.c - ddlp main program, driver.

    This function will process options, open the ddlp_g.schema file and initiate
    the parsing of the ddlp_g.schema.

-----------------------------------------------------------------------*/

#define MOD ddlp
#include "db.star.h"
#include "version.h"
#include "parser.h"
#include "ddldefs.h"

#if !defined(NO_PREPRO)
static char prepro[63];
static char grepv[] = "grep -v '#'";
static char command[900];
static char include_path[400];
#endif /* NO_PREPRO */


DDLP_G ddlp_g; /* all global variables, saving namespace */

#if defined(UNICODE)
#define UPARM L##"[-u] "
#else
#define UPARM ""
#endif

#if !defined(NO_PREPRO)
#define IPARM DB_TEXT("[-i] ")
#else
#define IPARM DB_TEXT("")
#endif
static int usage(DB_TCHAR *errMsg)
{
    register DB_TCHAR **psz;
    static DB_TCHAR *cmdHelp[] = {
        DB_TEXT("usage: ddlp [-d]") IPARM DB_TEXT("[-n] [-r]") UPARM DB_TEXT("[-x] [-z] [-sg [<mode>,]<passwd>] [-s[-]] ddlfile"),
        DB_TEXT("where:"),
        DB_TEXT("  -d     Allow duplicate field and struct names."),
#if !defined(NO_PREPRO)
        DB_TEXT("  -i     Instruct cc to search for this path for include file."),
#endif /* NO_PREPRO */
        DB_TEXT("  -n     Instruct ddlp to omit writing the ASCII names."),
        DB_TEXT("  -r     Generate the File Structure Report."),
#if defined(UNICODE)
        DB_TEXT("  -u     Specify Unicode input (ddl) and output (include & dbd) files."),
#endif
        DB_TEXT("  -x     Generate a cross-reference listing."),
        DB_TEXT("  -z     Turn off printing of SIZEOF_??? constants."),
        DB_TEXT("  -sg    Specify encryption information for the database. (SafeGarde only)"),
        DB_TEXT("            <mode> can \"low\", \"med\" (default), or \"high\""),
        DB_TEXT("* -s     Case sensitivity.  Specify -s- to turn case sensitivity off."),
        DB_TEXT("* indicates default options"),
        DB_TEXT("")
    };

    if (errMsg)
        vftprintf(stderr, DB_TEXT("%s\n"), errMsg);

    for (psz = cmdHelp; **psz; ++psz)
        vftprintf(stderr, DB_TEXT("%s\n"), *psz);

    return(1);
}

/* db.* Database Definition Language Processor */

int MAIN(int argc, DB_TCHAR *argv[])
{
    int       i;
    long      memused;
    long      memtotal;
#if defined(SAFEGARDE)
    DB_TCHAR *cp;
    DB_TCHAR *password = NULL;
    int       mode = NO_ENC;
#endif
#if !defined(NO_PREPRO)
    char     *ptr;
#endif /* NO_PREPRO */

    /* initialize global variables explicitly (for VxWorks) */
    memset(&ddlp_g, 0, sizeof(DDLP_G));
    ddlp_g.outfile = stdout;

    setvbuf(stdout, NULL, _IONBF, 0 );
    setvbuf(stderr, NULL, _IONBF, 0 );

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Database Definition Language Processor")));
    ddlp_g.ddlfile[0] = 0;

    /* process option switches */
    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] == DB_TEXT('-'))
        {
            switch (vtotlower(argv[i][1]))
            {
                case DB_TEXT('d'):
                    ddlp_g.d_flag = 1;
                    break;

                case DB_TEXT('n'):
                    ddlp_g.n_flag = 1;
                    break;

                case DB_TEXT('r'):
                    ddlp_g.r_flag = 1;
                    break;

                case DB_TEXT('u'):
                    ddlp_g.u_flag = 1;
                    break;

                case DB_TEXT('x'):
                    ddlp_g.x_flag = 1;
                    break;

                case DB_TEXT('z'):
                    ddlp_g.z_flag = 1;
                    break;

                case DB_TEXT('s'):
                    switch (vtotlower(argv[i][2]))
                    {
                        case DB_TEXT('g'):
#if defined(SAFEGARDE)
                            if (i == argc - 1)
                                return usage("No password specified");

                            cp = vtstrchr(argv[++i], DB_TEXT(','));
                            if (cp != NULL)
                                *cp++ = '\0';

                            if (cp) {
                                if (vtstricmp(argv[i], "low") == 0)
                                    mode = LOW_ENC;
                                else if (vtstricmp(argv[i], "med") == 0)
                                    mode = MED_ENC;
                                else if (vtstricmp(argv[i], "high") == 0)
                                    mode = HIGH_ENC;
                                else
                                    return usage("Invalid encryption mode");

                                password = cp;
                            }
                            else
                            {
                                mode = MED_ENC;
                                password = argv[i];
                            }
#else
                            return usage("SafeGarde not available in this version");
#endif
                            break;

                        case DB_TEXT('-'):
                            ddlp_g.s_flag = 1;
                            break;

                        case DB_TEXT('\0'):
                            ddlp_g.s_flag = 0;
                            break;

                        default:
                            return usage(DB_TEXT("Invalid argument"));
                    }

                    break;

#if !defined(NO_PREPRO)
                case 'i':
                    if (ddlp_g.i_flag)
                    {
                        strcat(include_path, " ");
                        strcat(include_path, &argv[i][2]);
                    }
                    else  /* First -I encountered */
                    {
                        ddlp_g.i_flag = 1;
                        strcpy(include_path, &argv[i][2]);
                    }

                    break;
#endif

                default:
                    return usage(NULL);
            }
        }
        else
        {
            if (vtstrlen(ddlp_g.ddlfile) == 0)
                vtstrcpy(ddlp_g.ddlfile, argv[i]);
            else
                return(usage(DB_TEXT("Error: only one ddl file allowed")));
        }
    }

    if (vtstrlen(ddlp_g.ddlfile) == 0)
        return(usage(DB_TEXT("Error: ddl file required")));

#if !defined(NO_PREPRO)
    if (!psp_fileValidate(ddlp_g.ddlfile))
#else
    if ((ddlp_g.schema = vtfopen(ddlp_g.ddlfile, ddlp_g.u_flag ? DB_TEXT("rb") : DB_TEXT("r"))) == NULL)
#endif
    {
        vftprintf(stderr, DB_TEXT("Error (errno=%d): unable to open file '%s'\n"),
            errno, ddlp_g.ddlfile);
        return(1);
    }

    psp_init();

#if !defined(NO_PREPRO)
    if ((ptr = psp_getenv("DBCCOM")) == NULL)
    {
#if defined(__GNUC__)
        strcpy(prepro, "gcc -E -x c-header");
#else  /* __GNUC__ */
        strcpy(prepro, "cc -E");
#endif /* __GNUC__ */
    }
    else
    {
        strcpy(prepro, ptr);
        strcat(prepro, " -E");
    }

    if (ddlp_g.i_flag)
        sprintf(command, "%s %s %s | %s", prepro, include_path, ddlp_g.ddlfile, grepv);
    else
        sprintf(command, "%s %s | %s", prepro, ddlp_g.ddlfile, grepv);

    if ((ddlp_g.schema = popen(command, "r")) == NULL)
    {
        fprintf(stderr, "popen Error %s(%d): command:%s\n",
                strerror(errno), errno, command);
        psp_term();
        return 1;
    }
#endif /* NO_PREPRO */

#if defined(SAFEGARDE)
    if (mode && (ddlp_g.sg = sg_create(mode, password)) == NULL)
        return usage(DB_TEXT("Failed to create SafeGarde context"));
#endif

    tableInit();
    ddlpInit();
    lexInit();
    init_align();
    init_lists();
    yyparse();
    fclose(ddlp_g.schema);
    if (!ddlp_g.abort_flag)
        finish_up();
    if (ddlp_g.tot_errs == 0)
    {
        write_tables();
        if (!ddlp_g.abort_flag)
        {
            if (ddlp_g.r_flag)
                print_tables();
            if (ddlp_g.x_flag)
                print_xref();

            /* compute memory reqts */
            vftprintf(stderr, DB_TEXT("Runtime dictionary memory requirements:\n\n"));
            memused = (long)ddlp_g.tot_files * (long)sizeof(FILE_ENTRY);

            vftprintf(stderr, DB_TEXT("   file table  : %6ld\n"), memused);
            memtotal = memused;
            memused = (long)ddlp_g.tot_records * (long)sizeof(RECORD_ENTRY);

            vftprintf(stderr, DB_TEXT("   record table: %6ld\n"), memused);
            memtotal += memused;
            memused = (long)ddlp_g.tot_fields * (long)sizeof(FIELD_ENTRY);

            vftprintf(stderr, DB_TEXT("   field table : %6ld\n"), memused);
            memtotal += memused;
            memused = (long)ddlp_g.tot_sets * (long)sizeof(SET_ENTRY);

            vftprintf(stderr, DB_TEXT("   set table   : %6ld\n"), memused);
            memtotal += memused;
            memused = (long)ddlp_g.tot_members * (long)sizeof(MEMBER_ENTRY);

            vftprintf(stderr, DB_TEXT("   member table: %6ld\n"), memused);
            memtotal += memused;
            memused = (long)ddlp_g.tot_sort_fields * (long)sizeof(SORT_ENTRY);

            vftprintf(stderr, DB_TEXT("   sort table  : %6ld\n"), memused);
            memtotal += memused;
            memused = (long)ddlp_g.tot_comkeyflds * (long)sizeof(KEY_ENTRY);

            vftprintf(stderr, DB_TEXT("   key table   : %6ld\n\n"), memused);
            memtotal += memused;
            vftprintf(stderr, DB_TEXT("   total       : %6ld\n\n"), memtotal);
        }
    }
    free_lists();
    free_xref_list();
    vftprintf(stderr, DB_TEXT("%d error%s detected\n"), ddlp_g.tot_errs,
        ddlp_g.tot_errs == 1 ? DB_TEXT("") : DB_TEXT("s"));

#if defined(SAFEGARDE)
    if (ddlp_g.sg)
        sg_delete(ddlp_g.sg);
#endif

    psp_term();
    return ddlp_g.tot_errs;
}


VXSTARTUP("ddlp", 0)

