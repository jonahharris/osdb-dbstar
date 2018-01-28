/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbedit utility                                    *
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

    dbe_comm.c - DBEDIT, command line parser

    The parsing function ( parse() ) scans each token in the command
    line, and its arguments, calling itself recursively to check
    successive levels of command. This is purely a syntax check - no
    commands are processed here. The results of the parse are stored
    in an array of token structures.
    This parser is not used in edit hex mode.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_err.h"
#include "dbe_ext.h"


/* ********************** LOCAL VARIABLES **************************** */

static int n_kwd;             /* Size of active keyword table */
static int kwd_1;             /* First keyword in active table */
static DBE_KWD *keywords;     /* Pointer to active keyword table */

#define N_KWD   45            /* Number of keywords/tokens, dbedit mode */
#define KWD_1   9             /* First true keyword in table, dbedit mode */

/* ---------- KEYWORDS MUST BE IN ASCENDING ASCII ORDER -------------- */

static DBE_KWD mainkwds[N_KWD] = {     /* Keyword table for dbedit mode */
    /* If you change this table, you must also */
    /* change K_ definitions in dbe_type.h */
/* First byte */
    {DB_TEXT(""), 0, 1, 1, 0x00, 0x63, 0xC6, 0x03, 0x05, 0xC8},   /* 0x80 Initial state */
    {DB_TEXT(""), 0, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x40 Record name */
    {DB_TEXT(""), 0, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x20 Set name */
    {DB_TEXT(""), 0, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x10 Field name */
    {DB_TEXT(""), 0, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x08 Key name */
    {DB_TEXT(""), 0, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x04 File name */
    {DB_TEXT(""), 0, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x02 Address */
    {DB_TEXT(""), 0, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x01 Decimal number */

/* Second byte */
    {DB_TEXT(""), 0, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x80 Hex number */
    {DB_TEXT("?"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},          /* 0x40 */
    {DB_TEXT("base"), 1, 1, 1, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00},       /* 0x20 */
    {DB_TEXT("count"), 1, 1, 1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},      /* 0x10 */
    {DB_TEXT("dba"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},        /* 0x08 */
    {DB_TEXT("dchain"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},     /* 0x04 */
    {DB_TEXT("display"), 2, 0, 1, 0x00, 0x08, 0x08, 0x20, 0x82, 0x30},    /* 0x02 */
    {DB_TEXT("edit"), 2, 1, 1, 0x00, 0x1C, 0x11, 0x54, 0xE0, 0x10},       /* 0x01 */

/* Third byte */
    {DB_TEXT("exit"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},       /* 0x80 */
    {DB_TEXT("fields"), 3, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},     /* 0x40 */
    {DB_TEXT("file"), 3, 0, 1, 0x05, 0x80, 0x00, 0x00, 0x00, 0x00},       /* 0x20 */
    {DB_TEXT("first"), 3, 1, 1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},      /* 0x10 */
    {DB_TEXT("fld"), 2, 0, 1, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},        /* 0x08 */
    {DB_TEXT("goto"), 1, 1, 1, 0x02, 0x00, 0x30, 0x58, 0x70, 0x00},       /* 0x04 */
    {DB_TEXT("help"), 3, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},       /* 0x02 */
    {DB_TEXT("hex"), 3, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},        /* 0x01 */

/* Fourth byte */
    {DB_TEXT("key"), 1, 0, 1, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00},        /* 0x80 */
    {DB_TEXT("last"), 1, 1, 1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},       /* 0x40 */
    {DB_TEXT("mem"), 1, 0, 1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},        /* 0x20 */
    {DB_TEXT("next"), 4, 1, 1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},       /* 0x18 */
    {DB_TEXT("nextrec"), 5, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},    /* 0x08 */
    {DB_TEXT("nextslot"), 5, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x04 */
    {DB_TEXT("nofields"), 3, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x02 */
    {DB_TEXT("notitles"), 3, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /* 0x01 */

/* Fifth byte */
    {DB_TEXT("opt"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},        /* 0x80 */
    {DB_TEXT("own"), 2, 1, 1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},        /* 0x40 */
    {DB_TEXT("prev"), 4, 1, 1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},       /* 0x20 */
    {DB_TEXT("prevrec"), 5, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},    /* 0x10 */
    {DB_TEXT("record"), 3, 0, 1, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00},     /* 0x08 */
    {DB_TEXT("reread"), 3, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},     /* 0x04 */
    {DB_TEXT("set"), 2, 0, 1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},        /* 0x02 */
    {DB_TEXT("show"), 2, 0, 1, 0x00, 0x00, 0x28, 0x80, 0x0A, 0x00},       /* 0x01 */

/* Sixth byte */
    {DB_TEXT("source"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},     /* 0x80 */
    {DB_TEXT("titles"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},     /* 0x40 */
    {DB_TEXT("ts"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},         /* 0x20 */
    {DB_TEXT("type"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},       /* 0x10 */
    {DB_TEXT("verify"), 1, 1, 1, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00}      /* 0x08 */

};


#define N_XKWD  13                  /* Number of keywords/tokens, hex mode */
#define XKWD_1  0                   /* First true keyword, hex mode */

/* ---------- KEYWORDS MUST BE IN ASCENDING ASCII ORDER -------------- */

static DBE_KWD hexkwds[N_XKWD] = {     /* Keyword table for hex mode */
    /* If you change this table, you must also */
    /* change X_ definitions in dbe_type.h */

    {DB_TEXT("+"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("-"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("<<"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("<"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("="), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT(">>"), 2, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT(">"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("?"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("cancel"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("end"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("help"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("print"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {DB_TEXT("write"), 1, 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

};

/* ********************** FUNCTIONS ********************************** */

/* Select keyword table (switch between dbedit and hex modes)
*/
void dbe_select(int table)
{
    if (table == 0)
    {
        keywords = mainkwds;
        n_kwd = N_KWD;
        kwd_1 = KWD_1;
    }
    else
    {
        keywords = hexkwds;
        n_kwd = N_XKWD;
        kwd_1 = XKWD_1;
    }
}


/* Parse command line recursively
   check no. & types of arguments for each token
*/
int parse(DB_TCHAR **lineptr, int *tokenptr, DBE_TOK *tokens, int max_tok,
          int lastkwd, DB_TCHAR *errstr, int errlen, DB_TASK *task)
{
    DB_TCHAR         *p, *q, *line;
    unsigned char     mask;
    int               i, n, token, lasttoken, kwd, error, max_args, key;
    DB_TCHAR          buffer[NAMELEN];
    long              ln;
    DB_ADDR           dba;

    line = *lineptr;
    token = *tokenptr;
    lasttoken = token - 1;
    error = 0;

    if (token >= max_tok)
        error = ERR_OVF;
    max_args = keywords[lastkwd].max_args;

    for (i = 0; i < max_args && error == 0; i++)
    {
        /* Get next token - return if there aren't any more */
        p = gettoken(line, buffer, sizeof(buffer) / sizeof(DB_TCHAR));
        if (!buffer[0])
        {
            if (i < keywords[lastkwd].min_args)
                error = UNX_END;
            break;
        }

        /* Is current token a keyword, or what ? */
        if ((kwd = getkwd(buffer, 1)) < 0)
        {
            if ((n = getrec(buffer, task)) < 0)
            {
                if ((n = getset(buffer, task)) < 0)
                {
                    if ((n = getfld(buffer, &key, task)) < 0)
                    {
                        if ((n = getfile(buffer, task)) < 0)
                        {
                            if ((ln = getlong(buffer)) < 0L)
                            {
                                if ((ln = gethex(buffer)) < 0L)
                                {
                                    if ((getdba(buffer, &dba)) < 0)
                                    {
                                        error = (getkwd(buffer, 0) < 0) ?
                                            BAD_TOK : BAD_KWD;
                                        for ( i = 0, q = buffer;
                                              *q && i < errlen - 1;
                                              i++, q++, errstr++)
                                        {
                                            *errstr = *q;
                                        }
                                        *errstr = 0;
                                        break;
                                    }
                                    tokens[token].type = TYPE_DBA;
                                    tokens[token].dbaval = dba;
                                    kwd = ADDRESS;
                                }
                                else
                                {
                                    tokens[token].type = TYPE_HEX;
                                    tokens[token].lval = ln;
                                    kwd = HEXNUM;
                                }
                            }
                            else
                            {
                                tokens[token].type = TYPE_NUM;
                                tokens[token].lval = ln;
                                kwd = DECNUM;
                            }
                        }
                        else
                        {
                            tokens[token].type = TYPE_FIL;
                            tokens[token].ival = n;
                            kwd = FILENAME;
                        }
                    }
                    else
                    {
                        tokens[token].type = key ? TYPE_KEY : TYPE_FLD;
                        tokens[token].ival = n;
                        kwd = key ? KEYNAME : FLDNAME;
                    }
                }
                else
                {
                    tokens[token].type = TYPE_SET;
                    tokens[token].ival = n;
                    kwd = SETNAME;
                }
            }
            else
            {
                tokens[token].type = TYPE_REC;
                tokens[token].ival = n;
                kwd = RECNAME;
            }
        }
        else
        {
            tokens[token].type = TYPE_KWD;
            tokens[token].ival = kwd;
        }

        /* Is the token a valid argument of the last one ? */
        mask = (unsigned char) (0x80 >> (kwd % BITS_PER_BYTE));
        if (!(keywords[lastkwd].args[kwd / BITS_PER_BYTE] & mask))
        {
            if (tokens[token].type == TYPE_KWD)
            {
                /* There could be a record, set or field whose name
                    is same as a keyword
                */
                if ((n = getrec(buffer, task)) >= 0)
                {
                    tokens[token].type = TYPE_REC;
                    kwd = RECNAME;
                }
                else if ((n = getset(buffer, task)) >= 0)
                {
                    tokens[token].type = TYPE_SET;
                    kwd = SETNAME;
                }
                else if ((n = getfld(buffer, &key, task)) >= 0)
                {
                    tokens[token].type = key ? TYPE_KEY : TYPE_FLD;
                    kwd = key ? KEYNAME : FLDNAME;
                }
                if (n >= 0)
                {
                    tokens[token].ival = n;
                    mask = (unsigned char) (0x80 >> (kwd % BITS_PER_BYTE));
                }
                else if ((ln = gethex(buffer)) >= 0L)
                {
                    /* If it's not one of the above, it could be a hex
                        number (e.g. ed could be edit or 0xED )
                    */
                    tokens[token].type = TYPE_HEX;
                    tokens[token].lval = ln;
                    mask = (unsigned char) (0x80 >> ((kwd = HEXNUM) %
                            BITS_PER_BYTE));
                }
            }
        }

        if (keywords[lastkwd].args[kwd / BITS_PER_BYTE] & mask)
        {
            tokens[token++].parent = lasttoken;    /* Store it */
            if ((error = parse(&p, &token, tokens, max_tok, kwd,
                                     errstr, errlen, task)) < 0)
            {
                break;
            }
        }
        else
        {
            /* Current token not an argument of last one -
               go back up a level
            */
            break;
        }

        /* Done this token and its arguments - do next one */
        line = p;
    }

    if (error >= 0)
    {
        *lineptr = line;                 /* Current position in command line */
        *tokenptr = token;               /* Current position in token array */
        if (lasttoken < 0)
        {
            p = gettoken(line, buffer, sizeof(buffer) / sizeof(DB_TCHAR));
            if (buffer[0])
            {
                for ( i = 0, q = buffer;
                        *q && i < errlen - 1;
                        i++, q++, errstr++)
                {
                    *errstr = *q;
                }
                *errstr = 0;
                error = UNX_TOK;
            }
        }
    }

    return (error);
}


/* Get first token from input string
*/
DB_TCHAR *gettoken(DB_TCHAR *string, DB_TCHAR *token, int maxlen)
{
    register int   i;
    DB_TCHAR      *p, *q;

    p = string;
    q = token;
    i = 0;

    while (*p && *p <= DB_TEXT(' '))
        p++;

    if (*p)
    {
        while ((*p > DB_TEXT(' ')) && (*p != DB_TEXT(',')) && (i < maxlen - 1))
        {
            *q++ = *p++;
            i++;
        }
        if (*p == DB_TEXT(','))
            p++;
    }
    *q = 0;
    return (p);
}


/* Test whether string is a valid keyword - if so return index in kwd array
*/
int getkwd(DB_TCHAR *string, int check)
{
    register int   i;
    size_t         len;
    int            n;

    len = vtstrlen(string);
    for (i = kwd_1; i < n_kwd; i++)
    {
        if (keywords[i].min_len == 0)
            continue;
        if (check && (len < (size_t) keywords[i].min_len))
            continue;
        if ((n = vtstrnicmp(string, keywords[i].id, len)) == 0)
            return (i);
        if (n < 0)
            break;
    }
    return (-1);
}


/* Test whether string is a database address (NOT whether it's in valid range)
*/
int getdba(DB_TCHAR *string, DB_ADDR *dba_ptr)
{
    DB_TCHAR   *p;
    int         n;
    long        ln;
    FILE_NO     fno;
    F_ADDR      rno;

    p = string;
    if (*p == DB_TEXT('['))
        p++;
    if ((! vstscanf(p, decimal ? DB_TEXT("%d") : DB_TEXT("%x"), &n))
     || (n > FILEMASK) || (n < 0))
        return (-1);
    fno = (FILE_NO) n;
    while (*p && *p != DB_TEXT(':'))
        p++;
    if (!*p)
        return (-1);
    p++;
    if ((! *p)
     || (! vstscanf(p, decimal ? DB_TEXT("%ld") : DB_TEXT("%lx"), &ln))
     || (ln > MAXRECORDS) || (ln < 0L))
    {
        return (-1);
    }
    rno = ln;
    d_encode_dba(fno, rno, dba_ptr);
    return (0);
}


/* Test whether string is a valid (positive) decimal number
*/
long getlong(DB_TCHAR *string)
{
    DB_TCHAR *p;

    if (vtstrlen(string) > NUMLEN)
        return (-1L);
    for (p = string; *p; p++)
    {
        if ((*p < DB_TEXT('0')) || (*p > DB_TEXT('9')))
            return (-1L);
    }
    return (vttol(string));
}


/* Test whether string is a valid (positive) hex number
*/
long gethex(DB_TCHAR *string)
{
    register size_t  i;
    long             n = 0L;
    DB_TCHAR        *p;

    for (p = string, i = 0; *p; p++, i++)
    {
        if (i >= sizeof(n) * 2)
            return (-1L);
        n <<= 4;
        if ((*p >= DB_TEXT('0')) && (*p <= DB_TEXT('9')))
            n += *p - DB_TEXT('0');
        else if ((*p >= DB_TEXT('a')) && (*p <= DB_TEXT('f')))
            n += *p - DB_TEXT('a') + 10;
        else if ((*p >= DB_TEXT('A')) && (*p <= DB_TEXT('F')))
            n += *p - DB_TEXT('A') + 10;
        else
            return (-1L);
    }
    return (n);
}


/* Test whether string is a valid (positive) number
*/
long getnum(DB_TCHAR *string)
{
    while (*string && (*string <= DB_TEXT(' ')))
        string++;
    return (decimal ? getlong(string) : gethex(string));
}


/* Test whether string is a file name - return index in task->file_table or -1
*/
int getfile(DB_TCHAR *name, DB_TASK *task)
{
    register int i;

    /* case sensitive search -- UNIX */
    for (i = 0; i < task->size_ft; i++)
    {
        if (psp_fileNameCmp(name, task->file_table[i].ft_name) == 0)
            return i;
    }
    return -1;
}


/* Test whether string is a record name - return index in task->record_table or -1
*/
int getrec(DB_TCHAR *name, DB_TASK *task)
{
    register int i;
    
    for (i = 0; i < task->size_rt; i++)
    {
        if (vtstricmp(name, task->record_names[i]) == 0)
            return (i);
    }
    return (-1);
}


/* Test whether string is a field name - return index in task->field_table or -1
*/
int getfld(DB_TCHAR *name, int *key, DB_TASK *task)
{
    register int   i;
    DB_TCHAR       SaveChar;
    int            rec;
    DB_TCHAR      *p = NULL;
    int            start = 0, end = 0;

    *key = 0;

    /* look for record.field */
    for (i = 0; i < (int) vtstrlen(name); i++)
    {
        if (name[i] == DB_TEXT('.'))
        {
            SaveChar = name[i];
            name[i] = 0;
            rec = getrec(name, task);
            name[i] = SaveChar;
            if (rec >= 0)
            {
                p = name + i + 1;
                start = task->record_table[rec].rt_fields;
                end = task->record_table[rec].rt_fields + task->record_table[rec].rt_fdtot;
                break;
            }
        }
    }

    if (i == (int) vtstrlen(name))
    {
        p = name;
        start = 0;
        end = task->size_fd;
    }

    /* must be just a field name */
    for (i = start; i < end; i++)
    {
        if (vtstricmp(p, task->field_names[i]) == 0)
        {
            if (task->field_table[i].fd_key != 'n')
                *key = 1;
            return (i);
        }
    }
    return (-1);
}


/* Test whether string is a set name - return index in task->set_table or -1
*/
int getset(DB_TCHAR *name, DB_TASK *task)
{
    register int i;

    for (i = 0; i < task->size_st; i++)
    {
        if (vtstricmp(name, task->set_names[i]) == 0)
            return (i);
    }
    return (-1);
}


