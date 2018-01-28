%{
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
%}

%start dal_gram

%union {
     STRTOK tstr;
     NUMTOK tnum;
}

%token <tnum> T_WHILEOK T_EXIT T_PRINT T_INPUT T_ABORT 
%token <tnum> T_SCHEMA T_CURRENCY T_REWIND
%token <tstr> T_IDENT T_STRING

%token <tnum> '{' '}' ';' ',' '.' '(' ')'

%type <tstr> dal_gram param fld stmt stmts fcnstart iocom filespec end_mark

%%

dal_gram :   stmts end_mark
          | end_mark
          ;
stmts   :   stmt
    {
        int i;
        if (!batch )
        {
            for (i=0; i<loop_lvl; i++)
                vtprintf( DB_TEXT("   ") );
            vtprintf( DB_TEXT("d_") );
        }
    }
          | stmts stmt
    {
        int i;
        if (!batch )
        {
            for (i=0; i<loop_lvl; i++)
                vtprintf( DB_TEXT("   ") );
            vtprintf( DB_TEXT("d_") );
        }
    }
          ;
stmt    :   whileok {}
          |   fcn_call {}
          |   io {}
          |   rewind {}
          |   schema {}
          |   currency {}
          |   error ';'
    {
        dderror( DB_TEXT("invalid statement"), $2.numline );
    }
          ;
whileok :   whilestart '{' stmts '}'
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
          |   whilestart '{' error '}'
    {
        yyerrok;
        dderror( DB_TEXT("While loop aborted"), $4.numline );
        loop_lvl--;
        freeloop( &loopstack[loop_lvl] );
    }
          ;
whilestart  :    T_WHILEOK
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
          ;
fcn_call    :   fcnstart '(' params ')' ';'
    {
        if (loop_lvl ==0 )
        {
            dalexec( curinst );
            freeinst( &curinst );
        }
    }
          |   fcnstart '(' params ')' error ';'
    {
        yyerrok;
        dderror( DB_TEXT("Characters following closing paren"), $6.numline );
    }
          |       fcnstart '(' error ';'
    {
        yyerrok;
        dderror( DB_TEXT("Missing closing paren"), $4.numline );
    }
          |       fcnstart error ';'
    {
        yyerrok;
        dderror( DB_TEXT("Invalid function call"), $3.numline );
    }
          ;
fcnstart    :   T_IDENT
    {
        newinst( &curinst );
        if ( curinst == NULL )
            return(1);
        vtstrncpy( curinst->i_name, $1.str, 9 );
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
          ;
params      :   /* Empty */
          |   param
    {
        vtstrncpy( curinst->i_p1, $1.str, NAMELEN );
        vtstrncpy( curinst->i_f1, fld[0], NAMELEN );
    }
          |   param ',' param
    {
        vtstrncpy( curinst->i_p1, $1.str, NAMELEN );
        vtstrncpy( curinst->i_f1, fld[0], NAMELEN );
        vtstrncpy( curinst->i_p2, $3.str, NAMELEN );
        vtstrncpy( curinst->i_f2, fld[1], NAMELEN );
    }
          |   param ',' param ',' param
    {
        vtstrncpy( curinst->i_p1, $1.str, NAMELEN );
        vtstrncpy( curinst->i_f1, fld[0], NAMELEN );
        vtstrncpy( curinst->i_p2, $3.str, NAMELEN );
        vtstrncpy( curinst->i_f2, fld[1], NAMELEN );
        vtstrncpy( curinst->i_p3, $5.str, NAMELEN );
        vtstrncpy( curinst->i_f3, fld[2], NAMELEN );
    }
          |   param ',' param ',' param ','
    {
        yyerrok;
        dderror(DB_TEXT("Too many parameters"), $6.numline);
    }
          ;
param   :   T_IDENT
    {
        vtstrcpy( $$.str, $1.str );
        nparams++;
    }
          |   T_IDENT '.' T_IDENT
    {
        vtstrcpy( fld[nparams], $3.str );
        nparams++;
    }
          |   T_STRING
    {
        vtstrcpy( $$.str, genlit( $1.str ) );
        fld[nparams][0] = 0;
        nparams++;
    }
          ;
io      :   iostart flds ';'
    {
        if ( loop_lvl == 0)
        {
            dalexec( curinst );
            freeinst( &curinst );
        }
    }
          |   iostart '(' filespec ')' flds ';'
    {
        vtstrcpy( curinst->i_p1, $3.str );
        if ( loop_lvl == 0 )
        {
            dalexec( curinst );
            freeinst( &curinst );
        }
    }
          ;
filespec    :   T_STRING
          |   T_IDENT
          ;
iostart :   iocom
    {
        newinst( &curinst );
        if ( curinst == NULL )
            return(1);
        vtstrcpy( curinst->i_name, $1.str );
        if ( loop_lvl )
        {
            if ( previnst == NULL )
                loopstack[loop_lvl-1]->i_loop = curinst;
            else
                previnst->i_next = curinst;
            previnst = curinst;
        }
    }
          ;
iocom   :   T_PRINT
    {
        vtstrcpy( $$.str, DB_TEXT("print") );
    }
          |   T_INPUT
    {
        vtstrcpy( $$.str, DB_TEXT("input") );
    }
          ;
flds    :   fld
    {
        newprint( &curprint );
        if ( curprint == NULL )
            return(1);
        curinst->i_pfld = curprint;
        vtstrcpy( curprint->pf_rec, prflds.pf_rec );
        vtstrcpy( curprint->pf_fld, prflds.pf_fld );
    }
          |   flds ',' fld
    {
        newprint( &(curprint->pf_next) );
        if ( curprint == NULL )
            return(1);
        curprint = curprint->pf_next;
        vtstrcpy( curprint->pf_rec, prflds.pf_rec );
        vtstrcpy( curprint->pf_fld, prflds.pf_fld );
    }
          ;
fld     :   T_IDENT
    {
        vtstrcpy( prflds.pf_rec, $1.str );
        prflds.pf_fld[0] = 0;
    }
          |   T_IDENT '.' T_IDENT
    {
        vtstrcpy( prflds.pf_rec, $1.str );
        vtstrcpy( prflds.pf_fld, $3.str );
    }
          |   T_STRING
    {
        vtstrcpy( prflds.pf_rec, genlit( $1.str ) );
        prflds.pf_fld[0] = 0;
    }
          ;
rewind      :   T_REWIND filespec ';'
    {
        newinst( &curinst );
        if ( curinst == NULL )
            return(1);
        vtstrcpy( curinst->i_p1, $2.str );
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
          ;
schema  :   T_SCHEMA ';'
    {
        dalschem( DB_TEXT("") );
    }
          |   T_SCHEMA T_IDENT ';'
    {
        dalschem( $2.str );
    }
          |   T_SCHEMA error ';'
    {
        yyerrok;
        dderror( DB_TEXT(" Invalid Parameter (s)"), $3.numline );
    }
          ;
currency        :       T_CURRENCY ';'
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
                     ;
end_mark    :   T_EXIT  ';'
    {
        dalio_close();
        d_close(DalDbTask);
        return(0);
    }
          |   T_ABORT ';'
    {
        dalio_close();
        return(1);
    }
          ;
%%

