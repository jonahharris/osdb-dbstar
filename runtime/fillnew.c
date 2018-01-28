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
    Create and fill a new record
*/
int INTERNAL_FCN dfillnew(
    int nrec,                   /* record number */
    const void *recval,         /* record value */
    DB_TASK *task,
    int dbn)                    /* database number */
{
    DB_ULONG      timestamp;
    SET_PTR       cosp;         /* current owner's set pointer */
    SET_ENTRY    *set_ptr;
    int           set, iset;
    DB_ADDR       db_addr;
    short         recnum;
    FILE_NO       file;
    F_ADDR        rec_addr;
    char         *ptr;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY  *fld_ptr;
    int           fld, fldtot;
    char          key[MAXKEYSIZE];
    int           stat;

    if (nrec_check(nrec, &nrec, &rec_ptr, task) != S_OKAY)
        return (task->db_status);

    if (rec_ptr->rt_fdtot == -1)
        return (dberr(S_DELSYS));

    if (rec_ptr->rt_fdtot && !recval)
        return (dberr(S_INVPTR));

    recnum = (short) NUM2EXT(nrec, rt_offset);

    /* check for duplicate keys */
    db_addr = task->curr_rec;
    for ( fld = rec_ptr->rt_fields, fldtot = fld + rec_ptr->rt_fdtot,
          fld_ptr = &task->field_table[fld];
          (fld < fldtot) || ((fld < task->size_fd) && (fld_ptr->fd_type == COMKEY));
          ++fld, ++fld_ptr)
    {
        if ((fld_ptr->fd_key == UNIQUE) && !(fld_ptr->fd_flags & OPTKEYMASK))
        {
            if (fld_ptr->fd_type != COMKEY)
                ptr = (char *) recval + fld_ptr->fd_ptr - rec_ptr->rt_data;
            else
                key_bldcom(fld, (char *) recval, ptr = key, FALSE, task);  /* Don't complement */

            dkeyfind(FLDMARK * (long) recnum + (fld - rec_ptr->rt_fields), ptr,
                     task, dbn);

            task->curr_rec = db_addr;
            if (task->db_status == S_OKAY)
                return (task->db_status = S_DUPLICATE);
            else if (task->db_status == S_NOTFOUND)
                task->db_status = S_OKAY;
        }
    }

    file = rec_ptr->rt_file;         /* pull out the file number */

    /* select a record pointer to use */
    if (dio_pzalloc(file, &rec_addr, task) != S_OKAY)
        return (task->db_status);

    ENCODE_DBA(NUM2EXT(file, ft_offset), rec_addr, &db_addr);

    /* read record */
    if (dio_read(db_addr, (char **) &ptr, PGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* zero fill the record */
    memset(ptr, '\0', rec_ptr->rt_len);

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
              set < TABLE_SIZE(Size_st);
              ++set, ++set_ptr, ++iset)
        {
            if ( (set_ptr->st_flags & TIMESTAMPED) &&
                 (set_ptr->st_own_rt == nrec))
            {
                if (r_pset(iset, ptr, (char *) &cosp, task) != S_OKAY)
                    return (task->db_status);
            }
        }

    }

    /* copy the record value into place */
    if (rec_ptr->rt_fdtot)
    {
        /* if the last field is a character string, only copy thru
           null terminator
        */
        short rlen = rec_ptr->rt_len - rec_ptr->rt_data;
        fld_ptr = &task->field_table[rec_ptr->rt_fields + rec_ptr->rt_fdtot - 1];
        if (fld_ptr->fd_type == CHARACTER && fld_ptr->fd_dim[1] == 0)
            rlen -= fld_ptr->fd_len;

        memcpy(ptr + rec_ptr->rt_data, recval, rlen);
        if (fld_ptr->fd_type == CHARACTER && fld_ptr->fd_dim[1] == 0)
            strncpy(ptr + fld_ptr->fd_ptr, (char *)recval + rlen, fld_ptr->fd_len);
    }

    /* for each keyed field, enter the key value into the key file */
    for ( fld = rec_ptr->rt_fields, fldtot = fld + rec_ptr->rt_fdtot,
          fld_ptr = &task->field_table[fld];
          (NUM2EXT(fld, fd_offset) < DB_REF(Size_fd)) &&
          (  (fld < fldtot) ||
             ((fld < task->size_fd) && (fld_ptr->fd_type == COMKEY)) );
          ++fld, ++fld_ptr)
    {
        if ((fld_ptr->fd_key != 'n') && !(fld_ptr->fd_flags & OPTKEYMASK))
        {
            if (fld_ptr->fd_type != COMKEY)
                ptr = (char *) recval + fld_ptr->fd_ptr - rec_ptr->rt_data;
            else
                key_bldcom(fld, (char *) recval, ptr = key, TRUE, task);

            if ((stat = key_insert(fld, ptr, db_addr, task)) != S_OKAY)
            {
                r_delrec((short) nrec, db_addr, task);
                if (dio_write(db_addr, PGFREE, task) != S_OKAY)
                    return (task->db_status);

                return (task->db_status = stat);
            }
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


