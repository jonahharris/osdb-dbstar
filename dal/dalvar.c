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

    dalvar.c - DAL variables handling.

    These functions allocate, store, lookup, and free the variables
    used by DAL to store records, fields, and literals.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "daldef.h"

/* ********************** TYPE DEFINITIONS *************************** */
struct varlist
{
    int v_type;
    int v_ndx;
    DB_TCHAR v_name[10];
    char *v_cont;
};

/* ********************** GLOBAL VARIABLE DECLARATIONS *************** */
static int varsize = 0, litnum = 0;
static struct varlist vars[100];


void init_dalvar()
{
    /* for VxWorks - initialize globals explicitly */
    varsize = litnum = 0;
    memset(vars, 0, sizeof(vars));
}


void freevar()
{
    int i;

    for (i = 0; i < varsize; i++)
    {
        psp_freeMemory(vars[i].v_cont, 0);
        vars[i].v_cont = NULL;
    }
    litnum = 0;
    varsize = 0;
}

char *addvar(int type, int ndx, DB_TCHAR *name, int size)
{
    vars[varsize].v_type = type;
    vars[varsize].v_ndx = ndx;
    vtstrcpy(vars[varsize].v_name, name);
    vars[varsize].v_cont = (char *) psp_getMemory(size, 0);
    if (vars[varsize].v_cont == NULL)
    {
        vftprintf(stderr, DB_TEXT("Out of memory!\n"));
        return (NULL);
    }
    varsize++;
    return (vars[varsize - 1].v_cont);
}

char *findvar(int type, DB_TCHAR *name, int *ndx)
{
    int i;

    for (i = 0; i < varsize; i++)
    {
        if (vars[i].v_type == type && vtstrcmp(vars[i].v_name, name) == 0)
        {
            *ndx = vars[i].v_ndx;
            return (vars[i].v_cont);
        }
    }
    return (NULL);
}

DB_TCHAR *genlit(DB_TCHAR *literal)
{
    static DB_TCHAR litname[10] = DB_TEXT("");
    int i;
    DB_TCHAR *p;

    /* search for existing identical literal */
    for (i = 0; i < varsize; i++)
        if (vars[i].v_type == LITERAL
         && vtstrcmp((DB_TCHAR *)vars[i].v_cont, literal) == 0)
            return (vars[i].v_name);

    vstprintf(litname, DB_TEXT("))%d(("), litnum++);
    p = (DB_TCHAR *)addvar(LITERAL, -1, litname,
        (vtstrlen(literal) + 1) * sizeof(DB_TCHAR));
    vtstrcpy(p, literal);
    return (litname);
}


