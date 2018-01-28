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

    exfcns.c - Import specification execution functions

    This file contains functions the perform the actual import work under
    the direction of the dbexec() function.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "impdef.h"
#include "impvar.h"

/* ********************** TYPE DEFINITIONS *************************** */
#define VALLEN 240
struct ref
{
    int r_recndx;
    DB_TCHAR r_val[VALLEN];
};

/* ********************** LOCAL VARIABLE DECLARATIONS **************** */
static char *ralloc(int);

/* execute one record specification */
int rec_exec(struct rec *rptr)
{
    int st, i;

    /* under which conditions are this record to be created? */
    switch (rptr->rec_htype)
    {

            /* create regardless */
        case DBIMP_AUTO:
            if (rec_create(rptr->rec_ndx, rptr->rec_fldptr) == FAILURE)
                return (FAILURE);
            break;

            /* create if it has not yet been created */
        case DBIMP_CREATE:
            if ((st = rec_find(rptr->rec_ndx, rptr->rec_hptr)) == NOTFOUND)
            {
                if (rec_create(rptr->rec_ndx, rptr->rec_fldptr) == FAILURE)
                    return (FAILURE);
                if (rec_ref(rptr->rec_ndx, rptr->rec_hptr) == FAILURE)
                    return (FAILURE);
            }
            else if (st == SKIP)
                break;
            else if (st == FAILURE)
                return (FAILURE);
            break;

            /* update the field contents only, create if it is not found */
        case DBIMP_UPDATE:
            if ((st = rec_find(rptr->rec_ndx, rptr->rec_hptr)) == FOUND)
            {
                if (rec_imp(rptr->rec_ndx, rptr->rec_fldptr) == FAILURE)
                    return (FAILURE);
            }
            else if (st == NOTFOUND)
            {
                if (rec_create(rptr->rec_ndx, rptr->rec_fldptr) == FAILURE)
                    return (FAILURE);
                if (rec_ref(rptr->rec_ndx, rptr->rec_hptr) == FAILURE)
                    return (FAILURE);
            }
            else if (st == SKIP)
                break;
            else
                return (FAILURE);
            break;

            /* find the record only */
        case DBIMP_FIND:
            if ((st = rec_find(rptr->rec_ndx, rptr->rec_hptr)) == NOTFOUND)
            {
                /* nullify the current of record type */
                imp_g.currecs[rptr->rec_ndx] = DBA_NONE;
            }
            else if (st == SKIP)
                break;
            else if (st == FAILURE)
                return (FAILURE);

            break;
    }

    for (i = imp_g.rbs[rptr->rec_ndx]; i < imp_g.rbe[rptr->rec_ndx]; i++)
        imp_g.bs[imp_g.rmt[i]] = rptr->rec_ndx;

    return (OK);
}


/* execute a set connection specification */
int con_exec(struct con *cptr)
{
    int otype, mtype;
    int system = FALSE;      /* true if the owner record type is system record */
    DB_TASK *task = imp_g.dbtask;

    /* select the record types involved in the connection */
    otype = task->set_table[cptr->con_ndx].st_own_rt;

    mtype = imp_g.bs[cptr->con_ndx];

    if ((long) imp_g.currecs[otype] == SKIP_DBA)
        return (OK);

    /* the owner record must be the current record of its type */
    if (imp_g.currecs[otype] == DBA_NONE)
    {
        /* maybe the owner is the system record */
        if (task->record_table[otype].rt_fdtot == -1)
        {
            /* the owner is the system record, let it go */
            system = TRUE;
        }
        else
        {
            if (!imp_g.silent)
                vftprintf(stderr,
                    DB_TEXT("**WARNING** set owner (type %s) has no current record\n"),
                    task->set_names[cptr->con_ndx]);
            return (OK);
        }
    }

    if ((long) imp_g.currecs[mtype] == SKIP_DBA)
        return (OK);

    /* the member record must be the current record of its type */
    if (imp_g.currecs[mtype] == DBA_NONE)
    {
        if (!imp_g.silent)
            vftprintf(stderr,
                DB_TEXT("**WARNING** set member (type %s) has no current record\n"),
                task->set_names[cptr->con_ndx]);
        return (OK);
    }

    /* make the future owner the current owner of the set type */
    if (!system)
    {
        if (d_csoset(SETMARK + cptr->con_ndx, imp_g.currecs + otype, task, CURR_DB) != S_OKAY)
        {
            vftprintf(stderr,
                DB_TEXT("**ERROR** %d:  unable to set owner of set\n"),
                task->db_status);
            dbimp_abort(DB_TEXT("Execution terminated"));
            return (FAILURE);
        }
    }

    /* make the future member the current record */
    if (d_crset(imp_g.currecs + mtype, task, CURR_DB) != S_OKAY)
    {
        vftprintf(stderr,
            DB_TEXT("**ERROR** %d:  unable to set member of set\n"),
            task->db_status);
        dbimp_abort(DB_TEXT("Execution terminated"));
        return (FAILURE);
    }

    /* finally, the actual set connection */
    if (d_connect(SETMARK + cptr->con_ndx, task, CURR_DB) != S_OKAY)
    {
        if (task->db_status == S_ISOWNED)
        {
            /* We should assume that no new member record has been created in
             * this iteration of the FOREACH loop. */
            task->db_status = S_OKAY;
        }
        else
        {
            vftprintf(stderr, DB_TEXT("Unsuccessful connect, %d\n"), task->db_status);
            dbimp_abort(DB_TEXT("Execution terminated"));
            return (FAILURE);
        }
    }
    return (OK);
}

int rec_create(int ndx, struct fld *fptr)
{
    char *record;
    DB_ADDR dba;
    DB_TASK *task = imp_g.dbtask;

    /* reserve space for this record */
    if ((record = ralloc(ndx)) == NULL)
        return (FAILURE);

    /* fill the record with all data from the current line */
    if (fld_move(fptr, record) == FAILURE)
    {
        if (!imp_g.silent)
            vftprintf(stderr,
                DB_TEXT("**WARNING** This record (type %s) was not created\n"),
                task->record_names[ndx]);
        psp_freeMemory(record, 0);
        return (FAILURE);
    }

    /* create the record */
    if (d_fillnew(RECMARK + ndx, record, task, CURR_DB) != S_OKAY)
    {
        vftprintf(stderr,
            DB_TEXT("**ERROR** unable to create record, task->db_status=%d\n"),
            task->db_status);
        vftprintf(stderr,
            DB_TEXT("            record type %s\n"), task->record_names[ndx]);
        dbimp_abort(DB_TEXT("Execution terminated"));
        return (FAILURE);
    }

    psp_freeMemory(record, 0);

    /* make this record current */
    d_crget(&dba, task, CURR_DB);
    imp_g.currecs[ndx] = dba;
    return (OK);
}

int rec_ref(int ndx, struct handling *hptr)
{
    DB_TCHAR *val;
    struct ref r;
    DB_TASK *task = imp_g.dbtask;

    if ((val = asm_val(hptr)) == NULL)
        return (FAILURE);

    r.r_recndx = ndx;
    memset(r.r_val, 0, VALLEN * sizeof(DB_TCHAR));
    vtstrncpy(r.r_val, val, VALLEN);

    if (key_insert(imp_g.keyfld, (char *) &r, imp_g.currecs[ndx] , task) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Unable to create key in working database, %d\n"),
            task->db_status);
        imp_g.abort_flag = 1;
        return (FAILURE);
    }

    return (OK);
}


int rec_find(int ndx, struct handling *hptr)
{
    DB_TCHAR *val;
    struct ref r;
    DB_ADDR dba;
    DB_TASK *task = imp_g.dbtask;

    if ((val = asm_val(hptr)) == NULL)
        return (FAILURE);

    if (vtstrcmp(val, DB_TEXT("NULL")) == 0)
    {
        imp_g.currecs[ndx] = (DB_ADDR) SKIP_DBA;
        return (SKIP);
    }

    r.r_recndx = ndx;
    memset(r.r_val, 0, VALLEN * sizeof(DB_TCHAR));
    vtstrncpy(r.r_val, val, VALLEN);

    if (key_init(imp_g.keyfld , task) != S_OKAY)
    {
        vftprintf(stderr, DB_TEXT("Internal error %d: key_init\n"), task->db_status);
        imp_g.abort_flag = 1;
        return (FAILURE);
    }
    dba = NULL_DBA;
    if (key_locpos((char *) &r, &dba , task) != S_OKAY)
    {
        if (task->db_status == S_NOTFOUND)
            return (NOTFOUND);
        else
        {
            vftprintf(stderr, DB_TEXT("Internal error %d: key_locpos\n"), task->db_status);
            imp_g.abort_flag = 1;
            return (FAILURE);
        }
    }

    imp_g.currecs[ndx] = dba;
    return (FOUND);
}


int rec_imp(int ndx, struct fld *fptr)
{
    char *record;

    if ((record = ralloc(ndx)) == NULL)
        return (FAILURE);

    /* pull out the current contents of the record */
    d_crset(&(imp_g.currecs[ndx]), imp_g.dbtask, CURR_DB);
    d_recread(record, imp_g.dbtask, CURR_DB);

    /* fill in any new/changed field values */
    if (fld_move(fptr, record) == FAILURE)
    {
        psp_freeMemory(record, 0);
        return (FAILURE);
    }

    /* write the record back */
    d_recwrite(record, imp_g.dbtask, CURR_DB);

    psp_freeMemory(record, 0);
    return (OK);
}


static char *ralloc(int ndx)
{
    char *c;
    DB_TASK *task = imp_g.dbtask;

    c = psp_cGetMemory(task->record_table[ndx].rt_len - task->record_table[ndx].rt_data + 1, 0);
    if (c == NULL)
    {
        dbimp_abort(DB_TEXT("exfcns: Out of memory!"));
        imp_g.abort_flag = 1;
    }
    return (c);
}


