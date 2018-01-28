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

/* db.* dal Utility type definitions */

#define BEG_PRINTABLE 0                /* start of printable types */
#define RECORD  0
#define FIELD   1
#define LITERAL 2
#define INTPTR  3
#define DBAPTR  4
#define CLOCK_T 5
#define END_PRINTABLE CLOCK_T

#define SET     6
#define FLDPTR  7
#define RECPTR  8


#define N_TSK            0        /* no parameters */
#define I_TSK            1        /* Integer */
#define L_TSK            2        /* Literal */
#define S_TSK_DBN        3        /* Set type */
#define F_TSK_DBN        4        /* Field type */
#define R_TSK_DBN        5        /* Record type */
#define S_S_TSK_DBN      6        /* Set type, Set type */
#define FP_TSK           7        /* Field contents ptr */
#define RP_TSK_DBN       8        /* Record contents ptr */
#define DP_TSK_DBN       9        /* DB_ADDR ptr */
#define IP_TSK_DBN      10        /* Integer ptr */
#define S_IP_TSK_DBN    11        /* Set type, Integer ptr */
#define S_DP_TSK_DBN    12        /* Set type, DB_ADDR ptr */
#define R_RP            13        /* Record type, Record contents ptr */
#define F_FP            14        /* Field type, Field contents ptr */
#define S_F_FP_TSK_DBN  15        /* Set type, Field type, Field contents ptr */
#define F_L_TSK_DBN     16        /* Field type, Literal */
#define I_LP_TSK_DBN    17        /* Integer, Lock Packet */
#define R_L_TSK_DBN     18        /* Record type, Literal */
#define CP_IP_TSK_DBN   19        /* Currency buffer ptr, Integer ptr */
#define REN_TSK         20        /* d_renfile function */
#define C_TSK_DBN       21        /* Currency buffer */
#define L_L_SG_TSK      22        /* Literal, Literal */
#define S_L_TSK_DBN     23        /* Set type, Literal */
#define I_I_TSK         24        /* Integer, Integer */
#define U               25        /* Unimplemented */

#define N_TSK_DBN       26        /* no parameters */
#define I_TSK_DBN       27        /* Integer */
#define F_FP_TSK_DBN    29        /* Field type, Field contents ptr */
#define R_RP_TSK_DBN    30        /* Record type, Record contents ptr */
#define CT              31        /* clock_t  */
#define CT_CT_CT        32        /* clock_t, clock_t clock_t */

#define DAL_ERR   -100

typedef int (EXTERNAL_FCN * D_API_FCN) ();

struct fcnlist
{
    DB_TCHAR    f_name[10];
    D_API_FCN   fcn;
    int         f_fcntype;
};

typedef struct inst
{
    DB_TCHAR             i_name[30];
    DB_TCHAR             i_p1[30];
    DB_TCHAR             i_f1[30];
    DB_TCHAR             i_p2[30];
    DB_TCHAR             i_f2[30];
    DB_TCHAR             i_p3[30];
    DB_TCHAR             i_f3[30];
    struct inst         *i_next;
    struct inst         *i_loop;
    struct printfield   *i_pfld;
} INST;

typedef struct printfield
{
    DB_TCHAR             pf_rec[30];
    DB_TCHAR             pf_fld[30];
    struct printfield   *pf_next;
} PRINTFIELD;


#ifdef VXWORKS
/*
    For VxWorks, redefine functions and global variables defined by
    yacc, to avoid naming conflicts with other db.* utilities built
    with yacc.
*/
#define yyparse      dal_parse
#define yy_parse     dal_xparse
#define yylex        dal_lex
#define yyerror      dal_error
#define yychar       dal_char
#define yynerrs      dal_nerrs
#define yyerrflag    dal_errflag
#define yylval       dal_lval
#define yyval        dal_val
#define yyv          dal_v
#define yyexit       dal_exit
#define yydebug      dal_debug
#define yytoken      dal_token
#define dderror      dal_dderror

#endif /* VXWORKS */


extern int  yyparse(void);
extern int  dalcurr(FILE *);
extern int  dalexec(INST *);
extern void dalerror(DB_TCHAR *);
extern void newinst(INST **);
extern void newprint(PRINTFIELD **);
extern int  EXTERNAL_FCN def_rec(int, char *, DB_TASK *, int);
extern int  EXTERNAL_FCN def_fld(int, char *, DB_TASK *, int);
extern int  dal_rewind(INST *);
extern int  dalio(INST *);
extern void dalio_close(void);
extern int  getcstring(char *, int, FILE *);
extern int  getwstring(wchar_t *, int, FILE *);
extern void init_dallex(void);
extern int  yylex(void);
extern void yyerror(DB_TCHAR *);
extern void dderror(DB_TCHAR *, int);
extern int  dalschem(DB_TCHAR *);
extern void init_dalvar(void);
extern void freevar(void);
extern char *addvar(int, int, DB_TCHAR *, int);
extern char *findvar(int, DB_TCHAR *, int *);
extern DB_TCHAR *genlit(DB_TCHAR *);
extern void freeloop(INST **);
extern void freeinst(INST **);
extern int  dalwhile(INST *);
extern void bgets(char *, int, int);
extern int  nextc(int);
extern void pr_mtype(FILE *, int, DB_ADDR);
extern void pr_otype(FILE *, int, DB_ADDR);
extern int  EXTERNAL_FCN get_clock(clock_t *);
extern int  EXTERNAL_FCN cmp_clock(clock_t *, clock_t *, clock_t *);


