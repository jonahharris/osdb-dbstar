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

/* Internal function Prototypes */
static int INTERNAL_FCN sortcmp(SET_ENTRY *, char *, char *, DB_TASK *);

/* ======================================================================
    Connect the current record as member of set
*/
int INTERNAL_FCN dconnect(int set, DB_TASK *task, int dbn)
{
    MEM_PTR    crmp;       /* current record's member pointer */
    SET_PTR    cosp;       /* current owner's set pointer */
    MEM_PTR    cmmp;       /* current member's member pointer */
    MEM_PTR    nmmp;       /* next member's member pointer */
    char      *crec;       /* ptr to current record contents in cache */
    char      *orec;       /* ptr to current owner record contents in cache */
    char      *mrec;       /* ptr to current member record contents in cache */
    char      *nrec;       /* ptr to next member record contents in cache */
    DB_ADDR    mdba = 0;   /* db address of current member record */
    DB_ADDR    ndba = 0;   /* db address of next member record */
    short      ordering;   /* set order control variable */
    int        compare;    /* sort comparison result */
    short      file;
    DB_ULONG   slot;
    SET_ENTRY *set_ptr;
    DB_ADDR   *co_ptr;
    DB_ADDR   *cm_ptr;

    memset(&crmp, 0, sizeof(crmp));
    memset(&cosp, 0, sizeof(cosp));
    memset(&cmmp, 0, sizeof(cmmp));
    memset(&nmmp, 0, sizeof(nmmp));

    if (nset_check(set, &set, (SET_ENTRY **) &set_ptr, task) != S_OKAY)
        return (task->db_status);

    /* make sure we have a current record */
    if (task->curr_rec == NULL_DBA)
        return (dberr(S_NOCR));

    /* make sure we have a current owner */
    if (*(co_ptr = &task->curr_own[set]) == NULL_DBA)
        return (dberr(S_NOCO));

    crec = orec = mrec = nrec = NULL;

    /* read current record */
    if (dio_read(task->curr_rec, &crec, PGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* read owner record */
    if (dio_read(*co_ptr, &orec, PGHOLD, task) != S_OKAY)
    {
        dio_release(task->curr_rec, PGFREE, task);
        return (task->db_status);
    }

    /* get copy of current record's member ptr for set */
    if (r_gmem(set, crec, &crmp, task) != S_OKAY)
        goto quit;

    /* ensure record not already connected to set */
    if (!crmp.owner == NULL_DBA)
    {
        dberr(S_ISOWNED);
        goto quit;
    }

    /* get set pointer from owner */
    if (r_gset(set, orec, &cosp, task) != S_OKAY)
        goto quit;

    /* set current record's owner to current owner of set */
    crmp.owner = *co_ptr;

    cm_ptr = &task->curr_mem[set];

    /* make insertion based on set order specfication */
    if (cosp.first == NULL_DBA)
    {
        /* set is empty */
        /* set current owner's first and last to current record */
        cosp.first = cosp.last = task->curr_rec;

        /* set current record's next and previous to null */
        crmp.next = crmp.prev = NULL_DBA;
        goto inserted;
    }

    /* order is as specified in DDL */
    ordering = set_ptr->st_order;

    for (;;)       /* infinite loop exited from within */ 
    {
        switch (ordering)
        {
            case ASCENDING:
            case DESCENDING:
                /* perform a linked insertion sort on set - position the
                   current member to the proper place and then switch to order
                   NEXT.
                */
                for (mdba = cosp.first; ; mdba = cmmp.next)
                {
                    /* read member record and get member pointer
                       from member record
                    */
                    if (dio_read(mdba, &mrec, NOPGHOLD, task) != S_OKAY ||
                        r_gmem(set, mrec, &cmmp, task) != S_OKAY)
                    {
                        goto quit;
                    }

                    /* compare sort fields of current record with member record */
                    compare = sortcmp(set_ptr, crec, mrec, task);
                    if ((ordering == ASCENDING) ? (compare <= 0) : (compare >= 0))
                    {
                        /* found insertion position - make previous member the
                           current member of set and switch to next order
                           processing
                        */
                        *cm_ptr = cmmp.prev;
                        ordering = NEXT;
                        break;
                    }
                    if (cmmp.next == NULL_DBA)
                    {
                        /* connect at end of list */
                        *cm_ptr = mdba;
                        ordering = NEXT;
                        break;
                    }
                    if (dio_release(mdba, NOPGFREE, task) != S_OKAY)
                        return (task->db_status);
                }
                if (dio_release(mdba, NOPGFREE, task) != S_OKAY)
                    return (task->db_status);
                break;

            case FIRST:
                crmp.next = cosp.first;
                /* read current owner's first member and get first member's
                   member pointer
                */
                mdba = cosp.first;
                if (dio_read(mdba, &mrec, PGHOLD, task) != S_OKAY ||
                    r_gmem(set, mrec, &cmmp, task) != S_OKAY)
                {
                    goto quit;
                }

                /* set current member's previous, and current owner's first,
                   to current record
                */
                cmmp.prev = cosp.first = task->curr_rec;
                goto inserted;

            case NEXT:
                if (! *cm_ptr)
                {
                    /* if no current member, next is same as first */
                    ordering = FIRST;
                    break;
                }

                /* insert record after current member */
                mdba = *cm_ptr;

                /* read current member record & get member ptr from
                   current member
                */
                if (dio_read(mdba, &mrec, PGHOLD, task) != S_OKAY ||
                    r_gmem(set, mrec, &cmmp, task) != S_OKAY)
                {
                    goto quit;
                }

                if (cmmp.owner != *co_ptr)
                {
                     /* set consistancy clash with another user between locks */
                     *cm_ptr = NULL_DBA;
                     dberr(S_SETCLASH);
                     goto quit;
                }

                /* set current record's next to current member's next */
                crmp.next = cmmp.next;

                /* set current record's prev to current member */
                crmp.prev = mdba;

                /* set current member's next ro current record */
                cmmp.next = task->curr_rec;

                if (crmp.next == NULL_DBA)
                {
                    /* current record at end of list - update set pointer's
                       last member
                    */
                    cosp.last = task->curr_rec;
                    goto inserted;
                }

                /* read next member record and member pointer from next member */
                ndba = crmp.next;
                if (dio_read(ndba, &nrec, PGHOLD, task) != S_OKAY ||
                    r_gmem(set, nrec, &nmmp, task) != S_OKAY)
                {
                    goto quit;
                }

                /* set previous pointer in next member to current record */
                nmmp.prev = task->curr_rec;
                goto inserted;

            case LAST:
                /* set current member to owner's last pointer */
                *cm_ptr = cosp.last;
                
                ordering = NEXT;        /* switch to order next */
                break;

            default:
                /* there are no other possible orderings! */
                return (dberr(SYS_INVSORT));
        }                                /* switch */
    }                                   /* while (true) */

inserted:
    /* increment total members in set */
    ++cosp.total;

    /* check for timestamp */
    if (set_ptr->st_flags & TIMESTAMPED)
    {
        DECODE_DBA(*co_ptr, &file, &slot);
        file = NUM2INT(file, ft_offset);
        cosp.timestamp = dio_pzgetts(file, task);
    }

    if (mrec)
    {
        /* put member pointer back into member record and mark member
           record as modified
        */
        if (r_pmem(set, mrec, (char *) &cmmp, task) != S_OKAY ||
            dio_write(mdba, PGFREE, task) != S_OKAY)
        {
            goto quit;
        }
    }
    if (nrec)
    {
        /* put member pointer back into next record and mark next
           record as modified
        */
        if (r_pmem(set, nrec, (char *) &nmmp, task) != S_OKAY ||
            dio_write(ndba, PGFREE, task) != S_OKAY)
        {
            goto quit;
        }
    }

    /* put set pointer back into owner record and mark owner record as
       modified; put member pointer back into current record mark current
       record as modified
    */
    if (r_pset(set, orec, (char *) &cosp, task) != S_OKAY ||
        dio_write(*co_ptr, PGFREE, task) != S_OKAY ||
        r_pmem(set, crec, (char *) &crmp, task) != S_OKAY ||
        dio_write(task->curr_rec, PGFREE, task) != S_OKAY)
    {
        goto quit;
    }

    /* set current member to current record */
    *cm_ptr = task->curr_rec;

    /* check and fetch timestamps */
    if (task->db_tsrecs)
        dutscr(&task->cm_time[set], task, dbn);
    if (task->db_tssets)
        task->cs_time[set] = cosp.timestamp;

    return (task->db_status);

/* error return */
quit:
    if (crec)
        dio_release(task->curr_rec, PGFREE, task);

    if (orec)
        dio_release(*co_ptr, PGFREE, task);

    if (mrec)
        dio_release(mdba, PGFREE, task);

    if (nrec)
        dio_release(ndba, PGFREE, task);

    return (task->db_status);
}


/* ======================================================================
    Compare two sort fields
*/
static int INTERNAL_FCN sortcmp(SET_ENTRY *set_ptr, char *mem1, char *mem2, DB_TASK *task)
{
    short rn1, rn2;                  /* record numbers for mem1 & mem2 */
    MEMBER_ENTRY *mt1, *mt2;
    register MEMBER_ENTRY *mt;
    int mem, memtot;
    int cmp;                            /* fldcmp result */
    register int maxflds;
    register SORT_ENTRY *srt1_ptr, *srt2_ptr;
    FIELD_ENTRY *fld_ptr;

    /* extract record numbers from record header */
    memcpy(&rn1, mem1, sizeof(short));
    rn1 &= ~RLBMASK;                    /* mask off rlb */
    memcpy(&rn2, mem2, sizeof(short));
    rn2 &= ~RLBMASK;                    /* mask off rlb */
    rn1 += task->curr_db_table->rt_offset;
    rn2 += task->curr_db_table->rt_offset;

    /* locate task->member_table entries for these record types */
    mt2 = mt1 = NULL;
    for (mem = set_ptr->st_members, memtot = mem + set_ptr->st_memtot,
            mt = &task->member_table[mem]; mem < memtot; ++mem, ++mt)
    {
        if (mt->mt_record == rn1)
        {
            mt1 = mt;
            if (mt2 != NULL)
                break;
        }
        if (mt->mt_record == rn2)
        {
            mt2 = mt;
            if (mt1 != NULL)
                break;
        }
    }

    /* set maxflds to number of sort fields in set (min(mt1,mt2)) */
    maxflds = (mt1->mt_totsf <= mt2->mt_totsf) ? mt1->mt_totsf : mt2->mt_totsf;

    /* do comparison for each field */
    for ( srt1_ptr = &task->sort_table[mt1->mt_sort_fld],
            srt2_ptr = &task->sort_table[mt2->mt_sort_fld]; maxflds--;
            ++srt1_ptr, ++srt2_ptr)
    {
        /* compare the two fields */
        /* computation is pointer to next sort field in member record */
        fld_ptr = &task->field_table[srt1_ptr->se_fld];
        if ((cmp = fldcmp(fld_ptr, mem1 + fld_ptr->fd_ptr,
                mem2 + task->field_table[srt2_ptr->se_fld].fd_ptr, task)) != FALSE)
        {
            /* return at first unequal fields */
            return cmp;
        }
    }

    /* fields match */
    return 0;
}


