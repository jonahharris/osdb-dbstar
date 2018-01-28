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

    dbe_show.c - DBEDIT, show command

    This file contains all the functions for the show command (i.e. to
    output information about the database structure).

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_str.h"
#include "dbe_err.h"
#include "dbe_io.h"
#include "dbe_ext.h"


int show(DBE_TOK *tokens, int *tokenptr, DB_TASK *task)
{
    int i, token, error;

    error = 0;
    token = *tokenptr + 1;

    /* If no argument was specified, show everything */
    if (tokens[token].parent != token - 1 || tokens[token].type == 0)
    {
        show_nft(task);
        show_nrt(-1, task);
        show_nst(task);
        show_nkt(task);
        show_nfd(task);
    }
    else
    {
        switch (tokens[token++].ival)
        {

            case K_FLD:
                if (tokens[token].parent == token - 1)
                {                          /* One field */
                    i = tokens[token++].ival;
                    show_fd(&i, 0, 0, task);
                }
                else
                {
                    show_nfd(task);         /* All fields */
                }
                break;

            case K_FILE:
                if (tokens[token].parent == token - 1)
                {                          /* One file */
                    i = (tokens[token].type == TYPE_FIL) ?
                        tokens[token].ival :
                        (int) tokens[token].lval;
                    if (i >= task->size_ft)
                        error = BAD_FILE;
                    else
                        show_ft(i, task);    /* All files */
                    token++;
                }
                else
                {
                    show_nft(task);
                }
                break;

            case K_KEY:
                if (tokens[token].parent == token - 1)
                {                          /* One key */
                    i = tokens[token++].ival;
                    show_fd(&i, 0, 1, task);
                }
                else
                {
                    show_nkt(task);         /* All keys */
                }
                break;

            case K_RECORD:
                if (tokens[token].parent == token - 1)      /* One record */
                    show_rt(tokens[token++].ival, task);
                else
                    show_nrt(-1, task);     /* All records */
                break;

            case K_SET:
                if (tokens[token].parent == token - 1)      /* One set */
                    show_st(tokens[token++].ival, task);
                else
                    show_nst(task);         /* All sets */
                break;
        }
    }

    *tokenptr = token;
    return (error);
}

/* ********************** Show files ********************************* */

/* Show comma separated list of all files in database
*/
int show_nft(DB_TASK *task)
{
    int i, j, len, comma;
    DB_TCHAR *p;

    dbe_out(dbe_getstr(M_SHOWFL), STDOUT);
    comma = 0;
    j = SCRWIDTH;
    for (i = 0; i < task->size_ft; i++)
    {
        len = vtstrlen(task->file_table[i].ft_name);
        if (len + 2 + j >= SCRWIDTH)
        {                                /* Add 2 for comma & space */
            if (comma)
                dbe_out(DB_TEXT(",\n"), STDOUT);
            dbe_out(p = dbe_getstr(M_TAB), STDOUT);
            j = vtstrlen(p);
        }
        else if (comma)
        {
            dbe_out(DB_TEXT(", "), STDOUT);
            j += 2;
        }
        dbe_out(task->file_table[i].ft_name, STDOUT);
        comma = 1;
        j += len;
    }
    dbe_out(DB_TEXT("\n"), STDOUT);
    return 0;
}


/* Show details for one file
*/
int show_ft(int index, DB_TASK *task)
{
    int i;

    if (task->file_table[index].ft_type == 'd')
        show_nrt(index, task);
    else
    {
        if (task->file_table[index].ft_type == 'k')
        {
            dbe_out(dbe_getstr(M_SHOWKL), STDOUT);
            for (i = 0; i < task->size_fd;)
            {
                if (task->field_table[i].fd_keyfile == index)
                    show_fd(&i, 0, 1, task);
                else
                    i++;
            }
        }
    }
    return 0;
}

/* ********************** Show records ******************************* */

/* Show comma separated list of all records in database
*/
int show_nrt(
    int file_no,                        /* File containing records to  */
    DB_TASK *task)                      /* be shown (-1 for all files) */
{
    int i, j, len, comma;
    DB_TCHAR *p;

    dbe_out(dbe_getstr(M_SHOWRD), STDOUT);
    comma = 0;
    j = SCRWIDTH;
    for (i = 0; i < task->size_rt; i++)
    {
        if (file_no >= 0 && task->record_table[i].rt_file != file_no)
            continue;
        len = vtstrlen(task->record_names[i]);
        if (len + 2 + j >= SCRWIDTH)
        {                                /* Add 2 for comma & space */
            if (comma)
                dbe_out(DB_TEXT(",\n"), STDOUT);
            dbe_out(p = dbe_getstr(M_TAB), STDOUT);
            j = vtstrlen(p);
        }
        else
        {
            if (comma)
            {
                dbe_out(DB_TEXT(", "), STDOUT);
                j += 2;
            }
        }
        dbe_out(task->record_names[i], STDOUT);
        comma = 1;
        j += len;
    }
    dbe_out(DB_TEXT("\n"), STDOUT);

    return 0;
}


/* Show details for one record
*/
int show_rt(int index, DB_TASK *task)
{
    DB_TCHAR buffer[LINELEN];
    int i, first, last, n, comma;

    /* Record Type */

    vstprintf(buffer, DB_TEXT("%s%s (%d)"),
        dbe_getstr(M_SHOWRT), task->record_names[index], index);
    dbe_out(buffer, STDOUT);

    /* Contained in file... */

    vtstrcpy(buffer, dbe_getstr(M_SHOWRC));
    vtstrcat(buffer, task->file_table[task->record_table[index].rt_file].ft_name);
    dbe_out(buffer, STDOUT);

    /* Record Length */

    n = task->record_table[index].rt_len;
    vstprintf(buffer, decimal ? DB_TEXT("%s%d") : DB_TEXT("%s%x"),
        dbe_getstr(M_SHOWRL), n);
    dbe_out(buffer, STDOUT);

    /* Offset to Data */

    n = task->record_table[index].rt_data;
    vstprintf(buffer, decimal ? DB_TEXT("%s%d") : DB_TEXT("%s%x"),
        dbe_getstr(M_SHOWRO), n);
    dbe_out(buffer, STDOUT);

    /* Flags */

    vtstrcpy(buffer, dbe_getstr(M_SHOWRG));
    comma = 0;
    if (task->record_table[index].rt_flags & TIMESTAMPED)
    {
        vtstrcat(buffer, dbe_getstr(M_FLAGRT));
        comma = 1;
    }
    if (task->record_table[index].rt_flags & STATIC)
    {
        if (comma)
            vtstrcat(buffer, DB_TEXT(", "));
        vtstrcat(buffer, dbe_getstr(M_FLAGRS));
        comma = 1;
    }
    if (task->record_table[index].rt_flags & COMKEYED)
    {
        if (comma)
            vtstrcat(buffer, DB_TEXT(", "));
        vtstrcat(buffer, dbe_getstr(M_FLAGRC));
    }
    vtstrcat(buffer, DB_TEXT("\n"));
    dbe_out(buffer, STDOUT);

    /* Fields */

    vtstrcpy(buffer, dbe_getstr(M_SHOWRF));
    dbe_out(buffer, STDOUT);
    first = task->record_table[index].rt_fields;
    last = first + task->record_table[index].rt_fdtot;
    for (i = first; i < last;)
        show_fd(&i, 0, 0, task);

    return 0;
}

/* ********************** Show sets ********************************** */

/* Show comma separated list of all sets in database
*/
int show_nst(DB_TASK *task)
{
    int i, j, len, comma;
    DB_TCHAR *p;

    dbe_out(dbe_getstr(M_SHOWSL), STDOUT);
    comma = 0;
    j = SCRWIDTH;
    for (i = 0; i < task->size_st; i++)
    {
        len = vtstrlen(task->set_names[i]);
        if (len + 2 + j >= SCRWIDTH)
        {                                /* Add 2 for comma & space */
            if (comma)
                dbe_out(DB_TEXT(",\n"), STDOUT);
            dbe_out(p = dbe_getstr(M_TAB), STDOUT);
            j = vtstrlen(p);
        }
        else
        {
            if (comma)
            {
                dbe_out(DB_TEXT(", "), STDOUT);
                j += 2;
            }
        }
        dbe_out(task->set_names[i], STDOUT);
        comma = 1;
        j += len;
    }
    dbe_out(DB_TEXT("\n"), STDOUT);
    
    return 0;
}


/* Show details for one set
*/
int show_st(int index, DB_TASK *task)
{
    DB_TCHAR buffer[LINELEN];
    int i, n, first, last, order = 0, fields, comma;

    /* Set Type */

    vstprintf(buffer, DB_TEXT("%s%s (%d)"),
        dbe_getstr(M_SHOWST), task->set_names[index], index);
    dbe_out(buffer, STDOUT);

    /* Order */

    vtstrcpy(buffer, dbe_getstr(M_SHOWSD));
    fields = 0;
    switch (task->set_table[index].st_order)
    {
        case DB_TEXT('f'):
            order = M_SETFST;
            break;
        case DB_TEXT('l'):
            order = M_SETLST;
            break;
        case DB_TEXT('n'):
            order = M_SETNXT;
            break;
        case DB_TEXT('a'):
            order = M_SETASC;
            fields = 1;
            break;
        case DB_TEXT('d'):
            order = M_SETDSC;
            fields = 1;
            break;
    }
    vtstrcat(buffer, dbe_getstr(order));
    dbe_out(buffer, STDOUT);
    if (fields)
    {
        vtstrcpy(buffer, dbe_getstr(M_TAB));
        vtstrcat(buffer, dbe_getstr(M_SHOWSF));
        comma = 0;
        for (i = 0; i < task->size_srt; i++)
        {
            if (task->sort_table[i].se_set == index)
            {
                if (comma)
                    vtstrcat(buffer, DB_TEXT(", "));
                vtstrcat(buffer, task->field_names[task->sort_table[i].se_fld]);
                comma = 1;
            }
        }
        dbe_out(buffer, STDOUT);
    }

    /* Owner */

    n = task->set_table[index].st_own_rt;
    vstprintf(buffer, DB_TEXT("%s%s (%d)"),
        dbe_getstr(M_SHOWSO), task->record_names[n], n);
    dbe_out(buffer, STDOUT);

    /* Members */

    vtstrcpy(buffer, dbe_getstr(M_SHOWSM));
    first = task->set_table[index].st_members;
    last = first + task->set_table[index].st_memtot;
    comma = 0;
    for (i = first; i < last; i++)
    {
        n = task->member_table[i].mt_record;
        vstprintf(buffer + vtstrlen(buffer),
            DB_TEXT("%s (%d)"), task->record_names[n], n);
        if (i != last - 1)
            vtstrcat(buffer, DB_TEXT(", "));
    }
    dbe_out(buffer, STDOUT);

    /* Flags */

    vtstrcpy(buffer, dbe_getstr(M_SHOWSG));
    if (task->set_table[index].st_flags & TIMESTAMPED)
        vtstrcat(buffer, dbe_getstr(M_FLAGST));

    vtstrcat(buffer, DB_TEXT("\n"));
    dbe_out(buffer, STDOUT);

    return 0;
}

/* ********************** Show keys ********************************** */

/* Show list of keys in database, in same format as normal fields
*/
int show_nkt(DB_TASK *task)
{
    int i;

    dbe_out(dbe_getstr(M_SHOWKL), STDOUT);
    for (i = 0; i < task->size_fd;)
        show_fd(&i, 0, 1, task);

    return 0;
}

/* ********************** Show fields ******************************** */

/* Show list of fields in database
*/
int show_nfd(DB_TASK *task)
{
    int i;

    dbe_out(dbe_getstr(M_SHOWRF), STDOUT);
    for (i = 0; i < task->size_fd;)
        show_fd(&i, 0, 0, task);

    return 0;
}


/* Show one field
*/
int show_fd(
    int *index_ptr,      /* Index in field table */
    int level,           /* Recursion level, for tabs */
    int keys,            /* Flag - only show keys */
    DB_TASK *task)
{
    FIELD_ENTRY *fd_ptr;
    DB_TCHAR buffer[LINELEN], dimstr[16];
    int i, j, n, index, comma, optkey, compound;
    short dim, flags;

    n = (index = *index_ptr) + 1;
    fd_ptr = &task->field_table[index];
    flags = fd_ptr->fd_flags;

    /* Just key fields ? */

    if (keys && fd_ptr->fd_key == 'n')
    {
        *index_ptr = n;
        return(0);
    }

    /* Spacing */

    vtstrcpy(buffer, dbe_getstr(M_TAB));
    for (i = 0; i < level; i++)
        vtstrcat(buffer, dbe_getstr(M_TAB));

    /* Key fields - compound / optional / unique */

    if (((compound = (fd_ptr->fd_type == 'k')) != 0) || fd_ptr->fd_key != 'n')
    {
        if (compound)
            vtstrcat(buffer, dbe_getstr(M_COMP));
        if (((flags & OPTKEYMASK) >> OPTKEYSHIFT) & OPTKEYNDX)
            vtstrcat(buffer, dbe_getstr(M_OPT));
        if (fd_ptr->fd_key == 'u')
            vtstrcat(buffer, dbe_getstr(M_UNIQ));
        vtstrcat(buffer, dbe_getstr(M_KEY));
    }

    /* Field type */

    if (flags & UNSIGNEDFLD)
        vtstrcat(buffer, dbe_getstr(M_UNSIGN));
    switch (fd_ptr->fd_type)
    {
        case 'c':
            vtstrcat(buffer, dbe_getstr(M_CHAR));
            break;
        case 'C':
            vtstrcat(buffer, dbe_getstr(M_WIDECHAR));
            break;
        case 's':
            vtstrcat(buffer, dbe_getstr(M_SHORT));
            break;
        case 'i':
            vtstrcat(buffer, dbe_getstr(M_INT));
            break;
        case 'l':
            vtstrcat(buffer, dbe_getstr(M_LONG));
            break;
        case 'f':
            vtstrcat(buffer, dbe_getstr(M_FLOAT));
            break;
        case 'F':
            vtstrcat(buffer, dbe_getstr(M_DOUBLE));
            break;
        case 'd':
            vtstrcat(buffer, dbe_getstr(M_ADDR));
            break;

        case 'g':
            vtstrcat(buffer, dbe_getstr(M_STRUCT));
            vtstrcat(buffer, DB_TEXT("\n"));
            dbe_out(buffer, STDOUT);
            while ((task->field_table[n].fd_flags & STRUCTFLD) && (n < task->size_fd))
                show_fd(&n, level + 1, 0, task);
            vtstrcpy(buffer, dbe_getstr(M_TAB));
            vtstrcat(buffer, DB_TEXT("} "));
            break;
    }

    /* Field name */

    vtstrcat(buffer, task->field_names[index]);

    /* Array dimensions */

    for (i = 0; i < MAXDIMS; i++)
    {
        if (!(dim = fd_ptr->fd_dim[i]))
            break;
        vstprintf(dimstr, DB_TEXT("[%d]"), dim);
        vtstrcat(buffer, dimstr);
    }

    /* Compound keys */

    if (compound)
        vtstrcat(buffer, DB_TEXT(" {"));
    vtstrcat(buffer, DB_TEXT("\n"));
    if (compound)
    {
        dbe_out(buffer, STDOUT);
        for (j = fd_ptr->fd_ptr; task->key_table[j].kt_key == index; j++)
        {
            vtstrcpy(buffer, dbe_getstr(M_TAB));
            for (i = 0; i < level + 1; i++)
                vtstrcat(buffer, dbe_getstr(M_TAB));
            vtstrcat(buffer, task->field_names[task->key_table[j].kt_field]);
            if (task->key_table[j].kt_sort == 'a')
                vtstrcat(buffer, dbe_getstr(M_ASC));
            else
                vtstrcat(buffer, dbe_getstr(M_DESC));
            vtstrcat(buffer, DB_TEXT("\n"));
            dbe_out(buffer, STDOUT);
        }
        vtstrcpy(buffer, dbe_getstr(M_TAB));
        for (i = 0; i < level; i++)
            vtstrcat(buffer, dbe_getstr(M_TAB));
        vtstrcat(buffer, DB_TEXT("}\n"));
    }

    dbe_out(buffer, STDOUT);

    /* Flags */

    if (flags)
    {
        comma = 0;
        vtstrcpy(buffer, dbe_getstr(M_TAB));
        for (i = 0; i < level; i++)
            vtstrcat(buffer, dbe_getstr(M_TAB));
        vtstrcat(buffer, dbe_getstr(M_SHOWFF));
        if (flags & SORTFLD)
        {
            vtstrcat(buffer, dbe_getstr(M_FLAGFF));
            comma = 1;
        }
        if (flags & STRUCTFLD)
        {
            if (comma)
                vtstrcat(buffer, DB_TEXT(", "));
            vtstrcat(buffer, dbe_getstr(M_FLAGFS));
            comma = 1;
        }
        if (flags & UNSIGNEDFLD)
        {
            if (comma)
                vtstrcat(buffer, DB_TEXT(", "));
            vtstrcat(buffer, dbe_getstr(M_FLAGFU));
            comma = 1;
        }
        if (flags & COMKEYED)
        {
            if (comma)
                vtstrcat(buffer, DB_TEXT(", "));
            vtstrcat(buffer, dbe_getstr(M_FLAGFC));
            comma = 1;
        }
        if ((optkey = ((flags & OPTKEYMASK) >> OPTKEYSHIFT) & OPTKEYNDX) != 0)
        {
            optkey--;
            if (comma)
                vtstrcat(buffer, DB_TEXT(", "));
            vstprintf(buffer + vtstrlen(buffer), DB_TEXT("%s%d"),
                dbe_getstr(M_FLAGFO), optkey);
        }
        vtstrcat(buffer, DB_TEXT("\n"));
        dbe_out(buffer, STDOUT);
    }

    *index_ptr = n;

    return 0;
}


