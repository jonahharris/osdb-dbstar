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

    dbe_edx.c - DBEDIT, edit hex command

    This file contains all functions for the edit hex command.
    Contents of the current file are read as needed into pages of
    the cache. All changes made during edit hex mode form one
    transaction, which is ended when the user ends hex edit mode,
    or aborted and restarted if the user issues the cancel command.

    The normal command line parser is not used in hex mode, because
    commands & arguments are not necessarily separated by spaces.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_str.h"
#include "dbe_err.h"
#include "dbe_ext.h"

/* ********************** EXTERNAL FUNCTIONS ************************* */

/* ********************** LOCAL VARIABLES **************************** */
static DBE_PAGE curr_page;
static F_ADDR curr_pos;
static int dbe_xtrans;

/* Values for dbe_xtrans - a transaction is started on entry into
   hex edit mode, and if changes are made to any cache pages during
   hex editing then the transaction is ended with d_trend when the
   user quits hex edit mode. If no pages are modified then the
   transaction is aborted with d_trabort.
*/
#define DBE_XNOTRANS  0
#define DBE_XACTIVE   1
#define DBE_XMODIFIED 2

int edit_hex(DB_TASK *task)
{
    DB_TCHAR line[LINELEN], buffer[16], *p, *errstr;
    char     cstring[LINELEN];
    int      error, exit_code, kwd, len;
    long     ln;

    exit_code = 0;
    dbe_xtrans = DBE_XNOTRANS;
    dbe_select(1);
    edx_start(task);

    /* Loop until end command is typed in */
    while (exit_code == 0)
    {
        errstr = NULL;
        vstprintf(line,
            decimal ? DB_TEXT("%06ld:") : DB_TEXT("%05lx:"), curr_pos);
        vtstrcat(line, dbe_getstr(M_HEX));
        if ((error = dbe_getline(line, line, sizeof(line) / sizeof(DB_TCHAR))) >= 0)
        {
            p = getxcomm(line, buffer, sizeof(buffer) / sizeof(DB_TCHAR));
            if (p == line)
                continue;
            if ((kwd = getkwd(buffer, 1)) >= 0)
            {
                switch (kwd)
                {
                    case X_CHARF:           /* Move N characters forward */
                    case X_CHARB:           /* Move N characters backwards */
                    case X_LINEF:           /* Move N lines forward */
                    case X_LINEB:           /* Move N lines backwards */
                    case X_PRINT:           /* Print N lines */
                        if (!(*p))
                        {
                            ln = 1;
                        }
                        else if ((ln = getnum(p)) <= 0)
                        {
                            error = BAD_NUM;
                            errstr = p;
                            break;
                        }
                        if (kwd == X_LINEF || kwd == X_LINEB)
                            ln *= HEX_BYTES;
                        if (kwd == X_CHARB || kwd == X_LINEB)
                            ln = -ln;
                        error = (kwd == X_PRINT) ?
                            edx_print(ln, task) : edx_goto(ln, 1, task);
                        break;

                    case X_GOTO:            /* Jump to position N in file */
                        if (!(*p))
                        {
                            error = UNX_END;
                        }
                        else if ((ln = getnum(p)) < 0)
                        {
                            error = BAD_NUM;
                            errstr = p;
                        }
                        else
                        {
                            error = edx_goto(ln, 0, task);
                        }
                        break;

                    case X_SRCHF:           /* Search forwards for string */
                    case X_SRCHB:           /* Search backwards for string */
                    case X_WRITE:           /* Write string */
                        if (!(*p))
                        {
                            error = UNX_END;
                        }
                        else if (gettext(p, cstring, sizeof(cstring), &len) == p)
                        {
                            error = BAD_STR;
                            errstr = p;
                        }
                        else
                        {
                            error = (kwd == X_WRITE) ?
                                edx_write(cstring, len, task) :
                                edx_search(cstring, len, kwd == X_SRCHF ? 1 : -1, task);
                        }
                        break;

                    case X_CANCEL:          /* Cancel edits */
                        edx_start(task);
                        break;

                    case X_END:             /* Go back to dbedit mode */
                        error = edx_end(task);
                        exit_code = 1;
                        break;

                    case X_QMARK:           /* Help */
                    case X_HELP:
                        help(1, task);
                        break;
                }
            }
            else
            {
                error = BAD_COM;
            }
        }
        if (error)
            dbe_err(error, errstr);
    }
    dbe_select(0);
    return(0);
}


/* Get command from command line - tokens not necessarily separated by spaces
*/
DB_TCHAR *getxcomm(DB_TCHAR *tstring, DB_TCHAR *token, int maxlen)
{
    register int   i;
    DB_TCHAR      *p, *q;

    p = tstring;
    q = token;
    i = 0;

    while (*p && *p <= DB_TEXT(' '))
        p++;
    if (vistalpha(*p))
    {
        while (vistalpha(*p) && i < maxlen - 1)
        {
            *q++ = *p++;
            i++;
        }
    }
    else
    {
        while (*p && *p > DB_TEXT(' ') && !vistalpha(*p) && !vistdigit(*p)
            && *p != DB_TEXT('"') && i < maxlen - 1)
        {
            *q++ = *p++;
            i++;
        }
    }
    *q = 0;
    return (p);
}


/* Get string from command line - may be ascii string in quotes, Unicode
   widechar string in quotes preceded by 'L', or series of hex bytes
   separated by spaces. Returns length in bytes, even if string is widechar
   string.
*/
DB_TCHAR *gettext(DB_TCHAR *in, char *out, int size, int *len)
{
    int       i, hexval;
    DB_TCHAR *start, *p;
    int       wide = 0;
    wchar_t  *wout;
    wchar_t   wstr[2];
    char      cstr[2];

    while (*in && *in <= DB_TEXT(' '))
        in++;
    start = in;

    if (*in == DB_TEXT('L'))
    {
        wide = 1;
        in++;
    }

    if (*in == DB_TEXT('"'))
    {                                   /* Ascii string */
        in++;
        for (i = 0; (*in != DB_TEXT('"')) && (i < size - 1); in++)
        {
            if (!(*in))
                return (start);
            if (*in == DB_TEXT('\\'))
            {
                if (wide)
                {
                    /* wchar_t result required in output string */
                    wout = (wchar_t *) out;
                    switch (*(++in))
                    {
                        case DB_TEXT('n'): *wout = L'\n'; break;
                        case DB_TEXT('r'): *wout = L'\r'; break;
                        case DB_TEXT('t'): *wout = L'\t'; break;
                        case DB_TEXT('b'): *wout = L'\b'; break;

                        case DB_TEXT('0'):
                            if ((in[1] < DB_TEXT('0')) || (in[1] > DB_TEXT('7'))
                             || (in[2] < DB_TEXT('0')) || (in[2] > DB_TEXT('7')))
                            {
                                *wout = 0;
                                break;
                            }

                        default:        /* Octal number */
                            if ( (in[0] >= DB_TEXT('0')) && (in[0] <= DB_TEXT('3'))
                              && (in[1] >= DB_TEXT('0')) && (in[1] <= DB_TEXT('7'))
                              && (in[2] >= DB_TEXT('0')) && (in[2] <= DB_TEXT('7')) )
                            {
                                *wout = (wchar_t) (((in[0] - DB_TEXT('0')) << 6)
                                                      & ((in[1] - DB_TEXT('0')) << 3)
                                                      & (in[2] - DB_TEXT('0')));
                                in += 2;
                            }
                            else
                            {
#if defined(UNICODE)
                                *wout = *in;
#else
                                cstr[0] = *in;
                                cstr[1] = '\0';
                                atow(cstr, wstr, sizeof(wstr)/sizeof(wchar_t));
                                *wout = wstr[0];
#endif
                            }
                            break;
                    }
                }
                else
                {
                    switch (*(++in))
                    {
                        case DB_TEXT('n'): *out = '\n'; break;
                        case DB_TEXT('r'): *out = '\r'; break;
                        case DB_TEXT('t'): *out = '\t'; break;
                        case DB_TEXT('b'): *out = '\b'; break;

                        case DB_TEXT('0'):
                            if ((in[1] < DB_TEXT('0')) || (in[1] > DB_TEXT('7'))
                             || (in[2] < DB_TEXT('0')) || (in[2] > DB_TEXT('7')))
                            {
                                *out = 0;
                                break;
                            }

                        default:        /* Octal number */
                            if ( (in[0] >= DB_TEXT('0')) && (in[0] <= DB_TEXT('3'))
                              && (in[1] >= DB_TEXT('0')) && (in[1] <= DB_TEXT('7'))
                              && (in[2] >= DB_TEXT('0')) && (in[2] <= DB_TEXT('7')) )
                            {
                                *out = (char) (((in[0] - DB_TEXT('0')) << 6)
                                             & ((in[1] - DB_TEXT('0')) << 3)
                                             & (in[2] - DB_TEXT('0')));
                                in += 2;
                            }
                            else
                            {
#if defined(UNICODE)
                                wstr[0] = *in;
                                wstr[1] = 0;
                                wtoa(wstr, cstr, sizeof(cstr));
                                *out = cstr[0];
#else
                                *out = *in;
#endif
                            }
                            break;
                    }
                }
            }
            else if (wide)
            {
                wout = (wchar_t *) out;
#if defined(UNICODE)
                *wout = *in;
            }
            else
            {
                wstr[0] = *in;
                wstr[1] = 0;
                wtoa(wstr, cstr, sizeof(cstr));
                *out = cstr[0];
#else
                cstr[0] = *in;
                cstr[1] = '\0';
                atow(cstr, wstr, sizeof(wstr) / sizeof(wchar_t));
                *wout = wstr[0];
            }
            else
            {
                *out = *in;
#endif
            }
            if (wide)
            {
                out += sizeof(wchar_t);
                i += sizeof(wchar_t);
            }
            else
            {
                out++;
                i++;
            }
        }
        in++;
    }
    else
    {
        for (i = 0, p = in; i < size - 1; i++)
        {                                /* Hex string */
            while ((*p) && (*p == DB_TEXT(' ')))
                p++;
            if (! (*p))
                break;
            if ((*p >= DB_TEXT('0')) && (*p <= DB_TEXT('9')))
                hexval = *p - DB_TEXT('0');
            else if ((*p >= DB_TEXT('a')) && (*p <= DB_TEXT('f')))
                hexval = *p - DB_TEXT('a') + 10;
            else if ((*p >= DB_TEXT('A')) && (*p <= DB_TEXT('F')))
                hexval = *p - DB_TEXT('A') + 10;
            else
                break;
            hexval <<= 4;
            p++;
            if (! (*p))
                break;
            if ((*p >= DB_TEXT('0')) && (*p <= DB_TEXT('9')))
                hexval += *p - DB_TEXT('0');
            else if ((*p >= DB_TEXT('a')) && (*p <= DB_TEXT('f')))
                hexval += *p - DB_TEXT('a') + 10;
            else if ((*p >= DB_TEXT('A')) && (*p <= DB_TEXT('F')))
                hexval += *p - DB_TEXT('A') + 10;
            else
                break;
            *out++ = (char) (hexval & 0xFF);
            in = ++p;
        }
    }
    *out = 0;
    *len = i;
    return (i >= size - 1 ? start : in);
}


/* Start hex edit - set current position to current record or, if none,
   start of current file
*/
int edx_start(DB_TASK *task)
{
    if (dbe_xtrans != DBE_XNOTRANS)
    {
        d_trabort(task);
        dbe_xtrans = DBE_XNOTRANS;
    }
    if (d_trbegin(DB_TEXT("edithex"), task) != S_OKAY)
        return (ERR_READ);
    dbe_xtrans = DBE_XACTIVE;
    curr_pos = dbe_chkdba(task->curr_rec, task) ? 0L : phys_addr(task->curr_rec, task);
    memset(&curr_page, 0, sizeof(DBE_PAGE));
    if (dbe_xread(curr_pos, &curr_page, task))
    {
        if (curr_pos)
        {
            curr_pos = (F_ADDR) 0;
            dbe_xread(curr_pos, &curr_page, task);
        }
    }
    return (0);
}


/* Goto new position in file
*/
int edx_goto(long n, int mode, DB_TASK *task)
{
    long        new_pos;
    DBE_PAGE    new_page;
    int         error;

    new_pos = (mode == 1) ? curr_pos + n : n;
    if (new_pos < 0L)
        return (ERR_FPOS);
    if (  (curr_page.pg_ptr == NULL) || (new_pos < curr_page.start) ||
            (new_pos > curr_page.end) )
    {
        if ((error = dbe_xread(new_pos, &new_page, task)) != 0)
            return (error);
        curr_page = new_page;
    }
    curr_pos = new_pos;
    return (0);
}


/* Print n lines of hex bytes from file, starting from current position
*/
int edx_print(long n, DB_TASK *task)
{
    long        count, page_count, line_count, pos;
    int         error;
    char       *p;
    DBE_PAGE    page;

    if (curr_page.pg_ptr == NULL)
        return (0);

    error = 0;
    pos = curr_pos;
    page = curr_page;
    p = pos - page.start + page.pg_ptr->buff;

    /* Calculate number of bytes to print */
    count = n * HEX_BYTES;

    /* Print until correct number of bytes processed */
    while (count)
    {
        /* Calculate how many bytes to print from this page */
        page_count = (count > page.end - pos + 1) ? page.end - pos + 1 : count;

        /* If end of page, get new page */
        if (!page_count)
        {
            if ((error = dbe_xread(pos, &page, task)) != 0)
                break;

            p = pos - page.start + page.pg_ptr->buff;
        }
        else
        {
            /* Otherwise print till end of page, or till finished */
            while (page_count)
            {
                line_count = page_count > HEX_BYTES ? HEX_BYTES : page_count;
                line_count = do_line(pos, p, line_count, 0);
                count -= line_count;
                page_count -= line_count;
                p += line_count;
                pos += line_count;
            }
        }
    }

    /* Flush buffer in case of incomplete line */
    do_line(pos, p, 0L, 1);

    /* Go back to original position */
    if (dbe_xread(curr_pos, &curr_page, task))
    {
        curr_pos = 0L;
        memset(&curr_page, 0, sizeof(DBE_PAGE));
    }
    return (error);
}


/* Overwrite page contents with string - may affect more than one page
   NOTE: This function does not write to disk - it updates appropriate
   cache pages, which may get swapped out to the transaction log file
   when new pages are read in. The database itself is not updated till
   hex mode is ended.
*/
int edx_write(char *cstring, int len, DB_TASK *task)
{
    int         i, j, error;
    long        n, count;
    F_ADDR      pos;
    DBE_PAGE    page;
    char       *p;

    error = 0;
    count = len;
    pos = curr_pos;
    page = curr_page;

    /* Before starting edit, make sure all affected pages can be
       accessed, and hold them all in cache till write is complete
    */
    if (count > 0 && pos >= page.start && pos < page.end && page.pg_ptr)
        page.pg_ptr->holdcnt++;     /* hold current page in cache */

    for (;;)
    {
        /* calculate number of bytes in this page, from current position */
        n = page.end - pos + 1;
        if (n < 0)
            n = 0;
        count -= n;
        pos += n;

        /* if we're writing less than the number of bytes in page, then
           we don't need any more pages - otherwise get the next page
        */
        if (count <= 0)
            break;
        else if ((error = dbe_xread(pos, &page, task)) == ERR_FPOS)
            error = ERR_EOF;
        if (error)
            return (error);

        /* keep this page in cache till we've copied the data into it */
        page.pg_ptr->holdcnt++;
    }

    /* now go back to current position, and start copying data */
    count = len;
    pos = curr_pos;
    page = curr_page;
    p = pos - page.start + page.pg_ptr->buff;
    i = 0;
    while (count && !error)
    {
        /* calculate number of bytes to copy to this page, then copy them */
        n = count > page.end - pos + 1 ? page.end - pos + 1 : count;
        for (j = 0; j < n; j++)
            p[j] = cstring[i++];
        count -= n;
        pos += n;

        /* finished with this page - don't care if it gets swapped out */
        page.pg_ptr->holdcnt--;

        /* must call d_trend on exiting hex edit mode - this will write
           all modified pages to database
        */
        dbe_xtrans = DBE_XMODIFIED;
        page.pg_ptr->modified = TRUE;

        /* still some more to do - get next page */
        if (count)
        {
            error = dbe_xread(pos, &page, task);  /* holdcnt should already be set */
            p = pos - page.start + page.pg_ptr->buff;
        }
    }

    /* may have swapped out current page - read it back in */
    if (dbe_xread(curr_pos, &curr_page, task))
        return (ERR_READ);

    /* Print changed bytes */
    edx_print(1L, task);
    return (0);
}


/* Search forwards or backwards from current position for string, reading
   new pages as required
*/
int edx_search(char *cstring, int len, int mode, DB_TASK *task)
{
    int         found, error;
    F_ADDR      pos;
    DBE_PAGE    page;
    char       *p;

    found = error = 0;
    pos = curr_pos;
    page = curr_page;

    while (!error)
    {
        p = pos - page.start + page.pg_ptr->buff;

        /* Do not release this page during comparison (edx_comp) */
        page.pg_ptr->holdcnt++;

        if (mode > 0)
        {
            /* Go through page forwards */
            while (!error && pos <= page.end)
            {
                if (*p == *cstring)
                    error = edx_comp(&page, pos, &found, cstring, len, task);
                if (found)
                    break;
                pos++;
                p++;
            }
        }
        else
        {
            /* Go through page backwards */
            while (!error && pos >= page.start)
            {
                if (*p == *cstring)
                    error = edx_comp(&page, pos, &found, cstring, len, task);
                if (found)
                    break;
                pos--;
                p--;
            }
        }

        /* Finished with this page - release it */
        page.pg_ptr->holdcnt--;

        if (found || error)
            break;

        /* Get next page, if there is one */
        if (  (pos < 0L) ||
                ((error = dbe_xread(pos, &page, task)) == ERR_EOF) ||
                (error == ERR_FPOS) )
        {
            error = ERR_SNF;
        }
    }
    if (found)
    {
        curr_pos = pos;
        dbe_xread(curr_pos, &curr_page, task);
        edx_print(1L, task);
    }
    else
    {
        if (!error)
            error = ERR_SNF;
        if (dbe_xread(curr_pos, &curr_page, task))
        {
            curr_pos = (F_ADDR) 0;
            memset(&curr_page, 0, sizeof(DBE_PAGE));
        }
    }
    return (error);
}


/* Compare contents of page, starting at current position, with string
*/
int edx_comp(
    DBE_PAGE *ppage,             /* Current page */
    long pos,                    /* Current position in file */
    int *flagptr,                /* Result of comparison */
    char *cstring,               /* String to compare */
    int len,                     /* Length of string */
    DB_TASK *task)
{
    register int   i, j;
    int            error;
    char           *p;
    DBE_PAGE        page, *pp;

    *flagptr = error = 0;
    pp = ppage;
    p = pos - pp->start + pp->pg_ptr->buff;
    for (i = j = 1; i < len; i++, j++)
    {
        /* Reached the end of this page ? */
        if (pos + i > ppage->end)
        {
            if ((error = dbe_xread(pos + i, pp = &page, task)) != 0)
            {                             /* New page */
                if (error == ERR_EOF || error == ERR_FPOS)
                    error = 0;
                break;
            }
            p = pos + i - pp->start + pp->pg_ptr->buff;
            j = 0;
        }
        if (cstring[i] != p[j])
            break;
    }
    if (i == len)
        *flagptr = 1;
    return (error);
}


/* End hex edit - write all updated pages to disk
*/
int edx_end(DB_TASK *task)
{
    int         error = 0;
    DB_ADDR     here;
    short       fno;
    DB_ULONG    rno;
    DB_TCHAR    line[10];

    /* If we are now positioned (because of moving) within a record
       other than the current record, see if the user wishes to make
       this the current record.
    */
    d_decode_dba(task->curr_rec, &fno, &rno);
    here = phys_to_dba(fno, curr_pos, task);
    if ((here > 0) && (here != task->curr_rec))
    {
        dbe_getline(dbe_getstr(M_MOVE), line, sizeof(line) / sizeof(DB_TCHAR));
        if ((line[0] == DB_TEXT('y')) || (line[0] == DB_TEXT('Y')))
            task->curr_rec = here;
    }
    if (dbe_xtrans == DBE_XMODIFIED)
    {
        if (d_trend(task) == S_OKAY)
            error = dbe_xwrite(task);
        else
            error = ERR_WRIT;
    }
    else if (dbe_xtrans == DBE_XACTIVE)
    {
        d_trabort(task);
    }
    dbe_xtrans = DBE_XNOTRANS;
    return (error);
}


/* Print bytes in hex & ascii, buffering output till complete line is formed.
   Return number of bytes processed.
*/
long do_line(
    long addr,                   /* Address for start of line */
    char *cstring,               /* Byte string to print */
    long count,                  /* Number of bytes to print */
    int flush)                   /* Print even if line is incomplete */
{
#if defined(UNICODE)
    static wchar_t wbuf[HEX_BYTES + 1];
#endif
    static char    cbuf[HEX_BYTES + 1];
    static int     index = 0;
    static long    address;
    DB_TCHAR       hex_line[50];
    int            i;

    if (count + index > HEX_BYTES)
        count = HEX_BYTES - index;
    if (index == 0)
        address = addr;
    for (i = 0; i < count; i++)
        cbuf[index++] = cstring[i];

    /* If buffer's full, or if flag's set & buffer's not empty, print it */
    if ((index == HEX_BYTES) || (flush && (index > 0)))
    {
        do_hex(address, cbuf, hex_line, index);
        for (i = 0; i < index; i++)
        {
            if ((((int) cbuf[i] & 0xFF) < ' ')
                 || ((int) cbuf[i] & 0xFF) > 126)
            {
                cbuf[i] = '.';
            }
        }
        cbuf[i] = '\0';
#if defined(UNICODE)
        atow(cbuf, wbuf, HEX_BYTES + 1);
        wbuf[HEX_BYTES] = 0;
        disp_str(hex_line, wbuf);
#else
        disp_str(hex_line, cbuf);
#endif
        index = 0;
    }
    return (count);
}


