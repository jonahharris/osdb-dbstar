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

/* Internal function prototypes */
static void INTERNAL_FCN chk_desc_key(int, FIELD_ENTRY *, const char *, char *, DB_TASK *);

/* ======================================================================
    Delete optional key value
*/
int INTERNAL_FCN dkeydel(long field, DB_TASK *task, int dbn)
{
    int fld;                    /* field number */
    int rec, rn;                /* record type of current record */
    char *rptr;                 /* pointer to current record */
    const char *fptr;           /* pointer to field contents */
    char ckey[MAXKEYSIZE];      /* compound key data */
    int stat;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    /* ensure current record is valid for this field */
    dcrtype(&rn, task, dbn);

    if (rec != NUM2INT(rn - RECMARK, rt_offset))
        return (dberr(S_BADFIELD));

    /* ensure field is an optional key field */
    if (!(fld_ptr->fd_flags & OPTKEYMASK))
        return (dberr(S_NOTOPTKEY));

    /* read current record */
    stat = dio_read(task->curr_rec, (char **) &rptr, PGHOLD, task);
    if (stat == S_OKAY)
    {
        /* Make sure that the key has been stored */
        if (r_tstopt(fld_ptr, rptr, task) == S_OKAY)
        {
            if (dio_release(task->curr_rec, PGFREE, task) != S_OKAY)
                return (task->db_status);

            return (task->db_status = S_NOTFOUND);
        }
        else if (task->db_status == S_DUPLICATE)
            task->db_status = S_OKAY;

        if (fld_ptr->fd_type == COMKEY)
        {
            key_bldcom(fld, rptr + rec_ptr->rt_data, ckey, TRUE, task);
            fptr = ckey;
        }
        else
            fptr = rptr + fld_ptr->fd_ptr;

        /* delete key from value in current record */
        stat = key_delete(fld, fptr, task->curr_rec, task);

        /* Clear the optional key flag in the record */
        if (stat == S_OKAY)
            stat = r_clropt(fld_ptr, rptr, task);

        /* The data record has been updated */
        if (dio_write(task->curr_rec, PGFREE, task) != S_OKAY)
            return (task->db_status);
    }

    return (task->db_status = stat);
}


/* ======================================================================
    Check for optional key existence
*/
int INTERNAL_FCN dkeyexist(long field, DB_TASK *task, int dbn)
{
    int fld;                    /* field number */
    int rec, rn;                /* record type of current record */
    char *rptr;                 /* pointer to current record */
    int stat;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    /* ensure current record is valid for this field */
    dcrtype(&rn, task, dbn);

    if (rec != NUM2INT(rn - RECMARK, rt_offset))
        return (dberr(S_BADFIELD));

    /* ensure field is an optional key field */
    if (!(fld_ptr->fd_flags & OPTKEYMASK))
        return (dberr(S_NOTOPTKEY));

    /* read current record */
    if (dio_read(task->curr_rec, &rptr, NOPGHOLD, task) != S_OKAY)
        return (task->db_status);

    /* Check the bit map and return S_OKAY if already stored,
        else S_NOTFOUND
    */
    if ((stat = r_tstopt(fld_ptr, rptr, task)) == S_OKAY)
        stat = S_NOTFOUND;
    else if (stat == S_DUPLICATE)
        stat = S_OKAY;

    if (dio_release(task->curr_rec, NOPGFREE, task) != S_OKAY)
        return (task->db_status);

    return (task->db_status = stat);
}


/* ======================================================================
    Find record thru key field
*/
int INTERNAL_FCN dkeyfind(
    long field,                  /* field constant */
    const void *fldval,          /* value of the data field */
    DB_TASK *task,
    int dbn)                     /* database number */
{
    int fld, rec;
    DB_ADDR dba;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;
    char ckey[MAXKEYSIZE];

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY ||
        key_init(fld, task) != S_OKAY) /* initialize key function operation */
        return (task->db_status);

    if (fldval == NULL)
        key_boundary(KEYFIND, &dba, task);
    else
    {
        /* locate record with specified key */
        if (fld_ptr->fd_type == 'k')
        {
            chk_desc_key(fld, fld_ptr, fldval, ckey, task);
            fldval = ckey;
        }
        dba = NULL_DBA;

        if (key_locpos(fldval, &dba, task) != S_OKAY)
            return (task->db_status);

        /* set current record to found db addr */
        task->curr_rec = dba;
    }

    /* set timestamp */
    if (task->curr_rec && task->db_tsrecs)
        dutscr(&task->cr_time, task, dbn);

    return (task->db_status);
}


/* ======================================================================
    Check compound key value for descending fields
*/
static void INTERNAL_FCN chk_desc_key(int fld, FIELD_ENTRY *fld_ptr,
                                                  const char *fldval, char *ckey,
                                                  DB_TASK *task)
{
    register int kt_lc;                 /* loop control */
    int entries,i;
    short *dim_ptr;

    float fv;
    double dv;

    char *fptr;
    char *tptr;
    FIELD_ENTRY *kfld_ptr;
    register KEY_ENTRY *key_ptr;

    /* complement descending compound key values */
    for (kt_lc = task->size_kt - fld_ptr->fd_ptr,
         key_ptr = &task->key_table[fld_ptr->fd_ptr];
         (--kt_lc >= 0) && (key_ptr->kt_key == fld); ++key_ptr)
    {
        kfld_ptr = &task->field_table[key_ptr->kt_field];
        fptr = (char *) fldval + key_ptr->kt_ptr;
        tptr = ckey + key_ptr->kt_ptr;
        if (key_ptr->kt_sort == 'd')
        {
            entries = 1;
            for (i = 0, dim_ptr = kfld_ptr->fd_dim;
                    (i < MAXDIMS) && *dim_ptr;
                    ++i, ++dim_ptr)
                entries *= *dim_ptr;

            switch (kfld_ptr->fd_type)
            {
                case FLOAT:
                {
                    float *float_fptr = (float *)fptr,
                          *float_tptr = (float *)tptr;

                    for(i = 0; i < entries; i++,float_fptr++,float_tptr++)
                    {
                        memcpy(&fv, float_fptr, sizeof(float));
                        fv = (float) 0.0 - fv;
                        memcpy( float_tptr, &fv, sizeof(float));
                    }

                    break;
                }

                case DOUBLE:
                {
                    double *double_fptr = (double *)fptr,
                           *double_tptr = (double *)tptr;

                    for(i = 0; i < entries; i++,double_fptr++,double_tptr++)
                    {
                        memcpy(&dv, double_fptr, sizeof(double));
                        dv = 0.0 - dv;
                        memcpy( double_tptr, &dv, sizeof(double));
                    }

                    break;
                }

                case CHARACTER:
                    if (kfld_ptr->fd_dim[0] > 1 && kfld_ptr->fd_dim[1] == 0)
                        key_acpy(tptr, fptr, kfld_ptr->fd_len);
                    else
                        key_cmpcpy(tptr, fptr, kfld_ptr->fd_len);

                    break;

                case WIDECHAR:
                    if (kfld_ptr->fd_dim[0] > 1 && kfld_ptr->fd_dim[1] == 0)
                    {
                        key_wacpy((wchar_t *)tptr, (wchar_t *)fptr,
                                (short)(kfld_ptr->fd_len / sizeof(wchar_t)));
                        break;
                    }

                default:
                    key_cmpcpy(tptr, fptr, kfld_ptr->fd_len);
                    break;
            }
        }
        else
        {
            if (kfld_ptr->fd_type == CHARACTER && kfld_ptr->fd_dim[1] == 0)
                strncpy(tptr, fptr, kfld_ptr->fd_len);
            else if (kfld_ptr->fd_type == WIDECHAR && kfld_ptr->fd_dim[1] == 0)
                vwcsncpy((wchar_t *)tptr, (wchar_t *)fptr, kfld_ptr->fd_len / sizeof(wchar_t));
            else
                memcpy(tptr, fptr, kfld_ptr->fd_len);
        }
    }
}


/* ======================================================================
    Find first key
*/
int INTERNAL_FCN dkeyfrst(long field, DB_TASK *task, int dbn)
{
    int fld, rec;
    DB_ADDR dba;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) == S_OKAY &&
        key_init(fld, task) == S_OKAY) /* initialize key function operation */
    {
        if (key_boundary(KEYFRST, &dba, task) == S_OKAY)
        {
            task->curr_rec = dba;

            /* set timestamp */
            if (task->db_tsrecs)
                dutscr(&task->cr_time, task, dbn);
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Find last key
*/
int INTERNAL_FCN dkeylast(long field, DB_TASK *task, int dbn)
{
    int fld, rec;
    DB_ADDR dba;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) == S_OKAY &&
        key_init(fld, task) == S_OKAY) /* initialize key function operation */
    {
        if (key_boundary(KEYLAST, &dba, task) == S_OKAY)
        {
            task->curr_rec = dba;

            /* set timestamp */
            if (task->db_tsrecs)
                dutscr(&task->cr_time, task, dbn);
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Find next record thru key field
*/
int INTERNAL_FCN dkeynext(long field, DB_TASK *task, int dbn)
{
    int fld, rec;
    DB_ADDR dba;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) == S_OKAY &&
        key_init(fld, task) == S_OKAY) /* initialize key function operation */
    {
        if (key_scan(KEYNEXT, &dba, task) == S_OKAY)
        {
            task->curr_rec = dba;

            /* set timestamp */
            if (task->db_tsrecs)
                dutscr(&task->cr_time, task, dbn);
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Find previous record thru key field
*/
int INTERNAL_FCN dkeyprev(long field, DB_TASK *task, int dbn)
{
    int fld, rec;
    DB_ADDR dba;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) == S_OKAY &&
        key_init(fld, task) == S_OKAY) /* initialize key function operation */
    {
        if (key_scan(KEYPREV, &dba, task) == S_OKAY)
        {
            task->curr_rec = dba;

            /* set timestamp */
            if (task->db_tsrecs)
                dutscr(&task->cr_time, task, dbn);
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Store optional key value
*/
int INTERNAL_FCN dkeystore(long field, DB_TASK *task, int dbn)
{
    int fld;                    /* field number */
    int rec, rn;                /* record type of current record */
    char *rptr;                 /* pointer to current record */
    char *fptr;                 /* pointer to field contents */
    char ckey[MAXKEYSIZE];      /* compound key */
    int stat;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY *fld_ptr;

    if (nfld_check(field, &rec, &fld, &rec_ptr, &fld_ptr, task) != S_OKAY)
        return (task->db_status);

    /* Make sure we have a current record */
    if (!task->curr_rec)
        return (dberr(S_NOCR));

    /* ensure current record is valid for this field */
    dcrtype(&rn, task, dbn);

    if (rec != NUM2INT(rn - RECMARK, rt_offset))
        return (dberr(S_BADFIELD));

    /* ensure field is an optional key field */
    if (!(fld_ptr->fd_flags & OPTKEYMASK))
        return (dberr(S_NOTOPTKEY));

    /* read current record */
    if ((stat = dio_read(task->curr_rec, &rptr, PGHOLD, task)) == S_OKAY)
    {
        /* Make sure that the key has not already been stored */
        if ((stat = r_tstopt(fld_ptr, rptr, task)) != S_OKAY)
        {
            if (dio_release(task->curr_rec, PGFREE, task) != S_OKAY)
                return (task->db_status);
            return(task->db_status = (stat == S_DUPLICATE) ? S_OKAY : stat);
        }

        if (fld_ptr->fd_type == COMKEY)
        {
            key_bldcom(fld, rptr + rec_ptr->rt_data, ckey, TRUE, task);
            fptr = ckey;
        }
        else
            fptr = rptr + fld_ptr->fd_ptr;

        /* if this is a unique key field, make sure the key does
           not already exist
        */
        if (fld_ptr->fd_key == UNIQUE)
        {
            DB_ADDR dba = task->curr_rec;

            /* If the key field is not optional, or optional and stored */
            stat = dkeyfind(field, fptr, task, task->curr_db);
            task->curr_rec = dba;
            if (stat == S_OKAY)
            {
                /* another record is already using this key value */
                if (dio_release(task->curr_rec, PGFREE, task) != S_OKAY)
                    return (task->db_status);
                return (task->db_status = S_DUPLICATE);
            }
            if (stat == S_NOTFOUND)
               stat = S_OKAY;
            task->db_status = stat;
        }

        /* store key from value in current record */
        stat = key_insert(fld, fptr, task->curr_rec, task);

        /* Set the optional key bit */
        if (stat == S_OKAY)
            stat = r_setopt(fld_ptr, rptr, task);

        /* The data record has been modified */
        if (dio_write(task->curr_rec, PGFREE, task) != S_OKAY)
            return (task->db_status);
    }

    return (task->db_status = stat);
}


