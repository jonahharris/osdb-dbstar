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

typedef union INIT_PAGE_U
{
    struct
    {
        F_ADDR dchain;                   /* delete chain pointer */
        F_ADDR next;                     /* next page or record slot */
        DB_ULONG timestamp;              /* file's timestamp value */
        time_t cdate;                    /* creation date,time */
        time_t bdate;                    /* date/time of last backup */
        DB_TCHAR vdb_id[LENVDBID];       /* db.* id mark */
    } pg0;
    struct
    {
        /* page 1 of key file */
        time_t chg_date;                 /* date of last page change */
        /* char arrays are used to avoid alignment problems */
        char init_int[SHORT_SIZE];       /* # filled slots on key file;
                                            System record # on data file */
        char init_addr[DB_ADDR_SIZE];    /* NONE node pointer on key file;
                                            System record db_addr on data file */
        char init_crts[LONG_SIZE];       /* if system record is timestamped */
        char init_upts[LONG_SIZE];       /* if system record is timestamped */
    } pg1;
} INIT_PAGE;

/* ======================================================================
    Database initialization function
*/
int INTERNAL_FCN dinitialize(DB_TASK *task, int dbn)
{
    FILE_NO fno;

    if (task->dboptions & READONLY)
        return (dberr(S_READONLY));

    if (!task->dbopen)
        return (dberr(S_DBOPEN));

    /* make sure we have the necessary locks */
    if (task->dbopen == 1)           /* shared mode */
    {
        FILE_NO offset = task->curr_db_table->ft_offset;
        for (fno = 0; fno < DB_REF(Size_ft); ++fno)
        {
            if (!task->excl_locks[fno + offset])
                return (dberr(S_EXCLUSIVE));
        }
    }

    /* initialize db files in task->file_table */
    for (fno = 0; fno < DB_REF(Size_ft); ++fno)
    {
        if (dinitfile(fno, task, dbn) != S_OKAY)
            break;
    }

    return (task->db_status);
}



/* ======================================================================
    Initialize database file
*/
int INTERNAL_FCN dinitfile(
    FILE_NO fno,            /* file table entry of file to be initialized */
    DB_TASK *task,
    int dbn)
{
    INIT_PAGE    *page;
    DB_ADDR       addr;
    DB_ULONG      ts;
    short         rno,
                  rec;
    short         pgsize;
    FILE_ENTRY   *fptr;
    RECORD_ENTRY *rec_ptr;
    PSP_FH        dbfile;
    const SG     *sg = NULL;

    if (!task->dbopen)
        return (dberr(S_DBOPEN));

    if (task->dboptions & READONLY)
        return (dberr(S_READONLY));

    /* make sure file number is within proper range [435] */
    if (fno < 0 || fno > DB_REF(Size_ft))
        return (dberr(S_NOFILE));

    fno += task->curr_db_table->ft_offset;
    fptr = &task->file_table[fno];
    pgsize = fptr->ft_pgsize;

    if (task->dbopen == 1 && !task->excl_locks[fno])
        return (dberr(S_EXCLUSIVE));

    /* If file is open - close it */
    if (fptr->ft_status == OPEN)
        dio_close(fno, task);

    /* clear file's pages from the cache */
    dio_clrfile(fno, task);

    dbfile = psp_fileOpen(fptr->ft_name, O_TRUNC | O_RDWR,
                          PSP_FLAG_SYNC | PSP_FLAG_DENYRW);
    if (!dbfile)
        return (dberr(S_NOFILE));

    if (fptr->ft_initial)
    {
        if (psp_fileSetSize(dbfile, fptr->ft_initial * pgsize) < 0)
            return (dberr(S_NOFILE));
    }

    if ((page = (INIT_PAGE *) psp_cGetMemory(pgsize, 0)) == NULL)
        return (dberr(S_NOMEMORY));

    page->pg0.cdate = psp_timeSecs();
    ddbver(DB_TEXT("%V"), page->pg0.vdb_id, 48);
    page->pg0.bdate = 0;

    sg = task->sgs[fno];

    if (fptr->ft_type == KEY)
    {
        /* initialize key file */
        page->pg0.dchain = DCH_NONE;
        page->pg0.next = (F_ADDR) 2;
        page->pg0.timestamp = (DB_ULONG) 0;   /* not really used by key file */

        if (sg)
            (*sg->enc)(sg->data, page, PGZEROSZ);

        if (psp_fileSeekWrite(dbfile, 0, page, pgsize) != pgsize)
            dberr(S_BADWRITE);
        else
        {
            memset(page, '\0', pgsize);
            page->pg1.chg_date = psp_timeSecs();

            /* node 1, current # of filled slots */
            memset(page->pg1.init_int, '\0', sizeof(short));

            /* node 1, NONE page pointer */
            addr = DBA_NONE;
            memcpy(page->pg1.init_addr, &addr, sizeof(DB_ADDR));

            if (sg)
                (*sg->enc)(sg->data, page, pgsize);

            if (psp_fileSeekWrite(dbfile, pgsize, page, pgsize) != pgsize)
                dberr(S_BADWRITE);
        }
    }
    else
    {
        /* initialize data file */
        page->pg0.dchain = (F_ADDR) 0;
        page->pg0.timestamp = (DB_ULONG) 1;

        /* check to see if this file contains a system record */
        for (rec = 0, rec_ptr = task->record_table; rec < task->size_rt;
             ++rec, ++rec_ptr)
        {
            if (rec_ptr->rt_fdtot == -1 && rec_ptr->rt_file == fno)
            {
                /* this file contains the system record */
                page->pg0.next = (F_ADDR) 2;
                if (sg)
                    (*sg->enc)(sg->data, page, PGZEROSZ);

                if (psp_fileSeekWrite(dbfile, 0, page, pgsize) < pgsize)
                {
                    dberr(S_BADWRITE);
                    break;
                }

                memset(page, '\0', pgsize);
                page->pg1.chg_date = psp_timeSecs();
                ENCODE_DBA(NUM2EXT(fno, ft_offset), 1L, &addr);
                memcpy(page->pg1.init_addr, &addr, sizeof(DB_ADDR));
                rno = (short) NUM2EXT(rec, rt_offset);
                memcpy(page->pg1.init_int, &rno, sizeof(short));
                if (rec_ptr->rt_flags & TIMESTAMPED)
                {
                    /* timestamp system record */
                    ts = (DB_ULONG) 1;
                    memcpy(page->pg1.init_crts, &ts, sizeof(DB_ULONG));
                    memcpy(page->pg1.init_upts, &ts, sizeof(DB_ULONG));
                }

                if (sg)
                    (*sg->enc)(sg->data, page, pgsize);

                if (psp_fileSeekWrite(dbfile, pgsize, page, pgsize) < pgsize)
                    dberr(S_BADWRITE);

                break;
            }
        }

        if (rec == task->size_rt && task->db_status == S_OKAY)
        {
            page->pg0.next = (F_ADDR) 1;
            if (sg)
                (*sg->enc)(sg->data, page, PGZEROSZ);

            if (psp_fileSeekWrite(dbfile, 0, page, pgsize) < pgsize)
                dberr(S_BADWRITE);
        }
    }

    /* close database file */
    psp_fileClose(dbfile);
    
    /* re-read file header */
    if (task->db_status == S_OKAY)
    {
        dio_pzread(fno, task);
        dio_close(fno, task);
    }

    psp_freeMemory(page, 0);
    return (task->db_status);
}


