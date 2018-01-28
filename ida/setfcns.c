/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* -------------------------------------------------------------------------
    IDA - Set membership scan control functions
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "ddlnms.h"
#include "ida_d.h"
#include "ida.h"

/*************************** LOCAL VARIABLES ******************************/

#define RPP 16                            /* records per page */
static long       tot_members;            /* total members in set */
static int        pgtot;                  /* total members on page */
static int        tot_pages;              /* total pages */
static int        cur_page;               /* current page displayed */
static int        top_member;             /* entry number of top member on page */
static char       rectxt[255];            /* record display text */
static char       set_order[12];          /* set ordering string */
static DB_ADDR    seldba[RPP];            /* selection page dba's */
static int        last_set;               /* last set selected */
static char      *memtbl[RPP] = {NULL};
static int        set = -1;               /* #defined set number */

/************************** FORWARD REFERENCES ****************************/
void ssnfcn(void);

/* ========================================================================
    Scan and view set
*/
void ssfcn()
{
    register int i;

    if (memtbl[0] == NULL)
    {
        for (i = 0; i < RPP; ++i)
        {
            if ((memtbl[i] = malloc(80)) == NULL)
            {
                usererr("cannot allocate display memory");
                memtbl[0] = NULL;
                return;
            }
        }
    }
    if ((set = set_select()) == -1)
        return;
    if (d_members(set, &tot_members, task, CURR_DB) != S_OKAY)
        return;
    if (tot_members == 0)
    {
        usererr("set has no members");
        return;
    }
    tot_pages = (int) tot_members / RPP + (tot_members % RPP == 0 ? 0 : 1);
    cur_page = 0;
    switch (task->set_table[set - SETMARK].st_order)
    {
        case ASCENDING:   strcpy(set_order, "ascending");     break;
        case DESCENDING:  strcpy(set_order, "descending");    break;
        case FIRST:       strcpy(set_order, "first");         break;
        case LAST:        strcpy(set_order, "last");          break;
        case NEXT:        strcpy(set_order, "next");          break;
        default:          break;
    }
    ssnfcn();
    menu(SS_MENU);
}

/* ========================================================================
    Scan next page of members
*/
void ssnfcn()
{
    int rtype;
    register int i;
    DB_ADDR dba;
    char *rec;

    if (cur_page == tot_pages)
    {
        usererr("last page");
        return;
    }
    ++cur_page;
    if (cur_page == 1)
    {
        if (d_findfm(set, task, CURR_DB) != S_OKAY)
            return;
    }
    top_member = (cur_page - 1) * RPP + 1;
    for ( pgtot = 0;
          (pgtot < RPP) && (task->db_status == S_OKAY);
          d_findnm(set, task, CURR_DB), ++pgtot)
    {
        d_crtype(&rtype, task, CURR_DB);
        rtype -= RECMARK;
        d_csmget(set, &dba, task, CURR_DB);
        if (dio_read(dba, &rec, NOPGHOLD , task) != S_OKAY)
            return;
        memcpy(&(seldba[pgtot]), &dba, DB_ADDR_SIZE);
        rectotxt(rec, rectxt);
        rectxt[54] = '\0';
        if (dio_release(dba, NOPGFREE , task) != S_OKAY)
            return;
        sprintf(memtbl[pgtot], "%-16s  %-42s", task->record_names[rtype], rectxt);
    }
    tprintf("@m0300@E\n@SSet:@s %-16s  ", task->set_names[set - SETMARK]);
    tprintf("@SOrder:@s %-10s  ", set_order);
    tprintf("@STotal members:@s %-5ld   ", tot_members);
    tprintf("@SPage@s %d", cur_page);
    tprintf(" @Sof@s %d\n\n", tot_pages);
    tprintf("@SENTRY RECORD TYPE       RECORD CONTENTS@s\n");
    for (i = 0; i < pgtot; ++i)
    {
        tprintf("%4d.", top_member + i);
        tprintf(" %s\n", memtbl[i]);
    }
}

/* ========================================================================
    Redisplay first page of set
*/
void ssffcn()
{
    cur_page = 0;
    ssnfcn();
}

/* ========================================================================
    Select member record from current page
*/
void sssfcn()
{
    register int i;

    if ((i = list_selection(7, pgtot, memtbl, top_member, 0, 0)) != -1)
    {
        tprintf("@c");
        d_crset(&(seldba[i]), task, CURR_DB);
        d_csmset(set, &(seldba[i]), task, CURR_DB);
        disp_rec(&(seldba[i]));
        menu(R_MENU);
    }
}

/* ========================================================================
    Display current owner of set
*/
void sofcn()
{
    if ((set = set_select()) != -1)
    {
        if (d_findco(set, task, CURR_DB) == S_OKAY)
            disp_rec(NULL);
    }
}

/* ========================================================================
    Find first member of set
*/
void sffcn()
{
    if ((set = set_select()) != -1)
    {
        if (d_findfm(set, task, CURR_DB) == S_OKAY)
        {
            disp_rec(NULL);
        }
        else if (task->db_status == S_EOS)
        {
            set = -1;
            usererr("no more members in set");
        }
    }
}

/* ========================================================================
    Find next member of set
*/
void snfcn()
{
    if (set == -1)
    {
        usererr("must do first before doing next");
    }
    else if (d_findnm(set, task, CURR_DB) == S_OKAY)
    {
        disp_rec(NULL);
    }
    else if (task->db_status == S_EOS)
    {
        set = -1;
        usererr("no more members in set");
    }
}

/* ========================================================================
    Find last member of set
*/
void slfcn()
{
    if ((set = set_select()) != -1)
    {
        if (d_findlm(set, task, CURR_DB) == S_OKAY)
        {
            disp_rec(NULL);
        }
        else if (task->db_status == S_EOS)
        {
            set = -1;
            usererr("no more members in set");
        }
    }
}

/* ========================================================================
    Find previous member of set
*/
void spfcn()
{
    if (set == -1)
    {
        usererr("must do last before doing previous");
    }
    else if (d_findpm(set, task, CURR_DB) == S_OKAY)
    {
        disp_rec(NULL);
    }
    else if (task->db_status == S_EOS)
    {
        set = -1;
        usererr("no more members in set");
    }
}

/* ========================================================================
    Connect current record to set
*/
void scfcn()
{
    if ((set = set_select()) != -1)
    {
        if (d_connect(set, task, CURR_DB) == S_OKAY)
            usererr("record connected to set");
    }
}

/* ========================================================================
    Disconnect current member of set
*/
void sdfcn()
{
    if ((set = set_select()) != -1)
    {
        if (d_discon(set, task, CURR_DB) >= S_OKAY)
            usererr("record disconnected from set");
    }
}

/* ========================================================================
    Display total members of set
*/
void stfcn()
{
    long tot;
    char total[20];

    if ((set = set_select()) != -1)
    {
        if (d_members(set, &tot, task, CURR_DB) == S_OKAY)
        {
            sprintf(total, "%ld", tot);
            tprintf("@m0400Set %s has %s members\n",
                task->set_names[set - SETMARK], total);
        }
    }
}

/* ========================================================================
    Select set for processing
*/
int set_select()
{
    int set = 0;

    if (task->size_st)
    {
        tprintf("@m0400@ESELECT SET:\n\n");
        last_set = list_selection(6, task->size_st, task->set_names, 0, last_set, 1);
        if (last_set >= 0)
            set = last_set + SETMARK;
        tprintf("@m0300@E");
    }
    else
    {
        usererr("no sets defined in database");
        set = -1;
    }
    return (set);
}


