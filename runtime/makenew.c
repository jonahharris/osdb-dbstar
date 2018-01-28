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
    Set the value of a key field
*/
int INTERNAL_FCN dsetkey(long field, const void *fldvalue, DB_TASK *task,
                        int dbn)
{
    struct sk *sk_ptr;
    struct sk *sk_p;
    int fld, rec;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    if (fld_ptr->fd_key == 'n')
        return (dberr(S_NOTKEY));

    ll_access(&task->sk_list);
    while ((sk_ptr = (struct sk *) ll_next(&task->sk_list)) != NULL)
    {
        if (sk_ptr->sk_fld == fld)
        {
            if ((fld_ptr->fd_type != CHARACTER && fld_ptr->fd_type != WIDECHAR) ||
                fld_ptr->fd_dim[1])
                memcpy(sk_ptr->sk_val, fldvalue, fld_ptr->fd_len);
            else if (fld_ptr->fd_dim[0])
            {
                if (fld_ptr->fd_type == WIDECHAR)
                    wcsncpy((wchar_t *)sk_ptr->sk_val, (wchar_t *)fldvalue,
                            fld_ptr->fd_len / sizeof(wchar_t));
                else
                    strncpy(sk_ptr->sk_val, fldvalue, fld_ptr->fd_len);
            }
            else
            {
                if (fld_ptr->fd_type == WIDECHAR) * 
                    ((wchar_t *)(sk_ptr->sk_val)) = *((wchar_t *) fldvalue)
                    ;
                else
                    *(sk_ptr->sk_val) = *((char *) fldvalue);
            }

            ll_deaccess(&task->sk_list);
            return (task->db_status);
        }
    }

    /* need to allocate a slot for a new fld */
    sk_p = (struct sk *) psp_getMemory(sizeof(struct sk), 0);
    if (sk_p == NULL)
        return (dberr(S_NOMEMORY));

    if (ll_prepend(&task->sk_list, (char *) sk_p, task) != S_OKAY)
        return (task->db_status);

    sk_p->sk_fld = (short) fld;
    sk_p->sk_val = psp_getMemory(fld_ptr->fd_len + 1, 0);
    if (sk_p->sk_val == NULL)
        return (dberr(S_NOMEMORY));

    if ((fld_ptr->fd_type != CHARACTER && fld_ptr->fd_type != WIDECHAR) ||
        fld_ptr->fd_dim[1])
        memcpy(sk_p->sk_val, fldvalue, fld_ptr->fd_len);
    else if (fld_ptr->fd_dim[0])
    {
        if (fld_ptr->fd_type == WIDECHAR)
            wcsncpy((wchar_t *)sk_p->sk_val, (wchar_t *)fldvalue,
                    fld_ptr->fd_len / sizeof(wchar_t));
        else
            strncpy(sk_p->sk_val, fldvalue, fld_ptr->fd_len);
    }
    else
    {
        if (fld_ptr->fd_type == WIDECHAR)
            *((wchar_t *)(sk_p->sk_val)) = *((wchar_t *) fldvalue);
        else
            *(sk_p->sk_val) = *((char *) fldvalue);
    }

    ll_deaccess(&task->sk_list);

    return (task->db_status);
}



/* ======================================================================
    Free the memory allocated for the task->sk_list
*/
int INTERNAL_FCN sk_free(DB_TASK *task)
{
    struct sk *sk_ptr;

    ll_access(&task->sk_list);
    while ((sk_ptr = (struct sk *) ll_next(&task->sk_list)) != NULL)
    {
        psp_freeMemory(sk_ptr->sk_val, 0);
        psp_freeMemory(sk_ptr, 0);
    }

    ll_deaccess(&task->sk_list);
    ll_free(&task->sk_list, task);

    return task->db_status;
}


/* ======================================================================
    Create a new empty record
*/
int INTERNAL_FCN dmakenew(int nrec, DB_TASK *task, int dbn)
{
    DB_ULONG      timestamp;
    SET_PTR       cosp;          /* current owner's set pointer */
    SET_ENTRY    *set_ptr;
    int           set, iset;
    DB_ADDR       db_addr;
    short         recnum, fld;
    FILE_NO       file;
    F_ADDR        rec_addr;
    char         *ptr;
    struct sk    *sk_ptr;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY  *fld_ptr;
    int           fldtot, stat;
    
    if (nrec_check(nrec, &nrec, &rec_ptr, task) != S_OKAY)
        return (task->db_status);

    if (rec_ptr->rt_fdtot == -1)
        return (S_DELSYS);

    recnum = (short) NUM2EXT(nrec, rt_offset);

    if (rec_ptr->rt_flags & COMKEYED)
        return (dberr(S_COMKEY));

    /* check for duplicate keys */
    db_addr = task->curr_rec;
    for (fld = rec_ptr->rt_fields, fldtot = fld + rec_ptr->rt_fdtot,
         fld_ptr = &task->field_table[fld]; fld < fldtot; ++fld, ++fld_ptr)
    {
        if (fld_ptr->fd_key == UNIQUE)
        {
            /* locate the key value in the set_key table */
            ll_access(&task->sk_list);
            while ((sk_ptr = (struct sk *) ll_next(&task->sk_list)) != NULL &&
                    sk_ptr->sk_fld != fld)
                ; /* NOP */

            if (sk_ptr == NULL)
            {
                ll_deaccess(&task->sk_list);
                return (dberr(S_KEYREQD));
            }

            dkeyfind(FLDMARK * (long) recnum + (long) (fld - rec_ptr->rt_fields),
                     sk_ptr->sk_val, task, dbn);
            task->curr_rec = db_addr;
            ll_deaccess(&task->sk_list);
            if (task->db_status == S_OKAY)
                return (task->db_status = S_DUPLICATE);
            else if (task->db_status == S_NOTFOUND)
                task->db_status = S_OKAY;
        }
    }

    /* pull out the file number */
    file = rec_ptr->rt_file;

    /* select a record pointer to use */
    if (dio_pzalloc(file, &rec_addr, task) != S_OKAY)
        return (task->db_status);

    ENCODE_DBA(NUM2EXT(file, ft_offset), rec_addr, &db_addr);

    /* read record */
    if (dio_read(db_addr, &ptr, PGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* zero fill the record */
    memset(ptr, 0, rec_ptr->rt_len);

    /* place the record number and db_addr at the start of the record */
    memcpy(ptr, &recnum, sizeof(short));
    memcpy(ptr + sizeof(short), &db_addr, DB_ADDR_SIZE);

    /* check for timestamp */
    if (rec_ptr->rt_flags & TIMESTAMPED)
    {
        timestamp = dio_pzgetts(file, task);
        memcpy(ptr + RECCRTIME, &timestamp, sizeof(long));
        memcpy(ptr + RECUPTIME, &timestamp, sizeof(long));
    }
    else
        timestamp = 0L;

    if (task->db_tssets)
    {
        memset(&cosp, '\0', sizeof(cosp));
        cosp.timestamp = dio_pzgetts(file, task);
        for ( set = 0, iset = ORIGIN(st_offset), set_ptr = &task->set_table[iset];
              set < TABLE_SIZE(Size_st); ++set, ++set_ptr, ++iset)
        {
            if (set_ptr->st_flags & TIMESTAMPED && set_ptr->st_own_rt == nrec)
            {
                if (r_pset(iset, ptr, (char *) &cosp, task) != S_OKAY)
                    return (task->db_status);
            }
        }

    }

    /* for each keyed field, enter the key value into the key file */
    for ( fld = rec_ptr->rt_fields, fldtot = fld + rec_ptr->rt_fdtot,
          fld_ptr = &task->field_table[fld]; fld < fldtot; ++fld, ++fld_ptr)
    {
        if (fld_ptr->fd_key != 'n' && !(fld_ptr->fd_flags & OPTKEYMASK))
        {
            /* locate the key value in the set_key table */
            ll_access(&task->sk_list);
            sk_ptr = (struct sk *) ll_first(&task->sk_list);
            while (sk_ptr != NULL)
            {
                if (sk_ptr->sk_fld == fld)
                {
                    stat = key_insert(fld, sk_ptr->sk_val, db_addr, task);
                    if (stat != S_OKAY)
                    {
                        if (dio_write(db_addr, PGFREE, task) != S_OKAY)
                            return (task->db_status);

                        r_delrec((short) nrec, db_addr, task);
                        ll_deaccess(&task->sk_list);
                        return (task->db_status = stat);
                    }

                    if ((fld_ptr->fd_type != CHARACTER &&
                         fld_ptr->fd_type != WIDECHAR) || fld_ptr->fd_dim[0])
                    {
                        memcpy(ptr + fld_ptr->fd_ptr, sk_ptr->sk_val,
                               fld_ptr->fd_len);
                    }
                    else
                    {
                        if (fld_ptr->fd_type == WIDECHAR)
                            wcsncpy((wchar_t *)(ptr + fld_ptr->fd_ptr),
                                    (wchar_t *)sk_ptr->sk_val,
                                    fld_ptr->fd_len / sizeof(wchar_t));
                        else
                            strncpy(ptr + fld_ptr->fd_ptr, sk_ptr->sk_val,
                                    fld_ptr->fd_len);
                    }

                    break;
                }

                sk_ptr = (struct sk *) ll_next(&task->sk_list);
            }

            ll_deaccess(&task->sk_list);
            if (sk_ptr == NULL)
                return (dberr(S_KEYREQD));
        }
    }

    if (dio_write(db_addr, PGFREE, task) == S_OKAY)
    {
        task->curr_rec = db_addr;

        if (task->db_tsrecs)
            task->cr_time = timestamp;
    }

    return (task->db_status);
}


