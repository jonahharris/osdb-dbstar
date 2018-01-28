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

/* db_Import translator structure definitions */

/* return codes */
#define OK 0
#define FOUND 1
#define NOTFOUND 2
#define SKIP 3
#define FAILURE 4

#define SKIP_DBA (-2L)

#define REFS 512
#define FILELEN 81
#define FIELDLEN 20

#define LOOP 0
#define ENDLOOP 1
#define RECORD 2
#define CONNECT 3


struct spec
{
    union
    {
        struct rec *sp_recptr;
        struct con *sp_conptr;
        struct loop *sp_looptr;
        struct endloop *sp_endptr;
    }  u;
    int sp_type;
    struct spec *sp_next;
};

#define MAXFIELDS 128

struct dbflds
{
    int db_fldname;
};

struct loop
{
    FILE *l_fp;
    int l_nflds;
    int l_linelen;
    int l_numrecs;
    int l_maxrecs;
    struct dbflds l_dbf[MAXFIELDS];
    DB_TCHAR l_line[4300];
    DB_TCHAR l_fname[FILELEN];
};

struct endloop
{
    struct spec *e_ptr;
};


#ifdef VXWORKS
/*
    For VxWorks, redefine functions and global variables defined by
    yacc, to avoid naming conflicts with other db.* utilities built
    with yacc.
*/
#define yyparse      dbimp_parse
#define yy_parse     dbimp_xparse
#define yylex        dbimp_lex
#define yyerror      dbimp_error
#define yychar       dbimp_char
#define yynerrs      dbimp_nerrs
#define yyerrflag    dbimp_errflag
#define yylval       dbimp_lval
#define yyval        dbimp_val
#define yyv          dbimp_v
#define yyexit       dbimp_exit
#define yydebug      dbimp_debug
#define yytoken      dbimp_token
#define dderror      dbimp_dderror
#define ddwarning    dbimp_ddwarning

#endif /* VXWORKS */

#define DBIMP_CREATE 0
#define DBIMP_UPDATE 1
#define DBIMP_AUTO 2
#define DBIMP_FIND 3
struct rec
{
    int rec_ndx;                        /* index into task->record_table */
    int rec_htype;                      /* handling type */
    struct handling *rec_hptr;
    struct fld *rec_fldptr;
};

struct con
{
    int con_ndx;                        /* index into task->set_table */
};

struct fld
{
    int fld_ndx;                        /* index into task->field_table */
    DB_TCHAR fld_file[FILELEN];
    int fld_name;
    int fld_dim[MAXDIMS];
    int fld_dims;
    int fld_offset;
    struct fld *fld_next;
};

struct handling
{
    DB_TCHAR h_file[FILELEN];
    int h_name;
};


struct spec *new_spec();
struct endloop *new_end();
struct loop *new_loop();
struct rec *new_rec();
struct fld *new_fld();
struct handling *new_hand();
struct con *new_con();

extern DB_TCHAR *asm_val(struct handling *);
extern int dbexec(struct spec *);
extern FILE *dbf_open(DB_TCHAR *);
extern int dbf_read(FILE *, DB_TCHAR *);
extern void dbf_close(FILE *);
extern int rec_exec(struct rec *);
extern int con_exec(struct con *);
extern int rec_create(int, struct fld *);
extern int rec_ref(int, struct handling *);
extern int rec_find(int, struct handling *);
extern int rec_imp(int, struct fld *);
extern int fld_move(struct fld *, char *);
extern int mv_char(char *, int, int, DB_TCHAR *);
extern int mv_binary(char *, int, int, DB_TCHAR *);
extern int mv_wchar(char *, int, int, DB_TCHAR *);
extern int mv_wbinary(char *, int, int, DB_TCHAR *);
extern int mv_fld(char *, int, int, DB_TCHAR *);
extern DB_TCHAR *find_fld(int, DB_TCHAR *, int *);
extern DB_TCHAR next_char(DB_TCHAR *, int *);
extern int blanks(DB_TCHAR *, int);
extern int vec_idx(int, short *, int *, int *);
extern int yyparse(void);
extern int init_key(void);
extern int yylex(void);
extern void yyerror(DB_TCHAR *);
extern void dderror(DB_TCHAR *, int);
extern void ddwarning(DB_TCHAR *);
extern void dbimp_abort(DB_TCHAR *);
extern struct spec *new_spec(int);
extern struct endloop *new_end(struct spec *);
extern struct loop *new_loop(DB_TCHAR *);
extern struct rec *new_rec(int, struct handling *, struct fld *);
extern struct handling *new_hand(DB_TCHAR *, int);
extern struct con *new_con(void);
extern struct fld *new_fld(DB_TCHAR *, int *, int, DB_TCHAR *, int *, int);



