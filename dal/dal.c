#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "dal.y"
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

#include "db.star.h"
#include "parser.h"
#include "daldef.h"
#include "dalvar.h"

#if defined(UNICODE)
#define yyerror(s) yyerror(L ## s)
#endif

extern int line;

static int nparams;
static DB_TCHAR fld[3][NAMELEN];

static struct printfield prflds;


/* QNX yacc and bison (GNU) take care of this */
#if !defined(QNX) && !defined(VXWORKS) && !defined(yyerrok) && !defined(YYBISON)
#define yyerrok yyerrflag = 0
#endif
#line 41 "dal.y"
typedef union {
     STRTOK tstr;
     NUMTOK tnum;
} YYSTYPE;
#line 53 "y.tab.c"
#define T_WHILEOK 257
#define T_EXIT 258
#define T_PRINT 259
#define T_INPUT 260
#define T_ABORT 261
#define T_SCHEMA 262
#define T_CURRENCY 263
#define T_REWIND 264
#define T_IDENT 265
#define T_STRING 266
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    4,    4,    3,    3,    3,    3,    3,    3,
    3,    9,    9,   15,   10,   10,   10,   10,    5,   16,
   16,   16,   16,   16,    1,    1,    1,   11,   11,    7,
    7,   17,    6,    6,   18,   18,    2,    2,    2,   12,
   13,   13,   13,   14,    8,    8,
};
short yylen[] = {                                         2,
    2,    1,    1,    2,    1,    1,    1,    1,    1,    1,
    2,    4,    4,    1,    5,    6,    4,    3,    1,    0,
    1,    3,    5,    6,    1,    3,    1,    3,    6,    1,
    1,    1,    1,    1,    1,    3,    1,    3,    1,    3,
    2,    3,    3,    2,    2,    2,
};
short yydefred[] = {                                      0,
    0,   14,    0,   33,   34,    0,    0,    0,    0,   19,
    0,    3,    0,    0,   32,    2,    5,    6,    7,    8,
    9,   10,    0,    0,   11,   45,   46,    0,    0,   41,
   44,   31,   30,    0,    4,    1,    0,    0,    0,    0,
   39,    0,   35,    0,   43,   42,   40,   18,    0,    0,
   27,    0,    0,    0,    0,    0,    0,   28,    0,   17,
    0,    0,    0,   13,   12,   38,    0,   36,   26,    0,
    0,   15,    0,    0,   16,   29,    0,   24,
};
short yydgoto[] = {                                      11,
   52,   43,   12,   13,   14,   15,   34,   16,   17,   18,
   19,   20,   21,   22,   23,   53,   24,   44,
};
short yysindex[] = {                                   -230,
  -47,    0,  -38,    0,    0,  -21,  -57,  -18, -250,    0,
    0,    0, -230,  -39,    0,    0,    0,    0,    0,    0,
    0,    0,  -74,  -40,    0,    0,    0,   -9,   -8,    0,
    0,    0,    0,   -6,    0,    0,   -5, -246, -220,    6,
    0, -250,    0,  -37,    0,    0,    0,    0,   -4,   10,
    0,   13,   17,  -54, -121, -206,   19,    0, -248,    0,
 -204, -219,  -56,    0,    0,    0, -248,    0,    0,   18,
    4,    0,  -36, -219,    0,    0,   20,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   24,    0,  -35,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  -30,
    0,   25,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   26,
    0,    0,    0,    0,    0,    0,   27,    0,
};
short yygindex[] = {                                      0,
  -49,   11,   -7,   30,    0,    0,   31,   59,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    7,
};
#define YYTABLESIZE 226
short yytable[] = {                                      42,
   38,   30,   72,   65,   25,   35,   59,   59,   37,   49,
   25,   25,   70,   25,   32,   33,   40,   41,   50,   51,
   26,   58,   76,   37,   77,    1,    2,    3,    4,    5,
    6,    7,    8,    9,   10,   54,    2,   27,    4,    5,
   31,    7,    8,    9,   10,   50,   51,   35,   39,   45,
   46,   56,   47,   48,   60,   61,   62,   63,   66,   67,
   69,   74,   75,   78,   20,   21,   22,   23,   55,   68,
   64,   36,   57,   73,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    1,    2,    0,    4,    5,    0,
    7,    8,    9,   10,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   28,   71,
    0,    0,    0,    0,    0,    0,    0,   29,    0,    0,
    0,    0,    0,    0,    0,    0,   37,    0,    0,    0,
    0,    0,    0,    0,   40,   41,
};
short yycheck[] = {                                      40,
   40,   59,   59,  125,   59,   13,   44,   44,   44,  256,
   41,   59,   62,   44,  265,  266,  265,  266,  265,  266,
   59,   59,   59,   59,   74,  256,  257,  258,  259,  260,
  261,  262,  263,  264,  265,  256,  257,   59,  259,  260,
   59,  262,  263,  264,  265,  265,  266,   55,  123,   59,
   59,   46,   59,   59,   59,   46,   44,   41,  265,   41,
  265,   44,   59,   44,   41,   41,   41,   41,   39,   59,
  125,   13,   42,   67,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  256,  257,   -1,  259,  260,   -1,
  262,  263,  264,  265,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  256,  256,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  265,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  256,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  265,  266,
};
#define YYFINAL 11
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 266
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'('","')'",0,0,"','",0,"'.'",0,0,0,0,0,0,0,0,0,0,0,0,"';'",0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"T_WHILEOK","T_EXIT","T_PRINT","T_INPUT","T_ABORT","T_SCHEMA","T_CURRENCY",
"T_REWIND","T_IDENT","T_STRING",
};
char *yyrule[] = {
"$accept : dal_gram",
"dal_gram : stmts end_mark",
"dal_gram : end_mark",
"stmts : stmt",
"stmts : stmts stmt",
"stmt : whileok",
"stmt : fcn_call",
"stmt : io",
"stmt : rewind",
"stmt : schema",
"stmt : currency",
"stmt : error ';'",
"whileok : whilestart '{' stmts '}'",
"whileok : whilestart '{' error '}'",
"whilestart : T_WHILEOK",
"fcn_call : fcnstart '(' params ')' ';'",
"fcn_call : fcnstart '(' params ')' error ';'",
"fcn_call : fcnstart '(' error ';'",
"fcn_call : fcnstart error ';'",
"fcnstart : T_IDENT",
"params :",
"params : param",
"params : param ',' param",
"params : param ',' param ',' param",
"params : param ',' param ',' param ','",
"param : T_IDENT",
"param : T_IDENT '.' T_IDENT",
"param : T_STRING",
"io : iostart flds ';'",
"io : iostart '(' filespec ')' flds ';'",
"filespec : T_STRING",
"filespec : T_IDENT",
"iostart : iocom",
"iocom : T_PRINT",
"iocom : T_INPUT",
"flds : fld",
"flds : flds ',' fld",
"fld : T_IDENT",
"fld : T_IDENT '.' T_IDENT",
"fld : T_STRING",
"rewind : T_REWIND filespec ';'",
"schema : T_SCHEMA ';'",
"schema : T_SCHEMA T_IDENT ';'",
"schema : T_SCHEMA error ';'",
"currency : T_CURRENCY ';'",
"end_mark : T_EXIT ';'",
"end_mark : T_ABORT ';'",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#line 372 "dal.y"

#line 259 "y.tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 3:
#line 60 "dal.y"
{
        int i;
        if (!batch )
        {
            for (i=0; i<loop_lvl; i++)
                vtprintf( DB_TEXT("   ") );
            vtprintf( DB_TEXT("d_") );
        }
    }
break;
case 4:
#line 70 "dal.y"
{
        int i;
        if (!batch )
        {
            for (i=0; i<loop_lvl; i++)
                vtprintf( DB_TEXT("   ") );
            vtprintf( DB_TEXT("d_") );
        }
    }
break;
case 5:
#line 80 "dal.y"
{}
break;
case 6:
#line 81 "dal.y"
{}
break;
case 7:
#line 82 "dal.y"
{}
break;
case 8:
#line 83 "dal.y"
{}
break;
case 9:
#line 84 "dal.y"
{}
break;
case 10:
#line 85 "dal.y"
{}
break;
case 11:
#line 87 "dal.y"
{
        dderror( DB_TEXT("invalid statement"), yyvsp[0].tnum.numline );
    }
break;
case 12:
#line 92 "dal.y"
{
        loop_lvl--;
        if ( loop_lvl == 0 )
        {
            dalwhile( loopstack[0] );
            freeloop( &loopstack[0] );
        }
        else
            previnst = loopstack[loop_lvl];
    }
break;
case 13:
#line 103 "dal.y"
{
        yyerrok;
        dderror( DB_TEXT("While loop aborted"), yyvsp[0].tnum.numline );
        loop_lvl--;
        freeloop( &loopstack[loop_lvl] );
    }
break;
case 14:
#line 111 "dal.y"
{
        int i;
        newinst( &curinst );
        if ( curinst == NULL )
            return(1);
        loopstack[loop_lvl++] = curinst;
        vtstrcpy( curinst->i_name, DB_TEXT("while") );
        if ( loop_lvl > 1 )
        {
            if ( previnst == NULL )
                loopstack[loop_lvl-2]->i_loop = curinst;
            else
                previnst->i_next = curinst;
        }
        previnst = NULL;
        if ( !batch )
        {
            for (i=0; i<loop_lvl; i++ )
                vtprintf( DB_TEXT("   ") );
            vtprintf( DB_TEXT("d_") );
        }
    }
break;
case 15:
#line 135 "dal.y"
{
        if (loop_lvl ==0 )
        {
            dalexec( curinst );
            freeinst( &curinst );
        }
    }
break;
case 16:
#line 143 "dal.y"
{
        yyerrok;
        dderror( DB_TEXT("Characters following closing paren"), yyvsp[0].tnum.numline );
    }
break;
case 17:
#line 148 "dal.y"
{
        yyerrok;
        dderror( DB_TEXT("Missing closing paren"), yyvsp[0].tnum.numline );
    }
break;
case 18:
#line 153 "dal.y"
{
        yyerrok;
        dderror( DB_TEXT("Invalid function call"), yyvsp[0].tnum.numline );
    }
break;
case 19:
#line 159 "dal.y"
{
        newinst( &curinst );
        if ( curinst == NULL )
            return(1);
        vtstrncpy( curinst->i_name, yyvsp[0].tstr.str, 9 );
        nparams = 0;
        fld[0][0] = fld[1][0] = fld[2][0] = 0;
        if ( loop_lvl )
        {
            if ( previnst == NULL )
                loopstack[loop_lvl-1]->i_loop = curinst;
            else
                previnst->i_next = curinst;
            previnst = curinst;
        }
    }
break;
case 21:
#line 178 "dal.y"
{
        vtstrncpy( curinst->i_p1, yyvsp[0].tstr.str, NAMELEN );
        vtstrncpy( curinst->i_f1, fld[0], NAMELEN );
    }
break;
case 22:
#line 183 "dal.y"
{
        vtstrncpy( curinst->i_p1, yyvsp[-2].tstr.str, NAMELEN );
        vtstrncpy( curinst->i_f1, fld[0], NAMELEN );
        vtstrncpy( curinst->i_p2, yyvsp[0].tstr.str, NAMELEN );
        vtstrncpy( curinst->i_f2, fld[1], NAMELEN );
    }
break;
case 23:
#line 190 "dal.y"
{
        vtstrncpy( curinst->i_p1, yyvsp[-4].tstr.str, NAMELEN );
        vtstrncpy( curinst->i_f1, fld[0], NAMELEN );
        vtstrncpy( curinst->i_p2, yyvsp[-2].tstr.str, NAMELEN );
        vtstrncpy( curinst->i_f2, fld[1], NAMELEN );
        vtstrncpy( curinst->i_p3, yyvsp[0].tstr.str, NAMELEN );
        vtstrncpy( curinst->i_f3, fld[2], NAMELEN );
    }
break;
case 24:
#line 199 "dal.y"
{
        yyerrok;
        dderror(DB_TEXT("Too many parameters"), yyvsp[0].tnum.numline);
    }
break;
case 25:
#line 205 "dal.y"
{
        vtstrcpy( yyval.tstr.str, yyvsp[0].tstr.str );
        nparams++;
    }
break;
case 26:
#line 210 "dal.y"
{
        vtstrcpy( fld[nparams], yyvsp[0].tstr.str );
        nparams++;
    }
break;
case 27:
#line 215 "dal.y"
{
        vtstrcpy( yyval.tstr.str, genlit( yyvsp[0].tstr.str ) );
        fld[nparams][0] = 0;
        nparams++;
    }
break;
case 28:
#line 222 "dal.y"
{
        if ( loop_lvl == 0)
        {
            dalexec( curinst );
            freeinst( &curinst );
        }
    }
break;
case 29:
#line 230 "dal.y"
{
        vtstrcpy( curinst->i_p1, yyvsp[-3].tstr.str );
        if ( loop_lvl == 0 )
        {
            dalexec( curinst );
            freeinst( &curinst );
        }
    }
break;
case 32:
#line 243 "dal.y"
{
        newinst( &curinst );
        if ( curinst == NULL )
            return(1);
        vtstrcpy( curinst->i_name, yyvsp[0].tstr.str );
        if ( loop_lvl )
        {
            if ( previnst == NULL )
                loopstack[loop_lvl-1]->i_loop = curinst;
            else
                previnst->i_next = curinst;
            previnst = curinst;
        }
    }
break;
case 33:
#line 259 "dal.y"
{
        vtstrcpy( yyval.tstr.str, DB_TEXT("print") );
    }
break;
case 34:
#line 263 "dal.y"
{
        vtstrcpy( yyval.tstr.str, DB_TEXT("input") );
    }
break;
case 35:
#line 268 "dal.y"
{
        newprint( &curprint );
        if ( curprint == NULL )
            return(1);
        curinst->i_pfld = curprint;
        vtstrcpy( curprint->pf_rec, prflds.pf_rec );
        vtstrcpy( curprint->pf_fld, prflds.pf_fld );
    }
break;
case 36:
#line 277 "dal.y"
{
        newprint( &(curprint->pf_next) );
        if ( curprint == NULL )
            return(1);
        curprint = curprint->pf_next;
        vtstrcpy( curprint->pf_rec, prflds.pf_rec );
        vtstrcpy( curprint->pf_fld, prflds.pf_fld );
    }
break;
case 37:
#line 287 "dal.y"
{
        vtstrcpy( prflds.pf_rec, yyvsp[0].tstr.str );
        prflds.pf_fld[0] = 0;
    }
break;
case 38:
#line 292 "dal.y"
{
        vtstrcpy( prflds.pf_rec, yyvsp[-2].tstr.str );
        vtstrcpy( prflds.pf_fld, yyvsp[0].tstr.str );
    }
break;
case 39:
#line 297 "dal.y"
{
        vtstrcpy( prflds.pf_rec, genlit( yyvsp[0].tstr.str ) );
        prflds.pf_fld[0] = 0;
    }
break;
case 40:
#line 303 "dal.y"
{
        newinst( &curinst );
        if ( curinst == NULL )
            return(1);
        vtstrcpy( curinst->i_p1, yyvsp[-1].tstr.str );
        vtstrcpy( curinst->i_name, DB_TEXT("rewind") );
        if ( loop_lvl )
        {
            if ( previnst == NULL )
                loopstack[loop_lvl-1]->i_loop = curinst;
            else
                previnst->i_next = curinst;
            previnst = curinst;
        }
        if ( loop_lvl == 0 )
        {
            dalexec( curinst );
            freeinst( &curinst );
        }
    }
break;
case 41:
#line 325 "dal.y"
{
        dalschem( DB_TEXT("") );
    }
break;
case 42:
#line 329 "dal.y"
{
        dalschem( yyvsp[-1].tstr.str );
    }
break;
case 43:
#line 333 "dal.y"
{
        yyerrok;
        dderror( DB_TEXT(" Invalid Parameter (s)"), yyvsp[0].tnum.numline );
    }
break;
case 44:
#line 339 "dal.y"
{
        newinst( &curinst );
        if ( curinst == NULL )
            return(1);
        vtstrcpy( curinst->i_name, DB_TEXT("currency") );
        if ( loop_lvl )
        {
            if ( previnst == NULL )
                loopstack[loop_lvl-1]->i_loop = curinst;
            else
                previnst->i_next = curinst;
            previnst = curinst;
        }
        if (loop_lvl ==0 )
        {
            dalexec( curinst );
            freeinst( &curinst );
        }
    }
break;
case 45:
#line 360 "dal.y"
{
        dalio_close();
        d_close(DalDbTask);
        return(0);
    }
break;
case 46:
#line 366 "dal.y"
{
        dalio_close();
        return(1);
    }
break;
#line 779 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
