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
    Return lock status for record type
*/
int INTERNAL_FCN dreclstat(int rec, DB_TCHAR *lstat, DB_TASK *task, int dbn)
{
    RECORD_ENTRY *rec_ptr;

    if (nrec_check(rec, &rec, &rec_ptr, task) != S_OKAY)
        return (task->db_status);

    if (task->dbopen >= 2)
        *lstat = DB_TEXT('f');
    else
    {
        if (rec_ptr->rt_flags & STATIC)
            *lstat = DB_TEXT('s');
        else
            *lstat = task->rec_locks[rec].fl_type;
    }

    return (task->db_status);
}


/* ======================================================================
    Return lock status for set type
*/
int INTERNAL_FCN dsetlstat(int set, DB_TCHAR *lstat, DB_TASK *task, int dbn)
{
    SET_ENTRY *set_ptr;

    if (nset_check(set, &set, (SET_ENTRY **) &set_ptr, task) != S_OKAY)
        return (task->db_status);

    if (task->dbopen >= 2)
        *lstat = DB_TEXT('f');
    else
        *lstat = task->set_locks[set].fl_type;

    return (task->db_status);
}


/* ======================================================================
    Return lock status for key type
*/
int INTERNAL_FCN dkeylstat(long key, DB_TCHAR *lstat, DB_TASK *task, int dbn)
{
    int fld, rec;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(key, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    if (fld_ptr->fd_key == NOKEY)
        return (dberr(S_NOTKEY));

    if (task->dbopen >= 2)
        *lstat = DB_TEXT('f');
    else
    {
        if (task->file_table[fld_ptr->fd_keyfile].ft_flags & STATIC)
            *lstat = DB_TEXT('s');
        else
            *lstat = task->key_locks[fld_ptr->fd_keyno].fl_type;
    }

    return (task->db_status);
}


