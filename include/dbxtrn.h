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

#ifndef DBXTRN_H
#define DBXTRN_H


typedef struct PAGE_TAG_S
{
    short    lru_page;     /* last lru page */
    short    num_pages;    /* number of pages currently in cache */
    DB_ULONG lookups;
    DB_ULONG hits;
} PAGE_TAG;

typedef struct PAGE_TABLE_S
{
    PAGE_ENTRY       *pg_table;   /* page table */
    short             pgtab_sz;   /* max pages to manage */
    LOOKUP_ENTRY     *lookup;     /* hash lookup table */
    short             lookup_sz;  /* number of buckets in hash */
#ifdef DBSTAT
    MEM_STATS         mem_stats;
    CACHE_STATS       cache_stats;
#endif
} PAGE_TABLE;

/* Define Cache table */
typedef struct CACHE_S
{
    PAGE_TABLE    db_pgtab;       /* Database page table */
    PAGE_TABLE    ix_pgtab;       /* Index page table */

    PAGE_TAG     *tag_table;
    short         max_tags;
    short         num_tags;

    DB_BOOLEAN    prealloc;

    short         largest_page;   /* size of largest page in pg_cache */
    char         *dbpgbuff;       /* allocated by dio_init used by o_update */
} CACHE;

/* Database Dictionary Tables */

#define LOCK_STACK_SIZE    10

typedef struct TASK_S
{
    int                  errnum;       /* primary error code */
    int                  err_ex;       /* secondary error code */
    int                  errno_c;      /* O/S error code */

    char                *errfile;      /* not used unless DBERR_EX is defined */
    int                  errline;

    CACHE               *cache;
    PAGE_ENTRY          *last_datapage;
    PAGE_ENTRY          *last_keypage;
    PAGE_ENTRY          *last_ixpage;
    short                db_tag;
    short                key_tag;
    short                ix_tag;
    DB_TCHAR             dbuserid[USERIDLEN];
    DB_TCHAR             dbdpath[FILENMLEN * 2];
    DB_TCHAR             dbfpath[FILENMLEN * 2];
    DB_TCHAR            *lockmgrn;
    int                  readlocksecs;
    int                  writelocksecs;
    DB_TCHAR             lm_addr[FILENMLEN];     /* lockmgr address */
    DB_TCHAR             dbtaf[FILENMLEN];
    DB_TCHAR             iniFile[FILENMLEN];
    DB_TCHAR            *dbtmp;
    DB_TCHAR             ctbpath[FILENMLEN];
    DB_TCHAR             trans_id[TRANS_ID_LEN];
    DB_TCHAR             dblog[FILENMLEN];
    CNTRY_TBL           *country_tbl;
    int                  ctbl_activ;
    int                  trlog_flag;             /* set only by user implemented fcns */
    int                  dbopen;
    short                inDBSTAR;
    short                trcommit;               /* are we in d_trend()? */
    PSP_FH               lfn;
    int                  no_of_keys;
    DB_ULONG             dboptflag;              /* which Dboptions have been explicitly set */
    DB_ULONG             dboptions;
    int                  ov_header_written;
    int                  ov_setup_done;
    int                  lock_lvl;
    char                *crloc;                  /* location in page buffer of current record */
    int                  lock_stack[LOCK_STACK_SIZE];
    DB_BOOLEAN           cache_ovfl;             /* must be here, not in db_cache */
    PGZERO              *pgzero;
    short                last_keyfld;
    KEY_INFO            *key_info;
    KEY_TYPE             key_type;
    off_t                ov_initaddr;
    off_t                ov_rootaddr;
    off_t                ov_nextaddr;
    RI_ENTRY            *root_ix;
    llist                ren_list;
    short                page_size;
    DB_ADDR              curr_rec;
    DB_ADDR             *curr_own;
    DB_ADDR             *curr_mem;
    FILE_NO              ov_file;
    FILE_ENTRY          *file_table;
    short                size_ft;
    RECORD_ENTRY        *record_table;
    short                size_rt;
    SET_ENTRY           *set_table;
    short                size_st;
    MEMBER_ENTRY        *member_table;
    short                size_mt;
    SORT_ENTRY          *sort_table;
    short                size_srt;
    FIELD_ENTRY         *field_table;
    short                size_fd;
    KEY_ENTRY           *key_table;
    short                size_kt;
    DB_TCHAR            *objnames;
    DB_TCHAR           **record_names;
    DB_TCHAR           **field_names;
    DB_TCHAR           **set_names;
    llist                sk_list;
    ERRORPROC            error_func;
    short                old_size_ft;
    short                old_size_fd;
    short                old_size_st;
    short                old_size_mt;
    short                old_size_srt;
    short                old_size_kt;
    short                old_size_rt;
    short                old_no_of_dbs;
    int                  curr_db;
    int                  set_db;
    short                no_of_dbs;
    DB_ENTRY            *curr_db_table;
    RN_ENTRY            *curr_rn_table;
    DB_ENTRY            *db_table;
    RN_ENTRY            *rn_table;
    DB_ULONG            *co_time;
    DB_ULONG            *cm_time;
    DB_ULONG            *cs_time;
    DB_ULONG             cr_time;
    DB_BOOLEAN           db_tsrecs;
    DB_BOOLEAN           db_tssets;
    int                  dbwait_time;
    int                  db_timeout;
    int                  db_lockmgr;
    int                  keyl_cnt;
    size_t               lp_size;
    size_t               fp_size;  
    int                 *app_locks;
    int                 *excl_locks;
    int                 *kept_locks;
    LOCK_DESCR          *rec_locks;
    LOCK_DESCR          *set_locks;
    LOCK_DESCR          *key_locks;
    LM_LOCK             *lock_pkt;
    LM_FREE             *free_pkt;
    FILE_NO             *file_refs;
    const SG           **sgs;
    void                *enc_buff;
    short                enc_size;
    DB_BOOLEAN           session_active;
    int                  cnt_open_files;         /* count of currently open files */
    int                  last_file;              /* least recently used file */
    int                  rlb_status;
    double               nap_factor;             /* lmcg sleep time */
    DB_TCHAR             type[2];                /* open type (s or x)--used by dopen() */

    int                  db_status;

#ifdef DBSTAT
    GEN_STATS            gen_stats;
    FILE_STATS          *file_stats;
    MSG_STATS            msg_stats[L_LAST+1];
#endif /* DBSTAT */

    DB_ULONG             db_trace;
    short                db_indent;
    DB_ULONG             db_debug;

    KEY_INFO            *curkey;
    FIELD_ENTRY         *cfld_ptr;
    int                  key_len;
    int                  key_data;
    int                  slot_len;
    short                max_slots;
    short                mid_slot;
    FILE_NO              keyfile;
    short                fldno;
    short                keyno;
    int                  unique;
    short                prefix;

    int                  struct_key_chk;

    PSP_LMC              lmc;
    DB_TCHAR             lmc_type[LMCTYPELEN];
} TASK, DB_TASK;

#ifndef INTERNAL_H

extern DB_TASK Orgtask;

#endif      /* INTERNAL_H */

#endif      /* DBXTRN_H */

