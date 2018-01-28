/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbedit utility                                    *
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

    dbe_vrfy.c - DBEDIT, verify command

    The verify command checks the consistency of one instance of a set.
    The check is similar to that performed by DBCHECK, except that DBCHECK
    reads through all records in the owner and member files, and also
    performs other checks not done by verify (e.g. key files).

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_str.h"
#include "dbe_err.h"
#include "dbe_ext.h"
#include "dbe_io.h"

/* ************************** FUNCTIONS ****************************** */

/* Verify a set, i.e. check owner & member pointers.

    First establish whether current record is the owner, or a member,
    of the set to be checked. If it is neither, return an error.
    If the current record is a member, find the owner, and check its
    type.

    The verify starts from the owner record. Its set pointers are
    checked, and then the members are read sequentially (from first
    to last), each previous pointer and owner pointer being tested for
    consistency.

*/
int verify(DBE_TOK *tokens, int *tokenptr, DB_TCHAR *errstr, DB_TASK *task)
{
    int         i, error, err, status, mt, token, first, last;
    long        error_count;
    char       *p;
    short       own_fno;
    short       own_type, curr_type, type, st;
    DB_ADDR     own_dba = 0;
    MEM_PTR     memptr;
    DB_ULONG    own_rno;
    int         hold = NOPGHOLD;

    error = err = 0;
    error_count = 0L;
    own_fno = -1;
    token = *tokenptr + 1;
    st = (short) tokens[token].ival;

    /* Test for valid current record */
    if ((error = dbe_chkdba(task->curr_rec, task)) < 0)
        return (error);

    /* Get nextslot values for all files */
    for (i = 0; i < task->size_ft; i++)
    {
        if (task->file_table[i].ft_type == DATA)
            dio_pzread((FILE_NO) i, task);
    }

    /* Get current record type and owner record type */
    memcpy(&curr_type, slot.buffer, sizeof(short));
    if (curr_type & RLBMASK)
        curr_type &= ~RLBMASK;
    own_type = task->set_table[st].st_own_rt;

    /* Is current record owner of set to be verified ? */
    if (curr_type == own_type)
    {
        own_dba = task->curr_rec;
        p = slot.buffer;
    }
    else
    {
        /* Current record is not owner - must find and read owner.
           Is current record member of set to be verified ?
        */
        first = task->set_table[st].st_members;
        last = first + task->set_table[st].st_memtot;
        for (mt = first; mt < last; mt++)
        {
            if (curr_type == task->member_table[mt].mt_record)
                break;
        }

        if (mt < last)
        {
            /* Current record is a member - find owner */
            memcpy(&memptr, slot.buffer + task->member_table[mt].mt_mem_ptr,
                   sizeof(MEM_PTR));

            /* Check owner address is valid, and owner is in right file */
            own_dba = memptr.owner;
            d_decode_dba(own_dba, &own_fno, &own_rno);
            if (vrfydba(own_dba, task)
             || (own_fno != task->record_table[own_type].rt_file))
            {
                pr_error(0);
                pr_errdba(BAD_OWNP, memptr.owner);
                pr_ownmem(st, task->curr_rec, M_VRFYMA, task);
                error_count++;
            }

            /* Read the owner record - hold it in cache while verifying */
            status = dio_read(own_dba, &p, hold = PGHOLD , task);
            if (status == S_OKAY && p != NULL)
            {
                /* Check type of record just read */
                memcpy(&type, p, sizeof(short));
                if (type & RLBMASK)
                    type &= ~RLBMASK;
                if (type != own_type)
                {
                    pr_error(0);
                    pr_errtype(BAD_OWNT, (int) type);
                    pr_ownmem(st, task->curr_rec, M_VRFYMA, task);
                    error_count++;
                }
            }
            else
                error = ERR_READ;
        }
        else
        {
            /* Current record is neither owner nor member of set */
            error = ERR_CSET;
        }
    }

    /* Verify set owner pointers of owner record */
    if (error >= 0 && !error_count)
        error_count = chk_own(p, st, own_dba, task);

    /* Finished verifying - release owner from cache, if held */
    if (hold == PGHOLD)
        dio_release(own_dba, PGFREE , task);

    *tokenptr = ++token;

    if (error >= 0)
    {
        pr_total(error_count);
        if (err)
            error = err;
    }
    if ((error == ERR_READ) || (error == ERR_OPN))
    {
        if (own_fno >= 0 && own_fno < task->size_ft)
            vtstrcpy(errstr, task->file_table[own_fno].ft_name);
    }
    return (error);
}


/* ******************* Check owner pointers ************************** */

/* chk_own - based on function of same name in setchk.c
*/
long chk_own(char *own_rec, short st, DB_ADDR own_dba, DB_TASK *task)
{
    SET_PTR        setptr;
    MEM_PTR        memptr;
    DB_ADDR        mem_dba, prev_mem;
    register int   mt;
    short          fno;
    char          *p;
    short          type;
    F_ADDR         mcount;
    int            error, status;
    long           error_count;
    DB_ULONG       recNum;

    error_count = 0L;

    /* copy the set structure from the record */
    memcpy(&setptr, own_rec + task->set_table[st].st_own_ptr, sizeof(SET_PTR));

    /* check the validity of the first and last pointers */
    if (vrfydba(setptr.last, task))
    {
        pr_error(0);
        pr_errdba(BAD_LAST, setptr.last);
        pr_ownmem(st, own_dba, M_VRFYOA, task);
        error_count++;
    }

    if (vrfydba(setptr.first, task))
    {
        pr_error(0);
        pr_errdba(BAD_FRST, setptr.first);
        pr_ownmem(st, own_dba, M_VRFYOA, task);
        error_count++;
        return (error_count);      /* must have good first ptr to proceed */
    }

    /* get the DB_ADDR of the first member */
    mem_dba = setptr.first;
    mcount = 0;
    prev_mem = NULL_DBA;

    /* scan all members in this set */
    while (mem_dba != NULL_DBA)
    {
        error = 0;

        /* make sure is it a good database address */
        if (vrfydba(mem_dba, task))
        {
            pr_error(error++);
            pr_errdba(BAD_MEMP, mem_dba);
            ++error_count;
            return (error_count);
        }

        /* read the member record if necessary */
        if (mem_dba == task->curr_rec)
        {
            p = slot.buffer;
        }
        else if (mem_dba == own_dba)
        {
            p = own_rec;
        }
        else
        {
            d_decode_dba(mem_dba, &fno, &recNum);

            status = dio_read(mem_dba, &p, NOPGHOLD , task);
            if (status != S_OKAY || p == NULL)
            {
                dbe_err(ERR_READ, task->file_table[fno].ft_name);
                return (error_count);
            }
        }

        /* obtain and validate the member type */
        memcpy(&type, p, sizeof(short));
        type &= ~RLBMASK;

        if ((mt = val_mtype(st, type, task)) == -1)
        {
            pr_error(error++);
            pr_errtype(BAD_MEMT, (int) type);
        }

        /* obtain the member pointers */
        memcpy(&memptr, p + task->member_table[mt].mt_mem_ptr, sizeof(MEM_PTR));
        mcount++;

        /* if this is the first member, the PREV pointer must be null */
        if (mem_dba == setptr.first)
        {
            if (memptr.prev != NULL_DBA)
            {
                pr_error(error++);
                dbe_err(ERR_PNN, NULL);
            }
        }

        /* if this is the last member, the NEXT pointer must be null */
        if (mem_dba == setptr.last)
        {
            if (memptr.next != NULL_DBA)
            {
                pr_error(error++);
                dbe_err(ERR_NNN, NULL);
                ++error_count;
                pr_ownmem(st, mem_dba, M_VRFYMA, task);
                mem_dba = NULL_DBA;
                continue;
            }
        }

        /* If PREV pointer is null, must be first member of set */
        if (memptr.prev == NULL_DBA)
        {
            if (mem_dba != setptr.first)
            {
                pr_error(error++);
                dbe_err(ERR_PN, NULL);
            }
        }

        /* If NEXT pointer is null, must be last member of set */
        if (memptr.next == NULL_DBA)
        {
            if (mem_dba != setptr.last)
            {
                pr_error(error++);
                dbe_err(ERR_NN, NULL);
            }
        }

        /* make sure that the owner pointers match up */
        if (memptr.owner != own_dba)
        {
            pr_error(error++);
            dbe_err(ERR_MEMP, NULL);
        }

        /* the previous pointer should point the the previous member */
        if (memptr.prev != prev_mem)
        {
            pr_error(error++);
            dbe_err(ERR_PREV, NULL);
        }

        /* summarize any errors */
        if (error)
        {
            ++error_count;
            pr_ownmem(st, mem_dba, M_VRFYMA, task);
        }

        /* point to the next member */
        prev_mem = mem_dba;
        mem_dba = memptr.next;
    }                                   /* end scan all members in this set */

    /* verify the membership count */
    if (mcount != setptr.total)
    {
        pr_error(0);
        ++error_count;
        pr_errcount(setptr.total, mcount);
        pr_ownmem(st, own_dba, M_VRFYOA, task);
    }
    return (error_count);
}


/* locate the member table entry for this record type - copied from setchk.c
*/
int val_mtype(short st, short type, DB_TASK *task)
{
    register short i;

    for ( i = task->set_table[st].st_members;
          i < task->set_table[st].st_members + task->set_table[st].st_memtot;
          i++ )
    {
        if (task->member_table[i].mt_record == type)
            return (i);
    }
    return (-1);
}


/* Print error, and address of record with bad set owner or member pointer
*/
int pr_ownmem(short st, DB_ADDR dba, int message, DB_TASK *task)
{
    DB_TCHAR    line[LINELEN];
    short       fno;
    DB_ULONG    rno;

    d_decode_dba(dba, &fno, &rno);
    vstprintf(line,
        decimal ? DB_TEXT("%s%s%s[%d:%lu]\n") : DB_TEXT("%s%s%s[%x:%lx]\n"),
        dbe_getstr(M_VRFYST), task->set_names[st], dbe_getstr(message),
        fno, rno);
    dbe_out(line, STDERR);

    return 0;
}


/* General function for printing error message with database address
*/
int pr_errdba(int error, DB_ADDR dba)
{
    DB_TCHAR    buffer[32];
    short       fno;
    DB_ULONG    rno;

    d_decode_dba(dba, &fno, &rno);
    vstprintf(buffer, decimal ? DB_TEXT("[%d:%ld]") : DB_TEXT("[%x:%lx]"),
              fno, rno);
    dbe_err(error, buffer);

    return 0;
}


/* Print bad record type error, with record type read from file
*/
int pr_errtype(int error, int type)
{
    DB_TCHAR buffer[32];

    vstprintf(buffer, decimal ? DB_TEXT("%d") : DB_TEXT("%x"), type);
    dbe_err(error, buffer);

    return 0;
}


/* Print bad membership count error, with actual and stored membership counts
*/
int pr_errcount(long stored, long actual)
{
    DB_TCHAR line[LINELEN];
    long     n1, n2;

    n1 = stored;
    n2 = actual;
    vstprintf(line, DB_TEXT("%s%ld%s%ld\n"),
              dbe_getstr(M_VRFYMM), n1,
              dbe_getstr(M_VRFYAM), n2);
    dbe_out(line, STDERR);

    return 0;
}


/* Print total number of errors found
*/
int pr_total(long count)
{
    DB_TCHAR line[LINELEN];

    dbe_out(dbe_getstr(M_VRFYCO), STDERR);
    vstprintf(line, DB_TEXT("%ld%s"),
        count, dbe_getstr(count == 1L ? M_VRFY1E : M_VRFYER));
    dbe_out(line, STDERR);

    return 0;
}


/* Print opening error message, if this is the first error in this slot
*/
int pr_error(int count)
{
    if (count == 0)
        dbe_out(dbe_getstr(M_VRFYDE), STDERR);

    return 0;
}


/* Check database address for validity
*/
int vrfydba(DB_ADDR dba, DB_TASK *task)
{
    short    fno;
    DB_ADDR  rno;

    d_decode_dba(dba, &fno, &rno);

    if ((fno < 0) || (fno >= task->size_ft))
        return (BAD_DBA);
    if (task->file_table[fno].ft_type == 'k')
        return (BAD_DFIL);
    if ((F_ADDR) rno < 0 || (F_ADDR) rno >= task->pgzero[fno].pz_next)
        return (BAD_DBA);

    return (0);
}


