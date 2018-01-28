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
    IDA - Record Display & Edit Functions
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "ddlnms.h"
#include "ida_d.h"
#include "ida.h"
#include "keyboard.h"

/************************** EXTERNAL FUNCTIONS ****************************/
extern char *fldtotxt();

/*************************** GLOBAL VARIABLES *****************************/
char fldtxt[MAXSZ];                    /* field text buffer */

/************************** EXTERNAL VARIABLES ****************************/
extern int entry_flag;                 /* = 1 if enter record, 0 if modify */
extern int autoset;                    /* auto set connection flag */

/*************************** LOCAL  VARIABLES *****************************/
#define REV 1                          /* reverse video on */
#define NOREV 0                        /* reverse video off */
#define FPP 14                         /* fields per page */

static int rt;                         /* record type */
static char *rec;                      /* pointer to record area */
static int curpg;                      /* current page number */
static int totpg;                      /* total number of pages of data
                                                     * fields */
static int totflds;                    /* total number of editable fields */
static int topfld;                     /* fldlist entry at top of page */
static int *fldlist = NULL;            /* editable field list */
static int creat_flag = 0;             /* create record flag */
static char **optkeynms = NULL;        /* optional key field names */
static int *optkeyflds = NULL;         /* optional key field numbers */
static int tot_optkeys;                /* total opt keys in current record */
static int lastoptrec = -1;            /* last optional key record type */
static int okey = 0;                   /* optional key list selection */

/*************************** LOCAL  FUNCTIONS *****************************/
static void pr_field(int, int);
static void bld_optkey_tab(int);

/* ========================================================================
    Print data field contents
*/
static void pr_field(
    int fld,                            /* index to fldlist table */
    int rev)                            /* reverse video flag */
{
    int fno, rn, cn;
    register int i;

    if (rev)
        tprintf("@R");

    fno = fldlist[fld];
    rn = fld - topfld + 7;
    cn = 0;
    tprintf("@M@e%-16s >", &rn, &cn, task->field_names[fno]);
    fldtotxt(fno, rec + (task->field_table[fno].fd_ptr - task->record_table[rt].rt_data), fldtxt);
    for (i = 0; i < 60 && fldtxt[i]; ++i)
        tprintf("%c", fldtxt + i);
    if (fldtxt[i])
        tprintf(">");
    else
        tprintf("<");

    if (rev)
        tprintf("@r");
}

/* ========================================================================
    Display/edit record
*/
void disp_rec(DB_ADDR *dba)
{
    int size;

    if (dba != NULL)
        task->curr_rec = *dba;

    if (d_crtype(&rt, task, CURR_DB) != S_OKAY)
        return;
    rt -= RECMARK;
    if (rt < 0)
    {
        usererr("invalid database address");
        task->curr_rec = NULL_DBA;
        return;
    }
    if (task->record_table[rt].rt_fdtot <= 0)
    {
        /* system or fieldless record */
        tprintf("@m0400@E@SRECORD:@s %-16s", task->record_names[rt]);
        tprintf(" @SSIZE:@s %-4d", task->record_table[rt].rt_len);
        return;
    }
    size = task->record_table[rt].rt_len - task->record_table[rt].rt_data;
    /* allocate memory for record */
    if ((rec = malloc(size)) == NULL)
    {
        usererr("insufficient memory");
        return;
    }
    if (d_recread(rec, task, CURR_DB) == S_OKAY)
    {
        d_curkey(task, CURR_DB);
        show_rec(rt, rec);
    }
    free(rec);
}

/* ========================================================================
    Display next page of data fields
*/
void next_rec()
{
    register int fld;

    if (curpg == totpg)
    {
        usererr("last page");
        return;
    }
    ++curpg;
    tprintf("@m0400@E@SRECORD:@s %-16s @SSIZE:@s %-4d",
              task->record_names[rt], task->record_table[rt].rt_len);
    tprintf(" @STOTAL FIELDS:@s %-3d ", totflds);
    tprintf("@m0468@SPAGE@s %d", curpg);
    tprintf(" @SOF@s %d\n\n", totpg);
    topfld = (curpg - 1) * FPP;
    for (fld = topfld; fld < totflds && fld - topfld < FPP; ++fld)
    {
        pr_field(fld, NOREV);
    }
}

/* ========================================================================
    Show record for entry/modify
*/
void show_rec(
    int t,                              /* record type */
    char *r)                            /* pointer to record area */
{
    register int i;

    rt = t;
    rec = r;

    if (task->record_table[rt].rt_fdtot == -1)
    {
        /* system record */
        tprintf("@m0400@E@SRECORD:@s %-16s @SSIZE:@s %-4d",
                  task->record_names[rt], task->record_table[rt].rt_len);
        return;
    }
    /* allocate editable field list array */
    if (fldlist)
        free((char *) fldlist);
    fldlist = (int *) calloc(task->record_table[rt].rt_fdtot, sizeof(int));
    if (fldlist == NULL)
    {
        usererr("insufficient memory");
        return;
    }
    /* build editable field list */
    for (totflds = 0, i = task->record_table[rt].rt_fields;
          i < task->record_table[rt].rt_fields + task->record_table[rt].rt_fdtot;
          ++totflds)
    {
        fldlist[totflds] = i;
        if (task->field_table[i].fd_type == GROUPED)
        {
            /* skip subfields - they're not editable */
            for (++i; i < task->size_fd && (STRUCTFLD & task->field_table[i].fd_flags); ++i)
                ;
        }
        else
            ++i;
    }
    totpg = totflds / FPP + (totflds % FPP == 0 ? 0 : 1);
    curpg = 0;
    next_rec();
}

/* ========================================================================
    Edit displayed record
*/
void edit_rec()
{
    int fldp, fld, col, dir;
    char msg[81], *tp;

    for (fld = topfld; fld < totflds && fld - topfld < FPP;)
    {
        /* display field info */
        tprintf("@m0400@e@SFIELD:@s %-20s ", task->field_names[fldlist[fld]]);
        tprintf("@m0430 @STYPE:@s ");
        switch (task->field_table[fldlist[fld]].fd_key)
        {
            case UNIQUE:
                tprintf("unique key ");
                break;
            case DUPLICATES:
                tprintf("key ");
                break;
        }
        if (task->field_table[fldlist[fld]].fd_flags & UNSIGNEDFLD)
            tprintf("unsigned ");
        switch (task->field_table[fldlist[fld]].fd_type)
        {
            case CHARACTER:
                tprintf("char");
                break;
            case SHORTINT:
                tprintf("short");
                break;
            case REGINT:
                tprintf("int");
                break;
            case LONGINT:
                tprintf("long");
                break;
            case FLOAT:
                tprintf("float");
                break;
            case DOUBLE:
                tprintf("double");
                break;
            case DBADDR:
                tprintf("db_addr");
                break;
            case GROUPED:
                tprintf("struct");
                break;
        }
        tprintf("@m0464 @SSIZE:@s %d\n", task->field_table[fldlist[fld]].fd_len);

        /* process displayed fields */
        fldp = task->field_table[fldlist[fld]].fd_ptr - task->record_table[rt].rt_data;
        pr_field(fld, REV);
        if (creat_flag)
            fldtxt[0] = '\0';
        else
            fldtotxt(fldlist[fld], rec + fldp, fldtxt);
flded:
        if ((dir = ed_field(creat_flag)) != 0)
        {
            if ((tp = txttofld(fldlist[fld], fldtxt, rec + fldp)) != NULL)
            {
                /* input error */
                col = (int) (tp - fldtxt);
                sprintf(msg, "input error at pos %d: ", col);
                strncat(msg, tp, 24);
                usererr(msg);
                goto flded;
            }
            else
                pr_field(fld, NOREV);
        }
        else
        {
            pr_field(fld, NOREV);
            break;
        }
        fld += dir;
        if (fld < topfld)
            fld = topfld;
    }
}

/* ========================================================================
    Create record
*/
void create_rec()
{
    register int i, start, end, offset;

    if (task->record_table[rt].rt_fdtot <= 0)
    {
        /* system or fieldless record */
        return;
    }
    /* clear record area for displayed fields */
    offset = task->record_table[rt].rt_data;
    start = task->field_table[fldlist[topfld]].fd_ptr - offset;
    i = fldlist[totflds - topfld < FPP ? totflds - 1 : topfld + FPP - 1];
    end = task->field_table[i].fd_ptr + task->field_table[i].fd_len - offset;
    for (i = start; i < end; ++i)
        rec[i] = '\0';

    /* redisplay record contents */
    --curpg;
    next_rec();

    creat_flag = 1;
    edit_rec();
    creat_flag = 0;
}

/* ========================================================================
    Display previous page of fields
*/
void prev_rec()
{
    if (curpg <= 1)
    {
        usererr("first page");
        return;
    }
    curpg -= 2;
    next_rec();
}

/* ========================================================================
    Write entered/modified record to database
*/
void write_rec()
{
    int stat;
    char msg[81];
    register int i, set;

    if (task->record_table[rt].rt_fdtot == -1)
    {
        usererr("cannot write system record");
        return;
    }
    if (entry_flag)
    {
        if ((stat = d_fillnew(rt + RECMARK, rec, task, CURR_DB)) == S_OKAY)
            usererr("record entered");
        if (autoset)
        {
            /* connect record to each set of which it is a member */
            for (set = 0; set < task->size_st; ++set)
            {
                for (i = task->set_table[set].st_members;
                     i < task->set_table[set].st_members + task->set_table[set].st_memtot;
                     ++i)
                {
                    if (task->member_table[i].mt_record == rt)
                    {
                        if (d_connect(set + SETMARK, task, CURR_DB) == S_OKAY)
                        {
                            strcpy(msg, "record connected to set ");
                            strcat(msg, task->set_names[set]);
                            usererr(msg);
                        }
                        else
                        {
                            strcpy(msg, "record not connect to set ");
                            strcat(msg, task->set_names[set]);
                            usererr(msg);
                        }
                    }
                }
            }
        }
    }
    else
    {
        if ((stat = d_recwrite(rec, task, CURR_DB)) == S_OKAY)
            usererr("record modified");
    }
    if (stat == S_DUPLICATE)
        usererr("duplicate key");
}

/* ========================================================================
    Build optional key table
*/
static void bld_optkey_tab(int rec)
{
    int tot;
    register int i;

    for (tot = 0, i = task->record_table[rec].rt_fields;
          task->field_table[i].fd_rec == rec; ++i)
    {
        if (task->field_table[i].fd_flags & OPTKEYMASK)
            ++tot;
    }
    if (tot)
    {
        tot_optkeys = tot;
        if (optkeyflds)
        {
            free((char *) optkeyflds);
            free((char *) optkeynms);
        }
        optkeyflds = (int *) calloc(tot, sizeof(int));
        optkeynms = (char **) calloc(tot, sizeof(char *));
        if (!optkeyflds || !optkeynms)
        {
            tot_optkeys = 0;
            dberr(S_NOMEMORY);
            return;
        }
        for (tot = 0, i = task->record_table[rec].rt_fields;
             task->field_table[i].fd_rec == rec; ++i)
        {
            if (task->field_table[i].fd_flags & OPTKEYMASK)
            {
                optkeyflds[tot] = i;
                optkeynms[tot++] = task->field_names[i];
            }
        }
        lastoptrec = rec;
    }
}

/* ========================================================================
    Store optional key
*/
void store_key()
{
    long ckey;
    int rec;

    if (d_crtype(&rec, task, CURR_DB) == S_OKAY)
    {
        rec -= RECMARK;
        if (lastoptrec != rec)
        {
            bld_optkey_tab(rec);
        }
        if (tot_optkeys)
        {
            /* select optional key to be stored */
            tprintf("@m0300@E\nSELECT OPTIONAL KEY FIELD:\n");
            okey = list_selection(6, tot_optkeys, optkeynms, 1, okey, 1);
            tprintf("@m0300@E");
            if (okey >= 0)
            {
                ckey = (long) (FLDMARK * rec + optkeyflds[okey]
                               - task->record_table[rec].rt_fields);
                if (d_keystore(ckey, task, CURR_DB) == S_OKAY)
                    usererr("optional key stored");
            }
            /* Redisplay record */
            curpg = 0;
            next_rec();
        }
        else
            usererr("no optional keys defined");
    }
}

/* ========================================================================
    Delete optional key
*/
void del_key()
{
    long ckey;
    int rec;

    if (d_crtype(&rec, task, CURR_DB) == S_OKAY)
    {
        rec -= RECMARK;
        if (lastoptrec != rec)
        {
            bld_optkey_tab(rec);
        }
        if (tot_optkeys)
        {
            /* select optional key to be stored */
            tprintf("@m0300@E\nSELECT OPTIONAL KEY FIELD:\n");
            okey = list_selection(6, tot_optkeys, optkeynms, 1, okey, 1);
            tprintf("@m0300@E");
            if (okey >= 0)
            {
                ckey = (long) (FLDMARK * rec + optkeyflds[okey]
                               - task->record_table[rec].rt_fields);
                if (d_keydel(ckey, task, CURR_DB) == S_OKAY)
                    usererr("optional key deleted");
            }
            /* Redisplay record */
            curpg = 0;
            next_rec();
        }
        else
            usererr("no optional keys defined");
    }
}

/* ========================================================================
    Exit and clear screen function
*/
void clear_exit()
{
    tprintf("@c");
}


