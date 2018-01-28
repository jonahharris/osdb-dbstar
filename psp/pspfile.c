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

/* pspfile.c - Contains generic file functions */

#include "psp.h"
#include "pspint.h"

#define PSP_STATE_EMPTY  0
#define PSP_STATE_CLOSED 1
#define PSP_STATE_OPEN   2

#define PSP_BLKSIZE 4096
typedef struct {
    size_t offset;
    char   block[PSP_BLKSIZE];
} PSPSTREAM;

typedef struct {
    DB_TCHAR      *f_name;
    PSP_FDESC      f_desc;
    FILE          *f_fhandle;
    PSP_SEM        f_sem;
    PSPSTREAM     *f_stream;
    psp_thread_t   f_tid;
    unsigned int   f_flags;
    unsigned short f_xflags;
    short          f_count;
    char           f_state;
    char           f_active;
    char           f_ru;
} PSPFTAB;

#define FTABINCR 32

static PSPFTAB  **ftab = NULL;
static PSP_MEMTAG fileTag = 0;
static long       handleLimit = 0;
static long       filesOpen = 0;
static size_t     handles = 0;
static size_t     ftabSize = 0;
static size_t     lruNext = 0;
static PSP_SEM    fileSem = NO_PSP_SEM;

/* ==========================================================================
    Cleanup a file table entry
*/
static void cleanupFile(
    register PSPFTAB *p)
{
    if (p->f_xflags & PSP_FLAG_SYNC)
        psp_syncDelete(p->f_sem);

    p->f_state = PSP_STATE_EMPTY;
    if (p->f_stream)
        psp_zFreeMemory(&p->f_stream, fileTag);

    if (p->f_name)
        psp_zFreeMemory(&p->f_name, fileTag);
}

/* ==========================================================================
    Open a file that is currently closed.
*/
static short reopenFile(
    register PSPFTAB *p)
{
    short rv;

    if (p->f_xflags & PSP_FLAG_FPRINTF) {
        if (p->f_flags & O_TRUNC)
            psp_fileRemove(p->f_name);

        if ((p->f_fhandle = vtfopen(p->f_name, DB_TEXT("a+t"))) == NULL) {
            cleanupFile(p);
            return PSP_FAILED;
        }
    }
    else {
        rv = psp_flOpen(p->f_name, p->f_flags, p->f_xflags, &p->f_desc);
        if (!p->f_desc) {
            cleanupFile(p);
            return PSP_FAILED;
        }
    }

    filesOpen++;
    p->f_state = PSP_STATE_OPEN; 
    return PSP_OKAY;
}

/* ==========================================================================
    Select the least recently used file and close it to reuse the file handle
*/
static void closeOneOpenFile(
    void)
{
    register PSPFTAB *p = ftab[lruNext];

    /* the lruNext index will cycle around the length of the file table in
        search of a handle which has not been "recently used".  This is the
        only place where the f_ru flag will be cleared.  It is set when the
        file is opened or accessed.  Thus, a file will be selected for closing
        by this algorithm only when this algorithm has cleared the flag and
        then found it cleared a second time around. */

    for ( ; ; ) {
        if (p) {
            if (p->f_state == PSP_STATE_OPEN && !p->f_active &&
                    !(p->f_xflags & PSP_FLAG_STICKY)) {
                if (!p->f_ru)
                    break;
                else
                    p->f_ru = 0;
            }
        }

        if (++lruNext >= ftabSize)
            lruNext = 0;

        p = ftab[lruNext];
    }

    /* this file may now be closed */
    if (p->f_xflags & PSP_FLAG_FPRINTF)
        fclose(p->f_fhandle); 
    else
        psp_flClose(p->f_desc);

    filesOpen--;
    p->f_state = PSP_STATE_CLOSED;
    if (++lruNext >= ftabSize)
        lruNext = 0;
}

/* ==========================================================================
    Begin access to a file
*/
static void BeginFileAccess(
    register PSPFTAB *p)
{
    if (p->f_xflags & PSP_FLAG_SYNC)
        psp_syncEnterExcl(p->f_sem);

    if (!handleLimit)
        return;

    p->f_active++;
    p->f_ru = 1;
    if (p->f_state != PSP_STATE_OPEN) {
        psp_syncEnterExcl(fileSem);
        if (filesOpen >= handleLimit)
            closeOneOpenFile();

        psp_syncExitExcl(fileSem);

        reopenFile(p);
    }
}

/* ==========================================================================
    End access to a file
*/
static void EndFileAccess(
    register PSPFTAB *p)
{
    if (handleLimit)
        p->f_active--;

    if (p->f_xflags & PSP_FLAG_SYNC)
        psp_syncExitExcl(p->f_sem);
}

/* ==========================================================================
    Initialize the PSP file handling subsytem
*/
int psp_fileInit(
    void)
{
    filesOpen = 0;
    handles = 0;
    lruNext = 0;

    if ((fileSem = psp_syncCreate(PSP_MUTEX_SEM)) == NO_PSP_SEM)
        return PSP_FAILED;

    if ((fileTag = psp_createTag(NULL, PSP_TAG_NOSEM)) == NULL_MEMTAG) {
        psp_syncDelete(fileSem);
        return PSP_FAILED;
    }

    ftabSize = FTABINCR;
    ftab = psp_cGetMemory(FTABINCR * sizeof(PSPFTAB *), fileTag);
    if (ftab == NULL) {
        psp_freeTagMemory(fileTag, 1);
        psp_syncDelete(fileSem);
        return PSP_FAILED;
    }

    return PSP_OKAY;
}

/* ==========================================================================
    Terminate the PSP file handling subsytem
*/
void psp_fileTerm(
    void)
{
    size_t            ii;
    register PSPFTAB *p;

    for (ii = 0; ii < handles; ii++) {
        p = ftab[ii];
        if (!p || p->f_state == PSP_STATE_EMPTY)
            break;

        if (p->f_state == PSP_STATE_OPEN) {
            if (p->f_xflags & PSP_FLAG_FPRINTF)
                fclose(p->f_fhandle);
            else
                psp_flClose(p->f_desc);
        }

        if (p->f_xflags & PSP_FLAG_SYNC)
            psp_syncDelete(p->f_sem);
    }

    psp_freeTagMemory(fileTag, 1);
    psp_syncDelete(fileSem);
}

/* ==========================================================================
    Open a file
*/
PSP_FH psp_fileOpen(
    const DB_TCHAR *name,
    unsigned int    flags,
    unsigned short  xflags)
{
    register size_t   ii;
    register PSPFTAB *p = NULL;

    if (!psp_inited)
        return NULL;

    if (flags & O_TRUNC)
        flags |= O_CREAT;  /* O_TRUNC requires O_CREAT */

    psp_syncEnterExcl(fileSem);

    for (ii = 0; ii < handles; ii++) {
        p = ftab[ii];
        if (!p || p->f_state == PSP_STATE_EMPTY)
            continue;

        if (vtstrcmp(p->f_name, name) == 0) {
            if (p->f_xflags != xflags || p->f_flags != flags) {
                vtprintf(DB_TEXT("Already open file (%s) opened with different flags\n"), name);
                psp_syncExitExcl(fileSem);
                return NULL;
            }

            p->f_count++;
            return p;
        }
    }

    do {
        if (handles < ftabSize)
            p = ftab[handles++] = psp_cGetMemory(sizeof(PSPFTAB), fileTag);
        else {
            for (ii = 0; ii < ftabSize; ii++) {
                p = ftab[ii];
                if (!p || p->f_state == PSP_STATE_EMPTY)
                     break;
            }

            if (ii == ftabSize) {
                ftab = psp_cExtendMemory(ftab, (ftabSize + FTABINCR) * sizeof(PSPFTAB *),
                        FTABINCR * sizeof(PSPFTAB *), fileTag);
                ftabSize += FTABINCR;
                p = ftab[ii];
            }
        }
    } while (!p);

    p->f_flags  = flags;
    p->f_xflags = xflags;
    p->f_state  = PSP_STATE_CLOSED;
    p->f_count  = 1;
    p->f_ru     = 1;
    p->f_name   = psp_strdup(name, fileTag);
    p->f_sem    = (xflags & PSP_FLAG_SYNC) ? psp_syncCreate(PSP_MUTEX_SEM) : NO_PSP_SEM;
    p->f_tid    = psp_threadId();

    if (xflags & PSP_FLAG_STREAM)
        p->f_stream = psp_getMemory(sizeof(PSPSTREAM), fileTag);
    else
        p->f_stream = NULL;

    if (reopenFile(p) != PSP_OKAY)
         p = NULL;
    else
         p->f_flags &= O_TRUNC;  /* must turn of O_TRUNC after initial open */

    psp_syncExitExcl(fileSem);

    return p;
}

/* ==========================================================================
    Close a file
*/
void psp_fileClose(
    PSP_FH handle)
{
    register PSPFTAB *p = (PSPFTAB *) handle;

    psp_syncEnterExcl(fileSem);

    if (--p->f_count == 0) {
        if (p->f_state = PSP_STATE_OPEN) {
            if (p->f_xflags & PSP_FLAG_FPRINTF)
                fclose(p->f_fhandle);
            else
                psp_flClose(p->f_desc);

            filesOpen--;
        }

        cleanupFile(p);
    }

    psp_syncExitExcl(fileSem);
}

/* ==========================================================================
    Write formatted output to a file
*/
int psp_filePrintf(
    PSP_FH          handle,
    const DB_TCHAR *format,
    ...)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    int               bytes;
    va_list           mark;

    if (!(p->f_xflags & PSP_FLAG_FPRINTF)) {
        /* TBD: Log out warning about accessing binary file */
        vtprintf(DB_TEXT("File not opened with printf capability\n"));
    }

    if (p->f_stream) {
        /* TBD: Log out warning about accessing stream file */
        vtprintf(DB_TEXT("Invalid file mode, file is a stream\n"));
    }

    BeginFileAccess(p);

    va_start(mark, format);
    bytes = vvftprintf(p->f_fhandle, format, mark);
    va_end(mark);

    EndFileAccess(p);

    return bytes;
}

/* ==========================================================================
    Seek to a specified location in a file
*/
void psp_fileSeek(
    PSP_FH handle,
    size_t addr)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    int               bytes;

    if (p->f_xflags & PSP_FLAG_SYNC) {
        /* TBD: Log out warning about accessing file needing synchronization */
        vtprintf(DB_TEXT("Invalid file mode, file was opened with PSP_FLAG_SYNC\n"));
    }

    BeginFileAccess(p);

    if (p->f_stream) {
        psp_flSeek(p->f_desc, (addr / PSP_BLKSIZE) * PSP_BLKSIZE, SEEK_SET);
        bytes = psp_flRead(p->f_desc, p->f_stream->block, PSP_BLKSIZE);
        if (bytes < 0)
            bytes = 0;

        if (bytes < PSP_BLKSIZE)
            p->f_stream->block[bytes] = DB_TEXT('\0');

        addr %= PSP_BLKSIZE;
        p->f_stream->offset = (size_t) bytes < addr ? (size_t) bytes : addr;
    }
    else
        psp_flSeek(p->f_desc, addr, SEEK_SET);

    EndFileAccess(p);
}

/* ==========================================================================
    Write to a file
*/
int psp_fileWrite(
    PSP_FH      handle,
    const void *buf,
    size_t      size)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    int               bytes;

    if (p->f_xflags & PSP_FLAG_SYNC) {
        /* TBD: Log out warning about accessing file needing synchronization */
        vtprintf(DB_TEXT("Invalid file mode, file was opened with PSP_FLAG_SYNC\n"));
    }

    if (p->f_stream) {
        /* TBD: Log out warning about accessing stream file */
        vtprintf(DB_TEXT("Invalid file mode, file is a stream\n"));
    }

    if (size == 0)
         return 0;

    BeginFileAccess(p);

    if ((bytes = psp_flWrite(p->f_desc, buf, size)) == -1) {
        /* TBD: Log out error about write failure */
        vtprintf(DB_TEXT("Write failed, errno = %d\n"), errno);
    }

    EndFileAccess(p);

    return bytes;
}

/* ==========================================================================
    Read from a file
*/
int psp_fileRead(
    PSP_FH  handle,
    void   *buf,
    size_t  size)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    int               bytes;

    if (p->f_xflags & PSP_FLAG_SYNC) {
        /* TBD: Log out warning about accessing file needing synchronization */
        vtprintf(DB_TEXT("Invalid file mode, file was opened with PSP_FLAG_SYNC\n"));
    }

    if (p->f_stream) {
        /* TBD: Log out warning about accessing stream file */
        vtprintf(DB_TEXT("Invalid file mode, file is a stream\n"));
    }

    if (size == 0)
         return 0;

    BeginFileAccess(p);

    if ((bytes = psp_flRead(p->f_desc, buf, size)) == -1) {
        /* TBD: Log out error about write failure */
        vtprintf(DB_TEXT("Read failed, errno = %d\n"), errno);
    }

    EndFileAccess(p);

    return bytes;
}

/* ==========================================================================
    Write to a specified location in a file
*/
int psp_fileSeekWrite(
    PSP_FH      handle,
    size_t      addr,
    const void *buf,
    size_t      size)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    short             rv = 0;
    int               bytes;

    if (!(p->f_xflags & PSP_FLAG_SYNC)) {
        /* TBD: Log out warning about accessing a non-synchronized file */
        vtprintf(DB_TEXT("Invalid file mode, file must be opened with PSP_FLAG_SYNC\n"));
    }

    if (p->f_stream) {
        /* TBD: Log out warning about accessing stream file */
        vtprintf(DB_TEXT("Invalid file mode, file is a stream\n"));
    }

    if (size == 0)
        return 0;

    BeginFileAccess(p);

    bytes = psp_flSeekWrite(p->f_desc, addr, buf, size);

    EndFileAccess(p);

    if (bytes == -1) {
        /* TBD: Log out error on failure */
        vtprintf(DB_TEXT("Failed to seek and write, errno = %d\n"), errno);
    }

    return bytes;
}

/* ==========================================================================
    Read from a specified location in a file
*/
int psp_fileSeekRead(
    PSP_FH handle,
    size_t addr,
    void  *buf,
    size_t size)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    int               bytes;

    if (!(p->f_xflags & PSP_FLAG_SYNC)) {
        /* TBD: Log out warning about accessing a non-synchronized file */
        vtprintf(DB_TEXT("Invalid file mode, file must be opened with PSP_FLAG_SYNC\n"));
    }

    if (p->f_stream) {
        /* TBD: Log out warning about accessing stream file */
        vtprintf(DB_TEXT("Invalid file mode, file is a stream\n"));
    }

    if (size == 0)
        return 0;

    BeginFileAccess(p);

    bytes = psp_flSeekRead(p->f_desc, addr, buf, size);

    EndFileAccess(p);

    if (bytes == -1) {
        /* TBD: Log out error on failure */
        vtprintf(DB_TEXT("Failed to seek and read, errno = %d\n"), errno); 
    }

    return bytes;
}

/* ==========================================================================
   See if we need to read any more data from the file
*/
static int check_block(
    PSP_FDESC  desc,
    PSPSTREAM *stream)
{
    int bytes;

    if (stream->offset == PSP_BLKSIZE) {
        bytes = psp_flRead(desc, stream->block, PSP_BLKSIZE);
        if (bytes == 0)
            return 0;

        if (bytes != PSP_BLKSIZE)
            stream->block[bytes] = '\0';

        stream->offset = 0;
    }
    
    return 1;
}

/* ==========================================================================
   Return the next string from a file
*/
char *psp_fileGets(
    PSP_FH handle,
    char  *buf,
    size_t size)
{
    register PSPFTAB   *p = (PSPFTAB *) handle;
    register PSPSTREAM *stream;
    char               *cur;
    char               *cp = buf;
    char               *pp;
    size_t              len = 0;

    if (!p->f_stream) {
        /* TBD: Log out warning about invalid file open mode */
        vtprintf(DB_TEXT("Invalid file mode, file is not a stream\n"));
    }

    BeginFileAccess(p);

    stream = p->f_stream;
    buf[0] = '\0';
    do {
        if (!check_block(p->f_desc, stream))
            break;

        cur = stream->block + stream->offset;
        if ((pp = strchr(cur, '\n')) != NULL)
            len = pp - cur + 1;
        else
            len = PSP_BLKSIZE;

        if (len > size - 1)
            len = size - 1;

        strncpy(cp, cur, len);
        cp[len] = '\0';

        len = strlen(cp);
        cp += len;
        size -= len;
        stream->offset += len;
    } while (!pp && len && stream->offset == PSP_BLKSIZE);

    EndFileAccess(p);

    if (!len)
        return NULL;

    return buf;
}

/* ==========================================================================
    Return the next character from a file.
*/
char psp_fileGetc(
    PSP_FH handle)
{
    register PSPFTAB   *p = (PSPFTAB *) handle;
    register PSPSTREAM *stream;
    char                ch = (char) -1;

    if (!p->f_stream) {
        /* TBD: Log out warning about invalid file open mode */
        vtprintf(DB_TEXT("Invalid file mode, file is not a stream\n"));
    }

    BeginFileAccess(p);

    stream = p->f_stream;
    if (check_block(p->f_desc, stream)) {
        ch = stream->block[stream->offset];
        if (ch)
            stream->offset++;
    }

    EndFileAccess(p);

    return ch;
}

/* ==========================================================================
    Return the length of a file.
*/
size_t psp_fileLength(
    PSP_FH handle)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    size_t            len;

    BeginFileAccess(p);

    len = psp_flSize(p->f_desc);

    EndFileAccess(p);

    return len;
}

/* ==========================================================================
   Set the current size of the file
*/
int psp_fileSetSize(
    PSP_FH handle,
    size_t size)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    int               ret;

    BeginFileAccess(p);

    ret = psp_flSetSize(p->f_desc, size);

    EndFileAccess(p);

    return ret;
}

/* ==========================================================================
    Lock a file
*/
short psp_fileLock(
    PSP_FH handle)
{
    short             ret;
    register PSPFTAB *p = (PSPFTAB *) handle;

    BeginFileAccess(p);

    if (p->f_xflags & PSP_FLAG_SYNC)
       psp_flSeek(p->f_desc, 0, SEEK_SET);

    ret = psp_flLock(p->f_desc);

    EndFileAccess(p);

    return ret;
}

/* ==========================================================================
    Unlock a file
*/
void psp_fileUnlock(
    PSP_FH handle)
{
    register PSPFTAB *p = (PSPFTAB *) handle;

    BeginFileAccess(p);

    if (p->f_xflags & PSP_FLAG_SYNC)
       psp_flSeek(p->f_desc, 0, SEEK_SET);

    psp_flUnlock(p->f_desc);

    EndFileAccess(p);
}

/* ==========================================================================
    Synchronize writes to a file
*/
void psp_fileSync(
    PSP_FH handle)
{
    register PSPFTAB *p = (PSPFTAB *) handle;

    BeginFileAccess(p);

    psp_flSync(p->f_desc);

    EndFileAccess(p);
}

/* ==========================================================================
   Return the last access time of a file
*/
long psp_fileLastAccess(
    PSP_FH handle)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    long              ret;

    BeginFileAccess(p);

    ret = psp_flLastAccess(p->f_desc);

    EndFileAccess(p);

    return ret;
}

/* ==========================================================================
   Return last modified time
*/
long psp_fileModTime(
    PSP_FH handle)
{
    register PSPFTAB *p = (PSPFTAB *) handle;
    long              ret;

    BeginFileAccess(p);

    ret = psp_flModTime(p->f_desc);

    EndFileAccess(p);

    return ret;
}    

/* ==========================================================================
   Copy the contents of a file to a second file
*/
int psp_fileCopy(
    const DB_TCHAR *src,
    const DB_TCHAR *dest)
{
    PSP_FH hSrc;
    PSP_FH hDest;
    char  *buf;
    int    bytes;

    if (vtstrcmp(src, dest) == 0)
        return PSP_OKAY;

    psp_fileRemove(dest);
    if ((hSrc = psp_fileOpen(src, O_RDONLY, 0)) == NULL)
        return PSP_FAILED;

    if ((hDest = psp_fileOpen(dest, O_CREAT|O_RDWR|O_EXCL, 0)) == NULL) {
        psp_fileClose(hSrc);
        return PSP_FAILED;
    }

    buf = psp_getMemory(PSP_BLKSIZE, 0);

    while ((bytes = psp_fileRead(hSrc, buf, PSP_BLKSIZE)) > 0)
        psp_fileWrite(hDest, buf, PSP_BLKSIZE);

    psp_fileClose(hSrc);
    psp_fileClose(hDest);
    psp_freeMemory(buf, 0);

    return PSP_OKAY;
}

/* ==========================================================================
   Move the contents of a file to a second file
*/
int psp_fileMove(
    const DB_TCHAR *src,
    const DB_TCHAR *dest)
{
    int stat = PSP_OKAY;

    if (vtstrcmp(src, dest) == 0)
        return PSP_OKAY;

    psp_fileRemove(dest);
    if (psp_fileRename(src, dest)) {
        if ((stat = psp_fileCopy(src, dest)) == PSP_OKAY)
            psp_fileRemove(src);
    }

    return stat;
}

/* ==========================================================================
   Return the current file handle limit
*/
long psp_fileHandleLimit(
    void)
{
    return handleLimit;
}

/* ==========================================================================
   Set the file handle limit
*/
void psp_fileSetHandleLimit(
    long limit)
{
    handleLimit = limit;
}
