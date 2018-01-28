/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbedit utility                                    *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* General macro and structure definitions for DBEDIT */

/* ********************** Lengths & sizes **************************** */

#define TBUFFLEN 10           /* Max. no of token in command line */
#define LINELEN 128           /* Input line length */
#define NUMLEN 10             /* Max. number of digits in a number */
#define SCRWIDTH 80           /* Width of screen */
#define HEX_BYTES 12          /* Number of hex bytes to print on a line */
#define ASCII_BYTES 30        /* Number of ascii bytes to print per line */
#define BLOCKSIZE 1024        /* Size of buffer in edit hex blocks */

/* Edit hex block status values */

#define B_WRITE   1           /* Write block to disk on exit */
#define B_CHANGED 2           /* Do not re-use block */
#define B_EOF     4           /* Block contains end of file */

/* ********************** Structure Definitions ********************** */

typedef struct dbe_kwd
{                             /* Keyword */
    DB_TCHAR id[10];
    int min_len;
    int min_args;
    int max_args;
    unsigned char args[6];
}  DBE_KWD;

typedef struct dbe_tok
{                             /* Token */
    int type;
    int parent;
    int ival;
    DB_ULONG lval;
    DB_ADDR dbaval;
}  DBE_TOK;

typedef struct dbe_slot
{                             /* Current record IO buffer */
    short size;
    char *buffer;
    F_ADDR address;
}  DBE_SLOT;

typedef struct dbe_page
{                             /* Used in edix hex mode */
    F_ADDR start;
    F_ADDR end;
    PAGE_ENTRY *pg_ptr;
}  DBE_PAGE;

/* *************************** Token types *************************** */

#define TYPE_KWD 1            /* Token type keyword */
#define TYPE_DBA 2            /* Token type database address */
#define TYPE_NUM 3            /* Token type number */
#define TYPE_HEX 4            /* Hex number */
#define TYPE_SET 5            /* Set name */
#define TYPE_REC 6            /* Record name */
#define TYPE_FIL 7            /* File name */
#define TYPE_FLD 8            /* Field name */
#define TYPE_KEY 9            /* Key name */

/* ***************** dbedit mode non-keyword tokens ****************** */

#define RECNAME   1           /* These values must   */
#define SETNAME   2           /* match definition of */
#define FLDNAME   3           /* structure mainkwds  */
#define KEYNAME   4
#define FILENAME  5
#define ADDRESS   6
#define DECNUM    7
#define HEXNUM    8

/* *********************** dbedit mode keywords ********************** */

#define K_QMARK   9           /* These values must   */
#define K_BASE   10           /* match definition of */
#define K_COUNT  11           /* structure mainkwds  */
#define K_DBA    12           /* and must be in asc- */
#define K_DCHAIN 13           /* ending ascii order  */
#define K_DISP   14
#define K_EDIT   15
#define K_EXIT   16
#define K_FIELDS 17
#define K_FILE   18
#define K_FIRST  19
#define K_FLD    20
#define K_GOTO   21
#define K_HELP   22
#define K_HEX    23
#define K_KEY    24
#define K_LAST   25
#define K_MEM    26
#define K_NEXT   27
#define K_NEXTR  28
#define K_NEXTS  29
#define K_NOFLDS 30
#define K_NOTLS  31
#define K_OPT    32
#define K_OWN    33
#define K_PREV   34
#define K_PREVR  35
#define K_RECORD 36
#define K_REREAD 37
#define K_SET    38
#define K_SHOW   39
#define K_SOURCE 40
#define K_TITLES 41
#define K_TS     42
#define K_TYPE   43
#define K_VERIFY 44

/* ************************ hex mode keywords ************************ */

/* These values must match definition of structure hexkwds */

#define X_CHARF   0           /* Character forwards */
#define X_CHARB   1           /* Character backwards */
#define X_SRCHB   2           /* Search backwards */
#define X_LINEB   3           /* Line backwards */
#define X_GOTO    4           /* Goto position */
#define X_SRCHF   5           /* Search forwards */
#define X_LINEF   6           /* Line forwards */
#define X_QMARK   7           /* Help, question mark */
#define X_CANCEL  8           /* Cancel edits */
#define X_END     9           /* End hex mode */
#define X_HELP    10          /* Help */
#define X_PRINT   11          /* Print hex lines */
#define X_WRITE   12          /* Overwrite file contents with string */


