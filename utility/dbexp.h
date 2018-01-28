/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbexp utility                                     *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

typedef struct _EXPOPTS
{
    DB_BOOLEAN  comma;
    DB_BOOLEAN  decimal;
    DB_BOOLEAN  extended;
    DB_BOOLEAN  silent;
    DB_BOOLEAN  unicode;
    DB_BOOLEAN  rec_addr;
    DB_BOOLEAN  mem_addr;
    DB_TCHAR    sep_char;
    DB_TCHAR    esc_char;
    DB_TCHAR   *dbname;
    DB_TCHAR   *names;
    DB_TCHAR  **recnames;
    int        *rt_index;
    int         n_recs;
    SG         *sg;
} EXPOPTS;

#define MAXEXPFILELEN 80

typedef struct _EXPFILE
{
    FILE       *fp;
    DB_BOOLEAN  open;
    DB_BOOLEAN  removed;
    int         recent;
    DB_TCHAR    name[MAXEXPFILELEN + 1];
} EXPFILE;

extern void cvt_dba(DB_TCHAR *, DB_ADDR, EXPOPTS *);
extern int  pr_mems(FILE *, short, char *, EXPOPTS *, DB_TASK *);
extern int  pr_data(FILE *, short, char *, EXPOPTS *, DB_TASK *);
extern void EXTERNAL_FCN dbexp_dberr(int, DB_TCHAR *);

