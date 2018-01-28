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
#line 2 "ddlp.y"
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

/*------------------------------------------------------------------------ 

    db.* Database Definition Language Processor

    ddlp.y -- YACC grammar and semantics source

------------------------------------------------------------------------*/

#include "db.star.h"
#include "parser.h"
#include "ddldefs.h"

#if defined(UNICODE)
#define yyerror(s) yyerror(L ## s)
#endif

#define YYMAXDEPTH 480

static  DB_TCHAR fld_comp[NAMELEN];
static  DB_TCHAR rec_comp[NAMELEN];
static  short pagesize = 0;  /* if it is still zero, figure it out later */

static  CONST_INFO *ci[MAXDIMS];
static  TYPE_INFO *ti;
static  OM_INFO *om;

static  ID_INFO *id_list, *currid, *ids;
static  MEM_INFO *memList, *currmem, *mem;
static  struct set_info *current_set;
static  int last_ordering;
static  int dim = 0;        /* current # of array dimensions */
static  int elts;
static  int n_optkeys;      /* number of optional keys in record */
static  short tempnum;
static  int sign_flag;
static  int inline_struct = 0;
static  FIELD_ENTRY struct_fd; /* save struct def when processing struct flds */
static  int in_db = 0;


/*lint -e525 */
/*lint -e778 */     /* constant expression evaluates to 0 */

/* Initialize static variables, etc. */
void ddlpInit (void)
{
     pagesize = 0;
     inline_struct = 0;
     dim = 0;
     ddlp_g.abort_flag = 0;
}

/* QNX yacc and byacc (GNU) takes care of this */
#if !defined(QNX) && !defined(VXWORKS) && !defined(yyerrok) && !defined(YYBISON)
#define yyerrok yyerrflag = 0
#endif
#line 76 "ddlp.y"
typedef union {
    STRTOK tstr;
    NUMTOK tnum;
} YYSTYPE;
#line 88 "y.tab.c"
#define T_ALLOCATION 257
#define T_ASCENDING 258
#define T_BITMAP 259
#define T_BLOB 260
#define T_BY 261
#define T_COMPOUND 262
#define T_CONST 263
#define T_CONTAINS 264
#define T_DATA 265
#define T_DATABASE 266
#define T_DEFINE 267
#define T_DESCENDING 268
#define T_DIRECTREF 269
#define T_FILE 270
#define T_FIRST 271
#define T_INCLUDE 272
#define T_INITIAL 273
#define T_KEY 274
#define T_LAST 275
#define T_LINE 276
#define T_MEMBER 277
#define T_NEXT 278
#define T_NUMBER 279
#define T_OPTIONAL 280
#define T_ORDER 281
#define T_OWNER 282
#define T_PCTINCREASE 283
#define T_PAGESIZE 284
#define T_RECORD 285
#define T_RECS 286
#define T_RELATEDTO 287
#define T_SET 288
#define T_SETS 289
#define T_STATIC 290
#define T_STRUCT 291
#define T_THRU 292
#define T_TIMESTAMP 293
#define T_TYPEDEF 294
#define T_UNIQUE 295
#define T_UNSIGNED 296
#define T_VARILEN 297
#define T_IDENT 298
#define T_STRING 299
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    1,    1,    1,    4,    4,    3,    3,    3,
    3,    3,    2,    2,    2,    2,    2,    2,    8,    8,
   10,   18,   18,   19,   19,   25,   26,   26,   27,   27,
   22,   22,   22,    5,    5,    5,    5,   30,   30,   30,
   31,   31,    7,    7,    6,    6,    6,    6,    6,   20,
   20,   20,   20,   20,    9,    9,    9,   34,   34,   35,
   35,   36,   36,   36,   36,   32,   37,   37,   38,   38,
   38,   33,   33,   16,   16,   16,   16,   14,   15,   15,
   39,   39,   40,   40,   40,   41,   41,   41,   42,   42,
   28,   28,   28,   43,   43,   21,   21,   29,   29,   44,
   45,   45,   46,   46,   11,   11,   12,   47,   48,   48,
   49,   50,   50,   50,   17,   17,   17,   17,   17,   17,
   17,   17,   51,   13,   53,   53,   53,   53,   53,   53,
   23,   52,   52,   24,   24,
};
short yylen[] = {                                         2,
    4,    3,    1,    2,    1,    1,    2,    1,    1,    1,
    1,    1,    5,    5,    5,    5,    3,    2,    3,    3,
    2,    2,    2,    5,    4,    1,    1,    2,    5,    3,
    5,    5,    3,    3,    6,    6,    3,    1,    3,    2,
    1,    3,    1,    2,    1,    1,    1,    1,    1,    4,
    3,    4,    3,    3,    6,    8,    4,    0,    3,    1,
    2,    3,    3,    3,    3,    3,    1,    1,    0,    3,
    3,    1,    1,    4,    3,    2,    3,    3,    1,    2,
    1,    2,    6,    3,    2,    2,    4,    0,    0,    1,
    3,    2,    0,    1,    0,    2,    1,    2,    1,    0,
    1,    2,    3,    3,    1,    2,    5,    2,    1,    2,
    3,    1,    1,    0,    5,    4,    5,    4,    5,    4,
    4,    3,    4,    3,    1,    1,    1,    1,    1,    1,
    3,    1,    2,    3,    5,
};
short yydefred[] = {                                      0,
    0,    0,    0,    0,   26,    0,    0,    8,    0,    0,
    0,    9,   10,   11,    0,   12,    0,   18,    0,    0,
    0,   21,   90,   97,    0,    0,    7,    0,   49,    0,
    0,   67,   68,   79,    0,    0,    0,    0,   43,    0,
   46,    0,    0,   47,   48,   45,    0,    0,   23,   22,
    0,    0,   17,    0,   37,   34,    0,   19,   20,   33,
  100,   96,    0,    0,    0,    0,    0,    0,   80,    0,
    0,    0,    0,    0,    0,    2,   44,    0,   94,   76,
    0,    0,   81,    0,    0,    0,    0,    0,   73,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    1,    0,    0,    0,    0,    5,  122,    0,    0,
  132,    0,   54,    0,    0,   51,    0,   38,   53,    0,
    0,    0,    4,   85,    0,    0,   75,    0,  105,   82,
   92,    0,   77,   78,    0,    0,    0,    0,    0,   66,
    0,   25,   28,    0,   15,   13,   16,   14,    0,    0,
   32,   31,    0,    0,  101,  116,    0,  118,    0,    0,
    0,    0,  121,  133,   40,    0,   50,    0,   52,  120,
    0,   84,  100,    0,    0,   74,  106,   91,   57,   72,
    0,    0,    0,    0,    0,    0,   60,    0,    0,    0,
   24,   30,  100,   35,   36,    0,    0,  102,  115,  117,
  127,  128,  125,  126,  129,  130,    0,    0,  123,    0,
  134,   42,   39,  119,    0,  108,    0,    0,    0,    0,
    0,    0,   59,   61,    0,   70,   71,    0,  103,  104,
  124,    0,    0,    0,    0,    0,    0,    0,  109,    0,
   62,   63,   64,   65,   55,   29,  131,  135,   86,    0,
   83,  112,  113,    0,  107,  110,    0,    0,  111,   56,
   87,
};
short yydgoto[] = {                                       7,
   76,    8,    9,   10,   11,   39,   40,   12,   41,   13,
  128,  129,  161,   42,   43,   44,   45,   14,   24,   46,
   25,   16,  209,  111,   17,   93,   94,   95,  100,  117,
  118,   47,   90,  138,  186,  187,   48,  140,   82,   83,
  236,   26,   84,  101,  154,  155,  175,  238,  239,  254,
   68,  112,  207,
};
short yysindex[] = {                                   -171,
 -227, -242, -234, -201,    0, -252,    0,    0, -204, -162,
 -153,    0,    0,    0,  -51,    0, -111,    0,  -45,  -73,
 -226,    0,    0,    0, -224, -190,    0, -153,    0, -163,
 -147,    0,    0,    0, -223, -157, -229, -121,    0,  -39,
    0, -105, -222,    0,    0,    0, -181,  -98,    0,    0,
   48,  -88,    0, -185,    0,    0, -165,    0,    0,    0,
    0,    0,  -39, -221, -220,  -99,   70,  -81,    0,  138,
  -57,  -54, -219,    0,  139,    0,    0,  145,    0,    0,
 -252, -106,    0, -243,  -99,   85,  -44,  154,    0,  178,
  128,  -88,   97,  -88, -252,  -50,  -49,  131,  132,  -46,
  136,    0,  -99,  -81,  -99,  -81,    0,    0,  -48,  -69,
    0, -101,    0,  171,  190,    0,   40,    0,    0,   83,
  -99,  -81,    0,    0, -218,  -88,    0, -102,    0,    0,
    0,  -35,    0,    0,  179, -150,  -89,  -24, -164,    0,
  120,    0,    0, -216,    0,    0,    0,    0,  124,  125,
    0,    0, -155,  136,    0,    0, -101,    0, -101, -213,
  -32,  -52,    0,    0,    0,  -43,    0,  -42,    0,    0,
 -101,    0,    0,  -34,  129,    0,    0,    0,    0,    0,
  178,  192,  198,  201,  202,  -41,    0, -215,  172,  175,
    0,    0,    0,    0,    0,  176,  180,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  213,  -20,    0, -215,
    0,    0,    0,    0, -158,    0,  -19,   10,   -2,    2,
    4,   13,    0,    0,   87,    0,    0,  221,    0,    0,
    0,  223,  114,   -8,   -3,  238,  -67, -108,    0, -215,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    8,
    0,    0,    0,  244,    0,    0,  118,    9,    0,    0,
    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,   12,    0,    0,   39,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -249,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -249,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    1,   11,    0,    0,   28,    0,    0,
   12, -249,    0,    0,    0,    0,    0,  -36,    0,   42,
 -160, -249,    0, -110,   12,    0,    0,    0,    0,    0,
  -56,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  121,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -176,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -53,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   42,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  250,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  252,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
short yygindex[] = {                                      0,
  -12,    0,    0,  303,  304,    3,  287,    0,    0,    0,
    0,  189,    0,    0,    0,    0,    0,    0,  214,    0,
    7,    0,    0,  -90,    0,   74,    0,  -21, -145,  -58,
  151,    0,  184,  141,    0,  137,    0,    0,    0,  242,
    0,    0,    0,    0,    0,  173,    0,    0,   88,    0,
  105,   57,    0,
};
#define YYTABLESIZE 327
short yytable[] = {                                     223,
    5,  116,   99,   72,  119,   98,  211,   50,  146,  148,
    3,   52,  152,  120,   27,   54,  255,   57,   75,   75,
   81,  164,   75,   75,   95,   75,   70,  215,   18,   80,
  131,   60,   66,   85,  103,  105,  121,  172,    5,  192,
  114,   93,   77,   23,  201,   95,   93,  228,   93,   56,
  102,  132,   58,  108,  202,   20,   71,  203,    1,   72,
   81,  204,    3,   21,  205,   77,  164,    4,  164,  127,
   19,   59,  133,   61,   67,   86,   67,   67,   67,  173,
  164,  193,  115,  168,  206,   75,    5,  125,   87,    6,
  156,    1,  158,   96,    2,    3,   22,   95,  167,  163,
    4,  144,   29,    2,  174,   30,   31,   62,  170,   69,
  234,   32,   97,   98,  189,  176,   88,   89,   95,    5,
   33,   93,    6,  196,   64,   49,  168,   69,  235,  225,
  168,   34,   99,  190,   35,    3,   36,   69,   69,   37,
   65,  169,  197,   38,  199,  245,  200,  180,   89,   78,
   78,  233,    5,  107,  107,  126,  107,  168,  214,  126,
  157,  168,  159,   95,   41,  141,   73,  143,  104,  106,
   92,   91,  248,   79,   79,  110,  260,  122,  171,   41,
   93,  257,   55,  182,   95,   93,   51,   93,  183,  237,
  252,   79,  109,  184,  185,  110,  113,  123,  114,   99,
  253,  114,   98,  124,   49,  145,  147,  134,  210,  151,
   53,  135,   99,   15,  136,   98,   74,  137,  139,   30,
   31,  142,   15,  149,  150,   32,  153,   72,  162,  165,
   99,  182,  160,   98,   33,  166,  183,  179,  178,  188,
  115,  184,  185,  115,  191,   34,  194,  195,   35,  208,
   36,  217,  219,   37,  212,  115,   49,   38,  220,   49,
   49,  221,  222,  216,  226,   49,    3,  227,  229,    3,
    3,  231,  230,  240,   49,    3,  241,  232,  237,  246,
  242,  247,  243,    5,    3,   49,    5,    5,   49,  249,
   49,  244,    5,   49,  250,    3,  251,   49,    3,  258,
    3,    5,  259,    3,    6,   58,  261,    3,   88,   89,
  114,   27,    5,   28,   63,    5,  177,    5,  213,  181,
    5,  218,  224,  130,    5,  256,  198,
};
short yycheck[] = {                                      41,
    0,   59,   59,   40,   59,   59,   59,   59,   59,   59,
    0,  123,   59,   72,  125,   61,  125,   91,  125,  125,
   42,  112,  125,  125,  274,  125,  256,  173,  256,   42,
  274,  256,  256,  256,  256,  256,  256,  256,  291,  256,
  256,  291,   40,  296,  258,  295,  296,  193,  298,  123,
   63,  295,  279,   66,  268,  298,  286,  271,  263,  289,
   82,  275,  267,  298,  278,   63,  157,  272,  159,   82,
  298,  298,   85,  298,  298,  298,  298,  298,  298,  298,
  171,  298,  298,   44,  298,  125,  291,   81,  270,  294,
  103,  263,  105,  279,  266,  267,  298,  274,   59,  112,
  272,   95,  256,  266,  126,  259,  260,  298,  121,  270,
  269,  265,  298,  279,  279,  128,  298,  299,  295,  291,
  274,  298,  294,  279,  288,  125,   44,  285,  287,  188,
   44,  285,  298,  298,  288,  125,  290,  298,  299,  293,
  288,   59,  298,  297,  157,   59,  159,  298,  299,  256,
  256,  210,  125,  256,  256,  262,  256,   44,  171,  262,
  104,   44,  106,  274,   44,   92,  288,   94,   64,   65,
  123,  270,   59,  280,  280,  277,   59,   73,  122,   59,
  291,  240,  256,  273,  295,  296,  298,  298,  278,  298,
  258,  280,  123,  283,  284,  277,   59,   59,  256,  256,
  268,  256,  256,   59,  256,  256,  256,  123,  261,  256,
  256,  256,  269,    0,   61,  269,  256,   40,   91,  259,
  260,  125,    9,   93,   93,  265,   91,  264,  298,   59,
  287,  273,  281,  287,  274,   46,  278,   59,  274,  264,
  298,  283,  284,  298,  125,  285,  123,  123,  288,  282,
  290,  123,   61,  293,  298,  298,  256,  297,   61,  259,
  260,   61,   61,  298,   93,  265,  256,   93,   93,  259,
  260,   59,   93,  264,  274,  265,  279,  298,  298,   59,
  279,   59,  279,  256,  274,  285,  259,  260,  288,  298,
  290,  279,  265,  293,  298,  285,   59,  297,  288,  292,
  290,  274,   59,  293,  266,  264,  298,  297,   59,  298,
   59,    9,  285,   10,   28,  288,  128,  290,  168,  136,
  293,  181,  186,   82,  297,  238,  154,
};
#define YYFINAL 7
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 299
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'('","')'",0,0,"','",0,"'.'",0,0,0,0,0,0,0,0,0,0,0,0,"';'",0,"'='",
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'['",0,"']'",0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,"T_ALLOCATION","T_ASCENDING","T_BITMAP","T_BLOB","T_BY","T_COMPOUND",
"T_CONST","T_CONTAINS","T_DATA","T_DATABASE","T_DEFINE","T_DESCENDING",
"T_DIRECTREF","T_FILE","T_FIRST","T_INCLUDE","T_INITIAL","T_KEY","T_LAST",
"T_LINE","T_MEMBER","T_NEXT","T_NUMBER","T_OPTIONAL","T_ORDER","T_OWNER",
"T_PCTINCREASE","T_PAGESIZE","T_RECORD","T_RECS","T_RELATEDTO","T_SET","T_SETS",
"T_STATIC","T_STRUCT","T_THRU","T_TIMESTAMP","T_TYPEDEF","T_UNIQUE",
"T_UNSIGNED","T_VARILEN","T_IDENT","T_STRING",
};
char *yyrule[] = {
"$accept : ddl",
"ddl : cpp_cmnds db_spec ddl_stmts closing_brace",
"ddl : db_spec ddl_stmts closing_brace",
"closing_brace : '}'",
"closing_brace : '}' ';'",
"closing_brace : error",
"cpp_cmnds : cpp_cmnd",
"cpp_cmnds : cpp_cmnd cpp_cmnds",
"cpp_cmnd : const_cmnd",
"cpp_cmnd : define_cmnd",
"cpp_cmnd : include_cmnd",
"cpp_cmnd : struct_cmnd",
"cpp_cmnd : typedef_cmnd",
"const_cmnd : T_CONST T_IDENT '=' T_NUMBER ';'",
"const_cmnd : T_CONST T_IDENT '=' T_IDENT ';'",
"const_cmnd : T_CONST T_IDENT '=' T_NUMBER error",
"const_cmnd : T_CONST T_IDENT '=' T_IDENT error",
"const_cmnd : T_CONST T_IDENT error",
"const_cmnd : T_CONST error",
"define_cmnd : T_DEFINE T_IDENT T_NUMBER",
"define_cmnd : T_DEFINE T_IDENT T_IDENT",
"include_cmnd : T_INCLUDE T_IDENT",
"struct_cmnd : struct_spec ';'",
"struct_cmnd : struct_spec error",
"struct_spec : struct_init T_IDENT '{' struct_elems '}'",
"struct_spec : struct_init '{' struct_elems '}'",
"struct_init : T_STRUCT",
"struct_elems : struct_elem",
"struct_elems : struct_elem struct_elems",
"struct_elem : key_spec type_spec T_IDENT size_specs ';'",
"struct_elem : key_spec type_spec error",
"typedef_cmnd : T_TYPEDEF type_spec T_IDENT size_specs ';'",
"typedef_cmnd : T_TYPEDEF type_spec T_IDENT size_specs error",
"typedef_cmnd : T_TYPEDEF type_spec error",
"db_spec : T_DATABASE T_IDENT '{'",
"db_spec : T_DATABASE T_IDENT '[' T_NUMBER ']' '{'",
"db_spec : T_DATABASE T_IDENT '[' T_IDENT ']' '{'",
"db_spec : T_DATABASE T_IDENT error",
"id_list : id_item",
"id_list : id_list ',' id_item",
"id_list : error ';'",
"id_item : T_IDENT",
"id_item : T_IDENT '.' T_IDENT",
"ddl_stmts : ddl_stmt",
"ddl_stmts : ddl_stmts ddl_stmt",
"ddl_stmt : timestamp",
"ddl_stmt : file_spec",
"ddl_stmt : record_spec",
"ddl_stmt : set_spec",
"ddl_stmt : error",
"timestamp : T_TIMESTAMP T_RECS id_list ';'",
"timestamp : T_TIMESTAMP T_RECS ';'",
"timestamp : T_TIMESTAMP T_SETS id_list ';'",
"timestamp : T_TIMESTAMP T_SETS ';'",
"timestamp : T_TIMESTAMP error ';'",
"file_spec : file_type file_id allocation T_CONTAINS id_list ';'",
"file_spec : file_type T_IDENT '=' file_id allocation T_CONTAINS id_list ';'",
"file_spec : file_type T_FILE error ';'",
"allocation :",
"allocation : '(' alloc_opts ')'",
"alloc_opts : alloc_opt",
"alloc_opts : alloc_opts alloc_opt",
"alloc_opt : T_INITIAL '=' T_NUMBER",
"alloc_opt : T_NEXT '=' T_NUMBER",
"alloc_opt : T_PCTINCREASE '=' T_NUMBER",
"alloc_opt : T_PAGESIZE '=' T_NUMBER",
"file_type : data_or_key T_FILE file_size",
"data_or_key : T_DATA",
"data_or_key : T_KEY",
"file_size :",
"file_size : '[' T_NUMBER ']'",
"file_size : '[' T_IDENT ']'",
"file_id : T_IDENT",
"file_id : T_STRING",
"record_spec : rec_id field_defs key_defs closing_brace",
"record_spec : rec_id field_defs closing_brace",
"record_spec : rec_id closing_brace",
"record_spec : rec_type error closing_brace",
"rec_id : rec_type T_IDENT '{'",
"rec_type : T_RECORD",
"rec_type : T_STATIC T_RECORD",
"field_defs : field_spec",
"field_defs : field_defs field_spec",
"field_spec : key_spec type_spec T_IDENT size_specs om_field ';'",
"field_spec : key_spec type_spec error",
"field_spec : error ';'",
"om_field : T_DIRECTREF T_IDENT",
"om_field : T_RELATEDTO T_IDENT T_THRU T_IDENT",
"om_field :",
"sign :",
"sign : T_UNSIGNED",
"key_spec : opt_key T_UNIQUE T_KEY",
"key_spec : opt_key T_KEY",
"key_spec :",
"opt_key : T_OPTIONAL",
"opt_key :",
"type_spec : sign T_IDENT",
"type_spec : struct_spec",
"size_specs : size_init size_decls",
"size_specs : size_init",
"size_init :",
"size_decls : size",
"size_decls : size_decls size",
"size : '[' T_NUMBER ']'",
"size : '[' T_IDENT ']'",
"key_defs : key_field",
"key_defs : key_defs key_field",
"key_field : T_COMPOUND key_id '{' comfld_defs '}'",
"key_id : key_spec T_IDENT",
"comfld_defs : comfld_spec",
"comfld_defs : comfld_defs comfld_spec",
"comfld_spec : T_IDENT key_order ';'",
"key_order : T_ASCENDING",
"key_order : T_DESCENDING",
"key_order :",
"set_spec : T_BITMAP T_SET set_prefix members closing_brace",
"set_spec : T_BITMAP T_SET error closing_brace",
"set_spec : T_BLOB T_SET set_prefix members closing_brace",
"set_spec : T_BLOB T_SET error closing_brace",
"set_spec : T_VARILEN T_SET set_prefix members closing_brace",
"set_spec : T_VARILEN T_SET error closing_brace",
"set_spec : T_SET set_prefix members closing_brace",
"set_spec : T_SET error closing_brace",
"set_prefix : T_IDENT '{' ordering owner",
"ordering : T_ORDER order_type ';'",
"order_type : T_FIRST",
"order_type : T_LAST",
"order_type : T_ASCENDING",
"order_type : T_DESCENDING",
"order_type : T_NEXT",
"order_type : T_IDENT",
"owner : T_OWNER T_IDENT ';'",
"members : member",
"members : members member",
"member : T_MEMBER T_IDENT ';'",
"member : T_MEMBER T_IDENT T_BY id_list ';'",
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
#line 1171 "ddlp.y"

/* $Revision:   1.25.1.6  $ */

#line 526 "y.tab.c"
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
case 4:
#line 104 "ddlp.y"
{
        ddwarning(DB_TEXT("';' not required after '}'"), yyvsp[0].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 5:
#line 110 "ddlp.y"
{
        yyerrok;
        dderror(DB_TEXT("missing '}'"), ddlp_g.line);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 13:
#line 134 "ddlp.y"
{
        if (!add_const(yyvsp[-3].tstr.str, (short) yyvsp[-1].tnum.num))
            ddwarning(DB_TEXT("constant redefined"), yyvsp[-3].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 14:
#line 141 "ddlp.y"
{
        if (find_const(yyvsp[-1].tstr.str, &tempnum))
        {
            if (!add_const(yyvsp[-3].tstr.str, tempnum))
                ddwarning(DB_TEXT("constant redefined"), yyvsp[-3].tstr.strline);
        }
        else
            dderror(DB_TEXT("constant not defined"), yyvsp[-1].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 15:
#line 153 "ddlp.y"
{
        dderror(DB_TEXT("missing ';'"), yyvsp[-1].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 16:
#line 159 "ddlp.y"
{
        dderror(DB_TEXT("missing ';'"), yyvsp[-1].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 17:
#line 165 "ddlp.y"
{
        dderror(DB_TEXT("missing '=' after identifier"), yyvsp[-1].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 18:
#line 171 "ddlp.y"
{
        dderror(DB_TEXT("missing identifier after const"), yyvsp[-1].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 19:
#line 181 "ddlp.y"
{
        if (!add_const(yyvsp[-1].tstr.str, (short) yyvsp[0].tnum.num))
            ddwarning(DB_TEXT("constant redefined"), yyvsp[-1].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 20:
#line 188 "ddlp.y"
{
        if (find_const(yyvsp[0].tstr.str, &tempnum))
        {
            if (!add_const(yyvsp[-1].tstr.str, tempnum))
                ddwarning(DB_TEXT("const redefined"), yyvsp[-1].tstr.strline);
        }
        else
            dderror(DB_TEXT("constant not defined"), yyvsp[0].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 23:
#line 210 "ddlp.y"
{
        dderror(DB_TEXT("missing ';'"), ddlp_g.line);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 24:
#line 220 "ddlp.y"
{
        DB_TCHAR tempbuff[NAMELEN];

        vtstrcpy(tempbuff, DB_TEXT("_s_"));
        vtstrcat(tempbuff, yyvsp[-3].tstr.str);
        
        if (ddlp_g.elem_list)
        {
            if (!add_struct_type(tempbuff))
            {
                vstprintf(ddlp_g.msg, DB_TEXT("type already defined: %s"), yyvsp[-3].tstr.str);
                dderror(ddlp_g.msg, yyvsp[-3].tstr.strline);
            }
        }
        
        if ((ti = find_type(tempbuff, 0)) == NULL)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("structure '%s' not defined.\n"), yyvsp[-3].tstr.str);
            dderror(ddlp_g.msg, yyvsp[-3].tstr.strline);
        }

        memcpy(&ddlp_g.fd_entry, &struct_fd, sizeof(FIELD_ENTRY));

        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 25:
#line 247 "ddlp.y"
{
        if (!add_struct_type(DB_TEXT("_s_temp_")))
            dderror(DB_TEXT("cannot nest structures"), yyvsp[-2].tnum.numline);
        else
        {
            ti = find_type(DB_TEXT("_s_temp_"), 0);
            memcpy(&ddlp_g.fd_entry, &struct_fd, sizeof(FIELD_ENTRY));
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 26:
#line 263 "ddlp.y"
{ memcpy(&struct_fd, &ddlp_g.fd_entry, sizeof(FIELD_ENTRY)); }
break;
case 29:
#line 275 "ddlp.y"
{
        if (!add_elem(yyvsp[-2].tstr.str, ddlp_g.fd_entry.fd_key, ti, (short *) ddlp_g.fd_entry.fd_dim, ci))
        {
            vstprintf(ddlp_g.msg, DB_TEXT("element already defined: %s"), yyvsp[-2].tstr.str);
            dderror(ddlp_g.msg, yyvsp[-2].tstr.strline);
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 30:
#line 285 "ddlp.y"
{
        dderror(DB_TEXT("invalid field name"), ddlp_g.line);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 31:
#line 295 "ddlp.y"
{
        if (!add_type(yyvsp[-2].tstr.str, ti))
            dderror(DB_TEXT("type already defined"), yyvsp[-2].tstr.strline);
        del_type(DB_TEXT("_s_temp_"));
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 32:
#line 303 "ddlp.y"
{
        dderror(DB_TEXT("missing ';'"), yyvsp[-2].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 33:
#line 309 "ddlp.y"
{
        dderror(DB_TEXT("invalid type"), yyvsp[-2].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 34:
#line 319 "ddlp.y"
{
        in_db = 1;
        vtstrcpy(ddlp_g.db_name, yyvsp[-1].tstr.str);
    }
break;
case 35:
#line 324 "ddlp.y"
{
        in_db = 1;
        if ( yyvsp[-2].tnum.num < 128 )
        {
            ddwarning(DB_TEXT("database page size must be at least 128 bytes"),
                yyvsp[-2].tnum.numline);
            pagesize = 0;
        }
        else if ( yyvsp[-2].tnum.num > 32766 )
        {
            ddwarning(DB_TEXT("maximum database page size is 32766"), yyvsp[-2].tnum.numline);
            pagesize = 0;
        }
        else
        {
            pagesize = (short) yyvsp[-2].tnum.num;
        }
        vtstrcpy(ddlp_g.db_name, yyvsp[-4].tstr.str);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 36:
#line 346 "ddlp.y"
{
        in_db = 1;
        if (find_const(yyvsp[-2].tstr.str, &tempnum))
        {
            if (tempnum < 128)
            {
                ddwarning(DB_TEXT("database page size must be at least 128 bytes"),
                        yyvsp[-2].tstr.strline);
                pagesize = 0;
            }
            else if (tempnum > 32766)
            {
                ddwarning(DB_TEXT("maximum database page size is 32766"), yyvsp[-2].tstr.strline);
                pagesize = 0;
            }
            else
                pagesize = tempnum;

            vtstrcpy(ddlp_g.db_name, yyvsp[-4].tstr.str);
        }
        else
            dderror(DB_TEXT("constant not defined"), yyvsp[-2].tstr.strline);

        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 37:
#line 373 "ddlp.y"
{
        in_db = 1;
        yyerrok;
        dderror(DB_TEXT("missing '{'"), yyvsp[-1].tstr.strline);
        vtstrcpy(ddlp_g.db_name, yyvsp[-1].tstr.str);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 38:
#line 386 "ddlp.y"
{
        if ((id_list = (ID_INFO *) psp_cGetMemory(sizeof(ID_INFO), 0)) == NULL)
            ddlp_abort(DB_TEXT("out of memory"));

        vtstrcpy(id_list->id_name, fld_comp);
        vtstrcpy(id_list->id_rec, rec_comp);
        if (ddlp_g.s_flag)
        {
            vtstrlwr(id_list->id_name);
            vtstrlwr(id_list->id_rec);
        }
        id_list->id_line = ddlp_g.line;
        id_list->next_id = NULL;
        currid = id_list;
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 39:
#line 404 "ddlp.y"
{
        for ( ids = id_list; ids; ids = ids->next_id )
        {
            if ( vtstrcmp(ids->id_name, fld_comp) == 0 )
            {
                if ( vtstrcmp(ids->id_rec, rec_comp) == 0 )
                {
                    if ( vtstrlen(rec_comp) )
                        vstprintf(ddlp_g.msg,
                            DB_TEXT("identifier %s.%s is already specified in this list"),
                            ids->id_rec, ids->id_name);
                    else
                        vstprintf(ddlp_g.msg,
                            DB_TEXT("identifier %s is already specified in this list"),
                            ids->id_name);
                    dderror(ddlp_g.msg, ddlp_g.line);
                    break;
                }
            }
        }
        if ( !ids )
        {
            currid->next_id = (ID_INFO *) psp_cGetMemory(sizeof(ID_INFO), 0);
            if ((currid = currid->next_id) == NULL)
                ddlp_abort(DB_TEXT("out of memory"));

            vtstrcpy(currid->id_name, fld_comp);
            vtstrcpy(currid->id_rec, rec_comp);
            if (ddlp_g.s_flag)
            {
                vtstrlwr(currid->id_name);
                vtstrlwr(currid->id_rec);
            }   
            currid->id_line = ddlp_g.line;
            currid->next_id = NULL;
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 40:
#line 444 "ddlp.y"
{
        dderror(DB_TEXT("non-identifier specified in list"), yyvsp[0].tnum.numline);
        ddlp_unput(DB_TEXT(';'));
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 41:
#line 455 "ddlp.y"
{
        if (vtstricmp(yyvsp[0].tstr.str, DB_TEXT("SYSTEM")) == 0)
            vtstrcpy(fld_comp, DB_TEXT("system"));
        else
            vtstrcpy(fld_comp, yyvsp[0].tstr.str);
        rec_comp[0] = 0;
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 42:
#line 465 "ddlp.y"
{
        vtstrcpy(fld_comp, yyvsp[0].tstr.str);
        if ( !ddlp_g.d_flag )
            ddwarning(DB_TEXT("must use '-d' switch to qualify field names"), ddlp_g.line);
        else
            vtstrcpy(rec_comp, yyvsp[-2].tstr.str);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 48:
#line 488 "ddlp.y"
{
        while (memList)
        {
            mem = memList->next_mem;
            psp_freeMemory(memList, 0);
            memList = mem;
        }
    }
break;
case 49:
#line 497 "ddlp.y"
{
        yyerror("invalid ddl statement");
    }
break;
case 50:
#line 505 "ddlp.y"
{
        ts_recs(id_list);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 51:
#line 511 "ddlp.y"
{
        ts_recs(NULL);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 52:
#line 517 "ddlp.y"
{
        ts_sets(id_list);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 53:
#line 523 "ddlp.y"
{
        ts_sets(NULL);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 54:
#line 529 "ddlp.y"
{
        dderror(DB_TEXT("invalid timestamp statement"), yyvsp[-2].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 55:
#line 539 "ddlp.y"
{
        add_file(NULL, id_list, yyvsp[-2].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 56:
#line 545 "ddlp.y"
{
        add_file(yyvsp[-6].tstr.str, id_list, yyvsp[-6].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 57:
#line 551 "ddlp.y"
{
        dderror(DB_TEXT("invalid file specification"), yyvsp[-2].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 58:
#line 561 "ddlp.y"
{ }
break;
case 59:
#line 563 "ddlp.y"
{ }
break;
case 62:
#line 575 "ddlp.y"
{ ddlp_g.ft_entry.ft_initial = yyvsp[0].tnum.num; }
break;
case 63:
#line 577 "ddlp.y"
{ ddlp_g.ft_entry.ft_next = yyvsp[0].tnum.num; }
break;
case 64:
#line 579 "ddlp.y"
{ ddlp_g.ft_entry.ft_pctincrease = yyvsp[0].tnum.num; }
break;
case 65:
#line 581 "ddlp.y"
{
        if (yyvsp[0].tnum.num < 128)
        {
            ddwarning(DB_TEXT("file page size must be at least 128"), yyvsp[0].tnum.numline);
            ddlp_g.ft_entry.ft_pgsize = pagesize;
        }
        else if (yyvsp[0].tnum.num > 32766)
        {
            ddwarning(DB_TEXT("maximum file page size is 32766"), yyvsp[0].tnum.numline);
            ddlp_g.ft_entry.ft_pgsize = pagesize;
        }
        else
        {
            ddlp_g.ft_entry.ft_pgsize = (short) yyvsp[0].tnum.num;
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 66:
#line 604 "ddlp.y"
{ ddlp_g.ft_entry.ft_initial = ddlp_g.ft_entry.ft_next = ddlp_g.ft_entry.ft_pctincrease = 0; }
break;
case 67:
#line 610 "ddlp.y"
{ ddlp_g.ft_entry.ft_type = 'd'; }
break;
case 68:
#line 612 "ddlp.y"
{ ddlp_g.ft_entry.ft_type = 'k'; }
break;
case 69:
#line 618 "ddlp.y"
{
        if (pagesize == 0 && ddlp_g.ft_entry.ft_type == 'k')
            ddlp_g.ft_entry.ft_pgsize = 1024;     /* THIS SHOULD CHANGE IF MAXKEYSIZE CHANGES */
        else
            ddlp_g.ft_entry.ft_pgsize = pagesize;
    }
break;
case 70:
#line 625 "ddlp.y"
{
        if ( yyvsp[-1].tnum.num < 128 )
        {
            ddwarning(DB_TEXT("file page size must be at least 128"), yyvsp[-1].tnum.numline);
            ddlp_g.ft_entry.ft_pgsize = pagesize;
        }
        else if ( yyvsp[-1].tnum.num > 32766 )
        {
            ddwarning(DB_TEXT("maximum file page size is 32766"), yyvsp[-1].tnum.numline);
            ddlp_g.ft_entry.ft_pgsize = pagesize;
        }
        else
        {
            ddlp_g.ft_entry.ft_pgsize = (short) yyvsp[-1].tnum.num;
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 71:
#line 644 "ddlp.y"
{
        if (find_const(yyvsp[-1].tstr.str, &tempnum))
        {
            if (tempnum < 128)
            {
                ddwarning(DB_TEXT("file page size must be at least 128"), yyvsp[-1].tstr.strline);
                ddlp_g.ft_entry.ft_pgsize = pagesize;
            }
            else if (tempnum > 32766)
            {
                ddwarning(DB_TEXT("maximum file page size is 32766"), yyvsp[-1].tstr.strline);
                ddlp_g.ft_entry.ft_pgsize = pagesize;
            }
            else
                ddlp_g.ft_entry.ft_pgsize = (short) tempnum;
        }
        else
            dderror(DB_TEXT("constant not defined"), yyvsp[-1].tstr.strline);

        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 72:
#line 671 "ddlp.y"
{
        cpfile(ddlp_g.ft_entry.ft_name, yyvsp[0].tstr.str, yyvsp[0].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 73:
#line 677 "ddlp.y"
{
        cpfile(ddlp_g.ft_entry.ft_name, yyvsp[0].tstr.str, yyvsp[0].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 77:
#line 690 "ddlp.y"
{
        dderror(DB_TEXT("invalid record spec"), yyvsp[-2].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 78:
#line 700 "ddlp.y"
{
        if (vtstrcmp(yyvsp[-1].tstr.str, DB_TEXT("system")) == 0)
            ddlp_abort(DB_TEXT("'system' is a reserved record name"));
        vtstrcpy(rec_comp, yyvsp[-1].tstr.str);
        add_record(yyvsp[-1].tstr.str, yyvsp[-1].tstr.strline);
        n_optkeys = 0;
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 79:
#line 714 "ddlp.y"
{ ddlp_g.rt_entry.rt_flags = 0; }
break;
case 80:
#line 716 "ddlp.y"
{ ddlp_g.rt_entry.rt_flags = STATIC; }
break;
case 83:
#line 728 "ddlp.y"
{ 
        if (ti)
        {
            /* If the name has not been entered */
            if (ti->elem == NULL)
            {
                int tot_dims;
                short tmp_dims[MAXDIMS];

                ddlp_g.fd_entry.fd_type = ti->type_char;
                ddlp_g.fd_entry.fd_len  = ti->type_size;
                ddlp_g.fd_entry.fd_flags |= (unsigned short)(ti->type_sign ? UNSIGNEDFLD : 0);
                /* calculate length of field */
                memset(tmp_dims, 0, sizeof(tmp_dims));
                for (elts = 1, tot_dims = 0; dim < MAXDIMS && ti->dims[tot_dims]; ++tot_dims)
                {  
                    tmp_dims[tot_dims] = ti->dims[tot_dims];
                    elts = elts * ti->dims[tot_dims];
                }
                for (dim = 0; dim < MAXDIMS && ddlp_g.fd_entry.fd_dim[dim]; ++dim, ++tot_dims)
                {
                    if (tot_dims >= MAXDIMS)
                    {
                        vstprintf(ddlp_g.msg,
                            DB_TEXT("too many array dimensions, maximum is %d"),
                            MAXDIMS);
                        dderror(ddlp_g.msg, yyvsp[-3].tstr.strline);
                        break;
                    }
                    tmp_dims[tot_dims] = ddlp_g.fd_entry.fd_dim[dim];
                    elts = elts * ddlp_g.fd_entry.fd_dim[dim];
                }
                memcpy(ddlp_g.fd_entry.fd_dim, tmp_dims, sizeof(tmp_dims));
                if (elts)
                    ddlp_g.fd_entry.fd_len *= (short) elts;
                add_field(rec_comp, yyvsp[-3].tstr.str, yyvsp[-3].tstr.strline, ci, ti, om);
            }
            else
            {
                add_field(rec_comp, NULL, yyvsp[-3].tstr.strline, ci, ti, om);
                add_struct_fields(rec_comp, yyvsp[-3].tstr.strline, ti->elem);
                if (!vtstrcmp(ti->type_name, DB_TEXT("_s_temp_")))
                {
                    del_type(DB_TEXT("_s_temp_"));
                    ti = NULL;
                }
                close_struct(rec_comp, yyvsp[-3].tstr.str, yyvsp[-3].tstr.strline);
            }
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 84:
#line 781 "ddlp.y"
{
        dderror(DB_TEXT("invalid field name"), ddlp_g.line);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 85:
#line 787 "ddlp.y"
{
        dderror(DB_TEXT("invalid field spec"), yyvsp[0].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 86:
#line 797 "ddlp.y"
{
        /* Handle DIRECTREF macro for om */
        if ((om = (OM_INFO *) psp_cGetMemory(sizeof(OM_INFO), 0)) == NULL)
            ddlp_abort(DB_TEXT("out of memory"));

        om->related = FALSE;
        if (ti->type_char != 'd')
            dderror(DB_TEXT("invalid field spec"), yyvsp[-1].tnum.numline); 
        else
        {
            om->dref = TRUE;
            vtstrcpy(om->dref_name, yyvsp[0].tstr.str); 
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 87:
#line 814 "ddlp.y"
{
        /* Handle RELATEDTO macro for om */
        if ((om = (OM_INFO *) psp_cGetMemory(sizeof(OM_INFO), 0)) == NULL)
	    ddlp_abort(DB_TEXT("out of memory"));

        om->dref = FALSE;
        om->related = TRUE;
        vtstrcpy(om->rel_rec, yyvsp[-2].tstr.str);
        vtstrcpy(om->rel_field, vtstrupr(yyvsp[0].tstr.str));
    }
break;
case 88:
#line 825 "ddlp.y"
{
        if ((om = (OM_INFO *)psp_cGetMemory(sizeof(OM_INFO), 0)) == NULL)
	    ddlp_abort(DB_TEXT("out of memory"));

        om->dref = FALSE;
        om->related = FALSE;
    }
break;
case 89:
#line 837 "ddlp.y"
{ sign_flag = 0; }
break;
case 90:
#line 839 "ddlp.y"
{ sign_flag = 1; }
break;
case 91:
#line 845 "ddlp.y"
{
        if (!in_db)
            dderror(DB_TEXT("predefined structures cannot contain keys"), yyvsp[0].tnum.numline);
        else
            ddlp_g.fd_entry.fd_key = 'u';
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 92:
#line 854 "ddlp.y"
{
        if (!in_db)
            dderror(DB_TEXT("predefined structures cannot contain keys"), yyvsp[0].tnum.numline);
        else
            ddlp_g.fd_entry.fd_key = 'd';
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 93:
#line 863 "ddlp.y"
{  ddlp_g.fd_entry.fd_flags = 0; ddlp_g.fd_entry.fd_key = 'n'; }
break;
case 94:
#line 869 "ddlp.y"
{
        /* Optional key number will begin with 1 */
        ++n_optkeys;
        if ( n_optkeys > OPTKEY_LIMIT )
        {
            vstprintf(ddlp_g.msg,
                DB_TEXT("optional key per record limit (%d keys) exceeded"),
                OPTKEY_LIMIT);
            dderror(ddlp_g.msg, yyvsp[0].tnum.numline);
        }
        else
            ddlp_g.fd_entry.fd_flags = (short) (n_optkeys << OPTKEYSHIFT);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 95:
#line 885 "ddlp.y"
{ ddlp_g.fd_entry.fd_flags = 0; }
break;
case 96:
#line 891 "ddlp.y"
{ 
        if ((ti = find_type(yyvsp[0].tstr.str, sign_flag)) == NULL)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("invalid data type: %s"), yyvsp[0].tstr.str);
            dderror(ddlp_g.msg, yyvsp[0].tstr.strline);
        }
        if ( vtstrcmp(yyvsp[0].tstr.str, DB_TEXT("int")) == 0 )
        {
            vstprintf(ddlp_g.msg, DB_TEXT("non-portable data type: %s"), yyvsp[0].tstr.str);
            ddwarning(ddlp_g.msg, yyvsp[0].tstr.strline);
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 100:
#line 917 "ddlp.y"
{
        /* clear array dimension info */
        for ( dim = 0; dim < MAXDIMS; ++dim )
        {
            ddlp_g.fd_entry.fd_dim[dim] = 0;
            ci[dim] = NULL;
        }
        dim = 0;
    }
break;
case 103:
#line 937 "ddlp.y"
{
        if ( dim == MAXDIMS )
        {
            vstprintf(ddlp_g.msg, DB_TEXT("too many array dimensions, maximum is %d"), MAXDIMS);
            dderror(ddlp_g.msg, yyvsp[-1].tnum.numline);
        }
        else
            ddlp_g.fd_entry.fd_dim[dim++] = (short) (yyvsp[-1].tnum.num);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 104:
#line 949 "ddlp.y"
{
        if (find_const(yyvsp[-1].tstr.str, &tempnum))
        {
            if (dim == MAXDIMS)
            {
                vstprintf(ddlp_g.msg, DB_TEXT("too many array dimensions, maximum is %d"), MAXDIMS);
                dderror(ddlp_g.msg, yyvsp[-1].tstr.strline);
            }
            else
            {
                ddlp_g.fd_entry.fd_dim[dim] = tempnum;
                ci[dim++] = find_const(yyvsp[-1].tstr.str, &tempnum);
            }
        }
        else
            dderror(DB_TEXT("constant not defined"), yyvsp[-1].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 108:
#line 984 "ddlp.y"
{ 
        /* clear array dimension info */
        for ( dim = 0; dim < MAXDIMS; ++dim )
            ddlp_g.fd_entry.fd_dim[dim] = 0, ci[dim] = NULL;
        dim = 0;
        ddlp_g.fd_entry.fd_type = 'k';
        add_field(rec_comp, yyvsp[0].tstr.str, yyvsp[0].tstr.strline, ci, NULL, NULL); 
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 111:
#line 1005 "ddlp.y"
{
        add_key(rec_comp, yyvsp[-2].tstr.str, yyvsp[-2].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 112:
#line 1015 "ddlp.y"
{ ddlp_g.kt_entry.kt_sort = 'a'; }
break;
case 113:
#line 1017 "ddlp.y"
{ ddlp_g.kt_entry.kt_sort = 'd'; }
break;
case 114:
#line 1019 "ddlp.y"
{ ddlp_g.kt_entry.kt_sort = 'a'; }
break;
case 115:
#line 1025 "ddlp.y"
{
        current_set->om_set_type = DB_BITMAP;
    }
break;
case 116:
#line 1029 "ddlp.y"
{
        dderror(DB_TEXT("invalid set spec"), yyvsp[-3].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 117:
#line 1035 "ddlp.y"
{
        current_set->om_set_type = DB_BLOB;
    }
break;
case 118:
#line 1039 "ddlp.y"
{
        dderror(DB_TEXT("invalid set spec"), yyvsp[-3].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 119:
#line 1045 "ddlp.y"
{
        current_set->om_set_type = VLTEXT;
    }
break;
case 120:
#line 1049 "ddlp.y"
{
        dderror(DB_TEXT("invalid set spec"), yyvsp[-3].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 122:
#line 1056 "ddlp.y"
{
        dderror(DB_TEXT("invalid set spec"), yyvsp[-2].tnum.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 123:
#line 1066 "ddlp.y"
{
        last_ordering = ddlp_g.st_entry.st_order;
        current_set = add_set(yyvsp[-3].tstr.str, yyvsp[0].tstr.str, yyvsp[-3].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 125:
#line 1082 "ddlp.y"
{ ddlp_g.st_entry.st_order = 'f'; }
break;
case 126:
#line 1084 "ddlp.y"
{ ddlp_g.st_entry.st_order = 'l'; }
break;
case 127:
#line 1086 "ddlp.y"
{ ddlp_g.st_entry.st_order = 'a'; }
break;
case 128:
#line 1088 "ddlp.y"
{ ddlp_g.st_entry.st_order = 'd'; }
break;
case 129:
#line 1090 "ddlp.y"
{ ddlp_g.st_entry.st_order = 'n'; }
break;
case 130:
#line 1092 "ddlp.y"
{
        vstprintf(ddlp_g.msg, DB_TEXT("invalid order type, '%s'"), yyvsp[0].tstr.str);
        dderror(ddlp_g.msg, yyvsp[0].tstr.strline);
        ddlp_g.st_entry.st_order = 'f';
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 131:
#line 1104 "ddlp.y"
{ 
        vtstrcpy(yyval.tstr.str, yyvsp[-1].tstr.str); 
        add_xref(NULL, yyvsp[-1].tstr.str, 'r', yyvsp[-1].tstr.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 132:
#line 1115 "ddlp.y"
{
        memList = (MEM_INFO *) psp_cGetMemory(sizeof(MEM_INFO), 0);
        if (memList == NULL)
            ddlp_abort(DB_TEXT("out of memory"));

        vtstrcpy(memList->rec_name, yyvsp[0].tstr.str);
        memList->next_mem = NULL;
        currmem = memList;
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 133:
#line 1127 "ddlp.y"
{
        currmem->next_mem = (MEM_INFO *) psp_cGetMemory(sizeof(MEM_INFO), 0);
        if ((currmem = currmem->next_mem) == NULL)
            ddlp_abort(DB_TEXT("out of memory"));

        vtstrcpy(currmem->rec_name, yyvsp[0].tstr.str);
        currmem->next_mem = NULL;
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 134:
#line 1142 "ddlp.y"
{
        if ( last_ordering == 'a' || last_ordering == 'd' )
            dderror(DB_TEXT("sort field(s) missing from member clause"), yyvsp[-2].tnum.numline);
        if ( vtstrcmp(yyvsp[-1].tstr.str, DB_TEXT("system")) == 0 )
            dderror(DB_TEXT("'system' cannot be a member"), yyvsp[-1].tstr.strline);
        else 
        {
            add_member(yyvsp[-1].tstr.str, NULL, yyvsp[-1].tstr.strline);
            vtstrcpy(yyval.tstr.str, yyvsp[-1].tstr.str); 
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
case 135:
#line 1156 "ddlp.y"
{
        if ( last_ordering != 'a' && last_ordering != 'd' )
            dderror(DB_TEXT("sort field(s) not required in member clause"), yyvsp[-4].tnum.numline);
        if ( vtstrcmp(yyvsp[-3].tstr.str, DB_TEXT("system")) == 0 )
            dderror(DB_TEXT("'system' cannot be a member"), yyvsp[-3].tstr.strline);
        else
        {
            add_member(yyvsp[-3].tstr.str, id_list, yyvsp[-3].tstr.strline);
            vtstrcpy(yyval.tstr.str, yyvsp[-3].tstr.str); 
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
break;
#line 1686 "y.tab.c"
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
