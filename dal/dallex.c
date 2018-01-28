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

    dallex.c - DAL lexical analyzer

    This function will read the input stream until it recognizes
    a token.  The token value and type will be passed back to
    the parser.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */

#include "db.star.h"
#include "daldef.h"
#include "parser.h"
#include "y.tab.h"

/* ********************** TYPE DEFINITIONS *************************** */
#define MAXSTACK 20
struct key_entry
{
    int lx;
    DB_TCHAR id[12];
};

/* ********************** GLOBAL VARIABLE DECLARATIONS *************** */
struct key_entry keywords[] = {
    {T_WHILEOK,    DB_TEXT("whileok")},
    {T_PRINT,      DB_TEXT("print")},
    {T_INPUT,      DB_TEXT("input")},
    {T_SCHEMA,     DB_TEXT("schema")},
    {T_REWIND,     DB_TEXT("rewind")},
    {T_CURRENCY,   DB_TEXT("curr")},
    {T_CURRENCY,   DB_TEXT("currency")},
    {T_EXIT,       DB_TEXT("exit")},
    {T_ABORT,      DB_TEXT("abort")},
    {0,            DB_TEXT("")}
};
static int pbstack[MAXSTACK];          /* pushed back input stack */
static int bp = -1;                    /* pbstack pointer */
static DB_TINT ch = DB_TEXT('\n');     /* Last input character */
int line = 0;                          /* current input line number */
int tot_errs = 0;                      /* total errors encountered */

/* ********************** EXTERNAL VARIABLE DECLARATIONS ************* */
extern FILE   *fdal;
extern int     yydebug;
extern int     yychar;
extern int     batch;


static int input(void);
static int unput(int);


void init_dallex()
{
    /* for VxWorks - initialize globals explicitly */
    memset(pbstack, 0, sizeof(pbstack));
    bp = -1;
    ch = DB_TEXT('\n');
    line = 0;
    tot_errs = 0;
}


/* Lexical Analyzer
*/
int yylex()
{

    int   state = 0,
          i = 0,
          n;

    for (;;)
    {
        switch (state)
        {
            case 0:
                ch = input();
                if (vistspace(ch))
                    state = 0;
                else if (ch == DB_TEXT('#'))
                    state = 1;
                else if (ch == DB_TEXT('/'))
                    state = 2;
                else if (ch == DB_TEOF)
                    return (-1);
                else
                    state = 5;
                break;

                /* handling # comments */
            case 1:
                if ((ch = input()) != DB_TEXT('\n'))
                    state = 1;
                else
                    state = 0;
                break;

                /* handling C style comments */
            case 2:
                if ((ch = input()) == DB_TEXT('*'))
                    state = 3;
                else
                    state = 5;
                break;

            case 3:
                if ((ch = input()) == DB_TEXT('*'))
                    state = 4;
                else
                    state = 3;
                break;

            case 4:
                if ((ch = input()) == DB_TEXT('/'))
                {
                    state = 0;
                }
                else
                {
                    if (unput(ch))
                        return (-1);
                    state = 3;
                }
                break;

                /* handling identifiers and keywords */
            case 5:
                if (vistalnum(ch))
                {
                    yylval.tstr.str[i = 0] = (DB_TCHAR) ch;
                    state = 6;
                }
                else
                {
                    state = 7;
                }
                break;

            case 6:
                /* 'i' was set by state 5 */
                for (ch = input();
                      vistalnum(ch) || (ch == DB_TEXT('_'));
                      ch = input())
                {
                    if (i < NAMELEN - 2)
                        yylval.tstr.str[++i] = (DB_TCHAR) ch;
                }
                yylval.tstr.str[++i] = 0;
                if (unput(ch))
                    return (-1);

                /* Look up identifier in keywords */
                for (i = 0; keywords[i].lx; i++)
                {
                    if (vtstrcmp(keywords[i].id, yylval.tstr.str) == 0)
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

                /* handling integers */
            case 7:
                if (vistdigit(ch))
                {
                    n = 0;
                    while (vistdigit(ch))
                    {
                        n = 10 * n + ch - DB_TEXT('0');
                        ch = input();
                    }
                    if (unput(ch))
                        return (-1);
                    state = 0;
                }
                else
                {
                    state = 8;
                }
                break;

                /* handling quoted strings */
            case 8:
                if (ch == DB_TEXT('"'))
                {
                    for (i = 0, ch = input(); ch != DB_TEXT('"'); ch = input(), i++)
                        yylval.tstr.str[i] = (DB_TCHAR) ch;
                    yylval.tstr.str[i] = 0;
                    yylval.tstr.strline = line;
                    return (T_STRING);
                }
                else
                {
                    state = 9;
                }
                break;

            case 9:
                yylval.tnum.num = ch;
                yylval.tnum.numline = line;
                if ((ch = input()) != DB_TEXT('\n'))
                    if (unput(ch))
                        return (-1);
                return (yylval.tnum.num);

            default:
                break;
        }
    }
}

/* Input next character
*/
static int input()
{
    int ch;

    if (bp >= 0)
    {
        ch = pbstack[bp--];
    }
    else
    {
        ch = vgettc(fdal);
        if (ch == DB_TEXT('\n'))
            line++;
    }
    return (ch);
}

/* Put character back on input
*/
static int unput(int ch)
{
    if (++bp > MAXSTACK)
    {
        vftprintf(stderr, DB_TEXT("pbstack overflow\n"));
        return(-1);
    }
    else
    {
        pbstack[bp] = ch;
    }
    return(0);
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
    ++tot_errs;
    if (batch)
        vftprintf(stderr, DB_TEXT("line %d: %s\n"), ln, s);
    else
        vftprintf(stderr, DB_TEXT("%s\n"), s);
}


