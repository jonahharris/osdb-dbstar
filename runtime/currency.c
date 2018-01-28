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
    Test timestamp status of current member
*/
int INTERNAL_FCN dcmstat(int set, DB_TASK *task, int dbn)
{
    DB_ULONG cts, dbuts;
    DB_ULONG cmts;

    if (dctscm(set, &cts, task, dbn) == S_OKAY)
    {
        if (cts)
        {
            cmts = task->cm_time[NUM2INT(set - SETMARK, st_offset)];
            if (cts > cmts)
                task->db_status = S_DELETED;
            else
            {
                dutscm(set, &dbuts, task, dbn);
                if (dbuts > cmts)
                    task->db_status = S_UPDATED;
            }
        }
        else
            dberr(S_TIMESTAMP);
    }
    return (task->db_status);
}


/* ======================================================================
    Get current member type
*/
int INTERNAL_FCN dcmtype(int set, int *cmtype, DB_TASK *task, int dbn)
{
    char *mrec;
    short crt;
    SET_ENTRY *set_ptr;
    int dbopen_sv;

    if (nset_check(set, &set, (SET_ENTRY **) &set_ptr, task) == S_OKAY)
    {
        if (!task->curr_mem[set])
            dberr(S_NOCM);
        else
        {
            /* set up to allow unlocked read */
            dbopen_sv = task->dbopen;
            task->dbopen = 2;

            /* Read current member */
            if (dio_read(task->curr_mem[set], (char **) &mrec,
                         NOPGHOLD, task) == S_OKAY)
            {
                /* Fetch record type from record header */
                memcpy(&crt, mrec, sizeof(short));
                if (dio_release(task->curr_mem[set], NOPGFREE, task) == S_OKAY)
                {
                    crt &= ~RLBMASK;                 /* mask off rlb */
                    *cmtype = (int) crt + RECMARK;
                }
            }

            task->dbopen = dbopen_sv;
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Test timestamp status of current owner
*/
int INTERNAL_FCN dcostat(int set, DB_TASK *task, int dbn)
{
    DB_ULONG cts, dbuts;
    DB_ULONG cots;

    if (dctsco(set, &cts, task, dbn) == S_OKAY)
    {
        if (cts)
        {
            cots = task->co_time[NUM2INT(set - SETMARK, st_offset)];
            if (cts > cots)
                task->db_status = S_DELETED;
            else
            {
                dutsco(set, &dbuts, task, dbn);
                if (dbuts > cots)
                    task->db_status = S_UPDATED;
            }
        }
        else
            dberr(S_TIMESTAMP);
    }

    return (task->db_status);
}


/* ======================================================================
    Get current owner type
*/
int INTERNAL_FCN dcotype(int set, int *cotype, DB_TASK *task, int dbn)
{
    char *orec;
    short crt;
    int dbopen_sv;
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, (SET_ENTRY **) &set_ptr, task) == S_OKAY)
    {
        if (!task->curr_own[set])
            dberr(S_NOCO);
        else
        {
            /* set up to allow unlocked read */
            dbopen_sv = task->dbopen;
            task->dbopen = 2;

            /* Read current owner */
            if (dio_read(task->curr_own[set], (char **) &orec,
                         NOPGHOLD, task) == S_OKAY)
            {
                /* Fetch record type from record header */
                memcpy(&crt, orec, sizeof(short));
                if (dio_release(task->curr_own[set], NOPGFREE, task) == S_OKAY)
                {
                    crt &= ~RLBMASK;                 /* mask off rlb */
                    *cotype = (int) crt + RECMARK;
                }
            }

            task->dbopen = dbopen_sv;
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Get current record
*/
int INTERNAL_FCN dcrget(DB_ADDR *dba, DB_TASK *task, int dbn)
{
    if ((*dba = task->curr_rec) == NULL_DBA)
        task->db_status = S_NOCR;

    return (task->db_status);
}


/* ======================================================================
    Read data from field  of current record
*/
int INTERNAL_FCN dcrread(
    long field,             /* Field constant */
    void *data,             /* Data area to contain field contents */
    DB_TASK *task,
    int dbn)
{
    int fld, rec, stat;
    int dbopen_sv;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    /* Make sure we have a current record */
    if (!task->curr_rec)
        return (dberr(S_NOCR));

    if (check_dba(task->curr_rec, task) != S_OKAY)
        return (task->db_status);

    /* set up to allow unlocked read */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* Read current record */
    dio_read(task->curr_rec, (char **) &task->crloc, NOPGHOLD, task);
    task->dbopen = dbopen_sv;
    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* Get data from record */
    stat = r_gfld(fld_ptr, task->crloc, data, task);

    if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    /* set timestamp */
    if (task->db_tsrecs && (stat == S_OKAY))
        dutscr(&task->cr_time, task, dbn);

    return stat;
}


/* ======================================================================
    Set current record
*/
int INTERNAL_FCN dcrset(DB_ADDR *dba, DB_TASK *task, int dbn)
{
    short crt;
    int dbopen_sv;

    if (dba == NULL || *dba == NULL_DBA)
    {
        task->curr_rec = NULL_DBA;
        return (task->db_status);
    }

    if (check_dba(*dba, task) != S_OKAY)
        return (task->db_status);

    task->curr_rec = *dba;
    /* check to make sure the record is not deleted */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;                         /* setup to allow unlocked read */
    dio_read(*dba, (char **) &task->crloc, NOPGHOLD, task);
    task->dbopen = dbopen_sv;
    if (task->db_status != S_OKAY)
        return (task->db_status);

    memcpy(&crt, task->crloc, sizeof(short));
    if (dio_release(*dba, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    /* non-negative record type means not deleted */
    if (crt < 0)
    {
        /* the record is deleted */
        task->curr_rec = NULL_DBA;
        return (task->db_status = S_DELETED);
    }

    return (task->db_status);
}


/* ======================================================================
    Test timestamp status of current record
*/
int INTERNAL_FCN dcrstat(DB_TASK *task, int dbn)
{
    DB_ULONG cts, dbuts;

    if (dctscr(&cts, task, dbn) == S_OKAY)
    {
        if (cts)
        {
            dutscr(&dbuts, task, dbn);
            if (cts > task->cr_time)
                task->db_status = S_DELETED;
            else if (dbuts > task->cr_time)
                task->db_status = S_UPDATED;
        }
        else
            dberr(S_TIMESTAMP);
    }

    return (task->db_status);
}


/* ======================================================================
    Get current record type
*/
int INTERNAL_FCN dcrtype(int *crtype, DB_TASK *task, int dbn)
{
    short crt;
    int dbopen_sv;

    if (!task->curr_rec)
        dberr(S_NOCR);
    else
    {
        dbopen_sv = task->dbopen; /* set up to allow unlocked read */
        task->dbopen = 2;

        /* Read current record */
        if (dio_read(task->curr_rec, (char **) &task->crloc, NOPGHOLD, task) == S_OKAY)
        {
            /* Fetch record type from record header */
            memcpy(&crt, task->crloc, sizeof(short));

            if (dio_release(task->curr_rec, NOPGFREE, task) == S_OKAY)
            {
                if (crt < 0)
                {
                    /* Current record is deleted */
                    task->curr_rec = NULL_DBA;
                    task->dbopen = dbopen_sv;
                    return (task->db_status = S_DELETED);
                }
                crt &= ~RLBMASK;                 /* mask off rlb */
                *crtype = (int) crt + RECMARK;
            }
        }
        task->dbopen = dbopen_sv;
    }

    return (task->db_status);
}


/* ======================================================================
    Write data to a field  in the current record
*/
int INTERNAL_FCN dcrwrite(
    long     field,          /* field constant */
    void    *data,           /* data area to contain field contents */
    DB_TASK *task,
    int      dbn)            /* database number */
{
    DB_ULONG timestamp;
    int rec, fld;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, (int *) &fld,
            (RECORD_ENTRY **) &rec_ptr,
            (FIELD_ENTRY **) &fld_ptr, task) != S_OKAY)
    {
        return (task->db_status);
    }

    /* compound keys cannot be updated directly */
    if (fld_ptr->fd_type == COMKEY)
        return (dberr(S_ISCOMKEY));

    /* field used in compound keys cannot be updated directly */
    if (fld_ptr->fd_flags & COMKEYED)
        return (dberr(S_COMKEY));

    /* Make sure we have a current record */
    if (!task->curr_rec)
    {
        return (dberr(S_NOCR));
    }
    else if (check_dba(task->curr_rec, task) != S_OKAY)
    {
        return (task->db_status);
    }

    /* Read current record */
    if (dio_read(task->curr_rec, (char **) &task->crloc, PGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* check out the field */
    if (r_chkfld((short)fld, fld_ptr, task->crloc, data, task) != S_OKAY)
    {
        dio_release(task->curr_rec, PGFREE, task);
        return (task->db_status);
    }

    /* put data into record and return */
    if (r_pfld((short)fld, fld_ptr, task->crloc, data, &task->curr_rec, task) != S_OKAY)
    {
        dio_release(task->curr_rec, PGFREE, task);
        return (task->db_status);
    }

    /* check for timestamp */
    if (rec_ptr->rt_flags & TIMESTAMPED)
    {
        timestamp = dio_pzgetts(rec_ptr->rt_file, task);
        memcpy(task->crloc + RECUPTIME, &timestamp, sizeof(long));
    }

    dio_write(task->curr_rec, PGFREE, task);

    if ((task->db_status == S_OKAY) && (rec_ptr->rt_flags & TIMESTAMPED))
        task->cr_time = timestamp;

    return (task->db_status);
}


/* ======================================================================
    Get current set member
*/
int INTERNAL_FCN dcsmget(
    int      set,          /* Set table entry */
    DB_ADDR *dba,          /* db address of record to become current */
    DB_TASK *task,
    int      dbn)          /* database number */
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    if ((*dba = task->curr_mem[set]) == NULL_DBA)
        task->db_status = S_NOCM;

    return (task->db_status);
}


/* ======================================================================
    Read data from field of current set member
*/
int INTERNAL_FCN dcsmread(
    int      nset,      /* Set constant */
    long     field,     /* Field constant */
    void    *data,      /* Data area to contain field contents */
    DB_TASK *task,
    int      dbn)
{
    int fld,
        rec,
        set;
    int dbopen_sv;
    char            *recp;
    SET_ENTRY       *set_ptr;
    RECORD_ENTRY    *rec_ptr;
    FIELD_ENTRY     *fld_ptr;

    if (nset_check(nset, &set, &set_ptr, task) != S_OKAY ||
        nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    /* Make sure we have a current member */
    if (!task->curr_mem[set])
        return (dberr(S_NOCM));

    if (check_dba(task->curr_mem[set], task) != S_OKAY)
        return (task->db_status);

    /* set up to allow unlocked read */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* Read current member */
    dio_read(task->curr_mem[set], (char **) &recp, NOPGHOLD, task);
    task->dbopen = dbopen_sv;
    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* Get data from record */
    r_gfld(fld_ptr, recp, data, task);
    if (dio_release(task->curr_mem[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    /* check and fetch timestamps */
    if (task->db_status == S_OKAY)
    {
        if (task->db_tsrecs)
            dutscr(&task->cm_time[set], task, dbn);

        if (task->db_tssets)
            dutscs(nset, &task->cs_time[set], task, dbn);
    }

    return (task->db_status);
}


/* ======================================================================
    Set current set member
*/
int INTERNAL_FCN dcsmset(
    int      set,            /* Set table entry */
    DB_ADDR *dba,            /* db address of record to become current */
    DB_TASK *task,
    int      dbn)            /* database number */
{
    short type;
    char *ptr;
    SET_ENTRY *set_ptr;
    MEMBER_ENTRY *mem_ptr;
    int mem, memtot;
    int dbopen_sv;

    if (nset_check(set, &set, (SET_ENTRY **) &set_ptr, task) != S_OKAY)
        return (task->db_status);

    if (dba == NULL || *dba == NULL_DBA)
        task->curr_mem[set] = NULL_DBA;
    else if (check_dba(*dba, task) == S_OKAY)
    {
        /* get the record type */
        /* check to make sure the record is not deleted */
        dbopen_sv = task->dbopen;
        task->dbopen = 2;                         /* setup to allow unlocked read */
        dio_read(*dba, (char **) &ptr, NOPGHOLD, task);
        task->dbopen = dbopen_sv;
        if (task->db_status != S_OKAY)            /* from dio_read() */
            return (task->db_status);

        memcpy(&type, ptr, sizeof(short));
        if (dio_release(*dba, NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        type &= ~RLBMASK;
        type += task->curr_db_table->rt_offset;

        for (mem = set_ptr->st_members, memtot = mem + set_ptr->st_memtot,
             mem_ptr = &task->member_table[mem]; mem < memtot; ++mem, ++mem_ptr)
        {
            if (mem_ptr->mt_record == type)
            {
                dbopen_sv = task->dbopen;
                task->dbopen = 2;                   /* setup to allow unlocked read */
                r_smem(dba, set, task);
                task->dbopen = dbopen_sv;
                return (task->db_status);
            }
        }

        dberr(S_INVMEM);
    }

    return (task->db_status);
}


/* ======================================================================
    Write data to a field in the current set member
*/
int INTERNAL_FCN dcsmwrite(
    int         set,       /* Set constant */
    long        field,     /* Field constant */
    const void *data,      /* Data area to contain field contents */
    DB_TASK    *task,
    int         dbn)       /* database number */
{
    DB_ULONG timestamp;
    int rec, fld;
    char *recp;
    SET_ENTRY *set_ptr;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;
    DB_ADDR *cm_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY || 
        nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    /* compound keys cannot be updated directly */
    if (fld_ptr->fd_type == COMKEY)
        return (dberr(S_ISCOMKEY));

    /* field used in compound keys cannot be updated directly */
    if (fld_ptr->fd_flags & COMKEYED)
        return (dberr(S_COMKEY));

    /* Make sure we have a current member */
    if (!*(cm_ptr = &task->curr_mem[set]))
        return (dberr(S_NOCM));

    if (check_dba(task->curr_mem[set], task) != S_OKAY)
        return (task->db_status);

    /* Read current member */
    if (dio_read(*cm_ptr, (char **) &recp, PGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* check out the field */
    if (r_chkfld((short)fld, fld_ptr, recp, data, task) != S_OKAY)
    {
        dio_release(*cm_ptr, PGFREE, task);
        return (task->db_status);
    }

    /* Put data into record */
    if (r_pfld((short)fld, fld_ptr, recp, data, cm_ptr, task) != S_OKAY)
    {
        dio_release(*cm_ptr, PGFREE, task);
        return (task->db_status);
    }

    /* check for timestamp */
    if (rec_ptr->rt_flags & TIMESTAMPED)
    {
        timestamp = dio_pzgetts(rec_ptr->rt_file, task);
        memcpy(recp + RECUPTIME, &timestamp, sizeof(long));
    }

    dio_write(*cm_ptr, PGFREE, task);
    if ((task->db_status == S_OKAY) && (rec_ptr->rt_flags & TIMESTAMPED))
        task->cm_time[set] = timestamp;

    return (task->db_status);
}


/* ======================================================================
    Get current set owner
*/
int INTERNAL_FCN dcsoget(
    int      set,          /* Set table entry */
    DB_ADDR *dba,          /* db address of record to become current */
    DB_TASK *task,
    int      dbn)          /* database number */
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, (SET_ENTRY **) &set_ptr, task) == S_OKAY)
    {
        if ((*dba = task->curr_own[set]) == NULL_DBA)
            task->db_status = S_NOCO;
    }

    return (task->db_status);
}


/* ======================================================================
    Read data from field  of current set owner
*/
int INTERNAL_FCN dcsoread(
    int      nset,       /* Set constant */
    long     field,      /* Field constant */
    void    *data,       /* Data area to contain field contents */
    DB_TASK *task,
    int      dbn)
{
    int fld,
        rec,
        set;
    int dbopen_sv;
    char            *recp;
    SET_ENTRY       *set_ptr;
    RECORD_ENTRY    *rec_ptr;
    FIELD_ENTRY     *fld_ptr;

    if (nset_check(nset, &set, &set_ptr, task) != S_OKAY ||
        nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    /* Make sure we have a current owner */
    if (!task->curr_own[set])
        return (dberr(S_NOCO));

    if (check_dba(task->curr_own[set], task) != S_OKAY)
        return (task->db_status);

    /* set up to allow unlocked read */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* Read current owner */
    dio_read(task->curr_own[set], (char **) &recp, NOPGHOLD, task);
    task->dbopen = dbopen_sv;
    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* Get data from record return */
    r_gfld(fld_ptr, recp, data, task);

    if (dio_release(task->curr_own[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    /* check and fetch timestamps */
    if (task->db_status == S_OKAY)
    {
        if (task->db_tsrecs)
            dutscr(&task->co_time[set], task, dbn);

        if (task->db_tssets)
            dutscs(nset, &task->cs_time[set], task, dbn);
    }

    return (task->db_status);
}


/* ======================================================================
    Set current set owner
*/
int INTERNAL_FCN dcsoset(
    int      set,          /* Set table entry */
    DB_ADDR *dba,          /* db address of record to become current */
    DB_TASK *task,
    int      dbn)          /* database number */
{
    short type;
    char *ptr;
    SET_ENTRY *set_ptr;
    int dbopen_sv;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    if (dba == NULL || *dba == NULL_DBA)
        task->curr_own[set] = task->curr_mem[set] = NULL_DBA;
    else if (check_dba(*dba, task) == S_OKAY)
    {
        /* get the record type */
        /* check to make sure the record is not deleted */
        dbopen_sv = task->dbopen;
        task->dbopen = 2;                         /* setup to allow unlocked read */
        dio_read(*dba, (char **) &ptr, NOPGHOLD, task);
        task->dbopen = dbopen_sv;
        if (task->db_status != S_OKAY)
            return (task->db_status);

        memcpy(&type, ptr, sizeof(short));
        if (dio_release(*dba, NOPGFREE, task) != S_OKAY)
            return (task->db_status);
        type &= ~RLBMASK;
        type += task->curr_db_table->rt_offset;

        if (set_ptr->st_own_rt != type)
            return (dberr(S_INVOWN));

        task->curr_own[set] = *dba;
        task->curr_mem[set] = NULL_DBA;
    }

    return (task->db_status);
}


/* ======================================================================
    Write data to a field in the current set owner
*/
int INTERNAL_FCN dcsowrite(
    int         set,       /* Set constant */
    long        field,     /* Field constant */
    const void *data,      /* Data area to contain field contents */
    DB_TASK    *task,
    int         dbn)       /* database number */
{
    DB_ULONG timestamp;
    int rec, fld;
    char *recp;
    SET_ENTRY *set_ptr;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;
    DB_ADDR *co_ptr;

    if (nset_check(set, &set, &set_ptr, task) != S_OKAY ||
        nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    /* compound keys cannot be updated directly */
    if (fld_ptr->fd_type == COMKEY)
        return (dberr(S_ISCOMKEY));

    /* field used in compound keys cannot be updated directly */
    if (fld_ptr->fd_flags & COMKEYED)
        return (dberr(S_COMKEY));

    /* Make sure we have a current owner */
    if (!*(co_ptr = &task->curr_own[set]))
        return (dberr(S_NOCO));

    if (check_dba(task->curr_own[set], task) != S_OKAY)
        return (task->db_status);

    /* Read current owner */
    if (dio_read(*co_ptr, (char **) &recp, PGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* check out the field */
    if (r_chkfld((short)fld, fld_ptr, recp, data, task) != S_OKAY)
    {
        dio_release(*co_ptr, PGFREE, task);
        return (task->db_status);
    }

    /* Put data into record */
    if (r_pfld((short)fld, fld_ptr, recp, data, co_ptr, task) != S_OKAY)
    {
        dio_release(*co_ptr, PGFREE, task);
        return (task->db_status);
    }
    /* check for timestamp */
    if (rec_ptr->rt_flags & TIMESTAMPED)
    {
        timestamp = dio_pzgetts(rec_ptr->rt_file, task);
        memcpy(recp + RECUPTIME, &timestamp, sizeof(long));
    }

    dio_write(*co_ptr, PGFREE, task);
    if ((task->db_status == S_OKAY) && (rec_ptr->rt_flags & TIMESTAMPED))
        task->co_time[set] = timestamp;

    return (task->db_status);
}


/* ======================================================================
    Test timestamp status of current set
*/
int INTERNAL_FCN dcsstat(int set, DB_TASK *task, int dbn)
{
    DB_ULONG dbuts;

    if (dutscs(set, &dbuts, task, dbn) == S_OKAY)
    {
        if (dbuts)
        {
            if (dbuts > task->cs_time[NUM2INT(set - SETMARK, st_offset)])
                task->db_status = S_UPDATED;
        }
        else
            dberr(S_TIMESTAMP);
    }

    return (task->db_status);
}


