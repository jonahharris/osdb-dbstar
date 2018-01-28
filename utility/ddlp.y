%{
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
%}

%start ddl

%union {
    STRTOK tstr;
    NUMTOK tnum;
}

%token <tnum> T_ALLOCATION T_ASCENDING T_BITMAP T_BLOB T_BY T_COMPOUND
%token <tnum> T_CONST T_CONTAINS T_DATA T_DATABASE T_DEFINE T_DESCENDING
%token <tnum> T_DIRECTREF T_FILE T_FIRST T_INCLUDE T_INITIAL T_KEY T_LAST
%token <tnum> T_LINE T_MEMBER T_NEXT T_NUMBER T_OPTIONAL T_ORDER T_OWNER
%token <tnum> T_PCTINCREASE T_PAGESIZE T_RECORD T_RECS T_RELATEDTO T_SET
%token <tnum> T_SETS T_STATIC T_STRUCT T_THRU T_TIMESTAMP T_TYPEDEF T_UNIQUE
%token <tnum> T_UNSIGNED T_VARILEN
%token <tstr> T_IDENT T_STRING

%token <tnum> '{' '}' ';' ','

%type <tnum> closing_brace const_cmnd cpp_cmnd cpp_cmnds db_spec ddl ddl_stmt
%type <tnum> ddl_stmts define_cmnd file_spec include_cmnd key_defs key_field
%type <tnum> ordering rec_id rec_type record_spec set_spec struct_cmnd
%type <tnum> struct_spec timestamp type_spec typedef_cmnd
%type <tstr> owner member
%%

ddl             :       cpp_cmnds db_spec ddl_stmts closing_brace
                     |       db_spec ddl_stmts closing_brace
                     ;
closing_brace   :       '}'
                     |       '}' ';'
    {
        ddwarning(DB_TEXT("';' not required after '}'"), $2.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       error
    {
        yyerrok;
        dderror(DB_TEXT("missing '}'"), ddlp_g.line);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

cpp_cmnds       :       cpp_cmnd
                     |       cpp_cmnd cpp_cmnds
                     ;

cpp_cmnd        :       const_cmnd
                     |       define_cmnd
                     |       include_cmnd
                     |       struct_cmnd
                     |       typedef_cmnd
                     ;

/* ------------------------------------------------------------------------ */

const_cmnd      :       T_CONST T_IDENT '=' T_NUMBER ';'
    {
        if (!add_const($2.str, (short) $4.num))
            ddwarning(DB_TEXT("constant redefined"), $2.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_CONST T_IDENT '=' T_IDENT ';'
    {
        if (find_const($4.str, &tempnum))
        {
            if (!add_const($2.str, tempnum))
                ddwarning(DB_TEXT("constant redefined"), $2.strline);
        }
        else
            dderror(DB_TEXT("constant not defined"), $4.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_CONST T_IDENT '=' T_NUMBER error
    {
        dderror(DB_TEXT("missing ';'"), $4.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_CONST T_IDENT '=' T_IDENT error
    {
        dderror(DB_TEXT("missing ';'"), $4.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_CONST T_IDENT error
    {
        dderror(DB_TEXT("missing '=' after identifier"), $2.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_CONST error
    {
        dderror(DB_TEXT("missing identifier after const"), $1.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

define_cmnd     :       T_DEFINE T_IDENT T_NUMBER
    {
        if (!add_const($2.str, (short) $3.num))
            ddwarning(DB_TEXT("constant redefined"), $2.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_DEFINE T_IDENT T_IDENT
    {
        if (find_const($3.str, &tempnum))
        {
            if (!add_const($2.str, tempnum))
                ddwarning(DB_TEXT("const redefined"), $2.strline);
        }
        else
            dderror(DB_TEXT("constant not defined"), $3.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

include_cmnd    :       T_INCLUDE T_IDENT
                     ;

/* ------------------------------------------------------------------------ */

struct_cmnd     :       struct_spec ';'
                     |       struct_spec error
    {
        dderror(DB_TEXT("missing ';'"), ddlp_g.line);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

struct_spec     :       struct_init T_IDENT '{' struct_elems '}'
    {
        DB_TCHAR tempbuff[NAMELEN];

        vtstrcpy(tempbuff, DB_TEXT("_s_"));
        vtstrcat(tempbuff, $2.str);
        
        if (ddlp_g.elem_list)
        {
            if (!add_struct_type(tempbuff))
            {
                vstprintf(ddlp_g.msg, DB_TEXT("type already defined: %s"), $2.str);
                dderror(ddlp_g.msg, $2.strline);
            }
        }
        
        if ((ti = find_type(tempbuff, 0)) == NULL)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("structure '%s' not defined.\n"), $2.str);
            dderror(ddlp_g.msg, $2.strline);
        }

        memcpy(&ddlp_g.fd_entry, &struct_fd, sizeof(FIELD_ENTRY));

        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |        struct_init '{' struct_elems '}'
    {
        if (!add_struct_type(DB_TEXT("_s_temp_")))
            dderror(DB_TEXT("cannot nest structures"), $2.numline);
        else
        {
            ti = find_type(DB_TEXT("_s_temp_"), 0);
            memcpy(&ddlp_g.fd_entry, &struct_fd, sizeof(FIELD_ENTRY));
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

struct_init     :        T_STRUCT
    { memcpy(&struct_fd, &ddlp_g.fd_entry, sizeof(FIELD_ENTRY)); }
                     ;

/* ------------------------------------------------------------------------ */

struct_elems    :       struct_elem
                     |       struct_elem struct_elems
                     ;

/* ------------------------------------------------------------------------ */

struct_elem     :       key_spec type_spec T_IDENT size_specs ';'
    {
        if (!add_elem($3.str, ddlp_g.fd_entry.fd_key, ti, (short *) ddlp_g.fd_entry.fd_dim, ci))
        {
            vstprintf(ddlp_g.msg, DB_TEXT("element already defined: %s"), $3.str);
            dderror(ddlp_g.msg, $3.strline);
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       key_spec type_spec error
    {
        dderror(DB_TEXT("invalid field name"), ddlp_g.line);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

typedef_cmnd    :       T_TYPEDEF type_spec T_IDENT size_specs ';'
    {
        if (!add_type($3.str, ti))
            dderror(DB_TEXT("type already defined"), $3.strline);
        del_type(DB_TEXT("_s_temp_"));
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_TYPEDEF type_spec T_IDENT size_specs error
    {
        dderror(DB_TEXT("missing ';'"), $3.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_TYPEDEF type_spec error
    {
        dderror(DB_TEXT("invalid type"), $1.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

db_spec         :       T_DATABASE T_IDENT '{'
    {
        in_db = 1;
        vtstrcpy(ddlp_g.db_name, $2.str);
    }
                     |       T_DATABASE T_IDENT '[' T_NUMBER ']' '{'
    {
        in_db = 1;
        if ( $4.num < 128 )
        {
            ddwarning(DB_TEXT("database page size must be at least 128 bytes"),
                $4.numline);
            pagesize = 0;
        }
        else if ( $4.num > 32766 )
        {
            ddwarning(DB_TEXT("maximum database page size is 32766"), $4.numline);
            pagesize = 0;
        }
        else
        {
            pagesize = (short) $4.num;
        }
        vtstrcpy(ddlp_g.db_name, $2.str);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_DATABASE T_IDENT '[' T_IDENT ']' '{'
    {
        in_db = 1;
        if (find_const($4.str, &tempnum))
        {
            if (tempnum < 128)
            {
                ddwarning(DB_TEXT("database page size must be at least 128 bytes"),
                        $4.strline);
                pagesize = 0;
            }
            else if (tempnum > 32766)
            {
                ddwarning(DB_TEXT("maximum database page size is 32766"), $4.strline);
                pagesize = 0;
            }
            else
                pagesize = tempnum;

            vtstrcpy(ddlp_g.db_name, $2.str);
        }
        else
            dderror(DB_TEXT("constant not defined"), $4.strline);

        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_DATABASE T_IDENT error
    {
        in_db = 1;
        yyerrok;
        dderror(DB_TEXT("missing '{'"), $2.strline);
        vtstrcpy(ddlp_g.db_name, $2.str);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

id_list         :       id_item
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
                     |       id_list ',' id_item
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
                     |       error ';'
    {
        dderror(DB_TEXT("non-identifier specified in list"), $2.numline);
        ddlp_unput(DB_TEXT(';'));
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

id_item         :       T_IDENT
    {
        if (vtstricmp($1.str, DB_TEXT("SYSTEM")) == 0)
            vtstrcpy(fld_comp, DB_TEXT("system"));
        else
            vtstrcpy(fld_comp, $1.str);
        rec_comp[0] = 0;
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_IDENT '.' T_IDENT
    {
        vtstrcpy(fld_comp, $3.str);
        if ( !ddlp_g.d_flag )
            ddwarning(DB_TEXT("must use '-d' switch to qualify field names"), ddlp_g.line);
        else
            vtstrcpy(rec_comp, $1.str);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

ddl_stmts       :       ddl_stmt
                     |       ddl_stmts ddl_stmt
                     ;

/* ------------------------------------------------------------------------ */

ddl_stmt        :       timestamp
                     |       file_spec
                     |       record_spec
                     |       set_spec
    {
        while (memList)
        {
            mem = memList->next_mem;
            psp_freeMemory(memList, 0);
            memList = mem;
        }
    } 
                     |       error 
    {
        yyerror("invalid ddl statement");
    }
                     ;

/* ------------------------------------------------------------------------ */

timestamp       :       T_TIMESTAMP T_RECS id_list ';'
    {
        ts_recs(id_list);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_TIMESTAMP T_RECS ';'
    {
        ts_recs(NULL);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_TIMESTAMP T_SETS id_list ';'
    {
        ts_sets(id_list);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_TIMESTAMP T_SETS ';'
    {
        ts_sets(NULL);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_TIMESTAMP error ';'
    {
        dderror(DB_TEXT("invalid timestamp statement"), $1.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

file_spec       :       file_type file_id allocation T_CONTAINS id_list ';'
    {
        add_file(NULL, id_list, $4.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       file_type T_IDENT '=' file_id allocation T_CONTAINS id_list ';'
    {
        add_file($2.str, id_list, $2.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       file_type T_FILE error ';'
    {
        dderror(DB_TEXT("invalid file specification"), $2.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

allocation      :      
    { }
                     |       '(' alloc_opts ')'
    { }
                     ;

/* ------------------------------------------------------------------------ */

alloc_opts      :       alloc_opt
                     |       alloc_opts alloc_opt
                     ;

/* ------------------------------------------------------------------------ */

alloc_opt       :       T_INITIAL '=' T_NUMBER
    { ddlp_g.ft_entry.ft_initial = $3.num; }
                     |       T_NEXT '=' T_NUMBER
    { ddlp_g.ft_entry.ft_next = $3.num; }
                     |       T_PCTINCREASE '=' T_NUMBER
    { ddlp_g.ft_entry.ft_pctincrease = $3.num; }
                     |       T_PAGESIZE '=' T_NUMBER
    {
        if ($3.num < 128)
        {
            ddwarning(DB_TEXT("file page size must be at least 128"), $3.numline);
            ddlp_g.ft_entry.ft_pgsize = pagesize;
        }
        else if ($3.num > 32766)
        {
            ddwarning(DB_TEXT("maximum file page size is 32766"), $3.numline);
            ddlp_g.ft_entry.ft_pgsize = pagesize;
        }
        else
        {
            ddlp_g.ft_entry.ft_pgsize = (short) $3.num;
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

file_type       :       data_or_key T_FILE file_size
    { ddlp_g.ft_entry.ft_initial = ddlp_g.ft_entry.ft_next = ddlp_g.ft_entry.ft_pctincrease = 0; }
                     ;

/* ------------------------------------------------------------------------ */

data_or_key     :       T_DATA
    { ddlp_g.ft_entry.ft_type = 'd'; }
                     |       T_KEY
    { ddlp_g.ft_entry.ft_type = 'k'; }
                     ;

/* ------------------------------------------------------------------------ */

file_size       :
    {
        if (pagesize == 0 && ddlp_g.ft_entry.ft_type == 'k')
            ddlp_g.ft_entry.ft_pgsize = 1024;     /* THIS SHOULD CHANGE IF MAXKEYSIZE CHANGES */
        else
            ddlp_g.ft_entry.ft_pgsize = pagesize;
    }
                     |       '[' T_NUMBER ']' 
    {
        if ( $2.num < 128 )
        {
            ddwarning(DB_TEXT("file page size must be at least 128"), $2.numline);
            ddlp_g.ft_entry.ft_pgsize = pagesize;
        }
        else if ( $2.num > 32766 )
        {
            ddwarning(DB_TEXT("maximum file page size is 32766"), $2.numline);
            ddlp_g.ft_entry.ft_pgsize = pagesize;
        }
        else
        {
            ddlp_g.ft_entry.ft_pgsize = (short) $2.num;
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       '[' T_IDENT ']'
    {
        if (find_const($2.str, &tempnum))
        {
            if (tempnum < 128)
            {
                ddwarning(DB_TEXT("file page size must be at least 128"), $2.strline);
                ddlp_g.ft_entry.ft_pgsize = pagesize;
            }
            else if (tempnum > 32766)
            {
                ddwarning(DB_TEXT("maximum file page size is 32766"), $2.strline);
                ddlp_g.ft_entry.ft_pgsize = pagesize;
            }
            else
                ddlp_g.ft_entry.ft_pgsize = (short) tempnum;
        }
        else
            dderror(DB_TEXT("constant not defined"), $2.strline);

        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

file_id         :       T_IDENT
    {
        cpfile(ddlp_g.ft_entry.ft_name, $1.str, $1.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_STRING
    {
        cpfile(ddlp_g.ft_entry.ft_name, $1.str, $1.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

record_spec     :       rec_id field_defs key_defs closing_brace
                     |       rec_id field_defs closing_brace
                     |       rec_id closing_brace
                     |       rec_type error closing_brace
    {
        dderror(DB_TEXT("invalid record spec"), $1.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

rec_id          :       rec_type T_IDENT '{'
    {
        if (vtstrcmp($2.str, DB_TEXT("system")) == 0)
            ddlp_abort(DB_TEXT("'system' is a reserved record name"));
        vtstrcpy(rec_comp, $2.str);
        add_record($2.str, $2.strline);
        n_optkeys = 0;
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

rec_type        :       T_RECORD
    { ddlp_g.rt_entry.rt_flags = 0; }
                     |       T_STATIC T_RECORD
    { ddlp_g.rt_entry.rt_flags = STATIC; }
                     ;

/* ------------------------------------------------------------------------ */

field_defs      :       field_spec
                     |       field_defs field_spec
                     ;

/* ------------------------------------------------------------------------ */

field_spec      :       key_spec type_spec T_IDENT size_specs om_field ';'
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
                        dderror(ddlp_g.msg, $3.strline);
                        break;
                    }
                    tmp_dims[tot_dims] = ddlp_g.fd_entry.fd_dim[dim];
                    elts = elts * ddlp_g.fd_entry.fd_dim[dim];
                }
                memcpy(ddlp_g.fd_entry.fd_dim, tmp_dims, sizeof(tmp_dims));
                if (elts)
                    ddlp_g.fd_entry.fd_len *= (short) elts;
                add_field(rec_comp, $3.str, $3.strline, ci, ti, om);
            }
            else
            {
                add_field(rec_comp, NULL, $3.strline, ci, ti, om);
                add_struct_fields(rec_comp, $3.strline, ti->elem);
                if (!vtstrcmp(ti->type_name, DB_TEXT("_s_temp_")))
                {
                    del_type(DB_TEXT("_s_temp_"));
                    ti = NULL;
                }
                close_struct(rec_comp, $3.str, $3.strline);
            }
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       key_spec type_spec error
    {
        dderror(DB_TEXT("invalid field name"), ddlp_g.line);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       error ';'
    {
        dderror(DB_TEXT("invalid field spec"), $2.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

om_field        :       T_DIRECTREF T_IDENT
    {
        /* Handle DIRECTREF macro for om */
        if ((om = (OM_INFO *) psp_cGetMemory(sizeof(OM_INFO), 0)) == NULL)
            ddlp_abort(DB_TEXT("out of memory"));

        om->related = FALSE;
        if (ti->type_char != 'd')
            dderror(DB_TEXT("invalid field spec"), $1.numline); 
        else
        {
            om->dref = TRUE;
            vtstrcpy(om->dref_name, $2.str); 
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |        T_RELATEDTO T_IDENT T_THRU T_IDENT
    {
        /* Handle RELATEDTO macro for om */
        if ((om = (OM_INFO *) psp_cGetMemory(sizeof(OM_INFO), 0)) == NULL)
	    ddlp_abort(DB_TEXT("out of memory"));

        om->dref = FALSE;
        om->related = TRUE;
        vtstrcpy(om->rel_rec, $2.str);
        vtstrcpy(om->rel_field, vtstrupr($4.str));
    }
                     |
    {
        if ((om = (OM_INFO *)psp_cGetMemory(sizeof(OM_INFO), 0)) == NULL)
	    ddlp_abort(DB_TEXT("out of memory"));

        om->dref = FALSE;
        om->related = FALSE;
    }
                     ;

/* ------------------------------------------------------------------------ */
            
sign            :
    { sign_flag = 0; }
                     |       T_UNSIGNED
    { sign_flag = 1; }
                     ;

/* ------------------------------------------------------------------------ */

key_spec        :       opt_key T_UNIQUE T_KEY
    {
        if (!in_db)
            dderror(DB_TEXT("predefined structures cannot contain keys"), $3.numline);
        else
            ddlp_g.fd_entry.fd_key = 'u';
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       opt_key T_KEY
    {
        if (!in_db)
            dderror(DB_TEXT("predefined structures cannot contain keys"), $2.numline);
        else
            ddlp_g.fd_entry.fd_key = 'd';
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |
    {  ddlp_g.fd_entry.fd_flags = 0; ddlp_g.fd_entry.fd_key = 'n'; }
                     ;

/* ------------------------------------------------------------------------ */

opt_key         :       T_OPTIONAL
    {
        /* Optional key number will begin with 1 */
        ++n_optkeys;
        if ( n_optkeys > OPTKEY_LIMIT )
        {
            vstprintf(ddlp_g.msg,
                DB_TEXT("optional key per record limit (%d keys) exceeded"),
                OPTKEY_LIMIT);
            dderror(ddlp_g.msg, $1.numline);
        }
        else
            ddlp_g.fd_entry.fd_flags = (short) (n_optkeys << OPTKEYSHIFT);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |
    { ddlp_g.fd_entry.fd_flags = 0; }
                     ;

/* ------------------------------------------------------------------------ */

type_spec       :       sign T_IDENT
    { 
        if ((ti = find_type($2.str, sign_flag)) == NULL)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("invalid data type: %s"), $2.str);
            dderror(ddlp_g.msg, $2.strline);
        }
        if ( vtstrcmp($2.str, DB_TEXT("int")) == 0 )
        {
            vstprintf(ddlp_g.msg, DB_TEXT("non-portable data type: %s"), $2.str);
            ddwarning(ddlp_g.msg, $2.strline);
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       struct_spec
                     ;

/* ------------------------------------------------------------------------ */

size_specs      :       size_init size_decls
                     |       size_init
                     ;

/* ------------------------------------------------------------------------ */

size_init       :
    {
        /* clear array dimension info */
        for ( dim = 0; dim < MAXDIMS; ++dim )
        {
            ddlp_g.fd_entry.fd_dim[dim] = 0;
            ci[dim] = NULL;
        }
        dim = 0;
    }
                     ;

/* ------------------------------------------------------------------------ */

size_decls      :       size
                     |       size_decls size
                     ;

/* ------------------------------------------------------------------------ */

size            :       '[' T_NUMBER ']'
    {
        if ( dim == MAXDIMS )
        {
            vstprintf(ddlp_g.msg, DB_TEXT("too many array dimensions, maximum is %d"), MAXDIMS);
            dderror(ddlp_g.msg, $2.numline);
        }
        else
            ddlp_g.fd_entry.fd_dim[dim++] = (short) ($2.num);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       '[' T_IDENT ']'
    {
        if (find_const($2.str, &tempnum))
        {
            if (dim == MAXDIMS)
            {
                vstprintf(ddlp_g.msg, DB_TEXT("too many array dimensions, maximum is %d"), MAXDIMS);
                dderror(ddlp_g.msg, $2.strline);
            }
            else
            {
                ddlp_g.fd_entry.fd_dim[dim] = tempnum;
                ci[dim++] = find_const($2.str, &tempnum);
            }
        }
        else
            dderror(DB_TEXT("constant not defined"), $2.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

key_defs        :       key_field
                     |       key_defs key_field
                     ;

/* ------------------------------------------------------------------------ */

key_field       :       T_COMPOUND key_id '{' comfld_defs '}'
                     ;

/* ------------------------------------------------------------------------ */

key_id          :       key_spec T_IDENT
    { 
        /* clear array dimension info */
        for ( dim = 0; dim < MAXDIMS; ++dim )
            ddlp_g.fd_entry.fd_dim[dim] = 0, ci[dim] = NULL;
        dim = 0;
        ddlp_g.fd_entry.fd_type = 'k';
        add_field(rec_comp, $2.str, $2.strline, ci, NULL, NULL); 
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

comfld_defs     :       comfld_spec
                     |       comfld_defs comfld_spec
                     ;

/* ------------------------------------------------------------------------ */

comfld_spec     :       T_IDENT key_order ';'
    {
        add_key(rec_comp, $1.str, $1.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

key_order       :       T_ASCENDING
    { ddlp_g.kt_entry.kt_sort = 'a'; }
                     |       T_DESCENDING
    { ddlp_g.kt_entry.kt_sort = 'd'; }
                     |
    { ddlp_g.kt_entry.kt_sort = 'a'; }
                     ;

/* ------------------------------------------------------------------------ */

set_spec        :       T_BITMAP T_SET set_prefix members closing_brace
    {
        current_set->om_set_type = DB_BITMAP;
    }
                     |       T_BITMAP T_SET error closing_brace
    {
        dderror(DB_TEXT("invalid set spec"), $1.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_BLOB T_SET set_prefix members closing_brace
    {
        current_set->om_set_type = DB_BLOB;
    } 
                     |       T_BLOB T_SET error closing_brace
    {
        dderror(DB_TEXT("invalid set spec"), $1.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_VARILEN T_SET set_prefix members closing_brace
    {
        current_set->om_set_type = VLTEXT;
    }
                     |       T_VARILEN T_SET error closing_brace
    {
        dderror(DB_TEXT("invalid set spec"), $1.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_SET set_prefix members closing_brace
                     |       T_SET error closing_brace
    {
        dderror(DB_TEXT("invalid set spec"), $1.numline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

set_prefix      :       T_IDENT '{' ordering owner
    {
        last_ordering = ddlp_g.st_entry.st_order;
        current_set = add_set($1.str, $4.str, $1.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

ordering        :       T_ORDER order_type ';'
                     ;

/* ------------------------------------------------------------------------ */

order_type      :       T_FIRST
    { ddlp_g.st_entry.st_order = 'f'; }
                     |       T_LAST
    { ddlp_g.st_entry.st_order = 'l'; }
                     |       T_ASCENDING
    { ddlp_g.st_entry.st_order = 'a'; }
                     |       T_DESCENDING
    { ddlp_g.st_entry.st_order = 'd'; }
                     |       T_NEXT
    { ddlp_g.st_entry.st_order = 'n'; }
                     |       T_IDENT
    {
        vstprintf(ddlp_g.msg, DB_TEXT("invalid order type, '%s'"), $1.str);
        dderror(ddlp_g.msg, $1.strline);
        ddlp_g.st_entry.st_order = 'f';
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

owner           :       T_OWNER T_IDENT ';'
    { 
        vtstrcpy($$.str, $2.str); 
        add_xref(NULL, $2.str, 'r', $2.strline);
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;

/* ------------------------------------------------------------------------ */

members         :       member
    {
        memList = (MEM_INFO *) psp_cGetMemory(sizeof(MEM_INFO), 0);
        if (memList == NULL)
            ddlp_abort(DB_TEXT("out of memory"));

        vtstrcpy(memList->rec_name, $1.str);
        memList->next_mem = NULL;
        currmem = memList;
        if (ddlp_g.abort_flag)
            return(-1);
    } 
                     |       members member
    {
        currmem->next_mem = (MEM_INFO *) psp_cGetMemory(sizeof(MEM_INFO), 0);
        if ((currmem = currmem->next_mem) == NULL)
            ddlp_abort(DB_TEXT("out of memory"));

        vtstrcpy(currmem->rec_name, $2.str);
        currmem->next_mem = NULL;
        if (ddlp_g.abort_flag)
            return(-1);
    } 
                     ;

/* ------------------------------------------------------------------------ */

member          :       T_MEMBER T_IDENT ';'
    {
        if ( last_ordering == 'a' || last_ordering == 'd' )
            dderror(DB_TEXT("sort field(s) missing from member clause"), $1.numline);
        if ( vtstrcmp($2.str, DB_TEXT("system")) == 0 )
            dderror(DB_TEXT("'system' cannot be a member"), $2.strline);
        else 
        {
            add_member($2.str, NULL, $2.strline);
            vtstrcpy($$.str, $2.str); 
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     |       T_MEMBER T_IDENT T_BY id_list ';'
    {
        if ( last_ordering != 'a' && last_ordering != 'd' )
            dderror(DB_TEXT("sort field(s) not required in member clause"), $1.numline);
        if ( vtstrcmp($2.str, DB_TEXT("system")) == 0 )
            dderror(DB_TEXT("'system' cannot be a member"), $2.strline);
        else
        {
            add_member($2.str, id_list, $2.strline);
            vtstrcpy($$.str, $2.str); 
        }
        if (ddlp_g.abort_flag)
            return(-1);
    }
                     ;
%%

/* $Revision:   1.25.1.6  $ */

