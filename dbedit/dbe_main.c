/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbedit utility                                    *
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

    dbe_main.c - DBEDIT main program

    This function consists of a loop which inputs, parses, and processes
    one command line with each iteration.

-----------------------------------------------------------------------*/

#define MOD dbedit
#include "db.star.h"
#include "version.h"
#include "dbe_type.h"
#include "dbe_err.h"
#include "dbe_str.h"
#include "dbe_io.h"
#include "dbe_ext.h"


/* ********************** GLOBAL VARIABLE DECLARATIONS *************** */

DBE_SLOT slot;
int decimal, titles, fields, changed, unicode;

/* ------------------------------------------------------------------------ */

int MAIN(int argc, DB_TCHAR **argv)
{
    int      error = 0,
             err,
             exit_code = 0,
             tindex;
    DBE_TOK  tokens[TBUFFLEN];
    DB_TCHAR line[LINELEN],
            *lptr;
    DB_TCHAR errstr[NAMELEN];
    DB_TASK *task;

    dbe_ioinit();

    /* Print the copyright message */
    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Database Edit")));

    /* Check for usage */
    if (argc <= 1)
    {
        dbe_err(USAGE, NULL);
        return (1);
    }

    /* Initialize - dbe_init gets database name from argv and opens it */
    if ((error = d_opentask(&task)) != S_OKAY)
    {
        dbe_err(ERR_OPEN, NULL);
        return (1);
    }
    if ((error = dbe_init(argc, argv, task)) < 0)
    {
        d_closetask(task);
        dbe_err(error, NULL);
        return (1);
    }

    /* Get input line-by-line and process it */
    dbe_select(0);
    while (exit_code == 0)
    {
        lptr = line;
        errstr[0] = 0;
        err = tindex = 0;
        memset(tokens, 0, sizeof(tokens));

        /* Get command line in ascii form */
        error = dbe_getline(dbe_getstr(M_PROMPT), line, sizeof(line) / sizeof(DB_TCHAR));
        if (!error && !line[0])
            continue;

        /* Convert ascii to tokens & process - errors < 0, warnings > 0 */
        if (error >= 0)
        {
            err = parse(&lptr, &tindex, tokens, TBUFFLEN,
                        0, errstr, sizeof(errstr), task);
            err = process(tokens, tindex, &exit_code, errstr, err, task);
        }
        if (err)
            error = err;
        if (error)
            dbe_err(error, errstr);
    }

    /* Terminate */
    if ((error = dbe_term(errstr, task)) < 0)
        dbe_err(error, errstr);

    return 0;
}


/* ------------------------------------------------------------------------ */
/* Execute commands stored in token array */

int process(DBE_TOK *tokens, int max_tok, int *exit_code, DB_TCHAR *errstr,
            int error, DB_TASK *task)
{
    int tok, warning;

    warning = error;

    /* If error occurred in parsing it may have been source command. Input
     * file name cannot be handled by parser - always rejected */
    if (error < 0)
    {
        if (error == UNX_TOK
         && tokens[0].type == TYPE_KWD
         && tokens[0].ival == K_SOURCE)
            error = in_open(errstr);
        return error;
    }

    for (tok = error = 0; tok < max_tok && error >= 0;)
    {

        switch (tokens[tok].type)
        {

            case TYPE_KWD:
                switch (tokens[tok].ival)
                {
                    case K_SHOW:
                        error = show(tokens, &tok, task);
                        break;
                    case K_DISP:
                        error = disp(tokens, &tok, task);
                        break;
                    case K_BASE:
                        error = base(tokens, &tok);
                        break;
                    case K_EDIT:
                        error = edit(tokens, &tok, errstr, task);
                        break;
                    case K_GOTO:
                        error = dgoto(tokens, &tok, errstr, task);
                        break;
                    case K_VERIFY:
                        error = verify(tokens, &tok, errstr, task);
                        break;
                    case K_REREAD:
                        error = reread(task);
                        tok++;
                        break;
                    case K_QMARK:
                    case K_HELP:
                        error = help(0, task);
                        tok++;
                        break;
                    case K_NOTLS:
                        titles = 0;
                        tok++;
                        break;
                    case K_TITLES:
                        titles = 1;
                        tok++;
                        break;
                    case K_NOFLDS:
                        fields = 0;
                        tok++;
                        break;
                    case K_FIELDS:
                        fields = 1;
                        tok++;
                        break;
                    case K_SOURCE:
                        error = UNX_END;
                        tok++;
                        break;
                    case K_EXIT:
                        *exit_code = 1;
                        tok++;
                        break;
                    default:
                        tok++;
                        break;
                }
                break;

            default:
                tok++;
                break;
        }
    }
    return (error ? error : warning);
}


VXSTARTUP("dbedit", 0)

