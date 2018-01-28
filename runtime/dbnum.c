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
    Find Database Number
*/
int INTERNAL_FCN ddbnum(
    const DB_TCHAR *dbn,                /* database name */
    DB_TASK *task)
{
    int       dbt_lc;                   /* loop control */
    DB_ENTRY *db_ptr;
    DB_TCHAR *ptr;
    DB_TCHAR  dbName[DBNMLEN],
              dbPath[FILENMLEN];

    /* break apart the path from the file name */
    if (vtstrlen(dbn) >= FILENMLEN)
        goto namelen;

    vtstrcpy(dbPath, dbn);
    ptr = psp_pathGetFile(dbPath);
    if (vtstrlen(ptr) >= DBNMLEN)
        goto namelen;

    vtstrcpy(dbName, ptr);
    *ptr = DB_TEXT('\0');

    if (vtstrlen(dbPath) >= DB_PATHLEN)
        goto namelen;

    for (dbt_lc = 0; dbt_lc < task->no_of_dbs; ++dbt_lc)
    {
        db_ptr = &task->db_table[dbt_lc];
        if (vtstrcmp(dbName, db_ptr->db_name) == 0 && (!dbPath[0] ||
                vtstrcmp(dbPath, db_ptr->db_path) == 0))
            return dbt_lc;
    }

    return (task->db_status = S_INVDB);

namelen:
    return (dberr(S_NAMELEN));
}



