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
    IDA - Currency Manipulation Functions
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "ddlnms.h"
#include "ida_d.h"
#include "ida.h"

/*************************** GLOBAL VARIABLES *****************************/
int autoset = 0;                       /* automatic set connection flag */

/*************************** LOCAL  VARIABLES *****************************/
#define EPP 14                         /* currency table entries per page */
static int tot_entries;                /* total number of table entries */
static int pgtot;                      /* total entries on page */
static int tot_pages;                  /* total pages */
static int cur_page;                   /* current page number */
static int top_entry;                  /* entry number at top of page */
static char **curtbl = NULL;           /* currency table entries table */
static int set1, set2;                 /* set selection values */
static int last_sel = 0;               /* last selection */

/************************ FORWARD DECLARATIONS ****************************/
void cdnfcn(void);

/* ========================================================================
    Current owner set selection
*/
void cofcn()
{
    if ((set1 = set_select()) != -1)
        menu(CO_MENU);
}

/* ========================================================================
    Current member set selection
*/
void cmfcn()
{
    if ((set1 = set_select()) != -1)
        menu(CM_MENU);
}

/* ========================================================================
    Current record selection
*/
void crfcn()
{
    menu(CR_MENU);
}

/* ========================================================================
    Toggle automatic set connection flag
*/
void cafcn()
{
    if (autoset)
    {
        autoset = 0;
        usererr("automatic set connection is OFF");
    }
    else
    {
        autoset = 1;
        usererr("automatic set connection is ON");
    }
}

/* ========================================================================
    Display currency table
*/
void cdfcn()
{
    int            setc,
                   rectype,
                   cotype,
                   cmtype;
    register int   i;
    short          fno;
    DB_ULONG       rno;

    tot_entries = 2 * task->size_st + 1;
    tot_pages = tot_entries / EPP + (tot_entries % EPP == 0 ? 0 : 1);
    cur_page = 0;
    if (curtbl == NULL)
    {
        curtbl = (char **) calloc(tot_entries, sizeof(char *));
        for (i = 0; i < tot_entries; ++i)
        {
            if (curtbl == NULL || (curtbl[i] = malloc(80)) == NULL)
            {
                usererr("Unable to allocate sufficient memory");
                return;
            }
        }
    }
    if (task->curr_rec)
    {
        if (d_crtype(&rectype, task, CURR_DB) != S_OKAY)
            return;
        rectype -= RECMARK;
    }
    d_decode_dba(task->curr_rec, &fno, &rno);
    sprintf(curtbl[0], "Record                                %-16s  [%d:%lu]",
        task->curr_rec ? task->record_names[rectype] : "**NULL**", fno, rno);

    for (i = 0; i < task->size_st; ++i)
    {
        setc = i + SETMARK;
        if (task->curr_own[i])
        {
            if (d_cotype(setc, &cotype, task, CURR_DB) != S_OKAY)
                return;
            cotype -= RECMARK;
        }
        d_decode_dba(task->curr_own[i], &fno, &rno);
        sprintf(curtbl[2 * i + 1], "owner of  %-26s  %-16s  [%d:%lu]",
            task->set_names[i], (task->curr_own[i]) ? task->record_names[cotype] : "**NULL**",
            fno, rno);

        if (task->curr_mem[i])
        {
            if (d_cmtype(setc, &cmtype, task, CURR_DB) != S_OKAY)
                return;
            cmtype -= RECMARK;
        }
        d_decode_dba(task->curr_mem[i], &fno, &rno);
        sprintf(curtbl[2 * i + 2], "member of %-26s  %-16s  [%d:%ld]",
            task->set_names[i], (task->curr_mem[i]) ? task->record_names[cmtype] : "**NULL**",
            fno, rno);
    }
    cdnfcn();
    menu(CD_MENU);
}

/* ========================================================================
    Display next page of currency entries
*/
void cdnfcn()
{
    register int i;

    if (cur_page == tot_pages)
    {
        usererr("last page");
        return;
    }
    top_entry = (++cur_page - 1) * EPP;
    pgtot = (tot_entries - top_entry < EPP) ? tot_entries - top_entry : EPP;
    tprintf("@m0300@E\n@SCURRENCY TABLE@s");
    if (autoset)
        tprintf("@m0425@Rautomatic set connection@r");
    tprintf("@m0460@SPage@s %d ", cur_page);
    tprintf(" @Sof@s %d", tot_pages);
    tprintf("@m0600@SENTRY@s");
    tprintf("@m0644@SRECORD@s");
    tprintf("@m0661@SDB_ADDR@s\n");
    for (i = top_entry; i < tot_entries && i - top_entry < EPP; ++i)
    {
        tprintf("%4d.", i);
        tprintf(" %s\n", curtbl[i]);
    }
}

/* ========================================================================
    Display previous page of currency entries
*/
void cdpfcn()
{
    if (cur_page <= 1)
    {
        usererr("first page");
        return;
    }
    cur_page -= 2;
    cdnfcn();
}

/* ========================================================================
    Select record from currency entries
*/
void cdsfcn()
{
    register int i;

    i = list_selection(7, pgtot, &(curtbl[top_entry]), top_entry, last_sel, 0);
    if (i == -1)
        return;
    last_sel = i;
    i = top_entry + i;
    tprintf("@c");
    if (i == 0)
        disp_rec(NULL);
    else if (i % 2 == 1)
        disp_rec(&(task->curr_own[i / 2]));
    else
        disp_rec(&(task->curr_mem[i / 2 - 1]));
}

/* ========================================================================
    Exit currency display menu
*/
void cdxfcn()
{
    register int i;

    if (curtbl)
    {
        for (i = 0; i < tot_entries; ++i)
        {
            if (curtbl[i])
                free(curtbl[i]);
        }
        free((char *) curtbl);
        curtbl = NULL;
    }
    tprintf("@c");
}

/* ========================================================================
    Set current owner to current record
*/
void corfcn()
{
    d_setor(set1, task, CURR_DB);
}

/* ========================================================================
    Change current owner to current owner
*/
void coofcn()
{
    if ((set2 = set_select()) != -1)
        d_setoo(set1, set2, task, CURR_DB);
}

/* ========================================================================
    Change current owner to current member
*/
void comfcn()
{
    if ((set2 = set_select()) != -1)
        d_setom(set1, set2, task, CURR_DB);
}

/* ========================================================================
    Change current owner to database address
*/
void cocfcn()
{
    DB_ADDR dba;

    switch (rd_dba(&dba))
    {
        case -1:    break;
        case 0:     usererr("invalid database address");   break;
        default:    d_csoset(set1, &dba, task, CURR_DB);         break;
    }
}

/* ========================================================================
    Change current member to current record
*/
void cmrfcn()
{
    d_setmr(set1, task, CURR_DB);
}

/* ========================================================================
    Change current owner to current owner
*/
void cmofcn()
{
    if ((set2 = set_select()) != -1)
        d_setmo(set1, set2, task, CURR_DB);
}

/* ========================================================================
    Change current member to current member
*/
void cmmfcn()
{
    if ((set2 = set_select()) != -1)
        d_setmm(set1, set2, task, CURR_DB);
}

/* ========================================================================
    Change current member to database address
*/
void cmcfcn()
{
    DB_ADDR dba;

    switch (rd_dba(&dba))
    {
        case -1:   break;
        case 0:    usererr("invalid database address");    break;
        default:   d_csmset(set1, &dba, task, CURR_DB);          break;
    }
}

/* ========================================================================
    Change current record to current owner
*/
void crofcn()
{
    if ((set1 = set_select()) != -1)
        d_setro(set1, task, CURR_DB);
}

/* ========================================================================
    Change current record to current member
*/
void crmfcn()
{
    if ((set1 = set_select()) != -1)
        d_setrm(set1, task, CURR_DB);
}

/* ========================================================================
    Change current record to database address
*/
void crcfcn()
{
    DB_ADDR dba;

    switch (rd_dba(&dba))
    {
        case -1:    break;
        case 0:     usererr("invalid database address");   break;
        default:    d_crset(&dba, task, CURR_DB);                break;
    }
}

/* ========================================================================
    Change current owner of set to current record: edit record edition
*/
void setor()
{
    if ((set1 = set_select()) != -1)
    {
        if (d_setor(set1, task, CURR_DB) == S_OKAY)
            usererr("current owner changed to current record");
    }
    disp_rec(NULL);
}

/* ========================================================================
    Test current record timestamp
*/
void ctrfcn()
{
    char msg[80];
    int rec;

    if (!task->db_tsrecs)
        usererr("no record types are timestamped");
    else if (d_crtype(&rec, task, CURR_DB) == S_OKAY)
    {
        rec -= RECMARK;
        sprintf(msg, "current record (%s) ", task->record_names[rec]);
        if (d_crstat(task, CURR_DB) == S_OKAY)
            strcat(msg, "has not changed");
        else if (task->db_status == S_UPDATED)
            strcat(msg, "has been modified");
        else if (task->db_status == S_DELETED)
            strcat(msg, "has been deleted");
        else
            return;
        strcat(msg, " since last accessed");
        tprintf("@m2200@e@R%s@r\n", msg);
        tprintf("@Rpress any key to continue@r");
        tgetch();
        tprintf("@m0300@E");
    }
}

/* ========================================================================
    Test current owner timestamp
*/
void ctofcn()
{
    char msg[80];
    int set, rec, sn;

    if (!task->db_tsrecs)
        usererr("no record types are timestamped");
    else if ((set = set_select()) != -1)
    {
        if (d_cotype(set, &rec, task, CURR_DB) == S_OKAY)
        {
            rec -= RECMARK;
            sn = set - SETMARK;
            sprintf(msg,
                "current owner (%s) of %s ", task->record_names[rec], task->set_names[sn]);
            if (d_costat(set, task, CURR_DB) == S_OKAY)
                strcat(msg, "has not changed");
            else if (task->db_status == S_UPDATED)
                strcat(msg, "has been modified");
            else if (task->db_status == S_DELETED)
                strcat(msg, "has been deleted");
            else
                return;
            strcat(msg, " since last accessed");
            tprintf("@m2200@e@R%s@r\n", msg);
            tprintf("@Rpress any key to continue@r");
            tgetch();
            tprintf("@m0300@E");
        }
    }
}

/* ========================================================================
    Test current member timestamp
*/
void ctmfcn()
{
    char msg[80];
    int set, rec, sn;

    if (!task->db_tsrecs)
        usererr("no record types are timestamped");
    else if ((set = set_select()) != -1)
    {
        if (d_cmtype(set, &rec, task, CURR_DB) == S_OKAY)
        {
            rec -= RECMARK;
            sn = set - SETMARK;
            sprintf(msg, "current member (%s) of %s ",
                task->record_names[rec], task->set_names[sn]);
            if (d_cmstat(set, task, CURR_DB) == S_OKAY)
                strcat(msg, "has not changed");
            else if (task->db_status == S_UPDATED)
                strcat(msg, "has been modified");
            else if (task->db_status == S_DELETED)
                strcat(msg, "has been deleted");
            else
                return;
            strcat(msg, " since last accessed");
            tprintf("@m2200@e@R%s@r\n", msg);
            tprintf("@Rpress any key to continue@r");
            tgetch();
            tprintf("@m0300@E");
        }
    }
}

/* ========================================================================
    Test current set timestamp
*/
void ctsfcn()
{
    char msg[80];
    int sn, set;

    if (!task->db_tssets)
        usererr("no set types are timestamped");
    else if ((set = set_select()) != -1)
    {
        sn = set - SETMARK;
        sprintf(msg, "current instance of set %s ", task->set_names[sn]);
        if (d_csstat(set, task, CURR_DB) == S_OKAY)
            strcat(msg, "has not changed");
        else if (task->db_status == S_UPDATED)
            strcat(msg, "has been modified");
        else
            return;
        strcat(msg, " since last accessed");
        tprintf("@m2200@e@R%s@r\n", msg);
        tprintf("@Rpress any key to continue@r");
        tgetch();
        tprintf("@m0300@E");
    }
}


