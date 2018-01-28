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

/* db.* dal variable declarations */

extern struct fcnlist fcns[];

extern int nfcns, batch;

extern int (*(pgen[])) ();

extern INST *curinst, *previnst;
extern INST *loopstack[20];

extern int loop_lvl, nparam;

extern PRINTFIELD *curprint;

extern DB_TASK *DalDbTask;

extern SG *dal_sg;


