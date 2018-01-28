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

short vrand(void);


/* ======================================================================
    Open a binary file for shared access.
    File access should be done with db_ functions.
*/
PSP_FH EXTERNAL_FCN open_b(
    const DB_TCHAR *filenm,
    unsigned int    flags,
    unsigned short  xflags,
    DB_TASK        *task)
{
    PSP_FH fh;
    int    err;

    if (task->dboptions & READONLY)
    {
        flags &= ~O_RDWR;
        flags |= O_RDONLY;
    }

    if ((fh = psp_fileOpen(filenm, flags, xflags)) == NULL &&
          ((err = psp_errno()) == EACCES || err == EPIPE))
    {
        dberr(S_EACCESS);
        return NULL;
    }

    return fh;
}


/* ======================================================================
    Cause a file's contents to be commited to disk

    Use operating system specific function to force disk write on systems
    where there may be a write cache.
*/
void EXTERNAL_FCN commit_file(
    PSP_FH   file_handle,            /* OS file handle ( from open ) */
    DB_TASK *task)
{
    /* Only call fsync if the SYNCFILES option is on */
    if (task->dboptions & SYNCFILES)
        psp_fileSync(file_handle);
}


/*************************************************************************/

void INTERNAL_FCN adjust_naptime (int adj, DB_TASK *task)
{
    if (adj == BY_NAP_SUCCESS)
    {
        task->nap_factor *= SUCCESS_FACTOR;
        task->nap_factor = task->nap_factor < MIN_FACTOR ? MIN_FACTOR : task->nap_factor;
    }
    else
    {
        task->nap_factor *= FAILURE_FACTOR;
        task->nap_factor = task->nap_factor > MAX_FACTOR ? MAX_FACTOR : task->nap_factor;
    }
}

/**************************************************************************/

void INTERNAL_FCN naptime(DB_TASK *task)
{
    int stat = S_OKAY;

    if (task->nap_factor < (2 * ONE_FACTOR))
    {
        /* i.e == ONE_FACTOR; == is a bad compare for floating point numbers.
            The formality of the nap is necessary to allow a second user to
            get access to the dbl file.  But when only one user is in the
            database, nap for as little as possible.  dbl_open() sets task->nap_factor
            to and from ONE_FACTOR.
        */
        psp_sleep(1L);
    }
    else
    {
        /* sleep from 0.005 sec to 2.25 secs, based on history of locking
            with a bit of randomness */
        psp_sleep((long)(1000.0 * task->nap_factor) + (long)(vrand() % 250));
    }

    if (stat != S_OKAY)
        task->db_status = stat;
}

/* Use our own rand() function because the application program might need
    the real one with a repeatable sequence.

    This is a multiplicative congruential random number generator which has
    a period of 2^32 to return successive pseudo-random numbers in the range
    from 0 to 2^15 - 1.

*/
short vrand()
{
    static long vseed = 0L;
    
    if (vseed == 0L)
        vseed = psp_seed();

    vseed = 0x015a4e35L * vseed + 1;
    return (short)((unsigned short)(vseed >> 16) & 0x7fff);
}



