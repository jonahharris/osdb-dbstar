/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* maximum size of data field edit buffer */
#define MAXSZ 1600

/* Generic keyboard special function key definitions */
#define K_HOME   '\002'
#define K_END    '\003'
#define K_UP     '\004'
#define K_DOWN   '\005'
#define K_LEFT   '\006'
#define K_RIGHT  '\007'
#define K_PGUP   '\013'
#define K_PGDN   '\014'
#define K_ESC    '\033'
#define K_CANCEL '\030'
#define K_BREAK  '\032'
#define K_FTAB   '\011'
#define K_BTAB   '\036'
#define K_INS    '\001'
#define K_DEL    '\177'
#define K_F1     '\016'
#define K_F2     '\017'
#define K_F3     '\020'
#define K_F4     '\021'
#define K_F5     '\022'
#define K_F6     '\023'
#define K_F7     '\024'
#define K_F8     '\025'
#define K_F9     '\026'
#define K_F10    '\027'

/*------------------------------------------------------------------------

        constant  termcap  key
        -------   -------  ---
        K_HOME    kh       ^B
        K_END     --       ^C
        K_UP      ku       ^D
        K_DOWN    kd       ^E
        K_LEFT    kl       ^F
        K_RIGHT   kr       ^G
        K_PGUP    --       ^K
        K_PGDN    --       ^L
        K_ESC     --       ESC (^[)
        K_CANCEL  --       ^X
        K_BREAK   --       ^Z
        K_FTAB    --       ^I
        K_BTAB    --       ^^
        K_INS     --       ^A
        K_DEL     --       DEL
        K_F1      k0       ^N
        K_F2      k1       ^O
        K_F3      k2       ^P
        K_F4      k3       ^Q
        K_F5      k4       ^R
        K_F6      k5       ^S
        K_F7      k6       ^T
        K_F8      k7       ^U
        K_F9      k8       ^V
        K_F10     k9       ^W
------------------------------------------------------------------------*/


