%{
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
%}

%start import_spec

%union {
    STRTOK tstr; 
    NUMTOK tnum;
}

%token <tnum> T_DATABASE T_FOR T_ON T_FIELD T_CONNECT T_END T_RECORD
%token <tnum> T_CREATE T_UPDATE T_FIND T_NUMBER
%token <tstr> T_IDENT T_STRING

%token <tnum> '{' '}' ';' ',' '=' '.'

%type <tstr> rec_id field_def fields
%type <tnum> handling ss subscript for_id open_mark update_def create_def
%type <tnum> find_def

%%

import_spec     :       db_spec for_loops close_mark
                     ;
db_spec         :       T_DATABASE T_IDENT open_mark
    {
        int i;

        /* open up the database and read in the ddl tables */
        vtstrcpy( imp_g.dbname, $2.str );
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
                     ;
open_mark       :       '{'
                     |       ';'
                     ;
for_loops       :       for_loop
                     |       for_loops for_loop
                     ;
for_loop        :       for_id stmts '}'
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
                     |       for_id error '}'
    {
        dderror( DB_TEXT("error in 'foreach' statment"), $1.numline );
    }
                     ;
for_id          :       T_FOR T_STRING '{'
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
        imp_g.curspec->u.sp_looptr = new_loop( $2.str );
        if (imp_g.curspec->u.sp_looptr == NULL)
            return 1;
        imp_g.curloop[imp_g.loop_lvl] = imp_g.curspec;
        imp_g.loop_lvl++;
    }
                     ;
stmts           :       stmt
                     |       stmts stmt
                     ;
stmt            :       for_loop
                     |       record_spec
                     |       connect_spec
                     ;
record_spec     :       rec_id handling field_defs '}'
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
        imp_g.curspec->u.sp_recptr = new_rec( $2.num, imp_g.hlist, imp_g.fldlist );
        if (imp_g.curspec->u.sp_recptr == NULL)
            return 1;
        imp_g.curspec->u.sp_recptr->rec_ndx = imp_g.recndx;
        if ( imp_g.curspec->u.sp_recptr->rec_ndx < 0 )
        {
            vftprintf(stderr, 
                    DB_TEXT("**WARNING**  record name '%s' not found in schema\n"),
                    $1.str);
            imp_g.tot_warnings++;
        }

        /* initialize the field and handling list headers */
        imp_g.fldlist = NULL;
        imp_g.hlist = NULL;
    }
                     |       T_RECORD error '}'
    {
        dderror( DB_TEXT("invalid record spec"), $1.numline );
    }
                     ;
rec_id          :       T_RECORD T_IDENT '{'
    {
        /* save the record type name */
        vtstrcpy($$.str, $2.str);
        imp_g.recndx = getrec($2.str, imp_g.dbtask);
    }
                     |       T_RECORD error '{'
    {
        dderror( DB_TEXT("invalid record spec"), $1.numline );
    }
                     ;
handling        :       
    { $$.num = DBIMP_AUTO; }
                     |       update_def
    { $$.num = DBIMP_UPDATE; }
                     |       create_def
    { $$.num = DBIMP_CREATE; }
                     |       find_def
    { $$.num = DBIMP_FIND; }
                     ;
update_def      :       T_UPDATE T_ON key_spec ';'
                     ;
create_def      :       T_CREATE T_ON key_spec ';'
                     ;
find_def        :       T_FIND T_ON key_spec ';'
                     ;
key_spec        :       T_NUMBER
    {
        /* the handling list is only one element now */
        imp_g.hlist = new_hand( imp_g.curloop[imp_g.loop_lvl-1]->u.sp_looptr->l_fname, $1.num );
        if (imp_g.hlist == NULL)
            return 1;
    }
                 ;
field_defs      :       field_init
                |       field_init fields
                ;
field_init      :
    {
        imp_g.fldlist = NULL;
    }
                ;
fields          :       field_def
                |       field_def fields
                ;
field_def       :       T_FIELD field_spec '=' line_spec ';'
    {
        imp_g.newfld->fld_next = imp_g.fldlist;
        imp_g.fldlist = imp_g.newfld;
    }
                |       T_FIELD error ';'
    {
        dderror( DB_TEXT("bad field specification"), $1.numline );
    }
                ;
field_spec      :       fsinit T_IDENT subscripts
    {
        /* create the field structure, with zero offset */
        imp_g.newfld = new_fld( DB_TEXT(""), NULL, 0, $2.str, dim[0], dims[0] );
        if (imp_g.newfld == NULL)
            return 1;
    }
                     |       fsinit T_IDENT subscripts '.' T_IDENT subscripts
    {
        /* create the field structure, with offset for subscript */
        imp_g.newfld = new_fld( $2.str, dim[0], dims[0], $5.str, dim[1], dims[1] );
        if (imp_g.newfld == NULL)
            return 1;
    }
                     ;
fsinit          :
    {
        fsinit = -1; 
    }
                     ;
subscripts      :       subinit
                |       subinit ss
                ;
subinit         :
    {
        int i;
        fsinit++;
        dims[fsinit] = 0;
        for ( i=0; i<MAXDIMS; i++ ) dim[fsinit][i] = 0;
    }
                ;
ss              :       subscript
                |       ss subscript
                ;
subscript       :       '[' T_NUMBER ']'
    {
        if ( dims[fsinit] >= MAXDIMS )
            dderror( DB_TEXT("too many dimensions"), $2.numline );
        else
            dim[fsinit][dims[fsinit]++] = $2.num;
    }
                ;
line_spec       :       T_NUMBER
    {
        /* assume that this number refers to the current file */
        vtstrcpy( imp_g.newfld->fld_file, imp_g.curloop[imp_g.loop_lvl-1]->u.sp_looptr->l_fname );
        imp_g.newfld->fld_name = $1.num;
    }
                     |       T_STRING '.' T_NUMBER
    {
        /* save the explicit file name and number */
        vtstrcpy( imp_g.newfld->fld_file, $1.str );
        imp_g.newfld->fld_name = $3.num;
    }
                     ;
connect_spec    :       T_CONNECT T_IDENT ';'
    {
        /* save the connect specification on the list */
        imp_g.curspec->sp_next = new_spec( CONNECT );
        if (imp_g.curspec->sp_next == NULL)
            return 1;
        imp_g.curspec = imp_g.curspec->sp_next;
        imp_g.curspec->u.sp_conptr = new_con();
        if (imp_g.curspec->u.sp_conptr == NULL)
            return 1;
        imp_g.curspec->u.sp_conptr->con_ndx = getset( $2.str, imp_g.dbtask );
        if ( imp_g.curspec->u.sp_conptr->con_ndx < 0 )
        {
            vftprintf(stderr,
                         DB_TEXT("**WARNING**  set '%s' not found in schema\n"),
                         $2.str);
            imp_g.tot_warnings++;
        }
    }
                     |       T_CONNECT error ';'
    {
        dderror( DB_TEXT("invalid connect spec"), $1.numline );
    }
                     ;
close_mark      :       '}'
    {
        d_close(imp_g.dbtask);
    }
                     |        T_END ';'
    {
        d_close(imp_g.dbtask);
    }
                     ;
%%

