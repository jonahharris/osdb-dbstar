/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database lock manager                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* ======================================================================
   FILEID routines
*/

#include "lm.h"

#define MAXFILELEN   256         /* including terminating 0 */

typedef struct FILEIDTAB_S
{
   unsigned long  offset;  /* Offset from start of string table */
   unsigned long  hash_id; /* Hash code for searching */
   long           prefix;  /* Pathname part of filename (== -1 if none) */
   unsigned short active;  /* Number of active users for this name */
   DB_BYTE        length;  /* String length in bytes */
} FILEIDTAB;

typedef struct {
   FILEIDTAB    *fileid_tab;
   DB_TCHAR     *string_tab;
   DB_TCHAR      name[MAXFILELEN];
   long          max_names;
   unsigned long max_size;
   unsigned long next_offset;
   unsigned long free_space;

} FILEID_ENTRY;

/*
   Hash definitions.

   Note that we do not actually store the names in a hash table.  Instead,
   we use the computed hash value to speed up the linear search - we can do an
   arithmetic, rather than a string, comparison.
*/
#define NUM_PRIMES 13
static DB_USHORT primes[NUM_PRIMES] =
{
   22037, 21031, 19013, 16943, 14939, 12959, 11027,
   9127, 7237, 5441, 3673, 2027, 2
};

static int fileid_alloc(
    FILEID_ENTRY *fentry,
    long          names,
    unsigned long size)
{
    FILEIDTAB    *fnew = NULL;
    DB_TCHAR     *snew = NULL;
    unsigned long newsize;

    if (names > fentry->max_names) {
        newsize = (unsigned long) names * sizeof(FILEIDTAB);
        if ((fnew = (FILEIDTAB *)psp_cGetMemory(newsize, 0)) == NULL)
            return LM_NOMEMORY;

        if (fentry->max_names) {
            memcpy(fnew, fentry->fileid_tab,
                    fentry->max_names * sizeof(FILEIDTAB));
            psp_freeMemory(fentry->fileid_tab, 0);
        }

        fentry->fileid_tab = fnew;
        fentry->max_names = names;
    }

    if (size > fentry->max_size) {
        snew = (DB_TCHAR *) psp_cGetMemory(size * sizeof(DB_TCHAR), 0);
        if (snew == NULL)
            return LM_NOMEMORY;

        if (fentry->max_size) {
            memcpy(snew, fentry->string_tab, fentry->max_size);
            psp_freeMemory(fentry->string_tab, 0);
        }

        fentry->string_tab = snew;
        fentry->max_size = size;
    }

    return LM_OKAY;
}

int fileid_init(
    FIDTAB       *ftab,
    long          names,
    unsigned long size)
{
    int           stat;
    FILEID_ENTRY *fentry;

    if ((fentry = psp_cGetMemory(sizeof(FILEID_ENTRY), 0)) == NULL)
        return LM_NOMEMORY;

    if ((stat = fileid_alloc(fentry, names, size)) != LM_OKAY)
        return stat;

    *ftab = fentry;
    return LM_OKAY;
}


void fileid_deinit(
    FIDTAB ftab)
{
    FILEID_ENTRY *fentry = (FILEID_ENTRY *) ftab;

    if (fentry->fileid_tab)
        psp_freeMemory(fentry->fileid_tab, 0);

    if (fentry->string_tab)
        psp_freeMemory(fentry->string_tab, 0);

    psp_freeMemory(fentry, 0);
}


static void fileid_hash(
    DB_TCHAR      *filename,
    int            len,
    unsigned long *hash)
{
    int           ii;
    unsigned long tmp_hash = 0L;
    DB_TCHAR     *p = filename;

    for (ii = 0; ii < len; ii++, p++)
        tmp_hash += primes[ii % NUM_PRIMES] * (DB_USHORT)fnupper(*p);

    *hash = tmp_hash;
}


static void fileid_compress(
    FILEID_ENTRY *fentry)
{
    FILEIDTAB    *ft;
    DB_TCHAR     *st = fentry->string_tab;
    long          ii;
    long          smallest_idx = 0;
    unsigned long cur_pos = 0;
    unsigned long smallest;
    unsigned long offset;

    do {
        smallest = fentry->max_size;

        ft = fentry->fileid_tab;
        for (ii = 0; ii < fentry->max_names; ii++, ft++) {
            offset = ft->offset;
            if (ft->active && offset < smallest && offset >= cur_pos) {
                smallest = offset;
                smallest_idx = ii;
            }
        }

        if (smallest != fentry->max_size) {
            ft = &fentry->fileid_tab[smallest_idx];
            if (smallest != cur_pos) {
                memmove(&st[cur_pos], &st[smallest], ft->length *
                        sizeof(DB_TCHAR));
                ft->offset = cur_pos;
            }

            cur_pos += ft->length;
        }

    } while (smallest != fentry->max_size);

    fentry->next_offset = cur_pos;
}


static long fileid_lookup(
    FILEID_ENTRY *fentry,
    DB_TCHAR     *filename,
    int           len)
{
    DB_TCHAR      save;
    DB_TCHAR     *p;
    int           plen;
    long          ii;
    long          prefix = -1;
    unsigned long hash;
    FILEIDTAB    *ft;

    if (len) {
        p = &filename[len - 1];
        if (*p != DIRCHAR) {
            plen = len - 1;
            while (p > filename && *--p != DIRCHAR)
                --plen;

            if (*p == DIRCHAR) {
                save = *++p;
                *p = 0;
                prefix = fileid_lookup(fentry, filename, plen);
                *(filename = p) = save;
                if (prefix == -1)
                    return -1;

                len -= plen;
            }
        }
    }

    fileid_hash(filename, len, &hash);

    for (ft = fentry->fileid_tab, ii = 0; ii < fentry->max_names; ii++, ft++) {
        if (ft->active && ft->length == (DB_BYTE) len + 1 &&
                ft->hash_id == hash && ft->prefix == prefix &&
                psp_fileNamenCmp(&fentry->string_tab[ft->offset], filename,
                len) == 0)
            break;
    }

    if (ii == fentry->max_names)
        return -1;
  
    return ii;
}


long fileid_fnd(
    FIDTAB    ftab,
    DB_TCHAR *filename)
{
    return fileid_lookup((FILEID_ENTRY *) ftab, filename, vtstrlen(filename));
}

static long fileid_insert(
    FILEID_ENTRY *fentry,
    DB_TCHAR     *filename,
    int           len)
{
    int        plen;
    int        stat;
    long       jj;
    long       prefix = -1;
    DB_TCHAR  *p;
    DB_TCHAR   save;
    FILEIDTAB *ft;

    if (len >= MAXFILELEN)
        return -1;  /* string too long */

    if ((jj = fileid_lookup(fentry, filename, len)) == -1) {
        if (len) {
            p = &filename[len - 1];
            if (*p != DIRCHAR) {
                plen = len-1;
                while (p > filename && *--p != DIRCHAR)
                    --plen;

                if (*p == DIRCHAR) {
                    save = *++p;
                    *p = 0;
                    prefix = fileid_insert(fentry, filename, plen);
                    *(filename = p) = save;
                    len -= plen;
                }
            }
        }

        ft = fentry->fileid_tab;
        for (jj = 0; jj < fentry->max_names; jj++, ft++) {
            if (ft->active == 0)
                break;
        }

        if (jj == fentry->max_names) {
            if (fileid_alloc(fentry, fentry->max_names + 32,
                    fentry->max_size) != LM_OKAY)
                return -1;

            ft = &fentry->fileid_tab[jj];
        }

        fileid_hash(filename, len, &ft->hash_id);

        ++len;   /* include terminating 0 */
        if ((fentry->next_offset + len) > fentry->max_size) {
            if (fentry->free_space >= (DB_BYTE) len)
                fileid_compress(fentry);

            if ((fentry->next_offset + len) > fentry->max_size) {
                stat = fileid_alloc(fentry, fentry->max_names,
                        fentry->max_size + 1024);
                if (stat != LM_OKAY)
                    return -1;
            }
        }

        memcpy(&fentry->string_tab[fentry->next_offset], filename,
                len * sizeof(DB_TCHAR));

        ft->offset = fentry->next_offset;
        ft->prefix = prefix;
        ft->length = (DB_BYTE)len;
        ft->active = 1;

        fentry->next_offset += len;
        fentry->free_space  -= len;
    }
    else {
        ft = &fentry->fileid_tab[jj];
        if ((prefix = ft->prefix) != -1)
            fentry->fileid_tab[prefix].active++;

        ft->active++;
    }

    return jj;
}


long fileid_add(
    FIDTAB   ftab,
    DB_TCHAR *filename)
{
   return fileid_insert((FILEID_ENTRY *) ftab, filename, vtstrlen(filename));
}


void fileid_del(
    FIDTAB ftab,
    long   fileid)
{
    FILEID_ENTRY *fentry = (FILEID_ENTRY *) ftab;
    FILEIDTAB    *ft = &fentry->fileid_tab[fileid];

    if (ft->prefix != -1)
        fileid_del(ftab, ft->prefix);

    if (ft->active)
        ft->active--;

    if (ft->active == 0)
        fentry->free_space += ft->length;
}


static int fileid_string(
    FILEID_ENTRY *fentry,
    long          fileid,
    DB_TCHAR     *string)
{
    int         len = 0;
    FILEIDTAB  *ft = &fentry->fileid_tab[fileid];

    if (ft->prefix != -1)
        len = fileid_string(fentry, ft->prefix, string);

    memcpy(string + len, &fentry->string_tab[ft->offset],
            ft->length * sizeof(DB_TCHAR));

    return len + ft->length - 1;
}


DB_TCHAR *fileid_get(
    FIDTAB ftab,
    long   fileid)
{
    FILEID_ENTRY *fentry = (FILEID_ENTRY *) ftab;
    FILEIDTAB    *ft;

    fentry->name[0] = 0;

    if (fileid != -1) {
        ft = &fentry->fileid_tab[fileid];
        if (ft->active > 0) {
            if (ft->prefix == -1)
                return &fentry->string_tab[ft->offset];

            fileid_string(fentry, fileid, fentry->name);
        }
    }

    return fentry->name;
}

