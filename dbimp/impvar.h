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

/*
    Put all global variables in one structure, to reduce naming conflicts
    on VxWorks.
*/

typedef struct _IMP_G
{
    DB_TCHAR dbname[FILELEN];    /* the db.* database name */
    DB_TCHAR specname[81];       /* import spec file name */
    DB_TCHAR sep_char;           /* separation character */
    DB_TCHAR esc_char;           /* escape character */
    DB_TCHAR *keyfile;

    int slsize;
    int keylen;
    int keyfld;

    int silent;
    int unicode;
    int tot_errs;                /* total errors encountered */
    int tot_warnings;
    int abort_flag;
    int loop_lvl;                /* top of loop stack */
    int rf;
    int recndx;

    int *rbs;
    int *rbe;
    int *bs;
    int *rmt;

    struct spec     *curloop[20];    /* loop stack */
    struct spec     *curspec;
    struct spec     *specstart;
    struct rec      *currec;
    struct fld      *fldlist;
    struct fld      *newfld;
    struct handling *hlist;

    DB_ADDR *currecs;  /* indices into 'refs' of the current records (per type) */

    DB_TASK *dbtask;   /* must be global, for yacc generated function imp() */

    FILE *fspec;
    SG   *sg;

} IMP_G;


extern IMP_G imp_g;


