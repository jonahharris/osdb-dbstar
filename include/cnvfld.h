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

#ifdef VXWORKS
/*
    For VxWorks, redefine function names to avoid naming conflicts between
    db.* utilities, in case they are loaded into memory at the same time
*/

#ifdef DATDUMP
#define fldtotxt      datdump_fldtotxt
#endif

#ifdef DBCHECK
#define fldtotxt      dbcheck_fldtotxt
#endif

#ifdef DBEDIT
#define fldtotxt      dbedit_fldtotxt
#endif

#ifdef KEYDUMP
#define fldtotxt      keydump_fldtotxt
#endif

#endif /* VXWORKS */

extern DB_TCHAR *fldtotxt(int, char *, DB_TCHAR *, int, DB_TASK *);


