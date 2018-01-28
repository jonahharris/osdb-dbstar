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


FILE *dbf_open(DB_TCHAR *fname)
{
    return (vtfopen(fname, imp_g.unicode ? DB_TEXT("rb") : DB_TEXT("r")));
}


int dbf_read(FILE *fp, DB_TCHAR *line)
{
    DB_TINT c;

    if (fp == NULL)
        return (0);

    /* read up to next line terminator */
    while ((c = vgettc(fp)) != DB_TEXT('\n') && c != DB_TEOF)
        *line++ = (DB_TCHAR) c;
    *line = 0;
    if (c == DB_TEOF)
        return (0);
    return (1);

}


void dbf_close(FILE *fp)
{
    if (fp != NULL)
        fclose(fp);
}


