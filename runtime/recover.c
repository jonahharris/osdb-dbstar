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

#include "db.star.h"


typedef struct RFT_ENTRY_S
{
    short pgsize;                    /* file page size */
    DB_TCHAR fname[FILENMLEN];          /* file name */
    RI_ENTRY rix;                       /* root index entry */
}  RFT_ENTRY;                          /* recovery file table entry */

static short nfiles;                   /* number of files in logged db */
static RFT_ENTRY *rft = NULL;      /* db recovery file table */
static int max_pg_size;                   /* max db page size */
static char *pgbuf = NULL;         /* db page buffer */
static long *ixbuf = NULL;         /* index page buffer */


/* ======================================================================
    Perform automatic database recovery
*/
int INTERNAL_FCN drecover(const DB_TCHAR *logfile, DB_TASK *task)
{
    PSP_FH        lfh;              /* log file descriptor */
    PSP_FH        dbf;              /* database file descriptor */
    int           bm_size;
    size_t        size;
    unsigned long rd_opt;
    F_ADDR        ix_pg_cnt, ip, ixp_addr, db_page,
                  pz_base;
    unsigned int  mask, wordpos, ixp_used;
    int           slot, fno;
    RFT_ENTRY    *rft_ptr;
    DB_TCHAR      realLog[FILENMLEN], *ptr;
    
    /* even with READONLY option set, attempt to recover */
    rd_opt = task->dboptions & READONLY;
    if (rd_opt)
        task->dboptions &= ~READONLY;

    /* Since all the LOG files for a database group must be in the same
       physical place, and since the lockmgr will never ask you to recover
       anyone using a different TAF--each database group having only one
       TAF--and since userA's log path may be invalid for userB doing the
       recovery, userB can find userA's log file by using userB's log path.
    */
    vtstrcpy(realLog, task->dblog);
    ptr = vtstrrchr(realLog, DIRCHAR);
    if (ptr)                      /* keep your path, lose your file */
        ptr[1] = DB_TEXT('\0');
    else
        realLog[0] = DB_TEXT('\0');

    ptr = vtstrrchr(logfile, DIRCHAR);
    if (ptr)                      /* lose their path, keep their file */
        ++ptr;
    else
        ptr = (DB_TCHAR *) logfile; /* cast kills const/volatile warning */

    vtstrcat(realLog, ptr);               /* add their file to your path */
    
    /* ptr to const data, not const ptr to data--compiler warning */
    logfile = (const DB_TCHAR *) realLog;

    /* allocate memory for index page buffer */
    if ((ixbuf = (long *) psp_getMemory(IX_EPP * sizeof(F_ADDR), 0)) == NULL)
        goto memerr;

    /* open log file */
    if ((lfh = open_b(logfile, O_RDWR, PSP_FLAG_SYNC, task)) == NULL)
        goto logerr;

    /* read number of files in logged db */
    if (psp_fileSeekRead(lfh, 0, &nfiles, sizeof(short)) < (int) sizeof(short))
        goto readerr;

    rft = (RFT_ENTRY *) psp_getMemory(nfiles * sizeof(RFT_ENTRY), 0);
    if (rft == NULL)
        goto memerr;

    /* read page size and name of each file in logged db */
    size = sizeof(short) + (FILENMLEN * sizeof(DB_TCHAR));
    for (max_pg_size = fno = 0, rft_ptr = rft; fno < nfiles; ++fno, ++rft_ptr)
    {
        if (psp_fileSeekRead(lfh, sizeof(short) + fno * size, rft_ptr,
                size) < (int) size)
            goto readerr;

        if (rft_ptr->pgsize > max_pg_size)
            max_pg_size = rft_ptr->pgsize;
    }
    
    /* allocate page buffer */
    if ((pgbuf = psp_getMemory(max_pg_size, 0)) == NULL)
        goto memerr;

    /* read root index for each file */
    size = sizeof(short) + size * nfiles;
    for (fno = 0, rft_ptr = rft; fno < nfiles; ++fno, ++rft_ptr)
    {
        if (psp_fileSeekRead(lfh, size + fno * RI_ENTRY_IOSIZE, &rft_ptr->rix,
                RI_ENTRY_IOSIZE) < (int) RI_ENTRY_IOSIZE)
            goto readerr;
    }

    /* read index page ri_bitmaps */
    for (fno = 0, rft_ptr = rft; fno < nfiles; ++fno, ++rft_ptr)
    {
        if (rft_ptr->rix.base)
        {
            bm_size = RI_BITMAP_SIZE(rft_ptr->rix.pg_cnt) * sizeof(int);
            rft_ptr->rix.ri_bitmap = (int *) psp_getMemory(bm_size, 0);
            if (rft_ptr->rix.ri_bitmap == NULL)
                goto memerr;

            if (psp_fileSeekRead(lfh, rft_ptr->rix.base, rft_ptr->rix.ri_bitmap,
                    bm_size) < bm_size)
                goto readerr;
        }
    }

    /* update db files */
    for (fno = 0, rft_ptr = rft; fno < nfiles; ++fno, ++rft_ptr)
    {
        if (rft_ptr->rix.base == 0L)
            continue;
        
        if ((dbf = open_b(rft_ptr->fname, O_RDWR, PSP_FLAG_SYNC, task)) == NULL)
            goto fileerr;
        
        ix_pg_cnt = IX_SIZE(rft_ptr->rix.pg_cnt);
        size = rft_ptr->pgsize;
        
        /* read file's index pages */
        for (ip = 0; ip < ix_pg_cnt; ++ip)
        {
            wordpos = (unsigned int) (ip / BUI);
            mask = 1 << (BUI - 1 - (unsigned int) (ip - (F_ADDR) wordpos * BUI));
            ixp_used = rft_ptr->rix.ri_bitmap[wordpos] & mask;
            if (ixp_used)
            {
                /* read index page from log file */
                ixp_addr = rft_ptr->rix.base + RI_BITMAP_SIZE(rft_ptr->rix.pg_cnt) *
                        sizeof(int) + ip * IX_PAGESIZE;

                if (psp_fileSeekRead(lfh, ixp_addr, ixbuf,
                        IX_PAGESIZE) < IX_PAGESIZE)
                    goto readerr;

                /* scan each db page entry on index page */
                for (slot = 0; slot < IX_EPP; ++slot)
                {
                    if (ixbuf[slot])
                    {
                        long curr_time;

                        db_page = (F_ADDR) (ip * IX_EPP + slot + 1);

                        /* read db page from log file */
                        if (psp_fileSeekRead(lfh, ixbuf[slot], pgbuf,
                                size) < (int) size)
                            goto readerr;
        
#ifdef DB_DEBUG
                        if (task->db_status == S_OKAY && (task->db_debug & PAGE_CHECK))
                        {
                            DB_ADDR page_id;

                            ENCODE_DBA(min(fno, 255), db_page, &page_id);
                            if (memcmp(pgbuf, &page_id, sizeof(DB_ADDR)) != 0)
                            {
                                dberr(SYS_INVLOGPAGE);
                                goto readerr;
                            }
                        }
#endif

                        /* write db page to db file */
                        curr_time = psp_timeSecs(); /* get the current time */
                        memcpy(pgbuf, &curr_time, sizeof(curr_time));
                        if (psp_fileSeekWrite(dbf, db_page * size, pgbuf,
                                size) < (int) size)
                            goto writeerr;
                    }
                }
            }
        }

        /* read page zero from log file */
        if (rft_ptr->rix.pz_modified)
        {
            pz_base = rft_ptr->rix.base + RI_BITMAP_SIZE(rft_ptr->rix.pg_cnt) *
                      sizeof(int);
            pz_base += IX_SIZE(rft_ptr->rix.pg_cnt) * IX_PAGESIZE;
          
            if (psp_fileSeekRead(lfh, pz_base, pgbuf, PGZEROSZ) <
                    (int) PGZEROSZ)
                goto readerr;
            
            /* write page zero to db file */
            if (psp_fileSeekWrite(dbf, 0, pgbuf, PGZEROSZ) < (int) PGZEROSZ)
                goto writeerr;
        }

        psp_fileClose(dbf);
        psp_freeMemory(rft_ptr->rix.ri_bitmap, 0);
    }

    psp_freeMemory(rft, 0);
    psp_freeMemory(pgbuf, 0);
    psp_freeMemory(ixbuf, 0);
    psp_fileClose(lfh);

    if (task->db_status == S_OKAY && tafbuf.user_count > 0)
        --tafbuf.user_count;

    goto exit;

fileerr:
    dberr(S_NOFILE);
    goto exit;

memerr:
    dberr(S_NOMEMORY);
    goto exit;

logerr:
    goto exit;

readerr:
    dberr(S_BADREAD);
    goto exit;

writeerr:
    dberr(S_BADWRITE);
    goto exit;

exit:
    task->dboptions |= rd_opt;
    return (task->db_status);
}


