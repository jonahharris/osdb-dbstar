/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database kernel                                             *
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


/* ======================================================================
    Map ASCII-Characters for output and sorting
*/
int INTERNAL_FCN dmapchar(
    unsigned char inchar,        /* value of character to be mapped */
    unsigned char outchar,       /* output character as    ... */
    const char   *sort_str,      /* sort string (max. len = 2) */
    unsigned char subsort,       /* subsort value, to distinguish
                                  * between two equal values
                                  * (e.g. 'a' and 'A', if necessary) */
    DB_TASK *task )
{
    int indx;
    
    if (strlen(sort_str) > 2)
        return (dberr(S_INVSORT));

    /* Is character-set table already installed? */
    if (!task->ctbl_activ)
    {
        if (ctbl_alloc(task) != S_OKAY)
            return (task->db_status);

        task->ctbl_activ = TRUE;
    }

    /* Modify table for inchar specifications */
    indx = inchar;

    task->country_tbl[indx].out_chr = outchar;
    task->country_tbl[indx].sort_as1 = sort_str[0];
    task->country_tbl[indx].sort_as2 = sort_str[1];
    task->country_tbl[indx].sub_sort = subsort;

    return (task->db_status);
}

/* ======================================================================
    read MAP_FILE and make appropriate d_mapchar-calls
*/
int INTERNAL_FCN ctb_init(DB_TASK *task)
{
    PSP_FH        *ctb;
    unsigned char  inchar;
    unsigned char  outchar;
    unsigned char  subsort;

    char     p[80];
    char    *ptr;
    char     sortas[3];
    int      subs_i;
    DB_TCHAR ctb_name[FILENMLEN * 2];

    vtstrcpy(ctb_name, task->ctbpath);
    vtstrcat(ctb_name, CTBNAME);

    ctb = psp_fileOpen(ctb_name, O_RDONLY, PSP_FLAG_DENYNO | PSP_FLAG_STREAM);
    if (ctb != NULL)
    {
        while (psp_fileGets(ctb, p, sizeof(p)) != NULL)
        {
            ptr = p;
            if (stricmp(ptr, "ignorecase") == 0)
            {
                if (ctbl_ignorecase(task) != S_OKAY)
                    break;
            }
            else
            {
                inchar = ptr[0];
                outchar = ptr[2];
                subs_i = ptr[4] - '0';
                ptr = strchr(&ptr[4], ',');
                sortas[0] = (char)(ptr ? *(ptr + 1) : '\0');
                sortas[1] = (char)(ptr ? *(ptr + 2) : '\0');
                sortas[2] = '\0';
                subsort = (unsigned char) subs_i;

                if (dmapchar(inchar, outchar, sortas, subsort, task) != S_OKAY)
                    break;
            }
        }

        psp_fileClose(ctb);
    }

    return task->db_status;
}

/* ======================================================================
    Ignore case in string comparisons by mapping lower case to upper case
*/
int INTERNAL_FCN ctbl_ignorecase(DB_TASK *task)
{
    int i;
    
    if (!task->ctbl_activ)
    {
        if (ctbl_alloc(task) != S_OKAY)
            return task->db_status;

        task->ctbl_activ = TRUE;
    }

    for (i = 'a'; i <= 'z'; i++)
        task->country_tbl[i].sort_as1 = (char) (i - (int)'a' + (int)'A');

    task->dboptions |= IGNORECASE;
    return S_OKAY;
}

/* ======================================================================
    Use case in string comparisons
*/
int INTERNAL_FCN ctbl_usecase(DB_TASK *task)
{
    int i;
    
    if (task->ctbl_activ)
    {
        for (i = 'a'; i <= 'z'; i++)
            task->country_tbl[i].sort_as1 = (char) i;
    }

    task->dboptions &= ~IGNORECASE;
    return S_OKAY;
}

/* ======================================================================
    Allocate and initialize country_table
*/
int INTERNAL_FCN ctbl_alloc(DB_TASK *task)
{
    task->country_tbl = (CNTRY_TBL *) psp_cGetMemory(256 * sizeof(CNTRY_TBL) + 1, 0);
    if (task->country_tbl == NULL)
        dberr(S_NOMEMORY);

    return (task->db_status);
}

/* ======================================================================
    Free country table
*/
void EXTERNAL_FCN ctbl_free(DB_TASK *task)
{
    psp_freeMemory(task->country_tbl, 0);
    task->ctbl_activ = FALSE;
}


