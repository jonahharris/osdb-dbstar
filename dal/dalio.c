/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dal utility                                       *
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

    dalio.c - DAL I/O functions.

    These functions perform the reading and writing of ASCII data
    into/out of db.* records.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "daldef.h"
#include "dalvar.h"

#define DAL
#include "getnames.h"

/* ********************** TYPE DEFINITIONS *************************** */
#define INPUT 0
#define OUTPUT 1

#define DALFILES 20

struct fio
{
    int      io_type;
    FILE    *io_fp;
    DB_TCHAR io_name[15];
};

/* ********************** LOCAL VARIABLE DECLARATIONS **************** */

static DB_TCHAR line[90];
static struct fio f[DALFILES];

/* ********************** LOCAL FUNCTION DECLARATIONS **************** */
static int getfield(int, char *, DB_TASK *);
static int putfield(int, char *, DB_TASK *);

/* ********************** LOCAL VARIABLE DECLARATIONS **************** */
static int EOLN = 0;                   /* used for detecting End Of Line */
static int nio = 0;
static FILE *fp;

extern int dal_unicode;

/* ------------------------------------------------------------------------ */

void dalio_close()
{
    int j;
    
    for (j = 0; j < nio; j++)
    {
        if (f[j].io_fp != NULL)
        {
            fclose(f[j].io_fp);
            f[j].io_fp = NULL;
            f[j].io_name[0] = 0;
        }
    }
    nio = 0;
    fp = NULL;
    EOLN = 0;
}

/* ------------------------------------------------------------------------ */

int dalio(INST *pi)
{
    PRINTFIELD    *p;
    char          *val;
    int            type = 0,
                   ndx,
                   j,
                   fldnum,
                   intval;
    DB_TINT        c;
    DB_ADDR        dba;
    int            file_no;
    F_ADDR         rec_no;
    clock_t        ctval;
    short          file;
    DB_ADDR        slot;
    DB_TCHAR      *pmode;
    DB_TASK       *task = DalDbTask; /* for macros, e.g. task->field_table */
    int            out  = vtstrcmp(pi->i_name, DB_TEXT("print")) == 0;
    int            stat = S_OKAY;

    if (vtstrlen(pi->i_p1))
    {
        for (j = 0; j < nio; j++)
        {
            if (vtstrcmp(pi->i_p1, f[j].io_name) == 0)
                break;
        }

        if (j == nio)
        {
            if (nio >= DALFILES)
            {
                vftprintf(stderr,
                    DB_TEXT("Unable to open file %s, file limit reached\n"),
                    pi->i_p1);
                return (DAL_ERR);
            }

            vtstrcpy(f[nio].io_name, pi->i_p1);

            if (out)
                pmode = dal_unicode ? DB_TEXT("wb") : DB_TEXT("w");
            else
                pmode = dal_unicode ? DB_TEXT("rb") : DB_TEXT("r");

            if ((f[nio].io_fp = vtfopen(pi->i_p1, pmode)) == NULL)
            {
                vftprintf(stderr, DB_TEXT("Unable to open file %s for %s\n"),
                    pi->i_p1, out ? DB_TEXT("output") : DB_TEXT("input"));
                return (DAL_ERR);
            }
            fp = f[nio].io_fp;
            f[nio++].io_type = out ? OUTPUT : INPUT;
        }
        else
        {
            if (out)
            {
                if (f[j].io_type == OUTPUT)
                {
                    fp = f[j].io_fp;
                }
                else
                {
                    fclose(f[j].io_fp);
                    pmode = dal_unicode ? DB_TEXT("wb") : DB_TEXT("w");
                    if ((f[j].io_fp = vtfopen(pi->i_p1, pmode)) == NULL)
                    {
                        vftprintf(stderr,
                            DB_TEXT("Unable to open file %s for output\n"), pi->i_p1);
                        return (DAL_ERR);
                    }
                    fp = f[j].io_fp;
                    f[j].io_type = OUTPUT;
                    vtprintf(DB_TEXT("File '%s' re-opened for output\n"), pi->i_p1);
                }
            }
            else
            {
                if (f[j].io_type == INPUT)
                {
                    fp = f[j].io_fp;
                }
                else
                {
                    fclose(f[j].io_fp);
                    pmode = dal_unicode ? DB_TEXT("rb") : DB_TEXT("r");
                    if ((f[j].io_fp = vtfopen(pi->i_p1, pmode)) == NULL)
                    {
                        vftprintf(stderr,
                            DB_TEXT("Unable to open file %s for input\n"),
                            pi->i_p1);
                        return (DAL_ERR);
                    }
                    fp = f[j].io_fp;
                    f[j].io_type = INPUT;
                    vtprintf(DB_TEXT("File '%s' re-opened for input\n"), pi->i_p1);
                }
            }
        }
    }
    else
    {
        fp = out ? stdout : stdin;
    }

    p = pi->i_pfld;

    while (p)
    {
        for (j = BEG_PRINTABLE; j <= END_PRINTABLE; j++)
        {
            if ((val = findvar(j, p->pf_rec, &ndx)) != NULL)
            {
                type = j;
                break;
            }
        }
        if (val == NULL)
        {
            vftprintf(stderr, DB_TEXT("Undefined variable - %s\n"), p->pf_rec);
            return (DAL_ERR);
        }

        /* can not get here without type getting set, so ignore the warning */
        if (type == RECORD)
        {
            if (vtstrlen(p->pf_fld))
            {
                type = FIELD;
                fldnum = (int) getfld(p->pf_fld, ndx, task);
                if (fldnum == -1)
                {
                    vftprintf(stderr,
                        DB_TEXT("Field name '%s' not in database\n"), p->pf_fld);
                    return (DAL_ERR);
                }
                val = val + task->field_table[fldnum].fd_ptr -
                                task->record_table[ndx].rt_data;
                ndx = fldnum;
            }
        }
        if (!out && fp == stdin)
        {
            vtprintf(DB_TEXT("%s"), p->pf_rec);
            if (vtstrlen(p->pf_fld))
                vtprintf(DB_TEXT(".%s: "), p->pf_fld);
            else
                vtprintf(DB_TEXT(": "));
        }

        switch (type)
        {
            case RECORD:
                c = 0;
                if ((! out) && (fp == stdin))
                    vtprintf(DB_TEXT("\n"));

                for (j = 0; j < task->record_table[ndx].rt_fdtot; j++)
                {
                    fldnum = task->record_table[ndx].rt_fields + j;
                    if (task->field_table[fldnum].fd_type == GROUPED)
                        continue;
                    if (out)
                    {
                        stat = putfield(fldnum, val + task->field_table[fldnum].fd_ptr -
                                        task->record_table[ndx].rt_data, task);
                    }
                    else
                    {
                        if (fp == stdin)
                            vtprintf(DB_TEXT("   %s: "), task->field_names[fldnum]);

                        stat = getfield(fldnum, val + task->field_table[fldnum].fd_ptr -
                                        task->record_table[ndx].rt_data, task);

                        if (fp == stdin)
                        {
                            if (! EOLN)
                            {
                                do
                                {
                                    c = vgettc(fp);
                                } while ((c != DB_TEXT('\n')) && (c != DB_TEOF));
                            }
                            else
                            {
                                EOLN = 0;
                            }
                        }
                    }
                    if (stat)
                        return (stat);
                }
                if ((! out) && (fp != stdin))
                {
                    if (! EOLN)
                    {
                        do
                        {
                            c = vgettc(fp);
                        } while ((c != DB_TEXT('\n')) && (c != DB_TEOF));
                    }
                    else
                    {
                        EOLN = 0;
                    }
                }
                if (c == DB_TEOF)
                    return (S_EOS);
                break;

            case FIELD:
                if (task->field_table[ndx].fd_type == COMKEY)
                {
                    FIELD_ENTRY  *cfld_ptr = &task->field_table[ndx];
                    KEY_ENTRY    *key_ptr;
                    int           kt_lc;

                    if ((! out) && (fp == stdin))
                        vtprintf(DB_TEXT("\n"));

                    for ( kt_lc = task->size_kt - cfld_ptr->fd_ptr,
                              key_ptr = &task->key_table[cfld_ptr->fd_ptr];
                          (--kt_lc >= 0) && (key_ptr->kt_key == ndx);
                          ++key_ptr)
                    {
                        if (out)
                        {
                            stat = putfield(key_ptr->kt_field,
                                                 val + key_ptr->kt_ptr, task);
                        }
                        else
                        {
                            if (fp == stdin)
                                vtprintf(DB_TEXT("   %s: "), task->field_names[key_ptr->kt_field]);

                            stat = getfield(key_ptr->kt_field,
                                            val + key_ptr->kt_ptr, task);

                            if (fp == stdin)
                            {
                                if (!EOLN)
                                {
                                    do
                                    {
                                        c = vgettc(fp);
                                    } while ((c != DB_TEXT('\n')) && (c != DB_TEOF));
                                }
                                else
                                {
                                    EOLN = 0;
                                }
                            }
                        }
                    }
                }
                else
                {
                     stat = out ? putfield(ndx, val, task) :
                                  getfield(ndx, val, task);
                }
                if (stat)
                    return (stat);
                break;

            case CLOCK_T:
                if (out)
                {
                     memcpy(&ctval, val, sizeof(clock_t));
                     vftprintf(fp, DB_TEXT("%4.2f seconds"), (float)ctval/CLOCKS_PER_SEC);
                }
                break;

            case LITERAL:
                if (out)
                    vftprintf(fp, (DB_TCHAR *)val);
                break;

            case INTPTR:
                if (out)
                {
                    memcpy(&intval, val, sizeof(int));
                    vftprintf(fp, DB_TEXT("%6d"), intval);
                }
                else
                {
                    if (stat = gettstring(line, 7, fp))
                        return (stat);
                    vstscanf(line, DB_TEXT("%d"), (int *) val);
                }
                break;

            case DBAPTR:
                if (out)
                {
                    memcpy(&dba, val, DB_ADDR_SIZE);
                    d_decode_dba(dba, &file, &slot);
                    vftprintf(fp, DB_TEXT("[%d:%lu]"), (int)file, slot);
                }
                else
                {
                    if (fp == stdin)
                        vtprintf(DB_TEXT("\n   Enter file_no: "));
                    stat = gettstring(line, 7, fp);
                    if (stat)
                        return (stat);
                    vstscanf(line, DB_TEXT("%d"), &file_no);
                    if (fp == stdin)
                        vtprintf(DB_TEXT("   Enter rec_no : "));
                    stat = gettstring(line, 14, fp);
                    if (stat)
                        return (stat);
                    vstscanf(line, DB_TEXT("%ld"), &rec_no);
                    d_encode_dba((short)file_no, rec_no, &dba);
                    memcpy(val, &dba, DB_ADDR_SIZE);
                }
                break;

            default:
                break;
        }
        p = p->pf_next;
    }
    
    if (out)
    {
        vftprintf(fp, DB_TEXT("\n"));
        fflush(fp);
    }
    
    return (stat);
}

/* ------------------------------------------------------------------------ */

static int getfield(int ndx, char *val, DB_TASK *task)
{
    short       shortval;
    int         intval,
                stat,
                len;
    long        longval;
    float       flval;
    double      dbval;

    stat = S_OKAY;

    switch (task->field_table[ndx].fd_type)
    {
        case CHARACTER:
            stat = getcstring(val, task->field_table[ndx].fd_len + 1, fp);
            len = strlen(val);
            if (len && val[len-1] == '\n')
                val[len-1] = '\0';
            break;

        case WIDECHAR:
            stat = getwstring((wchar_t *)val,
                (task->field_table[ndx].fd_len / sizeof(wchar_t)) + 1, fp);
            len = vwcslen((wchar_t *)val);
            if (len && val[len-1] == L'\n')
                val[len-1] = 0;
            break;

        case SHORTINT:
            stat = gettstring(line, 7, fp);
            vstscanf(line, DB_TEXT("%hd"), &shortval);
            memcpy(val, &shortval, sizeof(short));
            break;

        case REGINT:
            stat = gettstring(line, 7, fp);
            vstscanf(line, DB_TEXT("%d"), &intval);
            memcpy(val, &intval, sizeof(int));
            break;

        case LONGINT:
            stat = gettstring(line, 11, fp);
            vstscanf(line, DB_TEXT("%ld"), &longval);
            memcpy(val, &longval, sizeof(long));
            break;

        case FLOAT:
            stat = gettstring(line, 11, fp);
            vstscanf(line, DB_TEXT("%f"), &flval);
            memcpy(val, &flval, sizeof(float));
            break;

        case DOUBLE:
            stat = gettstring(line, 11, fp);
            vstscanf(line, DB_TEXT("%lf"), &dbval);
            memcpy(val, &dbval, sizeof(double));
            break;
    }
    return (stat);
}

/* ------------------------------------------------------------------------ */

static int putfield(int ndx, char *val, DB_TASK *task)
{
    DB_TCHAR    fmt[10];
    short       shortval;
    int         intval;
    long        longval;
    float       flval;
    double      dbval;
#if defined(UNICODE)
    wchar_t     wbuffer[FILENMLEN];
#else
    char        cbuffer[FILENMLEN];
#endif

    switch (task->field_table[ndx].fd_type)
    {
        case CHARACTER:
            vstprintf(fmt, DB_TEXT("%%-%d.%ds"),
                task->field_table[ndx].fd_len, task->field_table[ndx].fd_len);
#if defined(UNICODE)
            atow(val, wbuffer, sizeof(wbuffer) / sizeof(wchar_t));
            wbuffer[sizeof(wbuffer) / sizeof(wchar_t) - 1] = 0;
            vftprintf(fp, fmt, wbuffer);
#else
            vftprintf(fp, fmt, val);
#endif
            break;

        case WIDECHAR:
            vstprintf(fmt, DB_TEXT("%%-%d.%ds"),
                task->field_table[ndx].fd_len / sizeof(wchar_t),
                task->field_table[ndx].fd_len / sizeof(wchar_t));
#if defined(UNICODE)
            vftprintf(fp, fmt, val);
#else
            wtoa((wchar_t *)val, cbuffer, sizeof(cbuffer));
            cbuffer[sizeof(cbuffer) - 1] = '\0';
            vftprintf(fp, fmt, cbuffer);
#endif
            break;

        case SHORTINT:
            memcpy(&shortval, val, sizeof(short));
            vftprintf(fp, DB_TEXT("%6d"), shortval);
            break;

        case REGINT:
            memcpy(&intval, val, sizeof(int));
            vftprintf(fp, DB_TEXT("%6d"), intval);
            break;

        case LONGINT:
            memcpy(&longval, val, sizeof(long));
            vftprintf(fp, DB_TEXT("%10ld"), longval);
            break;

        case FLOAT:
            memcpy(&flval, val, sizeof(float));
            vftprintf(fp, DB_TEXT("%10.10g"), flval);
            break;

        case DOUBLE:
            memcpy(&dbval, val, sizeof(double));
            vftprintf(fp, DB_TEXT("%10.10g"), dbval);
            break;
    }
    return S_OKAY;
}

/* ------------------------------------------------------------------------ */

int getcstring(char *str, int len, FILE *fp)
{
    int c = 0;
    char *save;

    save = str;

    while (--len > 0)
    {
        c = getc(fp);
        if (c == EOF)
            break;
        if (c == '\n')
        {
            EOLN = 1;
            break;
        }
        *str++ = (char) c;
    }

    if (len)
        *str = 0;
    else
        *--str = 0;

    len = strlen(save);
    while (len && (save[len - 1] == ' '))
    {
        save[len - 1] = 0;
        len--;
    }

    if (c == EOF)
        return (S_EOS);
    else
        return (S_OKAY);
}

/* ------------------------------------------------------------------------ */


int getwstring(wchar_t *str, int len, FILE *fp)
{
    wint_t c = 0;
    wchar_t *save;

    save = str;

    while (--len > 0)
    {
        c = vgetwc(fp);
        if (c == DB_WEOF)
            break;
        if (c == L'\n')
        {
            EOLN = 1;
            break;
        }
        *str++ = (wchar_t) c;
    }

    if (len)
        *str = 0;
    else
        *--str = 0;

    len = vwcslen(save);
    while (len && (save[len - 1] == L' '))
    {
        save[len - 1] = 0;
        len--;
    }

    if (c == DB_WEOF)
        return (S_EOS);
    else
        return (S_OKAY);
}


/* ------------------------------------------------------------------------ */

int dal_rewind(INST *pi)
{
    int found, j;
    DB_TCHAR *pmode;

    /* check to see if it's the right command */
    if (vtstrcmp(pi->i_name, DB_TEXT("rewind")) != 0)
        return (DAL_ERR);

    /* check if name is on list of files being used */
    for (found = FALSE, j = 0; j < nio; j++)
    {
        if (!vtstrcmp(pi->i_p1, f[j].io_name))
        {
            found = TRUE;
            break;
        }
    }
    if (!found)
    {
        vftprintf(stderr, DB_TEXT("file '%s' not rewound\n"), pi->i_p1);
        return (DAL_ERR);
    }
    else if (f[j].io_type == INPUT)
    {                                   /* it's an input file */
        if (f[j].io_fp != NULL)
        {                                /* it's open */
            if (fseek(f[j].io_fp, (long) 0, 0) != 0)
            {
                vftprintf(stderr, DB_TEXT("Problem rewinding file - %s\n"),
                          f[j].io_name);
                return (DAL_ERR);
            }
        }
        else
        {
            /* file's closed so let's open it */
            pmode = dal_unicode ? DB_TEXT("rb") : DB_TEXT("r");
            if ((f[j].io_fp = vtfopen(f[j].io_name, pmode)) == NULL)
            {
                vftprintf(stderr, DB_TEXT("Problem rewinding file - %s\n"),
                          f[j].io_name);
                return (DAL_ERR);
            }
        }
    }
    else
    {                                   /* file is an output file */
        if (f[j].io_fp != NULL)          /* it's open */
            fclose(f[j].io_fp);
        pmode = dal_unicode ? DB_TEXT("wb") : DB_TEXT("w");
        if ((f[j].io_fp = vtfopen(f[j].io_name, pmode)) == NULL)
        {
            vftprintf(stderr, DB_TEXT("Problem rewinding file - %s\n"),
                      f[j].io_name);
            return (DAL_ERR);
        }
    }
    return (S_OKAY);
}


