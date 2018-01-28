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

#ifndef VERSION_H
#define VERSION_H

#define DBSTAR_UNICODE   DB_TEXT("Ansi")

#define DBSTAR_VERSION   DB_TEXT("1.0.0")
#define DBSTAR_MAJOR_VER DB_TEXT("1.0")
#define DBSTAR_BLDNO     DB_TEXT("0")
#define DBSTAR_VER_DATE  DB_TEXT("[29-Feb-2000]")
#define DBSTAR_LABEL     DB_TEXT("db.* ") DBSTAR_VERSION DB_TEXT(" ") DBSTAR_VER_DATE

#define DBSTAR_COPYRIGHT_DATE  DB_TEXT("Copyright (c) 2000")
#define DBSTAR_COPYRIGHT_OWNER DB_TEXT("Centura Software Corporation")
#define DBSTAR_COPYRIGHT       DBSTAR_COPYRIGHT_DATE DB_TEXT(" ") DBSTAR_COPYRIGHT_OWNER DB_TEXT(".  All Rights Reserved.\n\n")

#define DBSTAR_LM_DESC   DB_TEXT("\nLock Manager - ") DBSTAR_LABEL DB_TEXT("\n") DBSTAR_COPYRIGHT
#define DBSTAR_UTIL_DESC(LABEL) DB_TEXT("\n") LABEL DB_TEXT(" Utility - ") DBSTAR_LABEL DB_TEXT("\n") DBSTAR_COPYRIGHT

#endif /* VERSION_H */

