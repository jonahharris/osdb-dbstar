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

#include "db.star.h"
#include "dbcache.h"

static ID_ENTRY *addToCache (long, PAGE_CACHE *);
static ID_ENTRY *idLookup   (long, short *, PAGE_CACHE *);
static ID_ENTRY *touchEntry (ID_ENTRY *, PAGE_CACHE *);

#define DB_DENYRW 0

/* ======================================================================
     Allocate and initialize a cache
*/
PAGE_CACHE *cache_alloc()
{
    register short i;
    PAGE_CACHE *csh;

    /* allocate cache */
    if ((csh = (PAGE_CACHE *) psp_cGetMemory(sizeof(PAGE_CACHE), 0)) == NULL)
        return NULL;

    /* initialize cache */
    csh->max_ent   = NUM_CACHE_PAGES;
    csh->no_ent    = 0;
    csh->mru_ent   = NULL;
    csh->lru_ent   = NULL;

    /* allocate spare cache page */
    csh->new_id_entry = (ID_ENTRY *) psp_cGetMemory(sizeof(ID_ENTRY), 0);
    if (!csh->new_id_entry)
       goto failed;

    csh->new_id_entry->data = psp_cGetMemory(CACHE_PG_SIZE, 0);
    if (!csh->new_id_entry->data)
        goto failed;

    csh->used = (PAGE_FILE *) psp_cGetMemory(sizeof(PAGE_FILE), 0);
    if (!csh->used)
        goto failed;

    csh->dchain = (PAGE_FILE *) psp_cGetMemory(sizeof(PAGE_FILE), 0);
    if (!csh->dchain)
        goto failed;

    /* allocate cache pages */
    for ( i = 0; i < csh->max_ent; ++i )
    {
        csh->id_lookup[i] = (ID_ENTRY *) psp_cGetMemory(sizeof(ID_ENTRY), 0);
        if (!csh->id_lookup[i])
            goto failed;

        csh->id_lookup[i]->data = (void *) psp_cGetMemory(CACHE_PG_SIZE, 0);
        if (!csh->id_lookup[i]->data)
            goto failed;
    }

    return csh;

failed:
    cache_free(csh);
    return (PAGE_CACHE *) NULL;
}

/* ======================================================================
     Clear a cache
*/
void cache_clear(PAGE_CACHE *csh)
{
    register int i;

    /* clear cache */
    csh->no_ent    = 0;
    csh->mru_ent   = NULL;
    csh->lru_ent   = NULL;

    /* clear lookup table */
    for ( i = 0; i < csh->max_ent; ++i )
    {
        csh->id_lookup[i]->id = 0;
        csh->id_lookup[i]->prev_lru = NULL;
        csh->id_lookup[i]->next_lru = NULL;
    }
}

/* ======================================================================
     Find and or load entry in cache by id
*/
void *cache_find(long id, PAGE_CACHE *csh)
{
    ID_ENTRY *idp = NULL;

    if (id == 0)
        return NULL;

    if ((idp = idLookup(id, NULL, csh)) == NULL)
        idp = addToCache(id, csh);
    else
        touchEntry(idp, csh);

    return idp ? idp->data : NULL;
}

/* ======================================================================
     Free a cache
*/
void cache_free(PAGE_CACHE *csh)
{
    register short i;

    delete_temp_files(csh);

    for ( i = 0; i < csh->max_ent; ++i )
    {
        if (csh->id_lookup[i])
        {
            if (csh->id_lookup[i]->data)
                psp_freeMemory(csh->id_lookup[i]->data, 0);

            psp_freeMemory(csh->id_lookup[i], 0);
        }
    }

    if (csh->new_id_entry)
    {
        if (csh->new_id_entry->data)
            psp_freeMemory(csh->new_id_entry->data, 0);

        psp_freeMemory(csh->new_id_entry, 0);
    }
    if (csh->used)
        psp_freeMemory(csh->used, 0);

    if (csh->dchain)
        psp_freeMemory(csh->dchain, 0);

    psp_freeMemory(csh, 0);
}

void delete_temp_files(PAGE_CACHE *csh)
{
    if (csh->used->desc != NULL)
    {
        psp_fileClose(csh->used->desc);
        psp_fileRemove(csh->used->name);
    }

    if (csh->dchain->desc != NULL)
    {
        psp_fileClose(csh->dchain->desc);
        psp_fileRemove(csh->dchain->name);
    }

    csh->dchain->desc = csh->used->desc = NULL;
    csh->dchain->last_page = csh->used->last_page = 0;
}

/* ======================================================================
     Add an entry into a cache
*/
static ID_ENTRY *addToCache(long id, PAGE_CACHE *csh)
{
    short new_ent, old_ent;
    ID_ENTRY *idp, *idq;
    ID_ENTRY *old_id_entry = NULL;

    /* load the new page into new_id_entry */
    csh->new_id_entry->id = id;
    csh->new_id_entry->prev_lru = NULL;
    csh->new_id_entry->next_lru = NULL;
    loader(id, csh->new_id_entry->data, csh);

    /* find position to add new entry in cache */
    idLookup(id, &new_ent, csh);

    /* if cache is full, swap a page out */
    if ( csh->no_ent == csh->max_ent )
    {
        /* remove lru_ent */

        idq = csh->lru_ent;
        csh->lru_ent = idq->prev_lru;
        csh->lru_ent->next_lru = NULL;

        if ((idq = idLookup(idq->id, &old_ent, csh)) == NULL)
            return NULL;

        /* write page to temp file */
        if (zapper(idq->id, idq->data, csh) == -1)
            return NULL;

        if (old_ent < csh->no_ent - 1)
        {
            old_id_entry = csh->id_lookup[old_ent];
            memmove(&csh->id_lookup[old_ent], &csh->id_lookup[old_ent + 1],
                (csh->no_ent - old_ent - 1) * sizeof(ID_ENTRY *));
        }

        if (old_ent < new_ent)
            --new_ent;

        --csh->no_ent;
    }

    /* add entry into id_lookup table */
    if (new_ent < csh->no_ent)
    {
        if (old_id_entry == NULL)
            old_id_entry = csh->id_lookup[csh->no_ent];
        memmove(&csh->id_lookup[new_ent+1], &csh->id_lookup[new_ent],
            (csh->no_ent - new_ent) * sizeof(ID_ENTRY *));
    }

    if (old_id_entry == NULL)
        old_id_entry = csh->id_lookup[new_ent];

    /*
        By now old_id_entry should be pointing at a page that can
        be re-used; csh->new_id_entry points at the page that we
        want to put into the cache. 
    */
    idp = csh->id_lookup[new_ent] = csh->new_id_entry;
    csh->new_id_entry = old_id_entry;

    ++csh->no_ent;

    /* place at head of mru entries */
    if (csh->mru_ent != NULL)
    {
        idp->next_lru = csh->mru_ent;
        csh->mru_ent->prev_lru = idp;
    }

    csh->mru_ent = idp;
    if (csh->lru_ent == NULL)
         csh->lru_ent = idp;

    return idp;
}

/* ======================================================================
     Lookup up an entry by id within a cache
*/
static ID_ENTRY *idLookup(long id, short *ent, PAGE_CACHE *csh)
{
    register short i, l, u;
    ID_ENTRY *idp = NULL;

    /* use binary search to find id match */
    for (i = l = 0, u = (short) (csh->no_ent - 1); u >= l; )
    {
        /* look for match on id */
        i = (short) ((l + u) / 2);
        idp = csh->id_lookup[i];
        if (id < idp->id)
            u = (short) (i - 1);
        else if ( id > idp->id)
            l = (short) (i + 1);
        else
        {
            if (ent)
                *ent = i;
            return idp;
        }
    }
    if ( ent )
    {
        if (idp && (id > idp->id))
            i++;
        *ent = i;
    }

    return NULL;
}

/* ======================================================================
     Touch LRU & MRU entries within a cache
*/
static ID_ENTRY *touchEntry(ID_ENTRY *idp, PAGE_CACHE *csh)
{
    if (idp->prev_lru == NULL)
        return idp;
    else
        idp->prev_lru->next_lru = idp->next_lru;
    
    if (idp->next_lru == NULL)
        csh->lru_ent = idp->prev_lru;
    else
        idp->next_lru->prev_lru = idp->prev_lru;

    idp->next_lru          = csh->mru_ent;
    csh->mru_ent->prev_lru = idp;
    idp->prev_lru          = NULL;
    csh->mru_ent           = idp;
    return idp;
}

/***************************************************************************/
/***************************************************************************/
/*                     Pagemap Routines                                    */
/***************************************************************************/
/***************************************************************************/

void loader(long id, void *data, PAGE_CACHE *csh)
{
    long       page_no;
    PAGE_FILE *pfile;

    if (id >= USAGE_ID)
    {
        page_no = id - USAGE_ID;
        pfile = csh->used;
    }
    else
    {
        page_no = id - DCHAIN_ID;
        pfile = csh->dchain;
    }
    if (pfile->desc == NULL || page_no > pfile->last_page)
    {
        memset(data, 0, CACHE_PG_SIZE);
        return;
    }
    psp_fileSeek(pfile->desc, page_no * CACHE_PG_SIZE);
    if (psp_fileRead(pfile->desc, data, CACHE_PG_SIZE) != CACHE_PG_SIZE)
        memset(data, 0, CACHE_PG_SIZE);
}

/***************************************************************************/

short zapper(long id, void *data, PAGE_CACHE *csh)
{
    char      *zero_data;
    long       page_no;
    long       pg;
    PAGE_FILE *pfile;

    if (id >= USAGE_ID)
    {
        page_no = id - USAGE_ID;
        pfile = csh->used;
    }
    else
    {
        page_no = id - DCHAIN_ID;
        pfile = csh->dchain;
    }

    if (pfile->desc == NULL)
    {
        if ((pfile->desc = psp_fileOpen(pfile->name, O_CREAT | O_TRUNC | O_RDWR, PSP_FLAG_DENYRW)) == NULL)
        {
            vtprintf(DB_TEXT("Open failed on dbcheck temporary file. errno = %d\n"), errno);
            return(-1);
        }
    }

    if (page_no > pfile->last_page)
    {
        /*
            Writing beyond end of file - fill pages in between with zeros
        */
        if ((zero_data = (char *) psp_cGetMemory(CACHE_PG_SIZE, 0)) == NULL)
        {
            vtprintf(DB_TEXT("Out of memory\n"));
            return(-1);
        }

        for (pg = pfile->last_page + 1; pg < page_no; pg++)
        {
            if (psp_fileSeekWrite(pfile->desc, pg * CACHE_PG_SIZE, zero_data, CACHE_PG_SIZE) != CACHE_PG_SIZE)
            {
                vtprintf(DB_TEXT("Write failed on dbcheck temporary file %s. errno = %d\n"),
                         pfile->name, errno);
                psp_freeMemory(zero_data, 0);
                return(-1);
            }
        }
        psp_freeMemory(zero_data, 0);
    }

    if (psp_fileSeekWrite(pfile->desc, page_no * CACHE_PG_SIZE, data, CACHE_PG_SIZE) != CACHE_PG_SIZE)
    {
        vtprintf(DB_TEXT("Write failed on dbcheck temporary file %s. errno = %d\n"),
                 pfile->name, errno);
        return(-1);
    }

    if (page_no > pfile->last_page)
        pfile->last_page = page_no;

    return(1);
}


