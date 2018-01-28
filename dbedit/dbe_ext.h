/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbedit utility                                    *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* External definitions for DBEDIT */

extern DBE_SLOT slot;
extern int decimal, titles, fields, changed, unicode;

extern void dbe_select(int);
extern int parse(DB_TCHAR **, int *, struct dbe_tok *, int, int, DB_TCHAR *, int, DB_TASK *);
extern DB_TCHAR *gettoken(DB_TCHAR *, DB_TCHAR *, int);
extern int getkwd(DB_TCHAR *, int);
extern int getdba(DB_TCHAR *, DB_ADDR *);
extern long getlong(DB_TCHAR *);
extern long gethex(DB_TCHAR *);
extern long getnum(DB_TCHAR *);
extern int getfile(DB_TCHAR *, DB_TASK *);
extern int getrec(DB_TCHAR *, DB_TASK *);
extern int getfld(DB_TCHAR *, int *, DB_TASK *);
extern int getset(DB_TCHAR *, DB_TASK *);
extern int disp(struct dbe_tok *, int *, DB_TASK *);
extern int disp_type(DB_TASK *);
extern int disp_dba(void);
extern int disp_ts(DB_TASK *);
extern int disp_opt(DB_TASK *);
extern int disp_set(int, DB_TASK *);
extern int disp_mem(int, DB_TASK *);
extern int disp_fld(int, DB_TASK *);
extern int disp_str(DB_TCHAR *, DB_TCHAR *);
extern int do_hex(long, char *, DB_TCHAR *, int);
extern DB_TCHAR *do_hexstr(char *, DB_TCHAR *, int);
extern int do_ascii(DB_TCHAR *, DB_TCHAR *, int);
extern short getrtype(char *, DB_TASK *);
extern int testopt(int, char *, DB_TASK *);
extern int edit(struct dbe_tok *, int *, DB_TCHAR *, DB_TASK *);
extern int edit_type(char *, DB_TASK *);
extern int edit_dba(char *, int, DB_TASK *);
extern int edit_opt(char *, DB_TASK *);
extern int edit_num(char *);
extern int edit_long(long, long *);
extern int edit_dchain(DB_TASK *);
extern int edit_nextslot(DB_TASK *);
extern int edit_hex(DB_TASK *);
extern DB_TCHAR *getxcomm(DB_TCHAR *, DB_TCHAR *, int);
extern DB_TCHAR *gettext(DB_TCHAR *, char *, int, int *);
extern int edx_start(DB_TASK *);
extern int edx_goto(long, int, DB_TASK *);
extern int edx_print(long, DB_TASK *);
extern int edx_write(char *, int, DB_TASK *);
extern int edx_search(char *, int, int, DB_TASK *);
extern int edx_comp(struct dbe_page *, long, int *, char *, int, DB_TASK *);
extern int edx_end(DB_TASK *);
extern long do_line(long, char *, long, int);
extern int dbe_err(int, DB_TCHAR *);
extern int dgoto(struct dbe_tok *, int *, DB_TCHAR *, DB_TASK *);
extern int help(int, DB_TASK *);
extern int dbe_init(int, DB_TCHAR **, DB_TASK *);
extern int dbe_term(DB_TCHAR *, DB_TASK *);
extern void dbe_ioinit(void);
extern int dbe_out(DB_TCHAR *, FILE *);
extern int dbe_getline(DB_TCHAR *, DB_TCHAR *, int);
extern int in_open(DB_TCHAR *);
extern int in_close(void);
extern F_ADDR phys_addr(DB_ADDR, DB_TASK *);
extern DB_ADDR phys_to_dba(FILE_NO, F_ADDR, DB_TASK *);
extern int dbe_open(short, DB_TASK *);
extern int dbe_read(DB_ADDR, DB_TASK *);
extern int dbe_write(DB_TASK *);
extern int dbe_close(DB_TASK *);
extern int dbe_chkdba(DB_ADDR, DB_TASK *);
extern int read_dchain(F_ADDR *, DB_TASK *);
extern int write_dchain(F_ADDR, DB_TASK *);
extern int read_nextslot(DB_ADDR *, DB_TASK *);
extern int write_nextslot(DB_ADDR, DB_TASK *);
extern int dbe_xread(F_ADDR, struct dbe_page *, DB_TASK *);
extern int dbe_xwrite(DB_TASK *);
extern int main(int, DB_TCHAR **);
extern int process(struct dbe_tok *, int, int *, DB_TCHAR *, int, DB_TASK *);
extern int base(struct dbe_tok *, int *);
extern int reread(DB_TASK *);
extern int show(struct dbe_tok *, int *, DB_TASK *);
extern int show_nft(DB_TASK *);
extern int show_ft(int, DB_TASK *);
extern int show_nrt(int, DB_TASK *);
extern int show_rt(int, DB_TASK *);
extern int show_nst(DB_TASK *);
extern int show_st(int, DB_TASK *);
extern int show_nkt(DB_TASK *);
extern int show_nfd(DB_TASK *);
extern int show_fd(int *, int, int, DB_TASK *);
extern DB_TCHAR *dbe_getstr(int);
extern int verify(struct dbe_tok *, int *, DB_TCHAR *, DB_TASK *);
extern long chk_own(char *, short, DB_ADDR, DB_TASK *);
extern int val_mtype(short, short, DB_TASK *);
extern int pr_ownmem(short, DB_ADDR, int, DB_TASK *);
extern int pr_errdba(int, DB_ADDR);
extern int pr_errtype(int, int);
extern int pr_errcount(long, long);
extern int pr_total(long);
extern int pr_error(int);
extern int vrfydba(DB_ADDR, DB_TASK *);


