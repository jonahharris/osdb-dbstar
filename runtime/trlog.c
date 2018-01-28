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


/* Transaction logging functions:

   This module is intended to provide the hooks for implementing
   "after image" transaction logging which allows the re-creation
   of a (damaged) database from the last backup up to the time the
   database problem occurred.

   Transaction logging is initiated by setting the external variable
   "task->trlog_flag" to 1 (through the call to d_tron).  Once initiated,
   function d_trmark is called by d_trend in order to mark the
   beginning of a transaction update which will follow as a sequence
   of calls to d_trlog, one for each page being written to the database.
   Function d_trbound (called after all database pages have been
   successfully written to the database) is intended to delimit the
   end of a transaction on the logging device.

   The transaction logging device is normally a cartridge or nine
   track tape unit.  These functions must ensure that shared
   access to the logging device be protected against simultaneous use.

   The information which must minimally be stored is as follows:

        Transaction mark:

            - an arbitrary mark code
            - transaction id
            - time/date stamp

        Transaction information (one for each changed page):

            - transaction id
            - time/date stamp
            - file number
            - page number
            - page contents
            - page size

        Transaction boundary:

            - an arbitrary bound code
            - transaction id
            - time stamp
*/

/* ======================================================================
    Mark the start of transaction
*/
int INTERNAL_FCN dtrmark(DB_TASK *task)
{
    /* this should first secure exclusive access to the logging device and
     * then mark the start of the transaction. */
    return 0;
}

/* ======================================================================
    Bound the end of a transaction
*/
int INTERNAL_FCN dtrbound(DB_TASK *task)
{
    /* This should first bound the end of the transaction and then release */
    return 0;
}

/* ======================================================================
    Log a changed db page
*/
int INTERNAL_FCN dtrlog(
    int fno,                    /* file number */
    long pno,                   /* page number */
    const char *page,           /* page contents */
    int psz,                    /* page size */
    DB_TASK *task)
{
    return 0;
}



