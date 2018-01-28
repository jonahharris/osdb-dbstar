/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, utility programs                                  *
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
#include "cnvfld.h"

#define MAXTEXT 1600

static int txtcpy(int *, DB_TCHAR *, void *, int, DB_TASK *);
static int wtxtcpy(int *, DB_TCHAR *, void *, int, DB_TASK *);
#if defined(UNICODE)
#define ttxtcpy wtxtcpy
#else
#define ttxtcpy txtcpy
#endif

/* temporary struct text buffer */
static DB_TCHAR sbuf[MAXTEXT];


/* Convert data field to text */

DB_TCHAR *fldtotxt(
    int fld,                            /* task->field_table index */
    char *data,                         /* pointer to contents of field */
    DB_TCHAR *txt,                      /* pointer to text buffer */
    int hex,                            /* non-zero for db_addr's in hex */
    DB_TASK *task)
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
    DB_TCHAR          buf[40];       /* temporary formatted spec. buffer */
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
          fd_dims < 4 && task->field_table[fld].fd_dim[fd_dims];
          ++fd_dims)
    {
        fd_elts = fd_elts * task->field_table[fld].fd_dim[fd_dims];
    }

    if (fd_type == CHARACTER)
        fd_elts = 1;

    if (fd_type == WIDECHAR)
        fd_elts = 1;

    fd_size = fd_len / fd_elts;

    txtp = 0;
    *txt = 0;                        /* initialize display text buffer */
    for (elt = 0; elt < fd_elts; ++elt)
    {
        fd_pos = data + (elt * fd_size);
        if (fd_type != CHARACTER && fd_type != WIDECHAR)
        {
            for (cnt = 1, i = elt + 1; i < fd_elts; ++i, ++cnt)
            {
                /* compute repitition factor */
                if (memcmp(fd_pos, data + (i * fd_size), fd_size))
                    break;
            }
            if (elt)
                ttxtcpy(&txtp, txt, DB_TEXT(","), 0, task);
            if (cnt > 2)
            {
                vstprintf(buf, DB_TEXT("%d*"), cnt);
                ttxtcpy(&txtp, txt, buf, 0, task);
                elt = i - 1;
                fd_pos = data + (elt * fd_size);
            }
        }
        switch (fd_type)
        {
            case GROUPED:
                r_base = fd_pos - task->field_table[fld].fd_ptr;
                ttxtcpy(&txtp, txt, DB_TEXT("{"), 0, task);
                for ( i = fld + 1;
                      i < task->size_fd && (STRUCTFLD & task->field_table[i].fd_flags);
                      ++i)
                {
                    if (i - (fld + 1))
                        ttxtcpy(&txtp, txt, DB_TEXT(","), 0, task);
                    sm_pos = r_base + task->field_table[i].fd_ptr;
                    if (task->field_table[i].fd_type == CHARACTER
                     || task->field_table[i].fd_type == WIDECHAR)
                    {
                        ttxtcpy(&txtp, txt, DB_TEXT("\""), 0, task);
                        ttxtcpy(&txtp, txt, fldtotxt(i, sm_pos, sbuf, hex, task),
                                0, task);
                        ttxtcpy(&txtp, txt, DB_TEXT("\""), 0, task);
                    }
                    else
                    {
                        ttxtcpy(&txtp, txt, fldtotxt(i, sm_pos, sbuf, hex, task),
                                0, task);
                    }
                }
                ttxtcpy(&txtp, txt, DB_TEXT("}"), 0, task);
                break;

            case COMKEY:
                ttxtcpy(&txtp, txt, DB_TEXT("{"), 0, task);

                for (i = task->field_table[fld].fd_ptr;
                     (i < task->size_kt) && (fld == task->key_table[i].kt_key);
                     i++)
                {
                    if (i > task->field_table[fld].fd_ptr)
                        ttxtcpy(&txtp, txt, DB_TEXT(","), 0, task);

                    sm_pos = data + task->key_table[i].kt_ptr;

                    if (task->field_table[task->key_table[i].kt_field].fd_type == CHARACTER
                     || task->field_table[task->key_table[i].kt_field].fd_type == WIDECHAR)
                    {
                        ttxtcpy(&txtp, txt, DB_TEXT("\""), 0, task);
                        ttxtcpy(&txtp, txt, fldtotxt(task->key_table[i].kt_field,
                                sm_pos, sbuf, hex, task), 0, task);
                        ttxtcpy(&txtp, txt, DB_TEXT("\""), 0, task);
                    }
                    else
                    {
                        ttxtcpy(&txtp, txt, fldtotxt(task->key_table[i].kt_field,
                                sm_pos, sbuf, hex, task), 0, task);
                    }
                }
                ttxtcpy(&txtp, txt, DB_TEXT("}"), 0, task);
                break;

            case CHARACTER:
                if (fd_len == 1 || fd_dims > 1)
                    txtcpy(&txtp, txt, fd_pos, (int) fd_len, task);
                else
                    txtcpy(&txtp, txt, fd_pos, 0, task);
                break;

            case WIDECHAR:
                if ((fd_len / sizeof(wchar_t)) == 1 || fd_dims > 1)
                    wtxtcpy(&txtp, txt, fd_pos, (int) (fd_len / sizeof(wchar_t)), task);
                else
                    wtxtcpy(&txtp, txt, fd_pos, 0, task);
                break;

            case DBADDR:
                memcpy(&f_db_addr, fd_pos, DB_ADDR_SIZE);
                d_decode_dba(f_db_addr, &fno, &rno);
                vstprintf(buf, hex ? DB_TEXT("[%x:%lx]") : DB_TEXT("[%d:%ld]"),
                             fno, rno);
                ttxtcpy(&txtp, txt, buf, 0, task);
                break;

            case REGINT:
                if (task->field_table[fld].fd_flags & UNSIGNEDFLD)
                {
                    memcpy(&f_uint, fd_pos, INT_SIZE);
                    vstprintf(buf, DB_TEXT("%u"), f_uint);
                }
                else
                {
                    memcpy(&f_int, fd_pos, INT_SIZE);
                    vstprintf(buf, DB_TEXT("%d"), f_int);
                }
                ttxtcpy(&txtp, txt, buf, 0, task);
                break;

            case SHORTINT:
                if (task->field_table[fld].fd_flags & UNSIGNEDFLD)
                {
                    memcpy(&f_ushort, fd_pos, SHORT_SIZE);
                    vstprintf(buf, DB_TEXT("%u"), f_ushort);
                }
                else
                {
                    memcpy(&f_short, fd_pos, SHORT_SIZE);
                    vstprintf(buf, DB_TEXT("%d"), f_short);
                }
                ttxtcpy(&txtp, txt, buf, 0, task);
                break;

            case LONGINT:
                if (task->field_table[fld].fd_flags & UNSIGNEDFLD)
                {
                    memcpy(&f_ulong, fd_pos, LONG_SIZE);
                    vstprintf(buf, DB_TEXT("%lu"), f_ulong);
                }
                else
                {
                    memcpy(&f_long, fd_pos, LONG_SIZE);
                    vstprintf(buf, DB_TEXT("%ld"), f_long);
                }
                ttxtcpy(&txtp, txt, buf, 0, task);
                break;

            case FLOAT:
                memcpy(&f_float, fd_pos, FLOAT_SIZE);
                vstprintf(buf, DB_TEXT("%.20g"), f_float);
                ttxtcpy(&txtp, txt, buf, 0, task);
                break;

            case DOUBLE:
                memcpy(&f_double, fd_pos, DOUBLE_SIZE);
                vstprintf(buf, DB_TEXT("%.20g"), f_double);
                ttxtcpy(&txtp, txt, buf, 0, task);
                break;
            default:
                break;
        }
    }
    return (txt);
}


/* Copy data field display text string
*/
static int txtcpy(
    int      *txtp,        /* next text buffer position */
    DB_TCHAR *txt,         /* text buffer */
    void     *str,         /* character array */
    int       len,         /* length of str 0 => stop when */
    DB_TASK  *task)        /* null byte encountered */
{
    register int  i;
    unsigned char k;
    char         *p = (char *)str;
    DB_TCHAR      buf[10];
#if defined(UNICODE)
    char          s[2];

    s[0] = s[1] = '\0';
#endif

    for ( i = 0;
          (*txtp < MAXTEXT - 1) && ((len && i < len) || (!len && p[i]));
          ++i, ++(*txtp))
    {
        if ((p[i] < ' ') || (p[i] > '~') || (p[i] == '\\'))
        {
            k = (unsigned char) p[i];
            if (task->ctbl_activ && task->country_tbl[k].out_chr)
            {
#if defined(UNICODE)
                s[0] = (char) task->country_tbl[k].out_chr;
		atow(s, &txt[*txtp], 2);
#else
                txt[*txtp] = task->country_tbl[k].out_chr;
#endif
            }
            else
            {
                /* translate into \ codes */
                txt[(*txtp)++] = DB_TEXT('\\');
                switch (p[i])
                {
                    case '\n':  txt[*txtp] = DB_TEXT('n');    break;
                    case '\r':  txt[*txtp] = DB_TEXT('r');    break;
                    case '\t':  txt[*txtp] = DB_TEXT('t');    break;
                    case '\f':  txt[*txtp] = DB_TEXT('f');    break;
                    case '\b':  txt[*txtp] = DB_TEXT('b');    break;
                    case '\\':  txt[*txtp] = DB_TEXT('\\');   break;
                    case '\0':  txt[*txtp] = DB_TEXT('0');    break;
                    default:
                        /* convert to \ddd form */
                        vstprintf(buf, DB_TEXT("%03o"), (0xff & (int) p[i]));
                        vtstrcpy(txt + (*txtp), buf);
                        *txtp += vtstrlen(buf) - 1;
                        break;
                }
            }
        }
        else
        {
#if defined(UNICODE)
            s[0] = p[i];
	    atow(s, &txt[*txtp], 2);
#else
            txt[*txtp] = p[i];
#endif
        }
    }
    txt[*txtp] = 0;
    return (0);
}

static int wtxtcpy(int *txtp, DB_TCHAR *txt, void *str, int len, DB_TASK *task)
{
    wchar_t *p = (wchar_t *) str;

    if (!len)
        len = vwcslen(p);

#if defined(UNICODE)
    wcsncpy(&txt[*txtp], p, len);
#else
    wtoa(p, &txt[*txtp], len+1);
#endif

    *txtp += len;
    txt[*txtp] = '\0';
    return (0);
}


