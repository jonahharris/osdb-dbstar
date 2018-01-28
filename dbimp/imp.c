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
#line 2 "imp.y"
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
#include "parser.h"
#include "impvar.h"

#if defined(UNICODE)
#define yyerror(s) yyerror(L ## s)
#endif

#define DBIMP
#include "getnames.h"

static int dim[2][MAXDIMS];
static int dims[2];
static int fsinit;

/* Integer to string conversion */
char *itos();
#line 38 "imp.y"
typedef union {
    STRTOK tstr; 
    NUMTOK tnum;
} YYSTYPE;
#line 50 "y.tab.c"
#define T_DATABASE 257
#define T_FOR 258
#define T_ON 259
#define T_FIELD 260
#define T_CONNECT 261
#define T_END 262
#define T_RECORD 263
#define T_CREATE 264
#define T_UPDATE 265
#define T_FIND 266
#define T_NUMBER 267
#define T_IDENT 268
#define T_STRING 269
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,   12,    8,    8,   13,   13,   15,   15,    7,   16,
   16,   17,   17,   17,   18,   18,    1,    1,    4,    4,
    4,    4,    9,   10,   11,   21,   20,   20,   22,    3,
    3,    2,    2,   23,   23,   25,   26,   26,   27,    5,
    5,    6,   24,   24,   19,   19,   14,   14,
};
short yylen[] = {                                         2,
    3,    3,    1,    1,    1,    2,    3,    3,    3,    1,
    2,    1,    1,    1,    4,    3,    3,    3,    0,    1,
    1,    1,    4,    4,    4,    1,    1,    2,    0,    1,
    2,    5,    3,    3,    6,    0,    1,    2,    0,    1,
    2,    3,    1,    3,    3,    3,    1,    2,
};
short yydefred[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    5,    3,    4,
    2,    0,    0,    0,    0,    0,   12,    0,   10,   13,
   14,    0,   47,    1,    6,    9,    8,    0,    0,    0,
    0,    0,    0,    0,   29,   20,   21,   22,    7,   11,
   48,   46,   45,   18,   16,   17,    0,    0,    0,    0,
    0,   26,    0,    0,    0,   15,    0,    0,   28,   24,
   23,   25,    0,    0,    0,   31,   33,    0,   39,   43,
    0,    0,    0,    0,    0,   32,    0,    0,    0,   40,
   44,   39,    0,   41,   35,   42,
};
short yydgoto[] = {                                       2,
   16,   58,   59,   35,   79,   80,    6,   11,   36,   37,
   38,    3,    7,   24,   17,   18,   19,   20,   21,   50,
   53,   51,   64,   72,   65,   73,   74,
};
short yysindex[] = {                                   -251,
 -255,    0, -233,  -58, -238, -246, -123,    0,    0,    0,
    0,  -96,  -93, -248, -247, -242,    0, -125,    0,    0,
    0,  -26,    0,    0,    0,    0,    0,  -25,  -24, -109,
  -87, -222, -221, -220,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -227, -227, -227,  -84,
 -218,    0,  -16,  -15,  -14,    0, -210, -218,    0,    0,
    0,    0,  -12,  -13, -219,    0,    0, -241,    0,    0,
    5,   -9,    6,  -38, -213,    0, -212, -209,  -38,    0,
    0,    0,  -36,    0,    0,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -120,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -93,    0,    0,    0,    0,    0, -208,  -70,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   -2,  -43,    0,    0,    0,    0,  -42,    0,
    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
    0,    0,    3,    0,    0,  -17,    0,    0,    0,    0,
    0,    0,    0,    0,    4,    0,   45,    0,    0,    0,
  -19,    0,    0,    0,    0,  -18,    0,
};
#define YYTABLESIZE 140
short yytable[] = {                                      39,
   10,   23,   37,   38,   19,    1,    8,   28,   30,   13,
   25,    5,    4,   44,   14,   45,   15,   37,   38,   29,
   31,   32,   33,   34,    5,   70,   26,   71,   54,   55,
   12,   27,   41,   42,   43,   46,   47,   48,   49,   52,
   56,   57,   60,   61,   62,   63,   67,   68,   69,   76,
   75,   77,   78,   81,   30,   82,   86,   83,   34,   36,
   66,   84,   40,   85,    9,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    5,    0,    5,   14,    0,   15,   22,   19,
};
short yycheck[] = {                                     125,
   59,  125,   46,   46,  125,  257,    3,  256,  256,  256,
    7,  258,  268,  123,  261,  125,  263,   61,   61,  268,
  268,  264,  265,  266,  258,  267,  123,  269,   48,   49,
  269,  125,   59,   59,   59,  123,  259,  259,  259,  267,
  125,  260,   59,   59,   59,  256,   59,   61,  268,   59,
   46,   46,   91,  267,  125,  268,   93,  267,   61,  268,
   58,   79,   18,   82,  123,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  258,   -1,  258,  261,   -1,  263,  262,  260,
};
#define YYFINAL 2
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 269
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,"','",0,"'.'",0,0,0,0,0,0,0,0,0,0,0,0,"';'",0,"'='",0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'['",0,"']'",0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"T_DATABASE","T_FOR","T_ON","T_FIELD","T_CONNECT","T_END","T_RECORD","T_CREATE",
"T_UPDATE","T_FIND","T_NUMBER","T_IDENT","T_STRING",
};
char *yyrule[] = {
"$accept : import_spec",
"import_spec : db_spec for_loops close_mark",
"db_spec : T_DATABASE T_IDENT open_mark",
"open_mark : '{'",
"open_mark : ';'",
"for_loops : for_loop",
"for_loops : for_loops for_loop",
"for_loop : for_id stmts '}'",
"for_loop : for_id error '}'",
"for_id : T_FOR T_STRING '{'",
"stmts : stmt",
"stmts : stmts stmt",
"stmt : for_loop",
"stmt : record_spec",
"stmt : connect_spec",
"record_spec : rec_id handling field_defs '}'",
"record_spec : T_RECORD error '}'",
"rec_id : T_RECORD T_IDENT '{'",
"rec_id : T_RECORD error '{'",
"handling :",
"handling : update_def",
"handling : create_def",
"handling : find_def",
"update_def : T_UPDATE T_ON key_spec ';'",
"create_def : T_CREATE T_ON key_spec ';'",
"find_def : T_FIND T_ON key_spec ';'",
"key_spec : T_NUMBER",
"field_defs : field_init",
"field_defs : field_init fields",
"field_init :",
"fields : field_def",
"fields : field_def fields",
"field_def : T_FIELD field_spec '=' line_spec ';'",
"field_def : T_FIELD error ';'",
"field_spec : fsinit T_IDENT subscripts",
"field_spec : fsinit T_IDENT subscripts '.' T_IDENT subscripts",
"fsinit :",
"subscripts : subinit",
"subscripts : subinit ss",
"subinit :",
"ss : subscript",
"ss : ss subscript",
"subscript : '[' T_NUMBER ']'",
"line_spec : T_NUMBER",
"line_spec : T_STRING '.' T_NUMBER",
"connect_spec : T_CONNECT T_IDENT ';'",
"connect_spec : T_CONNECT error ';'",
"close_mark : '}'",
"close_mark : T_END ';'",
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
#line 322 "imp.y"

#line 248 "y.tab.c"
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
case 2:
#line 58 "imp.y"
{
        int i;

        /* open up the database and read in the ddl tables */
        vtstrcpy( imp_g.dbname, yyvsp[-1].tstr.str );
        d_on_opt( READNAMES, imp_g.dbtask );
        if ( d_open_sg( imp_g.dbname, DB_TEXT("o"), imp_g.sg, imp_g.dbtask ) != S_OKAY )
        {
            dbimp_abort( DB_TEXT("unable to open database\n") );
            return 1;
        }

        /* allocate space for the 'current record' table */
        imp_g.currecs = (DB_ADDR *)calloc( imp_g.dbtask->size_rt, sizeof(DB_ADDR) );
        if ( imp_g.currecs == NULL )
        {
            dbimp_abort( DB_TEXT("out of memory\n") );
            return 1;
        }

        for ( i=0; i<imp_g.dbtask->size_rt; i++ )
            imp_g.currecs[i] = (DB_ADDR)-1L;
    }
break;
case 7:
#line 89 "imp.y"
{
        /* a for loop has been fully recognized, terminate the definition */
        imp_g.loop_lvl--;
        imp_g.curspec->sp_next = new_spec( ENDLOOP );
        if (imp_g.curspec->sp_next == NULL)
            return 1;
        imp_g.curspec = imp_g.curspec->sp_next;
        imp_g.curspec->u.sp_endptr = new_end( imp_g.curloop[imp_g.loop_lvl] );
        if (imp_g.curspec->u.sp_endptr == NULL)
            return 1;
    }
break;
case 8:
#line 101 "imp.y"
{
        dderror( DB_TEXT("error in 'foreach' statment"), yyvsp[-2].tnum.numline );
    }
break;
case 9:
#line 106 "imp.y"
{
        /* create a new for loop specification, linked into the list */
        if (imp_g.specstart == NULL)
        {
            imp_g.specstart = new_spec( LOOP );
            if (imp_g.specstart == NULL)
                return 1;
            imp_g.curspec = imp_g.specstart;
        }
        else
        {
            imp_g.curspec->sp_next = new_spec( LOOP );
            if (imp_g.curspec->sp_next == NULL)
                return 1;
            imp_g.curspec = imp_g.curspec->sp_next;
        }

        /* start a new loop at the next level */
        imp_g.curspec->u.sp_looptr = new_loop( yyvsp[-1].tstr.str );
        if (imp_g.curspec->u.sp_looptr == NULL)
            return 1;
        imp_g.curloop[imp_g.loop_lvl] = imp_g.curspec;
        imp_g.loop_lvl++;
    }
break;
case 15:
#line 139 "imp.y"
{
        /* link in the start of a record specification */
        if (imp_g.specstart == NULL)
        {
            imp_g.specstart = new_spec( RECORD );
            if (imp_g.specstart == NULL)
                return 1;
            imp_g.curspec = imp_g.specstart;
        }
        else
        {
            imp_g.curspec->sp_next = new_spec( RECORD );
            if (imp_g.curspec->sp_next == NULL)
                return 1;
            imp_g.curspec = imp_g.curspec->sp_next;
        }

        /* locate the record type */
        imp_g.curspec->u.sp_recptr = new_rec( yyvsp[-2].tnum.num, imp_g.hlist, imp_g.fldlist );
        if (imp_g.curspec->u.sp_recptr == NULL)
            return 1;
        imp_g.curspec->u.sp_recptr->rec_ndx = imp_g.recndx;
        if ( imp_g.curspec->u.sp_recptr->rec_ndx < 0 )
        {
            vftprintf(stderr, 
                    DB_TEXT("**WARNING**  record name '%s' not found in schema\n"),
                    yyvsp[-3].tstr.str);
            imp_g.tot_warnings++;
        }

        /* initialize the field and handling list headers */
        imp_g.fldlist = NULL;
        imp_g.hlist = NULL;
    }
break;
case 16:
#line 174 "imp.y"
{
        dderror( DB_TEXT("invalid record spec"), yyvsp[-2].tnum.numline );
    }
break;
case 17:
#line 179 "imp.y"
{
        /* save the record type name */
        vtstrcpy(yyval.tstr.str, yyvsp[-1].tstr.str);
        imp_g.recndx = getrec(yyvsp[-1].tstr.str, imp_g.dbtask);
    }
break;
case 18:
#line 185 "imp.y"
{
        dderror( DB_TEXT("invalid record spec"), yyvsp[-2].tnum.numline );
    }
break;
case 19:
#line 190 "imp.y"
{ yyval.tnum.num = DBIMP_AUTO; }
break;
case 20:
#line 192 "imp.y"
{ yyval.tnum.num = DBIMP_UPDATE; }
break;
case 21:
#line 194 "imp.y"
{ yyval.tnum.num = DBIMP_CREATE; }
break;
case 22:
#line 196 "imp.y"
{ yyval.tnum.num = DBIMP_FIND; }
break;
case 26:
#line 205 "imp.y"
{
        /* the handling list is only one element now */
        imp_g.hlist = new_hand( imp_g.curloop[imp_g.loop_lvl-1]->u.sp_looptr->l_fname, yyvsp[0].tnum.num );
        if (imp_g.hlist == NULL)
            return 1;
    }
break;
case 29:
#line 216 "imp.y"
{
        imp_g.fldlist = NULL;
    }
break;
case 32:
#line 224 "imp.y"
{
        imp_g.newfld->fld_next = imp_g.fldlist;
        imp_g.fldlist = imp_g.newfld;
    }
break;
case 33:
#line 229 "imp.y"
{
        dderror( DB_TEXT("bad field specification"), yyvsp[-2].tnum.numline );
    }
break;
case 34:
#line 234 "imp.y"
{
        /* create the field structure, with zero offset */
        imp_g.newfld = new_fld( DB_TEXT(""), NULL, 0, yyvsp[-1].tstr.str, dim[0], dims[0] );
        if (imp_g.newfld == NULL)
            return 1;
    }
break;
case 35:
#line 241 "imp.y"
{
        /* create the field structure, with offset for subscript */
        imp_g.newfld = new_fld( yyvsp[-4].tstr.str, dim[0], dims[0], yyvsp[-1].tstr.str, dim[1], dims[1] );
        if (imp_g.newfld == NULL)
            return 1;
    }
break;
case 36:
#line 249 "imp.y"
{
        fsinit = -1; 
    }
break;
case 39:
#line 257 "imp.y"
{
        int i;
        fsinit++;
        dims[fsinit] = 0;
        for ( i=0; i<MAXDIMS; i++ ) dim[fsinit][i] = 0;
    }
break;
case 42:
#line 268 "imp.y"
{
        if ( dims[fsinit] >= MAXDIMS )
            dderror( DB_TEXT("too many dimensions"), yyvsp[-1].tnum.numline );
        else
            dim[fsinit][dims[fsinit]++] = yyvsp[-1].tnum.num;
    }
break;
case 43:
#line 276 "imp.y"
{
        /* assume that this number refers to the current file */
        vtstrcpy( imp_g.newfld->fld_file, imp_g.curloop[imp_g.loop_lvl-1]->u.sp_looptr->l_fname );
        imp_g.newfld->fld_name = yyvsp[0].tnum.num;
    }
break;
case 44:
#line 282 "imp.y"
{
        /* save the explicit file name and number */
        vtstrcpy( imp_g.newfld->fld_file, yyvsp[-2].tstr.str );
        imp_g.newfld->fld_name = yyvsp[0].tnum.num;
    }
break;
case 45:
#line 289 "imp.y"
{
        /* save the connect specification on the list */
        imp_g.curspec->sp_next = new_spec( CONNECT );
        if (imp_g.curspec->sp_next == NULL)
            return 1;
        imp_g.curspec = imp_g.curspec->sp_next;
        imp_g.curspec->u.sp_conptr = new_con();
        if (imp_g.curspec->u.sp_conptr == NULL)
            return 1;
        imp_g.curspec->u.sp_conptr->con_ndx = getset( yyvsp[-1].tstr.str, imp_g.dbtask );
        if ( imp_g.curspec->u.sp_conptr->con_ndx < 0 )
        {
            vftprintf(stderr,
                         DB_TEXT("**WARNING**  set '%s' not found in schema\n"),
                         yyvsp[-1].tstr.str);
            imp_g.tot_warnings++;
        }
    }
break;
case 46:
#line 308 "imp.y"
{
        dderror( DB_TEXT("invalid connect spec"), yyvsp[-2].tnum.numline );
    }
break;
case 47:
#line 313 "imp.y"
{
        d_close(imp_g.dbtask);
    }
break;
case 48:
#line 317 "imp.y"
{
        d_close(imp_g.dbtask);
    }
break;
#line 660 "y.tab.c"
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
