/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbimp utility                                     *
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
#include "impdef.h"
#include "impvar.h"

#define DBIMP
#include "getnames.h"


struct spec *new_spec(int type)
{
    struct spec *s;

    if ((s = (struct spec *) psp_cGetMemory(sizeof(struct spec), 0)) == NULL)
    {
        dbimp_abort(DB_TEXT("out of memory"));
	return NULL;
    }

    s->sp_type = type;
    s->sp_next = NULL;

    return (s);
}

struct endloop *new_end(struct spec *loopspec)
{
    struct endloop *e;

    e = (struct endloop *) psp_cGetMemory(sizeof(struct endloop), 0);
    if (e == NULL)
        dbimp_abort(DB_TEXT("out of memory"));
    else
        e->e_ptr = loopspec;
    return (e);
}


struct loop *new_loop(DB_TCHAR *fname)
{
    struct loop *l;

    l = (struct loop *) psp_cGetMemory(sizeof(struct loop), 0);
    if (l == NULL)
    {
        dbimp_abort(DB_TEXT("out of memory"));
        return (NULL);
    }
    if (vtstrlen(fname) >= FILELEN)
    {
        DB_TCHAR msg[80];
        vstprintf(msg, DB_TEXT("file name %s is too long"), fname);
        ddwarning(msg);
    }
    vtstrncpy(l->l_fname, fname, FILELEN);
    l->l_fp = NULL;
    return (l);
}


struct rec *new_rec(int h_type, struct handling *h_list, struct fld *f_list)
{
    struct rec *r;

    r = (struct rec *) psp_cGetMemory(sizeof(struct rec), 0);
    if (r == NULL)
    {
        dbimp_abort(DB_TEXT("out of memory"));
    }
    else
    {
        r->rec_htype = h_type;
        r->rec_hptr = h_list;
        r->rec_fldptr = f_list;
    }
    return (r);
}


struct handling *new_hand(DB_TCHAR *filename, int fldname)
{
    struct handling *h;

    h = (struct handling *) psp_cGetMemory(sizeof(struct handling), 0);
    if (h == NULL)
    {
        dbimp_abort(DB_TEXT("out of memory"));
    }
    else
    {
        vtstrcpy(h->h_file, filename);
        h->h_name = fldname;
    }
    return (h);
}


struct con *new_con()
{
    struct con *c;

    c = (struct con *) psp_cGetMemory(sizeof(struct con), 0);
    if (c == NULL)
        dbimp_abort(DB_TEXT("out of memory"));
    return (c);
}


/* create a fld struct, compute the offset if indexed */
struct fld *new_fld(DB_TCHAR *str_name, int *strdim, int strdims,
                    DB_TCHAR *f_name, int *elemdim, int elemdims)
{
    register int i;
    struct fld *f;
    int fd, sd;
    int dims, size, idx, nelem;
    DB_TASK *task = imp_g.dbtask;
    DB_TCHAR msg[80];

    f = (struct fld *) psp_cGetMemory(sizeof(struct fld), 0);
    if (f == NULL)
    {
        dbimp_abort(DB_TEXT("out of memory"));
        return (NULL);
    }
    f->fld_next = NULL;

    /* locate the field name in the schema */
    fd = getfld(f_name, imp_g.recndx, task);
    if (fd < 0)
    {
        vstprintf(msg, DB_TEXT("field '%s' not found in record"), f_name);
        ddwarning(msg);
        return (f);
    }
    f->fld_ndx = fd;

    if (vtstrlen(str_name))
    {
        sd = getfld(str_name, imp_g.recndx, task);
        if (sd < 0)
        {
            vstprintf(msg, DB_TEXT("field '%s' not found in record"), str_name);
            ddwarning(msg);
            return (f);
        }

        /* make sure it is structured */
        if (task->field_table[sd].fd_type != GROUPED)
        {
            vstprintf(msg, DB_TEXT("field %s is not a structured type"), str_name);
            ddwarning(msg);
            return (f);
        }

        /* check the range and number of subscripts */
        for (dims = 0; dims < MAXDIMS && task->field_table[sd].fd_dim[dims]; dims++)
            ;
        if (dims < strdims)
        {
            vstprintf(msg, DB_TEXT("too many subscripts in structure field %s"), str_name);
            ddwarning(msg);
            return (f);
        }
        if (dims > strdims)
        {
            vstprintf(msg, DB_TEXT("too few subscripts in structure field %s"), str_name);
            ddwarning(msg);
            return (f);
        }
        for (i = 0; i < dims && i < strdims; i++)
            if (strdim[i] >= task->field_table[sd].fd_dim[i])
            {
                vstprintf(msg, DB_TEXT("subscript out of range in structure field %s"),
                             str_name);
                ddwarning(msg);
                return (f);
            }

        /* compute an offset from the start of the data area to the starting
         * byte of this structure element */

        /* determine how many elements will be moved */
        for (nelem = 1, i = 0; i < dims; i++)
            nelem *= task->field_table[sd].fd_dim[i];

        size = task->field_table[sd].fd_len / nelem;

        /* find the starting element */
        vec_idx(dims, task->field_table[sd].fd_dim, strdim, &idx);

        f->fld_offset = task->field_table[fd].fd_ptr -
            task->record_table[task->field_table[sd].fd_rec].rt_data +
            idx * size;

    }
    else
        f->fld_offset = task->field_table[fd].fd_ptr -
            task->record_table[task->field_table[fd].fd_rec].rt_data;

    /* check the range and number of subscripts */
    for (dims = 0; dims < MAXDIMS && task->field_table[fd].fd_dim[dims]; dims++)
        ;
    if (dims < elemdims)
    {
        vstprintf(msg, DB_TEXT("too many subscripts in field %s"), f_name);
        ddwarning(msg);
        return (f);
    }
    for (i = 0; i < dims && i < elemdims; i++)
        if (elemdim[i] >= task->field_table[fd].fd_dim[i])
        {
            vstprintf(msg, DB_TEXT("subscript out of range in field %s"), f_name);
            ddwarning(msg);
            return (f);
        }

    /* check for an unusual byte array condition */
    if (elemdims && task->field_table[fd].fd_dim[dims - 1] == 1 && dims == elemdims + 1)
    {
        ddwarning(DB_TEXT("illegal usage of binary array"));
        return (f);
    }

    /* save subscript information in field structure */
    f->fld_dims = elemdims;
    for (i = 0; i < elemdims; i++)
        f->fld_dim[i] = elemdim[i];

    return (f);
}


