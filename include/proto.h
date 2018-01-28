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

#ifndef PROTO_H
#define PROTO_H

/* From dberr.c */
void      EXTERNAL_FCN dberr_msg(DB_TCHAR *, int, DB_TASK *);

#ifdef DBERR_EX
#define DBERR_FILE __FILE__
#define DBERR_LINE __LINE__
#else
#define DBERR_FILE NULL
#define DBERR_LINE 0
#endif

int  EXTERNAL_FCN _dberr(int, char *, int, DB_TASK *);
void EXTERNAL_FCN flush_dberr(DB_TASK *);
#define dberr(n)  _dberr(n, DBERR_FILE, DBERR_LINE, task)

/* From alloc.c: */

/* From dberr.c: */
void INTERNAL_FCN dbautorec(DB_TASK *);

/* From dblfcns.c: */
int INTERNAL_FCN initdbt(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN alloc_table(void **, size_t, size_t, DB_TASK *);
int INTERNAL_FCN free_table(void **, int, int, int, size_t, DB_TASK *);
int INTERNAL_FCN recovery_check(DB_TASK *);

/* From lockfcns.c: */
int  INTERNAL_FCN free_dblocks(int, DB_TASK *);

void INTERNAL_FCN termfree(int, DB_TASK *);
int  INTERNAL_FCN neterr(void);

int INTERNAL_FCN dupid_check(void);

#ifdef DBSTAT
/* From dbstat.c: */
void sync_MEM_STATS(MEM_STATS *, MEM_STATS *);
void sync_CACHE_STATS(CACHE_STATS *, CACHE_STATS *);

void INTERNAL_FCN STAT_mem_alloc(PAGE_TABLE *cache, size_t size);
void INTERNAL_FCN STAT_lookups(FILE_NO fno, DB_TASK *);
void INTERNAL_FCN STAT_hits(FILE_NO fno, DB_TASK *);
void INTERNAL_FCN STAT_pages(FILE_NO fno, short num, DB_TASK *);
void INTERNAL_FCN STAT_file_open(FILE_NO fno, DB_TASK *);
void INTERNAL_FCN STAT_pz_read(FILE_NO fno, size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_pz_write(FILE_NO fno, size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_pg_read(FILE_NO fno, size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_pg_write(FILE_NO fno, size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_rlb_read(FILE_NO fno, size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_rlb_write(FILE_NO fno, size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_new_page(FILE_NO fno, DB_TASK *);
void INTERNAL_FCN STAT_log_open(DB_TASK *);
void INTERNAL_FCN STAT_log_read(size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_log_write(size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_taf_open(DB_TASK *);
void INTERNAL_FCN STAT_taf_read(size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_taf_write(size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_dbl_open(DB_TASK *);
void INTERNAL_FCN STAT_dbl_read(size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_dbl_write(size_t amt, DB_TASK *);
void INTERNAL_FCN STAT_max_open(int num, DB_TASK *);
void INTERNAL_FCN STAT_trbegin(DB_TASK *);
void INTERNAL_FCN STAT_trend(DB_TASK *);
void INTERNAL_FCN STAT_trabort(DB_TASK *);
void INTERNAL_FCN STAT_lock(FILE_NO fno, int type, DB_TASK *);


/* Constants for use by STAT_lock() - they are offsets into the
    LOCK_STATS structure
*/
#define STAT_LOCK_r       0   /* r = read lock */
#define STAT_LOCK_w       1   /* w = write lock */
#define STAT_LOCK_x       2   /* x = exclusive lock */
#define STAT_LOCK_R       3   /* R = record lock bit */
#define STAT_LOCK_r2w     4
#define STAT_LOCK_r2x     5
#define STAT_LOCK_r2R     6
#define STAT_LOCK_w2r     7
#define STAT_LOCK_x2r     8
#define STAT_LOCK_R2r     9
#define STAT_FREE_r      10
#define STAT_FREE_w      11
#define STAT_FREE_x      12
#define STAT_FREE_R      13

#else
#define sync_MEM_STATS(a, b) /**/
#define sync_CACHE_STATS(a, b) /**/

#define STAT_mem_alloc(a, b) /**/
#define STAT_lookups(a, b) /**/
#define STAT_hits(a, b) /**/
#define STAT_pages(a, b, c) /**/
#define STAT_file_open(a, b) /**/
#define STAT_pz_read(a, b, c) /**/
#define STAT_pz_write(a, b, c) /**/
#define STAT_pg_read(a, b, c) /**/
#define STAT_pg_write(a, b, c) /**/
#define STAT_rlb_read(a, b, c) /**/
#define STAT_rlb_write(a, b, c) /**/
#define STAT_new_page(a, b) /**/
#define STAT_log_open(a) /**/
#define STAT_log_read(a, b) /**/
#define STAT_log_write(a, b) /**/
#define STAT_taf_open(a) /**/
#define STAT_taf_read(a, b) /**/
#define STAT_taf_write(a, b) /**/
#define STAT_dbl_open(a) /**/
#define STAT_dbl_read(a, b) /**/
#define STAT_dbl_write(a, b) /**/
#define STAT_max_open(a, b) /**/
#define STAT_trbegin(a) /**/
#define STAT_trend(a) /**/
#define STAT_trabort(a) /**/
#define STAT_lock(a, b, c) /**/
#define STAT_send_msg(a, b, c, d) /**/
#define STAT_recv_msg(a, b, c, d) /**/
#endif   /* DBSTAT */

/* From dio.c: */
int  EXTERNAL_FCN dio_open(FILE_NO, DB_TASK *);
int  INTERNAL_FCN dio_closelru(DB_TASK *);
int  EXTERNAL_FCN dio_close(FILE_NO, DB_TASK *);
int  INTERNAL_FCN dio_init(DB_TASK *);
void INTERNAL_FCN dio_free(int, DB_TASK *);
int  EXTERNAL_FCN dio_clear(int, DB_TASK *);
int  INTERNAL_FCN dio_clrfile(FILE_NO, DB_TASK *);
int  INTERNAL_FCN dio_clrpage(PAGE_ENTRY *, DB_TASK *);

void INTERNAL_FCN dio_ixclear(DB_TASK *);
int  EXTERNAL_FCN dio_flush(DB_TASK *);
int  EXTERNAL_FCN dio_get(FILE_NO, F_ADDR, char **, int, DB_TASK *);
int  INTERNAL_FCN dio_touch(FILE_NO, F_ADDR, int, DB_TASK *);
int  EXTERNAL_FCN dio_unget(FILE_NO, F_ADDR, int, DB_TASK *);
int  EXTERNAL_FCN dio_read(DB_ADDR, char **, int, DB_TASK *);
int  EXTERNAL_FCN dio_write(DB_ADDR, int, DB_TASK *);
int  EXTERNAL_FCN dio_release(DB_ADDR, int, DB_TASK *);

int  INTERNAL_FCN dio_rrlb(DB_ADDR, short *, DB_TASK *);
int  INTERNAL_FCN dio_wrlb(DB_ADDR, short, DB_TASK *);

int  INTERNAL_FCN dio_findpg(FILE_NO, F_ADDR, PAGE_ENTRY**, PAGE_ENTRY**, DB_TASK *);
int  EXTERNAL_FCN dio_getpg(FILE_NO, F_ADDR, short, PAGE_ENTRY**, PAGE_ENTRY**, DB_TASK *);

int  INTERNAL_FCN dio_out(PAGE_ENTRY *, DB_TASK *);
int  INTERNAL_FCN dio_in(PAGE_ENTRY *, DB_TASK *);

DB_ULONG INTERNAL_FCN dio_pzsetts(FILE_NO, DB_TASK *);
DB_ULONG INTERNAL_FCN dio_pzgetts(FILE_NO, DB_TASK *);

int EXTERNAL_FCN dio_pzinit(DB_TASK *);
int EXTERNAL_FCN dio_pzread(FILE_NO, DB_TASK *);
int EXTERNAL_FCN dio_pzalloc(FILE_NO, F_ADDR *, DB_TASK *);
int EXTERNAL_FCN dio_pzdel(FILE_NO, F_ADDR, DB_TASK *);
F_ADDR EXTERNAL_FCN dio_pznext(FILE_NO, DB_TASK *);
F_ADDR INTERNAL_FCN dio_pages(FILE_NO, DB_TASK *);
void INTERNAL_FCN dio_pzclr(DB_TASK *);

#ifdef DB_DEBUG
int INTERNAL_FCN dio_pzcheck(FILE_NO, DB_TASK *);
off_t INTERNAL_FCN dio_filesize(FILE_NO, DB_TASK *);
#endif /* DB_DEBUG */

int EXTERNAL_FCN dio_readfile(FILE_NO, off_t, void *, size_t, DB_ULONG, int *, DB_TASK *);
int EXTERNAL_FCN dio_writefile(FILE_NO, off_t, void *, size_t, DB_ULONG, DB_TASK *);

#ifdef DB_DEBUG
int INTERNAL_FCN check_cache(DB_TCHAR *, DB_TASK *);
#endif /* DB_DEBUG */

/* From inifile.c */
int INTERNAL_FCN initFromIniFile(DB_TASK *);

/* From inittab.c: */
int INTERNAL_FCN inittab(DB_TASK *, const SG *);
int EXTERNAL_FCN alloc_dict(DB_TASK *, const SG *);

/* From keyfcns.c: */
int EXTERNAL_FCN key_open(DB_TASK *);
void INTERNAL_FCN key_close(int, DB_TASK *);
int EXTERNAL_FCN key_init(int, DB_TASK *);
int INTERNAL_FCN key_reset(FILE_NO, DB_TASK *);
int EXTERNAL_FCN key_locpos(const char *, DB_ADDR *, DB_TASK *);
int EXTERNAL_FCN key_scan(int, DB_ADDR *, DB_TASK *);
int EXTERNAL_FCN key_boundary(int, DB_ADDR *, DB_TASK *);
int EXTERNAL_FCN key_insert(int, const char *, DB_ADDR, DB_TASK *);
int INTERNAL_FCN key_delete(int, const char *, DB_ADDR, DB_TASK *);

int EXTERNAL_FCN key_bldcom(int, char *, char *, int, DB_TASK *);
void INTERNAL_FCN key_cmpcpy(char *, char *, short);
void INTERNAL_FCN key_acpy(char *, char *, short);
void INTERNAL_FCN key_wacpy(wchar_t *, wchar_t *, short);

/* From libfcns.c: */
int INTERNAL_FCN nset_check(int, int *, SET_ENTRY **, DB_TASK *);
int INTERNAL_FCN nfld_check(long, int *, int *, RECORD_ENTRY **, FIELD_ENTRY **, DB_TASK *);
int INTERNAL_FCN nrec_check(int, int *, RECORD_ENTRY **, DB_TASK *);
int EXTERNAL_FCN fldcmp(FIELD_ENTRY *, const char *, const char *, DB_TASK *);
int INTERNAL_FCN SHORTcmp(const char *, const char *);
int EXTERNAL_FCN ADDRcmp(const DB_ADDR *, const DB_ADDR *);
int INTERNAL_FCN null_dba(const DB_ADDR);
int INTERNAL_FCN check_dba(DB_ADDR, DB_TASK *);

int INTERNAL_FCN ctblcmp(const unsigned char *, const unsigned char *, int, DB_TASK *);
DB_TCHAR *INTERNAL_FCN vstrnzcpy(DB_TCHAR *, const DB_TCHAR *, size_t);

/* From makenew.c: */
int INTERNAL_FCN sk_free(DB_TASK *);

/* From opens.c: */
PSP_FH EXTERNAL_FCN open_b(const DB_TCHAR *, unsigned int, unsigned short, DB_TASK *);
void   EXTERNAL_FCN commit_file(PSP_FH, DB_TASK *);
int    INTERNAL_FCN vnap(long);

int  INTERNAL_FCN RelinquishControl(void);
#define BY_NAP_SUCCESS 1
#define BY_NAP_FAILURE 0

/* apply some randomness */
#define SUCCESS_FACTOR  0.100
#define FAILURE_FACTOR  1.300

#define MAX_FACTOR      2.000
#define DEF_FACTOR      0.010
#define MIN_FACTOR      0.005
#define ONE_FACTOR      0.001    
/* if (2 * ONE_USER_NAP) >= MIN_FACTOR then change naptime() */

void INTERNAL_FCN adjust_naptime (int, DB_TASK *);
void INTERNAL_FCN naptime (DB_TASK *);

/* From ovfcns.c: */
int  EXTERNAL_FCN o_setup(DB_TASK *);
int  INTERNAL_FCN o_init(DB_TASK *);
int  INTERNAL_FCN o_fileinit(FILE_NO, DB_TASK *);
int  INTERNAL_FCN o_search(FILE_NO, F_ADDR, F_ADDR *, DB_TASK *);
int  INTERNAL_FCN o_pzwrite(FILE_NO, DB_TASK *);
int  INTERNAL_FCN o_flush(DB_TASK *);
int  INTERNAL_FCN o_update(DB_TASK *);
long INTERNAL_FCN o_pages(FILE_NO, DB_TASK *);
void INTERNAL_FCN o_free(int, DB_TASK *);
int  INTERNAL_FCN o_close(DB_TASK *);
int  INTERNAL_FCN o_write(PAGE_ENTRY *, DB_TASK *);

/* From recfcns.c: */
int INTERNAL_FCN r_chkfld(short, FIELD_ENTRY *, char *, const char *, DB_TASK *);
int INTERNAL_FCN r_delrec(short, DB_ADDR, DB_TASK *);
int INTERNAL_FCN r_gfld(FIELD_ENTRY *, char *, char *, DB_TASK *);
int INTERNAL_FCN r_gmem(int, char *, MEM_PTR *, DB_TASK *);
int INTERNAL_FCN r_gset(int, char *, SET_PTR *, DB_TASK *);
int INTERNAL_FCN r_pfld(short, FIELD_ENTRY *, char *, const char *, DB_ADDR *, DB_TASK *);
int INTERNAL_FCN r_pmem(int, char *, char *, DB_TASK *);
int INTERNAL_FCN r_pset(int, char *, char *, DB_TASK *);
int INTERNAL_FCN r_smem(DB_ADDR *, int, DB_TASK *);
int INTERNAL_FCN r_setopt(FIELD_ENTRY *, char *, DB_TASK *);
int INTERNAL_FCN r_clropt(FIELD_ENTRY *, char *, DB_TASK *);
int EXTERNAL_FCN r_tstopt(FIELD_ENTRY *, char *, DB_TASK *);

/* From pathfcns.c */
int INTERNAL_FCN con_dbd(DB_TCHAR *, DB_TCHAR *, DB_TCHAR *, DB_TASK *);
int EXTERNAL_FCN con_dbf(DB_TCHAR *, DB_TCHAR *, DB_TCHAR *, DB_TCHAR *, DB_TASK *);
DB_TCHAR *EXTERNAL_FCN get_element(const DB_TCHAR *, int);

/* From renfile.c: */
int INTERNAL_FCN renfiles(DB_TASK *);

/* From enter.c: */
int INTERNAL_FCN db_enter(int, int, DB_TASK *);
int INTERNAL_FCN db_exit(int, DB_TASK *);

#ifdef DB_TRACE
int INTERNAL_FCN api_enter(int, DB_TASK *);
int INTERNAL_FCN api_exit(DB_TASK *);
int INTERNAL_FCN fn_enter(DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN fn_exit(DB_TASK *);
int   db_printf(DB_TASK *, DB_TCHAR *fmt, ...);
int INTERNAL_FCN db_indent(DB_TASK *);
int INTERNAL_FCN db_undent(DB_TASK *);
#endif /* DB_TRACE */

/* From taffcns.c: */
int INTERNAL_FCN taf_open(DB_TASK *);
int INTERNAL_FCN taf_close(DB_TASK *);
int INTERNAL_FCN taf_access(int, DB_TASK *);
int INTERNAL_FCN taf_release(int, DB_TASK *);
int INTERNAL_FCN taf_add(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN taf_del(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN taf_login(DB_TASK *);
int INTERNAL_FCN taf_logout(DB_TASK *);
int INTERNAL_FCN taf_locking(int, int, long, DB_TASK *);

/* From truename.c */

/* From task.c */
int INTERNAL_FCN task_switch(DB_TASK *);
int INTERNAL_FCN create_cache(DB_TASK *);
int INTERNAL_FCN remove_cache(DB_TASK *);
int INTERNAL_FCN ntask_check(DB_TASK *);

/* From mapchar.c */
int  INTERNAL_FCN ctb_init(DB_TASK *);
int  INTERNAL_FCN ctbl_alloc(DB_TASK *);
void EXTERNAL_FCN ctbl_free(DB_TASK *);
int  INTERNAL_FCN ctbl_ignorecase(DB_TASK *);
int  INTERNAL_FCN ctbl_usecase(DB_TASK *);

/* From alloc.c */
DB_BOOLEAN  INTERNAL_FCN ll_access(llist *);
int         INTERNAL_FCN ll_append(llist *, char *, DB_TASK *);
void        INTERNAL_FCN ll_deaccess(llist *);
void        INTERNAL_FCN ll_free(llist *, DB_TASK *);
int         INTERNAL_FCN ll_prepend(llist *, char *, DB_TASK *);
char       *INTERNAL_FCN ll_first(llist *);
char       *INTERNAL_FCN ll_next(llist *);

/* Internal functions called by d_ functions */
int INTERNAL_FCN dcheckid(DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN dcloseall(DB_TASK *);
int INTERNAL_FCN dcmstat(int, DB_TASK *, int);
int INTERNAL_FCN dcmtype(int, int *, DB_TASK *, int);
int INTERNAL_FCN dconnect(int, DB_TASK *, int);
int INTERNAL_FCN dcostat(int, DB_TASK *, int);
int INTERNAL_FCN dcotype(int, int *, DB_TASK *, int);
int INTERNAL_FCN dcrget(DB_ADDR *, DB_TASK *, int);
int INTERNAL_FCN dcrread(long, void *, DB_TASK *, int);
int INTERNAL_FCN dcrset(DB_ADDR *, DB_TASK *, int);
int INTERNAL_FCN dcrstat(DB_TASK *, int);
int INTERNAL_FCN dcrtype(int *, DB_TASK *, int);
int INTERNAL_FCN dcrwrite(long, void *, DB_TASK *, int);
int INTERNAL_FCN dcsmget(int, DB_ADDR *, DB_TASK *, int);
int INTERNAL_FCN dcsmread(int, long, void *, DB_TASK *, int);
int INTERNAL_FCN dcsmset(int, DB_ADDR *, DB_TASK *, int);
int INTERNAL_FCN dcsmwrite(int, long, const void *, DB_TASK *, int);
int INTERNAL_FCN dcsoget(int, DB_ADDR *, DB_TASK *, int);
int INTERNAL_FCN dcsoread(int, long, void *, DB_TASK *, int);
int INTERNAL_FCN dcsoset(int, DB_ADDR *, DB_TASK *, int);
int INTERNAL_FCN dcsowrite(int, long, const void *, DB_TASK *, int);
int INTERNAL_FCN dcsstat(int, DB_TASK *, int);
int INTERNAL_FCN dctbpath(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN dctscm(int, DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dctsco(int, DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dctscr(DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dcurkey(DB_TASK *, int);
int INTERNAL_FCN ddbdpath(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN ddbfpath(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN ddblog(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN ddbnum(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN ddbstat(int, int, void *, int, DB_TASK *);
int INTERNAL_FCN ddbtaf(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN ddbtmp(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN ddbuserid(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN ddef_opt(unsigned long, DB_TASK *);
int INTERNAL_FCN ddelete(DB_TASK *, int);
int INTERNAL_FCN ddestroy(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN ddiscon(int, DB_TASK *, int);
int INTERNAL_FCN ddisdel(DB_TASK *, int);
int INTERNAL_FCN dfillnew(int, const void *, DB_TASK *, int);
int INTERNAL_FCN dfindco(int, DB_TASK *, int);
int INTERNAL_FCN dfindfm(int, DB_TASK *, int);
int INTERNAL_FCN dfindlm(int, DB_TASK *, int);
int INTERNAL_FCN dfindnm(int, DB_TASK *, int);
int INTERNAL_FCN dfindpm(int, DB_TASK *, int);
int INTERNAL_FCN dfldnum(int *, long, DB_TASK *, int);
int INTERNAL_FCN dfreeall(DB_TASK *);
int INTERNAL_FCN dgtscm(int, DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dgtsco(int, DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dgtscr(DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dgtscs(int, DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN diclose(DB_TASK *, int);
int INTERNAL_FCN dinitfile(FILE_NO, DB_TASK *, int);
int INTERNAL_FCN dinitialize(DB_TASK *, int);
int INTERNAL_FCN dinternals(DB_TASK *, int, int, int, void *, unsigned);
int INTERNAL_FCN dismember(int, DB_TASK *, int);
int INTERNAL_FCN disowner(int, DB_TASK *, int);
int INTERNAL_FCN dkeybuild(DB_TASK *, int);
int INTERNAL_FCN dkeydel(long, DB_TASK *, int);
int INTERNAL_FCN dkeyexist(long, DB_TASK *, int);
int INTERNAL_FCN dkeyfind(long, const void *, DB_TASK *, int);
int INTERNAL_FCN dkeyfree(long, DB_TASK *, int);
int INTERNAL_FCN dkeyfrst(long, DB_TASK *, int);
int INTERNAL_FCN dkeylast(long, DB_TASK *, int);
int INTERNAL_FCN dkeylock(long, DB_TCHAR *, DB_TASK *, int);
int INTERNAL_FCN dkeylstat(long, DB_TCHAR *, DB_TASK *, int);
int INTERNAL_FCN dkeynext(long, DB_TASK *, int);
int INTERNAL_FCN dkeyprev(long, DB_TASK *, int);
int INTERNAL_FCN dkeyread(void *, DB_TASK *);
int INTERNAL_FCN dkeystore(long, DB_TASK *, int);
int INTERNAL_FCN dlmclear(const DB_TCHAR *, const DB_TCHAR *, LMC_AVAIL_FCN *, DB_TASK *);
int INTERNAL_FCN dlmstat(DB_TCHAR *, int *, DB_TASK *);
int INTERNAL_FCN dlock(int, LOCK_REQUEST *, DB_TASK *, int);
int INTERNAL_FCN dlockcomm(LMC_AVAIL_FCN *, DB_TASK *);
int INTERNAL_FCN dlockmgr(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN dlocktimeout(int, int, DB_TASK *);
int INTERNAL_FCN dmakenew(int, DB_TASK *, int);
int INTERNAL_FCN dmapchar(unsigned char, unsigned char, const char *, unsigned char, DB_TASK *);
int INTERNAL_FCN dmembers(int, long *, DB_TASK *, int);
int INTERNAL_FCN doff_opt(unsigned long, DB_TASK *);
int INTERNAL_FCN don_opt(unsigned long, DB_TASK *);
int INTERNAL_FCN dopen(const DB_TCHAR *, const DB_TCHAR *, int, const SG *, DB_TASK *);
int INTERNAL_FCN dopentask(DB_TASK **);
int INTERNAL_FCN drdcurr(DB_ADDR * *, int *, DB_TASK *, int);
int INTERNAL_FCN ddbini(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN ddbver(DB_TCHAR *, DB_TCHAR *, int);
int INTERNAL_FCN drecfree(int, DB_TASK *, int);
int INTERNAL_FCN drecfrst(int, DB_TASK *, int);
int INTERNAL_FCN dreclast(int, DB_TASK *, int);
int INTERNAL_FCN dreclock(int, DB_TCHAR *, DB_TASK *, int);
int INTERNAL_FCN dreclstat(int, DB_TCHAR *, DB_TASK *, int);
int INTERNAL_FCN drecnext(DB_TASK *, int);
int INTERNAL_FCN drecnum(int *, int , DB_TASK *, int);
int INTERNAL_FCN drecover(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN drecprev(DB_TASK *, int);
int INTERNAL_FCN drecread(void *, DB_TASK *, int);
int INTERNAL_FCN drecset(int, DB_TASK *, int);
int INTERNAL_FCN drecstat(DB_ADDR, DB_ULONG, DB_TASK *, int);
int INTERNAL_FCN drecwrite(const void *, DB_TASK *, int);
int INTERNAL_FCN drenclean(DB_TASK *);
int INTERNAL_FCN drenfile(const DB_TCHAR *, FILE_NO, const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN drerdcurr(DB_ADDR * *, DB_TASK *, int);
int INTERNAL_FCN drlbclr(DB_TASK *, int);
int INTERNAL_FCN drlbset(DB_TASK *, int);
int INTERNAL_FCN drlbtst(DB_TASK *, int);
int INTERNAL_FCN dsetdb(int, DB_TASK *);
int INTERNAL_FCN dsetfiles(int, DB_TASK *);
int INTERNAL_FCN dsetfree(int, DB_TASK *, int);
int INTERNAL_FCN dsetkey(long, const void *, DB_TASK *, int);
int INTERNAL_FCN dsetlock(int, DB_TCHAR *, DB_TASK *, int);
int INTERNAL_FCN dsetlstat(int, DB_TCHAR *, DB_TASK *, int);
int INTERNAL_FCN dsetmm(int, int, DB_TASK *, int);
int INTERNAL_FCN dsetmo(int, int, DB_TASK *, int);
int INTERNAL_FCN dsetmr(int, DB_TASK *, int);
int INTERNAL_FCN dsetnum(int *, int , DB_TASK *, int);
int INTERNAL_FCN dsetom(int, int, DB_TASK *, int);
int INTERNAL_FCN dsetoo(int, int, DB_TASK *, int);
int INTERNAL_FCN dsetor(int, DB_TASK *, int);
int INTERNAL_FCN dsetpages(int, int, DB_TASK *);
int INTERNAL_FCN dsetrm(int, DB_TASK *, int);
int INTERNAL_FCN dsetro(int, DB_TASK *, int);
int INTERNAL_FCN dstscm(int, DB_ULONG, DB_TASK *, int);
int INTERNAL_FCN dstsco(int, DB_ULONG, DB_TASK *, int);
int INTERNAL_FCN dstscr(DB_ULONG, DB_TASK *, int);
int INTERNAL_FCN dstscs(int, DB_ULONG, DB_TASK *, int);
int INTERNAL_FCN dtimeout(int, DB_TASK *);
int INTERNAL_FCN dtrabort(DB_TASK *);/* dblfcns.c */
int INTERNAL_FCN dtrbegin(const DB_TCHAR *, DB_TASK *);
int INTERNAL_FCN dtrbound(DB_TASK *);
int INTERNAL_FCN dtrend(DB_TASK *);
int INTERNAL_FCN dtrlog(int, long, const char *, int, DB_TASK *);
int INTERNAL_FCN dtrmark(DB_TASK *);
int INTERNAL_FCN dutscm(int, DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dutsco(int, DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dutscr(DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dutscs(int, DB_ULONG *, DB_TASK *, int);
int INTERNAL_FCN dwrcurr(DB_ADDR *, DB_TASK *, int);

#endif /* PROTO_H */


