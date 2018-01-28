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
    Find first record of type
*/
int INTERNAL_FCN drecfrst(int rec, DB_TASK *task, int dbn)
{
    short         rectype;
    FILE_NO       ftype;
    DB_ADDR       dba;
    char         *recptr;
    int           dbopen_sv;
    F_ADDR        rno,
                  last;
    RECORD_ENTRY *rec_ptr;

    if (nrec_check(rec, &rec, &rec_ptr, task) != S_OKAY)
        return (task->db_status);

    rec -= task->curr_db_table->rt_offset;

    /* get the normalized number of file containing this record type */
    ftype = (short) NUM2EXT(rec_ptr->rt_file, ft_offset);

    if ((last = dio_pznext(rec_ptr->rt_file, task)) <= 0)
        return (task->db_status);

    rno = 1;
    do
    {
        /* make sure we haven't gone past the end of the file */
        if (rno >= last)
            return (task->db_status = S_NOTFOUND);

        /* create the database address to read */
        ENCODE_DBA(ftype, rno, &dba);

        /* set up to allow unlocked read */
        dbopen_sv = task->dbopen;
        task->dbopen = 2;

        /* read the record */
        dio_read(dba, &recptr, NOPGHOLD, task);
        task->dbopen = dbopen_sv;

        if (task->db_status != S_OKAY)
            return (task->db_status);

        /* get the record type out of the record */
        memcpy(&rectype, recptr, sizeof(short));
        if (dio_release(dba, NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        rectype &= ~RLBMASK;
        ++rno;
    } while ((int) rectype != rec);

    /* set the current record and type */
    task->curr_rec = dba;
    task->curr_rn_table->rn_type = rectype;
    task->curr_rn_table->rn_dba = dba;

    /* set timestamp */
    if (task->db_tsrecs)
        dutscr(&task->cr_time, task, dbn);

#if 0
#ifndef QNX
    if (task->dbopen == 1 &&
        (task->lockMgrComm == LMC_TCP || task->lockMgrComm == LMC_GENERAL) &&
        !task->app_locks[rec_ptr->rt_file] &&
        !task->excl_locks[rec_ptr->rt_file] &&
        !(task->file_table[rec_ptr->rt_file].ft_flags & STATIC))
        dio_close(rec_ptr->rt_file, task);
#endif
#endif

    return (task->db_status);
}


/* ======================================================================
    d_reclast - find last record occurance in database
*/
int INTERNAL_FCN dreclast(int rec, DB_TASK *task, int dbn)
{
    DB_ADDR       dba;        /* current database addr we're scanning */
    FILE_NO       ftype;      /* file desc for file holding rec */
    F_ADDR        last;       /* last slot in file */
    char         *recptr;     /* record from database */
    RECORD_ENTRY *rec_ptr;    /* RECORD ENTRY for this record */
    short         rectype;    /* record type from record */
    F_ADDR        rno;        /* current slot we're scanning */
    int           dbopen_sv;  /* saved copy of task->dbopen */

    /* validate and convert record number */
    if (nrec_check(rec, &rec, &rec_ptr, task) != S_OKAY)
        return (task->db_status);

    rec -= task->curr_db_table->rt_offset;

    /* get the last record # for this file */
    ftype = (short) NUM2EXT(rec_ptr->rt_file, ft_offset);
    if ((last = dio_pznext(rec_ptr->rt_file, task)) <= 0)
        return (task->db_status);

    /* start at the end, working backwards, find a matching record */
    rno = last - 1;
    do
    {
        if (rno < 1)
            return (task->db_status = S_NOTFOUND);

        /* create the database address, and read this record */
        ENCODE_DBA(ftype, rno, &dba);

        dbopen_sv = task->dbopen;
        task->dbopen = 2;                      /* setup to allow unlocked read */

        dio_read(dba, &recptr, NOPGHOLD, task);

        task->dbopen = dbopen_sv;

        if (task->db_status != S_OKAY)
            return (task->db_status);

        /* See if this record is of the type we're looking for */
        memcpy(&rectype, recptr, sizeof(short));
        if (dio_release(dba, NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        rectype &= ~((short) RLBMASK);   /* remove rlb */
        rno--;
    } while ((int) rectype != rec);

    /* when we get here, we know a match was found */
    task->curr_rec = dba;                  /* set current record */
    task->curr_rn_table->rn_type = rectype;/* setup for future recprev,recnext */
    task->curr_rn_table->rn_dba = dba;

    /* set timestamp */
    if (task->db_tsrecs)
        dutscr(&task->cr_time, task, dbn);

#if 0
#ifndef QNX
    if (task->dbopen == 1 &&
        (task->lockMgrComm == LMC_TCP || task->lockMgrComm == LMC_GENERAL) &&
        !task->app_locks[rec_ptr->rt_file] &&
        !task->excl_locks[rec_ptr->rt_file] &&
        !(task->file_table[rec_ptr->rt_file].ft_flags & STATIC))
        dio_close(rec_ptr->rt_file, task);
#endif
#endif

    return (task->db_status);
}


/* ======================================================================
    Find next record of type
*/
int INTERNAL_FCN drecnext(DB_TASK *task, int dbn)
{
    short         rectype;
    short         fno;
    FILE_NO       ft;
    DB_ADDR       dba;
    int           dbopen_sv;
    int           rec_ndx;       /* Index into record table */
    RN_ENTRY     *rn_entry;
    RECORD_ENTRY *rec_ptr;       /* Pointer to record table */
    char         *recptr;
    DB_ULONG      rno,
                  last;

    /* look for the current record type */
    rn_entry = task->curr_rn_table;
    if (rn_entry->rn_type < 0)
        return (dberr(S_NOTYPE));

    /* get the record number and file number from the current record */
    if (rn_entry->rn_dba)
        DECODE_DBA(rn_entry->rn_dba, &fno, &rno)
    else
    {
        /* No current rec - get fno from rn_type */
        nrec_check(rn_entry->rn_type + RECMARK, &rec_ndx, &rec_ptr, task);
        fno = (FILE_NO) NUM2EXT(rec_ptr->rt_file, ft_offset);
        rno = 1;
    }

    ft = (FILE_NO) NUM2INT(fno, ft_offset);

    /* start looking at the next record number */
    if ((last = dio_pznext(ft, task)) <= 0)
        return (task->db_status);

    ++rno;
    do
    {
        /* make sure we haven't gone past the end of the file */
        if (rno >= last)
        {
            rn_entry->rn_dba = NULL_DBA;    /* set to wrap to beginning */
            return (task->db_status = S_NOTFOUND);
        }

        /* create the database address to read */
        ENCODE_DBA(fno, rno, &dba);

        /* set up to allow unlocked read */
        dbopen_sv = task->dbopen;
        task->dbopen = 2;

        /* read the record */
        dio_read(dba, &recptr, NOPGHOLD, task);

        task->dbopen = dbopen_sv;

        if (task->db_status != S_OKAY)
            return (task->db_status);

        /* get the record type out of the record */
        memcpy(&rectype, recptr, sizeof(short));
        if (dio_release(dba, NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        rectype &= ~RLBMASK;

        ++rno;
    } while (rectype != rn_entry->rn_type);

    /* set the current record */
    task->curr_rec = dba;
    rn_entry->rn_type = rectype;
    rn_entry->rn_dba = dba;

    /* set timestamp */
    if (task->db_tsrecs)
        dutscr(&task->cr_time, task, dbn);

#if 0
#ifndef QNX
    if (task->dbopen == 1 &&
        (task->lockMgrComm == LMC_TCP || task->lockMgrComm == LMC_GENERAL) &&
        !task->app_locks[ft] && !task->excl_locks[ft] &&
        !(task->file_table[ft].ft_flags & STATIC))
        dio_close(ft, task);
#endif
#endif

    return (task->db_status);
}


/* ======================================================================
    d_recprev - find previous record via database address
*/
int INTERNAL_FCN drecprev(DB_TASK *task, int dbn)
{
    DB_ADDR       dba;        /* current database addr we're scanning */
    short         fno;        /* current file we're scanning */
    DB_ULONG      last;       /* last slot in file */
    int           rec_ndx;    /* index of RECORD ENTRY (not used) */
    char         *recptr;     /* record from database */
    RN_ENTRY     *rn_entry;
    RECORD_ENTRY *rec_ptr;    /* RECORD ENTRY for this record */
    short         rectype;    /* record type from record */
    DB_ULONG      rno;        /* current slot we're scanning */
#if 0
#ifndef QNX
    FILE_NO       ft;         /* file table index for record */
#endif
#endif
    int           dbopen_sv;  /* saved copy of task->dbopen */

    /* setup current record and file number */
    rn_entry = task->curr_rn_table;
    if (rn_entry->rn_type < 0)
        return (dberr(S_NOTYPE));

    if (rn_entry->rn_dba)
    {
        DECODE_DBA(rn_entry->rn_dba, &fno, &rno);
#if 0
#ifndef QNX
        ft = NUM2INT(fno, ft_offset);
#endif
#endif
    }
    else
    {
        /* no current rec, get fno from rn_type */
        nrec_check(rn_entry->rn_type + RECMARK, &rec_ndx, &rec_ptr, task);
        fno = (FILE_NO) NUM2EXT(rec_ptr->rt_file, ft_offset);

        /* compute rno as last slot in file */
        if ((last = dio_pznext(rec_ptr->rt_file, task)) <= 0)
            return (task->db_status);

        rno = last;
#if 0
#ifndef QNX
        ft = rec_ptr->rt_file;
#endif
#endif
    }

    /* scan backwards looking for a record of the same type */
    rno--;
    do
    {
        if (rno < 1)
        {
            rn_entry->rn_dba = NULL_DBA;    /* set to wrap to end */
            return (task->db_status = S_NOTFOUND);
        }

        ENCODE_DBA(fno, rno, &dba);

        dbopen_sv = task->dbopen;
        task->dbopen = 2;                      /* setup to allow for unlocked read */

        dio_read(dba, &recptr, NOPGHOLD, task);

        task->dbopen = dbopen_sv;

        if (task->db_status != S_OKAY)
            return (task->db_status);

        /* see if we've found a match */
        memcpy(&rectype, recptr, sizeof(short));
        if (dio_release(dba, NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        rectype &= ~((short) RLBMASK);

        rno--;
    } while (rectype != rn_entry->rn_type);

    /* when we get here, we know a match was found */
    task->curr_rec = dba;                     /* set current record to match */
    rn_entry->rn_type = rectype;
    rn_entry->rn_dba = dba;

    /* set timestamp */
    if (task->db_tsrecs)
        dutscr(&task->cr_time, task, dbn);

#if 0
#ifndef QNX
    if (task->dbopen == 1 &&
        (task->lockMgrComm == LMC_TCP || task->lockMgrComm == LMC_GENERAL) &&
        !task->app_locks[ft] && !task->excl_locks[ft] &&
        !(task->file_table[ft].ft_flags & STATIC))
        dio_close(ft, task);
#endif
#endif

    return (task->db_status);
}


/* ======================================================================
    Read contents of current record
*/
int INTERNAL_FCN drecread(void *rec, DB_TASK *task, int dbn)
{
    short rt;                        /* record type */
    DB_ADDR dba;
    int dbopen_sv;
    RECORD_ENTRY *rec_ptr;

    /* Make sure we have a current record */
    if (!task->curr_rec)
        return (dberr(S_NOCR));

    if (check_dba(task->curr_rec, task) != S_OKAY)
        return (task->db_status);

    /* set up to allow unlocked read access */
    dbopen_sv = task->dbopen;
    task->dbopen = 2;

    /* read current record */
    dio_read(task->curr_rec, &task->crloc, NOPGHOLD, task);

    task->dbopen = dbopen_sv;

    if (task->db_status != S_OKAY)
        return (task->db_status);

    /* copy record type from record */
    memcpy(&rt, task->crloc, sizeof(short));
    if (rt < 0)
    {
        if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        return (task->db_status = S_DELETED);
    }

    if (rt & RLBMASK)
    {
        rt &= ~RLBMASK;                  /* mask off rlb */
        task->rlb_status = S_LOCKED;
    }
    else
        task->rlb_status = S_UNLOCKED;

    rec_ptr = &task->record_table[NUM2INT(rt, rt_offset)];

    /* Copy db_addr from record and check with task->curr_rec */
    memcpy(&dba, task->crloc + sizeof(short), DB_ADDR_SIZE);
    if (ADDRcmp(&dba, &task->curr_rec) != 0)
    {
        if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
            return (task->db_status);

        return (dberr(S_INVADDR));
    }

    /* Copy data from task->crloc into rec */
    memcpy(rec, &task->crloc[rec_ptr->rt_data], rec_ptr->rt_len - rec_ptr->rt_data);

    if (dio_release(dba, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    /* set timestamp */
    if (task->db_tsrecs)
        dutscr(&task->cr_time, task, dbn);

    return (task->db_status);
}


/* ======================================================================
    set record type and database address to current
*/
int INTERNAL_FCN drecset(int rec, DB_TASK *task, int dbn)
{
    FILE_NO       rfile;      /* file containing user specified rec */
    FILE_NO       fno;        /* file containing current record */
    int           rec_ndx;    /* Index into record table */
    RECORD_ENTRY *rec_ptr;    /* Pointer to record table */
    DB_ULONG      slot;

    /* Check rec parameter user passed */
    if (nrec_check(rec, &rec_ndx, &rec_ptr, task) != S_OKAY)
        return (task->db_status);

    /* Check to make sure current record is in this file */
    rfile = (FILE_NO) (NUM2EXT(rec_ptr->rt_file, ft_offset));
    DECODE_DBA(task->curr_rec, &fno, &slot);
    if (fno != rfile)
        return (dberr(S_INVREC));

    /* Everything is okay - save the type and database address */
    task->curr_rn_table->rn_type = (short) (rec - RECMARK);
    task->curr_rn_table->rn_dba = task->curr_rec;

#if 0
#ifndef QNX
    if (task->dbopen == 1 &&
        (task->lockMgrComm == LMC_TCP || task->lockMgrComm == LMC_GENERAL) &&
        !task->app_locks[rec_ptr->rt_file] &&
        !task->excl_locks[rec_ptr->rt_file] &&
        !(task->file_table[rec_ptr->rt_file].ft_flags & STATIC))
        dio_close(rec_ptr->rt_file, task);
#endif
#endif

    return (task->db_status);
}


/* ======================================================================
    Test timestamp status of record
*/
int INTERNAL_FCN drecstat(DB_ADDR dba, DB_ULONG rts, DB_TASK *task, int dbn)
{
    short rec;
    char *ptr;
    DB_ULONG cts, dbuts;
    int stat = S_OKAY;

    if (check_dba(dba, task) != S_OKAY ||
        dio_read(dba, &ptr, NOPGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* get record id */
    memcpy(&rec, ptr, sizeof(short));
    if (rec >= 0)
    {
        rec &= ~RLBMASK;                 /* mask off rlb */
        rec += task->curr_db_table->rt_offset;
        if (task->record_table[rec].rt_flags & TIMESTAMPED)
        {
            memcpy(&cts, ptr + RECCRTIME, sizeof(DB_ULONG));
            if (cts > rts)
                stat = S_DELETED;
            else
            {
                memcpy(&dbuts, ptr + RECUPTIME, sizeof(DB_ULONG));
                if (dbuts > rts)
                    stat = S_UPDATED;
            }
        }
        else
            stat = S_TIMESTAMP;
    }
    else
        stat = S_DELETED;

    if (dio_release(dba, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    if (stat != S_OKAY)
        task->db_status = stat;

    return (task->db_status);

}


/* ======================================================================
    Write contents to current record
*/
int INTERNAL_FCN drecwrite(const void *rec, DB_TASK *task, int dbn)
{
    DB_ULONG               timestamp;
    short                  rt;                  /* record type */
    char                  *fptr;                /* field data pointer */
    char                   ckey[MAXKEYSIZE];    /* current compound key data */
    char                   nkey[MAXKEYSIZE];    /* new compound key data */
    int                    stat;
    register short         fld;
    RECORD_ENTRY          *rec_ptr;
    register FIELD_ENTRY  *fld_ptr;

    /* Make sure we have a current record */
    if (!task->curr_rec)
        return (dberr(S_NOCR));

    if (check_dba(task->curr_rec, task) != S_OKAY)
        return (task->db_status);

    /* Read current record */
    if (dio_read(task->curr_rec, (char **) &task->crloc, PGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* copy record type from record */
    memcpy(&rt, task->crloc, sizeof(short));
    if (rt < 0)
    {
        if (dio_release(task->curr_rec, PGFREE, task) != S_OKAY)
            return (task->db_status);

        return (task->db_status = S_DELETED);
    }

    rt &= ~RLBMASK;                  /* mask off rlb */

    rt += task->curr_db_table->rt_offset;

    rec_ptr = &task->record_table[rt];

    /* Check out each field before they are changed */
    for (fld = rec_ptr->rt_fields, fld_ptr = &task->field_table[fld];
         (fld < task->size_fd) && (fld_ptr->fd_rec == rt); ++fld, ++fld_ptr)
    {
        /* Build compound key for new data supplied by user.  Note: cflag
           must be the same here as in the 1st key_bldcom for r_chkfld
        */
        if (fld_ptr->fd_type == COMKEY)
        {
            key_bldcom(fld, (char *) rec, nkey, FALSE, task);
            fptr = nkey;
        }
        else
            fptr = (char *) rec + fld_ptr->fd_ptr - rec_ptr->rt_data;

        if (!(fld_ptr->fd_flags & STRUCTFLD))
        {
            if ((stat = r_chkfld(fld, fld_ptr, task->crloc, fptr, task)) != S_OKAY)
            {
                if (dio_release(task->curr_rec, PGFREE, task) != S_OKAY)
                    return (task->db_status);

                return (task->db_status = stat);
            }
        }
    }

    /* Copy data from rec into task->crloc */
    for (fld = (short) ((rt == task->size_rt - 1) ? (task->size_fd - 1) :
         ((rec_ptr + 1)->rt_fields - 1)), fld_ptr = &task->field_table[fld];
         fld >= rec_ptr->rt_fields; --fld, --fld_ptr)
    {
        /* go backwards so comkeys are processed first */
        if (fld_ptr->fd_type == COMKEY)
        {
            /* build old and new keys */
            key_bldcom(fld, task->crloc + rec_ptr->rt_data, ckey, TRUE, task);
            key_bldcom(fld, (char *) rec, nkey, TRUE, task);

            /* make sure value has changed */
            /* if the key has been stored */
            if (memcmp(ckey, nkey, fld_ptr->fd_len) != 0 &&
                (!(fld_ptr->fd_flags & OPTKEYMASK) ||
                r_tstopt(fld_ptr, task->crloc, task)))
            {
                /* delete the old key */
                if (key_delete(fld, ckey, task->curr_rec, task) == S_OKAY)
                {
                    /* insert the new one */
                    stat = key_insert(fld, nkey, task->curr_rec, task);
                    if (stat != S_OKAY)
                    {
                        if (dio_release(task->curr_rec, PGFREE, task) != S_OKAY)
                            return (task->db_status);

                        return (task->db_status = stat);
                    }
                }
                else
                {
                    if (task->db_status == S_NOTFOUND)
                        dberr(S_KEYERR);
          
                    return (task->db_status);
                }
            }
        }
        else if (!(STRUCTFLD & fld_ptr->fd_flags))
        {
            /* ignore sub-fields of structures */
            if (r_pfld(fld, fld_ptr, task->crloc, (char *) rec + fld_ptr->fd_ptr -
                rec_ptr->rt_data, &task->curr_rec, task) != S_OKAY)
            {
                stat = task->db_status;
                if (dio_release(task->curr_rec, PGFREE, task) != S_OKAY)
                    return (task->db_status);

                return (task->db_status = stat);
            }
        }
    }

    /* check for timestamp */
    if (rec_ptr->rt_flags & TIMESTAMPED)
    {
        timestamp = dio_pzgetts(rec_ptr->rt_file, task);
        memcpy(task->crloc + RECUPTIME, &timestamp, sizeof(long));
    }
    else
        timestamp = 0L;

    /* write current record to page */
    dio_write(task->curr_rec, PGFREE, task);

    if (task->db_status == S_OKAY && task->db_tsrecs)
        task->cr_time = timestamp;

    return (task->db_status);
}


