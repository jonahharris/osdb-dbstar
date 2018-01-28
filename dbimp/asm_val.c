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
#include "impvar.h"



DB_TCHAR *asm_val(struct handling *hptr)
{
    DB_TCHAR *fld_cont;
    int i, width;
    struct loop *l = NULL;

    for (i = 0; i < imp_g.loop_lvl; i++)
    {
        if (vtstrcmp(hptr->h_file, imp_g.curloop[i]->u.sp_looptr->l_fname) == 0)
        {
            l = imp_g.curloop[i]->u.sp_looptr;
            break;
        }
    }
    if (i == imp_g.loop_lvl)
    {
        vftprintf(stderr, DB_TEXT("File %s is not open\n"), hptr->h_file);
        dbimp_abort(DB_TEXT("Execution terminated"));
        return (NULL);
    }
    /* search for the field's starting position in this line */
    fld_cont = find_fld(hptr->h_name, l->l_line, &width);

    return (fld_cont);
}


