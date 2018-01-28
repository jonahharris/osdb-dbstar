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

    dalwhile.c - Execute a whileok loop.

    This function will execute everything within one whileok loop.  It
    will recursively call itself to handle nested whileok's.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "daldef.h"
#include "dalvar.h"


void freeloop(INST **ppi)
{
    INST *instr, *next;

    instr = (*ppi)->i_loop;
    while (instr)
    {
        next = instr->i_next;
        if (vtstrcmp(instr->i_name, DB_TEXT("while")) == 0)
            freeloop(&instr);
        else
            freeinst(&instr);
        instr = next;
    }
    psp_freeMemory(*ppi, 0);
    *ppi = NULL;
}


void freeinst(INST **ppi)
{
    PRINTFIELD *p, *next;

    p = (*ppi)->i_pfld;
    while (p)
    {
        next = p->pf_next;
        psp_freeMemory(p, 0);
        p = next;
    }
    psp_freeMemory(*ppi, 0);
    *ppi = NULL;
}


int dalwhile(INST *pi)
{
    INST *instr;
    int stat;

    stat = S_OKAY;
    while (stat == S_OKAY)
    {
        instr = pi->i_loop;
        while (instr)
        {
            if (vtstrcmp(instr->i_name, DB_TEXT("while")) == 0)
                stat = dalwhile(instr);
            else
                stat = dalexec(instr);

            if (stat)
                break;
            instr = instr->i_next;
        }
    }
    if (stat > 0)
        return (S_OKAY);
    return (stat);
}


