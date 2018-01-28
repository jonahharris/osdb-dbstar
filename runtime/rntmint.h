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

#if !defined(RNTMINT_H)
#define RNTMINT_H

#define FROM_DLL     1
#define FROM_RUNTIME 2

void      INTERNAL_FCN dbInit(int);
void      INTERNAL_FCN dbTerm(int);
int       INTERNAL_FCN errstring(DB_TCHAR *, int, int, DB_TASK *);
int       INTERNAL_FCN initenv(DB_TASK *);
int       INTERNAL_FCN msg_trans(int, void *, size_t, void **, size_t *,
      DB_TASK *);
DB_TCHAR *INTERNAL_FCN dbstar_apistr(int);
int       INTERNAL_FCN send_lock_pkt(int *, DB_TASK *);
int       INTERNAL_FCN send_free_pkt(DB_TASK *);
void      INTERNAL_FCN reset_locks(DB_TASK *);

void      INTERNAL_FCN STAT_send_msg(int, size_t, size_t, DB_TASK *);
void      INTERNAL_FCN STAT_recv_msg(int, size_t, size_t, DB_TASK *);
#endif
