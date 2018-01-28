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

/*-----------------------------------------------------------------------

     dbcache.h - cache manipulation function prototypes.

-----------------------------------------------------------------------*/

#define NUM_CACHE_PAGES    8
#define CACHE_PG_SIZE      2048
#define DCHAIN_ID          1L
#define USAGE_ID           10001L

typedef struct ID_ENTRY_S {
    long id;
    void *data;
    struct ID_ENTRY_S *prev_lru;
    struct ID_ENTRY_S *next_lru;
} ID_ENTRY;

typedef struct PAGE_FILE_S {
    PSP_FH         desc;
    long           last_page;
    DB_TCHAR       name[256];
} PAGE_FILE;

typedef struct PAGE_CACHE_S {
    short max_ent;
    short no_ent;
    ID_ENTRY *mru_ent;
    ID_ENTRY *lru_ent;
    ID_ENTRY *id_lookup[NUM_CACHE_PAGES];
    ID_ENTRY *new_id_entry;
    PAGE_FILE *used;
    PAGE_FILE *dchain;
} PAGE_CACHE;

PAGE_CACHE *cache_alloc();
void  *cache_find(long, PAGE_CACHE *);
void   cache_free(PAGE_CACHE *);
void   cache_clear(PAGE_CACHE *);
void   loader(long, void *, PAGE_CACHE *);
short  zapper(long, void *, PAGE_CACHE *);
void   delete_temp_files(PAGE_CACHE *);


