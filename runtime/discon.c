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
    Disconnect the current member of set
*/
int INTERNAL_FCN ddiscon(
    int nset,                 /* set number */
    DB_TASK *task,
    int dbn)
{
    SET_PTR    cosp;          /* current owner's set pointer */
    MEM_PTR    cmmp;          /* member's member pointer */
    MEM_PTR    npmp;          /* next or previous member's member pointer */
    char      *orec = NULL;   /* ptr to current owner record contents in cache */
    char      *mrec = NULL;   /* ptr to member record contents in cache */
    char      *nprec = NULL;  /* ptr to next or prev record contents in cache */
    DB_ADDR    mdba;          /* db address of member record */
    DB_ADDR    npdba = 0;     /* db address of next or previous member */
    int        set;           /* task->set_table entry */
    int        stat;          /* status code variable */
    short      file;          /* file containing owner record */
    DB_ULONG   slot;
    SET_ENTRY *set_ptr;
    DB_ADDR   *co_ptr;
    DB_ADDR   *cm_ptr;

    if (nset_check(nset, &set, (SET_ENTRY **) &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current owner */
    if (!*(co_ptr = &task->curr_own[set]))
        return (dberr(S_NOCO));

    /* make sure we have a current member */
    if (!*(cm_ptr = &task->curr_mem[set]))
        return (dberr(S_NOCM));

    /* read member record */
    mdba = *cm_ptr;
    if (dio_read(mdba, (char **) &mrec, PGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* ensure record is connected */
    if ((stat = r_gmem(set, mrec, &cmmp, task)) != S_OKAY)
        goto quit;

    if (cmmp.owner == NULL_DBA)
    {                                   /* checks owner pointer */
        stat = S_NOTCON;
        goto quit;
    }

    if (cmmp.owner != *co_ptr)
    {
        /* set consistancy clash with another user between locks */
        *cm_ptr = NULL_DBA;
        stat = S_SETCLASH;
        goto quit;
    }

    /* read owner record */
    if ((stat = dio_read(*co_ptr, (char **) &orec, PGHOLD, task)) != S_OKAY)
        goto quit;

    /* get set pointer from owner */
    if ((stat = r_gset(set, orec, &cosp, task)) != S_OKAY)
        goto quit;

    if (cmmp.next == NULL_DBA)
        cosp.last = cmmp.prev;   /* last record in set */
    else
    {
        /* set next record's prev to current member's prev */
        npdba = cmmp.next;
        if (dio_read(npdba, (char **) &nprec, PGHOLD, task) != S_OKAY ||
            r_gmem(set, nprec, &npmp, task) != S_OKAY)
        {
            stat = task->db_status;
            goto quit;
        }

        npmp.prev = cmmp.prev;
        if (r_pmem(set, nprec, (char *) &npmp, task) != S_OKAY ||
            dio_write(npdba, PGFREE, task) != S_OKAY)
        {
            stat = task->db_status;
            goto quit;
        }

        nprec = NULL;
    }

    if (cmmp.prev == NULL_DBA)
    {
        /* first record in set */
        cosp.first = cmmp.next;
    }
    else
    {
        /* set previous record's next to current member's next */
        npdba = cmmp.prev;
        if (dio_read(npdba, (char **) &nprec, PGHOLD, task) != S_OKAY ||
            r_gmem(set, nprec, &npmp, task) != S_OKAY)
        {
            stat = task->db_status;
            goto quit;
        }

        npmp.next = cmmp.next;
        if (r_pmem(set, nprec, (char *) &npmp, task) != S_OKAY ||
            dio_write(npdba, PGFREE, task) != S_OKAY)
        {
            stat = task->db_status;
            goto quit;
        }

        nprec = NULL;
    }

    /* check for timestamp */
    if (set_ptr->st_flags & TIMESTAMPED)
    {
        DECODE_DBA(*co_ptr, &file, &slot);
        file = (FILE_NO) NUM2INT(file, ft_offset);
        cosp.timestamp = dio_pzgetts(file, task);
    }

    /* update membership count */
    --cosp.total;

    /* update owner record's set pointer */
    if (r_pset(set, orec, (char *) &cosp, task) != S_OKAY ||
        dio_write(*co_ptr, PGFREE, task) != S_OKAY)
    {
        stat = task->db_status;
        goto quit;
    }

    orec = NULL;

    /* update current record and current member */
    task->curr_rec = mdba;
    *cm_ptr = cmmp.next;

    /* make member record's member pointer null */
    cmmp.owner = cmmp.prev = cmmp.next = NULL_DBA;

    /* update member record */
    if (r_pmem(set, mrec, (char *) &cmmp, task) != S_OKAY ||
        dio_write(mdba, PGFREE, task) != S_OKAY)
        return (task->db_status);

    /* note timestamps */
    if (task->db_tsrecs)
        dutscr(&task->cr_time, task, dbn);

    if (task->db_tsrecs && *cm_ptr)
        dutscm(nset, &task->cm_time[set], task, dbn);

    /* check for timestamp */
    if (set_ptr->st_flags & TIMESTAMPED)
        task->cs_time[set] = cosp.timestamp;

    if (!(*cm_ptr))
        task->db_status = S_EOS;

    return (task->db_status);

quit:
    if (mrec)
    {
        if (dio_release(mdba, PGFREE, task) != S_OKAY)
            return (task->db_status);
    }

    if (orec)
    {
        if (dio_release(*co_ptr, PGFREE, task) != S_OKAY)
            return (task->db_status);
    }

    if (nprec)
    {
        if (dio_release(npdba, PGFREE, task) != S_OKAY)
            return (task->db_status);
    }

    return (task->db_status = stat);
}


