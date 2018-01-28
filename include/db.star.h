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

#ifndef DB_STAR_H
#define DB_STAR_H

#include "psp.h"

#define MULTI_TAFFILE

/*
    Unicode Definitions:

    UNICODE

    Defining this symbol causes all db.*'s internal string handling,
    e.g. filenames, user id, lock manager name etc, to be done with
    widechar strings.

    On Unix systems there is not much point in defining UNICODE,
    as all disk I/O and network protocol functions take single-byte
    character strings as arguments. Thus db.* must convert any Unicode
    strings to single-byte strings before calling these functions.
    Unicode strings require more memory (on Unix, four times as much)
    as single-byte strings.

    UNICODE_DATA

    Defining this symbol enables support for widechar data fields.
    Support for Unicode fields does not require db.* library to be
    compiled with UNICODE defined (i.e. Unicode fields can still be
    handled even if internal string manipulation is ANSI / ascii).
*/
#ifdef QNX

#if defined(__UNICODE__) || defined(_UNICODE)
#define UNICODE
#endif

#endif /* QNX */

/* ======================================================================== */
/* Status Codes                                                             */
/* ======================================================================== */

/*
    User errors
*/
#define S_USER_FIRST       -1    /* first user err */

#define S_DBOPEN           -1    /* database not opened */
#define S_INVSET           -2    /* invalid set */
#define S_INVREC           -3    /* invalid record */
#define S_INVDB            -4    /* invalid database */
#define S_INVFLD           -5    /* invalid field name */
#define S_INVADDR          -6    /* invalid db_address */
#define S_NOCR             -7    /* no current record */
#define S_NOCO             -8    /* set has no current owner */
#define S_NOCM             -9    /* set has no current member */
#define S_KEYREQD          -10   /* key value required */
#define S_BADTYPE          -11   /* invalid lock type */
#define S_HASMEM           -12   /* record is owner of non-empty set(s) */
#define S_ISMEM            -13   /* record is member of set(s) */
#define S_ISOWNED          -14   /* member already owned */
#define S_ISCOMKEY         -15   /* field is a compound key */
#define S_NOTCON           -16   /* record not connected to set */
#define S_NOTKEY           -17   /* field is not a valid key */
#define S_INVOWN           -18   /* record not legal owner of set */
#define S_INVMEM           -19   /* record not legal member of set */
#define S_SETPAGES         -20   /* error in d_setpages (database open or bad param) */
#define S_INCOMPAT         -21   /* incompatible dictionary file */
#define S_DELSYS           -22   /* illegal attempt to delete system record */
#define S_NOTFREE          -23   /* attempt to lock previously locked rec or set */
#define S_NOTLOCKED        -24   /* attempt to access unlocked record or set */
#define S_TRANSID          -25   /* transaction id not be supplied */
#define S_TRACTIVE         -26   /* transaction already active */
#define S_TRNOTACT         -27   /* transaction not currently active */
#define S_BADPATH          -28   /* the directory string is invalid */
#define S_TRFREE           -29   /* attempt to free a lock inside a transaction */
#define S_RECOVERY         -30   /* Auto-recovery is about to occur */
#define S_NOTRANS          -31   /* attempted update outside of transaction */
#define S_EXCLUSIVE        -32   /* functions requires exclusive db access */
#define S_STATIC           -33   /* Attempted to write lock a static file */
#define S_USERID           -34   /* No user id exists */
#define S_NAMELEN          -35   /* database file/path name too long */
#define S_RENAME           -36   /* invalid file number passed to d_renfile */
#define S_NOTOPTKEY        -37   /* field is not an optional key */
#define S_BADFIELD         -38   /* field not defined in current record type */
#define S_COMKEY           -39   /* record/field has/in a compound key */
#define S_INVNUM           -40   /* invalid record or set number */
#define S_TIMESTAMP        -41   /* record or set not timestamped */
#define S_BADUSERID        -42   /* invalid user id - not alphanumeric */
/* -43 */
#define S_INVENCRYPT       -44   /* Invalid encryption key */
/* -45 */
#define S_NOTYPE           -46   /* No current record type */
#define S_INVSORT          -47   /* Invalid country table sort string */
#define S_DBCLOSE          -48   /* database not closed */
#define S_INVPTR           -49   /* invalid pointer */
#define S_INVID            -50   /* invalid internal ID */
#define S_INVLOCK          -51   /* invalid lockmgr communication type */
#define S_INVTASK          -52   /* invalid task id */
#define S_NOLOCKCOMM       -53   /* must call d_lockcomm() before d_open() */
#define S_NOTIMPLEMENTED   -54  

#define S_USER_LAST        -54   /* last user error */

/*
    System errors
*/
#define S_SYSTEM_FIRST     -900  /* first system error */

#define S_NOSPACE          -900  /* out of disk space */
#define S_SYSERR           -901  /* system error */
#define S_FAULT            -902  /* page fault -- too many locked pages */
#define S_NOWORK           -903  /* cannot access dbQuery dictionary */
#define S_NOMEMORY         -904  /* unable to allocate sufficient memory */
#define S_NOFILE           -905  /* unable to locate a file */
#define S_DBLACCESS        -906  /* unable to open lfg, taf, dbl or log file */
#define S_DBLERR           -907  /* TAF or DBL I/O error */
#define S_BADLOCKS         -908  /* inconsistent database locks */
#define S_RECLIMIT         -909  /* file record limit reached */
#define S_KEYERR           -910  /* key file inconsistency detected */
/* -911 */
#define S_FSEEK            -912  /* Bad seek on database file */
#define S_LOGIO            -913  /* error reading/writing log file */
#define S_BADREAD          -914  /* Bad read on database/overflow file */
#define S_NETSYNC          -915  /* Network synchronization error */
#define S_DEBUG            -916  /* Debugging check interrupt */
#define S_NETERR           -917  /* Network communications error */
/* -918 */
#define S_BADWRITE         -919  /* Bad write on database/overflow file */
#define S_NOLOCKMGR        -920  /* Unable to open lockmgr session */
#define S_DUPUSERID        -921  /* DBUSERID is already used by someone else */
#define S_LMBUSY           -922  /* The lock manager table(s) are full */
#define S_DISCARDED        -923  /* attempt to lock discarded memory */
/* -924 */
#define S_LMCERROR         -925  /* Network layer error code */
/* -926 */
/* -927 */
/* -928 */
/* -929 */
/* -930 */
/* -931 */
/* -932 */
/* -933 */
/* -934 */
#define S_TAFCREATE        -935  /* failed to create taf file  */
/* -936 */
/* -937 */
/* -938 */
#define S_READONLY         -939  /* d_on_opt(READONLY), unable to update file */
#define S_EACCESS          -940  /* sharing violation, file in use */
/* -941 */
/* -942 */
#define S_RECFAIL          -943  /* automatic recovery failed */
#define S_TAFSYNC          -944  /* TAF file not consistant with LMC */
#define S_TAFLOCK          -945  /* Failed to lock taf file */
/* -946 */
#define S_REENTER          -947  /* db.* entered re-entrantly */

#define S_SYSTEM_LAST      -947  /* last system error */

/*
    Internal system errors
*/
#define S_INTERNAL_FIRST   -9001 /* first internal error */

/* -9001 */
/* -9002 */
#define SYS_BADTREE        -9003 /* b-tree malformed */
#define SYS_KEYEXIST       -9004 /* key value already exists */
/* -9005 */
#define SYS_LOCKARRAY      -9006 /* lock packet array exceeded */
/* -9007 */
#define SYS_BADFREE        -9008 /* attempt to free empty table */
#define SYS_BADOPTKEY      -9009 /* calculating optkey index */
/* -9010 */
#define SYS_IXNOTCLEAR     -9011 /* ix-cache not reset after trans */
#define SYS_INVLOGPAGE     -9012 /* invalid page in log file */
#define SYS_INVFLDTYPE     -9013 /* illegal field type */
#define SYS_INVSORT        -9014 /* illegal sort ordering */
/* -9015 */
#define SYS_INVPGTAG       -9016 /* invalid page tag */
#define SYS_INVHOLD        -9017 /* bad hold count */
#define SYS_HASHCYCLE      -9018 /* cycle detected in hash chain */
#define SYS_INVLRU         -9019 /* invalid lru page */
#define SYS_INVPAGE        -9020 /* invalid cache page */
#define SYS_INVPGCOUNT     -9021 /* bad page tag page count */
#define SYS_INVPGSIZE      -9022 /* invalid cache page size */
#define SYS_PZACCESS       -9023 /* invalid access to page zero */
#define SYS_BADPAGE        -9024 /* wrong page */
#define SYS_INVEXTEND      -9025 /* illegal attempt to extend file */
/* -9026 */
#define SYS_PZNEXT         -9027 /* bad pznext */
#define SYS_DCHAIN         -9028 /* bad dchain */
#define SYS_EOF            -9029 /* attempt to write past EOF */
#define SYS_FILEMODIFIED   -9030 /* locked file was modified by another user */

#define S_INTERNAL_LAST    -9030 /* last internal error */

/*
    Function statuses
*/
#define S_FUNCTION_FIRST   0     /* first function status */

#define S_OKAY             0     /* normal return, okay */
#define S_EOS              1     /* end of set */
#define S_NOTFOUND         2     /* record not found */
#define S_DUPLICATE        3     /* duplicate key */
#define S_KEYSEQ           4     /* field type used out of sequence in d_keynext */
#define S_UNAVAIL          5     /* database file currently unavailable */
#define S_DELETED          6     /* record/set deleted since last accessed */
#define S_UPDATED          7     /* record/set updated since last accessed */
#define S_LOCKED           8     /* current record's lock bit is set */
#define S_UNLOCKED         9     /* current record's lock bit is clear */
#define S_SETCLASH         10    /* set ptrs invalidated by another user */

#define S_FUNCTION_LAST    10    /* last function status */


/* ------------------------------------------------------------------------ */

typedef short           FILE_NO;    /* db.* file number */

typedef DB_ULONG        DB_ADDR;     /* 3 bytes for slot, 1 byte for file */
typedef long            F_ADDR;      /* file address: page or slot number */

#define MAXRECORDS      0x00FFFFFFL
#define NULL_DBA        0L


/* field number indicator = rec * FLDMARK + fld_in_rec */
#define FLDMARK 1000L

/* record number indicator */
#define RECMARK 10000

/* set number indicator -- must be greater than RECMARK */
#define SETMARK 20000

/* runtime option flags */
/* hi-order byte reserved for specifying option sets */
#define DCHAINUSE       0x00000001L    /* use the dchain for new records */
#define TRLOGGING       0x00000002L    /* use transaction logging/recovery */
#define ARCLOGGING      0x00000004L    /* use transaction archiving */
#define IGNORECASE      0x00000008L    /* use caseblind compares for strings */
#define DELETELOG       0x00000080L    /* have d_close() delete the log file */
#define SYNCFILES       0x00000100L    /* bypass any system cache */
#define PORTABLE        0x00000200L    /* use guard files instead of locking */
#define IGNOREENV       0x00000400L    /* ignore environment variables */
#define READONLY        0x00000800L    /* open database for readonly access */
#define TXTEST          0x00001000L    /* test transaction recovery */
#define NORECOVER       0x00002000L    /* For internal use only, will corrupt databases */
#define TRUNCATELOG     0x00008000L    /* truncate log file at each d_trbegin */
#define PREALLOC_CACHE  0x00020000L    /* allocate cache pages during d_open */
#define MULTITAF        0x00100000L    /* use multiple taf files */
#define READNAMES       0x00200000L    /* read rec/fld/set names on db open */
#define MBSSORT         0x00400000L    /* string sort using mbs functions */

/* DEBUG_OPT cannot be or'd together in one d_on/off_opt() call with non-DEBUG_OPTs
*/
#define DEBUG_OPT   (DB_ULONG) 0x40000000
#define PZVERIFY    (DEBUG_OPT | 0x00000001L)  /* verify pz_next against the file length */
#define PAGE_CHECK  (DEBUG_OPT | 0x00000002L)  /* track page id from dio_in until dio_out */
#define LOCK_CHECK  (DEBUG_OPT | 0x00000004L)  /* check file and page for illegal change by another process */
#define CACHE_CHECK (DEBUG_OPT | 0x00000008L)  /* check for cache corruption after each d_ call */
/*
    Note that PAGE_CHECK and LOCK_CHECK are mutually exclusive.
    WARNING! LOCK_CHECK will cause disk I/O on each cache access !!!
*/

/* TRACE_OPT cannot be or'd together in one d_on/off_opt() call with non-TRACE_OPTs
*/
#define TRACE_OPT     (DB_ULONG) 0x20000000
#define TRACE_DBERR   (TRACE_OPT | 0x00000001L)  /* trace errors if any trace flag set */
#define TRACE_API     (TRACE_OPT | 0x00000002L)  /* trace api functions */
#define TRACE_LOCKS   (TRACE_OPT | 0x00000004L)  /* trace file locks */
#define TRACE_UNICODE (TRACE_OPT | 0x00000008L)  /* output to trace file in Unicode text */

/* option tags for d_set_options
*/
#define OPT_MEM_HOOK 1

/* ------------------------------------------------------------------------ */

#ifndef max
#define min(a, b)    ((a) < (b) ? (a) : (b))
#define max(a, b)    ((a) > (b) ? (a) : (b))
#endif

/* function is referenced internally only */
#define INTERNAL_FCN    /**/
/* function may be exposed in shared library or DLL */
#define EXTERNAL_FCN    /**/

/* Version number string at start of DBD file must match one of these: */
#define dbd_VERSION_300  "V3.00\032"
#define dbd_VERSION_301  "V3.01\032"
#define dbd_VERSION_500  "V5.00\032"
#define dbd_VERSION_u500 "U5.00\032"

/* Numeric values representing DBD version strings: */
#define dbd_V300  300
#define dbd_V301  301
#define dbd_V500  500
#define dbd_U500 -500


/* ------------------------------------------------------------------------ */

typedef struct lock_request
{
    unsigned int item;                  /* record or set number */
    int type;                           /* lock type: 'r', 'w', 'x', 'k' */
}  LOCK_REQUEST;

#define TRANS_ID_LEN 21

typedef void (EXTERNAL_FCN * ERRORPROC) (int, DB_TCHAR *);

#ifndef TASK_DEFN
typedef void DB_TASK;
#else
#include "dbtype.h"
#endif

#define  CURR_DB        -1
#define  ALL_DBS        -2
#define  VOID_DB        -3

#ifdef __cplusplus
extern "C"
{
#endif

#include "dproto.h"

#ifdef __cplusplus
}
#endif

#endif /* DB.STAR_H */


