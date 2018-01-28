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

#ifndef DPROTO_H
#define DPROTO_H

int EXTERNAL_FCN d_checkid(DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_close(DB_TASK *);
int EXTERNAL_FCN d_closeall(DB_TASK *);
int EXTERNAL_FCN d_closetask(DB_TASK *);
int EXTERNAL_FCN d_cmstat(int, DB_TASK *, int);
int EXTERNAL_FCN d_cmtype(int, int *, DB_TASK *, int);
int EXTERNAL_FCN d_connect(int, DB_TASK *, int);
int EXTERNAL_FCN d_costat(int, DB_TASK *, int);
int EXTERNAL_FCN d_cotype(int, int *, DB_TASK *, int);
int EXTERNAL_FCN d_crget(DB_ADDR *, DB_TASK *, int);
int EXTERNAL_FCN d_crread(long, void *, DB_TASK *, int);
int EXTERNAL_FCN d_crset(DB_ADDR *, DB_TASK *, int);
int EXTERNAL_FCN d_crstat(DB_TASK *, int);
int EXTERNAL_FCN d_crtype(int *, DB_TASK *, int);
int EXTERNAL_FCN d_crwrite(long, void *, DB_TASK *, int);
int EXTERNAL_FCN d_csmget(int, DB_ADDR *, DB_TASK *, int);
int EXTERNAL_FCN d_csmread(int, long, void *, DB_TASK *, int);
int EXTERNAL_FCN d_csmset(int, DB_ADDR *, DB_TASK *, int);
int EXTERNAL_FCN d_csmwrite(int, long, const void *, DB_TASK *, int);
int EXTERNAL_FCN d_csoget(int, DB_ADDR *, DB_TASK *, int);
int EXTERNAL_FCN d_csoread(int, long, void *, DB_TASK *, int);
int EXTERNAL_FCN d_csoset(int, DB_ADDR *, DB_TASK *, int);
int EXTERNAL_FCN d_csowrite(int, long, const void *, DB_TASK *, int);
int EXTERNAL_FCN d_csstat(int, DB_TASK *, int);
int EXTERNAL_FCN d_ctbpath(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_ctscm(int, DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_ctsco(int, DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_ctscr(DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_curkey(DB_TASK *, int);
int EXTERNAL_FCN d_dbdpath(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_dberr(int, DB_TASK *);
int EXTERNAL_FCN d_dbfpath(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_dblog(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_dbnum(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_dbstat(int, int, void *, int, DB_TASK *);
int EXTERNAL_FCN d_dbtaf(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_dbtmp(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_dbuserid(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_decode_dba(DB_ADDR, short *, DB_ADDR *);
int EXTERNAL_FCN d_def_opt(unsigned long, DB_TASK *);
int EXTERNAL_FCN d_delete(DB_TASK *, int);
int EXTERNAL_FCN d_destroy(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_discon(int, DB_TASK *, int);
int EXTERNAL_FCN d_disdel(DB_TASK *, int);
int EXTERNAL_FCN d_encode_dba(short, DB_ADDR, DB_ADDR *);
int EXTERNAL_FCN d_fillnew(int, const void *, DB_TASK *, int);
int EXTERNAL_FCN d_findco(int, DB_TASK *, int);
int EXTERNAL_FCN d_findfm(int, DB_TASK *, int);
int EXTERNAL_FCN d_findlm(int, DB_TASK *, int);
int EXTERNAL_FCN d_findnm(int, DB_TASK *, int);
int EXTERNAL_FCN d_findpm(int, DB_TASK *, int);
int EXTERNAL_FCN d_fldnum(int *, long, DB_TASK *, int);
int EXTERNAL_FCN d_freeall(DB_TASK *);
int EXTERNAL_FCN d_gtscm(int, DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_gtsco(int, DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_gtscr(DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_gtscs(int, DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_iclose(DB_TASK *, int);
int EXTERNAL_FCN d_initfile(FILE_NO, DB_TASK *, int);
int EXTERNAL_FCN d_initialize(DB_TASK *, int);
int EXTERNAL_FCN d_internals(DB_TASK *, int, int, int, void *, unsigned);
int EXTERNAL_FCN d_iopen(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_iopen_sg(const DB_TCHAR *, const SG *, DB_TASK *);
int EXTERNAL_FCN d_ismember(int, DB_TASK *, int);
int EXTERNAL_FCN d_isowner(int, DB_TASK *, int);
int EXTERNAL_FCN d_keybuild(DB_TASK *, int);
int EXTERNAL_FCN d_keydel(long, DB_TASK *, int);
int EXTERNAL_FCN d_keyexist(long, DB_TASK *, int);
int EXTERNAL_FCN d_keyfind(long, const void *, DB_TASK *, int);
int EXTERNAL_FCN d_keyfree(long, DB_TASK *, int);
int EXTERNAL_FCN d_keyfrst(long, DB_TASK *, int);
int EXTERNAL_FCN d_keylast(long, DB_TASK *, int);
int EXTERNAL_FCN d_keylock(long, DB_TCHAR *, DB_TASK *, int);
int EXTERNAL_FCN d_keylstat(long, DB_TCHAR *, DB_TASK *, int);
int EXTERNAL_FCN d_keynext(long, DB_TASK *, int);
int EXTERNAL_FCN d_keyprev(long, DB_TASK *, int);
int EXTERNAL_FCN d_keyread(void *, DB_TASK *);
int EXTERNAL_FCN d_keystore(long, DB_TASK *, int);
int EXTERNAL_FCN d_lmclear(const DB_TCHAR *, const DB_TCHAR *, LMC_AVAIL_FCN *, DB_TASK *);
int EXTERNAL_FCN d_lmstat(DB_TCHAR *, int *, DB_TASK *);
int EXTERNAL_FCN d_lock(int, LOCK_REQUEST *, DB_TASK *, int);
int EXTERNAL_FCN d_lockcomm(LMC_AVAIL_FCN *, DB_TASK *);
int EXTERNAL_FCN d_lockmgr(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_locktimeout(int, int, DB_TASK *);
int EXTERNAL_FCN d_makenew(int, DB_TASK *, int);
int EXTERNAL_FCN d_mapchar(unsigned char, unsigned char, const char *, unsigned char, DB_TASK *);
int EXTERNAL_FCN d_members(int, long *, DB_TASK *, int);
int EXTERNAL_FCN d_off_opt(unsigned long, DB_TASK *);
int EXTERNAL_FCN d_on_opt(unsigned long, DB_TASK *);
int EXTERNAL_FCN d_open(const DB_TCHAR *, const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_open_sg(const DB_TCHAR *, const DB_TCHAR *, const SG *, DB_TASK *);
int EXTERNAL_FCN d_opentask(DB_TASK **);
int EXTERNAL_FCN d_rdcurr(DB_ADDR * *, int *, DB_TASK *, int);
int EXTERNAL_FCN d_dbini(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_dbver(DB_TCHAR *, DB_TCHAR *, int);
int EXTERNAL_FCN d_recfree(int, DB_TASK *, int);
int EXTERNAL_FCN d_recfrst(int, DB_TASK *, int);
int EXTERNAL_FCN d_reclast(int, DB_TASK *, int);
int EXTERNAL_FCN d_reclock(int, DB_TCHAR *, DB_TASK *, int);
int EXTERNAL_FCN d_reclstat(int, DB_TCHAR *, DB_TASK *, int);
int EXTERNAL_FCN d_recnext(DB_TASK *, int);
int EXTERNAL_FCN d_recnum(int *, int , DB_TASK *, int);
int EXTERNAL_FCN d_recover(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_recprev(DB_TASK *, int);
int EXTERNAL_FCN d_recread(void *, DB_TASK *, int);
int EXTERNAL_FCN d_recset(int, DB_TASK *, int);
int EXTERNAL_FCN d_recstat(DB_ADDR, DB_ULONG, DB_TASK *, int);
int EXTERNAL_FCN d_recwrite(const void *, DB_TASK *, int);
int EXTERNAL_FCN d_renclean(DB_TASK *);
int EXTERNAL_FCN d_renfile(const DB_TCHAR *, FILE_NO, const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_rerdcurr(DB_ADDR * *, DB_TASK *, int);
int EXTERNAL_FCN d_rlbclr(DB_TASK *, int);
int EXTERNAL_FCN d_rlbset(DB_TASK *, int);
int EXTERNAL_FCN d_rlbtst(DB_TASK *, int);
int EXTERNAL_FCN d_set_dberr(ERRORPROC, DB_TASK *);
int EXTERNAL_FCN d_setdb(int, DB_TASK *);
int EXTERNAL_FCN d_setfiles(int, DB_TASK *);
int EXTERNAL_FCN d_setfree(int, DB_TASK *, int);
int EXTERNAL_FCN d_setkey(long, const void *, DB_TASK *, int);
int EXTERNAL_FCN d_setlock(int, DB_TCHAR *, DB_TASK *, int);
int EXTERNAL_FCN d_setlstat(int, DB_TCHAR *, DB_TASK *, int);
int EXTERNAL_FCN d_setmm(int, int, DB_TASK *, int);
int EXTERNAL_FCN d_setmo(int, int, DB_TASK *, int);
int EXTERNAL_FCN d_setmr(int, DB_TASK *, int);
int EXTERNAL_FCN d_setnum(int *, int , DB_TASK *, int);
int EXTERNAL_FCN d_setom(int, int, DB_TASK *, int);
int EXTERNAL_FCN d_setoo(int, int, DB_TASK *, int);
int EXTERNAL_FCN d_setor(int, DB_TASK *, int);
int EXTERNAL_FCN d_setpages(int, int, DB_TASK *);
int EXTERNAL_FCN d_setrm(int, DB_TASK *, int);
int EXTERNAL_FCN d_setro(int, DB_TASK *, int);
int EXTERNAL_FCN d_stscm(int, DB_ULONG, DB_TASK *, int);
int EXTERNAL_FCN d_stsco(int, DB_ULONG, DB_TASK *, int);
int EXTERNAL_FCN d_stscr(DB_ULONG, DB_TASK *, int);
int EXTERNAL_FCN d_stscs(int, DB_ULONG, DB_TASK *, int);
int EXTERNAL_FCN d_timeout(int, DB_TASK *);
int EXTERNAL_FCN d_trabort(DB_TASK *);/* dblfcns.c */
int EXTERNAL_FCN d_trbegin(const DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN d_trbound(DB_TASK *);
int EXTERNAL_FCN d_trend(DB_TASK *);
int EXTERNAL_FCN d_trlog(int, long, const char *, int, DB_TASK *);
int EXTERNAL_FCN d_trmark(DB_TASK *);
int EXTERNAL_FCN d_utscm(int, DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_utsco(int, DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_utscr(DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_utscs(int, DB_ULONG *, DB_TASK *, int);
int EXTERNAL_FCN d_wrcurr(DB_ADDR *, DB_TASK *, int);

#endif /* ! DPROTO_H */


