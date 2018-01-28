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

#ifndef APIDEFS_H
#define APIDEFS_H

/* NOTE: This table must contain a contiguous enumeration of the function
         names.  Any changes must also be reflected in apinames.h.
*/

/*
    d_ API
*/
#define D_CHECKID              100
#define D_CLOSE                101
#define D_CLOSEALL             102
#define D_CMSTAT               103
#define D_CMTYPE               104
#define D_CONNECT              105
#define D_COSTAT               106
#define D_COTYPE               107
#define D_CRGET                108
#define D_CRREAD               109
#define D_CRSET                110
#define D_CRSTAT               111
#define D_CRTYPE               112
#define D_CRWRITE              113
#define D_CSMGET               114
#define D_CSMREAD              115
#define D_CSMSET               116
#define D_CSMWRITE             117
#define D_CSOGET               118
#define D_CSOREAD              119
#define D_CSOSET               120
#define D_CSOWRITE             121
#define D_CSSTAT               122
#define D_CTBPATH              123
#define D_CTSCM                124
#define D_CTSCO                125
#define D_CTSCR                126
#define D_CURKEY               127
#define D_DBDPATH              128
#define D_DBERR                129
#define D_DBFPATH              130
#define D_DBLOG                131
#define D_DBNUM                132
#define D_DBSTAT               133
#define D_DBTAF                134
#define D_DBTMP                135
#define D_DBUSERID             136
#define D_DEF_OPT              137
#define D_DELETE               138
#define D_DESTROY              139
#define D_DISCON               140
#define D_DISDEL               141
#define D_FILLNEW              142
#define D_FINDCO               143
#define D_FINDFM               144
#define D_FINDLM               145
#define D_FINDNM               146
#define D_FINDPM               147
#define D_FLDNUM               148
#define D_FREEALL              149
#define D_GTSCM                150
#define D_GTSCO                151
#define D_GTSCR                152
#define D_GTSCS                153
#define D_ICLOSE               154
#define D_INITFILE             155
#define D_INITIALIZE           156
#define D_INTERNALS            157
#define D_IOPEN                158
#define D_ISMEMBER             159
#define D_ISOWNER              160
#define D_KEYBUILD             161
#define D_KEYDEL               162
#define D_KEYEXIST             163
#define D_KEYFIND              164
#define D_KEYFREE              165
#define D_KEYFRST              166
#define D_KEYLAST              167
#define D_KEYLOCK              168
#define D_KEYLSTAT             169
#define D_KEYNEXT              170
#define D_KEYPREV              171
#define D_KEYREAD              172
#define D_KEYSTORE             173
#define D_LMCLEAR              174
#define D_LMSTAT               175
#define D_LOCK                 176
#define D_LOCKCOMM             177
#define D_LOCKMGR              178
#define D_LOCKTIMEOUT          179
#define D_MAKENEW              180
#define D_MAPCHAR              181
#define D_MEMBERS              182
#define D_OFF_OPT              183
#define D_ON_OPT               184
#define D_OPEN                 185
#define D_RDCURR               186
#define D_DBINI                187
#define D_RECFREE              188
#define D_RECFRST              189
#define D_RECLAST              190
#define D_RECLOCK              191
#define D_RECLSTAT             192
#define D_RECNEXT              193
#define D_RECNUM               194
#define D_RECOVER              195
#define D_RECPREV              196
#define D_RECREAD              197
#define D_RECSET               198
#define D_RECSTAT              199
#define D_RECWRITE             200
#define D_RENCLEAN             201
#define D_RENFILE              202
#define D_RERDCURR             203
#define D_RLBCLR               204
#define D_RLBSET               205
#define D_RLBTST               206
#define D_SET_DBERR            207
#define D_SET_DBTRUENAME       208
#define D_SETDB                209
#define D_SETFILES             210
#define D_SETFREE              211
#define D_SETKEY               212
#define D_SETLOCK              213
#define D_SETLSTAT             214
#define D_SETMM                215
#define D_SETMO                216
#define D_SETMR                217
#define D_SETNUM               218
#define D_SETOM                219
#define D_SETOO                220
#define D_SETOR                221
#define D_SETPAGES             222
#define D_SETRM                223
#define D_SETRO                224
#define D_STSCM                225
#define D_STSCO                226
#define D_STSCR                227
#define D_STSCS                228
#define D_TIMEOUT              229
#define D_TRABORT              230
#define D_TRBEGIN              231
#define D_TREND                232
#define D_UTSCM                233
#define D_UTSCO                234
#define D_UTSCR                235
#define D_UTSCS                236
#define D_WRCURR               237
#define D_OPENTASK             238
#define D_CLOSETASK            239
#define D_OPEN_SG              240
#define D_IOPEN_SG             241

#define DBSTAR_FIRST  D_CHECKID
#define DBSTAR_LAST   D_IOPEN_SG


#endif /* APIDEFS_H */


