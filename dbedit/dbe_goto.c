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

    dbe_goto.c - DBEDIT, goto command

    This file contains the function dgoto, which handles the goto
    command, i.e. moves to a new current record or file and reads
    the new current record, if there's a valid one.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_err.h"
#include "dbe_ext.h"


int dgoto(DBE_TOK *tokens, int *tokenptr, DB_TCHAR *errstr, DB_TASK *task)
{
    int         token, error, opt, set, mem, first, last, status;
    short       rec;
    short       fno;
    DB_ULONG    rno, recNum;
    DB_ADDR     new_rec;
    SET_PTR     sp;
    MEM_PTR     mp;

    error = opt = 0;
    token = *tokenptr + 1;
    new_rec = NULL_DBA;

    if (tokens[token].type == TYPE_KWD)
    {
        switch (opt = tokens[token++].ival)
        {
            case K_NEXTR:                 /* goto nextrec */
            case K_PREVR:                 /* goto prevrec */
                d_decode_dba(task->curr_rec, &fno, &rno);
                if (opt == K_NEXTR)
                {
                    rno++;
                }
                else
                {
                    if (!rno)
                        read_nextslot(&rno, task);
                    rno--;
                }
                d_encode_dba(fno, rno, &new_rec);
                if ((error = dbe_read(new_rec, task)) == BAD_DBA)
                    error = (opt == K_NEXTR) ? ERR_NREC : ERR_PREC;
                break;

            case K_FIRST:                 /* goto first member */
            case K_LAST:                  /* goto last member */
                set = tokens[token++].ival;
                if ((error = dbe_chkdba(task->curr_rec, task)) < 0)
                    break;
                
                if ((rec = getrtype(slot.buffer, task)) == -1)
                {
                    error = BAD_TYPE;
                    break;
                }
                if (task->set_table[set].st_own_rt != rec)
                {
                    error = ERR_OSET;
                    break;
                }
                memcpy(&sp, slot.buffer + task->set_table[set].st_own_ptr, SETPSIZE);
                new_rec = (opt == K_FIRST) ? sp.first : sp.last;
                d_decode_dba(new_rec, &fno, &recNum);
                if ((error = dbe_open(fno, task)) >= 0)
                    error = dbe_read(new_rec, task);
                break;

            case K_OWN:                   /* goto owner of set */
            case K_PREV:                  /* goto previous member */
            case K_NEXT:                  /* goto next member */
                set = tokens[token++].ival;
                if ((error = dbe_chkdba(task->curr_rec, task)) < 0)
                    break;
                if ((rec = getrtype(slot.buffer, task)) == -1)
                {
                    error = BAD_TYPE;
                    break;
                }

                first = task->set_table[set].st_members;
                last = first + task->set_table[set].st_memtot;
                for (mem = first; mem < last; mem++)
                {
                    if (task->member_table[mem].mt_record == rec)
                        break;
                }
                if (mem == last)
                {
                    error = ERR_MSET;
                    break;
                }
                memcpy(&mp, slot.buffer + task->member_table[mem].mt_mem_ptr,
                       MEMPSIZE);
                new_rec = (opt == K_OWN) ? mp.owner :
                    (opt == K_PREV) ? mp.prev : mp.next;
                d_decode_dba(new_rec, &fno, &recNum);
                if ((error = dbe_open(fno, task)) >= 0)
                    error = dbe_read(new_rec, task);
                if (error == BAD_DBA)
                {
                    if (opt == K_PREV)
                        error = ERR_PMEM;
                    else if (opt == K_NEXT)
                        error = ERR_NMEM;
                }
                break;

            case K_FILE:                  /* goto file */
                /* Get new file number */
                if (tokens[token].parent != token - 1)
                {
                    error = UNX_END;
                    break;
                }
                fno = (tokens[token].type == TYPE_FIL) ?
                    (FILE_NO) tokens[token].ival : (FILE_NO) tokens[token].lval;
                token++;
                rno = 0L;
                d_encode_dba(fno, rno, &new_rec);

                /* Open new file */
                error = dbe_open(fno, task);
                break;

            default:
                break;
        }
    }
    else if (tokens[token].type == TYPE_DBA)
    {                                   /* e.g. goto [1:20] */
        new_rec = tokens[token++].dbaval;
        d_decode_dba(new_rec, &fno, &recNum);
        if ((error = dbe_open(fno, task)) >= 0)
            error = dbe_read(new_rec, task);
    }

    if (error < 0)
    {
        /* Error - go back to current record, if there is one */
        if (task->curr_rec != NULL_DBA)
        {
            d_decode_dba(task->curr_rec, &fno, &recNum);
            status = changed;
            changed = 0;
            dbe_open(fno, task);
            changed = status;
        }

        /* Some sort of file error - copy file name into error string */
        if (  (error == ERR_OPN) || (error == ERR_READ) ||
                (error == ERR_WRIT) || (error == ERR_NREC) || (error == ERR_PREC))
        {
            vtstrcpy(errstr, task->file_table[fno].ft_name);
        }
    }
    else
    {
        /* No error - change current record to new position */
        task->curr_rec = new_rec;
        if (opt != K_FILE)
            error = dbe_chkdba(task->curr_rec, task);
    }

    *tokenptr = token;
    return (error);
}


