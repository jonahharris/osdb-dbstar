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
#include "version.h"

/* ======================================================================
    Returns a version string based on the specified format:

        %n  = Library name
        %v  = Version as "version.revision.build"
        %b  = Build number as "[Build #]"
        %V  = Full version as "%n %v %b"

        %c  = Copyright date
        %w  = Copyright by whom
        %r  = Rights
        %C  = Full copyright notice 
*/
int INTERNAL_FCN ddbver(DB_TCHAR *fmt, DB_TCHAR *buf, int buflen)
{
    DB_STRING ver;

    static DB_TCHAR *dbstar_desc  = DB_TEXT("db.*");
    static DB_TCHAR *dbstar_ver   = DBSTAR_VERSION DB_TEXT(" ") DBSTAR_UNICODE DB_TEXT(" ") DBSTAR_VER_DATE;
    static DB_TCHAR *dbstar_build = DB_TEXT("[build ") DBSTAR_BLDNO DB_TEXT("]");

    static DB_TCHAR *dbstar_copyright = DBSTAR_COPYRIGHT;
    static DB_TCHAR *dbstar_when  = DBSTAR_COPYRIGHT_DATE;
    static DB_TCHAR *dbstar_who   = DBSTAR_COPYRIGHT_OWNER;
    static DB_TCHAR *dbstar_arr   = DB_TEXT("All Rights Reserved.");

    buf[0] = DB_TEXT('\0');
    STRinit(&ver, buf, buflen);

    while (*fmt && STRavail(&ver))
    {
        if (*fmt == DB_TEXT('%'))
        {
            ++fmt;
            switch (*fmt)
            {
                case DB_TEXT('n'):
                    STRcat(&ver, dbstar_desc);
                    break;

                case DB_TEXT('v'):
                    STRcat(&ver, dbstar_ver);
                    break;

                case DB_TEXT('b'):
                    STRcat(&ver, dbstar_build);
                    break;

                case DB_TEXT('V'):
                    STRcat(&ver, dbstar_desc);
                    STRcat(&ver, DB_TEXT(" "));
                    STRcat(&ver, dbstar_ver);
                    break;

                case DB_TEXT('c'):
                    STRcat(&ver, dbstar_when);
                    break;

                case DB_TEXT('w'):
                    STRcat(&ver, dbstar_who);
                    break;

                case DB_TEXT('r'):
                    STRcat(&ver, dbstar_arr);
                    break;

                case DB_TEXT('C'):
                    STRcat(&ver, dbstar_copyright);
                    break;

                default:
                    STRccat(&ver, *fmt);
                    break;
            }
        }
        else
            STRccat(&ver, *fmt);

        ++fmt;
    }

    return S_OKAY;
}


