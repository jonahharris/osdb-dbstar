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

#ifndef OPTIONS_H
#define OPTIONS_H

#define DEFAULT_OPTS        (DB_ULONG) (DCHAINUSE | TRLOGGING | SYNCFILES)

#ifdef DB_DEBUG
#define DEFAULT_DEBUG_OPTS  (DB_ULONG) (~DEBUG_OPT & (PZVERIFY))
#endif
#ifdef DB_TRACE
#define DEFAULT_TRACE_OPTS  (DB_ULONG) (getenv("DBTRACE") ? 0x0FFFFFFF : 0)
#endif

/*
    some options cannot be changed once a database has been opened
*/
#define OPEN_OPTS        (DB_ULONG) (TRLOGGING | ARCLOGGING | SYNCFILES \
                                   | IGNOREENV | READONLY | PORTABLE \
                                   | NORECOVER | PREALLOC_CACHE | READNAMES)
                                      

#ifdef DB_DEBUG
#define DEBUG_OPEN_OPTS  (DB_ULONG) (PZVERIFY | PAGE_CHECK | LOCK_CHECK)
#endif
#ifdef DB_TRACE
#define TRACE_OPEN_OPTS  (DB_ULONG) (0)
#endif

#endif  /* OPTIONS_H */
