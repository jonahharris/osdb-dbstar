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

#ifdef DAL
#define getrec    dal_getrec
#define getset    dal_getset
#define getfld    dal_getfld
#define rec_const dal_rec_const
#define set_const dal_set_const
#define fld_const dal_fld_const
#endif

#ifdef DBIMP
#define getrec    dbimp_getrec
#define getset    dbimp_getset
#define getfld    dbimp_getfld
#define rec_const dbimp_rec_const
#define set_const dbimp_set_const
#define fld_const dbimp_fld_const
#endif

#endif /* VXWORKS */

extern int  getrec(DB_TCHAR *, DB_TASK *);
extern int  getset(DB_TCHAR *, DB_TASK *);
extern long getfld(DB_TCHAR *, int, DB_TASK *);
extern int  rec_const(int);
extern int  set_const(int);
extern long fld_const(int, DB_TASK *);


