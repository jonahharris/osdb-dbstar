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

#ifndef DBSTAT_H
#define DBSTAT_H
/*
    Notes on terminology:

    The data cache contains data and key pages read from the database.
    
    The ix cache contains index pages used to track data and key pages which
    have overflowed into the log file.

    'pz' means page zero, which refers to the zero'th page in each data or key
    file.  Page zero contains the head of the delete chain, and the location of
    the logical end of the file.

    'rlb' refers to record lock bits.

    'log' refers to the log, or overflow, file.  During a transaction, all
    modified pages are redirected to the log file rather than being written to
    the database.  When the transaction is ended, the log file holds all the
    changes so that recovery is possible.

    'taf' refers to the TAF file, which is used to indicate which kind of lock
    manager is being used, and to indicate if recovery is needed.

    'dbl' refers to the general lock manager's DataBase Lock file.  This file
    contains an image of the in-memory data structures.
*/

/*
    Lock statistics are available on a file-by-file basis from FILE_STATS, or
    summed across all files by GEN_STATS.
*/
typedef struct LOCK_STATS_S
{
    DB_ULONG read_locks;       /* how many read locks have been granted */
    DB_ULONG write_locks;      /* how many write locks have been granted */
    DB_ULONG excl_locks;       /* how many exclusive locks have been granted */
    DB_ULONG rlb_locks;        /* how many rlb locks have been granted */
    DB_ULONG read_to_write;    /* how many read locks upgraded to a write lock */
    DB_ULONG read_to_excl;     /* how many read locks upgraded to an exclusive lock */
    DB_ULONG read_to_rlb;      /* how many read locks upgraded to an rlb lock */
    DB_ULONG write_to_read;    /* how many write locks downgraded to a read lock */
    DB_ULONG excl_to_read;     /* how many exclusive locks downgraded to a read lock */
    DB_ULONG rlb_to_read;      /* how many rlb locks downgraded to a read lock */
    DB_ULONG free_read;        /* how many read locks have been freed */
    DB_ULONG free_write;       /* how many write locks have been freed */
    DB_ULONG free_excl;        /* how many exclusive locks have been freed */
    DB_ULONG free_rlb;         /* how many rlb locks have been freed */
} LOCK_STATS;


/*
    Cache memory statistics are available for the cache from GEN_STATS.
*/
typedef struct MEM_STATS_S
{
    DB_ULONG mem_used;         /* the amount of memory allocated by the cache */
    DB_ULONG max_mem;          /* the maximum amount of cache memory used at one time */
    DB_ULONG allocs;           /* the number of cache memory allocations that have
                                            been made */
} MEM_STATS;


/*
    Cache statistics are available on a file-by-file basis from FILE_STATS, or
    summed across all files by GEN_STATS.
*/
typedef struct CACHE_STATS_S
{
    DB_ULONG lookups;          /* the number of cache page lookups */
    DB_ULONG hits;             /* the number of cache hits */
    short num_pages;           /* the number of pages currently in the cache */
} CACHE_STATS, TAG_STATS;


/*
    Disk I/O statistics are available on a file-by-file basis from FILE_STATS, or
    summed across all files by GEN_STATS.
*/
typedef struct IO_STATS_S
{
    DB_ULONG read_count;       /* the number of physical disk reads */
    DB_ULONG read_bytes;       /* the number of bytes read */
    DB_ULONG write_count;      /* the number of physical disk writes */
    DB_ULONG write_bytes;      /* the number of bytes written */
} IO_STATS;


/*
    File statistics are available on a file-by-file basis from FILE_STATS, or
    summed across all files by GEN_STATS.
*/
typedef struct FILE_STATS_S
{
    CACHE_STATS cache_stats;   /* cache statistics for this file */
    DB_ULONG    file_opens;    /* number of times this file has been opened */
    IO_STATS    pz_stats;      /* statistics on page zero I/O for this file */
    IO_STATS    pg_stats;      /* statistics on data page I/O for this file */
    IO_STATS    rlb_stats;     /* statistics on rlb I/O access in this file */
    DB_ULONG    new_pages;     /* how many new pages have been created - this includes
                                            pages that are unneeded due to d_trabort() because
                                            the file does not get truncated. */
    LOCK_STATS  lock_stats;    /* statistics on file locks and frees */
} FILE_STATS;


/*
    Lock manager communication message statistics are available for each message
    type, or summed across all types by GEN_STATS.
*/
typedef struct MSG_STATS_S
{
    DB_ULONG msg_count;        /* number of messages sent to the lock manager */
    DB_ULONG send_packets;     /* number of network packets sent to the lock manager */
    DB_ULONG send_bytes;       /* total number of bytes sent */
    DB_ULONG recv_packets;     /* number of packets received from the lock manager */
    DB_ULONG recv_bytes;       /* total number of bytes received */
} MSG_STATS;


/*
    General db.* operations statistics
*/
typedef struct GEN_STATS_S
{
    DB_ULONG    dbenter;       /* how many times the d_ functions have been called */

    /* cache */
    MEM_STATS   dbmem_stats;   /* amount of memory used by the data cache */
    MEM_STATS   ixmem_stats;   /* amount of memory used by the ix cache */
    CACHE_STATS db_stats;      /* usage statistics for the data cache */
    CACHE_STATS ix_stats;      /* usage statistics for the ix cache */

    /* file I/O */
    DB_ULONG    file_opens;    /* how many times data and key files have been opened */
    IO_STATS    pz_stats;      /* file I/O statistics summed up for all page zeros */
    IO_STATS    pg_stats;      /* file I/O statistics summed up for all data and
                                            key files */
    IO_STATS    rlb_stats;     /* file I/O statistics summed up for all record
                                            lock bit access */
    DB_ULONG    new_pages;

    DB_ULONG    log_opens;     /* how many times the log file has been opened */
    IO_STATS    log_stats;     /* file I/O statistics for the log file */
    DB_ULONG    taf_opens;     /* how many times the TAF file has been opened */
    IO_STATS    taf_stats;     /* file I/O statistics for the TAF file */
    DB_ULONG    dbl_opens;     /* how many times the dbl file has been opened */
    IO_STATS    dbl_stats;     /* file I/O statistics for the dbl file */

    int      files_open;       /* current number of open files */
    int      max_files_open;   /* maximum number of open files at one time */

    /* transactions */
    DB_ULONG    trbegins;      /* the total number of calls to d_trbegin */
    DB_ULONG    trends;        /* the total number of calls to d_trend */
    DB_ULONG    traborts;      /* the total number of calls to d_trabort */
    DB_ULONG    trovfl;        /* the number of transactions which overflowed into the
                                            log file (i.e. were too large to fit in the cache) */

    /* lock manager */
    LOCK_STATS  lock_stats;    /* file locking statistics summed up for all files */
    MSG_STATS   msg_stats;     /* lock manager communication statistics summed up for all
                                            message types */
} GEN_STATS;


#define GEN_STAT   0
#define MEM_STAT   1    /* reserved */
#define TAG_STAT   2
#define FILE_STAT  3
#define MSG_STAT   4

#endif /* DBSTAT_H */


