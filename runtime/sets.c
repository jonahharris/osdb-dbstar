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
    Set current member to current member
*/
int INTERNAL_FCN dsetmm(
    int sett,                   /* set table entry number of target member */
    int sets,                   /* set table entry number of source member */
    DB_TASK *task,
    int dbn)                    /* database number */
{
    int cmtype;                 /* current member type */
    int mem, memtot;
    SET_ENTRY *set_ptr;
    int dbopen_sv;

    if (dcmtype(sets, &cmtype, task, dbn) == S_OKAY &&
        nset_check(sett, &sett, &set_ptr, task) == S_OKAY)
    {
        cmtype += task->curr_db_table->rt_offset - RECMARK;
        sets   += task->curr_db_table->st_offset - SETMARK;

        for ( mem = set_ptr->st_members, memtot = mem + set_ptr->st_memtot;
              mem < memtot; ++mem)
        {
            if (task->member_table[mem].mt_record == cmtype)
            {
                dbopen_sv = task->dbopen;
                task->dbopen = 2;                /* setup to allow unlocked read */
                r_smem(&task->curr_mem[sets], sett, task);
                task->dbopen = dbopen_sv;
                break;
            }
        }

        if (mem == memtot)
            dberr(S_INVMEM);
    }

    return (task->db_status);
}


/* ======================================================================
    Set current member to current owner
*/
int INTERNAL_FCN dsetmo(
    int setm,                   /* set table entry number of member */
    int seto,                   /* set table entry number of owner */
    DB_TASK *task,
    int dbn)                    /* database number */
{
    int mem, memtot;
    SET_ENTRY *setm_ptr, *seto_ptr;
    int dbopen_sv;

    if (nset_check(seto, &seto, &seto_ptr, task) == S_OKAY &&
        nset_check(setm, &setm, &setm_ptr, task) == S_OKAY)
    {
        if (null_dba(task->curr_own[seto]))
            dberr(S_NOCO);
        else
        {
            for (mem = setm_ptr->st_members, memtot = mem + setm_ptr->st_memtot;
                 mem < memtot; ++mem)
            {
                if (task->member_table[mem].mt_record == seto_ptr->st_own_rt)
                {
                    dbopen_sv = task->dbopen;
                    task->dbopen = 2;            /* setup to allow unlocked read */
                    r_smem(&task->curr_own[seto], setm, task);
                    task->dbopen = dbopen_sv;
                    break;
                }
            }

            if (mem == memtot)
                dberr(S_INVMEM);
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Set current member to current record
*/
int INTERNAL_FCN dsetmr(
    int set,                    /* set table entry number */
    DB_TASK *task,
    int dbn)                    /* database number */
{
    int crtype;                 /* current record type */
    int mem, memtot;
    SET_ENTRY *set_ptr;
    int dbopen_sv;

    if (!task->curr_rec)
        dberr(S_NOCR);
    else
    {
        if (dcrtype(&crtype, task, dbn) == S_OKAY &&
            nset_check(set, &set, &set_ptr, task) == S_OKAY)
        {
            crtype += DB_REF(rt_offset) - RECMARK;

            for (mem = set_ptr->st_members, memtot = mem + set_ptr->st_memtot;
                 mem < memtot; ++mem)
            {
                if (task->member_table[mem].mt_record != crtype)
                    continue;

                dbopen_sv = task->dbopen;
                task->dbopen = 2;                 /* setup to allow unlocked read */
                r_smem(&task->curr_rec, set, task);
                task->dbopen = dbopen_sv;
                break;
            }

            if (mem == memtot)
                dberr(S_INVMEM);
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Set current owner to current member
*/
int INTERNAL_FCN dsetom(
    int nseto,                  /* set table entry number of owner */
    int nsetm,                  /* set table entry number of member */
    DB_TASK *task,
    int dbn)                    /* database number */
{
    int seto, setm;
    int cmtype;                 /* current record type */
    SET_ENTRY *set_ptr;

    if (dcmtype(nsetm, &cmtype, task, dbn) != S_OKAY ||
        nset_check(nseto, &seto, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    cmtype += NUM2INT(-RECMARK, rt_offset);
    setm = NUM2INT(nsetm - SETMARK, st_offset);

    if (set_ptr->st_own_rt != cmtype)
        return (dberr(S_INVOWN));

    task->curr_own[seto] = task->curr_mem[setm];
    task->curr_mem[seto] = NULL_DBA;

    /* set timestamps */
    if (task->db_tsrecs)
    {
        task->co_time[seto] = task->cm_time[setm];
        task->cm_time[seto] = 0L;
    }

    if (task->db_tssets)
        dutscs(nseto, &task->cs_time[seto], task, dbn);

    return (task->db_status);
}


/* ======================================================================
    Set current owner to current owner
*/
int INTERNAL_FCN dsetoo(
    int nsett,                  /* set table entry number of target owner */
    int nsets,                  /* set table entry number of source owner */
    DB_TASK *task,
    int dbn)                    /* database number */
{
    int sett, sets;
    SET_ENTRY *sett_ptr, *sets_ptr;

    if (nset_check(nsett, &sett, &sett_ptr, task) != S_OKAY ||
        nset_check(nsets, &sets, &sets_ptr, task) != S_OKAY)
        return (task->db_status);

    if (sett_ptr->st_own_rt != sets_ptr->st_own_rt)
        return (dberr(S_INVOWN));

    if (null_dba(task->curr_own[sets]))
        return (dberr(S_NOCO));

    task->curr_own[sett] = task->curr_own[sets];
    task->curr_mem[sett] = NULL_DBA;

    /* set timestamps */
    if (task->db_tsrecs)
    {
        task->co_time[sett] = task->co_time[sets];
        task->cm_time[sett] = 0L;
    }

    if (task->db_tssets)
        dutscs(nsett, &task->cs_time[sett], task, dbn);

    return (task->db_status);
}


/* ======================================================================
    Set current owner to current record
*/
int INTERNAL_FCN dsetor(
    int nset,                   /* set number */
    DB_TASK *task,
    int dbn)                    /* database number */
{
    int set;
    int crtype;                 /* current record type */
    SET_ENTRY *set_ptr;

    if (nset_check(nset, &set, &set_ptr, task) != S_OKAY ||
        dcrtype(&crtype, task, dbn) != S_OKAY)
        return (task->db_status);

    crtype += NUM2INT(-RECMARK, rt_offset);

    if (set_ptr->st_own_rt != crtype)
        return (dberr(S_INVOWN));

    task->curr_own[set] = task->curr_rec;
    task->curr_mem[set] = NULL_DBA;

    /* set timestamps */
    if (task->db_tsrecs)
    {
        task->co_time[set] = task->cr_time;
        task->cm_time[set] = 0L;
    }

    if (task->db_tssets)
        dutscs(nset, &task->cs_time[set], task, dbn);

    return (task->db_status);
}


/* ======================================================================
    Set current record to current member
*/
int INTERNAL_FCN dsetrm(
    int set,                    /* set table entry number */
    DB_TASK *task,
    int dbn)                    /* database number */
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) == S_OKAY)
    {
        if (!task->curr_mem[set])
            dberr(S_NOCM);
        else
        {
            task->curr_rec = task->curr_mem[set];

            /* set timestamp */
            if (task->db_tsrecs)
                task->cr_time = task->cm_time[set];
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Set current record to current owner
*/
int INTERNAL_FCN dsetro(
    int set,                    /* set table entry number */
    DB_TASK *task,
    int dbn)                    /* database number */
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, &set_ptr, task) == S_OKAY)
    {
        if (!task->curr_own[set])
            dberr(S_NOCO);
        else
        {
            task->curr_rec = task->curr_own[set];

            /* set timestamp */
            if (task->db_tsrecs)
                task->cr_time = task->co_time[set];
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Find owner of current record
*/
int INTERNAL_FCN dfindco(int nset, DB_TASK *task, int dbn)
{
    int set;
    int stat;
    MEM_PTR mem;
    SET_ENTRY *set_ptr;

    if (nset_check(nset, &set, (SET_ENTRY **) &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* Make sure we have a current record */
    if (!task->curr_rec)
        return (dberr(S_NOCR));

    /* Read current record */
    if (dio_read(task->curr_rec, (char **) &task->crloc, NOPGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* Get the member ptr for this set */
    stat = r_gmem(set, task->crloc, &mem, task);

    if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
        return (task->db_status = stat);

    if (mem.owner == NULL_DBA)            /* Record not connected to set */
        return (dberr(S_NOTCON));

    /* set the new current owner and member */
    task->curr_own[set] = mem.owner;
    task->curr_mem[set] = task->curr_rec;
    task->curr_rec = mem.owner;

    /* set any timestamps */
    if (task->db_tsrecs)
    {
        dutscr(&task->cr_time, task, dbn);
        dutscm(nset, &task->cm_time[set], task, dbn);
        task->co_time[set] = task->cr_time;
    }

    if (task->db_tssets)
        dutscs(nset, &task->cs_time[set], task, dbn);

    return (task->db_status);
}


/* ======================================================================
    Find first member of set
*/
int INTERNAL_FCN dfindfm(int nset, DB_TASK *task, int dbn)
{
    SET_PTR setp;
    char *recp;
    int set;
    int stat;
    short crt;
    SET_ENTRY *set_ptr;

    if (nset_check(nset, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (!task->curr_own[set])
        return (dberr(S_NOCO));

    /* read current owner of set */
    if (dio_read(task->curr_own[set], (char **) &recp, NOPGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* Test whether current owner has been deleted */
    memcpy(&crt, recp, sizeof(short));

    if (crt < 0)
    {
        /* record is deleted */
        stat = S_DELETED;
    }
    else
    {
        /* get set pointer from record */
        stat = r_gset(set, recp, &setp, task);
    }

    if (dio_release(task->curr_own[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
        return (task->db_status = stat);

    /* set current record and member */
    task->curr_mem[set] = setp.first;

    if (!setp.first)                       /* end of set if no first member */
        return (task->db_status = S_EOS);

    task->curr_rec = setp.first;

    /* set timestamps */
    if (task->db_tsrecs)
    {
        dutscr(&task->cr_time, task, dbn);
        task->cm_time[set] = task->cr_time;
    }

    if (task->db_tssets)
    {
        /* only needed for system record support */
        dutscs(nset, &task->cs_time[set], task, dbn);
    }

    return (task->db_status);
}


/* ======================================================================
    Find last member of set
*/
int INTERNAL_FCN dfindlm(int nset, DB_TASK *task, int dbn)
{
    SET_PTR setp;
    char *recp;
    int set;
    int stat;
    short crt;
    SET_ENTRY *set_ptr;

    if (nset_check(nset, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (!task->curr_own[set])
        return (dberr(S_NOCO));

    /* read current owner of set */
    if (dio_read(task->curr_own[set], (char **) &recp, NOPGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* Test whether current owner has been deleted */
    memcpy(&crt, recp, sizeof(short));

    if (crt < 0)
    {
        /* record is deleted */
        stat = S_DELETED;
    }
    else
    {
        /* get set pointer from record */
        stat = r_gset(set, recp, &setp, task);
    }

    if (dio_release(task->curr_own[set], NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
        return (task->db_status = stat);

    /* set current record and member */
    task->curr_mem[set] = setp.last;

    if (!setp.last)                           /* end of set if no first member */
        return (task->db_status = S_EOS);

    task->curr_rec = setp.last;

    /* set timestamps */
    if (task->db_tsrecs)
    {
        dutscr(&task->cr_time, task, dbn);
        task->cm_time[set] = task->cr_time;
    }

    if (task->db_tssets)
    {
        /* only needed for system record support */
        dutscs(nset, &task->cs_time[set], task, dbn);
    }

    return (task->db_status);
}


/* ======================================================================
    Find next member of set
*/
int INTERNAL_FCN dfindnm(int nset, DB_TASK *task, int dbn)
{
    int set;
    short crt;
    MEM_PTR memp;
    int stat;
    char *recp;
    DB_ADDR *cm_ptr;
    DB_ADDR mem_dba;
    SET_ENTRY *set_ptr;

    if (nset_check(nset, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (!task->curr_own[set])
        return (dberr(S_NOCO));

    /* find first member if no current member */
    if (!*(cm_ptr = &task->curr_mem[set]))
        return (dfindfm(nset, task, dbn));

    mem_dba = *cm_ptr;

    /* read current member of set and get member pointer from record */
    if (dio_read(mem_dba, (char **) &recp, NOPGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* Test whether current member has been deleted */
    memcpy(&crt, recp, sizeof(short));

    if (crt < 0)
    {
        /* record is deleted */
        stat = S_DELETED;
    }
    else
    {
        stat = r_gmem(set, recp, &memp, task);
        if (memp.owner != task->curr_own[set])
        {
            /* set consistancy clash with another user between locks */
            *cm_ptr = NULL_DBA;
            stat = S_SETCLASH;
        }
    }

    if (dio_release(mem_dba, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
        return (task->db_status = stat);

    /* set current record and member */
    *cm_ptr = memp.next;

    if (!memp.next)                           /* end of set */
        return (task->db_status = S_EOS);

    task->curr_rec = memp.next;

    /* set timestamps */
    if (task->db_tsrecs)
    {
        dutscr(&task->cr_time, task, dbn);
        task->cm_time[set] = task->cr_time;
    }

    return (task->db_status);
}


/* ======================================================================
    Find previous member of set
*/
int INTERNAL_FCN dfindpm(int nset, DB_TASK *task, int dbn)
{
    int set;
    int stat;
    short crt;
    MEM_PTR memp;
    char *recp;
    DB_ADDR *cm_ptr;
    DB_ADDR mem_dba;
    SET_ENTRY *set_ptr;

    if (nset_check(nset, &set, &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (!task->curr_own[set])
        return (dberr(S_NOCO));

    /* find last member if no current member */
    if (!*(cm_ptr = &task->curr_mem[set]))
        return (dfindlm(nset, task, dbn));

    mem_dba = *cm_ptr;

    /* read current member of set and get member pointer from record */
    if (dio_read(mem_dba, (char **) &recp, NOPGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* Test whether current member has been deleted */
    memcpy(&crt, recp, sizeof(short));

    if (crt < 0)
    {
        /* record is deleted */
        stat = S_DELETED;
    }
    else
    {
        stat = r_gmem(set, recp, &memp, task);
        if (memp.owner != task->curr_own[set])
        {
            /* set consistancy clash with another user between locks */
            *cm_ptr = NULL_DBA;
            stat = S_SETCLASH;
        }
    }

    if (dio_release(mem_dba, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
        return (task->db_status = stat);

    /* set current record and member */
    *cm_ptr = memp.prev;

    if (!memp.prev)                           /* end of set */
        return (task->db_status = S_EOS);

    task->curr_rec = memp.prev;

    /* set timestamps */
    if (task->db_tsrecs)
    {
        dutscr(&task->cr_time, task, dbn);
        task->cm_time[set] = task->cr_time;
    }

    return (task->db_status);
}


