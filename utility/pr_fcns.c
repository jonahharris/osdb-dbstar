/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbexp utility                                     *
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

    pr_fcns.c - formatted printing functions used by dbexp

    These functions print the values of the member pointers in a record,
    and the field contents of a record.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbexp.h"

/* ********************** TYPE DEFINITIONS *************************** */
#define QUOTES    TRUE
#define NO_QUOTES FALSE

/* ********************** LOCAL FUNCTION DECLARATIONS **************** */
static int pr_struct(FILE *, char *, int, int, EXPOPTS *, DB_TASK *);
static int pr_elem(FILE *, char *, int, int, EXPOPTS *, DB_TASK *);
static int pr_binary(FILE *, char *, int, EXPOPTS *);
static int pr_fld(FILE *, char *, int, EXPOPTS *, DB_TASK *);
static int pr_ascii(FILE *, void *, DB_BOOLEAN, EXPOPTS *, DB_TASK *);

static int pr_wbinary(FILE *, char *, int, EXPOPTS *);
static int pr_unicode(FILE *, void *, DB_BOOLEAN, EXPOPTS *, DB_TASK *);

#if defined(UNICODE)
#define pr_string pr_unicode
#else
#define pr_string pr_ascii
#endif

/*
 * print all database addresses of owners of sets of which this record
 * type is a member.
 */
int pr_mems(
    FILE       *df,
    short    rec_type,
    char       *rec_area,
    EXPOPTS    *xo,
    DB_TASK    *task)
{
    int         i,
                offset,
                status;
    DB_ADDR     dba;
    DB_TCHAR    sdba[12];

    /* for each member pointer */
    for (i = 0; i < task->size_mt; i++)
    {
        if (task->member_table[i].mt_record == rec_type)
        {
            /* pull out the owner pointer */
            offset = task->member_table[i].mt_mem_ptr;
            memcpy(&dba, rec_area + offset, sizeof(DB_ADDR));

            /* convert the pointer into ASCII */
            cvt_dba(sdba, dba, xo);

            /* print it */
            status = pr_string(df, sdba, NO_QUOTES, xo, task);
            if (status != S_OKAY)
                return status;
        }
    }                                   /* end for each member pointer */
    return S_OKAY;
}

/*
 * print all data in this record.
 */
int pr_data(
    FILE       *df,
    short    rec_type,
    char       *rec_area,
    EXPOPTS    *xo,
    DB_TASK    *task)
{
    int         fld,
                dims,
                base,
                nelem,
                i,
                size,
                status;

    /* for each data field in this record */
    for ( fld = task->record_table[rec_type].rt_fields;
          fld < task->record_table[rec_type].rt_fields +
                task->record_table[rec_type].rt_fdtot;
          fld++)
    {
        /* If this field is structured */
        if (task->field_table[fld].fd_type == GROUPED)
        {
            /* determine if there is an array of structures */
            for ( dims = 0;
                  dims < MAXDIMS && task->field_table[fld].fd_dim[dims];
                  dims++)
                ;  /* empty body */

            base = task->field_table[fld].fd_ptr;

            /* if this is an array of structures */
            if (dims > 0)
            {
                /* determine how many elements will be printed */
                for (nelem = 1, i = 0; i < dims; i++)
                    nelem *= task->field_table[fld].fd_dim[i];

                size = task->field_table[fld].fd_len / nelem;

                for (i = 0; i < nelem; i++)
                    pr_struct(df, rec_area, base + i * size, fld, xo, task);
            }
            else
            {
                pr_struct(df, rec_area, base, fld, xo, task);
            }
        }
        else
        {
            /* make sure this field is not a part of a structure */
            if (task->field_table[fld].fd_flags & STRUCTFLD)
                continue;

            /* print a non-structured field */
            status = pr_elem(df, rec_area, task->field_table[fld].fd_ptr, fld, xo, task);
            if (status != S_OKAY)
                return status;
        }
    }
    return S_OKAY;
}

/* print all elements of one structure */
static int pr_struct(
    FILE       *df,
    char       *rec_area,
    int         offset,
    int         ndx,
    EXPOPTS    *xo,
    DB_TASK    *task)
{
    int      diff,
                fld,
                status;

    /* for each subfield */
    fld = ndx + 1;
    while (task->field_table[fld].fd_flags & STRUCTFLD)
    {
        diff = task->field_table[fld].fd_ptr - task->field_table[ndx].fd_ptr;
        status = pr_elem(df, rec_area, offset + diff, fld, xo, task);
        if (status != S_OKAY)
            return status;
        fld++;
    }
    return S_OKAY;
}

/* print all elements of a non-structured field */
static int pr_elem(
    FILE       *df,
    char       *rec_area,
    int         offset,
    int         ndx,
    EXPOPTS    *xo,
    DB_TASK    *task)
{
    int      i,
             dims,
             nelem,
             idx,
             fd_width,
             size,
             status;

    /* determine if there is an array */
    for (dims = 0; dims < MAXDIMS && task->field_table[ndx].fd_dim[dims]; dims++)
        ;

    /* if this field is an array */
    if (dims > 0)
    {
        /* determine how many elements will be printed */
        for (nelem = 1, i = 0; i < dims; i++)
            nelem *= task->field_table[ndx].fd_dim[i];

        size = task->field_table[ndx].fd_len / nelem;

        /* character types are copied instead of converted */
        if (task->field_table[ndx].fd_type == CHARACTER)
        {
            fd_width = task->field_table[ndx].fd_dim[dims - 1];

            if (fd_width > 1)
            {
                /* for each character string array element */
                for (idx = 0; idx < task->field_table[ndx].fd_len; idx += fd_width)
                {
                    status = pr_ascii(df, rec_area + offset + idx, QUOTES, xo, task);
                    if (status != S_OKAY)
                        return status;
                }
            }
            else
            {
                /* binary copy */
                fd_width = task->field_table[ndx].fd_dim[dims - 2];
                for (idx = 0; idx < task->field_table[ndx].fd_len; idx += fd_width)
                {
                    status = pr_binary(df, rec_area + offset + idx, fd_width, xo);
                    if (status != S_OKAY)
                        return status;
                }
            }
        }
        else if (task->field_table[ndx].fd_type == WIDECHAR)
        {
            fd_width = task->field_table[ndx].fd_dim[dims - 1];

            if (fd_width > 1)
            {
                /* for each character string array element */
                for (idx = 0; idx < task->field_table[ndx].fd_len; 
                      idx += fd_width * sizeof(wchar_t))
                {
                    status = pr_unicode(df, rec_area + offset + idx, QUOTES, xo, task);
                    if (status != S_OKAY)
                        return status;
                }
            }
            else
            {
                /* binary copy */
                fd_width = task->field_table[ndx].fd_dim[dims - 2];
                for (idx = 0; idx < task->field_table[ndx].fd_len; 
                      idx += fd_width * sizeof(wchar_t))
                {
                    status = pr_wbinary(df, rec_area + offset + idx, fd_width, xo);
                    if (status != S_OKAY)
                        return status;
                }
            }
        }
        else
        {
            /* else non-character data */
            /* now loop once for each element following */
            for (i = 0; i < nelem; i++)
            {
                status = pr_fld(df, rec_area + offset + i * size, ndx, xo, task);
                if (status != S_OKAY)
                    return status;
            }
        }
    }
    else
    {
        status = pr_fld(df, rec_area + offset, ndx, xo, task);
        if (status != S_OKAY)
            return status;
    }
    return S_OKAY;
}

static int pr_binary(
    FILE       *df,
    char       *rec_area,
    int         width,
    EXPOPTS    *xo)
{
    int      i;

    if (xo->comma)
    {
        if (vfputtc(xo->sep_char, df) != xo->sep_char)
            return S_NOSPACE;
    }
    else
    {
        xo->comma = TRUE;
    }

    for (i = 0; i < width; i++)
    {
        if (vftprintf(df, DB_TEXT("%02x"), *rec_area++ & 0xff) == 0)
            return S_NOSPACE;
    }
    return S_OKAY;
}


/*
 * print one field of data, based on its type.
 */
static int pr_fld(
    FILE       *df,
    char       *fld_area,
    int         ndx,
    EXPOPTS    *xo,
    DB_TASK    *task)
{
    DB_TCHAR    str[30];
    char        charint;
    short       shortint;
    int         regint;
    long        longint;
    float       floatval;
    double      doubval;
    DB_ADDR     dbaval;
    int         unsignedFld;
    int         status = S_OKAY;

    /* select action based on type */
    unsignedFld = task->field_table[ndx].fd_flags & UNSIGNEDFLD;
    switch (task->field_table[ndx].fd_type)
    {
        case CHARACTER:
            /* single character type */
            memcpy(&charint, fld_area, CHAR_SIZE);
            if (unsignedFld)
                vstprintf(str, DB_TEXT("%u"), (unsigned int) charint);
            else
                vstprintf(str, DB_TEXT("%d"), (int) charint);
            status = pr_string(df, str, NO_QUOTES, xo, task);
            break;

        case SHORTINT:
            /* short integer type */
            memcpy(&shortint, fld_area, SHORT_SIZE);
            if (unsignedFld)
                vstprintf(str, DB_TEXT("%u"), (unsigned int) ((unsigned short) shortint));
            else
                vstprintf(str, DB_TEXT("%d"), (int) shortint);
            status = pr_string(df, str, NO_QUOTES, xo, task);
            break;

        case REGINT:
            /* regular integer type */
            memcpy(&regint, fld_area, INT_SIZE);
            if (unsignedFld)
                vstprintf(str, DB_TEXT("%u"), (unsigned int) regint);
            else
                vstprintf(str, DB_TEXT("%d"), regint);
            status = pr_string(df, str, NO_QUOTES, xo, task);
            break;

        case LONGINT:
            /* long integer type */
            memcpy(&longint, fld_area, LONG_SIZE);
            if (unsignedFld)
                vstprintf(str, DB_TEXT("%lu"), (unsigned long) longint);
            else
                vstprintf(str, DB_TEXT("%ld"), longint);
            status = pr_string(df, str, NO_QUOTES, xo, task);
            break;

        case FLOAT:
            /* float type */
            memcpy(&floatval, fld_area, FLOAT_SIZE);
            vstprintf(str, DB_TEXT("%.20g"), floatval);
            status = pr_string(df, str, NO_QUOTES, xo, task);
            break;

        case DOUBLE:
            /* double float type */
            memcpy(&doubval, fld_area, DOUBLE_SIZE);
            vstprintf(str, DB_TEXT("%.20g"), doubval);
            status = pr_string(df, str, NO_QUOTES, xo, task);
            break;

        case DBADDR:
            /* database address type */
            memcpy(&dbaval, fld_area, DB_ADDR_SIZE);
            cvt_dba(str, dbaval, xo);
            status = pr_string(df, str, NO_QUOTES, xo, task);
            break;

        case GROUPED:
            /* grouped type - WRONG! */
            vftprintf(stderr, DB_TEXT("Grouped type found in pr_field\n"));
            break;
    }                                   /* end select action based on type */
    return status;
}

/*
 * print to the data file, adding quotes and/or backslash characters
 * to escape quotes and backslashes.
 */
static int pr_ascii(
    FILE          *df,
    void          *data,
    DB_BOOLEAN     quotes,
    EXPOPTS       *xo,
    DB_TASK       *task)
{
    unsigned char  k;
    char           buf[10];
    char          *str = (char *) data;
#if defined(UNICODE)
    DB_TCHAR       tbuf[10];
    DB_TCHAR       tstr[2];
#else
    DB_TCHAR      *tbuf = buf;
    DB_TCHAR      *tstr;
#endif

    if (xo->comma)
    {
        if (vfputtc(xo->sep_char, df) != xo->sep_char)
            return S_NOSPACE;
    }
    else
    {
        xo->comma = TRUE;
    }

    if (quotes)
    {
        if (vfputtc(DB_TEXT('"'), df) != DB_TEXT('"'))
            return S_NOSPACE;
    }

    /* for each character to be printed */
    while (*str)
    {
#if defined(UNICODE)
        atow(str, tstr, 2);
        tstr[1] = 0;
#else
        tstr = str;
#endif

        /* if the character needs to be escaped */
        if ((*str == '"') || (tstr[0] == xo->esc_char))
        {
            /* put an escape character in front of it */
            if (vfputtc(xo->esc_char, df) != xo->esc_char)
                return S_NOSPACE;
        }

        /* print the character */
        if ((*str < ' ') || (*str > '~'))
        {
            buf[1] = '\0';
            k = (unsigned char) *str;
            if (task->ctbl_activ && task->country_tbl[k].out_chr)
            {
                buf[0] = k;
            }
            else
            {
                switch (*str)
                {
                    case '\n':  buf[0] = 'n';  break;
                    case '\r':  buf[0] = 'r';  break;
                    case '\t':  buf[0] = 't';  break;
                    case '\f':  buf[0] = 'f';  break;
                    case '\b':  buf[0] = 'b';  break;
                    default:
                        if (xo->extended)
                            buf[0] = *str;
                        else /* convert to \ooo form */
                            sprintf(buf, "%03o", (0xff & (int) *str));
                        break;
                }

                /* if the character needs to be escaped */
                if (*buf != *str)
                {
                    if (vfputtc(xo->esc_char, df) != xo->esc_char)
                        return S_NOSPACE;
                }
            }

#if defined(UNICODE)
            atow(buf, tbuf, sizeof(tbuf) / sizeof(DB_TCHAR));
#endif
            if (vfputts(tbuf, df) == EOF)
                return S_NOSPACE;
        }
        else
        {
            if (vfputtc(tstr[0], df) != tstr[0])
                return S_NOSPACE;
        }
        str++;
    }

    if (quotes)
    {
        if (vfputtc(DB_TEXT('"'), df) != DB_TEXT('"'))
            return S_NOSPACE;
    }
    return S_OKAY;
}

void cvt_dba(
    DB_TCHAR   *sdba,
    DB_ADDR     dba,
    EXPOPTS    *xo)
{
    short       file;
    DB_ULONG    slot;

    if (dba == NULL_DBA)
    {
        vtstrcpy(sdba, DB_TEXT("NULL"));
    }
    else
    {
        if (xo->decimal)
        {
            vstprintf(sdba, DB_TEXT("%lu"), dba);
        }
        else
        {
            d_decode_dba(dba, &file, &slot);
            vstprintf(sdba, DB_TEXT("%d:%lu"), file, slot);
        }
    }
    return;
}


static int pr_unicode(
    FILE          *df,
    void          *data,
    DB_BOOLEAN     quotes,
    EXPOPTS       *xo,
    DB_TASK       *task)
{
    wchar_t       *str = (wchar_t *) data;
#if defined(UNICODE)
    DB_TCHAR      *tstr;
#else
    DB_TCHAR       tstr[2];
#endif

    if (xo->comma)
    {
        if (vfputtc(xo->sep_char, df) != xo->sep_char)
            return S_NOSPACE;
    }
    else
    {
        xo->comma = TRUE;
    }

    if (quotes)
    {
        if (vfputtc(DB_TEXT('"'), df) != DB_TEXT('"'))
            return S_NOSPACE;
    }

    /* for each character to be printed */
    while (*str)
    {
#if defined(UNICODE)
        tstr = str;
#else
        wtoa(str, tstr, 2);
        tstr[1] = 0;
#endif

        if (vfputtc(tstr[0], df) != tstr[0])
            return S_NOSPACE;
        str++;
    }

    if (quotes)
    {
        if (vfputtc(DB_TEXT('"'), df) != DB_TEXT('"'))
            return S_NOSPACE;
    }
    return S_OKAY;
}

static int pr_wbinary(
    FILE       *df,
    char       *rec_area,
    int         width,
    EXPOPTS    *xo)
{
    int      i;
    wchar_t *p = (wchar_t *) rec_area;

    if (xo->comma)
    {
        if (vfputtc(xo->sep_char, df) != xo->sep_char)
            return S_NOSPACE;
    }
    else
    {
        xo->comma = TRUE;
    }

    for (i = 0; i < width; i++)
    {
        if (vftprintf(df, DB_TEXT("%04x"), (unsigned int) *p++) == 0)
            return S_NOSPACE;
    }
    return S_OKAY;
}



