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

    dbexec.c - Import specification execution executive function

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "impdef.h"
#include "impvar.h"

/* ********************** LOCAL VARIABLE DECLARATIONS **************** */
static int lcount;


int dbexec(struct spec *start)
{
    struct spec *sp;

    sp = start;

    imp_g.loop_lvl = 0;

    /* for each entity specification */
    while (sp && !imp_g.abort_flag)
    {
        /* select the spec type: LOOP, ENDLOOP, CONNECT, REC */
        switch (sp->sp_type)
        {

            case LOOP:
                /* begin processing of a loop, place on stack if first time */
                {
                    struct loop *l;

                    l = sp->u.sp_looptr;

                    /* open the associated file if it is not opened */
                    if (l->l_fp == NULL)
                    {
                        l->l_fp = dbf_open(l->l_fname);
                        l->l_numrecs = 0;
                        imp_g.curloop[imp_g.loop_lvl++] = sp;
                        lcount = 1;
                    }

                    if (l->l_fp == NULL)
                    {
                        if (!imp_g.silent)
                        {
                            vftprintf(stderr,
                                DB_TEXT("\n**WARNING** file '%s' not found\n"),
                                l->l_fname);
                        }
                        /* fall thru to the dbf_read() and let it do the cleanup. */
                    }

                    /* read one data line from the file, exit loop if EOF */
                    if (!dbf_read(l->l_fp, l->l_line))
                    {
                        imp_g.loop_lvl--;
                        dbf_close(l->l_fp);
                        l->l_fp = NULL;
                        do
                        {
                            sp = sp->sp_next;
                        } while (sp->sp_type != ENDLOOP);

                        sp = sp->sp_next;
                        break;
                    }
                    if (!imp_g.silent)
                    {
                        vftprintf(stderr, DB_TEXT("[%s:%04d] %s\n"),
                            l->l_fname, lcount++, l->l_line);
                    }
                }
                sp = sp->sp_next;
                break;

            case ENDLOOP:
                /* pop the loop stack */
                sp = imp_g.curloop[imp_g.loop_lvl - 1];
                break;

            case RECORD:
                /* execute a record spec */
                if (rec_exec(sp->u.sp_recptr) == FAILURE)
                {
                    if (imp_g.abort_flag)
                        break;

                    /* skip until the ENDLOOP */
                    if (!imp_g.silent)
                    {
                        vftprintf(stderr,
                            DB_TEXT("**WARNING** skipping to end of FOREACH loop\n"));
                    }
                    do
                    {
                        sp = sp->sp_next;
                    } while (sp->sp_type != ENDLOOP);
                }
                else
                    sp = sp->sp_next;
                break;

            case CONNECT:
                /* execute a record connection */
                if (con_exec(sp->u.sp_conptr) == FAILURE)
                {
                    if (imp_g.abort_flag)
                        break;

                    /* skip until the ENDLOOP */
                    if (!imp_g.silent)
                    {
                        vftprintf(stderr,
                            DB_TEXT("**WARNING** skipping to end of FOREACH loop\n"));
                    }
                    do
                    {
                        sp = sp->sp_next;
                    } while (sp->sp_type != ENDLOOP);
                }
                else
                    sp = sp->sp_next;
                break;

            default:
                /* should never end up here */
                vftprintf(stderr, DB_TEXT("bad spec value: %d\n"), sp->sp_type);
                break;
        }
    }
    return (imp_g.abort_flag);
}


