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
static int INTERNAL_FCN rec_okay(int, int *, RECORD_ENTRY **, DB_TASK *);


/* ======================================================================
    Check for valid (external) set number and return (internal) set number
    and task->set_table pointer.
*/
int INTERNAL_FCN nset_check(register int nset, int *set, SET_ENTRY **set_ptr,
                                     DB_TASK *task)
{
    nset -= SETMARK;
    if (nset < 0 || nset >= TABLE_SIZE(Size_st))
        return (dberr(S_INVSET));

    *set_ptr = &task->set_table[*set = NUM2INT(nset, st_offset)];
    return (task->db_status);
}


/* ======================================================================
    Check for valid (external) field number and return (internal) record
    and field numbers and pointers.
*/
int INTERNAL_FCN  nfld_check(
    register long  nfld,
    int           *rec,
    int           *fld,
    RECORD_ENTRY **rec_ptr,
    FIELD_ENTRY  **fld_ptr,
    DB_TASK       *task)
{
    int   trec;
    int   tfld;

    if (!rec_okay(trec = (int) (nfld / FLDMARK), rec, rec_ptr, task) ||
        (tfld = (int) (nfld - trec * FLDMARK)) < 0 ||
        tfld >= TABLE_SIZE(Size_fd) || (*rec_ptr)->rt_fdtot <= 0)
        return (dberr(S_INVFLD));

    *fld_ptr = &task->field_table[*fld = tfld + (*rec_ptr)->rt_fields];
    return (task->db_status);
}



/* ======================================================================
    Check for valid (external) record number and return (internal) record
    number and pointer.
*/
int INTERNAL_FCN nrec_check(int nrec, int *rec, RECORD_ENTRY **rec_ptr,
                            DB_TASK *task)
{
    if (rec_okay(nrec - RECMARK, rec, rec_ptr, task))
        return (task->db_status);

    return (dberr(S_INVREC));
}


/* ======================================================================
    Internal record number check
*/
static int INTERNAL_FCN rec_okay(register int nrec, int *rec,
                                 RECORD_ENTRY **rec_ptr, DB_TASK *task)
{
    if (nrec < 0 || nrec >= TABLE_SIZE(Size_rt))
        return FALSE;

    *rec_ptr = &task->record_table[*rec = NUM2INT(nrec, rt_offset)];
    return TRUE;
}


/* ======================================================================
    Compare values of two unsigned character arrays
*/
static int INTERNAL_FCN uchar_cmp(unsigned char *uc1, unsigned char *uc2,
                                  int len, int string)
{
    int result = 0;

    while (!result && len--)
    {
        if (string && (*uc1 == 0 || *uc2 == 0))
            return (int)*uc1 - (int)*uc2;

        result = (int)*uc1++ - (int)*uc2++;
    }

    return result;
}

/* ======================================================================
    Compare values of two unsigned long values
*/
static int INTERNAL_FCN ulong_cmp(unsigned long ul1, unsigned long ul2)
{
    if (ul1 < ul2)
        return -1;

    if (ul1 > ul2)
        return 1;

    return 0;
}

/* ======================================================================
    Compare values of two db.* data fields
*/
int EXTERNAL_FCN fldcmp(
    FIELD_ENTRY  *fld_ptr,
    const char   *f1,
    const char   *f2,
    DB_TASK      *task)
{
    /*
        returns < 0 if f1 < f2,
                = 0 if f1 == f2,
                > 0 if f1 > f2
    */
    int            kt_lc;
    int            elt;
    int            result;
    int            len;
    int            cur_len;
    int            sub_len;
    int            entries;
    int            ufld;
    short          key_num;
    unsigned int   ui1, ui2;
    unsigned long  ul1, ul2;
    unsigned short us1, us2;
    int            i1, i2;
    long           l1, l2;
    short          s1, s2;
    float          F1, F2;
    double         d1, d2;
    FIELD_ENTRY   *fld_last;
    FIELD_ENTRY   *sfld_ptr;
    KEY_ENTRY     *key_ptr;
    short         *dptr;
    unsigned char *uc1;
    unsigned char *uc2;

    len = fld_ptr->fd_len;
    result = 0;

    /* compute number of array elements */
    entries = 1;
    for (elt = 0, dptr = fld_ptr->fd_dim; elt < MAXDIMS && *dptr; ++elt, ++dptr)
        entries *= *dptr;

    ufld = fld_ptr->fd_flags & UNSIGNEDFLD;

    switch (fld_ptr->fd_type)
    {
        case CHARACTER:
            uc1 = (unsigned char *) f1;
            uc2 = (unsigned char *) f2;

            if (fld_ptr->fd_dim[1])
            {
                /* multiply-dimentioned array */

                if (ufld)
                    return uchar_cmp(uc1, uc2, entries, 0);
                
                return memcmp(f1, f2, entries);
            }

            if (task->ctbl_activ)
                return ctblcmp(uc1, uc2, len, task);

            /* unsigned char string or single unsigned char */
            if (ufld)
                return uchar_cmp(uc1, uc2, entries, 1);

            /* single-dimensioned array -- string */
            if (fld_ptr->fd_dim[0])
            {
                if (task->dboptions & MBSSORT)
                {
                    if (task->dboptions & IGNORECASE)
                        return vmbsnicmp(f1, f2, len);

                    return vmbsncmp(f1, f2, len);
                }
                else
                {
                    if (task->dboptions & IGNORECASE)
                        return strnicmp(f1, f2, len);

                    return strncmp(f1, f2, len);
                }
            }

            /* single char */
            return (int) *f1 - (int) *f2;

        case REGINT:
            for (elt = 0; !result && elt < entries; ++elt)
            {
                if (ufld)
                {
                    memcpy(&ui1, f1 + elt * sizeof(int), sizeof(int));
                    memcpy(&ui2, f2 + elt * sizeof(int), sizeof(int));
                    result = ulong_cmp((unsigned long) ui1, (unsigned long) ui2);
                }
                else
                {
                    memcpy(&i1, f1 + elt * sizeof(int), sizeof(int));
                    memcpy(&i2, f2 + elt * sizeof(int), sizeof(int));
                    if (i1 < i2)
                        result = -1;
                    else if (i1 > i2)
                        result = 1;
                }
            }

            break;

        case LONGINT:
            for (elt = 0; !result && elt < entries; ++elt)
            {
                if (ufld)
                {
                    memcpy(&ul1, f1 + (elt * sizeof(long)), sizeof(long));
                    memcpy(&ul2, f2 + (elt * sizeof(long)), sizeof(long));
                    result = ulong_cmp(ul1, ul2);
                }
                else
                {
                    memcpy(&l1, f1 + (elt * sizeof(long)), sizeof(long));
                    memcpy(&l2, f2 + (elt * sizeof(long)), sizeof(long));
                    if (l1 < l2)
                        result = -1;
                    else if (l1 > l2)
                        result = 1;
                }
            }

            break;

        case WIDECHAR:
            ufld = 1;
            /*
                If it's not a one-dimensional array, just drop through
                and treat it like an array of unsigned shorts (or a single
                unsigned short value.
            */
            if (fld_ptr->fd_dim[0] > 0 && fld_ptr->fd_dim[1] == 0)
            {
                if (task->dboptions & IGNORECASE)
                    return vwcsnicoll((const wchar_t *)f1, (const wchar_t *)f2, len / sizeof(wchar_t));

                return vwcsncoll((const wchar_t *)f1, (const wchar_t *)f2, len / sizeof(wchar_t));
            }

        case SHORTINT:
            for (elt = 0; !result && elt < entries; ++elt)
            {
                if (ufld)
                {
                    memcpy(&us1, f1 + (elt * sizeof(short)), sizeof(short));
                    memcpy(&us2, f2 + (elt * sizeof(short)), sizeof(short));
                    result = ulong_cmp((unsigned long)us1, (unsigned long) us2);
                }
                else
                {
                    memcpy(&s1, f1 + (elt * sizeof(short)), sizeof(short));
                    memcpy(&s2, f2 + (elt * sizeof(short)), sizeof(short));
                    if (s1 < s2)
                        result = -1;
                    else if (s1 > s2)
                        result = 1;
                }
            }

            break;

        case FLOAT:
            for (elt = 0; elt < entries; ++elt)
            {
                memcpy(&F1, f1 + (elt * sizeof(float)), sizeof(float));
                memcpy(&F2, f2 + (elt * sizeof(float)), sizeof(float));
                if (F1 < F2) {
                    result = -1;
                    break;
                }
                else if (F1 > F2)
                {
                    result = 1;
                    break;
                }
            }

            break;

        case DOUBLE:
            for (elt = 0; elt < entries; ++elt)
            {
                memcpy(&d1, f1 + (elt * sizeof(double)), sizeof(double));
                memcpy(&d2, f2 + (elt * sizeof(double)), sizeof(double));
                if (d1 < d2)
                {
                    result = -1;
                    break;
                }
                else if (d1 > d2)
                {
                    result = 1;
                    break;
                }
            }

            break;

        case DBADDR:
            for (elt = 0; !result && elt < entries; ++elt)
            {
                result = ADDRcmp((DB_ADDR *) (f1 + (elt * sizeof(DB_ADDR))),
                        (DB_ADDR *) (f2 + (elt * sizeof(DB_ADDR))));
            }
            break;

        case GROUPED:
            len /= entries;               /* length of each entry */
            fld_last = &task->field_table[task->size_fd - 1];
            cur_len = 0;
            for (elt = 0; !result && elt < entries; ++elt)
            {
                for ( sfld_ptr = fld_ptr + 1;
                      !result && sfld_ptr->fd_flags & STRUCTFLD;
                      ++sfld_ptr)
                {
                    sub_len = cur_len + sfld_ptr->fd_ptr - fld_ptr->fd_ptr;
                    result = fldcmp(sfld_ptr, f1 + sub_len, f2 + sub_len, task);

                    /* This is added because on some protected mode systems
                        (i.e. Phar Lap 286) task->field_table[task->size_fd] (one past end)
                        is not defined.
                    */
                    if (sfld_ptr == fld_last)
                        break;
                }

                cur_len += len;
            }
            break;

        case COMKEY:
            kt_lc = task->size_kt - fld_ptr->fd_ptr;
            key_num = task->key_table[fld_ptr->fd_ptr].kt_key;

            for ( key_ptr = &task->key_table[fld_ptr->fd_ptr];
                  !result && kt_lc-- > 0 && key_ptr->kt_key == key_num;
                  ++key_ptr)
            {
                sub_len = key_ptr->kt_ptr;

                result = fldcmp(&task->field_table[key_ptr->kt_field],
                                f1 + sub_len, f2 + sub_len, task);
            }

            break;

        default:
            dberr(SYS_INVFLDTYPE);     /* should never get here */
            return 0;
    }

    return result;
}


/* ======================================================================
    compare the short variables
*/
int INTERNAL_FCN SHORTcmp(const char *i1, const char *i2)
{
    short I1, I2;

    memcpy(&I1, i1, sizeof(short));
    memcpy(&I2, i2, sizeof(short));
    if (I1 < I2)
        return -1;

    if (I1 > I2)
        return 1;

    return 0;
}


/* ======================================================================
    compare two DB_ADDR variables
*/
int EXTERNAL_FCN ADDRcmp(const DB_ADDR *d1, const DB_ADDR *d2)
{
    DB_ADDR     a1, a2;
    short       f1, f2;
    DB_ULONG    r1, r2;

    memcpy(&a1, d1, DB_ADDR_SIZE);
    memcpy(&a2, d2, DB_ADDR_SIZE);

    DECODE_DBA(a1, &f1, &r1);
    DECODE_DBA(a2, &f2, &r2);

    if (f1 == f2)
    {
        if (r1 < r2)
            return -1;

        if (r1 > r2)
            return 1;

        return 0;
    }

    return (f1 - f2);
}




/* ======================================================================
    check for empty DB_ADDR
*/
int INTERNAL_FCN null_dba(const DB_ADDR db_addr)
{
    return (db_addr == NULL_DBA);
}


/* ======================================================================
    check for valid DB_ADDR
*/
int INTERNAL_FCN check_dba(DB_ADDR dba, DB_TASK *task)
{
    short       fno;
    DB_ULONG    rno, last;

    DECODE_DBA(dba, &fno, &rno);

    if (fno < 0 || fno >= TABLE_SIZE(Size_ft))
        return (dberr(S_INVADDR));

    if (task->file_table[fno + DB_REF(ft_offset)].ft_type != 'd')
        return (dberr(S_INVADDR));

    /* Make sure page 0 has been read */
    if ((last = dio_pznext((FILE_NO) NUM2INT(fno, ft_offset), task)) <= 0)
        return (task->db_status);

    if (rno == 0L || rno >= last)
        dberr(S_INVADDR);

    return (task->db_status);
}


/* ======================================================================
    Compare two strings with sorting according to char-table
*/
int INTERNAL_FCN ctblcmp(
    const unsigned char *s,      /* String 1 */
    const unsigned char *t,      /* String 2 */
    int                  len,     /* Max. String length */
    DB_TASK             *task)
{
    int            x;
    unsigned char  f1, f2,
                        x1, x2;

    /* Always return immediately at first difference found */
    for ( ; len && *s && *t; len--)
    {
        if (task->country_tbl[*s].sort_as1)
            f1 = task->country_tbl[*s].sort_as1;
        else
            f1 = *s;

        if (task->country_tbl[*t].sort_as1)
            f2 = task->country_tbl[*t].sort_as1;
        else
            f2 = *t;

        if ((x = f1 - f2) != 0)
            return (x);

        /* Check sort_as2-values if sort_as1-values are equal */
        /*----------------------------------------------------*/
        x1 = task->country_tbl[*s].sort_as2;
        x2 = task->country_tbl[*t].sort_as2;
        if (x1 && x2)
        {
            /* We have an entry for char. of both strings */
            if ((x = x1 - x2) != 0)
                return x;
        }
        else
        {
            if (x1 || x2)
            {
                /* Only sort_as2 value for one string */
                if (x1)
                {
                    /* Compare with next character in string 2 */
                    t++;
                    if (task->country_tbl[*t].sort_as1)
                        f2 = task->country_tbl[*t].sort_as1;
                    else
                        f2 = *t;

                    if ((x = x1 - f2) != 0)
                        return x;
                }

                if (x2)
                {
                    /* Compare with next character in string 1 */
                    s++;
                    if (task->country_tbl[*s].sort_as1)
                        f1 = task->country_tbl[*s].sort_as1;
                    else
                        f1 = *s;

                    if ((x = f1 - x2) != 0)
                        return x;
                }
            }

            /* if both are equal compare sub_sort values */
            x = task->country_tbl[*s].sub_sort - task->country_tbl[*t].sub_sort;
            if (x != 0)
                return x;
        }

        s++;
        t++;
    }           /* end of for() */

    if (len)
    {
        if (*s)
            return 1;

        if (*t)
            return -1;
    }

    return 0;
}



/* ======================================================================
    Length limited string copy with enforced null termination
*/
DB_TCHAR *vstrnzcpy(
    DB_TCHAR       *s1,
    const DB_TCHAR *s2,
    size_t          len)
{
    size_t s2_len = vtstrlen(s2)+1;
    if (s2_len > len)
        s2_len = len;

    vtstrncpy(s1, s2, s2_len);
    s1[len - 1] = 0;

    return s1;
}


/* ======================================================================
    Functions to simplify construction of null-terminated strings which
    must fit in a fixed size buffer.
*/
DB_TCHAR *STRinit(DB_STRING *dest, DB_TCHAR *src, size_t len)
{
    dest->buflen = len;
    dest->strlen = vtstrlen(src);
    if (dest->strlen >= dest->buflen)
        src[dest->strlen = dest->buflen-1] = DB_TEXT('\0');

    return (dest->strbuf = src);
}

DB_TCHAR *STRcpy(DB_STRING *dest, DB_TCHAR *src)
{
    dest->strlen = vtstrlen(vstrnzcpy(dest->strbuf, src, dest->buflen));
    return dest->strbuf;
}

DB_TCHAR *STRcat(DB_STRING *dest, DB_TCHAR *src)
{
    dest->strlen += vtstrlen(vstrnzcpy(dest->strbuf + dest->strlen, src,
            dest->buflen - dest->strlen));

    return dest->strbuf;
}

DB_TCHAR *STRccat(DB_STRING *dest, int c)
{
    if (STRavail(dest))
    {
        dest->strbuf[dest->strlen++] = (DB_TCHAR)c;
        dest->strbuf[dest->strlen] = DB_TEXT('\0');
    }

    return dest->strbuf;
}

size_t STRavail(DB_STRING *dest)
{
    return dest->buflen - dest->strlen - 1;
}


