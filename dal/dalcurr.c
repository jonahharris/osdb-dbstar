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

    dalcurr.c - Print the currency tables.

    This file contains the function to interpret and print the
    currency tables when the "currency" command is executed
    in DAL.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "daldef.h"
#include "dalvar.h"


/* Print db address
*/
static void pr_dba(FILE *dmpf, DB_ADDR dba)
{
    short       fno;
    DB_ADDR     rno;

    if (dba != NULL_DBA)
    {
        d_decode_dba(dba, &fno, &rno);
        vftprintf(dmpf, DB_TEXT("[%03d:%05lu]"), fno, rno);
    }
    else
    {
        vftprintf(dmpf, DB_TEXT("NULL      "));
    }
}

/* Print set owner record type
*/
void pr_otype(FILE *dmpf, int set, DB_ADDR dba)
{
    int cotype;
    DB_TASK *task = DalDbTask;    /* required for macros, e.g. task->record_names */

    if (dba != NULL_DBA)
    {
        set += SETMARK;
        d_cotype(set, &cotype, DalDbTask, CURR_DB);
        cotype -= RECMARK;
        vftprintf(dmpf, DB_TEXT("%-10.10s"), task->record_names[cotype]);
    }
    else
        vftprintf(dmpf, DB_TEXT("          "));
}

/* Print set member record type
*/
void pr_mtype(FILE *dmpf, int set, DB_ADDR dba)
{
    int cmtype;
    DB_TASK *task = DalDbTask;    /* required for macros, e.g. task->record_names */

    if (dba != NULL_DBA)
    {
        set += SETMARK;
        d_cmtype(set, &cmtype, DalDbTask, CURR_DB);
        cmtype -= RECMARK;
        vftprintf(dmpf, DB_TEXT("%-10.10s"), task->record_names[cmtype]);
    }
    else
        vftprintf(dmpf, DB_TEXT("          "));
}

/* Print currency tables */
int dalcurr(FILE *dmpf)
{
    register int i;
    DB_ADDR *dba_optr, *dba_mptr;
    int crtype;
    DB_TASK *task = DalDbTask;    /* required for macros, e.g. task->curr_rec */

    vftprintf(dmpf, DB_TEXT("\nCurrent Database: %s\n"), DB_REF(db_name));
    vftprintf(dmpf, DB_TEXT("Current Record  : "));
    if (task->curr_rec != NULL_DBA)
    {
        d_crtype(&crtype, task, CURR_DB);
        crtype -= RECMARK;
        vftprintf(dmpf, DB_TEXT("%s "), task->record_names[crtype]);
        pr_dba(dmpf, task->curr_rec);
        vftprintf(dmpf, DB_TEXT("\n\n"));
    }
    else
        vftprintf(dmpf, DB_TEXT("NONE\n\n"));

    if (task->size_st)
    {
        vftprintf(dmpf, DB_TEXT("   Set Name        | Current Owner         | Current Member\n"));
        vftprintf(dmpf, DB_TEXT("   --------------- | --------------------- | ---------------------\n"));
        for (i = 0, dba_optr = task->curr_own, dba_mptr = task->curr_mem;
              i < task->size_st; ++i, ++dba_optr, ++dba_mptr)
        {
            vftprintf(dmpf, DB_TEXT("   %-15.15s | "), task->set_names[i]);
            pr_otype(dmpf, i, *dba_optr);
            vftprintf(dmpf, DB_TEXT(" "));
            pr_dba(dmpf, *dba_optr);
            vftprintf(dmpf, DB_TEXT(" | "));
            pr_mtype(dmpf, i, *dba_mptr);
            vftprintf(dmpf, DB_TEXT(" "));
            pr_dba(dmpf, *dba_mptr);
            vftprintf(dmpf, DB_TEXT("\n"));
        }
    }
    return (0);
}


