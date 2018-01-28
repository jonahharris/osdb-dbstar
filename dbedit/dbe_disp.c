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

    dbe_disp.c - DBEDIT, display command

    This file contains all the functions for the display command (i.e. to
    output the contents of the current record).

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "dbe_type.h"
#include "dbe_str.h"
#include "dbe_io.h"
#include "dbe_err.h"
#include "dbe_ext.h"

#define DBEDIT
#include "cnvfld.h"


int disp(DBE_TOK *tokens, int *tokenptr, DB_TASK *task)
{
    int n, token, error, option;

    error = 0;
    token = *tokenptr + 1;

    if ((error = dbe_chkdba(task->curr_rec, task)) == BAD_DBA)
        error = ERR_CREC;
    if (error >= 0)
    {
        error = 0;
        if (tokens[token].parent != token - 1 || tokens[token].type == 0)
        {
            /* Next token isn't an argument of disp command, or else there
               isn't another token, i.e. display everything
            */
            if ((error = disp_type(task)) != 0)
                return error;

            if ((error = disp_dba()) != 0)
                return error;
            
            if ((error = disp_ts(task)) != 0)
                return error;
            
            if ((error = disp_opt(task)) != 0)
                return error;

            if ((error = disp_set(-1, task)) != 0)
                return error;

            if ((error = disp_mem(-1, task)) != 0)
                return error;

            if ((error = disp_fld(-1, task)) != 0)
                return error;
        }
        else
        {
            switch (option = tokens[token++].ival)
            {
                case K_TYPE:   error = disp_type(task);    break;
                case K_DBA:    error = disp_dba();         break;
                case K_TS:     error = disp_ts(task);      break;
                case K_OPT:    error = disp_opt(task);     break;

                case K_SET:
                case K_MEM:
                case K_FLD:
                    n = -1;
                    if ((tokens[token].parent == token - 1) &&
                        (tokens[token].type != 0))
                    {
                        n = tokens[token++].ival;
                    }
                    if (option == K_SET)
                        error = disp_set(n, task);
                    else if (option == K_MEM)
                        error = disp_mem(n, task);
                    else
                        error = disp_fld(n, task);
                    break;

                default:
                    break;
            }
        }
    }

    *tokenptr = token;
    return (error);
}

/* ********************** Display type ******************************* */

int disp_type(DB_TASK *task)
{
    short rec_type;
    DB_TCHAR hex_line[50], ascii_line[30 + NAMELEN];

    /* Get record type */
    if ((rec_type = getrtype(slot.buffer, task)) == -1)
        return (BAD_TYPE);

    /* Print title */
    if (titles)
        disp_str(DB_TEXT(""), dbe_getstr(M_DISPRT));

    /* Print type */
    do_hex(slot.address, slot.buffer, hex_line, sizeof(short));
    vstprintf(ascii_line, DB_TEXT("%s (%d)"), task->record_names[rec_type], rec_type);
    disp_str(hex_line, ascii_line);
    if (titles)
        dbe_out(DB_TEXT("\n"), STDOUT);

    return (0);
}

/* ***************** Display dba - database address ****************** */

int disp_dba()
{
    DB_ADDR     dba;
    DB_TCHAR    hex_line[50], ascii_line[50];
    short       file;
    DB_ULONG    rec_num;

    /* Get dba */
    memcpy(&dba, slot.buffer + sizeof(short), DB_ADDR_SIZE);

    /* Print title */
    if (titles)
        disp_str(DB_TEXT(""), dbe_getstr(M_DISPDA));

    /* Print dba */
    do_hex(slot.address + sizeof(short), slot.buffer + sizeof(short),
           hex_line, DB_ADDR_SIZE);
    d_decode_dba(dba, &file, &rec_num);
    vstprintf(ascii_line, decimal ? DB_TEXT("[%d:%lu]") : DB_TEXT("[%x:%lx]"),
              file, rec_num);
    disp_str(hex_line, ascii_line);
    if (titles)
        dbe_out(DB_TEXT("\n"), STDOUT);
    return (0);
}

/* ************* Display ts - creation & update timestamp ************ */

int disp_ts(DB_TASK *task)
{
    DB_TCHAR    hex_line[50], ascii_line[50];
    short       rec_type;
    DB_ULONG    cts, uts;
    int         n;

    /* Get record type and check whether record is timestamped */
    if ((rec_type = getrtype(slot.buffer, task)) == -1)
        return (BAD_TYPE);

    if (!(task->record_table[rec_type].rt_flags & TIMESTAMPED))
        return(0);

    /* Print title */
    if (titles)
        disp_str(DB_TEXT(""), dbe_getstr(M_DISPCU));

    /* Get timestamps */
    memcpy(&cts, slot.buffer + RECCRTIME, sizeof(DB_ULONG));
    memcpy(&uts, slot.buffer + RECUPTIME, sizeof(DB_ULONG));

    /* Print timestamps */
    n = do_hex(slot.address + RECCRTIME, slot.buffer + RECCRTIME,
               hex_line, 2 * sizeof(DB_ULONG));
    vstprintf(ascii_line, DB_TEXT("%06ld/%06ld"), (long) cts, (long) uts);
    disp_str(hex_line, ascii_line);
    if (n < (int) (2 * sizeof(DB_ULONG)))
    {
        do_hex(-1L, slot.buffer + RECCRTIME + n, hex_line, 2 * sizeof(DB_ULONG) - n);
        ascii_line[0] = 0;
        disp_str(hex_line, ascii_line);
    }
    if (titles)
        dbe_out(DB_TEXT("\n"), STDOUT);
    return (0);
}

/* *************** Display opt - optional key bit map **************** */

int disp_opt(DB_TASK *task)
{
    short          rec;
    register int   fld;
    int            n, first, last, count, bytes, offset;
    DB_TCHAR       hex_line[50], ascii_line[50];
    char          *p;
    long           addr;

    /* Get record type */
    if ((rec = getrtype(slot.buffer, task)) == -1)
        return (BAD_TYPE);

    /* Count optional keys */
    first = task->record_table[rec].rt_fields;
    last = first + task->record_table[rec].rt_fdtot;
    for (count = 0, fld = first; fld < last; fld++)
        if (task->field_table[fld].fd_flags & OPTKEYMASK)
            count++;
    if (count == 0)
        return(0);

    /* Print title */
    if (titles)
        disp_str(DB_TEXT(""), dbe_getstr(M_DISPOK));

    /* Work out where optional key bit map starts */
    offset = task->record_table[rec].rt_flags & TIMESTAMPED ?
        RECHDRSIZE + 2 * sizeof(DB_ULONG) : RECHDRSIZE;

    /* Test for, and print stored keys */
    addr = slot.address + offset;
    p = slot.buffer + offset;
    fld = first;
    bytes = (count + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    ascii_line[0] = hex_line[0] = 0;

    while ((fld < last) || (bytes > 0))
    {
        if (fld < last)
        {
            if (task->field_table[fld].fd_flags & OPTKEYMASK)
            {
                /* Found on optional key */
                if (testopt(fld, slot.buffer, task))
                {
                    /* Found a stored optional key - will it fit on line */
                    if (vtstrlen(ascii_line) + vtstrlen(task->field_names[fld]) + 3
                         > ASCII_BYTES)
                    {
                        /* 3 for commas & spaces */
                        vtstrcat(ascii_line, DB_TEXT(","));
                        if (bytes > 0)
                        {
                            n = do_hex(addr, p, hex_line, bytes);
                            p += n;
                            bytes -= n;
                            addr = -1L;
                        }
                        else
                        {
                            hex_line[0] = 0;
                        }
                        disp_str(hex_line, ascii_line);
                        ascii_line[0] = hex_line[0] = 0;
                    }

                    /* Append key name to line */
                    if (ascii_line[0])
                        vtstrcat(ascii_line, DB_TEXT(", "));
                    vtstrcat(ascii_line, task->field_names[fld]);
                }
            }
            fld++;
        }
        else
        {
            n = do_hex(addr, p, hex_line, bytes);
            p += n;
            bytes -= n;
            addr = -1L;
            disp_str(hex_line, ascii_line);
            ascii_line[0] = hex_line[0] = 0;
        }
    }
    if (ascii_line[0])
        disp_str(hex_line, ascii_line);
    if (titles)
        dbe_out(DB_TEXT("\n"), STDOUT);
    return(0);
}

/* **************** Display set - set owner pointers ***************** */

int disp_set(int setno, DB_TASK *task)
{
    register int   set;
    short          rec;
    SET_PTR        sp;
    DB_TCHAR       hex_line[50], ascii_line[50];
    char          *p;
    int            n, found, bytes, lineno, title;
    long           addr;
    short          fileF, fileL;
    DB_ULONG       slotF, slotL;

    /* Get record type */
    if ((rec = getrtype(slot.buffer, task)) == -1)
        return (BAD_TYPE);

    /* Print the sets */
    for (set = title = found = 0; (set < task->size_st) && (! found); set++)
    {
        if (task->set_table[set].st_own_rt == rec)
        {
            if (setno >= 0)
            {
                if (set != setno)
                    continue;
                found = 1;
            }
            bytes = (task->set_table[set].st_flags & TIMESTAMPED) ?
                SETPSIZE : SETPSIZE - sizeof(DB_ULONG);
            addr = slot.address + task->set_table[set].st_own_ptr;
            p = slot.buffer + task->set_table[set].st_own_ptr;
            memcpy(&sp, p, SETPSIZE);
            lineno = 0;
            while (bytes || (lineno < 3))
            {
                if (bytes)
                {
                    n = do_hex(addr, p, hex_line, bytes);
                    bytes -= n;
                    p += n;
                    addr = -1L;
                }
                else
                {
                    hex_line[0] = 0;
                }
                switch (lineno)
                {
                    case 0:
                        d_decode_dba(sp.first, &fileF, &slotF);
                        d_decode_dba(sp.last, &fileL, &slotL);
                        vstprintf(ascii_line, decimal ?
                            DB_TEXT("%ld [%d:%ld] [%d:%ld]") :
                            DB_TEXT("%lx [%x:%lx] [%x:%lx]"),
                            (long) (sp.total), fileF, slotF, fileL, slotL);
                        break;

                    case 1:
                        if (task->set_table[set].st_flags & TIMESTAMPED)
                        {
                            vstprintf(ascii_line, DB_TEXT("%06ld"), (long) (sp.timestamp));
                            break;
                        }
                        lineno++;
                        /* fall thru ? */

                    case 2:
                        vtstrcpy(ascii_line, task->set_names[set]);
                        break;

                    default:
                        ascii_line[0] = 0;
                        break;
                }
                lineno++;
                if (titles && !title)
                {
                    disp_str(DB_TEXT(""), dbe_getstr(M_DISPSO));
                    title = 1;
                }
                disp_str(hex_line, ascii_line);
            }
        }
    }
    if (title)
        dbe_out(DB_TEXT("\n"), STDOUT);
    return ((setno < 0 || found) ? 0 : ERR_OSET);
}

/* **************** Display mem - set member pointers **************** */

int disp_mem(int setno, DB_TASK *task)
{
    register int   set, mem;
    short          rec;
    MEM_PTR        mp;
    DB_TCHAR       hex_line[50], ascii_line[50];
    char          *p;
    int            n, found, first, last, bytes, lineno, title;
    long           addr;
    short          fileO, fileP, fileN;
    DB_ULONG       slotO, slotP, slotN;

    /* Get record type */
    if ((rec = getrtype(slot.buffer, task)) == -1)
        return (BAD_TYPE);

    for (set = title = found = 0; (set < task->size_st) && (! found); set++)
    {
        if (setno >= 0 && set != setno)
            continue;
        first = task->set_table[set].st_members;
        last = first + task->set_table[set].st_memtot;
        for (mem = first; mem < last && !found; mem++)
        {
            if (task->member_table[mem].mt_record == rec)
            {
                if (setno >= 0)
                    found = 1;
                bytes = MEMPSIZE;
                addr = slot.address + task->member_table[mem].mt_mem_ptr;
                p = slot.buffer + task->member_table[mem].mt_mem_ptr;
                memcpy(&mp, p, MEMPSIZE);
                lineno = 0;
                while (bytes || (lineno < 2))
                {
                    if (bytes)
                    {
                        n = do_hex(addr, p, hex_line, bytes);
                        bytes -= n;
                        p += n;
                        addr = -1L;
                    }
                    else
                    {
                        hex_line[0] = 0;
                    }
                    switch (lineno)
                    {
                        case 0:
                            d_decode_dba(mp.owner, &fileO, &slotO);
                            d_decode_dba(mp.prev, &fileP, &slotP);
                            d_decode_dba(mp.next, &fileN, &slotN);
                            vstprintf(ascii_line, decimal ?
                                DB_TEXT("[%d:%ld] [%d:%ld] [%d:%ld]") :
                                DB_TEXT("[%x:%lx] [%x:%lx] [%x:%lx]"),
                                fileO, slotO, fileP, slotP, fileN, slotN);
                            break;

                        case 1:
                            vtstrcpy(ascii_line, task->set_names[set]);
                            break;

                        default:
                            ascii_line[0] = 0;
                            break;
                    }
                    lineno++;
                    if (titles && !title)
                    {
                        disp_str(DB_TEXT(""), dbe_getstr(M_DISPSM));
                        title = 1;
                    }
                    disp_str(hex_line, ascii_line);
                }
            }
        }
    }
    if (title)
        dbe_out(DB_TEXT("\n"), STDOUT);
    return ((setno < 0 || found) ? 0 : ERR_MSET);
}

/* ******************** Display fld - data fields ******************** */

int disp_fld(int fldno, DB_TASK *task)
{
    register int   fld;
    short          rec;
    DB_TCHAR       hex_line[50], ascii_line[30 + NAMELEN], *ascii_ptr;
    char          *hex_ptr;
    int            n, found, first, last, hex_len;
    size_t         ascii_len;
    long           addr;

    static DB_TCHAR ascii_buff[1600];

    if (!fields)
        return (0);

    /* Get record type */
    if ((rec = getrtype(slot.buffer, task)) == -1)
        return (BAD_TYPE);

    /* Do fields */
    first = task->record_table[rec].rt_fields;
    last = first + task->record_table[rec].rt_fdtot;
    for (fld = first, found = 0; (fld < last) && (! found); fld++)
    {
        if (fldno >= 0)
        {
            if (fld != fldno)
                continue;
            found = 1;
        }
        if (task->field_table[fld].fd_flags & STRUCTFLD)
            continue;

        /* Work out offsets */
        addr = slot.address + task->field_table[fld].fd_ptr;
        hex_ptr = slot.buffer + task->field_table[fld].fd_ptr;
        ascii_len = hex_len = task->field_table[fld].fd_len;

        /* Store ascii representation of field, if it'll fit */
        if (ascii_len > sizeof(ascii_buff) / sizeof(DB_TCHAR))
        {
            ascii_buff[0] = 0;
            ascii_len = 0;
        }
        else
        {
            fldtotxt(fld, hex_ptr, ascii_buff, !decimal, task);
            ascii_len = vtstrlen(ascii_buff);
        }
        ascii_ptr = ascii_buff;

        /* Print field title */
        if (titles)
        {
            vtstrcpy(ascii_line, dbe_getstr(M_DISPFD));
            vtstrcat(ascii_line, task->field_names[fld]);
            disp_str(DB_TEXT(""), ascii_line);
        }

        /* Print field contents */
        while ((ascii_len > 0) || (hex_len > 0))
        {
            if (ascii_len > 0)
            {
                n = do_ascii(ascii_ptr, ascii_line, ascii_len);
                ascii_len -= n;
                ascii_ptr += n;
            }
            else
            {
                ascii_line[0] = 0;
            }
            if (hex_len > 0)
            {
                n = do_hex(addr, hex_ptr, hex_line, hex_len);
                hex_len -= n;
                hex_ptr += n;
                addr = -1L;
            }
            else
            {
                hex_line[0] = 0;
            }
            disp_str(hex_line, ascii_line);
        }
/*
        if ( titles )
            dbe_out( DB_TEXT("\n"), STDOUT );
*/
    }
    return ((fldno < 0 || found) ? 0 : ERR_RFLD);
}

/* ********************* Miscellaneous functions ********************* */

/* Display a line of hex & ascii output
*/
int disp_str(DB_TCHAR *hex_str, DB_TCHAR *ascii_str)
{
    DB_TCHAR line[LINELEN];

    vstprintf(line, DB_TEXT("%-45.45s  %s\n"), hex_str, ascii_str);
    dbe_out(line, STDOUT);

    return 0;
}


/* Convert byte string into hex representation, with file address
*/
int do_hex(long addr, char *in_str, DB_TCHAR *out_str, int len)
{
    int addrlen;

    if (len > HEX_BYTES)
        len = HEX_BYTES;
    if (addr >= 0L)
        vstprintf(out_str,
            decimal ? DB_TEXT("%06ld:   ") : DB_TEXT(" %05lx:   "), addr);
    else
        vtstrcpy(out_str, DB_TEXT("          "));
    if ((addrlen = vtstrlen(out_str)) > 13)
        out_str[addrlen - 3] = 0;
    else if (addrlen > 10)
        out_str[10] = 0;
    do_hexstr(in_str, out_str, len);
    return (len);
}


/* Convert byte string into hex representation, without file address
*/
DB_TCHAR *do_hexstr(char *in_str, DB_TCHAR *out_str, int len)
{
    int i;

    while (*out_str)
        out_str++;
    for (i = 0; i < len; i++)
    {
        vstprintf(out_str, DB_TEXT("%02x "), (0xFF & (int) in_str[i]));
        out_str += 3;
    }
    *out_str = 0;
    return (out_str);
}


/* Copy ascii string, checking length
*/
int do_ascii(DB_TCHAR *in_str, DB_TCHAR *out_str, int len)
{
    if (len > ASCII_BYTES)
        len = ASCII_BYTES;
    vtstrncpy(out_str, in_str, len);
    out_str[len] = 0;
    return (len);
}


/* Get record type from slot
*/
short getrtype(char *buffer, DB_TASK *task)
{
    short rec_type;

    memcpy(&rec_type, buffer, sizeof(short));
    if (rec_type < 0)
        rec_type = (short) (~rec_type);
    if (rec_type & RLBMASK)
        rec_type &= ~RLBMASK;

    if ((rec_type < 0) || (rec_type >= task->size_rt))
        rec_type = -1;      /* error condition */

    return (rec_type);
}


/* Test whether optional key has been stored
*/
int testopt(int field,   /* Field table index of optional key */
            char *rec,   /* Pointer to record */
            DB_TASK *task)
{
    int offset;                         /* offset to the bit map */
    int keyndx;                         /* index into bit map of this key */
    int byteno, bitno;                  /* position within bit map of this
                                         * key */

    /* calculate the position to the bit map */
    offset = task->record_table[task->field_table[field].fd_rec].rt_flags & TIMESTAMPED ?
        RECHDRSIZE + 2 * sizeof(long) : RECHDRSIZE;

    /* extract the index into the bit map of this key */
    keyndx = (((task->field_table[field].fd_flags & OPTKEYMASK) >> OPTKEYSHIFT)
             & OPTKEYNDX) - 1;
    if (keyndx < 0)
        return (0);

    /* determine which byte, and which bit within the byte */
    byteno = keyndx / BITS_PER_BYTE;
    bitno = keyndx % BITS_PER_BYTE;

    /* extract the bit */
    if (*(rec + offset + byteno) & (1 << (BITS_PER_BYTE - bitno - 1)))
        return (1);
    else
        return (0);
}


