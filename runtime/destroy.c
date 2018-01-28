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
    Database remove function
*/

/* WARNING: this function will destroy the entire contents
   of the database
*/
int INTERNAL_FCN ddestroy(const DB_TCHAR *dbname, DB_TASK *task)
{
    int db_nbr;
    FILE_NO fno, lastfno;                 /* loop control */

    /* by not closing the databases until the end of the fcn,
       d_renfile() remains in effect as it should.
    */

    if (!task->dbopen)
        return (dberr(S_DBOPEN));

    db_nbr = ddbnum(dbname, task );
    if (db_nbr < 0)
        return (dberr(S_INVDB));

    dsetdb(db_nbr, task);

    /* make sure we have the necessary locks:
       one-user/exclusive mode, or shared mode with exclusive locks
    */
    lastfno = (FILE_NO) (task->curr_db_table->ft_offset + DB_REF(Size_ft));
    if (task->dbopen == 1)     /* shared mode */
    {
        for (fno = (FILE_NO) (lastfno - DB_REF(Size_ft)); fno < lastfno; ++fno)
        {
            if (!task->excl_locks[fno])
                return (dberr(S_EXCLUSIVE));
        }
    }

    /* clear db files from the cache */
    dio_clear(db_nbr, task);

    /* remove db files in task->file_table */
    for (fno = (FILE_NO) (lastfno - DB_REF(Size_ft)); fno < lastfno; fno++)
    {
        dio_close((FILE_NO) fno, task);
        psp_fileRemove(task->file_table[fno].ft_name);
    }

    diclose(task, ALL_DBS);

    return (task->db_status);
}


