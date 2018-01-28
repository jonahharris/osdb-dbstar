/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, keypack utility                                   *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/


typedef struct
{
    DB_TCHAR backpath[FILENMLEN]; /* path to backup directory */
    DB_BOOLEAN backup;            /* TRUE if backup selected */
    DB_BOOLEAN keep;              /* TRUE when original db is to kept intact */
}  KEYPACK_OPTS;

typedef struct
{
    F_ADDR or_size;            /* size of original key file */
    F_ADDR pk_size;            /* size of packed key file */
    F_ADDR or_total;           /* total size of all original key files */
    F_ADDR pk_total;           /* total size of all packed key files */
}  KEYPACK_STATS;


extern int packfile(FILE_NO, DB_TCHAR *, int, KEYPACK_OPTS *, KEYPACK_STATS *, DB_TASK *);

