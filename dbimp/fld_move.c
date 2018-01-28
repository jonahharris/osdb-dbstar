/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbimp utility                                     *
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
#include "impdef.h"
#include "impvar.h"

static DB_ADDR read_dba(DB_TCHAR *);
static int mv_struct(char *, int, int, DB_TCHAR *);
static int mv_elem(char *, int, int, int, int *, DB_TCHAR *);
static DB_TCHAR next_ch(DB_TCHAR *, int *, int *);

static int asc_fld;                    /* Current ASCII field position */

/**************************************************************************/

int fld_move(struct fld *fptr, char *record)
{
    register int fld, i;
    DB_TCHAR *line = NULL;
    int dims, size, nelem, base;
    int idx, endidx;
    DB_TASK *task = imp_g.dbtask;

    /* for each field */
    for ( ; fptr; fptr = fptr->fld_next)
    {
        /* see if the field is within the scope of loop stack */
        for (i = 0; i < imp_g.loop_lvl; i++)
        {
            if (vtstrcmp(fptr->fld_file, imp_g.curloop[i]->u.sp_looptr->l_fname) == 0)
            {
                line = imp_g.curloop[i]->u.sp_looptr->l_line;
                break;
            }
        }

        /* field spec not within any active file */
        if (i == imp_g.loop_lvl)
        {
            vftprintf(stderr, DB_TEXT("File %s is not open\n"), fptr->fld_file);
            dbimp_abort(DB_TEXT("Execution terminated"));
            return FAILURE;
        }

        asc_fld = fptr->fld_name;
        fld = fptr->fld_ndx;

        /* If this field is not a structure field */
        if (task->field_table[fld].fd_type != GROUPED)
        {
            /* move a non-structured field */
            if (mv_elem(record, fptr->fld_offset, fld, fptr->fld_dims,
                    fptr->fld_dim, line) == FAILURE)
                return FAILURE;

            continue;
        }

        /* determine if there is an array of structures */
        for (dims = 0; dims < MAXDIMS && task->field_table[fld].fd_dim[dims]; dims++)
            ;

        base = task->field_table[fld].fd_ptr -
                task->record_table[task->field_table[fld].fd_rec].rt_data;

        /* if this is an array of structures */
        if (dims <= 0)
        {
            mv_struct(record, base, fld, line);
            fptr = fptr->fld_next;
            continue;
        }

        /* determine how many elements will be moved */
        for (nelem = 1, i = 0; i < dims; i++)
            nelem *= task->field_table[fld].fd_dim[i];

        size = task->field_table[fld].fd_len / nelem;

        /* if this is a partial definition, find the starting element */
        vec_idx(dims, task->field_table[fld].fd_dim, fptr->fld_dim, &idx);

        /* find the ending element */
        if (fptr->fld_dims)
        {
            fptr->fld_dim[fptr->fld_dims - 1]++;
            vec_idx(dims, task->field_table[fld].fd_dim, fptr->fld_dim, &endidx);
            fptr->fld_dim[fptr->fld_dims - 1]--;
        }
        else
            endidx = nelem;

        if (dims > fptr->fld_dims)
        {
            /* now loop once for each element following */
            for (i = idx; i < endidx; i++)
                mv_struct(record, base + i * size, fld, line);
        }
        /* not partial definition */
        else
            mv_struct(record, base + idx * size, fld, line);
    }

    return OK;
}

/**************************************************************************/

/* move all elements of one structure */
static int mv_struct(char *record, int offset, int ndx, DB_TCHAR *line)
{
    int vec[MAXDIMS];
    int i, diff, fld;
    DB_TASK *task = imp_g.dbtask;

    for (i = 0; i < MAXDIMS; i++)
        vec[i] = 0;

    /* for each subfield */
    fld = ndx + 1;

    while (task->field_table[fld].fd_flags & STRUCTFLD)
    {
        diff = task->field_table[fld].fd_ptr - task->field_table[ndx].fd_ptr;
        if (mv_elem(record, offset + diff, fld, 0, vec, line) == FAILURE)
            return FAILURE;

        fld++;
    }

    return OK;
}

/**************************************************************************/

/* move all elements of a non-structured field */
static int mv_elem(char *record, int offset, int ndx, int flddims,
                   int *flddim, DB_TCHAR *line)
{
    int i, dims, size, nelem, idx, endidx, fd_width;
    DB_TASK *task = imp_g.dbtask;

    /* determine if there is an array */
    for (dims = 0; dims < MAXDIMS && task->field_table[ndx].fd_dim[dims]; dims++)
        ;

    /* if this field is an array */
    if (dims <= 0)
        return mv_fld(record, offset, ndx, line);


    /* determine how many elements will be moved */
    for (nelem = 1, i = 0; i < dims; i++)
        nelem *= task->field_table[ndx].fd_dim[i];

    size = task->field_table[ndx].fd_len / nelem;

    /* if this is a partial definition, find the starting element */
    vec_idx(dims, task->field_table[ndx].fd_dim, flddim, &idx);

    /* find the ending element */
    if (flddims)
    {
        flddim[flddims - 1]++;
        vec_idx(dims, task->field_table[ndx].fd_dim, flddim, &endidx);
        flddim[flddims - 1]--;
    }
    else
        endidx = nelem;

    /* character types are copied instead of converted */
    if (  (task->field_table[ndx].fd_type == CHARACTER
         || task->field_table[ndx].fd_type == WIDECHAR)
        && dims != flddims)
    {

        fd_width = task->field_table[ndx].fd_dim[dims - 1];

        /* if not partial definition of string */
        if (flddims && dims == flddims + 1)
        {
            if (task->field_table[ndx].fd_type == CHARACTER)
                return mv_char(record, offset + idx, fd_width, line);

            return mv_wchar(record, offset + idx, fd_width, line);
        }

        if (fd_width > 1)
        {
            /* for each character string array element */
            for (; idx < endidx; idx += fd_width)
            {
                if (task->field_table[ndx].fd_type == CHARACTER)
                {
                    if (mv_char(record, offset + idx, fd_width, line) == FAILURE)
                        return FAILURE;
                }
                else
                {
                    if (mv_wchar(record, offset + idx, fd_width, line) == FAILURE)
                        return FAILURE;
                }
            }
            return OK;
        }

        /* binary copy */
        fd_width = task->field_table[ndx].fd_dim[dims - 2];
        for (; idx < endidx; idx += fd_width)
        {
            if (task->field_table[ndx].fd_type == CHARACTER)
            {
                if (mv_binary(record, offset + idx, fd_width, line) == FAILURE)
                    return FAILURE;
            }
            else
            {
                if (mv_wbinary(record, offset + idx, fd_width, line) == FAILURE)
                    return FAILURE;
            }
        }
        return OK;
    }

    /* character data */
    if (dims <= flddims)
        return mv_fld(record, offset + idx * size, ndx, line);

    /* now loop once for each element following */
    for (i = idx; i < endidx; i++)
    {
        if (mv_fld(record, offset + i * size, ndx, line) == FAILURE)
            return FAILURE;
    }

    return OK;
}

/**************************************************************************/

int mv_char(char *record, int offset, int fd_width, DB_TCHAR *line)
{
    DB_TCHAR *fld_cont;
    int width;

    fld_cont = find_fld(asc_fld, line, &width);

    if (fld_cont == NULL)
        return FAILURE;

    if (fd_width <= width && fd_width > 1)
    {
        if (!imp_g.silent)
        {
            vftprintf(stderr,
                DB_TEXT("**WARNING**  db.* field not wide enough for %d; truncated\n"),
                asc_fld);
        }
        width = fd_width - 1;
    }

    /* copy into the record */
    ttoa(fld_cont, record + offset, width);

    asc_fld++;
    return OK;

}

/**************************************************************************/


int mv_wchar(char *record, int offset, int fd_width, DB_TCHAR *line)
{
    DB_TCHAR *fld_cont;
    int width;

    fld_cont = find_fld(asc_fld, line, &width);

    if (fld_cont == NULL)
        return FAILURE;

    if (fd_width <= width && fd_width > 1)
    {
        if (!imp_g.silent)
        {
            vftprintf(stderr,
                DB_TEXT("**WARNING**  db.* field not wide enough for %d; truncated\n"),
                asc_fld);
        }
        width = fd_width - 1;
    }

    /* copy into the record */
    record += offset;
    ttow(fld_cont, (wchar_t *) record, width);

    asc_fld++;
    return OK;

}


/**************************************************************************/

int mv_binary(char *record, int offset, int fd_width, DB_TCHAR *line)
{
    DB_TCHAR *fld_cont, bin[3];
    int width, i;
    int hex;

    fld_cont = find_fld(asc_fld, line, &width);

    if (fld_cont == NULL)
        return FAILURE;

    if (fd_width * 2 < width)
    {
        if (!imp_g.silent)
        {
            vftprintf(stderr,
                DB_TEXT("**WARNING**  db.* field not wide enough for %d; truncated\n"),
                asc_fld);
        }
        width = fd_width * 2;
    }

    /* copy into the record */
    for (i = 0; i < width;)
    {
        bin[0] = fld_cont[i++];
        bin[1] = fld_cont[i++];
        bin[2] = '\0';
        vstscanf(bin, DB_TEXT("%02x"), &hex);
        record[offset + (i - 2) / 2] = (char) hex;
    }

    asc_fld++;
    return OK;

}

/**************************************************************************/


int mv_wbinary(char *record, int offset, int fd_width, DB_TCHAR *line)
{
    DB_TCHAR *fld_cont, bin[5];
    wchar_t *p;
    int width, i, j;
    int hex;

    record += offset;
    p = (wchar_t *) record;

    fld_cont = find_fld(asc_fld, line, &width);

    if (fld_cont == NULL)
        return FAILURE;

    if (fd_width * 4 < width)
    {
        if (!imp_g.silent)
        {
            vftprintf(stderr,
                DB_TEXT("**WARNING**  db.* field not wide enough for %d; truncated\n"),
                asc_fld);
        }
    }

    /* copy into the record */
    for (i = j = 0; j < fd_width; j++)
    {
        bin[0] = fld_cont[i++];
        bin[1] = fld_cont[i++];
        bin[2] = fld_cont[i++];
        bin[3] = fld_cont[i++];
        bin[4] = '\0';
        vstscanf(bin, DB_TEXT("%04x"), &hex);
        p[j] = (wchar_t) hex;
    }

    asc_fld++;
    return OK;

}


/**************************************************************************/

static char read_char(DB_TCHAR *fld)
{
    int     num = 0;
    int     i = 0;
    int     escape = 0;
#if defined(UNICODE)
    wchar_t wnum[2];
    char    cnum[2];
#endif

    if (fld[0] != DB_TEXT('\''))
    {
        vstscanf(fld, DB_TEXT("%d"), &num);
        return (char) num;
    }

    while ((fld[++i] != DB_TEXT('\'')) || (escape == 1))
    {
        if (escape == 0)    /* No '\' yet. */
        {
            if (fld[i] == DB_TEXT('\\'))   /* We found a '\' character */
                escape = 1;
            else    /* Just a regular character */
            {
#if defined(UNICODE)
                wnum[0] = fld[i];
                wnum[1] = 0;
                wtoa(wnum, cnum, sizeof(cnum));
                num = cnum[0];
#else
                num = fld[i];
#endif
                escape = -1;
            }
        }
        else if (escape == -1)
        {
            /* a valid character has been found, but no closing "'" */
            vftprintf(stderr, DB_TEXT("**WARNING** invalid char %s\n"), fld);
            return (char) num;
        }
        else if (fld[i] >= DB_TEXT('0') && fld[i] < DB_TEXT('8'))  /* octal */
        {
            do
            {
                if (i > 4)  /* too many digits */
                {
                    vftprintf(stderr,
                        DB_TEXT("**WARNING** invalid octal value: %s\n"), fld);
                    return (char) num;
                }
                num *= 8;
                num += fld[i] - DB_TEXT('0');

                if (fld[++i] == DB_TEXT('\''))
                    escape = -1;

            } while (fld[i] >= DB_TEXT('0') && fld[i] < DB_TEXT('8'));

            if (fld[i] != DB_TEXT('\''))
            {
                vftprintf(stderr,
                    DB_TEXT("**WARNING** invalid octal value: %s\n"), fld);
                return (char) num;
            }
            i--;
        }
        else
        {
            escape = -1;
            switch (fld[i])
            {
                case DB_TEXT('n'):   num = '\n';    break;
                case DB_TEXT('r'):   num = '\r';    break;
                case DB_TEXT('f'):   num = '\f';    break;
                case DB_TEXT('t'):   num = '\t';    break;
                case DB_TEXT('b'):   num = '\b';    break;
                case DB_TEXT('v'):   num = '\v';    break;
                default:
#if defined(UNICODE)
                    wnum[0] = fld[i];
                    wnum[1] = 0;
                    wtoa(wnum, cnum, sizeof(cnum));
                    num = cnum[0];
#else
                    num = fld[i];
#endif
                    break;
            }
        }
    }

    if (fld[i + 1])
        vftprintf(stderr,
            DB_TEXT("**WARNING** invalid character value - %s\n"), fld);

    return (char) num;
}

/**************************************************************************/


static wchar_t read_wchar(DB_TCHAR *fld)
{
    int num = 0, i = 0, escape = 0;
#if !defined(UNICODE)
    wchar_t wnum[2];
    char cnum[2];
#endif

    if (fld[0] != DB_TEXT('\''))
    {
        vstscanf(fld, DB_TEXT("%d"), &num);
        return (wchar_t) num;
    }

    while ((fld[++i] != DB_TEXT('\'')) || (escape == 1))
    {
        if (escape == 0)    /* No '\' yet. */
        {
            if (fld[i] == DB_TEXT('\\'))   /* We found a '\' character */
                escape = 1;
            else    /* Just a regular character */
            {
#if defined(UNICODE)
                num = fld[i];
#else
                cnum[0] = fld[i];
                cnum[1] = 0;
                atow(cnum, wnum, sizeof(wnum));
                num = wnum[0];
#endif
                escape = -1;
            }
        }
        else if (escape == -1)
        {
            /* a valid character has been found, but no closing "'" */
            vftprintf(stderr, DB_TEXT("**WARNING** invalid char %s\n"), fld);
            return (wchar_t) num;
        }
        else if (fld[i] >= DB_TEXT('0') && fld[i] < DB_TEXT('8'))  /* octal */
        {
            do
            {
                if (i > 4)  /* too many digits */
                {
                    vftprintf(stderr,
                        DB_TEXT("**WARNING** invalid octal value: %s\n"), fld);
                    return (wchar_t) num;
                }
                num *= 8;
                num += fld[i] - DB_TEXT('0');

                if (fld[++i] == DB_TEXT('\''))
                    escape = -1;

            } while (fld[i] >= DB_TEXT('0') && fld[i] < DB_TEXT('8'));

            if (fld[i] != DB_TEXT('\''))
            {
                vftprintf(stderr,
                    DB_TEXT("**WARNING** invalid octal value: %s\n"), fld);
                return (wchar_t) num;
            }
            i--;
        }
        else
        {
            escape = -1;
            switch (fld[i])
            {
                case DB_TEXT('n'):   num = L'\n';   break;
                case DB_TEXT('r'):   num = L'\r';   break;
                case DB_TEXT('f'):   num = L'\f';   break;
                case DB_TEXT('t'):   num = L'\t';   break;
                case DB_TEXT('b'):   num = L'\b';   break;
                case DB_TEXT('v'):   num = L'\v';   break;
                default:
#if defined(UNICODE)
                    num = fld[i];
#else
                    cnum[0] = fld[i];
                    cnum[1] = 0;
                    atow(cnum, wnum, sizeof(wnum));
                    num = wnum[0];
#endif
                    break;
            }
        }
    }

    if (fld[i + 1])
        vftprintf(stderr,
            DB_TEXT("**WARNING** invalid character value - %s\n"), fld);

    return (wchar_t) num;
}


/**************************************************************************/

static DB_ADDR read_dba(DB_TCHAR *fld)
{
    int bracket = 0, colon = 0, i = 0;
    unsigned long fno = 0L;  /* need an ulong cuz may be a number instead */
    unsigned long rno = 0L;  /* of [fno:rno] */
    static DB_ADDR temp_dba;

    if (fld[i] == DB_TEXT('['))
    {
        bracket = 1;
        i++;
    }

    while ((bracket == 1) || (fld[i] != 0))
    {
        if (vistdigit(fld[i]))
        {
            if (colon)
                rno = rno * 10 + (fld[i] - DB_TEXT('0'));
            else
                fno = fno * 10 + (fld[i] - DB_TEXT('0'));
        }
        else
        {
            if (fld[i] == DB_TEXT(':'))
                colon = 1;
            else
            {
                if (fld[i] == DB_TEXT(']'))
                    bracket = -1;
            }
        }
        i++;
    }

    if ((bracket == 1) || (fld[i] != 0))
        vftprintf(stderr,
            DB_TEXT("**WARNING** invalid database address - %s\n"), fld);

    if ((bracket == -1) && (colon == 0))
        vftprintf(stderr,
            DB_TEXT("**WARNING** invalid database address - %s\n"), fld);

    if (colon == 0) /* Just a number, no [fno:rno] */
        temp_dba = fno;
    else
        d_encode_dba((short) fno, rno, &temp_dba);

    return temp_dba;
}

/**************************************************************************/

/* move one field */
int mv_fld(char *record, int offset, int ndx, DB_TCHAR *line)
{
    DB_TCHAR *fld_cont;
    int width;
    int blank;
    char   chnum;
    wchar_t wchnum;
    short  shnum;
    int    inum;
    long   lnum = 0;
    float  fnum;
    double dnum = 0.0;
    DB_ADDR dbanum = 0L;
    DB_TASK *task = imp_g.dbtask;

    fld_cont = find_fld(asc_fld, line, &width);

    if (fld_cont == NULL)
        return FAILURE;

    blank = blanks(fld_cont, width);

    switch (task->field_table[ndx].fd_type)
    {
        case CHARACTER:
            if (blank)
                chnum = 0;
            else
                chnum = read_char(fld_cont);
            memcpy(record + offset, &chnum, sizeof(char));
            break;

        case WIDECHAR:
            if (blank)
                wchnum = 0;
            else
                wchnum = read_wchar(fld_cont);
            memcpy(record + offset, &wchnum, sizeof(wchar_t));
            break;

        case SHORTINT:
            if (!blank)
                vstscanf(fld_cont, DB_TEXT("%ld"), &lnum);
            shnum = (short) lnum;
            memcpy(record + offset, &shnum, sizeof(short));
            break;

        case REGINT:
            if (!blank)
                vstscanf(fld_cont, DB_TEXT("%ld"), &lnum);
            inum = (int) lnum;
            memcpy(record + offset, &inum, sizeof(int));
            break;

        case DBADDR:
            if (blank)
                d_encode_dba(0, 0L, &dbanum);
            else
                dbanum = read_dba(fld_cont);
            memcpy(record + offset, &dbanum, sizeof(DB_ADDR));
            break;

        case LONGINT:
            if (!blank)
                vstscanf(fld_cont, DB_TEXT("%ld"), &lnum);
            memcpy(record + offset, &lnum, sizeof(long));
            break;

        case FLOAT:
            if (!blank)
                vstscanf(fld_cont, DB_TEXT("%lf"), &dnum);
            fnum = (float) dnum;
            memcpy(record + offset, &fnum, sizeof(float));
            break;

        case DOUBLE:
            if (!blank)
                vstscanf(fld_cont, DB_TEXT("%lf"), &dnum);
            memcpy(record + offset, &dnum, sizeof(double));
            break;

        default:
            if (!imp_g.silent)
            {
                vftprintf(stderr,
                    DB_TEXT("**WARNING**  unable to transfer into db.* data type %c\n"),
                    task->field_table[ndx].fd_type);
            }
            break;
    }

    asc_fld++;
    return OK;
}

/**************************************************************************/

static DB_TCHAR fld_cont[1024];

DB_TCHAR *find_fld(int fld, DB_TCHAR *line, int *width)
{
    int i, j, commas, nquotes, escaped, new_fld;
    DB_TCHAR *fc, c;

    i = 0;
    commas = 0;
    (*width) = 0;
    fc = fld_cont;
    fld_cont[0] = 0;
    nquotes = 0;
    new_fld = 0;

    /* IF A CHAR FIELD HAS QUOTES AROUND IT, THE CONTENTS OF THE FIELD
       WILL BE WHAT IS BETWEEN THE QUOTES WHILE THE WHITE SPACE OUTSIDE
       THE QUOTES IS IGNORED.  IF THE CHAR FIELD HAS NO QUOTES, EVERYTHING
       INCLUDING THE WHITE SPACE BETWEEN THE COMMAS WILL BE THE CONTENTS
       OF THE FIELD.
    */
    while (commas < fld)
    {
        switch ((c = next_ch(line, &i, &escaped)))
        {
            case DB_TEXT('"'):
                if (escaped)
                {
                    *fc++ = c;
                    *fc = 0;
                    (*width)++;
                }
                else if (nquotes == 1)
                {
                    /* end of quoted string */
                    nquotes = 0;

                    /* IF THERE IS SPACE BETWEEN THE QUOTE AND THE NEXT COMMA,
                        SKIP IT SO THAT IT IS NOT ADDED TO THE STRING.
                    */
                    do
                    {
                        c = next_ch(line, &i, &escaped);
                    } while ((! escaped) && (c != imp_g.sep_char) &&
                                ((c == DB_TEXT(' ')) || (c == DB_TEXT('\t'))));
                    if (c)
                        i--;  /* let the switch handle the non-white char */
                }
                else
                {
                    /* beginning of quoted string */
                    nquotes = 1;
                    (*width) = 0;
                    fld_cont[0] = 0;
                    fc = fld_cont;
                    new_fld = 0;
                }
                break;

            case 0:
                if (nquotes == 1)
                {
                    for (j = 0; j < i - 1; j++)
                        vfputtc(DB_TEXT(' '), stderr);
                    vftprintf(stderr, DB_TEXT("^\n**ERROR** no ending quote\n"));
                    return NULL;
                }
                if (commas <= fld - 2)
                {
                    for (j = 0; j < i - 1; j++)
                        vfputtc(' ', stderr);
                    vftprintf(stderr,
                        DB_TEXT("^\n**ERROR** not enough fields in input record\n"));
                    return NULL;
                }
                if (new_fld)
                {
                    fld_cont[0] = 0;
                    (*width) = 0;
                    fc = fld_cont;
                }
                commas++;
                break;

            default:
                if (c == imp_g.sep_char)
                {
                    if (nquotes == 1)
                    {
                        *fc++ = c;
                        *fc = 0;
                        (*width)++;
                    }
                    else
                    {
                        commas++;
                        if (commas < fld)
                        {
                            (*width) = 0;
                            fld_cont[0] = 0;
                            fc = fld_cont;
                        }
                        new_fld = 1;
                    }
                }
                else
                {
                    if (new_fld)
                    {
                        (*width) = 0;
                        fld_cont[0] = 0;
                        fc = fld_cont;
                        new_fld = 0;
                    }
                    *fc++ = c;
                    *fc = 0;
                    (*width)++;
                }
                break;
        }
    }

    return fld_cont;
}

/**************************************************************************/

static DB_TCHAR next_ch(DB_TCHAR *line, int *pos, int *escaped)
{
    DB_TCHAR retchar, oct[4];
    int i, octint;

    retchar = line[(*pos)];
    *escaped = 0;
    if (retchar == imp_g.esc_char)
    {
        *escaped = 1;
        (*pos)++;
        switch (line[(*pos)])
        {
            case DB_TEXT('n'):   retchar = DB_TEXT('\n');   break;
            case DB_TEXT('r'):   retchar = DB_TEXT('\r');   break;
            case DB_TEXT('t'):   retchar = DB_TEXT('\t');   break;
            case DB_TEXT('f'):   retchar = DB_TEXT('\f');   break;
            case DB_TEXT('b'):   retchar = DB_TEXT('\b');   break;
            default:
                /* convert from \ooo form */
                oct[0] = 0;
                for (i = 0;
                      i < 3 && line[(*pos)] >= DB_TEXT('0') && line[(*pos)] <= DB_TEXT('7');
                      i++, (*pos)++)
                {
                    oct[i] = line[(*pos)];
                    oct[i + 1] = 0;
                }
                if (vtstrlen(oct) > 0)
                {
                    vstscanf(oct, DB_TEXT("%o"), &octint);
                    retchar = (DB_TCHAR) octint;
                    (*pos)--;                    /* done below, causes too far */
                }
                else
                {
                    retchar = line[(*pos)];
                }
                break;
        }
    }
    else if (!retchar)
        return retchar;

    (*pos)++;
    return retchar;
}

/**************************************************************************/

int blanks(DB_TCHAR *s, int n)
{
    while (n--)
    {
        if (*s++ != DB_TEXT(' '))
            return 0;
    }

    return 1;
}

/**************************************************************************/

/* convert vector into index */
int vec_idx(int ndim, short *dim, int *vec, int *idx)
{
    register int i, j, prod;

    *idx = 0;
    if (ndim <= 0)
        return 0;

    for (i = 0; i < ndim - 1; i++)
    {
        prod = 1;
        for (j = i + 1; j < ndim; j++)
            prod *= dim[j];
        *idx += vec[i] * prod;
    }
    *idx += vec[ndim - 1];
    return 0;
}


