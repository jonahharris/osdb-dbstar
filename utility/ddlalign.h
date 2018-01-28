/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ddlp utility                                      *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/*------------------------------------------------------------------------ 

    db.* Database Definition Language Processor

    ddlalign.h

------------------------------------------------------------------------*/

#define BYTE_ALIGN  (-1)
#define MSC_ALIGN    0
#define BCC_ALIGN    1
#define ZOR_ALIGN    2
#define DEF_ALIGN    3

#define CHR_ALIGN    0 
#define SHT_ALIGN    1
#define REG_ALIGN    2
#define DBL_ALIGN    3
#define MAX_ALIGNS   4

extern unsigned short ddlp_g.str_pad[MAX_ALIGNS];
extern unsigned short ddlp_g.str_array_pad[MAX_ALIGNS];

short calc_align (int, unsigned char, unsigned short);
void     init_align (int);


