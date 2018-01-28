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

    dbe_misc.c - DBEDIT, functions for miscellaneous commands

    The function base() handles the base command.
    The function reread() handles the reread command.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_err.h"
#include "dbe_ext.h"


/* Perform base command (set global flag)
*/
int base(DBE_TOK *tokens, int *tokenptr)
{
    int token, error;

    error = 0;
    token = *tokenptr + 1;
    if (tokens[token].lval == 10L)
        decimal = 1;
    else if (tokens[token].lval == 16L)
        decimal = 0;
    else
        error = BAD_BASE;
    *tokenptr = ++token;
    return (error);
}


/* Perform reread command
*/
int reread(DB_TASK *task)
{
    changed = 0;
    return (dbe_read(task->curr_rec, task));
}


