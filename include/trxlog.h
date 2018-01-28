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

#ifndef TRXLOG_H
#define TRXLOG_H

/*
==========================================================================
    The following constants control the functioning of the cache overflow
    and transaction logging processes

    BUI             The number of bits in an unsigned int
    IX_PAGESIZE     The size (in bytes) of an index page
    IX_EPP          The number of entries that will fit on an index page
    RI_BITMAP_SIZE  The size of the index ri_bitmap (in unsigned int units)
    IX_SIZE         The number of index pages needed to control the db pages
    OADDR_OF_IXP    Calculates the overflow file address of an index page #

==========================================================================
*/

/* (BITS(unsigned int)) */
#define BUI (8*sizeof(unsigned int))

/* ((((256*sizeof(F_ADDR))+D_BLKSZ-1) / 512)*512) */
#define IX_PAGESIZE 1024

/* (IX_PAGESIZE / sizeof(F_ADDR)) */
#define IX_EPP 256
#define RI_BITMAP_SIZE(pcnt) ((int)((IX_SIZE(pcnt)+(BUI-1)) / BUI))
#define IX_SIZE(pcnt) ((long)( ((pcnt) + (IX_EPP-1)) / IX_EPP))

/* Next define the base file offsets for entries in the overflow file */

#define BM_BASE( file ) ( task->root_ix[file].base )
#define IX_BASE(file, pcnt) ((F_ADDR)(BM_BASE(file) + (RI_BITMAP_SIZE(pcnt)*sizeof(unsigned int))))
#define PZ_BASE(file, pcnt) ((F_ADDR)(IX_BASE(file, pcnt) + (IX_SIZE(pcnt)*IX_PAGESIZE)))

/* ========================================================================
*/

/* The following typedef'ed structure defines a single entry in the
    root index data.  */

typedef struct RI_ENTRY_S
{
    F_ADDR     pg_cnt;                  /* Number of pages currently in file */
    F_ADDR     base;                    /* Base of data stored in overflow */
    DB_BOOLEAN pz_modified;             /* Was page zero written to overflow? */
    int       *ri_bitmap;               /* Used index page ri_bitmap */
}  RI_ENTRY;

#define RI_ENTRY_IOSIZE (sizeof(RI_ENTRY)-sizeof(int *)+sizeof(short *))


/* page zero table entry */

/* size of the struct as stored in the files */
#define PGZEROSZ  (2 * sizeof(F_ADDR) + sizeof(DB_ULONG))

typedef struct PGZERO_S
{
    F_ADDR   pz_dchain;                 /* delete chain pointer */
    F_ADDR   pz_next;                   /* next available slot number */
    DB_ULONG pz_timestamp;              /* file's timestamp value */

    /* from here down, these item are not stored in the files */
    DB_BOOLEAN pz_modified;             /* TRUE if page has been modified */
    PAGE_ENTRY *pg_ptr;                 /* linked list of cache pages */

    /* The next two elements are only used if DB_DEBUG is defined,
        but they are not enclosed in an #ifdef DB_DEBUG section, as
        this would make debug and non-debug versions of the DLL and
        utilities incompatible.
    */
    off_t  file_size;                   /* length of file */
    time_t file_mtime;                  /* last modified time of file */

}  PGZERO;

/* binary search lookup table entry */

extern TAFFILE tafbuf;  /* Current TAF file after a * task->taf_access() call */

/* When the TAF file contains TAFLIMIT entries, it is permissible to
    access it if you are going to remove an entry, but not if you want
    to add an entry.  The following definitions are used as the parameter
    to task->taf_access().
*/
#define ADD_LOGFILE 1
#define DEL_LOGFILE 0

#define NO_TAFFILE_LOCK 2

#endif


