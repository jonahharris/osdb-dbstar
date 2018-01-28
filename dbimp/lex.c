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

/*-----------------------------------------------------------------------

    lex.c - Lexical analyzer for dbimp

    This function will select tokens from the import specification
    and give them to the parser.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "parser.h"
#include "impdef.h"
#include "impvar.h"
#include "y.tab.h"

static int input(void);
static void unput(int);

/* ********************** TYPE DEFINITIONS *************************** */
#define MAXSTACK 20

/* ********************** GLOBAL VARIABLE DECLARATIONS *************** */
struct key_entry
{
    int lx;
    DB_TCHAR id[12];
};

static struct key_entry keywords[] = {
    {T_DATABASE,   DB_TEXT("database")},
    {T_FOR,        DB_TEXT("foreach")},
    {T_RECORD,     DB_TEXT("record")},
    {T_FIELD,      DB_TEXT("field")},
    {T_CONNECT,    DB_TEXT("connect")},
    {T_END,        DB_TEXT("end")},
    {T_CREATE,     DB_TEXT("create")},
    {T_UPDATE,     DB_TEXT("update")},
    {T_FIND,       DB_TEXT("find")},
    {T_ON,         DB_TEXT("on")},
    {0,            DB_TEXT("")}
};

static int pbstack[MAXSTACK];          /* pushed back input stack */
static int bp = -1;                    /* pbstack pointer */
static DB_TINT ch = DB_TEXT('\n');     /* Last input character */
static int line = 0;                   /* current input line number */


/* Lexical Analyzer
*/
int yylex()
{
    register int i;

    for (;;)
    {
        while (ch <= DB_TEXT(' '))
        {
            while (ch == DB_TEXT('\n'))
            {
                ++line;
                if ((ch = input()) == DB_TEXT('#'))
                {                          /* ignore line */
                    while ((ch = input()) == DB_TEXT(' '))
                        ;
                    if (vistdigit(ch))
                    {
                        register int n;

                        n = 0;
                        while (vistdigit(ch))
                        {
                            n = 10 * n + ch - DB_TEXT('0');
                            ch = input();
                        }
                        line = n - 1;
                    }
                    while ((ch = input()) != DB_TEXT('\n'))
                        ;
                }
                else if (ch != DB_TEXT('\n'))
                    unput(ch);

                if (imp_g.abort_flag)
                    return (-1);
            }
            if (ch == DB_TEOF)
                return (-1);
            ch = input();
        }
        if (vistalpha(ch))
        {
            /* Build identifer */
            for (yylval.tstr.str[i = 0] = (DB_TCHAR) ch;
                    ch = input(), (vistalnum(ch) || (ch == DB_TEXT('_'))); )
            {
                if (i < NAMELEN - 1)
                    yylval.tstr.str[++i] = (DB_TCHAR) ch;
            }
            yylval.tstr.str[++i] = 0;

            /* Look up identifier in keywords */
            for (i = 0; keywords[i].lx; i++)
            {
                if (vtstricmp(keywords[i].id, yylval.tstr.str) == 0)
                    break;
            }
            if (keywords[i].lx)
            {
                yylval.tnum.numline = line;
                return (keywords[i].lx);
            }
            else
            {
                yylval.tstr.strline = line;
                return (T_IDENT);
            }
        }
        else if (vistdigit(ch))
        {
            yylval.tnum.num = 0;
            while (vistdigit(ch))
            {
                yylval.tnum.num = 10 * yylval.tnum.num + ch - DB_TEXT('0');
                ch = input();
            }
            yylval.tnum.numline = line;
            return (T_NUMBER);
        }
        else if (ch == DB_TEXT('"'))
        {
            for (i = 0; (ch = input()) != DB_TEXT('"'); i++)
                yylval.tstr.str[i] = (DB_TCHAR) ch;
            yylval.tstr.str[i] = 0;
            ch = input();
            yylval.tstr.strline = line;
            return (T_STRING);
        }
        else if (ch == DB_TEXT('/'))
        {
            if ((ch = input()) == DB_TEXT('*'))
            {
                /* ignore comment */
                for (;;)
                {
                    while ((ch = input()) != DB_TEXT('*'))
                    {
                        if (ch == DB_TEXT('\n'))
                            ++line;
                    }
                    if ((ch = input()) == DB_TEXT('/'))
                        break;
                    else
                        unput(ch);

                    if (imp_g.abort_flag)
                        return (-1);
                }
                ch = input();
                continue;
            }
            else
            {
                unput(ch);
                ch = DB_TEXT('/');
            }
            if (imp_g.abort_flag)
                return (-1);
        }
        yylval.tnum.num = ch;
        ch = input();
        yylval.tnum.numline = line;
        return (yylval.tnum.num);
    }
}


/* Input next character
*/
static int input()
{
    int c;

    if (bp >= 0)
        c = pbstack[bp--];
    else
        c = vgettc(imp_g.fspec);
    return (c);
}

/* Put character back on input
*/
static void unput(int c)
{
    if (++bp > MAXSTACK)
    {
        vftprintf(stderr, DB_TEXT("pbstack overflow\n"));
        imp_g.abort_flag = 1;
    }
    else
        pbstack[bp] = c;
}


/* Syntax error
*/
void yyerror(DB_TCHAR *s)
{
    dderror(s, line);
}


/* Data definition error
*/
void dderror(DB_TCHAR *s, int ln)
{
    ++imp_g.tot_errs;
    vftprintf(stderr, DB_TEXT("%s %d 0 : %s\n"), imp_g.specname, ln, s);
}


/* Abort ddlp
*/
void dbimp_abort(DB_TCHAR *s)
{
    vftprintf(stderr, DB_TEXT("Fatal error: %s\n"), s);
    vftprintf(stderr, DB_TEXT("\nImport terminated\n"));
    imp_g.abort_flag = 1;
}

void ddwarning(DB_TCHAR *msg)
{
    vftprintf(stderr, DB_TEXT("**WARNING** %s (line %d)\n"), msg, line);
    imp_g.tot_warnings++;
}


