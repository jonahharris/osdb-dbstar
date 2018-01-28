/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ddlp utility                                      *
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

    ddllex.c - ddlp lexical analyzer.

    This function will read input from the ddlp_g.schema file and return
    a 'token' value when it recognizes it in the input.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "parser.h"
#include "ddldefs.h"
#include "y.tab.h"

/* ********************** TYPE DEFINITIONS *************************** */
#define MAXSTACK 20

struct key_entry {
    int lx;
    DB_TCHAR id[12];
};

/* ********************** GLOBAL VARIABLE DECLARATIONS *************** */
static struct key_entry keywords[] = {
    {T_ASCENDING,  DB_TEXT("asc")},
    {T_ASCENDING,  DB_TEXT("ascending")},
    {T_BITMAP,     DB_TEXT("bitmap")},
    {T_BLOB,       DB_TEXT("blob")},
    {T_BY,         DB_TEXT("by")},
    {T_COMPOUND,   DB_TEXT("compound")},
    {T_CONST,      DB_TEXT("const")},
    {T_CONTAINS,   DB_TEXT("contains")},
    {T_DATA,       DB_TEXT("data")},
    {T_DATABASE,   DB_TEXT("database")},
    {T_DEFINE,     DB_TEXT("#define")},
    {T_DESCENDING, DB_TEXT("desc")},
    {T_DESCENDING, DB_TEXT("descending")},
    {T_DIRECTREF,  DB_TEXT("directref")},
    {T_FILE,       DB_TEXT("file")},
    {T_FIRST,      DB_TEXT("first")},
    {T_INCLUDE,    DB_TEXT("#include")},
    {T_KEY,        DB_TEXT("key")},
    {T_LAST,       DB_TEXT("last")},
    {T_LINE,       DB_TEXT("#line")},
    {T_MEMBER,     DB_TEXT("member")},
    {T_NEXT,       DB_TEXT("next")},
    {T_OPTIONAL,   DB_TEXT("opt")},
    {T_OPTIONAL,   DB_TEXT("optional")},
    {T_ORDER,      DB_TEXT("order")},
    {T_OWNER,      DB_TEXT("owner")},
    {T_RECORD,     DB_TEXT("record")},
    {T_RECS,       DB_TEXT("records")},
    {T_RELATEDTO,  DB_TEXT("relatedto")},
    {T_SET,        DB_TEXT("set")},
    {T_SETS,       DB_TEXT("sets")},
    {T_STATIC,     DB_TEXT("static")},
    {T_STRUCT,     DB_TEXT("struct")},
    {T_THRU,       DB_TEXT("thru")},   
    {T_TIMESTAMP,  DB_TEXT("timestamp")},
    {T_TYPEDEF,    DB_TEXT("typedef")},
    {T_UNIQUE,     DB_TEXT("unique")},
    {T_UNSIGNED,   DB_TEXT("unsigned")},
    {T_VARILEN,    DB_TEXT("varilen")},
    {T_ALLOCATION, DB_TEXT("allocation")},
    {T_INITIAL,    DB_TEXT("initial")},
    {T_PCTINCREASE,DB_TEXT("pctincrease")},
    {T_PAGESIZE,   DB_TEXT("pagesize")},
    {0,            DB_TEXT("")}
};

static int     pbstack[MAXSTACK];      /* pushed back input stack */
static int     bp = -1;                /* pbstack pointer */
static DB_TINT chlex = DB_TEXT('\n');  /* Last input character */

static void fmt_error (DB_TCHAR *, DB_TCHAR *);
static int input (void);



/* Initialize static variables, etc. */
void lexInit (void)
{
     ddlp_g.tot_errs = 0;
     ddlp_g.line = 0;
     bp = -1;
     chlex = DB_TEXT('\n');
}


/* Lexical Analyzer
*/
int yylex (void)
{
    register int i;

    for (;;)
    {
        while (chlex <= DB_TEXT(' '))
        {
            while (chlex == DB_TEXT('\n'))
            {
                ++ddlp_g.line;
                if ((chlex = input()) == DB_TEXT('#'))
                {
                    while ((chlex = input()) == DB_TEXT(' '))
                        ;
                    if (vistdigit(chlex))
                    {
                        register int n;

                        n = 0;
                        while (vistdigit(chlex))
                        {
                            n = 10*n + chlex - DB_TEXT('0');
                            chlex = input();
                        }
                        ddlp_g.line = n - 1;
                    }
                    else
                    {
                        ddlp_unput(chlex);
                        ddlp_unput(DB_TEXT('#'));
                    }
                }
                else if (chlex != DB_TEXT('\n'))
                {
                    ddlp_unput(chlex);
                }
            }
            if (chlex == DB_TEOF)
                return (-1);
            chlex = input();
        }
        if (vistalpha(chlex) || (chlex == DB_TEXT('#')) || (chlex == DB_TEXT('_')))
        {
            /* Build identifer */
            for ( i = 0, yylval.tstr.str[0] = (DB_TCHAR) chlex;
                  (chlex = input()), (vistalnum(chlex) || chlex == DB_TEXT('_'));
                )
            {
                if (i < NAMELEN - 2)
                    yylval.tstr.str[++i] = (DB_TCHAR) chlex;
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
                yylval.tnum.numline = ddlp_g.line;
                return (keywords[i].lx);
            }
            else
            {
                yylval.tstr.strline = ddlp_g.line;
                return (T_IDENT);
            }
        }
        else if (vistdigit(chlex))
        {
            /* Convert digit string to number */
            yylval.tnum.num = 0;
            while (vistdigit(chlex))
            {
                yylval.tnum.num = 10 * yylval.tnum.num + chlex - DB_TEXT('0');
                chlex = input();
            }
            yylval.tnum.numline = ddlp_g.line;
            return (T_NUMBER);
        }
        else if (chlex == DB_TEXT('"'))
        {
            for (i = 0; (chlex = input()) != DB_TEXT('"'); i++)
            {
                if (chlex == DB_TEXT('\n') || chlex == DB_TEOF)
                {
                    ddlp_abort(DB_TEXT("missing quote"));
                    return (-1);
                }
                yylval.tstr.str[i] = (DB_TCHAR) chlex;
            }
            yylval.tstr.str[i] = 0;
            chlex = input();
            yylval.tstr.strline = ddlp_g.line;
            return (T_STRING);
        }
        else if (chlex == DB_TEXT('/'))
        {
            if ((chlex = input()) == DB_TEXT('*'))
            {
                /* ignore comment */
                for (;;)
                {
                    while ((chlex = input()) != DB_TEXT('*'))
                    {
                        if (chlex == DB_TEXT('\n'))
                        {
                            ++ddlp_g.line;
                        }
                        else if (chlex == DB_TEOF)
                        {
                            ddlp_abort(DB_TEXT("premature EOF: probably missing '*/'"));
                            return (-1);
                        }
                    }
                    if ((chlex = input()) == DB_TEXT('/'))
                        break;
                    else
                        ddlp_unput(chlex);
                }
                chlex = input();
                continue;
            }
            else if (chlex == DB_TEXT('/'))
            {
                while ((chlex = input()) != DB_TEXT('\n'))
                    ;
                continue;
            }
            else
            {
                ddlp_unput(chlex);
                chlex = DB_TEXT('/');
            }
        }
        yylval.tnum.num = chlex;
        chlex = input();
        yylval.tnum.numline = ddlp_g.line;
        return (yylval.tnum.num);
    }
}


/* Input next character
*/
static int input (void)
{
    int chr;

    if (bp >= 0)
    {
        chr = pbstack[bp--];
    }
    else
    {
        chr = vgettc(ddlp_g.schema);
    }
    return (chr);
}

/* Put character back on input
*/
void ddlp_unput(int chr)
{
    if (++bp > MAXSTACK)
        ddlp_abort(DB_TEXT("pbstack overflow"));
    else
        pbstack[bp] = chr;
}


/* Syntax error
*/
void yyerror(DB_TCHAR *s)
{
    if (vtstrcmp(s, DB_TEXT("syntax error")))
        dderror(s, ddlp_g.line);
}


/* Data definition error
*/
void dderror(DB_TCHAR *s, int ln)
{
    DB_TCHAR errMsg[80+FILENMLEN];

    if (++ddlp_g.tot_errs > 10)
    {
        ddlp_abort(DB_TEXT("error limit"));
    }
    else
    {
        vstprintf(errMsg, DB_TEXT("%s %d 0 : ERROR : "), ddlp_g.ddlfile, ln);
        fmt_error(errMsg, s);
    }
}


/* Data definition warning
*/
void ddwarning(DB_TCHAR *s, int ln)
{
    DB_TCHAR errMsg[80+FILENMLEN];

    if (ln == -1L)
        ln = ddlp_g.line;
    vstprintf(errMsg, DB_TEXT("%s %d 0 : WARNING : "), ddlp_g.ddlfile, ln);
    fmt_error(errMsg, s);
}


/* Abort ddlp
*/
void ddlp_abort(DB_TCHAR *s)
{
    DB_TCHAR errMsg[80+FILENMLEN];

    vstprintf(errMsg, DB_TEXT("%s %d 0 : FATAL ERROR : "), ddlp_g.ddlfile, ddlp_g.line);
    fmt_error(errMsg, s);
    ++ddlp_g.tot_errs;

    ddlp_g.abort_flag = 1;
}

/* Format the printing of an error message
*/
static void fmt_error(DB_TCHAR *s1, DB_TCHAR *s2)
{
    int         len,
                i,
                firstln,
                s2len;
    DB_TCHAR   *ptr;

    /* This function seeks to indent multiple lines of the actual message,
       s2, to the level of the header, s1.
    */
    len = vtstrlen(s1);
    vftprintf(ddlp_g.outfile, DB_TEXT("%s"), s1);

    if (vtstrlen(s2) + len <= 79)
    {
        vftprintf(ddlp_g.outfile, DB_TEXT("%s\n"), s2);
        return;
    }

    firstln = 1;
    for (;;)
    {
        s2len = vtstrlen(s2);
        if (s2len == 0)
            return;

        if (s2len + len > 79)
            ptr = s2 + 79 - len;
        else
            ptr = s2;

        while (ptr > s2 && *ptr != DB_TEXT(' '))
            ptr--;
        if (ptr == s2)
        {
            /* this case means that there are no spaces on which to break, or
               that the rest of the string is known to fit in one ddlp_g.line
            */
            if (!firstln)
            {
                for (i = 0; i < len; i++)
                    vftprintf(ddlp_g.outfile, DB_TEXT(" "));
            }
            else
            {
                firstln = 0;
            }
            vftprintf(ddlp_g.outfile, DB_TEXT("%s\n"), s2);
            return;
        }

        *ptr = 0;
        if (!firstln)
        {
            for (i = 0; i < len; i++)
                vftprintf(ddlp_g.outfile, DB_TEXT(" "));
        }
        else
        {
            firstln = 0;
        }
        vftprintf(ddlp_g.outfile, DB_TEXT("%s\n"), s2);
        s2 = ptr + 1;
    }
}


