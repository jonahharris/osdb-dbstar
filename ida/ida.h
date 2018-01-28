/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

void  beep(void);
void  bld_keys(void);
void  dblist(void);
void  disp_rec(DB_ADDR *);
int   ed_field(int);
char *fldtotxt(int, char *, char *);
int   list_selection(int, int, char **, int, int, int);
int   menu(int);
int   rd_dba(DB_ADDR *);
int   rdtext(char *);
void  rectotxt(char *, char *);
int   set_select(void);
void  show_rec(int, char *);
char  tgetch(void);
int   ioc_on(void);
int   ioc_off(void);
void  next_rec(void);
void  edit_rec(void);

#ifdef QNX
int   tprintf(char *, ...);
#else
int   tprintf(char *, ...);
#endif /* QNX */

char *txttofld(int, char *, char *);
char *txttokey(int, char *, char *);
void  untgetch(char);
void  usererr(char *);

extern DB_TASK *task;


