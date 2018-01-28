/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
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
    IDA - Build List of Database Names
-----------------------------------------------------------------------*/
#include "db.star.h"
#include "ida.h"

/*************************** GLOBAL VARIABLES *****************************/

int    dbs_in_dir = 0;
char **dbnames = NULL;

/* =========================================================================
    Build list of database names
*/
void dblist()
{
    register int   i;
    char          *cp;
    char           dpath[50];
    PSP_DIR        dir;

    if (dbnames) /* free previously allocated memory */
    {
        for (i = 0; i < dbs_in_dir; ++i)
            psp_freeMemory(dbnames[i], 0);

        psp_freeMemory(dbnames, 0);
    }

    dbs_in_dir = 0;

    if ((i = vtstrlen(task->dbdpath)) > 0)
    {
        vtstrcpy(dpath, task->dbdpath);
        if (dpath[i - 1] == DIRCHAR)
            dpath[i - 1] = '\0';
    }
    else
        strcpy(dpath, ".");

    if ((dir = psp_pathOpen(dpath, "*.dbd")) == NULL)
    {
        usererr("unable to open directory");
        return;
    }

    /* count number of .dbd entries */
    while ((cp = psp_pathNext(dir)) != NULL)
    {
        psp_freeMemory(cp, 0);
        ++dbs_in_dir;
    }

    psp_pathClose(dir);

    if (dbs_in_dir == 0)
    {
        usererr("no databases found");
        return;
    }

    dbnames = (char **) psp_cGetMemory(dbs_in_dir * sizeof(char *), 0);

    dir = psp_pathOpen(dpath, "*.dbd");
    for (i = 0; i < dbs_in_dir; i++)
    {
        dbnames[i] = psp_pathNext(dir);
        if ((cp = strrchr(dbnames[i], '.')) != NULL)
	    *cp = '\0';
    }

    psp_pathClose(dir);
}


