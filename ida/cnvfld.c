/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* -------------------------------------------------------------------------
    IDA - Data Field Conversion Functions
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "ida.h"

/* temporary struct text buffer */
#define MAXTEXT 1600
static char sbuf[MAXTEXT];

static void txtcpy(int *, char *, char *, int);

/* =============================================================
    Convert data field to text
*/
char *fldtotxt(
    int fld,                            /* task->field_table index */
    char *data,                         /* pointer to contents of field */
    char *txt)                          /* pointer to text buffer */
{
    /* field value variables */
    DB_ADDR           f_db_addr;
    unsigned int      f_uint;
    unsigned short    f_ushort;
    unsigned long     f_ulong;
    int               f_int;
    short             f_short;
    long              f_long;

    float             f_float;
    double            f_double;
    int               txtp;
    short             fno;
    DB_ULONG          rno;
    char              buf[40];       /* temporary formatted spec. buffer */

    int               fd_type;       /* field type */
    int               fd_len;        /* total length of field */
    int               fd_size;       /* size of each entry in field */
    int               fd_elts;       /* number of array elements in field */
    int               fd_dims;       /* number of array dimensions in field */
    char             *fd_pos;        /* points to start of next data element */
    char             *sm_pos;        /* points to struct member data element */
    char             *r_base;        /* base address of record */

    int               cnt;
    register int      elt,
                      i;

    /* get pertinent info from task->field_table */
    fd_type = task->field_table[fld].fd_type;
    fd_len = task->field_table[fld].fd_len;

    /* compute number of array elements */
    for ( fd_elts = 1, fd_dims = 0;
          fd_dims <= MAXDIMS && task->field_table[fld].fd_dim[fd_dims];
          ++fd_dims)
    {
        fd_elts = fd_elts * task->field_table[fld].fd_dim[fd_dims];
    }
    if (fd_type == CHARACTER)
        fd_elts = 1;

    fd_size = fd_len / fd_elts;

    txtp = 0;
    *txt = '\0';                        /* initialize display text buffer */
    for (elt = 0; elt < fd_elts; ++elt)
    {
        fd_pos = data + (elt * fd_size);
        if (fd_type != CHARACTER)
        {
            for (cnt = 1, i = elt + 1; i < fd_elts; ++i, ++cnt)
            {
                /* compute repitition factor */
                if (memcmp(fd_pos, data + (i * fd_size), fd_size))
                    break;
            }
            if (elt)
                txtcpy(&txtp, txt, ",", 0);
            if (cnt > 2)
            {
                sprintf(buf, "%d*", cnt);
                txtcpy(&txtp, txt, buf, 0);
                elt = i - 1;
                fd_pos = data + (elt * fd_size);
            }
        }
        switch (fd_type)
        {
            case GROUPED:
                r_base = fd_pos - task->field_table[fld].fd_ptr;
                txtcpy(&txtp, txt, "{", 0);
                for (i = fld + 1;
                     i < task->size_fd && (STRUCTFLD & task->field_table[i].fd_flags); ++i)
                {
                    if (i - (fld + 1))
                        txtcpy(&txtp, txt, ",", 0);
                    sm_pos = r_base + task->field_table[i].fd_ptr;
                    if (task->field_table[i].fd_type == CHARACTER)
                    {
                        txtcpy(&txtp, txt, "\"", 0);
                        txtcpy(&txtp, txt, fldtotxt(i, sm_pos, sbuf), 0);
                        txtcpy(&txtp, txt, "\"", 0);
                    }
                    else
                    {
                        txtcpy(&txtp, txt, fldtotxt(i, sm_pos, sbuf), 0);
                    }
                }
                txtcpy(&txtp, txt, "}", 0);
                break;

            case CHARACTER:
                if (fd_len == 1 || fd_dims > 1)
                    txtcpy(&txtp, txt, fd_pos, fd_len);
                else
                    txtcpy(&txtp, txt, fd_pos, 0);
                break;

            case DBADDR:
                memcpy(&f_db_addr, fd_pos, DB_ADDR_SIZE);
                d_decode_dba(f_db_addr, &fno, &rno);
                sprintf(buf, "[%d:%ld]", fno, rno);
                txtcpy(&txtp, txt, buf, 0);
                break;

            case REGINT:
                if (task->field_table[fld].fd_flags & UNSIGNEDFLD)
                {
                    memcpy(&f_uint, fd_pos, INT_SIZE);
                    sprintf(buf, "%u", f_uint);
                }
                else
                {
                    memcpy(&f_int, fd_pos, INT_SIZE);
                    sprintf(buf, "%d", f_int);
                }
                txtcpy(&txtp, txt, buf, 0);
                break;

            case SHORTINT:
                if (task->field_table[fld].fd_flags & UNSIGNEDFLD)
                {
                    memcpy(&f_ushort, fd_pos, SHORT_SIZE);
                    sprintf(buf, "%u", f_ushort);
                }
                else
                {
                    memcpy(&f_short, fd_pos, SHORT_SIZE);
                    sprintf(buf, "%d", f_short);
                }
                txtcpy(&txtp, txt, buf, 0);
                break;

            case LONGINT:
                if (task->field_table[fld].fd_flags & UNSIGNEDFLD)
                {
                    memcpy(&f_ulong, fd_pos, LONG_SIZE);
                    sprintf(buf, "%lu", f_ulong);
                }
                else
                {
                    memcpy(&f_long, fd_pos, LONG_SIZE);
                    sprintf(buf, "%ld", f_long);
                }
                txtcpy(&txtp, txt, buf, 0);
                break;

            case FLOAT:
                memcpy(&f_float, fd_pos, FLOAT_SIZE);
                sprintf(buf, "%.20g", f_float);
                txtcpy(&txtp, txt, buf, 0);
                break;

            case DOUBLE:
                memcpy(&f_double, fd_pos, DOUBLE_SIZE);
                sprintf(buf, "%.20g", f_double);
                txtcpy(&txtp, txt, buf, 0);
                break;
                
            default:
                break;
        }
    }
    return (txt);
}

/* =============================================================
    Convert text to data field
*/
char *txttofld(int fld, char *txt, char *data)
{
    /* field value variables */
    DB_ADDR           f_db_addr;
    unsigned int      f_uint;
    unsigned short    f_ushort;
    unsigned long     f_ulong;
    int               f_int;
    short             f_short;
    long              f_long;

    float             f_float;
    double            f_double;

    short             sno;
    DB_ULONG          rno;

    int               fd_type;          /* field type */
    int               fd_len;           /* field length */
    int               fd_size;          /* size of each entry in field */
    int               fd_elts;          /* number of array elements in
                                         * field */
    int               fd_dims;          /* number of array dimensions in
                                         * field */
    char             *fd_pos;           /* points to start of next data
                                         * element */
    register char    *tp;               /* text pointer position */
    register int      i,
                      j,
                      cnt;
    char              del,
                     *tq;
    char             *sm_pos;           /* points to struct member data
                                         * element */
    char             *r_base;           /* base address of record */

    fd_len = task->field_table[fld].fd_len;
    fd_type = task->field_table[fld].fd_type;

    /* compute number of array elements */
    for ( fd_elts = 1, fd_dims = 0;
          fd_dims <= MAXDIMS && task->field_table[fld].fd_dim[fd_dims];
          ++fd_dims)
    {
        fd_elts = fd_elts * task->field_table[fld].fd_dim[fd_dims];
    }

    if (fd_type == CHARACTER && fd_dims < 2)
        fd_elts = 1;

    fd_size = fd_len / fd_elts;
    fd_pos = data;
    tp = txt;

    if (fd_type == CHARACTER)
    {
        *fd_pos = '\0';
        if (*tp == '"' || *tp == '\'')       /* delimited string */
            del = *tp++;
        else
            del = '\0';
        for (i = 0; i < fd_len && *tp != del && *tp; ++i, ++fd_pos)
        {
            if (task->ctbl_activ && task->country_tbl[(unsigned char) *tp].out_chr)
                *tp = task->country_tbl[(unsigned char) *tp].out_chr;
            if (*tp == '\\')
            {
                /* convert backslash code */
                ++tp;
                if (*tp >= '0' && *tp < '8')
                {
                    int count = 0;
                    /* convert from '\ddd' form */
                    f_int = 0;
                    while (*tp >= '0' && *tp < '8' && ++count < 4)
                        f_int = 8 * f_int + (int) (*tp++ - '0');
                    *fd_pos = (char) f_int;
                }
                else
                {
                    switch (*tp)
                    {
                        case 'n':   *fd_pos = '\n';   break;
                        case 'r':   *fd_pos = '\r';   break;
                        case 'b':   *fd_pos = '\b';   break;
                        case 't':   *fd_pos = '\t';   break;
                        case 'f':   *fd_pos = '\f';   break;
                        default:    *fd_pos = *tp;    break;
                    }
                    ++tp;
                }
            }
            else
            {
                *fd_pos = *tp++;
            }
            if ((fd_elts == 1) && (*fd_pos == '\0' || i == fd_len - 1))
                break;
        }
        if (fd_len > 1)
        {
            while (i++ < fd_len)
                *fd_pos++ = '\0';
        }
    }
    else
    {
        for (i = 0; i < fd_elts && *tp; ++i)
        {
            fd_pos = data + (i * fd_size);
            /* check for repeat factor */
            for (tq = tp, cnt = 0; *tq >= '0' && *tq <= '9'; ++tq)
                cnt = 10 * cnt + (*tq - '0');
            if (*tq == '*')
                tp = tq + 1;
            else
                cnt = 1;
            switch (fd_type)
            {
                case GROUPED:
                    if (*tp != '{')
                        return (tp);
                    r_base = fd_pos - task->field_table[fld].fd_ptr;
                    for (++tp, j = fld + 1;
                         j < task->size_fd && (STRUCTFLD & task->field_table[j].fd_flags);
                         ++j, ++tp)
                    {
                        sm_pos = r_base + task->field_table[j].fd_ptr;
                        if ((tq = txttofld(j, tp, sm_pos)) != NULL)
                            return (tq);
                        while (*tp && *tp != ',' && *tp != '}')
                            ++tp;
                    }
                    break;

                case REGINT:
                    if (task->field_table[fld].fd_flags & UNSIGNEDFLD)
                    {
                      if (!sscanf(tp, "%u", &f_uint))
                            return (tp);
                        memcpy(fd_pos, &f_uint, INT_SIZE);
                    }
                    else
                    {
                        if (!sscanf(tp, "%d", &f_int))
                            return (tp);
                        memcpy(fd_pos, &f_int, INT_SIZE);
                    }
                    break;

                case SHORTINT:
                    if (task->field_table[fld].fd_flags & UNSIGNEDFLD)
                    {
                        if (!sscanf(tp, "%u", &f_uint))
                            return (tp);
                        f_ushort = (unsigned short) f_uint;
                        memcpy(fd_pos, &f_ushort, SHORT_SIZE);
                    }
                    else
                    {
                        if (!sscanf(tp, "%d", &f_int))
                            return (tp);
                        f_short = (short) f_int;
                        memcpy(fd_pos, &f_short, SHORT_SIZE);
                    }
                    break;

                case LONGINT:
                    if (task->field_table[fld].fd_flags & UNSIGNEDFLD)
                    {
                        if (!sscanf(tp, "%lu", &f_ulong))
                            return (tp);
                        memcpy(fd_pos, &f_ulong, LONG_SIZE);
                    }
                    else
                    {
                        if (!sscanf(tp, "%ld", &f_long))
                            return (tp);
                        memcpy(fd_pos, &f_long, LONG_SIZE);
                    }
                    break;

                case FLOAT:
                    if (!sscanf(tp, "%f", &f_float))
                        return (tp);
                    memcpy(fd_pos, &f_float, FLOAT_SIZE);
                    break;

                case DOUBLE:
                    if (!sscanf(tp, "%lf", &f_double))
                        return (tp);
                    memcpy(fd_pos, &f_double, DOUBLE_SIZE);
                    break;

                case DBADDR:
                    tq = tp;
                    if (*tq != '[')
                        return (tp);
                    ++tq;
                    if (!sscanf(tq, "%d", &f_int))
                        return (tq);
                    sno = (short) f_int;
                    while (*tq && *tq != ':')
                        ++tq;
                    if (!*tq++)
                        return (tq);
                    if (!sscanf(tq, "%lu", &rno))
                        return (tq);
                    d_encode_dba(sno, rno, &f_db_addr);
                    memcpy(fd_pos, &f_db_addr, DB_ADDR_SIZE);
                    while (*tq && *tq != ']')
                        ++tq;
                    if (!*tq)
                        return (tq);
                    tp = tq;
                    break;

                default:
                    return (tp);
            }
            while (--cnt && i < fd_elts - 1)
                memcpy(data + (++i * fd_size), fd_pos, fd_size);

            while (*tp && *tp != ',')
                ++tp;
            if (*tp)
                ++tp;
        }
        if (i < fd_elts)
        {
            /* clear remainder of data field */
            for (i = i * fd_size; i < fd_len; ++i)
                data[i] = '\0';
        }
    }
    return (NULL);
}

/* =============================================================
    Copy data field display text string
*/
static void txtcpy(
    int *txtp,                          /* next text buffer position */
    char *txt,                          /* text buffer */
    char *str,                          /* character array */
    int len)                            /* length of str 0 => stop when
                                         * null byte encountered */
{
    register int i;

    unsigned char k;

    char buf[10];

    for (i = 0;
          *txtp < MAXTEXT - 1 && ((len && i < len) || (!len && str[i]));
          ++i, ++(*txtp))
    {
        if (str[i] < ' ' || str[i] > '~')
        {
            k = (unsigned char) str[i];
            if (task->ctbl_activ && task->country_tbl[k].out_chr)
            {
                txt[*txtp] = task->country_tbl[k].out_chr;
            }
            else
            {
                /* translate into \ codes */
                txt[(*txtp)++] = '\\';
                switch (str[i])
                {
                    case '\n':  txt[*txtp] = 'n';    break;
                    case '\r':  txt[*txtp] = 'r';    break;
                    case '\t':  txt[*txtp] = 't';    break;
                    case '\f':  txt[*txtp] = 'f';    break;
                    case '\b':  txt[*txtp] = 'b';    break;
                    case '\\':  txt[*txtp] = '\\';   break;
                    case '\0':  txt[*txtp] = '0';    break;
                    default:
                        /* convert to \ddd form */
                        sprintf(buf, "%03o", (0xff & (int) str[i]));
                        strcpy(txt + (*txtp), buf);
                        *txtp += strlen(buf) - 1;
                        break;
                }
            }
        }
        else
        {
            txt[*txtp] = str[i];
        }
    }
    txt[*txtp] = '\0';
}

/* =============================================================
    Convert text to compound key
*/
char *txttokey(int fld, char *txt, char *data)
{
    int i;
    register char *tp, *tq;

    tp = txt;
    if (*tp != '{')
        return (tp);
    for ( ++tp, i = task->field_table[fld].fd_ptr;
            task->key_table[i].kt_key == fld;
            ++i, ++tp)
    {
        if ((tq = txttofld(task->key_table[i].kt_field, tp,
                data + task->key_table[i].kt_ptr)) != NULL)
        {
            return (tq);
        }
        while (*tp && *tp != ',' && *tp != '}')
            ++tp;
    }
    return (NULL);
}


