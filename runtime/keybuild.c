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

#ifndef NO_KEYBUILD

/* ======================================================================
    db.* key file build utility
*/
int INTERNAL_FCN dkeybuild(DB_TASK *task, int dbn)
{
    FILE_ENTRY      *file_ptr;
    RECORD_ENTRY    *rec_ptr;
    FIELD_ENTRY     *field_ptr;
    int              keys = 0;
    int              stat;
    char            *rec,
                    *fptr;
    short            rid;
    DB_ADDR          dba;
    FILE_NO          fno;
    int              i;
    int              offset;
    F_ADDR           rno,
                     top;
    char             key[MAXKEYSIZE];

    /* make sure we have the necessary locks */
    if (task->dbopen == 1)           /* shared mode */
    {
        offset = task->curr_db_table->ft_offset;
        for (fno = 0; fno < DB_REF(Size_ft); ++fno)
        {
            if (! task->excl_locks[fno + offset])
                return (dberr(S_EXCLUSIVE));
        }
    }

    /* initialize all key files */
    for ( fno = DB_REF(ft_offset), file_ptr = &task->file_table[fno];
          fno < DB_REF(ft_offset) + DB_REF(Size_ft);
          ++fno, file_ptr++)
    {
        if (file_ptr->ft_type == KEY)
        {
            if (dinitfile((FILE_NO)NUM2EXT(fno, ft_offset), task, task->curr_db) != S_OKAY)
                return (task->db_status);

            key_reset(fno, task);
            keys = 1;
        }
    }

    if (!keys)
        return (task->db_status);

    /* scan each data file */
    for ( fno = DB_REF(ft_offset), file_ptr = &task->file_table[fno];
          fno < DB_REF(ft_offset) + DB_REF(Size_ft);
          ++fno, file_ptr++)
    {
        int has_keys = FALSE;

        if (file_ptr->ft_type != DATA)
            continue;

        /* see if file has any keys */
        for (rid = 0; rid < task->size_rt; ++rid)
        {
            if ( task->record_table[rid].rt_file == fno )
            {
                int fld;
                for ( fld = task->record_table[rid].rt_fields;
                      fld < task->size_fd && task->field_table[fld].fd_rec == rid;
                      ++fld)
                {
                    if ( task->field_table[fld].fd_key != NOKEY )
                    {
                        has_keys = TRUE;
                        break;
                    }
                }
            }
        }

        if ( !has_keys )
            continue;

        top = dio_pznext(fno, task) - 1L;

        /* read each record in data file */
        for (rno = 1L; rno <= top; ++rno)
        {
            /* read next record */
            ENCODE_DBA(NUM2EXT(fno, ft_offset), rno, &dba);
            if (dio_read(dba, &rec, PGHOLD, task) != S_OKAY)
                return (task->db_status);

            /* get record identification number */
            memcpy(&rid, rec, sizeof(short));

            /* remove the record lock bit */
            rid &= ~RLBMASK;

            if (rid >= DB_REF(Size_rt))
            {
                dio_release(dba, PGFREE, task);
                return (dberr(S_INVREC));
            }

            if (rid >= 0)
            {                             /* record not deleted */
                /* for each key field, enter the key value into the key file */
                for ( rec_ptr = &task->record_table[rid + DB_REF(rt_offset)],
                      i = rec_ptr->rt_fields;
                      i < task->size_fd && task->field_table[i].fd_rec == rid;
                      i++)
                {

                    field_ptr = &task->field_table[i];

                    /* skip if not a key field */
                    if (field_ptr->fd_key == NOKEY)
                        continue;

                    /* skip if optional and not stored */
                    if ((field_ptr->fd_flags & OPTKEYMASK) &&
                        !r_tstopt(field_ptr, rec, task))
                        continue;

                    /* build key if compound */
                    if (field_ptr->fd_type == COMKEY)
                    {
                        key_bldcom(i, rec + rec_ptr->rt_data, key, TRUE, task);
                        fptr = key;
                    }
                    else
                        fptr = rec + field_ptr->fd_ptr;

                    /* insert the key */
                    if ((stat = key_insert(i, fptr, dba, task)) != S_OKAY)
                    {
                        if (dio_release(dba, PGFREE, task) == S_OKAY)
                            task->db_status = stat;

                        return (task->db_status);
                    }
                }
            }

            /* free page hold */
            if (dio_release(dba, PGFREE, task) != S_OKAY)
                return (task->db_status);
        }
    }

    return (task->db_status);
}

#endif                                 /* NO_KEYBUILD */


