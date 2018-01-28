/***************************************************************************
 *                                                                         *
 * db.linux                                                                *
 * open source platform support package for Linux (tm)                     *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reseved.    *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      *
 *                                                                         *
 **************************************************************************/

/* pspmem.c - Contains the standard memory routines of the PSP */

#include "psp.h"
#include "pspint.h"

#define TAG_USED   1
#define TAG_ERRJMP 2

typedef struct _HDR {
    struct _HDR *prev; /* prev block on this tag */
    struct _HDR *next; /* next block on this tag */
} HDR;

typedef struct _TAG_ENTRY {
    struct _TAG_ENTRY *t_next;   /* next tag in tag list */
    struct _TAG_ENTRY *t_prev;   /* prev tag in tag list */
    PSP_SEM            t_sem;    /* semaphore for tag */
    HDR               *t_first;  /* head of memory block list */
    jmp_buf            t_jmpbuf; /* jump buffer for out of memory error */
    short              t_state;  /* tag state */
} TAG_ENTRY;

static PSP_SEM    tagTabSem = NO_PSP_SEM;
static TAG_ENTRY *tag0 = NULL;
static TAG_ENTRY *tagN = NULL;

/* ==========================================================================
    Initialize memory handling
*/
int psp_memInit(
    void)
{
    tag0 = tagN = (TAG_ENTRY *) calloc(1, sizeof(TAG_ENTRY));
    if (tag0 == NULL) {
         /* TBD: Log out of memory error */
         vtprintf(DB_TEXT("Out of memory initializing PSP memory module\n"));
         return PSP_FAILED;
    }

    tag0->t_state = TAG_USED;
    tag0->t_sem = NO_PSP_SEM; 

    if ((tag0->t_sem = psp_syncCreate(PSP_MUTEX_SEM)) == NO_PSP_SEM) {
        free(tag0);
        tag0 = tagN = NULL;
        return PSP_FAILED;
    }

    if ((tagTabSem = psp_syncCreate(PSP_MUTEX_SEM)) == NO_PSP_SEM) {
        psp_syncDelete(tag0->t_sem);
        free(tag0);
        tag0 = tagN = NULL;
        return PSP_FAILED;
    }

    return PSP_OKAY;
}

/* ==========================================================================
    Terminate memory handling
*/
void psp_memTerm(
    void)
{
    while (tag0)
        psp_freeTagMemory(tag0, 1);

    /* Do not delete the tagTabSem or tag0->t_sem semaphores because they
       will have been deleted by psp_syncShutdown */
}

/* ==========================================================================
    Create a new tag
*/
PSP_MEMTAG psp_createTag(
    jmp_buf jb,
    short   flags)
{
    register TAG_ENTRY *tt;

    if (!psp_inited)
        return NULL_MEMTAG;

    if ((tt = (TAG_ENTRY *) calloc(1, sizeof(TAG_ENTRY))) == NULL) {
        /* log out of memory error */
        return NULL_MEMTAG;
    }

    psp_syncEnterExcl(tagTabSem);

    tt->t_prev   = tagN;
    tagN->t_next = tt;
    tagN         = tt;

    psp_syncExitExcl(tagTabSem);

    if (jb != NULL) {
        memcpy(tt->t_jmpbuf, jb, sizeof(jmp_buf));
        tt->t_state = TAG_ERRJMP;
    }
    else
        tt->t_state = TAG_USED;

    if (flags & PSP_TAG_USESEM) {
        tt->t_sem = psp_syncCreate(PSP_MUTEX_SEM);
        psp_syncStart(tt->t_sem);
    }
    else
        tt->t_sem = NO_PSP_SEM;

    return tt;
}

/* ==========================================================================
    Free all allocated memory on a tag
*/
void psp_freeTagMemory(
    PSP_MEMTAG tag,
    short      freetag)
{
    PSP_SEM    sem;
    HDR       *curr;
    HDR       *prev;
    TAG_ENTRY *tt = tag;

    if (!psp_inited)
        return;

    if (tt->t_sem != NO_PSP_SEM && !psp_terminating)
        psp_syncEnterExcl(tt->t_sem);

    /* free all blocks allocated on this tag */
    for (curr = tt->t_first; curr; ) {
        prev = curr;
        curr = curr->next;
        free(prev);
    }

    tt->t_first = NULL;

    if (freetag) {
        if (tt->t_prev)
            tt->t_prev->t_next = tt->t_next;
        else
            tag0 = tt->t_next;

        if (tt->t_next)
            tt->t_next->t_prev = tt->t_prev;
        else
            tagN = tt->t_prev;

        sem = tt->t_sem;
        free(tt);
        if (sem != NO_PSP_SEM && !psp_terminating) {
            psp_syncExitExcl(sem);
            psp_syncDelete(sem);
        }
    }
    else if (tt->t_sem != NO_PSP_SEM && !psp_terminating)
        psp_syncExitExcl(tt->t_sem);
}

/* ==========================================================================
    Reset the jump buffer on a tag
*/
void psp_resetTag(
    jmp_buf    jb,
    PSP_MEMTAG tag)
{
    register TAG_ENTRY *tt;

    if (!psp_inited)
        return;

    tt = tag ? tag : tag0;
    if (tt->t_sem != NO_PSP_SEM)
        psp_syncEnterExcl(tt->t_sem);

    if (jb) {
        memcpy(tt->t_jmpbuf, jb, sizeof(jmp_buf));
        tt->t_state = TAG_ERRJMP;
    }
    else
        tt->t_state = TAG_USED;

    if (tt->t_sem != NO_PSP_SEM)
        psp_syncExitExcl(tt->t_sem);
}

/* ==========================================================================
    Allocate a block of memory
*/
void *psp_getMem(
    size_t          size,
    PSP_MEMTAG      tag,
    int             mode,
    const DB_TCHAR *data,
    long            locid)
{
    HDR       *pHdr;
    TAG_ENTRY *tt;

    if (!psp_inited || (!size && mode != MEM_STRDUP))
        return NULL;

    tt = tag ? tag : tag0;
    if (tt->t_sem != NO_PSP_SEM)
        psp_syncEnterExcl(tt->t_sem);

    switch (mode) {
        case MEM_ALLOC:
            pHdr = malloc(size + sizeof(HDR));        
            break;

        case MEM_CALLOC:
            pHdr = calloc(1, size + sizeof(HDR));
            break;

        case MEM_STRDUP:
            size = sizeof(DB_TCHAR) * (vtstrlen(data) + 1);
            /* fall through */
        case MEM_MEMDUP:
            if ((pHdr = malloc(size + sizeof(HDR))) != NULL)
                 memcpy((char *) pHdr + sizeof(HDR), data, size);

            break;

        default:
            return NULL;
    }

    if (!pHdr) {
        if (tt->t_sem != NO_PSP_SEM)
            psp_syncExitExcl(tt->t_sem);

        /* TBD:  Log out of memory error using locid for location info */
        vtprintf(DB_TEXT("Out of memory\n"));

        if (tt->t_state == TAG_ERRJMP)
            longjmp(tt->t_jmpbuf, 1);

        return NULL;
    }

    pHdr->prev = NULL;
    pHdr->next = tt->t_first;
    if (pHdr->next)
         pHdr->next->prev = pHdr;

    tt->t_first = pHdr;

    if (tt->t_sem != NO_PSP_SEM)
        psp_syncExitExcl(tt->t_sem);

    return pHdr + 1;
}

/* ==========================================================================
    Reallocate a block of memory
*/
void *psp_extendMem(
    const void *ptr,
    size_t      newsize,
    PSP_MEMTAG  tag,
    int         clear,
    size_t      incsize,
    long        locid)
{
    HDR       *pHdr;
    HDR       *prev;
    HDR       *next;
    HDR       *new;
    TAG_ENTRY *tt;

    if (!psp_inited)
        return NULL;

    if (!ptr)
        return psp_getMem(newsize, tag, clear ? MEM_CALLOC : MEM_ALLOC, NULL, locid);

    tt = tag ? tag : tag0;
    if (tt->t_sem != NO_PSP_SEM)
         psp_syncEnterExcl(tt->t_sem);

    pHdr = (HDR *) ptr - 1;
    prev = pHdr->prev;
    next = pHdr->next;

    if ((new = realloc(pHdr, newsize + sizeof(HDR))) == NULL) {
        if (tt->t_sem != NO_PSP_SEM)
            psp_syncExitExcl(tt->t_sem);

        /* TBD: Log out memory error using locid for location information. */
        vtprintf(DB_TEXT("Out of memory\n"));

        if (tt->t_state == TAG_ERRJMP)
            longjmp(tt->t_jmpbuf, 1);

        return NULL;
    }

    if (new != pHdr) {
        if (prev)
            prev->next = new;
        else
            tt->t_first = new;

        if (next)
            next->prev = new;
    }

    if (tt->t_sem != NO_PSP_SEM)
        psp_syncExitExcl(tt->t_sem);

    if (clear)
        memset((char *) (new + 1) + newsize - incsize, 0, incsize);

    return new + 1;
}


/* ==========================================================================
    Free a block of memory
*/
void psp_freeMemory(
    const void  *ptr,
    PSP_MEMTAG   tag)
{
    HDR      *pHdr;
    TAG_ENTRY *tt;

    if (!psp_inited)
        return;

    tt = tag ? tag : tag0;
    if (tt->t_sem != NO_PSP_SEM && !psp_terminating)
        psp_syncEnterExcl(tt->t_sem);

    pHdr  = (HDR *) ptr - 1;
    if (!pHdr->next && !pHdr->prev && tt->t_first != pHdr) {
        /* TBD: Log out memory error using locid for location information. */
        vtprintf(DB_TEXT("Attempting to free previously freed memory\n"));
        return;
    }

    if (pHdr->prev)
        pHdr->prev->next = pHdr->next;
    else
        tt->t_first = pHdr->next;

    if (pHdr->next)
        pHdr->next->prev = pHdr->prev;

    pHdr->next = pHdr->prev = NULL;

    free(pHdr);

    if (tt->t_sem != NO_PSP_SEM && !psp_terminating)
        psp_syncExitExcl(tt->t_sem);
}

