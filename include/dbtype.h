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

#ifndef DBTYPE_H
#define DBTYPE_H

/* If INTERNAL_H is defined, then this file is being included only for the
    struct definitions needed to use d_internals().  Therefore, much of the
    stuff in here is not needed and may even cause conflicts with other
    software packages.
    If this file is needed for all the other stuff as well as the struct
    definitions and "internal.h" is also needed, then this file must be
    #include-d BEFORE "internal.h" is #include-d.
*/

/* ---------------------------------------------------------------------------
    Configure the runtime
*/


#define DBSTAT

#ifdef DB_DEBUG
#define DBERR_EX     /* display file and line number for dberr */
#define DB_TRACE     /* enable runtime tracing */
#endif

/* ------------------------------------------------------------------------ */

#define MAX_ALLOC   ((unsigned long)USHRT_MAX)  /* max allocation size */
#define MAX_TABLE   ((unsigned long)INT_MAX)    /* max table entries */


#ifndef INTERNAL_H

#define PGHOLD    1
#define NOPGHOLD  0
#define PGFREE    1
#define NOPGFREE  0

#define KEYFIND   0
#define KEYNEXT   1
#define KEYPREV   2
#define KEYFRST   3
#define KEYLAST   4

/* Constants used by d_open and d_iopen */
#define FIRST_OPEN 0
#define INCR_OPEN  1

#endif /* INTERNAL_H */

/* dictionary attribute flags */
#define SORTFLD         0x0001   /* field is a set sort field */
#define STRUCTFLD       0x0002   /* field is sub-field of struct */
#define UNSIGNEDFLD     0x0004   /* field is unsigned */
#define COMKEYED        0x0010   /* (record contains) or (field included in) compound key */

#define TIMESTAMPED     0x0001   /* record/set is timestamped */
#define STATIC          0x0002   /* file/record is static */

#define TEMPORARY       0x0020   /* file is temporary (no lockmgr & or logging) */
#define NEEDS_COMMIT    0x0001   /* this file has been written to during trx */
#define TS_IS_CURRENT   0x0004   /* the timestamp for the file has already been updated */
#define NOT_TTS_FILE    0x0008   /* this file is NEVER under TTS control */

#define READ_LOCK       0x0100   /* file has been read locked by TTS */
#define WRITE_LOCK      0x0200   /* file has been write locked by TTS */
#define TTSCHECKED      0x0400   /* file has been checked for TTS */
#define NO_LOCK         0xFCFF   /* file has not been locked for TTS */

#ifndef INTERNAL_H

#ifndef TRUE
#define FALSE  (1==0)
#define TRUE   (! FALSE)
#endif

#define DBUSER_ALIVE_BYTE  1
#define DBUSER_COMMIT_BYTE 2

/* Number of elements in a vector */
#define arraysize(v) (sizeof(v)/sizeof(*(v)))

#define DBD_COMPAT_LEN 6
#define INT_SIZE     sizeof(int)
#define SHORT_SIZE   sizeof(short)
#define LONG_SIZE    sizeof(long)
#define CHAR_SIZE    sizeof(char)
#define DB_ADDR_SIZE sizeof(DB_ADDR)
#define PGHDRSIZE    4
#define CTBNAME      DB_TEXT("db.star.ctb")

#define FLOAT_SIZE   sizeof(float)
#define DOUBLE_SIZE  sizeof(double)

#endif /* INTERNAL_H */

#define DBA_NONE  ((DB_ADDR) -1)
#define DCH_NONE  ((F_ADDR) -1)

#define FILEMASK     0x00FFL
#define FILESHIFT    24


#define RECHDRSIZE   (sizeof(short) + DB_ADDR_SIZE)
#define ADDRMASK     MAXRECORDS
#define OPTKEYNDX    0x003F
#define OPTKEY_LIMIT 63
#define OPTKEYSHIFT  10
#define OPTKEYMASK   0xFC00
#define RLBMASK      0x4000

#if defined(UNICODE)
#define DBSTAR_UNICODE_FLAG 1
#else
#define DBSTAR_UNICODE_FLAG 0
#endif

/* creation timestamp in rec hdr */
#define RECCRTIME    (sizeof(short) + DB_ADDR_SIZE)

/* update timestamp in rec hdr */
#define RECUPTIME    (RECCRTIME + sizeof(long))

/* set pointer structure definition */
typedef struct
{
    F_ADDR  total;             /* total number of members in set */
    DB_ADDR first;             /* database address of first member in set */
    DB_ADDR last;              /* database address of last member in set */
    DB_ULONG timestamp;        /* set update timestamp - if used */
}  SET_PTR;

/* member pointer structure definition */
typedef struct
{
    DB_ADDR owner;             /* database address of owner record */
    DB_ADDR prev;              /* database address of previous member in set */
    DB_ADDR next;              /* database address of next member in set */
}  MEM_PTR;

/* max size of set pointer */
/* member count + prior + next + timestamp (opt) */
#define SETPSIZE  (sizeof(SET_PTR))
#define SP_MEMBERS 0                       /* Offset to total members in set ptr */
#define SP_FIRST   sizeof(long)            /* Offset to first member ptr in set ptr */
#define SP_LAST    (SP_FIRST+DB_ADDR_SIZE) /* Offset to last member ptr in set ptr */

#define SP_UTIME   (SP_LAST+DB_ADDR_SIZE)  /* Offset to timestamp in set ptr */

#define MEMPSIZE   (sizeof(MEM_PTR))       /* Size of member pointer = 3*DB_ADDR_SIZE */
#define MP_OWNER   0                       /* Offset to owner ptr in member ptr */
#define MP_PREV    (MP_OWNER+DB_ADDR_SIZE) /* Offset to previous member ptr in member ptr */
#define MP_NEXT    (MP_PREV+DB_ADDR_SIZE)  /* Offset to next member ptr in member ptr */

/* maximum length of a task->dbuserid (including null) */
#define USERIDLEN       16

/* maximum length of a database name (base file w/o path w/o extension) */
#define DBNMLEN         32
/*
    DB_PATHLEN = maxFullPath    - maxFile     + null
                  = ((FILENMLEN-1) - (DBNMLEN-1) + 1)
*/
#define DB_PATHLEN      (FILENMLEN - DBNMLEN - 1)

/* LMC transport name length in TAF file */
#define LMCTYPELEN      12

#define NAMELEN         32       /* max record/set/field name + null */
#define MAXKEYLEN       256      /* if changed, qdefns.h needs updating */

#define MAXKEYSIZE      (MAXKEYLEN * sizeof(DB_TCHAR))

#ifndef INTERNAL_H

#define DEFIXPAGES      5        /* default number of index cache pages */
#define DEFDBPAGES      100      /* default number of database cache pages */
#define MINIXPAGES      3        /* minimum number of index cache pages */
#define MINDBPAGES      5        /* minimum number of database cache pages */
#define LENVDBID        48

#endif /* INTERNAL_H */

#define MAXDIMS         3

#define OPEN            'o'
#define CLOSED          'c'
#define DATA            'd'
#define KEY             'k'
#define DB_OVERFLOW     'o'

/* virtual page table entry */
typedef struct page_entry
{
    FILE_NO     file;             /* file table entry number (0..task->size_ft-1) */
    F_ADDR      pageno;           /* database page number */
    F_ADDR      ovfl_addr;        /* overflow file address of page */
    struct page_entry *prev;      /* prev hash page in bucket chain */
    struct page_entry *next;      /* next hash page bucket chain */
    struct page_entry *p_pg;      /* prev page for file */
    struct page_entry *n_pg;      /* next page for file */
    short       tag;              /* page tag */
    short       recently_used;
    DB_BOOLEAN  modified;         /* TRUE if page has been modified */
    short       holdcnt;          /* "hold-in-cache" counter */
    short       buff_size;        /* size of page buffer */
    char       *buff;             /* page buffer pointer */
} PAGE_ENTRY;

typedef struct page_entry *LOOKUP_ENTRY;

#define   V3_FILENMLEN  48

typedef struct V300_FILE_ENTRY_S
{
    char  v3_ft_name[V3_FILENMLEN];     /* name of file */
    short v3_ft_desc;                   /* file descriptor */
    char  v3_ft_status;                 /* 'o'=opened, 'c'=closed */
    char  v3_ft_type;                   /* 'd'=data, 'k'=key, 'o'=overflow */
    short v3_ft_slots;                  /* record slots per page */
    short v3_ft_slsize;                 /* size of record slots in bytes */
    short v3_ft_pgsize;                 /* size of page */
    short v3_ft_flags;                  /* e.g. STATIC */
} V300_FILE_ENTRY;

typedef struct V301_FILE_ENTRY_S
{
    char  v3_ft_name[V3_FILENMLEN];     /* name of file */
    short v3_ft_desc;                   /* file descriptor */
    char  v3_ft_status;                 /* 'o'=opened, 'c'=closed */
    char  v3_ft_type;                   /* 'd'=data, 'k'=key, 'o'=overflow */
    short v3_ft_slots;                  /* record slots per page */
    short v3_ft_slsize;                 /* size of record slots in bytes */
    short v3_ft_pgsize;                 /* size of page */
    short v3_ft_flags;                  /* e.g. STATIC */
    short v3_ft_pctincrease;            /* % increase of each extension */   
    long  v3_ft_initial;                /* initial allocation in pages */
    long  v3_ft_next;                   /* initial extension in pages */
} V301_FILE_ENTRY;

typedef struct V500_FILE_ENTRY_S
{
    char  v5_ft_name[FILENMLEN];        /* name of file */
    long  v5_ft_desc;                   /* file descriptor */
    long  v5_ft_index;                  /* see FILE_ENTRY */
    char  v5_ft_status;                 /* 'o'=opened, 'c'=closed */
    char  v5_ft_type;                   /* 'd'=data, 'k'=key, 'o'=overflow */
    short v5_ft_slots;                  /* record slots per page */
    short v5_ft_slsize;                 /* size of record slots in bytes */
    short v5_ft_pgsize;                 /* size of page */
    short v5_ft_flags;                  /* e.g. STATIC */
    short v5_ft_pctincrease;            /* % increase of each extension */   
    long  v5_ft_initial;                /* initial allocation in pages */
    long  v5_ft_next;                   /* initial extension in pages */
    short v5_ft_recently_used;
} V500_FILE_ENTRY;

typedef struct U500_FILE_ENTRY_S
{
    wchar_t v5_ft_name[FILENMLEN];        /* name of file */
    long    v5_ft_desc;                   /* file descriptor */
    long    v5_ft_index;                  /* see FILE_ENTRY */
    char    v5_ft_status;                 /* 'o'=opened, 'c'=closed */
    char    v5_ft_type;                   /* 'd'=data, 'k'=key, 'o'=overflow */
    short   v5_ft_slots;                  /* record slots per page */
    short   v5_ft_slsize;                 /* size of record slots in bytes */
    short   v5_ft_pgsize;                 /* size of page */
    short   v5_ft_flags;                  /* e.g. STATIC */
    short   v5_ft_pctincrease;            /* % increase of each extension */   
    long    v5_ft_initial;                /* initial allocation in pages */
    long    v5_ft_next;                   /* initial extension in pages */
    short   v5_ft_recently_used;
} U500_FILE_ENTRY;

typedef struct FILE_ENTRY_S
{
    DB_TCHAR ft_name[FILENMLEN]; /* name of file */
    PSP_FH   ft_desc;            /* file descriptor */
    long     ft_index;           /* Only used if SHARED_FILE_HANDLES
                                    is defined, on Win32 systems - 
                                    index into File_desc_table (array
                                    of shared file descriptors). On
                                    ANSI I/O systems ft_desc is used,
                                    but Win32 native I/O file handles
                                    may have very large values */
    char     ft_status;          /* 'o'=opened, 'c'=closed */
    char     ft_type;            /* 'd'=data, 'k'=key, 'o'=overflow */
    short    ft_slots;           /* record slots per page */
    short    ft_slsize;          /* size of record slots in bytes */
    short    ft_pgsize;          /* size of page */
    short    ft_flags;           /* e.g. STATIC */
    short    ft_pctincrease;     /* % increase of each extension */   
    long     ft_initial;         /* initial allocation in pages */
    long     ft_next;            /* initial extension in pages */
    int      ft_lmtimestamp;     /* file timestamp from lock mgr */
    int      ft_cachetimestamp;  /* previous file timestamp from lock mgr */
} FILE_ENTRY;

typedef struct RECORD_ENTRY_S
{
    short rt_file;    /* file table entry of file containing record */
    short rt_len;     /* total length of record */
    short rt_data;    /* offset to start of data in record */
    short rt_fields;  /* first field def in task->field_table */
    short rt_fdtot;   /* total number of fields in record */
    short rt_flags;
} RECORD_ENTRY;

#define FIRST      'f'
#define LAST       'l'
#define NEXT       'n'

#define ASCENDING  'a'
#define DESCENDING 'd'
#define NOORDER    'n'

typedef struct SET_ENTRY_S
{
    short st_order;      /* 'f'=first, 'l'=last, 'a'=ascending,
                            'd'=descending, 'n'=no order */
    short st_own_rt;     /* record table entry of owner */
    short st_own_ptr;    /* offset to set pointers in record */
    short st_members;    /* index of first member record in member table */
    short st_memtot;     /* total number of members of set */
    short st_flags;      /* 0x0001 is set if record is timestamped */
} SET_ENTRY;


typedef struct MEMBER_ENTRY_S
{
    short mt_record;           /* record table entry for this member */
    short mt_mem_ptr;          /* offset to member ptrs in record */
    short mt_sort_fld;         /* sort table entry of first sort field */
    short mt_totsf;            /* total number of sort fields */
} MEMBER_ENTRY;


typedef struct SORT_ENTRY_S
{
    short se_fld;              /* field table entry of sort field */
    short se_set;              /* set table entry of sorted set */
}  SORT_ENTRY;

/* kinds of keys */
#define NOKEY      'n'
#define DUPLICATES 'd'
#define UNIQUE     'u'

/* kinds of fields */
#define FLOAT      'f'
#define DOUBLE     'F'
#define CHARACTER  'c'
#define WIDECHAR   'C'
#define SHORTINT   's'
#define REGINT     'i'
#define LONGINT    'l'
#define DBADDR     'd'
#define GROUPED    'g'
#define COMKEY     'k'

typedef struct FIELD_ENTRY_S
{
    char  fd_key;           /* see the #defines just above */
    unsigned char fd_type;  /* see the #defines just above */
    short fd_len;           /* length of field in bytes */
    short fd_dim[MAXDIMS];  /* size of each array dimension */
    short fd_keyfile;       /* task->file_table entry for key file */
    short fd_keyno;         /* key prefix number */
    short fd_ptr;           /* offset to field in record or 1st compound key field in task->key_table */
    short fd_rec;           /* record table entry of record containing field */
    short fd_flags;         /* see the #defines at the top of the file */
} FIELD_ENTRY;

/* compound key table entry declaration */
typedef struct KEY_ENTRY_S
{
    short kt_key;              /* compound key field number */
    short kt_field;            /* field number of included field */
    short kt_ptr;              /* offset to start of field data in key */
    short kt_sort;             /* 'a' = ascending, 'd' = descending */
} KEY_ENTRY;


/* database table entry declaration */
typedef struct DB_ENTRY_S
{
    /* NOTE: db_name, db_path and ft_offset defined in TASK if ONE_DB */
    short Size_ft;             /* size of this db's task->file_table */
    short ft_offset;           /* offset to this db's task->file_table entries */
    short Size_rt;             /* size of this db's task->record_table */
    short rt_offset;           /* offset to this db's task->record_table entries */
    short Size_fd;             /* size of this db's task->field_table */
    short fd_offset;           /* offset to this db's task->field_table entries */
    short Size_st;             /* size of this db's task->set_table */
    short st_offset;           /* offset to this db's task->set_table entries */
    short Size_mt;             /* size of this db's task->member_table */
    short mt_offset;           /* offset to this db's task->member_table entries */
    short Size_srt;            /* size of this db's task->sort_table */
    short srt_offset;          /* offset to this db's task->sort_table entries */
    short Size_kt;             /* size of this db's task->key_table */
    short kt_offset;           /* offset to this db's key table entries */
    short key_offset;          /* key prefix offset for this db */
    DB_ADDR  sysdba;           /* database address of system record */
    DB_ADDR  curr_dbt_rec;     /* this db's current record */
    DB_ULONG curr_dbt_ts;      /* this db's current record timestamp */
    short Page_size;           /* size of this db's page */
    DB_TCHAR *Objnames;        /* buffer for rec, fld & set names */
    DB_TCHAR db_path[DB_PATHLEN]; /* name of path to this database */
    DB_TCHAR db_name[DBNMLEN];    /* name of this database */
    short db_ver;              /* version of dbd file */
} DB_ENTRY;

/* Structure containing current record type & address for
    recfrst/set/next.. NOTE: rn_type and rd_dba defined in TASK when ONE_DB.
*/
typedef struct RN_ENTRY_S
{
    short rn_type;    /* Last record type supplied to recfrst/recset */
    DB_ADDR  rn_dba;     /* Last db addr computed by recfrst/recset/recnext */
} RN_ENTRY;

#define  DB_REF(item)            (task->curr_db_table->item)
#define  RN_REF(item)            (task->curr_rn_table->item)
#define  NUM2INT(num, offset)    ((num) + task->curr_db_table->offset)
#define  NUM2EXT(num, offset)    ((num) - task->curr_db_table->offset)
#define  ORIGIN(offset)          (task->curr_db_table->offset)

#ifndef INTERNAL_H

#define  TABLE_SIZE(size)        DB_REF(size)

#endif /* INTERNAL_H */

/* Country code table definition */
typedef struct CNTRY_TBL_S
{
    unsigned char out_chr;
    unsigned char sort_as1;
    unsigned char sort_as2;
    unsigned char sub_sort;
} CNTRY_TBL;

#ifndef INTERNAL_H

#define DB_CLOSE  close
#define DB_READ   read
#define DB_WRITE  write
#define DB_LSEEK lseek


#define DB_TELL(fd)   DB_LSEEK(fd, 0, SEEK_CUR)

/* File stream functions used in dbedit */
#define DB_FTELL ftell
#define DB_FSEEK fseek

#endif /* INTERNAL_H */

struct sk
{
    char *sk_val;
    short sk_fld;
};

/* node key search path stack entry: one per level per key field */
typedef struct NODE_PATH_S
{
    F_ADDR node;          /* node (page) number  */
    short slot;           /* slot number of key */
} NODE_PATH;

/* index key information: one entry per key field */
typedef struct KEY_INFO_S
{
    short level;          /* current level # in node path */
    short max_lvls;       /* maximum possible levels for key */
    short lstat;          /* last key function status */
    short fldno;          /* field number of key */
    FILE_NO keyfile;      /* key file containing this key */
    DB_ADDR dba;          /* db address of last key */
    char *keyval;         /* ptr to last key value */
    NODE_PATH *node_path; /* stack of node #s in search path */
} KEY_INFO;

/* index file node structure */
typedef struct
{
    time_t   last_chgd;   /* date/time of last change of this node */
    short used_slots;     /* current # of used slots in node */
    char     slots[1];    /* start of slot area */
}  NODE;

/* key slot structure */
typedef struct key_slot
{
    F_ADDR child;         /* child node pointer */
    short keyno;          /* key number */
    char data[1];         /* start of key data */
} KEY_SLOT;

typedef union key_type
{
    char kd[MAXKEYSIZE];  /* This makes KEY_TYPE the correct size */
    struct key_slot   ks;
} KEY_TYPE;

/* file rename table entry declarations */
typedef struct ren_entry
{
    DB_TCHAR *file_name;
    DB_TCHAR *ren_db_name;
    FILE_NO   file_no;
} REN_ENTRY;

#ifndef INTERNAL_H

typedef struct
{
    DB_TCHAR *strbuf;
    size_t    buflen;
    size_t    strlen;
} DB_STRING;

DB_TCHAR *STRinit(DB_STRING *, DB_TCHAR *, size_t);
DB_TCHAR *STRcpy(DB_STRING *, DB_TCHAR *);
DB_TCHAR *STRcat(DB_STRING *, DB_TCHAR *);
DB_TCHAR *STRccat(DB_STRING *, int);
size_t    STRavail(DB_STRING *);

#ifdef DB_TRACE

#define API_ENTER(fn)   {if (task->db_trace & TRACE_API) api_enter(fn, task);}
#define API_RETURN(c)   return((task->db_trace & TRACE_API) ? api_exit(task) : 0, (c))
#define API_EXIT()      {if (task->db_trace & TRACE_API) api_exit(task);}

#define FN_ENTER(fn)    {if (task->db_trace & TRACE_API) fn_enter(fn , task);}
#define FN_RETURN(c)    return((task->db_trace & TRACE_API) ? fn_exit(task) : 0, (c))
#define FN_EXIT()       {if (task->db_trace & TRACE_API) fn_exit(task);}

#else  /* DB_TRACE */

#define API_ENTER(fn)   /**/
#define API_RETURN(c)   return(c)
#define API_EXIT()      /**/

#define FN_ENTER(fn)    /**/
#define FN_RETURN(c)    return(c)
#define FN_EXIT()       /**/

#endif /* DB_TRACE */

#define ENCODE_DBA(file, slot, dba) (*(dba)=(((F_ADDR)(file)&FILEMASK)<<FILESHIFT)|\
                                    ((F_ADDR)(slot)&ADDRMASK))
#define DECODE_DBA(dba, file, slot) {*(file)=(short)(((F_ADDR)(dba)>>FILESHIFT)&FILEMASK);\
                                    *(slot)=(F_ADDR)(dba)&ADDRMASK;}

#endif /* INTERNAL_H */

#define DB_TIMEOUT     10     /* lock request wait 10 seconds in queue */

#define NET_TIMEOUT    3      /* how long to wait for connection */

/* record/set lock descriptor */
typedef struct lock_descr
{
    DB_TCHAR        fl_type;       /* 'r'ead, 'w'rite, e'x'clusive, 'f'ree */
    DB_TCHAR        fl_prev;       /* previous lock type */
    FILE_NO        *fl_list;       /* array of files used by record/set */
    int             fl_cnt;        /* Number of elements in fl_list */
    DB_BOOLEAN      fl_kept;       /* Is lock kept after transaction? */
} LOCK_DESCR;

/*
    Maximum number of transactions which can commit at a time. For
    non-Unicode implementations, keep the TAF file size under 512
    bytes. For Unicode this won't be enough - make it 1024 or 2048
    bytes. On Win32, a wchar_t is 2 bytes, on Unix it's 4 bytes.

    TAFLIMIT = (512 * sizeof(wchar_t) - 3 * sizeof(short) -
               ((LOCKMGRLEN + LMCTYPELEN) * sizeof(wchar_t))) /
               (LOGFILELEN * sizeof(wchar_t)

    This will yield a file size of 512 * sizeof(wchar_t) or smaller
*/

#define TAFLIMIT 9

/* structure to read TAF data into - size must be same for Unicode and ANSI */
typedef struct TAFFILE_S
{
    short    user_count;
    short    cnt;
    short    unicode;
    DB_TCHAR lmc_type[LMCTYPELEN];
    DB_TCHAR lockmgrn[LOCKMGRLEN];
    DB_TCHAR files[TAFLIMIT][LOGFILELEN];
}  TAFFILE;

typedef struct ll_elem
{
    struct ll_elem *next;
    char           *data;
}  ll_elem;

#define LL_ELEM_INIT()  { NULL, NULL }

typedef struct
{
    ll_elem *head;
    ll_elem *tail;
    ll_elem *curr;
}  llist;

#define LLIST_INIT()    { NULL, NULL, NULL }

#define NO_SEEK      ((off_t)-1)

#ifdef DBSTAT
#include "dbstat.h"     /* must be before dbxtrn.h */
#endif

#include "trxlog.h"

#ifndef MSGVER
#define MSGVER  0x500
#endif

#include "dblock.h"     /* must be before dbxtrn.h */


#if defined(TASK_DEFN)
#include "dbxtrn.h"

#ifndef INTERNAL_H
#include "proto.h"
#endif /* INTERNAL_H */
#endif

#include "apidefs.h"

#ifdef QNX

#define LOCK_FCN   LK_LOCK
#define UNLOCK_FCN LK_UNLCK

#else /* QNX */

#define UNLOCK_FCN 0
#define LOCK_FCN   1

#endif /* QNX */

#define READ_FCN   0
#define WRITE_FCN  1

#endif /* DBTYPE_H */


