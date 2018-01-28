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

    dbe_edit.c - DBEDIT, edit command

    This file contains functions to edit the current record (for editing
    of the current file - edit hex command - see DBE_EDX.C)

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_str.h"
#include "dbe_io.h"
#include "dbe_err.h"
#include "dbe_ext.h"


int edit(DBE_TOK *tokens, int *tokenptr, DB_TCHAR *errstr, DB_TASK *task)
{
    int         token, error, opt, set, mem, first, last, offset;
    short       fno;
    short       rec;
    DB_ULONG    recNum;

    error = 0;
    token = *tokenptr + 1;

    if (tokens[token].type == TYPE_KWD)
        switch (opt = tokens[token++].ival)
        {
            case K_TYPE:                  /* Edit record type */
                if ((error = dbe_chkdba(task->curr_rec, task)) >= 0)
                {
                    if ((error = edit_type(slot.buffer, task)) >= 0)
                        changed = 1;
                }
                break;

            case K_DBA:       /* Edit database Address stored in record */
                if ((error = dbe_chkdba(task->curr_rec, task)) >= 0)
                {
                    if ((error = edit_dba(slot.buffer + sizeof(short),
                            1, task)) >= 0)
                    {
                        changed = 1;
                    }
                }
                break;

            case K_OPT:                   /* Edit stored optional keys */
                if ((error = dbe_chkdba(task->curr_rec, task)) >= 0)
                {
                    if ((error = edit_opt(slot.buffer, task)) >= 0)
                        changed = 1;
                }
                break;

            case K_COUNT:                 /* Edit owner pointer */
            case K_FIRST:
            case K_LAST:
                set = tokens[token++].ival;
                if ((error = dbe_chkdba(task->curr_rec, task)) < 0)
                    break;
                if ((rec = getrtype(slot.buffer, task)) == -1)
                {
                    error = BAD_TYPE;
                    break;
                }

                /* Check current record is an owner of specified set */
                if (task->set_table[set].st_own_rt != rec)
                {
                    error = ERR_OSET;
                    break;
                }
                offset = task->set_table[set].st_own_ptr;
                if (opt == K_FIRST || opt == K_LAST)
                    offset += sizeof(long);
                if (opt == K_LAST)
                    offset += sizeof(DB_ADDR);
                error = (opt == K_COUNT) ?
                    edit_num(slot.buffer + offset) :
                    edit_dba(slot.buffer + offset, 0, task);
                if (error >= 0)
                    changed = 1;
                break;

            case K_OWN:                   /* Edit member pointer */
            case K_PREV:
            case K_NEXT:
                set = tokens[token++].ival;
                if ((error = dbe_chkdba(task->curr_rec, task)) < 0)
                    break;

                /* Check current record is an member of specified set */
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
                offset = task->member_table[mem].mt_mem_ptr;
                if (opt == K_PREV || opt == K_NEXT)
                    offset += sizeof(DB_ADDR);
                if (opt == K_NEXT)
                    offset += sizeof(DB_ADDR);
                if ((error = edit_dba(slot.buffer + offset, 0, task)) >= 0)
                    changed = 1;
                break;

            case K_DCHAIN:        /* Edit delete chain pointer, current file */
                error = edit_dchain(task);
                break;

            case K_NEXTS:              /* Edit next slot number, current file */
                error = edit_nextslot(task);
                break;

            case K_HEX:          /* Edit hex - bytewise edit of current file */
                if ((error = dbe_chkdba(task->curr_rec, task)) != ERR_CFIL)
                {
                    error = 0;

                    /* Changes to current record must appear in edit hex */
                    if (changed)
                    {
                        if ((error = dbe_write(task)) < 0)
                            break;
                        changed = 0;
                    }
                    edit_hex(task);

                    /* Contents of current record may have changed */
                    dbe_read(task->curr_rec, task);
                }
                break;

            default:
                break;
        }

    if (error == ERR_WRIT)
    {
        d_decode_dba(task->curr_rec, &fno, &recNum);
        vtstrcpy(errstr, task->file_table[fno].ft_name);
    }

    *tokenptr = token;
    return (error);
}


/* Edit record type *********************************************************
*/
int edit_type(char *ptr, DB_TASK *task)
{
    DB_TCHAR    line[80], type_str[40], *p;
    short       rec_type, new_type;
    int         del_flag, rlb_flag;
    long        lowbyte, highbyte;
    short       fno;
    DB_ULONG    slot;

    del_flag = rlb_flag = 0;

    /* Display old record type */
    vtstrcpy(line, dbe_getstr(M_EDITCV));
    p = do_hexstr(ptr, line, sizeof(short));

    /* Append record name */
    memcpy(&rec_type, ptr, sizeof(short));
    if (rec_type < 0)
    {
        rec_type = (short) (~rec_type);
        del_flag = 1;
    }
    if (rec_type & RLBMASK)
    {
        rec_type &= ~RLBMASK;
        rlb_flag = 1;
    }
    if (rec_type >= 0 && rec_type < task->size_rt)
    {
        vtstrcat(p, DB_TEXT(" "));
        vtstrcat(p, task->record_names[rec_type]);

        /* Record lock bit set ? */
        if (rlb_flag)
        vtstrcat(line, dbe_getstr(M_FLAGLB));

        /* Record deleted ? */
        if (del_flag)
            vtstrcat(line, dbe_getstr(M_FLAGDE));
    }
    vtstrcat(p, DB_TEXT("\n"));
    dbe_out(line, STDOUT);

    /* Get new record type */
    dbe_getline(dbe_getstr(M_EDITNV), line, sizeof(line) / sizeof(DB_TCHAR));
    p = gettoken(line, type_str, sizeof(type_str) / sizeof(DB_TCHAR));

    /* Nothing entered - don't change current value */
    if (!(type_str[0]))
        return (0);

    /* User may enter record name or number */
    if ((rec_type = (short) getrec(type_str, task)) < 0)
    {                                   /* If not record name */
        if ((lowbyte = gethex(type_str)) < 0)
            return (BAD_TYPE);
        highbyte = 0;
        p = gettoken(p, type_str, sizeof(type_str) / sizeof(DB_TCHAR));
        if (type_str[0])
        {
            if ((highbyte = gethex(type_str)) < 0)
                return (BAD_TYPE);
            highbyte <<= BITS_PER_BYTE;
            p = gettoken(p, type_str, sizeof(type_str) / sizeof(DB_TCHAR));
            if (type_str[0])
                return (BAD_TYPE);
        }
        rec_type = (short) (highbyte | lowbyte);
    }

    /* Check that new record type is in current file */
    d_decode_dba(task->curr_rec, &fno, &slot);
    new_type = rec_type;
    if (new_type < 0)
        new_type = (short) ~new_type;
    if (new_type & RLBMASK)
        new_type &= ~RLBMASK;
    if (new_type < 0 || new_type >= task->size_rt)
        return (BAD_TYPE);
    if (task->record_table[new_type].rt_file != (short) fno)
        return (ERR_RFIL);

    memcpy(ptr, &rec_type, sizeof(short));
    return (0);
}


/* Edit database address stored in record **********************************
*/
int edit_dba(char *ptr, int check, DB_TASK *task)
{
    DB_TCHAR    dba_str[20], prompt_str[60];
    DB_ADDR     dba;
    short       fno, c_fno;
    DB_ULONG    slot;

    /* Display old value */
    memcpy(&dba, ptr, DB_ADDR_SIZE);
    d_decode_dba(dba, &fno, &slot);
    vstprintf(dba_str, decimal ? DB_TEXT("[%d:%lu]") : DB_TEXT("[%x:%lx]"),
              fno, slot);
    vtstrcpy(prompt_str, dbe_getstr(M_EDITCV));
    vtstrcat(prompt_str, dba_str);
    vtstrcat(prompt_str, DB_TEXT("\n"));
    dbe_out(prompt_str, STDOUT);

    /* Get new value */
    dbe_getline(dbe_getstr(M_EDITNV), prompt_str, sizeof(prompt_str) / sizeof(DB_TCHAR));
    gettoken(prompt_str, dba_str, sizeof(dba_str) / sizeof(DB_TCHAR));

    /* Nothing entered - don't change current value */
    if (!(dba_str[0]))
        return (0);

    if (getdba(dba_str, &dba) < 0)
        return (BAD_DBA);

    /* Check that it's a valid address (not for owner / member pointers) */
    if (check)
    {
        d_decode_dba(dba, &fno, &slot);
        d_decode_dba(task->curr_rec, &c_fno, &slot);

        /* Zero always valid file number - for delete chain */
        if ((fno != 0) && (fno != c_fno))
            return (BAD_FILE);
    }
    memcpy(ptr, &dba, DB_ADDR_SIZE);
    return (0);
}


/* Edit stored optional keys
*/
int edit_opt(char *ptr, DB_TASK *task)
{
    register int   i, fld;
    short          rec;
    long           ln;
    int            n, first, last, count, len, bytes, offset, error;
    DB_TCHAR       hex_line[40], ascii_line[40], line[80], *pt;
    char          *p;

    error = 0;
    ascii_line[0] = hex_line[0] = 0;

    /* Get record type */
    if ((rec = getrtype(slot.buffer, task)) == -1)
    {
        return (error = BAD_TYPE);
    }

    /* Count optional keys */
    first = task->record_table[rec].rt_fields;
    last = first + task->record_table[rec].rt_fdtot;
    for (count = 0, fld = first; fld < last; fld++)
    {
        if (task->field_table[fld].fd_flags & OPTKEYMASK)
            count++;
    }
    if (count == 0)
        return (ERR_NOPT);

    /* Display current values, bit map in hex_line, names in ascii_line */
    offset = task->record_table[rec].rt_flags & TIMESTAMPED ?
        RECHDRSIZE + 2 * sizeof(DB_ULONG) : RECHDRSIZE;
    p = ptr + offset;
    len = bytes = (count + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    for (fld = first; fld < last || len > 0; fld++)
    {
        if (fld < last)
        {
            /* Each name on a new line, followed by stored / not stored */
            if (!(task->field_table[fld].fd_flags & OPTKEYMASK))
                continue;
            vtstrcpy(ascii_line, task->field_names[fld]);
            vtstrcat(ascii_line,
                     dbe_getstr(testopt(fld, ptr, task) ? M_FLAGKS : M_FLAGKN));
        }
        else
        {
            ascii_line[0] = 0;
        }
        if (len > 0)
        {
            n = len > 8 ? 8 : len;
            do_hexstr(p, hex_line, n);
            p += n;
            len -= n;
        }
        else
        {
            hex_line[0] = 0;
        }
        vstprintf(line, DB_TEXT("%-30.30s  %s\n"), hex_line, ascii_line);
        dbe_out(line, STDOUT);
    }

    /* Get new values */
    dbe_getline(dbe_getstr(M_EDITNV), line, sizeof(line) / sizeof(DB_TCHAR));
    ptr += offset;
    pt = gettoken(line, hex_line, sizeof(hex_line) / sizeof(DB_TCHAR));

    /* Assume new value entered as a bit mask */
    for (i = 0; i < bytes && hex_line[0]; i++)
    {
        if ((ln = gethex(hex_line)) < 0)
        {
            error = BAD_HEX;
            break;
        }
        pt = gettoken(pt, hex_line, sizeof(hex_line) / sizeof(DB_TCHAR));
    }

    /* It is a valid bit mask - can update current record */
    if (error >= 0)
    {
        pt = gettoken(line, hex_line, sizeof(hex_line) / sizeof(DB_TCHAR));
        for (i = 0; i < bytes && hex_line[0]; i++)
        {
            ln = gethex(hex_line);
            ptr[i] = (char) (0xFF & ln);
            pt = gettoken(pt, hex_line, sizeof(hex_line) / sizeof(DB_TCHAR));
        }
    }
    else
    {
        /* It isn't a bit mask - is it a comma-separated list of names ? */
        pt = gettoken(line, ascii_line, sizeof(ascii_line) / sizeof(DB_TCHAR));
        while (ascii_line[0])
        {
            if ((fld = getfld(ascii_line, &n, task)) < 0)
                return (error);
            pt = gettoken(pt, ascii_line, sizeof(ascii_line) / sizeof(DB_TCHAR));
        }

        /* Valid comma-separated list - update current record */
        for (i = 0; i < bytes; i++)
            ptr[i] = 0;
        pt = gettoken(line, ascii_line, sizeof(ascii_line) / sizeof(DB_TCHAR));
        while (ascii_line[0])
        {
            if ((fld = getfld(ascii_line, &n, task)) < 0)
                return (error);
            n = (((task->field_table[fld].fd_flags & OPTKEYMASK)
                    >> OPTKEYSHIFT) & OPTKEYNDX) - 1;
            if (n >= 0)
            {
                ptr[n / BITS_PER_BYTE] |=
                    1 << (BITS_PER_BYTE - n % BITS_PER_BYTE - 1);
            }
            pt = gettoken(pt, ascii_line, sizeof(ascii_line) / sizeof(DB_TCHAR));
        }
    }

    return (0);
}


/* Edit a number (e.g. membership count)
*/
int edit_num(char *ptr)
{
    long  num;
    long  old, new;
    int   error;

    /* Don't want to update current record till input has been validated */
    memcpy(&num, ptr, sizeof(long));
    old = num;
    if ((error = edit_long(old, &new)) < 0)
        return (error);
    num = new;
    memcpy(ptr, &num, sizeof(long));
    return (0);
}


/* Edit a number - may put rubbish in new
*/
int edit_long(long old, long *new)
{
    DB_TCHAR num_str[20], prompt_str[60];

    /* Display old value */
    vstprintf(num_str, decimal ? DB_TEXT("%ld") : DB_TEXT("%lx"), (long) old);
    vtstrcpy(prompt_str, dbe_getstr(M_EDITCV));
    vtstrcat(prompt_str, num_str);
    vtstrcat(prompt_str, DB_TEXT("\n"));
    dbe_out(prompt_str, STDOUT);

    /* Get new value */
    dbe_getline(dbe_getstr(M_EDITNV), prompt_str,
        sizeof(prompt_str) / sizeof(DB_TCHAR));
    gettoken(prompt_str, num_str, sizeof(num_str) / sizeof(DB_TCHAR));

    /* Anything entered ? */
    if (!(num_str[0]))
    {
        *new = old;
    }
    else if (decimal)       /* Base 10 or 16 ? */
    {
        if ((*new = getlong(num_str)) < 0L)
            return (BAD_NUM);
    }
    else
    {
        if ((*new = gethex(num_str)) < 0L)
            return (BAD_HEX);
    }
    return (0);
}


/* Edit delete chain pointer, current file
*/
int edit_dchain(DB_TASK *task)
{
    int      error;
    F_ADDR   dchain;
    long     old, new;

    if ((error = read_dchain(&dchain, task)) < 0)
        return (error);
    old = dchain;
    if ((error = edit_long(old, &new)) < 0)
        return (error);
    dchain = new;
    return (write_dchain(dchain, task));
}


/* Edit next slot number, current file
*/
int edit_nextslot(DB_TASK *task)
{
    int      error;
    DB_ULONG nextslot;
    long     old, new;

    if ((error = read_nextslot(&nextslot, task)) < 0)
        return (error);
    old = nextslot;
    if ((error = edit_long(old, &new)) < 0)
        return (error);
    nextslot = new;
    return (write_nextslot(nextslot, task));
}


