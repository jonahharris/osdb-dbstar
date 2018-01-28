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

/* Database error handler
*/

void EXTERNAL_FCN dbimp_dberr(int errnum, DB_TCHAR *errmsg)
{
    if (errnum != S_ISOWNED)
    {
        if (errnum < S_OKAY)
        {
            vtprintf(DB_TEXT("\n*** Database error %d - %s\n"), errnum, errmsg);
            vtprintf(DB_TEXT("\terrno = %d\n"), errno);
        }
    }
}


