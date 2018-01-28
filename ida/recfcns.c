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

/*--------------------------------------------------------------------------
    IDA - Record Manipulation Functions
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "ddlnms.h"
#include "ida_d.h"
#include "ida.h"

/************************** EXTERNAL FUNCTIONS ****************************/
extern char *fldtotxt();
extern char *txttofld();
extern char *txttokey();

/*************************** GLOBAL VARIABLES *****************************/
int entry_flag = 0;                    /* record entry flag */

/*************************** LOCAL  VARIABLES *****************************/
#define FIRST_FIND      0
#define LAST_FIND       1
#define RPP 16                         /* records per page */
static int key = -1;                   /* key field number */
static long keyc;                      /* key field constant */
static int pgtot;                      /* total keys on current page */
static int cur_page;                   /* current page number */
static int top_key;                    /* entry number of top key */
static char rectxt[255];               /* display value of record */
static char frstkey[255];              /* value of first search key */
static int frstrec;                    /* value of first record */
static DB_ADDR seldba[RPP];            /* selection page dba's */
static char *keytbl[RPP] = {NULL};     /* key page display table */
static char keynm[32];                 /* key field name */
static int rtype;                      /* record type */
static char *edrec = NULL;             /* ptr to record editing area */

/************************** FORWARD REFERENCES ****************************/
void rrsnfcn(void);
void rsnfcn(void);
void rectotxt(char *, char *);
static int txtcpy(int *, char *, char *);
void rkffcn(void);
void rklfcn(void);
static void rrflfcn(int);

/* ========================================================================
    Scan and view keys
*/
void rsfcn()
{
    char ckey[80];
    char *tp;
    register int i, kno;

    if (tot_keys == 0)
    {
        usererr("no key fields defined");
        return;
    }
    if (keytbl[0] == NULL)
    {                                   /* allocate table memory */
        for (i = 0; i < RPP; ++i)
        {
            if ((keytbl[i] = malloc(80)) == NULL)
            {
                usererr("cannot allocate display memory");
                keytbl[0] = NULL;
                return;
            }
        }
    }
    tprintf("@m0300@E\nSELECT KEY FIELD TO BE SCANNED:\n");
    key = list_selection(6, tot_keys, keynames, 1, key, 1);
    tprintf("@m0300@E");
    if (key == -1)
        return;
    strcpy(keynm, keynames[key]);
    kno = keyfields[key];
    rtype = task->field_table[kno].fd_rec;
    keyc = rtype * FLDMARK + kno - task->record_table[rtype].rt_fields;
    cur_page = 0;
    tprintf("@m0200enter key value: @e");
    switch (rdtext(ckey))
    {
        case -1:
            return;
        case 0:
            if (d_keyfrst(keyc, task, CURR_DB) == S_NOTFOUND)
                d_keynext(keyc, task, CURR_DB);
            frstkey[0] = '\0';
            break;
        default:
            if (task->field_table[kno].fd_type == 'k')
                tp = txttokey(kno, ckey, frstkey);
            else
                tp = txttofld(kno, ckey, frstkey);
            if (tp)
            {
                usererr("input error");
                return;
            }
            else
            {
                d_keyfind(keyc, frstkey, task, CURR_DB);
                if (task->db_status == S_NOTFOUND)
                {
                    d_keynext(keyc, task, CURR_DB);
                    usererr("record not found");
                }
            }
    }
    if (task->db_status != S_OKAY)
        return;
    rsnfcn();
    menu(RS_MENU);
}

/* ========================================================================
    Display next page of keys
*/
void rsnfcn()
{
    int dbopen_sv;
    DB_ADDR dba;
    char *rec;
    register int i;

    if (task->db_status == S_NOTFOUND)
    {
        usererr("no more keys");
        return;
    }
    ++cur_page;
    top_key = (cur_page - 1) * RPP + 1;
    for (pgtot = 0; pgtot < RPP && task->db_status == S_OKAY;
          d_keynext(keyc, task, CURR_DB), ++pgtot)
    {
        d_crget(&dba, task, CURR_DB);
        dbopen_sv = task->dbopen;
        task->dbopen = 2;
        dio_read(dba, &rec, NOPGHOLD , task);
        task->dbopen = dbopen_sv;
        if (task->db_status != S_OKAY)
            return;
        memcpy(&(seldba[pgtot]), &dba, DB_ADDR_SIZE);
        rectotxt(rec, rectxt);
        if (dio_release(dba, NOPGFREE , task) != S_OKAY)
            return;
        rectxt[70] = '\0';               /* truncate for display */
        strcpy(keytbl[pgtot], rectxt);
    }
    tprintf("@m0300@E\n@SRecord:@s %-16s  ", task->record_names[rtype]);
    tprintf("@SKey:@s %-16s                      ", keynm);
    tprintf("@SPage:@s %d\n\n", cur_page);
    tprintf("@SENTRY RECORD CONTENTS@s\n");
    for (i = 0; i < pgtot; ++i)
    {
        tprintf("%4d.", top_key + i);
        tprintf(" %s\n", keytbl[i]);
    }
}

/* ========================================================================
    Display first page of keys
*/
void rsffcn()
{
    cur_page = 0;
    d_keyfind(keyc, frstkey, task, CURR_DB);
    if (task->db_status == S_NOTFOUND)
        d_keynext(keyc, task, CURR_DB);
    rsnfcn();
}

/* ========================================================================
    Select record from key on current page
*/
void rssfcn()
{
    register int i;

    if ((i = list_selection(7, pgtot, keytbl, top_key, 0, 0)) != -1)
    {
        tprintf("@c");
        disp_rec(&(seldba[i]));
    }
}

/* ========================================================================
    Find record by key
*/
void rffcn()
{
    char ckey[80];
    char skey[255];
    char *tp;
    register int kno;

    if (tot_keys == 0)
    {
        usererr("no key fields defined");
        return;
    }
    tprintf("@m0300@E\nSELECT KEY FIELD:\n");
    key = list_selection(6, tot_keys, keynames, 1, key, 1);
    tprintf("@m0300@E");
    if (key == -1)
        return;
    kno = keyfields[key];
    rtype = task->field_table[kno].fd_rec;
    keyc = rtype * FLDMARK + kno - task->record_table[rtype].rt_fields;
    tprintf("@m0200enter key value: @e");
    switch (rdtext(ckey))
    {
        case -1:
            return;
        case 0:
            d_keyfrst(keyc, task, CURR_DB);
            break;
        default:
            if (task->field_table[kno].fd_type == 'k')
                tp = txttokey(kno, ckey, skey);
            else
                tp = txttofld(kno, ckey, skey);
            if (tp)
            {
                usererr("input error");
                return;
            }
            else
            {
                d_keyfind(keyc, skey, task, CURR_DB);
            }
    }
    if (task->db_status == S_NOTFOUND)
    {
        if (d_keynext(keyc, task, CURR_DB) == S_NOTFOUND)
        {
            usererr("no records found");
            return;
        }
        else
            usererr("record not found");
    }
    disp_rec(NULL);
}

/* ========================================================================
    Find next record by key
*/
void rnfcn()
{
    if (key >= 0 && d_keynext(keyc, task, CURR_DB) == S_OKAY)
        disp_rec(NULL);
    else
    {
        usererr("no more keys");
    }
}

/* ========================================================================
    Find previous record by key
*/
void rpfcn()
{
    if (key >= 0 && d_keyprev(keyc, task, CURR_DB) == S_OKAY)
        disp_rec(NULL);
    else
    {
        usererr("no more keys");
    }
}

/* ========================================================================
    Read record by database address
*/
void rgfcn()
{
    DB_ADDR dba;

    switch (rd_dba(&dba))
    {
        case -1:
            break;
        case 0:
            usererr("invalid database address");
            break;
        default:
            if (d_crset(&dba, task, CURR_DB) == S_OKAY)
                disp_rec(NULL);
    }
}

/* ========================================================================
    Enter records
*/
void refcn()
{
    int i, size;

    tprintf("@m0400@ESELECT RECORD TYPE TO BE ENTERED:");
    rtype = list_selection(6, task->size_rt, task->record_names, 0, rtype, 1);
    tprintf("@m0400@E");
    if (rtype == -1)
        return;

    size = task->record_table[rtype].rt_len - task->record_table[rtype].rt_data;
    /* allocate memory for record */
    if ((edrec = malloc(size)) == NULL)
    {
        usererr("insufficient memory");
        return;
    }
    /* clear record area */
    for (i = 0; i < size; ++i)
        edrec[i] = '\0';

    entry_flag = 1;

    show_rec(rtype, edrec);
    menu(REC_MENU);
    free(edrec);
}

/* ========================================================================
    Modify current record
*/
void rmfcn()
{
    int size;

    if (d_crtype(&rtype, task, CURR_DB) == S_OKAY)
    {
        rtype -= RECMARK;
        /* allocate memory for record */
        size = task->record_table[rtype].rt_len - task->record_table[rtype].rt_data;
        if ((edrec = malloc(size)) == NULL)
        {
            usererr("insufficient memory");
            return;
        }
        if (d_recread(edrec, task, CURR_DB) == S_OKAY)
        {
            entry_flag = 0;
            show_rec(rtype, edrec);
            menu(REC_MENU);
        }
        free(edrec);
    }
}

/* ========================================================================
    Delete current record
*/
void rdfcn()
{
    if (d_delete(task, CURR_DB) == S_OKAY)
        usererr("current record deleted");
}

/* ========================================================================
    Copy record text
*/
static int txtcpy(int *tp, char *txt, char *str)
{
    while (*tp < 255 && *str)
        txt[(*tp)++] = *str++;
    txt[*tp] = '\0';
    return (0);
}

/* ========================================================================
    Convert record contents to linear text
*/
void rectotxt(char *rec, char *txt)
{
    short rt;
    int fld, tp;
    extern char fldtxt[];


    memcpy(&rt, rec, sizeof(short));
    rt &= ~RLBMASK;                     /* mask off rlb */
    tp = 0;
    txtcpy(&tp, txt, "{");

    for (fld = task->record_table[rt].rt_fields;
         fld < task->record_table[rt].rt_fields + task->record_table[rt].rt_fdtot;
         ++fld)
    {
        if (!(task->field_table[fld].fd_flags & STRUCTFLD))
        {
            if (fld > task->record_table[rt].rt_fields)
                txtcpy(&tp, txt, ",");
            if (task->field_table[fld].fd_type == CHARACTER)
                txtcpy(&tp, txt, "\"");
            else if (task->field_table[fld].fd_dim[0])
                txtcpy(&tp, txt, "{");
            fldtotxt(fld, rec + task->field_table[fld].fd_ptr, fldtxt);
            txtcpy(&tp, txt, fldtxt);
            if (task->field_table[fld].fd_type == CHARACTER)
                txtcpy(&tp, txt, "\"");
            else if (task->field_table[fld].fd_dim[0])
                txtcpy(&tp, txt, "}");
        }
    }
    txtcpy(&tp, txt, "}");
}

/* ========================================================================
    Find first/last record by key
*/
static void rkflfcn(
    int flag)                           /* FIRST_FIND or LAST_FIND */
{
    register int kno;

    /* Make sure there are keys */
    if (tot_keys == 0)
    {
        usererr("no key fields defined");
        return;
    }

    /* present the user with the selection of keys */
    tprintf("@m0300@E\nSELECT KEY FIELD:\n");
    key = list_selection(6, tot_keys, keynames, 1, key, 1);
    tprintf("@m0300@E");
    if (key == -1)
        return;

    /* convert user reply to key field # */
    kno = keyfields[key];
    rtype = task->field_table[kno].fd_rec;
    keyc = rtype * FLDMARK + kno - task->record_table[rtype].rt_fields;

    /* Now go get the first/last record for this key type */
    if (flag == FIRST_FIND)
    {
        if (d_keyfrst(keyc, task, CURR_DB) == S_NOTFOUND)
        {
            usererr("no records found");
            return;
        }
    }
    else
    {
        if (d_keylast(keyc, task, CURR_DB) == S_NOTFOUND)
        {
            usererr("no records found");
            return;
        }
    }

    /* display the record */
    disp_rec(NULL);
    return;
}

/* ========================================================================
    Find first record by key
*/
void rkffcn()
{
    rkflfcn(FIRST_FIND);
}

/* ========================================================================
    Find last record by key
*/
void rklfcn()
{
    rkflfcn(LAST_FIND);
}

/* ========================================================================
    Read first/last record by database address
*/
static void rrflfcn(
    int flag)                           /* FIRST_FIND or LAST_FIND */
{
    /* Ask user for record selection */
    tprintf("@m0400@ESELECT RECORD TYPE:");
    rtype = list_selection(6, task->size_rt, task->record_names, 0, rtype, 1);
    tprintf("@m0400@E");
    if (rtype == -1)
        return;

    /* Now find the first/last record of this type */
    if (flag == FIRST_FIND)
    {
        if (d_recfrst(rtype + RECMARK, task, CURR_DB) == S_NOTFOUND)
        {
            usererr("no records found");
            return;
        }
    }
    else
    {
        if (d_reclast(rtype + RECMARK, task, CURR_DB) == S_NOTFOUND)
        {
            usererr("no records found");
            return;
        }
    }

    /* now display this record */
    disp_rec(NULL);
    return;
}

/* ========================================================================
    Read first record by database address
*/
void rrffcn()
{
    rrflfcn(FIRST_FIND);
}

/* ========================================================================
    Read last record by database address
*/
void rrlfcn()
{
    rrflfcn(LAST_FIND);
}

/* ========================================================================
    Find next record by database address
*/
void rrnfcn()
{
    if (d_recnext(task, CURR_DB) == S_OKAY)
        disp_rec(NULL);
    else
        usererr("no more records");
}

/* ========================================================================
    Find previous record by database address
*/
void rrpfcn()
{
    if (d_recprev(task, CURR_DB) == S_OKAY)
        disp_rec(NULL);
    else
        usererr("no more records");
}

/* ========================================================================
    Scan and view record by database address
*/
void rrsfcn()
{
    register int i;

    /* if the display table hasn't been allocated yet, do so now */
    if (keytbl[0] == NULL)
    {
        for (i = 0; i < RPP; i++)
        {
            if ((keytbl[i] = malloc(80)) == NULL)
            {
                usererr("cannot allocate display memory");
                keytbl[0] = NULL;
                return;
            }
        }
    }

    /* Ask user for record selection */
    tprintf("@m0400@ESELECT RECORD TYPE:");
    rtype = list_selection(6, task->size_rt, task->record_names, 0, rtype, 1);
    tprintf("@m0400@E");
    if (rtype == -1)
        return;

    /* Now go display the 1st page's worth of records */
    cur_page = 0;
    frstrec = rtype + RECMARK;
    if (d_recfrst(frstrec, task, CURR_DB) != S_OKAY)
    {
        usererr("no records found");
        return;
    }
    rrsnfcn();

    /* Now present the scan menu and get user response */
    menu(RRS_MENU);
    return;
}

/* ========================================================================
    Display next page of records
*/
void rrsnfcn()
{

    char *rec;                          /* points to record that was read */
    DB_ADDR dba;                        /* address of record to read */
    register int i;

    if (task->db_status != S_OKAY)
    {
        usererr("no more records");
        return;
    }
    /* take care of the page #, and the entry # */
    cur_page++;                         /* We're on next page */
    top_key = (cur_page - 1) * RPP + 1;

    /* read a pages worth of records, or until there are no more recs */
    for (pgtot = 0; pgtot < RPP && task->db_status == S_OKAY; d_recnext(task, CURR_DB), ++pgtot)
    {

        /* get the dba, read this record, then convert to ASCII */
        d_crget(&dba, task, CURR_DB);
        dio_read(dba, &rec, NOPGHOLD , task);
        if (task->db_status != S_OKAY)
            return;
        memcpy(&(seldba[pgtot]), &dba, DB_ADDR_SIZE);    /* save for select */
        rectotxt(rec, rectxt);           /* convert record to display text */
        rectxt[70] = '\0';               /* truncate for display */
        if (dio_release(dba, NOPGFREE , task) != S_OKAY)
            return;
        strcpy(keytbl[pgtot], rectxt);       /* move to display table */
    }
    /* now display these records */
    tprintf("@m0300@E\n@SRECORD:@s %-16s  ", task->record_names[rtype]);
    tprintf("                             @SPage:@s %d\n\n", cur_page);
    tprintf("@SRECORD CONTENTS:@s\n");
    for (i = 0; i < pgtot; i++)
    {
        tprintf("%4d.", top_key + i);
        tprintf(" %s\n", keytbl[i]);
    }
}

/* ========================================================================
    Read first page of records
*/
void rrsffcn()
{
    /* Read first page and display */
    cur_page = 0;
    if (d_recfrst(frstrec, task, CURR_DB) != S_OKAY)
    {
        usererr("no records found");
        return;
    }
    rrsnfcn();
}

/* ========================================================================
    Select record from page and display
*/
void rrssfcn()
{
    register int i;

    if ((i = list_selection(7, pgtot, keytbl, top_key, 0, 0)) != -1)
    {
        tprintf("@c");
        disp_rec(&(seldba[i]));
    }
}


