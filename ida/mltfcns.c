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
    IDA - Multi-User Control Functions
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "ddlnms.h"
#include "ida.h"

/**************************** LOCAL FUNCTIONS *****************************/
static char *lock_type(void);

/**************************** LOCAL VARIABLES *****************************/
static char id_text[80];

static char *lock_list[] = {
    "READ LOCK", "WRITE LOCK", "EXCLUSIVE LOCK", "KEEP LOCK"
};
static char *ltypes[] = {"r", "w", "x", "k"};
static int last_lock = 0;              /* last lock type selected */
static int last_rec = 0;               /* last record type selected */
static int last_set = 0;               /* last set type selected */
static int last_key = 0;               /* last key type selected */

/* ========================================================================
    Select type of record/set lock
*/
static char *lock_type()
{
    tprintf("@m0400@ESELECT TYPE OF LOCK:");
    if ((last_lock = list_selection(6, 4, lock_list, 0, last_lock, 1)) >= 0)
        return (ltypes[last_lock]);

    return (NULL);
}

/* ========================================================================
    Lock record type
*/
void lrfcn()
{
    int rec;
    char *ltype;

    tprintf("@m0400@ESELECT RECORD TYPE TO BE LOCKED:");
    if ((last_rec = list_selection(6, task->size_rt, task->record_names, 0, last_rec, 1)) >= 0)
    {
        rec = last_rec + RECMARK;
        if ((ltype = lock_type()) != NULL)
        {
            if (d_reclock(rec, ltype, task, CURR_DB) == S_OKAY)
                usererr("Record locked");
            else if (task->db_status == S_UNAVAIL)
                usererr("database file currently unavailable\007");
        }
    }
    tprintf("@m0300@E");
}

/* ========================================================================
    Lock set type
*/
void lsfcn()
{
    int set;
    char *ltype;

    if (task->size_st)
    {
        tprintf("@m0400@ESELECT SET TYPE TO BE LOCKED:");
        if ((last_set = list_selection(6, task->size_st, task->set_names, 0, last_set, 1)) >= 0)
        {
            set = last_set + SETMARK;
            if ((ltype = lock_type()) != NULL)
            {
                if (d_setlock(set, ltype, task, CURR_DB) == S_OKAY)
                    usererr("Set locked");
                else if (task->db_status == S_UNAVAIL)
                    usererr("database file currently unavailable\007");
            }
        }
        tprintf("@m0300@E");
    }
    else
        usererr("no sets defined in database");
}

/* ========================================================================
    Lock key type
*/
void lkfcn()
{
    long fld;
    int key, rec;
    char *ltype;

    tprintf("@m0300@E\nSELECT KEY TYPE TO BE LOCKED:\n");
    if ((key = list_selection(6, tot_keys, keynames, 1, 0, 1)) >= 0)
    {
        key = keyfields[key];
        rec = task->field_table[key].fd_rec;
        fld = rec * FLDMARK + key - task->record_table[rec].rt_fields;
        if ((ltype = lock_type()) != NULL)
        {
            if (d_keylock(fld, ltype, task, CURR_DB) == S_OKAY)
                usererr("Key file locked");
            else if (task->db_status == S_UNAVAIL)
                usererr("database file currently unavailable\007");
        }
    }
    tprintf("@m0300@E");
}

/* ========================================================================
    Set record lock bit of current record
*/
void lcfcn()
{
    if (d_rlbset(task, CURR_DB) == S_LOCKED)
        usererr("current record's lock bit is already set");
}

/* ========================================================================
    Set lock request timeout value
*/
void ltfcn()
{
    char val[20];
    int secs;

    tprintf("@m0200@ECurrent timeout is %d seconds. Enter new value: ",
              task->db_timeout);
    if (rdtext(val) > 0)
    {
        secs = task->db_timeout;
        sscanf(val, "%d", &secs);
        d_timeout(secs, task);
    }
}

/* ========================================================================
    Free record lock
*/
void frfcn()
{
    int rec;

    tprintf("@m0400@ESELECT RECORD TYPE TO BE FREED:");
    if ((rec = list_selection(6, task->size_rt, task->record_names, 0, 0, 1)) >= 0)
    {
        rec += RECMARK;
        if (d_recfree(rec, task, CURR_DB) == S_OKAY)
            usererr("Record lock freed");
    }
    tprintf("@m0300@E");
}

/* ========================================================================
    Free set lock
*/
void fsfcn()
{
    int set;

    if (task->size_st)
    {
        tprintf("@m0400@ESELECT SET TYPE TO BE FREED:");
        if ((set = list_selection(6, task->size_st, task->set_names, 0, 0, 1)) >= 0)
        {
            set += SETMARK;
            if (d_setfree(set, task, CURR_DB) == S_OKAY)
                usererr("Set lock freed");
        }
        tprintf("@m0300@E");
    }
    else
        usererr("no sets defined in database");
}

/* ========================================================================
    Free key type
*/
void fkfcn()
{
    long fld;
    int key, rec;

    tprintf("@m0300@E\nSELECT KEY TYPE TO BE FREED:\n");
    if ((last_key = list_selection(6, tot_keys, keynames, 1, last_key, 1)) >= 0)
    {
        key = keyfields[last_key];
        rec = task->field_table[key].fd_rec;
        fld = rec * FLDMARK + key - task->record_table[rec].rt_fields;
        if (d_keyfree(fld, task, CURR_DB) == S_OKAY)
            usererr("Key file freed");
    }
    tprintf("@m0300@E");
}

/* ========================================================================
    Free all locks
*/
void fafcn()
{
    if (d_freeall(task) == S_OKAY)
        usererr("All set/record locks freed");
}

/* ========================================================================
    Clear current record's lock bit
*/
void fcfcn()
{
    d_rlbclr(task, CURR_DB);
}


/* ========================================================================
    Transaction begin
*/
void tbfcn()
{
    tprintf("@m0200enter transaction id: ");
    if (rdtext(id_text) > 0)
    {
        if (d_trbegin(id_text, task) == S_OKAY)
            usererr("Transaction started");
        else
            usererr("Transaction failed");
    }

}

/* ========================================================================
    Transaction end
*/
void tefcn()
{
    if (d_trend(task) == S_OKAY)
        usererr("Transaction ended");
}

/* ========================================================================
    Transaction abort
*/
void tafcn()
{
    if (d_trabort(task) == S_OKAY)
        usererr("Transaction aborted");
}


