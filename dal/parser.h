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

#define NAMELEN 32
typedef struct id_info
{
    struct id_info *next_id;
    DB_TCHAR id_name[NAMELEN];
}  ID_INFO;

typedef struct strtok
{
    DB_TCHAR str[80];
    int strline;
}  STRTOK;

typedef struct numtok
{
    int num;
    int numline;
}  NUMTOK;

#include <ctype.h>

#ifndef MAX_IDENT
#define MAX_IDENT       80
#endif


