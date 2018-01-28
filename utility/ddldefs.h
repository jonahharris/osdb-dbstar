/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ddlp utility                                      *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/*------------------------------------------------------------------------

    db.* Database Definition Language Processor

    ddldefs.h

------------------------------------------------------------------------*/


#define BYTE_ALIGN  (-1)
#define DEF_ALIGN    3

#define CHR_ALIGN    0 
#define SHT_ALIGN    1
#define REG_ALIGN    2
#define DBL_ALIGN    3
#define MAX_ALIGNS   4


void pr_structs (void);
void pr_constants (void);
void write_header (void);
void write_dm_header (void);
struct field_info *pr_decl (register struct field_info *);

extern int yydebug;

/*
    All global variables are in one structure, to reduce naming conflicts
    on VxWorks.
*/
typedef struct _DDLP_G
{
    /* option flags */
    int d_flag;      /* allow duplicate field names */
#ifndef NO_PREPRO
    int i_flag;      /* include path for compiler pre-processor */
#endif
    int n_flag;      /* don't include record, field, set names in .dbd file */
    int r_flag;      /* report option flag */
    int s_flag;      /* case sensitivity - default is case sensitive */
    int u_flag;      /* input and output files in Unicode text */
    int x_flag;      /* print identifier cross reference */
    int z_flag;      /* don't include SIZEOF_??? consts */

    SG *sg;          /* SafeGarde control block */

    /* file handles */
    FILE *schema;
    FILE *ifile;
    FILE *outfile;   /* handle on normal destination for output */

    /* file names */
    DB_TCHAR db_name[FILENMLEN];
    DB_TCHAR ddlfile[FILENMLEN];
    DB_TCHAR ifn[FILENMLEN];
    DB_TCHAR msg[FILENMLEN * 3];

    /* padding boundaries */
    unsigned short str_pad[MAX_ALIGNS];            /* non-arrayed struct */
    unsigned short str_array_pad[MAX_ALIGNS];      /* arrayed struct */

    int tot_errs;    /* total errors encountered */
    int line;        /* current input line number */
    int abort_flag;  /* mainly for VxWorks - cannot call exit() */

    FILE_ENTRY   ft_entry;
    RECORD_ENTRY rt_entry;
    FIELD_ENTRY  fd_entry;
    SET_ENTRY    st_entry;
    KEY_ENTRY    kt_entry;
    MEMBER_ENTRY mt_entry;

    /* tables used in ddltable.c: */
    ELEM_INFO *elem_list;
    ELEM_INFO *elem_last;

    short tot_files;
    short tot_records;
    short tot_fields;
    short tot_sets;
    short tot_members;
    short tot_sort_fields;
    short tot_comkeyflds;

    struct file_info *file_list;
    struct file_info *file_last;

    struct record_info *rec_list;
    struct record_info *last_rec;

    struct field_info *field_list;
    struct field_info *last_fld;
    struct field_info *cur_struct;

    struct set_info *set_list;
    struct set_info *last_set;

    struct member_info *mem_list;
    struct member_info *last_mem;

    struct ddlkey_info *key_list;
    struct ddlkey_info *last_key;

    struct sort_info *sort_list;
    struct sort_info *last_sort;

} DDLP_G;

extern DDLP_G ddlp_g;

short calc_align(int, unsigned char, unsigned short);
void  init_align(void);


