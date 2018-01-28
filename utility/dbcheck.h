/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbcheck utility                                   *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* node number of root */
#define ROOT_ADDR 1L

/* null node pointer */
#define NULL_NODE ((F_ADDR)-1)

/* record with problems */
#define BAD_REC   (char *)1
#define BAD_STMP  (char *)1

/* make strings plural */
#define PLURAL(n, s, p)  ((n == 1) ? s : p)
#define PLULET(n)        PLURAL(n, DB_TEXT(""), DB_TEXT("s"))

/* offsets into record slots */
#define REC_RID      0
#define REC_DBA      (REC_RID + sizeof(short))
#define REC_STMP     (REC_DBA + sizeof(long))
#define REC_KEY(ts)  (REC_STMP + ((ts) ? 2*sizeof(long) : 0))

/* offsets into nodes */
#define NODE_TS      0
#define NODE_SLCNT   (NODE_TS + sizeof(long))
#define NODE_SLOTS   (NODE_SLCNT + sizeof(short))

/* offsets into key slots */
#define SLOT_CHILD   0
#define SLOT_KEYNO   (SLOT_CHILD + sizeof(F_ADDR))
#define SLOT_KEY     (SLOT_KEYNO + sizeof(short))
#define SLOT_DBA(kl) (SLOT_KEY + kl)

/* locate bits in long bit masks */
#define BITS_TO_BYTES(bits) (((bits) - 1) / BITS_PER_BYTE + 1)
#define BYTE_NO(bit)        ((bit) / BITS_PER_BYTE)
#define BIT_NO(bit)         ((bit) % BITS_PER_BYTE)


/* dbcheck command line arguments / options */
typedef struct _CHKOPTS
{
    DB_BOOLEAN     setscan;
    DB_BOOLEAN     keyscan;
    DB_BOOLEAN     datkey;
    DB_BOOLEAN     keydat;
    DB_BOOLEAN     timestmp;
    DB_BOOLEAN     ignorekey;
    short          report;
    short          numpages;
    short          numfiles;
    DB_BOOLEAN     treetrace;
    DB_BOOLEAN     counts;

    DB_TCHAR      *dbname;              /* database name */
    DB_TCHAR     **fnames;              /* list of file names to check */
    short          nfnames;             /* number of files in list */

    SG            *sg;

}  CHKOPTS;

/* set number and offset information */
typedef struct _SET_INFO
{
    short    setno;                  /* The number of the set             */
    short    setoff;                 /* The offset of the set information */
    short    sbfoff;                 /* The offset of the cached set      */
    short    ownrid;                 /* The record id of the set owner    */
    short    memoff;                 /* The offset of the member info     */
    short    mbfoff;                 /* The offset of the cached member   */
    DB_BOOLEAN  stampd;              /* Whether the set is timestamped    */
}  SET_INFO;

/* record's set information */
typedef struct _REC_SET_INFO
{
    short    cnt;                    /* Count of sets for record type */
    short    byt;                    /* Number of bytes to represent sets */
    SET_INFO   *lst;                 /* List of sets for record type  */
}  REC_SET_INFO;

/* record's key information */
typedef struct _REC_KEY_INFO
{
    short    cnt;                    /* Count of keys for record type     */
    short    byt;                    /* Number of bytes to represent keys */
    short   *lst;                    /* List of key fields for record     */
}  REC_KEY_INFO;

/* set pointers */
typedef struct _SET
{
    long        members;
    DB_ADDR     first;
    DB_ADDR     last;
}  SET;

/* member pointers */
typedef struct _MEMBER
{
    DB_ADDR     owner;
    DB_ADDR     prev;
    DB_ADDR     next;
}  MEMBER;

/* record check information */
typedef struct _CHKREC
{
    short       dba_msg,
                del_msg,
                lock_msg,
                rid_msg,
                idba_msg;
    DB_BOOLEAN  force;
    char       *pgptr;
    short       rid;
    DB_BOOLEAN  del,
                lck;
    DB_BOOLEAN  good;
}  CHKREC;

/* record header information */
typedef struct _RECINFO
{
    short       rid;
    DB_BOOLEAN  del,
                lck;
}  RECINFO;

/* slot information */
typedef struct _SLINFO
{
    short       levl;
    NODE       *node;
    char       *sl_addr;
    short       slot;
    DB_BOOLEAN  last;
    DB_BOOLEAN  all_null;
    F_ADDR      child;
    short       keyno;
    short       keyfd;
    short       keyln;
    DB_ADDR     dba;
    FIELD_ENTRY *kf_ptr;
}  SLINFO;

/* information about current file / slot for error reporting */
typedef struct _ERRINFO
{
    FILE_NO fno;
    F_ADDR  addr;
    F_ADDR  err_addr;
    long    tot_rec_err;
    long    tot_err_cnt;
}  ERRINFO;

/* database structure information */
typedef struct _DBINFO
{
    short         num_recs;             /* Number of elements in 3 arrays */
    REC_SET_INFO *owners;               /* 3 arrays, by record type, giving */
    REC_SET_INFO *members;              /* sets owned by and owning record, */
    REC_KEY_INFO *keys;                 /* and key fields in it             */
    short         num_keys;             /* Number of key fields in database */
    short        *key_fld;              /* 2 arrays, by key prefix, giving  */
    short        *key_len;              /* field table index and key length */
    F_ADDR       *rec_cnt;              /* Count of record instances checked */
    F_ADDR       *set_cnt;              /* Count of set members checked */
    F_ADDR       *key_cnt;              /* Count of key slots checked */
    F_ADDR       *tops;                 /* Nextslot values from each file */
    long          num_del;              /* Deleted slots / nodes, current file */
    PAGE_CACHE   *pcache;               /* Cache for dchain & key use bitmaps */
    CHKOPTS      *opts;                 /* Command line args / options */
    ERRINFO      *errinfo;              /* Number of errors in database */
}  DBINFO;

/* page cache information */
typedef struct _BTCACHE
{
    F_ADDR      addr;                         /* The file address */
    char       *pgbuf;                        /* The page data    */
}  BTCACHE;


/* error information structures */
typedef struct _ERR_PDBA
{
    short       setno;
    DB_ADDR     dba;
}  ERR_PDBA;

typedef struct _ERR_STYP
{
    short       setno;
    DB_ADDR     dba;
    short       rid;
}  ERR_STYP;

typedef struct _ERR_SCNT
{
    short       setno;
    F_ADDR      owncnt;
    F_ADDR      memcnt;
}  ERR_SCNT;

typedef struct _ERR_LEVL
{
    short       levl;
    short       mlvl;
}  ERR_LEVL;

typedef struct _ERR_PGZERO
{
    short       file;
    DB_ULONG    slot;
}  ERR_PGZERO;

typedef struct _ERR_KEYDUPES
{
    DB_ULONG    count;
    DB_ADDR     dba;
    DB_ADDR     frstdba;
    char *      buf;
    DB_BOOLEAN  frst_slot;
    DB_BOOLEAN  in_series;
    short       fldno;
}  ERR_KEYDUPES;

typedef struct _ERR_FSIZE
{
    DB_TCHAR   *fname;
    short       fsize;
}  ERR_FSIZE;

/* information about the key file currently being checked */
typedef struct _KEYINFO
{
    char        oldkey[MAXKEYSIZE];
    short       oldno;
    short       slot_len;
    DB_ADDR     olddba;
    short       max_levl;
    short       null_levl;
    long        btree_pages;
    ERR_KEYDUPES kdup;
}  KEYINFO;

#define ERS_ERR   0
#define ERS_WARN  1

#define ERT_NULL        0
#define ERT_SHORT       1
#define ERT_FADR        2
#define ERT_DBA         3
#define ERT_SETS        4
#define ERT_SPTR        5
#define ERT_SMEM        6
#define ERT_SMTP        7
#define ERT_SCNT        8
#define ERT_SOWN        9
#define ERT_SOTP        11
#define ERT_KINF        12
#define ERT_SINM        13
#define ERT_SINF        14
#define ERT_SDBA        15
#define ERT_SLCH        16
#define ERT_LEVL        17
#define ERT_SLIN        18
#define ERT_SYSTEM      19
#define ERT_PGZERO      20
#define ERT_UKNU        21
#define ERT_SIZE        22

#define BAD_LOCK        1
#define BAD_RID         2
#define BAD_RDBA        3
#define BAD_RSTP        4
#define BAD_SSTP        5
#define BAD_OFST        6
#define BAD_OLST        7
#define BAD_OMEM        8
#define BAD_OMDL        9
#define BAD_OMRC        10
#define BAD_OMTP        11
#define BAD_LOOP        12
#define BAD_MFPN        13
#define BAD_MLNN        14
#define BAD_MNPW        15
#define BAD_OMOW        16
#define BAD_OCNT        17
#define BAD_MOPB        18
#define BAD_MODL        19
#define BAD_MORC        20
#define BAD_MOIN        21
#define BAD_MONL        22
#define BAD_MNPP        23
#define BAD_MNNP        24
#define BAD_MOKEY       25
#define BAD_OKEY        26
#define BAD_MKEY        27
#define BAD_SLCT        28
#define BAD_KWDL        29
#define BAD_KEYP        30
#define BAD_KPFL        31
#define BAD_KDBA        32
#define BAD_KDAD        33
#define BAD_KREC        34
#define BAD_KDAT        35
#define BAD_TRLV        36
#define BAD_CHSM        37
#define BAD_CHSL        38
#define BAD_CHPT        39
#define BAD_BTRC        40
#define BAD_BTUO        41
#define BAD_BTDD        42
#define BAD_KPST        43
#define BAD_KBTL        44
#define BAD_DCHN        45
#define BAD_KCNDL       46
#define BAD_DCNDL       47
#define BAD_KDLNC       48
#define BAD_DDLNC       49
#define BAD_NCLP        50
#define BAD_RCLP        51
#define BAD_NNEN        52
#define BAD_DIOR        53
#define BAD_DIOG        54
#define BAD_SYSR        55
#define BAD_NEXTSLOT    56
#define BAD_DCHAIN      57
#define BAD_UKNU        58
#define BAD_SETP        59
#define BAD_SETN        60
#define BAD_OMPP        61
#define BAD_SIZE        62

#ifdef ERR_DEFINE
struct _ERR_DEFINE
{
    short type;
    short severity;
    DB_TCHAR *text;
}  chk_errs[] = {
/* BAD_LOCK */  ERT_NULL,     ERS_ERR,  DB_TEXT("record's lock bit is set\n"),
/* BAD_RID */   ERT_SHORT,    ERS_ERR,  DB_TEXT("record's id-number=%d is out of range or inappropriate for its file\n"),
/* BAD_RDBA */  ERT_DBA,      ERS_ERR,  DB_TEXT("record's dba is inconsistent: %s\n"),
/* BAD_RSTP */  ERT_NULL,     ERS_ERR,  DB_TEXT("record's timestamp(s) are invalid\n"),
/* BAD_SSTP */  ERT_SETS,     ERS_ERR,  DB_TEXT("set's timestamp is invalid\n"),
/* BAD_OFST */  ERT_SPTR,     ERS_ERR,  DB_TEXT("invalid first-pointer"),
/* BAD_OLST */  ERT_SPTR,     ERS_ERR,  DB_TEXT("invalid last-pointer"),
/* BAD_OMEM */  ERT_SPTR,     ERS_ERR,  DB_TEXT("member's next-pointer is invalid"),
/* BAD_OMDL */  ERT_SMEM,     ERS_ERR,  DB_TEXT("member record is deleted"),
/* BAD_OMRC */  ERT_SMEM,     ERS_ERR,  DB_TEXT("member record has invalid record-id and/or inconsistent dba"),
/* BAD_OMTP */  ERT_SMTP,     ERS_ERR,  DB_TEXT("member's record-id=%d is invalid member type for this set"),
/* BAD_LOOP */  ERT_SMEM,     ERS_ERR,  DB_TEXT("set has more members than it should; terminating scan at"),
/* BAD_MFPN */  ERT_SMEM,     ERS_ERR,  DB_TEXT("first member does not have NULL previous pointer"),
/* BAD_MLNN */  ERT_SMEM,     ERS_ERR,  DB_TEXT("last member does not have NULL next pointer"),
/* BAD_MNPW */  ERT_SMEM,     ERS_ERR,  DB_TEXT("member's previous-pointer does not point to previous member"),
/* BAD_OMOW */  ERT_SMEM,     ERS_ERR,  DB_TEXT("member's owner-pointer does not point to correct owner"),
/* BAD_OCNT */  ERT_SCNT,     ERS_ERR,  DB_TEXT("incorrect member-count:  owner's-count=%ld  actual=%ld\n"),
/* BAD_MOPB */  ERT_SOWN,     ERS_ERR,  DB_TEXT("member's owner-pointer is invalid"),
/* BAD_MODL */  ERT_SOWN,     ERS_ERR,  DB_TEXT("member's owner record is deleted"),
/* BAD_MORC */  ERT_SOWN,     ERS_ERR,  DB_TEXT("member's owner record invalid record-id and/or inconsistent dba"),
/* BAD_MOIN */  ERT_SOTP,     ERS_ERR,  DB_TEXT("member's owner record-id=%d is invalid owner type for this set"),
/* BAD_MONL */  ERT_SETS,     ERS_ERR,  DB_TEXT("owner-pointer is null, but next and previous are not\n"),
/* BAD_MNPP */  ERT_SETS,     ERS_ERR,  DB_TEXT("member has null previous-pointer but is not first member\n"),
/* BAD_MNNP */  ERT_SETS,     ERS_ERR,  DB_TEXT("member has null next-pointer but is not last member\n"),
/* BAD_MOKEY */ ERT_KINF,     ERS_ERR,  DB_TEXT("missing optional key\n"),
/* BAD_OKEY */  ERT_KINF,     ERS_ERR,  DB_TEXT("optional key with missing record bit\n"),
/* BAD_MKEY */  ERT_KINF,     ERS_ERR,  DB_TEXT("has a missing key\n"),
/* BAD_SLCT */  ERT_SHORT,    ERS_ERR,  DB_TEXT("page's slot-count=%d is out of range\n"),
/* BAD_KWDL */  ERT_NULL,     ERS_ERR,  DB_TEXT("deleted page found on the b-tree\n"),
/* BAD_KEYP */  ERT_SINM,     ERS_ERR,  DB_TEXT("key-prefix=%d is out of range\n"),
/* BAD_KPFL */  ERT_SINM,     ERS_ERR,  DB_TEXT("key-prefix=%d does not belong in this file\n"),
/* BAD_KDBA */  ERT_SDBA,     ERS_ERR,  DB_TEXT("record-dba=%s is invalid\n"),
/* BAD_KDAD */  ERT_SDBA,     ERS_ERR,  DB_TEXT("record-dba=%s is deleted\n"),
/* BAD_KREC */  ERT_SDBA,     ERS_ERR,  DB_TEXT("record-dba=%s has invalid record-id and/or inconsistent dba\n"),
/* BAD_KDAT */  ERT_SDBA,     ERS_ERR,  DB_TEXT("data does not match record-dba=%s data\n"),
/* BAD_TRLV */  ERT_LEVL,     ERS_ERR,  DB_TEXT("b-tree level=%d is deeper than theoretical-maximum=%d\n"),
/* BAD_CHSM */  ERT_NULL,     ERS_ERR,  DB_TEXT("child pointers at this node are not either all null or all non-null\n"),
/* BAD_CHSL */  ERT_NULL,     ERS_ERR,  DB_TEXT("null child pointers are not all at the same b-tree level\n"),
/* BAD_CHPT */  ERT_SLCH,     ERS_ERR,  DB_TEXT("child-pointer=%ld is invalid\n"),
/* BAD_BTRC */  ERT_SLIN,     ERS_ERR,  DB_TEXT("b-tree traceback at level=%d\n"),
/* BAD_BTUO */  ERT_SLIN,     ERS_ERR,  DB_TEXT("b-tree is unordered; b-tree level=%d\n"),
/* BAD_BTDD */  ERT_SDBA,     ERS_ERR,  DB_TEXT("record-dba=%s has duplicate entries in the b-tree\n"),
/* BAD_KPST */  ERT_NULL,     ERS_ERR,  DB_TEXT("node is not referenced by either the b-tree or delete chain\n"),
/* BAD_KBTL */  ERT_NULL,     ERS_ERR,  DB_TEXT("node is the child of more than one parent\n"),
/* BAD_DCHN */  ERT_FADR,     ERS_ERR,  DB_TEXT("delete chain's next-pointer=%ld is out of range\n"),
/* BAD_KCNDL */ ERT_NULL,     ERS_ERR,  DB_TEXT("node is not deleted, but is on delete chain\n"),
/* BAD_DCNDL */ ERT_NULL,     ERS_ERR,  DB_TEXT("record is not deleted, but is on delete chain\n"),
/* BAD_KDLNC */ ERT_NULL,     ERS_ERR,  DB_TEXT("node is deleted, but is not on delete chain\n"),
/* BAD_DDLNC */ ERT_NULL,     ERS_ERR,  DB_TEXT("record is deleted, but is not on delete chain\n"),
/* BAD_NCLP */  ERT_NULL,     ERS_ERR,  DB_TEXT("node is at the start and end of a loop in delete chain\n"),
/* BAD_RCLP */  ERT_NULL,     ERS_ERR,  DB_TEXT("record is at the start and end of a loop in delete chain\n"),
/* BAD_NNEN */  ERT_NULL,     ERS_WARN, DB_TEXT("warning: non-null data after the logical end of file\n"),
/* BAD_DIOR */  ERT_DBA,      ERS_ERR,  DB_TEXT("unexpected error in dio_read of dba=%s\n"),
/* BAD_DIOG */  ERT_FADR,     ERS_ERR,  DB_TEXT("unexpected error in dio_get of page=%ld\n"),
/* BAD_SYSR */  ERT_SYSTEM,   ERS_ERR,  DB_TEXT("record-id says SYSTEM, only one SYSTEM record allowed\n"),
/* BAD_NXTSL */ ERT_PGZERO,   ERS_WARN, DB_TEXT("Possible bad next_slot    pointer (0x%08lX) in file # %u\n"),
/* BAD_DCHN */  ERT_PGZERO,   ERS_WARN, DB_TEXT("Possible bad delete_chain pointer (0x%08lX) in file # %u\n"),
/* BAD_UKNU */  ERT_UKNU,     ERS_ERR,  DB_TEXT("Field %s is defined as unique key,\n     but the above value is duplicated in data file at %s\n"),
/* BAD_SETP */  ERT_SOWN,     ERS_ERR,  DB_TEXT("previous record's next record does not point to current member"),
/* BAD_SETN */  ERT_SOWN,     ERS_ERR,  DB_TEXT("next record's previous record does not point to current member"),
/* BAD_OMPP */  ERT_SPTR,     ERS_ERR,  DB_TEXT("member's prev-pointer is invalid"),
/* BAD_SIZE  */ ERT_SIZE,     ERS_WARN, DB_TEXT("Size of field %s (%d bytes) in database is incorrect for its type\n")
};

#endif

/* function prototypes */
static int   parse_args(int, DB_TCHAR **, CHKOPTS **);
static void  usage(void);
static int   setup(char **, CHKOPTS *, DBINFO **, DB_TASK *);
static int   setup_db(char **, CHKOPTS *, DBINFO **, DB_TASK *);
static int   setup_keys(DBINFO *, DB_TASK *);
static int   setup_sets(DBINFO *, DB_TASK *);
static int   setup_tots(DBINFO *, DB_TASK *);
static int   setup_cnts(DBINFO *, DB_TASK *);
static int   setup_temp_files(DBINFO *, DB_TASK *);
static void  free_dbinfo(DBINFO *);
static int   scan(char *, DBINFO *, DB_TASK *);
static int   chk_rec(DB_ADDR, char *, char *, CHKREC *, DBINFO *, DB_TASK *);
static int   chk_key(char *, int, DB_ADDR, DBINFO *, DB_TASK *);
static void  chk_stmp(char *, int, FILE_NO, DB_BOOLEAN, DBINFO *, DB_TASK *);
static int   chk_own(char *, short, DB_ADDR, DBINFO *, DB_TASK *);
static int   chk_mem(char *, short, DB_ADDR, DBINFO *, DB_TASK *);
static int   key_file(FILE_NO, DBINFO *, DB_TASK *);
static int   walk_key(FILE_NO, KEYINFO *, DBINFO *, DB_TASK *);
static int   scan_key(FILE_NO, F_ADDR, short, KEYINFO *, DBINFO *, DB_TASK *);
static int   chk_node(FILE_NO, F_ADDR, short, KEYINFO *, DBINFO *, DB_TASK *);
static int   chk_slot(FILE_NO, KEYINFO *, SLINFO *, DBINFO *, DB_TASK *);
static int   scan_child(FILE_NO, F_ADDR, KEYINFO *, SLINFO *, DBINFO *, DB_TASK *);
static int   reg_page(F_ADDR, PAGE_CACHE *, long);
static int   chk_pglst(FILE_NO, DBINFO *, DB_TASK *);
static int   read_del_chain(FILE_NO, DBINFO *, DB_TASK *);
static int   chk_del_chain(F_ADDR, DB_BOOLEAN, DBINFO *, DB_TASK *);
static int   tio_get(FILE_NO, F_ADDR, char **, int, DBINFO *, DB_TASK *);
static int   tio_unget(FILE_NO, F_ADDR, int, DBINFO *, DB_TASK *);
static int   tio_read(DB_ADDR, char **, DBINFO *, DB_TASK *);
static int   chk_dba(DB_ADDR, DBINFO *, DB_TASK *);
static int   get_rid_info(char *, struct _RECINFO *);
static void  pr_err(int, void *, DBINFO *, DB_TASK *);
static DB_TCHAR *pr_dba(DB_ADDR *);
static int   stat_report(FILE_NO, F_ADDR, DBINFO *, DB_TASK *);
static int   pr_stat(int, DBINFO *);
static int   pr_counts(DBINFO *, DB_TASK *);
static int   data_file(FILE_NO, char *, DBINFO *, DB_TASK *);
static SET_INFO *val_mtype(int, short, REC_SET_INFO *, DB_TASK *);


