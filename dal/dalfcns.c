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

    dalfcns.c - Functions used during command compilation.

    These functions are called during the compilation phase of
    DAL.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "daldef.h"
#include "dalvar.h"

/* ********************** EXTERNAL VARIABLE DECLARATIONS ************* */

void dalerror(DB_TCHAR *str)
{
    vftprintf(stderr, DB_TEXT("%s\n"), str);
}

void newinst(INST **ppi)
{
    if ((*ppi = (INST *) psp_cGetMemory(sizeof(INST), 0)) == NULL)
        vftprintf(stderr, DB_TEXT("dal: Out of memory\n"));
}

void newprint(PRINTFIELD **ppp)
{
    if ((*ppp = (PRINTFIELD *) psp_cGetMemory(sizeof(PRINTFIELD), 0)) == NULL)
        vftprintf(stderr, DB_TEXT("dal: Out of memory\n"));
}

int EXTERNAL_FCN def_rec(int rec, char *recptr, DB_TASK *task, int dbn)
{
    return (0);
}

int EXTERNAL_FCN def_fld(int fld, char *fldptr, DB_TASK *task, int dbn)
{
    return (0);
}

int EXTERNAL_FCN get_clock (clock_t *ctptr)
{
    *ctptr = clock();
    return (0);
}

int EXTERNAL_FCN cmp_clock (clock_t *start, clock_t *stop, clock_t *elap)
{
     *elap = *stop - *start;
     return (0);
}


