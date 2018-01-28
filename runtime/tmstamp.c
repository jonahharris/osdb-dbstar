/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database kernel                                             *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

#include "db.star.h"

/* ======================================================================
    Get creation timestamp of current member
*/
int INTERNAL_FCN dctscm(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int dbopen_sv;
    short rec;
    char *rptr;
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current member */
    if (!task->curr_mem[set])
        return (dberr(S_NOCM));

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* read current member */
    dio_read(task->curr_mem[set], &rptr, NOPGHOLD, task);
    task->dbopen = dbopen_sv;
    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* get record id */
    memcpy(&rec, rptr, sizeof(short));
    if (rec >= 0)
    {
        rec &= ~RLBMASK;                 /* mask off rlb */
        if (task->record_table[NUM2INT(rec, rt_offset)].rt_flags & TIMESTAMPED)
            memcpy(timestamp, rptr + RECCRTIME, sizeof(DB_ULONG));
        else
            *timestamp = 0L;
    }

    if (dio_release(task->curr_mem[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (rec < 0)
        task->db_status = S_DELETED;

    return (task->db_status);
}


/* ======================================================================
    Get creation timestamp of current owner
*/
int INTERNAL_FCN dctsco(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int dbopen_sv;
    short rec;
    char *rptr;
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (!task->curr_own[set])
        return (dberr(S_NOCO));

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* read current owner */
    dio_read(task->curr_own[set], (char **) &rptr, NOPGHOLD, task);
    task->dbopen = dbopen_sv;
    if (task->db_status != S_OKAY)
        return (task->db_status);
    
    /* get record id */
    memcpy(&rec, rptr, sizeof(short));
    if (rec >= 0)
    {
        rec &= ~RLBMASK;                 /* mask off rlb */
        if (task->record_table[NUM2INT(rec, rt_offset)].rt_flags & TIMESTAMPED)
            memcpy(timestamp, rptr + RECCRTIME, sizeof(DB_ULONG));
        else
            *timestamp = 0L;
    }

    if (dio_release(task->curr_own[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (rec < 0)
        task->db_status = S_DELETED;

    return (task->db_status);
}


/* ======================================================================
    Get creation timestamp of current record
*/
int INTERNAL_FCN dctscr(DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int dbopen_sv;
    short rec;

    /* make sure we have a current record */
    if (!task->curr_rec)
        return (dberr(S_NOCR));

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* read current record */
    dio_read(task->curr_rec, &task->crloc, NOPGHOLD, task);
    task->dbopen = dbopen_sv;
    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* get record id */
    memcpy(&rec, task->crloc, sizeof(short));
    if (rec >= 0)
    {
        rec &= ~RLBMASK;                 /* mask off rlb */
        if (task->record_table[NUM2INT(rec, rt_offset)].rt_flags & TIMESTAMPED)
            memcpy(timestamp, task->crloc + RECCRTIME, sizeof(DB_ULONG));
        else
            *timestamp = 0L;
    }

    if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (rec < 0)
        task->db_status = S_DELETED;

    return (task->db_status);
}


/* ======================================================================
    Get timestamp of current member
*/
int INTERNAL_FCN dgtscm(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current member */
    if (task->curr_mem[set])
    {
        if (task->db_tsrecs)
            *timestamp = task->cm_time[set];
        else
            dberr(S_TIMESTAMP);
    }
    else
        dberr(S_NOCM);

    return (task->db_status);
}


/* ======================================================================
    Get timestamp of current owner
*/
int INTERNAL_FCN dgtsco(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (task->curr_own[set])
    {
        if (task->db_tsrecs)
            *timestamp = task->co_time[set];
        else
            dberr(S_TIMESTAMP);
    }
    else
        dberr(S_NOCO);

    return (task->db_status);
}


/* ======================================================================
    Get timestamp of current record
*/
int INTERNAL_FCN dgtscr(DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    /* make sure we have a current record */
    if (task->curr_rec)
    {
        if (task->db_tsrecs)
            *timestamp = task->cr_time;
        else
            dberr(S_TIMESTAMP);
    }
    else
        dberr(S_NOCR);

    return (task->db_status);
}


/* ======================================================================
    Get timestamp of set
*/
int INTERNAL_FCN dgtscs(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (task->curr_own[set])
    {
        if (task->db_tssets)
            *timestamp = task->cs_time[set];
        else
            dberr(S_TIMESTAMP);
    }
    else
        dberr(S_NOCO);

    return (task->db_status);
}


/* ======================================================================
    Set timestamp of current member
*/
int INTERNAL_FCN dstscm(int set, DB_ULONG timestamp, DB_TASK *task, int dbn)
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current member */
    if (task->curr_mem[set])
    {
        if (task->db_tsrecs)
            task->cm_time[set] = timestamp;
        else
            dberr(S_TIMESTAMP);
    }
    else
        dberr(S_NOCM);

    return (task->db_status);
}


/* ======================================================================
    Set timestamp of current owner
*/
int INTERNAL_FCN dstsco(int set, DB_ULONG timestamp, DB_TASK *task, int dbn)
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (task->curr_own[set])
    {
        if (task->db_tsrecs)
            task->co_time[set] = timestamp;
        else
            dberr(S_TIMESTAMP);
    }
    else
        dberr(S_NOCO);

    return (task->db_status);
}


/* ======================================================================
    Set timestamp of current record
*/
int INTERNAL_FCN dstscr(DB_ULONG timestamp, DB_TASK *task, int dbn)
{
    /* make sure we have a current record */
    if (task->curr_rec)
    {
        if (task->db_tsrecs)
            task->cr_time = timestamp;
        else
            dberr(S_TIMESTAMP);
    }
    else
        dberr(S_NOCR);

    return (task->db_status);
}


/* ======================================================================
    Set timestamp of set
*/
int INTERNAL_FCN dstscs(int set, DB_ULONG timestamp, DB_TASK *task, int dbn)
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (task->curr_own[set])
    {
        if (task->db_tssets)
            task->cs_time[set] = timestamp;
        else
            dberr(S_TIMESTAMP);
    }
    else
        dberr(S_NOCO);

    return (task->db_status);
}


/* ======================================================================
    Get update timestamp of current member
*/
int INTERNAL_FCN dutscm(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int dbopen_sv;
    short rec;
    char *rptr;
    SET_ENTRY *set_ptr;
    int stat = S_OKAY;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current member */
    if (!task->curr_mem[set])
        return (dberr(S_NOCM));

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;
    
    /* read current member */
    dio_read(task->curr_mem[set], &rptr, NOPGHOLD, task);
    task->dbopen = dbopen_sv;

    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* get record id */
    memcpy(&rec, rptr, sizeof(short));
    if (rec >= 0)
    {
        rec &= ~RLBMASK;                 /* mask off rlb */
        rec += task->curr_db_table->rt_offset;
        if (task->record_table[rec].rt_flags & TIMESTAMPED)
            memcpy(timestamp, rptr + RECUPTIME, sizeof(DB_ULONG));
        else
            *timestamp = 0L;
    }
    else
        stat = S_DELETED;

    if (dio_release(task->curr_mem[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
        task->db_status = stat;

    return (task->db_status);
}


/* ======================================================================
    Get update timestamp of current owner
*/
int INTERNAL_FCN dutsco(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int dbopen_sv;
    short rec;
    char *rptr;
    SET_ENTRY *set_ptr;
    int stat = S_OKAY;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (!task->curr_own[set])
        return (dberr(S_NOCO));

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* read current owner */
    dio_read(task->curr_own[set], &rptr, NOPGHOLD, task);
    task->dbopen = dbopen_sv;

    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* get record id */
    memcpy(&rec, rptr, sizeof(short));
    if (rec >= 0)
    {
        rec &= ~RLBMASK;                 /* mask off rlb */
        rec += task->curr_db_table->rt_offset;
        if (task->record_table[rec].rt_flags & TIMESTAMPED)
            memcpy(timestamp, rptr + RECUPTIME, sizeof(DB_ULONG));
        else
            *timestamp = 0L;
    }
    else
        stat = S_DELETED;

    if (dio_release(task->curr_own[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
        task->db_status = stat;

    return (task->db_status);
}


/* ======================================================================
    Get update timestamp of current record
*/
int INTERNAL_FCN dutscr(DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    short rec;
    int dbopen_sv;
    int stat = S_OKAY;

    /* make sure we have a current record */
    if (!task->curr_rec)
        return (dberr(S_NOCR));

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* read current record */
    dio_read(task->curr_rec, &task->crloc, NOPGHOLD, task);
    task->dbopen = dbopen_sv;
    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* get record id */
    memcpy(&rec, task->crloc, sizeof(short));
    if (rec >= 0)
    {
        rec &= ~RLBMASK;                 /* mask off rlb */
        rec += task->curr_db_table->rt_offset;
        if (task->record_table[rec].rt_flags & TIMESTAMPED)
            memcpy(timestamp, task->crloc + RECUPTIME, sizeof(DB_ULONG));
        else
            *timestamp = 0L;
    }
    else
        stat = S_DELETED;

    if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
       task->db_status = stat;

    return (task->db_status);
}


/* ======================================================================
    Get update timestamp of set
*/
int INTERNAL_FCN dutscs(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat = S_OKAY;
    int dbopen_sv;
    short rec;
    char *rptr;
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (!task->curr_own[set])
        return (dberr(S_NOCO));

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;
 
    /* read current owner */
    dio_read(task->curr_own[set], &rptr, NOPGHOLD, task);
    task->dbopen = dbopen_sv;

    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* get record id to ensure record not deleted */
    memcpy(&rec, rptr, sizeof(short));
    if (rec >= 0)
    {
        if (set_ptr->st_flags & TIMESTAMPED)
            memcpy(timestamp, rptr + set_ptr->st_own_ptr + SP_UTIME, sizeof(DB_ULONG));
        else
            *timestamp = 0L;
    }
    else
        stat = S_DELETED;

    if (dio_release(task->curr_own[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
        task->db_status = stat;

    return (task->db_status);
}


