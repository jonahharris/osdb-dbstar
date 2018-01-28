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

    ddlxref.c - db.* DDLP symbol cross-reference module

    This module is called to produce the cross-reference listing
    when the -x option is used on the ddlp command line.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "version.h"
#include "parser.h"
#include "ddldefs.h"

/* ********************** TYPE DEFINITIONS *************************** */
/* source ddlp_g.line reference */
typedef struct line_ref {
    struct line_ref *ln_next;     /* next reference ptr */
    int ln_num;                   /* ddlp_g.line number */
} LINE_REF;

/* name cross reference record */
typedef struct xref {
    struct xref *x_next;          /* next name ptr */
    DB_TCHAR *x_name;             /* record, field, set name */
    DB_TCHAR *x_rec;              /* record name, if x_name is field */
    char x_type;                  /* 'r' = record, 'f' = field, 's' = set */
    LINE_REF x_refs;              /* first ddlp_g.line reference */
} XREF;

/* ********************** LOCAL VARIABLE DECLARATIONS **************** */
/* DDL cross-ref table */
static XREF *ddl_xrefs = NULL;


/* Initialize static variables, etc. */
void xrefInit (void)
{
    ddl_xrefs = NULL;
}


/* Add symbol reference
*/
void add_xref(DB_TCHAR *rname, DB_TCHAR *name, char type, int line_num)
{
    int               cmp;
    XREF             *xn;
    LINE_REF         *lp,
                     *lq;
    register XREF    *xp,
                     *xq;

    if (!ddlp_g.x_flag || line_num == 0L)
        return;

    for (cmp = 1, xq = NULL, xp = ddl_xrefs; xp; xp = xp->x_next)
    {
        if ((cmp = vtstrcmp(name, xp->x_name)) <= 0)
        {
            if (cmp == 0 && ddlp_g.d_flag && type == 'f' && xp->x_type == 'f')
            {
                if ((cmp = vtstrcmp(rname, xp->x_rec)) == 0)
                    break;
            }
            else
            {
                break;
            }
        }
        xq = xp;
    }
    if (cmp == 0)
    {
        xn = xp;
    }
    else
    {
        if ((xn = (XREF *) psp_cGetMemory(sizeof(XREF), 0)) == NULL)
        {
            ddlp_abort(DB_TEXT("out of memory"));
            return;
        }

        if ((xn->x_name = psp_strdup(name, 0)) == NULL)
        {
            ddlp_abort(DB_TEXT("out of memory"));
            return;
        }

        if (type == 'f')
        {
            if ((xn->x_rec = psp_strdup(rname, 0)) == NULL)
            {
                ddlp_abort(DB_TEXT("out of memory"));
                return;
            }
        }

        xn->x_type = type;
        if (xq)
        {
            xq->x_next = xn;
            xn->x_next = xp;
        }
        else
        {
            xn->x_next = xp;
            ddl_xrefs = xn;
        }
    }

    if (xn->x_refs.ln_num == 0)
        xn->x_refs.ln_num = line_num;
    else
    {
        for (lq = NULL, lp = xn->x_refs.ln_next; lp; lp = lp->ln_next)
            lq = lp;

        lp = (LINE_REF *)psp_cGetMemory(sizeof(LINE_REF), 0);
        if (lp == NULL)
        {
            ddlp_abort(DB_TEXT("out of memory"));
            return;
        }

        lp->ln_num = line_num;
        if (lq)
            lq->ln_next = lp;
        else
            xn->x_refs.ln_next = lp;
    }
}


/* Print cross-reference listing
*/
void print_xref (void)
{
    register XREF       *xp;
    register LINE_REF   *lp;
    int                  lines,
                         page,
                         refs;
    time_t               Clock;

    time(&Clock);
    lines = 55;
    page = 0;
    for (xp = ddl_xrefs; xp; xp = xp->x_next)
    {
        if (++lines > 55)
        {
            /* Print new page header */
            vftprintf(ddlp_g.outfile, DB_TEXT("\f\n"));
            vftprintf(ddlp_g.outfile, DB_TEXT("%72s %2d\r"), "Page", ++page);
            vftprintf(ddlp_g.outfile, DB_TEXT("db.* %s, DDL X-Ref Listing of File: %s\n"), DBSTAR_VERSION, ddlp_g.ddlfile);
            vftprintf(ddlp_g.outfile, DB_TEXT("%s\n"), vtctime(&Clock));
            lines = 0;
        }
        vftprintf(ddlp_g.outfile, DB_TEXT("%-24.24s "), xp->x_name);
        switch (xp->x_type)
        {
            case 'r':   vftprintf(ddlp_g.outfile, DB_TEXT("record "));  break;
            case 'f':   vftprintf(ddlp_g.outfile, DB_TEXT("field  "));  break;
            case 's':   vftprintf(ddlp_g.outfile, DB_TEXT("set    "));  break;
            default:    break;
        }
        for (refs = 0, lp = &(xp->x_refs); lp; lp = lp->ln_next)
        {
            if (refs++ > 8)
            {
                refs = 1;
                ++lines;
                vftprintf(ddlp_g.outfile, DB_TEXT("\n%32c"), ' ');
            }
            vftprintf(ddlp_g.outfile, DB_TEXT(" %4d"), lp->ln_num);
        }
        vftprintf(ddlp_g.outfile, DB_TEXT("\n"));
    }
}


/* Free cross-references
*/
void free_xref_list (void)
{
    XREF *p, *x_next;
    LINE_REF *x_ref, *ln_next;

    for (p = ddl_xrefs; p; p = x_next)
    {
        x_next = p->x_next;
        for (x_ref = p->x_refs.ln_next; x_ref; x_ref = ln_next)
        {
            ln_next = x_ref->ln_next;
            psp_freeMemory(x_ref, 0);
        }

        if (p->x_name)
            psp_freeMemory(p->x_name, 0);

        if (p->x_rec)
            psp_freeMemory(p->x_rec, 0);

        psp_freeMemory(p, 0);
    }
}


