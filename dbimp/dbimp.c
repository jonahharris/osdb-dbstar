/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbimp utility                                     *
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

    dbimp.c - Main program of dbimp, database import utility

    The import utility reads an import specification and processes
    ASCII text files to import them into a db.* database.

    This module is the main program, which parses the import spec
    (yyparse), and if there are no errors, executes the specification
    (dbexec).

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "impdef.h"
#include "impvar.h"
#include "version.h"


/* ======================================================================
    Global variable declaractions
*/
IMP_G imp_g;

static int pgs = 64;


/* ======================================================================
    Function prototypes
*/
static int usage(DB_TCHAR *);
static int get_opts(int, DB_TCHAR **, int *, DB_TCHAR **);
static int compile(void);
static int dbimpa(void);
void EXTERNAL_FCN dbimp_dberr(int, DB_TCHAR *);


/* ======================================================================
    db_VISTA Import Utility
*/

int MAIN(int argc, DB_TCHAR *argv[])
{
    int       ret = 1;
    int       mode;
    DB_TCHAR *passwd;

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Database Import")));

    /* initialize global variable explicitly (for VxWorks) */
    memset(&imp_g, 0, sizeof(IMP_G));
    imp_g.sep_char = DB_TEXT(',');
    imp_g.esc_char = DB_TEXT('\\');
    imp_g.keylen = 26;
    imp_g.keyfile = DB_TEXT("dbimp.kfl");
    imp_g.rf = DB_TEOF;

    /* process the command line */
    if ((ret = get_opts(argc, argv, &mode, &passwd)) == 0)
        return ret;

    /* Initialize db.* - database gets opened in dbimpa */
    if ((ret = d_opentask(&imp_g.dbtask)) != S_OKAY)
        return ret;

    imp_g.sg = NULL;
#if defined(SAFEGARDE)
    if (mode != NO_ENC && (imp_g.sg = sg_create(mode, passwd)) == NULL)
    {
        d_closetask(imp_g.dbtask);
        return -1;
    }
#endif

    if ((ret = d_set_dberr(dbimp_dberr, imp_g.dbtask)) == S_OKAY)
    {
        if ((ret = d_on_opt(0L, imp_g.dbtask)) == S_OKAY)
        {
            if ((ret = compile()) == 0)
                ret = dbimpa();

            if (ret == 0)
                vtprintf(DB_TEXT("Successful import\n"));
            else
                vtprintf(DB_TEXT("Import aborted!\n"));
        }
    }

    d_closetask(imp_g.dbtask);
    return ret;
}


#if defined(UNICODE)
#define UNICODE_OPT L"[-u] "
#else
#define UNICODE_OPT ""
#endif
/* ======================================================================
*/
static int usage(DB_TCHAR *opt)
{
    if (opt)
        vftprintf(stderr, DB_TEXT("Invalid option %s\n\n"), opt);

    vftprintf(stderr, DB_TEXT("usage: dbimp [-n] [-sg [<mode>,]<passwd>] [-pN] [-kN] [-e esc] [-s sep] ")
            UNICODE_OPT DB_TEXT("file\n"));
    vftprintf(stderr, DB_TEXT("where:\n"));
    vftprintf(stderr, DB_TEXT("  -n  eliminates output of data\n"));
    vftprintf(stderr, DB_TEXT("  -sg specifies the encryption mode and password.\n"));
    vftprintf(stderr, DB_TEXT("          mode can be 'low', 'med' (default), or 'high'\n"));
    vftprintf(stderr, DB_TEXT("  -kN adjusts 'create on' field length to size N (default  25)\n"));
    vftprintf(stderr, DB_TEXT("  -pN set the number of pages in cache to N\n"));
    vftprintf(stderr, DB_TEXT("  -e  causes 'esc' to be used rather than '\\'\n"));
    vftprintf(stderr, DB_TEXT("  -s  causes 'sep' to be used rather than ','\n"));
#if defined(UNICODE)
    vftprintf(stderr, DB_TEXT("  -u  process Unicode input files\n"));
#endif

    return 0;
}


/* ======================================================================
    Parse the command line arguments
    Returns 0 if success, non-zero if error.
*/

#define NEXT_ARG()   (--argc ? *++argv : NULL)

static int get_opts(int argc, DB_TCHAR **argv, int *mode, DB_TCHAR **passwd)
{
    int       ii;

    *mode = NO_ENC;

    for (ii = 1; ii < argc; ii++)
    {
        if (argv[ii][0] != DB_TEXT('-'))
            break;

        switch (vtotlower(argv[ii][1]))
        {
            case DB_TEXT('e'):
                if (ii == argc - 1)
                    return usage(argv[ii]);

                if (argv[++ii][1] != 0)
                    return usage(argv[--ii]);

                imp_g.esc_char = argv[ii][0];
                if (vistdigit(imp_g.esc_char))
                {
                    vftprintf(stderr, DB_TEXT("Escape character cannot be numeric"));
                    return usage(argv[--ii]);
                }

                break;

            case DB_TEXT('s'):
                if (ii == argc - 1)
                    return usage(argv[ii]);

                if (tolower(argv[ii][2]) == DB_TEXT('g'))
                {
                    DB_TCHAR *cp;

                    if ((cp = vtstrchr(argv[++ii], DB_TEXT(','))) != NULL)
                    {
                        *cp++ = '\0';
                        if (vtstricmp(argv[ii], DB_TEXT("low")) == 0)
                            *mode = LOW_ENC;
                        else if (vtstricmp(argv[ii], DB_TEXT("med")) == 0)
                            *mode = MED_ENC;
                        else if (vtstricmp(argv[ii], DB_TEXT("high")) == 0)
                            *mode = HIGH_ENC;
                        else
                            return usage(argv[--ii]);

                        *passwd = cp;
                    }
                    else
                    {
                        *mode = MED_ENC;
                        *passwd = argv[ii];
                    }
                }
                else if (argv[++ii][1] != 0)
                    return usage(argv[--ii]);
                else
                {
                    imp_g.sep_char = argv[ii][0];
                    if (isdigit(imp_g.sep_char))
                    {
                        vftprintf(stderr, DB_TEXT("Seperation character cannot be numeric"));
                        return usage(argv[-ii]);
                    }
                }

                break;

            case DB_TEXT('p'):
                if (ii == argc - 1)
                    return usage(argv[ii]);

                if ((pgs = (int) vttoi(argv[++ii])) == 0)
                    return usage(argv[--ii]);

                break;

            case DB_TEXT('k'):
                if (ii == argc - 1)
                    return usage(argv[ii]);

                if ((imp_g.keylen = (int) vttoi(argv[++ii])) == 0)
                    return usage(argv[--ii]);

                if (imp_g.keylen % 2)
                    imp_g.keylen++;

                if (imp_g.keylen < 2 || imp_g.keylen > 228)
                {
                    vftprintf(stderr, DB_TEXT("Key length %d is out of range (2-228)\n"), imp_g.keylen);
                    return usage(NULL);
                }

                break;

            case DB_TEXT('n'):
                imp_g.silent = 1;
                break;

#if defined(UNICODE)
            case DB_TEXT('u'):
                imp_g.unicode = 1;
                break;
#endif

            default:
                return usage(argv[ii]);
        }
    }

    if (ii == argc)
    {
        vftprintf(stderr, DB_TEXT("Import specification required\n"));
        return usage(NULL);
    }

    vtstrcpy(imp_g.specname, argv[ii++]);

    if (ii < argc)
        return usage(argv[ii]);

    return 1;
}


/* ======================================================================
*/
static int compile()
{
    if ( (imp_g.fspec = vtfopen(imp_g.specname,
                   imp_g.unicode ? DB_TEXT("rb") : DB_TEXT("r"))) == NULL )
    {
        vftprintf(stderr, DB_TEXT("unable to open spec file %s\n"), imp_g.specname);
        return 1;
    }

    /* compile the import specification */
    if ( yyparse() )
    {
        fclose(imp_g.fspec);
        return 1;
    }

    fclose(imp_g.fspec);

    vtprintf(DB_TEXT("Compilation complete:\n"));
    if (imp_g.tot_errs)
        vtprintf(DB_TEXT("    Syntax errors: %d\n"), imp_g.tot_errs);
    if (imp_g.tot_warnings)
        vtprintf(DB_TEXT("    Warnings:      %d\n"), imp_g.tot_warnings);

    if (imp_g.tot_errs != 0 || imp_g.tot_warnings != 0)
        return 1;

    return 0;
}


/* ======================================================================
*/
static int dbimpa()
{
    int i, j, k;
    int totmems = 0, cur_rmt, start, end;
    int ret;
    DB_TASK *task = imp_g.dbtask;

    if ((ret = d_setpages(pgs, 4, task)) != S_OKAY)
        return ret;

    if ((ret = d_on_opt(READNAMES, task)) != S_OKAY)
        return ret;

    if ((ret = d_open_sg(imp_g.dbname, DB_TEXT("o"), imp_g.sg, task)) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Unable to open database\n"));
        return ret;
    }

    /* extend database tables to add an extra key */
    if ((ret = init_key()) == S_OKAY)
    {
        vtprintf(DB_TEXT("\nStarting data import\n"));

        if (task->size_st > 0)
        {
            /* count how many member types there are */
            for (totmems = 0, i = 0; i < task->size_st; i++)
                totmems += task->set_table[i].st_memtot;
        }

        imp_g.rbs = (int *) psp_cGetMemory(task->size_rt * sizeof(int), 0);
        imp_g.rbe = (int *) psp_cGetMemory(task->size_rt * sizeof(int), 0);
        imp_g.bs  = (int *) psp_cGetMemory(task->size_st * sizeof(int), 0);
        imp_g.rmt = (int *) psp_cGetMemory(totmems * sizeof(int), 0);

        if (imp_g.rbs && imp_g.rbe && imp_g.bs && imp_g.rmt)
        {
            for (cur_rmt = 0, i = 0; i < task->size_rt; i++)
            {
                imp_g.rbs[i] = cur_rmt;
                imp_g.rbe[i] = cur_rmt;

                for (j = 0; j < task->size_st; j++)
                {
                    start = task->set_table[j].st_members;
                    end = start + task->set_table[j].st_memtot;

                    for (k = start; k < end; k++)
                    {
                        if (task->member_table[k].mt_record == i)
                        {
                            imp_g.rbe[i]++;
                            imp_g.rmt[cur_rmt++] = j;
                        }
                    }
                }
            }
            ret = dbexec(imp_g.specstart);

            psp_zFreeMemory(&imp_g.rbs, 0);
            psp_zFreeMemory(&imp_g.rbe, 0);
            psp_zFreeMemory(&imp_g.bs, 0);
            psp_zFreeMemory(&imp_g.rmt, 0);
        }
        else
            dberr(ret = S_NOMEMORY);
    }
    else
        vftprintf(stderr, DB_TEXT("Unable to create import key\n"));

    d_close(task);
    psp_fileRemove(imp_g.keyfile);

    return ret;
}


VXSTARTUP("dbimp", 0)

