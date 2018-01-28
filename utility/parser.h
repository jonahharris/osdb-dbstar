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

    db.* Database Definition Language Processor

    parser.h - ddlp parser header file.

-----------------------------------------------------------------------*/

#include <ctype.h>

/* ROM SPECIAL Set Types */
enum SPECIAL_TYPE  { SYSTEM, NORMAL, DB_BITMAP, DB_BLOB, VLTEXT, 
                     SUBCLASS, SUBCLASS_BASE, SUBCLASS_DERIVED,
                     POLYMORPH, POLYMORPH_BASE, POLYMORPH_DERIVED };
                            
enum MEMBER_TYPE   { FIRST_MEMBER, LAST_MEMBER };

enum INHERIT_TYPE  { INHERIT_DATA, MEMBER_DATA };

enum MEMBER_DATA_TYPE  { PUBLIC_DATA, PROTECTED_DATA, PRIVATE_DATA };

typedef struct id_info {
    struct id_info *next_id;
    DB_TCHAR id_name[NAMELEN];
    DB_TCHAR id_rec[NAMELEN];
    int id_line;
} ID_INFO;

typedef struct om_info {
    short dref;
    DB_TCHAR dref_name[NAMELEN];
    short related;
    DB_TCHAR rel_rec[NAMELEN];
    DB_TCHAR rel_field[NAMELEN];
} OM_INFO;

typedef struct mem_info {
    struct mem_info *next_mem;
    DB_TCHAR rec_name[NAMELEN];
} MEM_INFO;

typedef struct const_info
{
    struct const_info *next_const;
    DB_TCHAR const_name[NAMELEN];
    short value;
} CONST_INFO;

typedef struct elem_info
{
    struct elem_info *next_elem;
    DB_TCHAR field_name[NAMELEN];
    struct type_info *field_type;
    short dims[MAXDIMS];
    CONST_INFO *dim_const[MAXDIMS];
    int use_count;
    char key;
} ELEM_INFO;

typedef struct type_info
{
    struct   type_info *next_type;
    DB_TCHAR type_name[NAMELEN];
    short    type_size;
    char     type_char;
    int      type_sign;
    short    dims[MAXDIMS];
    ELEM_INFO *elem;
} TYPE_INFO;

/* file table */
struct file_info {
    struct file_info *next_file;
    DB_TCHAR fileid[NAMELEN];
    ID_INFO *name_list;
    FILE_ENTRY ft;
};

/* record table */
struct record_info {
    struct record_info *next_rec;
    DB_TCHAR rec_name[NAMELEN];
    short    om_rec_type;
    DB_TCHAR om_class_type[NAMELEN*2];  /* record name may have "_BaseObj" appended */
    short    om_polymorph_member_count;
    short    om_set_memberof_count;
    DB_TCHAR om_default_set[NAMELEN];
    short    om_key_count;
    short    rec_num;
    short    totown;
    short    totmem;
    RECORD_ENTRY rt;
};

/* data field table */
struct field_info {
    struct field_info *next_field;
    DB_TCHAR fld_name[NAMELEN];
    CONST_INFO *dim_const[MAXDIMS];
    TYPE_INFO *type;
    FIELD_ENTRY fd;
    OM_INFO  *object_info;
};

/* compound key table */
struct ddlkey_info
{
    struct ddlkey_info *next_key;
    KEY_ENTRY kt;
};

/* set table */
struct set_info
{
    struct set_info *next_set;
    DB_TCHAR set_name[NAMELEN];
    short om_set_type;
    struct record_info *set_rec;
    SET_ENTRY st;
};

/* member table */
struct member_info
{
    struct member_info *next_mbr;
    struct record_info *mem_rec;
    MEMBER_ENTRY mt;
};

/* set sort field table */
struct sort_info
{
    struct sort_info *next_sort;
    SORT_ENTRY sort_field;
};

typedef struct strtok {
    DB_TCHAR str[80];
    int strline;
} STRTOK;

typedef struct numtok {
    int num;
    int numline;
} NUMTOK;

/***************************************************************************/

#ifdef VXWORKS
/*
    For VxWorks, redefine functions and global variables defined by
    yacc, to avoid naming conflicts with other db.* utilities built
    with yacc.
*/
#define yyparse      ddlp_parse
#define yy_parse     ddlp_xparse
#define yylex        ddlp_lex
#define yyerror      ddlp_error
#define yychar       ddlp_char
#define yynerrs      ddlp_nerrs
#define yyerrflag    ddlp_errflag
#define yylval       ddlp_lval
#define yyval        ddlp_val
#define yyv          ddlp_v
#define yyexit       ddlp_exit
#define yydebug      ddlp_debug
#define yytoken      ddlp_token
#define dderror      ddlp_dderror
#define ddwarning    ddlp_ddwarning

int ddlp_stricmp(const char *, const char *));
#define vstricmp     ddlp_stricmp

#endif /* VXWORKS */


int                  add_const        (DB_TCHAR *, short);
int                  add_elem         (DB_TCHAR *, char, TYPE_INFO *, short *, CONST_INFO **);
struct field_info *  add_field        (DB_TCHAR *, DB_TCHAR *, int, CONST_INFO **, TYPE_INFO *, OM_INFO *);
struct file_info *   add_file         (DB_TCHAR *, ID_INFO *, int);
void                 add_key          (DB_TCHAR *, DB_TCHAR *, int);
struct member_info * add_member       (DB_TCHAR *, ID_INFO *, int);
struct record_info * add_record       (DB_TCHAR *, int);
struct set_info *    add_set          (DB_TCHAR *, DB_TCHAR *, int);
void                 add_struct_fields(DB_TCHAR *, int, ELEM_INFO *);
int                  add_struct_type  (DB_TCHAR *);
int                  add_type         (DB_TCHAR *, TYPE_INFO *);
void                 add_xref         (DB_TCHAR *, DB_TCHAR *, char, int);
void                 free_xref_list   (void);
void                 close_struct     (DB_TCHAR *, DB_TCHAR *, int);
void                 cpfile           (DB_TCHAR *, DB_TCHAR *, int);
void                 ddlp_abort       (DB_TCHAR *);
void                 dderror          (DB_TCHAR *, int);
void                 ddwarning        (DB_TCHAR *, int);
void                 del_type         (DB_TCHAR *);
CONST_INFO *         find_const       (DB_TCHAR *, short *);
struct record_info * find_rec         (DB_TCHAR *, short *);
TYPE_INFO *          find_type        (DB_TCHAR *, int);
void                 finish_up        (void);
void                 init_lists       (void);
void                 free_lists       (void);
void                 print_tables     (void);
void                 print_xref       (void);
DB_TCHAR *           strLower         (DB_TCHAR *);
DB_TCHAR *           strUpper         (DB_TCHAR *);
void                 ts_recs          (ID_INFO *);
void                 ts_sets          (ID_INFO *);
void                 ddlp_unput       (int);
void                 write_tables     (void);
void                 yyerror          (DB_TCHAR *);
int                  yylex            (void);
int                  yyparse          (void);
void                 ddlpInit         (void);
void                 lexInit          (void);
void                 tableInit        (void);
void                 xrefInit         (void);

