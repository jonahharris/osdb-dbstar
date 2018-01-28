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
#include "rntmint.h"
#include "apinames.h"

#if defined(DB_DEBUG)
/*  Uncomment the following line if you want debugging output to be held
    in RAM and flushed when the application terminates. This is useful
    for debugging problems related to timing, which may be affected by
    the delay caused by disk access. Do not use this feature if the
    application crashes, as the debugging output won't be flushed. */

/* #define MEM_DEBUG 1 */
#if defined(MEM_DEBUG)
#define DEBUG_BUFLEN 32768
#endif
#endif

/* ======================================================================
    Called once at the beginning of each external function
*/
int INTERNAL_FCN db_enter(int api, int dbn, DB_TASK *task)
{
    if (task == NULL)
        return S_INVTASK;

    /* Validate task pointer */
    if (ntask_check(task) < 0)
        return S_INVTASK;

    /* Check for reentrant use of task - this might happen if two
       threads shared the same DB_TASK, which is not allowed. It
       would also happen if the application called any d_ functions
       from its database error handler.
    */
    if (task->inDBSTAR)
        return (task->db_status = S_REENTER);

    /* db_status should be set to S_OKAY here (on entry to all d_ 
       functions), and nowhere else, unless to overwrite an error
       value which is not applicable.
    */
    task->db_status = S_OKAY;

    /* reset error info - this gets set when dberr() is called */
    task->errnum = 0;
    task->err_ex = 0;
    task->errno_c = 0;
    task->errfile = NULL;
    task->errline = 0;

    API_ENTER(api);

#ifdef DBSTAT
    task->gen_stats.dbenter++;
#endif

    if (dbn != VOID_DB)
    {
        if (!task->dbopen)
            return (dberr(S_DBOPEN));

        if (dbn == CURR_DB || dbn == ALL_DBS)
            dbn = task->set_db;
        else
        {
            if (dbn < 0 || dbn >= task->no_of_dbs)
                return (dberr(S_INVDB));
        }

        if (dbn != task->curr_db)
        {
            task->db_table[task->curr_db].curr_dbt_rec = task->curr_rec;
            task->db_table[task->curr_db].curr_dbt_ts = task->cr_time;
            
            task->curr_db_table = &task->db_table[task->curr_db = dbn];
            task->curr_rn_table = &task->rn_table[task->curr_db];
            task->curr_rec = task->curr_db_table->curr_dbt_rec;
            task->cr_time  = task->curr_db_table->curr_dbt_ts;
        }
    }

#ifdef DB_DEBUG
    if (task->db_debug & CACHE_CHECK)
        check_cache(DB_TEXT("at db_enter"), task);
#endif

    return (task->db_status);
}


/* ======================================================================
    Called at the end of each external function
*/
int INTERNAL_FCN db_exit(int code, DB_TASK *task)
{
    if (code == S_INVTASK || code == S_REENTER)
        return code;

    /* display any pending error */
    if (task->errnum < 0)
        flush_dberr(task);

    API_EXIT();

    return code;
}

#ifdef DB_TRACE

/* ======================================================================
    Return API function name
    (function must be defined because lockcomm.h may try to link with it)
*/
DB_TCHAR *INTERNAL_FCN dbstar_apistr(int api)
{
    if (api >= DBSTAR_FIRST && api <= DBSTAR_LAST)
        return dbstar_api[api-DBSTAR_FIRST];

    return NULL;
}

/* ======================================================================
    These should be called using the FN_ENTER, FN_RETURN, and FN_EXIT macros
    because they do the check for task->db_trace.
*/
int INTERNAL_FCN api_enter(int fn, DB_TASK *task)
{
    DB_TCHAR *name = dbstar_apistr(fn);
    db_printf(task, DB_TEXT("+ %s\n"), name ? name : DB_TEXT("?"));
    return db_indent(task);
}

int INTERNAL_FCN api_exit(DB_TASK *task)
{
    return db_undent(task);
}

int INTERNAL_FCN fn_enter(DB_TCHAR *name, DB_TASK *task)
{
    db_printf(task, DB_TEXT("+ %s\n"), name ? name : DB_TEXT("?"));
    return db_indent(task);
}

int INTERNAL_FCN fn_exit(DB_TASK *task)
{
    return db_undent(task);
}

static int INTERNAL_FCN db_trace_write(PSP_FH, const DB_TCHAR *, size_t,
                                       DB_TASK *);

/* ======================================================================
*/
int db_printf(DB_TASK *task, DB_TCHAR *fmt, ...)
{
    static PSP_SEM   debug_mutex = NO_PSP_SEM;
    static DB_TCHAR *debug_buf = NULL;
    static DB_TCHAR *debug_ptr = NULL;
    static unsigned int debug_len = 0;
    static DB_TCHAR *buf = NULL;
    static DB_TCHAR name[FILENMLEN] = DB_TEXT("");
    static PSP_FH trace_fd = NULL;
    static off_t last_len = (off_t)-1;
    static int eol = TRUE;
    int stat = S_OKAY;

    if (debug_mutex == NO_PSP_SEM)
        debug_mutex = psp_syncCreate(PSP_MUTEX_SEM);

    psp_syncEnterExcl(debug_mutex);

    if (!fmt)  /* cleanup */
    {
#ifdef MEM_DEBUG
        if (debug_buf)
        {
            if (!trace_fd)
                trace_fd = psp_fileOpen(name, O_WRONLY | O_CREAT, PSP_FLAG_DENYNO);

            if (trace_fd)
            {
                db_trace_write(trace_fd, debug_buf, debug_len, task);
                psp_fileClose(trace_fd);
            }

            trace_fd = NULL;
            psp_freeMemory(debug_buf, 0);
            debug_buf = debug_ptr = NULL;
            debug_len = 0;
        }
#else
        if (trace_fd != NULL)
            psp_fileClose(trace_fd);

        trace_fd = NULL;
#endif

        if (buf)
            psp_zFreeMemory(&buf, 0);

        psp_syncExitExcl(debug_mutex);
        psp_syncDelete(debug_mutex);
        debug_mutex = NO_PSP_SEM;
      
        return (stat == S_OKAY) ? TRUE : FALSE;
    }

    /*
        initialize
    */
    if (!buf)
    {
        if ((buf = psp_cGetMemory(512 * sizeof(DB_TCHAR), 0)) == NULL) {
            psp_syncExitExcl(debug_mutex);
            return FALSE;
        }
    }

    if (!name[0])
    {
        DB_TCHAR *path, *last;

        path = psp_getenv(DB_TEXT("DBTPATH"));
        if (path && *path)
        {
            vtstrcpy(name, path);
            last = &name[vtstrlen(name) - 1];
            if (*last++ != DIRCHAR)
                *last++ = DIRCHAR;
        }
        else
            last = vtstrcpy(name, DB_TEXT(""));

        vtstrcat(last, DB_TEXT("db.star.out"));
    }   

#ifdef MEM_DEBUG
    if (!debug_buf)
    {
        debug_buf = psp_cGetMemory(DEBUG_BUFLEN * sizeof(DB_TCHAR), 0);
        if (debug_buf == NULL)
        {
            psp_syncExitExcl(debug_mutex);
            return FALSE;
        }

        debug_ptr = debug_buf;
    }
#else
    if (trace_fd == NULL)
    {
        trace_fd = psp_fileOpen(name, O_WRONLY | O_CREAT, PSP_FLAG_DENYNO);
        if (trace_fd == NULL) {
            psp_syncExitExcl(debug_mutex);
            return FALSE;
        }
    }
#endif

#ifdef MEM_DEBUG
    if (debug_buf)
#else
    if (trace_fd != NULL)
#endif
    {
        DB_TCHAR *line, *pos;
        va_list args;

#ifndef MEM_DEBUG
        off_t filelen;
#endif
        static int last_task_no = -1;
        int task_no;

#ifndef MEM_DEBUG
        psp_fileSeek(trace_fd, 0);
        psp_fileLock(trace_fd);

        filelen = psp_fileLength(trace_fd);
        psp_fileSeek(trace_fd, filelen);
        task_no = ntask_check(task);
        if (filelen != last_len || task_no != last_task_no)
        {
            DB_TCHAR task_id[80];
            DB_TCHAR *ptr = task_id;

            if (!eol)
                ptr += vstprintf(ptr, DB_TEXT("\r\n"));
            ptr += vstprintf(ptr, DB_TEXT("<pid=%lx"), (long) psp_get_pid());
            ptr += vstprintf(ptr, DB_TEXT(", task: %d"), task_no);
            if (task->dbuserid[0])
                ptr += vstprintf(ptr, DB_TEXT(", user=%s"), task->dbuserid);
            else
            {
                DB_TCHAR *user = psp_getenv(DB_TEXT("DBUSERID"));
                ptr += vstprintf(ptr, DB_TEXT(", user=%s(?)"),
                                      user ? user : DB_TEXT(""));
            }
            vstprintf(ptr, DB_TEXT(">\r\n"));
            if (db_trace_write(trace_fd, task_id, vtstrlen(task_id), task) !=
                 (int)vtstrlen(task_id), task)
                return FALSE;

            last_task_no = task_no;

            eol = TRUE; /* force to new line */
        }
#endif /* MEM_DEBUG */

        va_start(args, fmt);
        vvstprintf(buf, fmt, args);
        line = buf;
        do
        {
            int space = task->db_indent;
            if (eol)
            {
                while (space-- > 0)
                {
#ifdef MEM_DEBUG
                    if (debug_len >= DEBUG_BUFLEN - 3)
                    {
                        if (trace_fd == NULL)
                            trace_fd = psp_fileOpen(name, O_WRONLY | O_CREAT,
                                    PSP_FLAG_DENYNO);

                        if (trace_fd != NULL)
                            stat = db_trace_write(trace_fd, debug_buf, debug_len, task);

                        debug_ptr = debug_buf;
                        debug_len = 0;
                    }

                    vtstrcpy(debug_ptr, DB_TEXT("  "));
                    debug_ptr += 2;
                    debug_len += 2;
#else
                    stat = db_trace_write(trace_fd, DB_TEXT("  "), 2, task);
#endif
                    if (stat < 0)
                        return FALSE;
                }
            }

            pos = vtstrchr(line, DB_TEXT('\n'));
            if (pos)
                *pos = 0;

            eol = (pos != NULL);
#ifdef MEM_DEBUG
            if (debug_len >= DEBUG_BUFLEN - vtstrlen(line) - 1)
            {
                if (!trace_fd)
                    trace_fd = psp_fileOpen(name, O_WRONLY | O_CREAT, PSP_FLAG_DENYNO);
                if (trace_fd)
                    stat = db_trace_write(trace_fd, debug_buf, debug_len, task);

                debug_ptr = debug_buf;
                debug_len = 0;
            }

            vtstrcpy(debug_ptr, line);
            debug_ptr += vtstrlen(line);
            debug_len += vtstrlen(line);
#else
            stat = db_trace_write(trace_fd, line, vtstrlen(line), task);
#endif
            if (stat != 0)
                return FALSE;

            if (pos)
            {
#ifdef MEM_DEBUG
                if (debug_len >= DEBUG_BUFLEN - 3)
                {
                    if (!trace_fd)
                    {
                        trace_fd = psp_fileOpen(name, O_WRONLY | O_CREAT,
                                PSP_FLAG_DENYNO);
                    }

                    if (trace_fd)
                        stat = db_trace_write(trace_fd, debug_buf, debug_len, task);

                    debug_ptr = debug_buf;
                    debug_len = 0;
                }

                vtstrcpy(debug_ptr, DB_TEXT("\r\n"));
                debug_ptr += 2;
                debug_len += 2;
#else
                stat = db_trace_write(trace_fd, DB_TEXT("\r\n"), 2, task);
#endif
                if (stat < 0)
                    return FALSE;

                line = ++pos;
            }
        }

        while (pos && *pos);
        va_end(args);
    
#ifndef MEM_DEBUG
        if (task->dboptions & SYNCFILES)
            psp_fileSync(trace_fd);

        last_len = psp_fileLength(trace_fd);

        psp_fileSeek(trace_fd, 0L);
        psp_fileUnlock(trace_fd);
#endif /* MEM_DEBUG */
    }

    psp_syncExitExcl(debug_mutex);

    return TRUE;
}

int INTERNAL_FCN db_indent(DB_TASK *task)
{
    return ++task->db_indent;
}

int INTERNAL_FCN db_undent(DB_TASK *task)
{
    if (task->db_indent)
        --task->db_indent;

    return task->db_indent;
}

static int INTERNAL_FCN db_trace_write(PSP_FH fh, const DB_TCHAR *buf,
                                       size_t len, DB_TASK *task)
{
#if defined(UNICODE)
    int   stat = S_OKAY;
    char *p;

    if (!(task->db_trace & TRACE_UNICODE)) {
        if ((p = psp_getMemory(len + 1, 0)) == NULL)
            return S_NOMEMORY;

        wtoa(buf, p, len + 1);
        if (psp_fileWrite(fh, buf, len) < len)
            stat = S_BADWRITE;

        psp_freeMemory(p, 0);

        return stat;
    }
#endif

    if (psp_fileWrite(fh, buf, len * sizeof(DB_TCHAR)) <
            (int) (len * sizeof(DB_TCHAR)))
        return S_BADWRITE;

    return S_OKAY;
}
#endif  /* DB_TRACE */


/* ======================================================================

    d_ API functions. The functions in this module are wrapper functions
    that call the db.* entry routine, then the actual processing function,
    then the db.* exit routine. The task status (task->db_status) is set
    to S_OKAY in the entry routine. Errors are reported by the exit
    routine.

   ====================================================================== */

int EXTERNAL_FCN d_checkid(DB_TCHAR *id, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_CHECKID, VOID_DB, task)) == S_OKAY)
        stat = dcheckid(id, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_close(DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_CLOSE, VOID_DB, task)) == S_OKAY)
        if (task->dbopen)
            diclose(task, ALL_DBS);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_closeall(DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_CLOSEALL, VOID_DB, task)) == S_OKAY)
        stat = dcloseall(task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_cmstat(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CMSTAT, dbn, task)) == S_OKAY)
        stat = dcmstat(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_cmtype(int set, int *cmtype, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CMTYPE, dbn, task)) == S_OKAY)
        stat = dcmtype(set, cmtype, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_connect(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CONNECT, dbn, task)) == S_OKAY)
        stat = dconnect(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_costat(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_COSTAT, dbn, task)) == S_OKAY)
        stat = dcostat(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_cotype(int set, int *cotype, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_COTYPE, dbn, task)) == S_OKAY)
        stat = dcotype(set, cotype, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_crget(DB_ADDR *dba, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CRGET, dbn, task)) == S_OKAY)
        stat = dcrget(dba, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_crread(long field, void *data, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CRREAD, dbn, task)) == S_OKAY)
        stat = dcrread(field, data, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_crset(DB_ADDR *dba, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CRSET, dbn, task)) == S_OKAY)
        stat = dcrset(dba, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_crstat(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CRSTAT, dbn, task)) == S_OKAY)
        stat = dcrstat(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_crtype(int *crtype, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CRTYPE, dbn, task)) == S_OKAY)
        stat = dcrtype(crtype, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_crwrite(long field, void *data, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CRWRITE, dbn, task)) == S_OKAY)
        stat = dcrwrite(field, data, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_csmget(int set, DB_ADDR *dba, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CSMGET, dbn, task)) == S_OKAY)
        stat = dcsmget(set, dba, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_csmread(int nset, long field, void *data, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CSMREAD, dbn, task)) == S_OKAY)
        stat = dcsmread(nset, field, data, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_csmset(int set, DB_ADDR *dba, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CSMSET, dbn, task)) == S_OKAY)
        stat = dcsmset(set, dba, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_csmwrite(int set, long field, const void *data, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CSMWRITE, dbn, task)) == S_OKAY)
        stat = dcsmwrite(set, field, data, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_csoget(int set, DB_ADDR *dba, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CSOGET, dbn, task)) == S_OKAY)
        stat = dcsoget(set, dba, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_csoread(int nset, long field, void *data, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CSOREAD, dbn, task)) == S_OKAY)
        stat = dcsoread(nset, field, data, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_csoset(int set, DB_ADDR *dba, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CSOSET, dbn, task)) == S_OKAY)
        stat = dcsoset(set, dba, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_csowrite(int set, long field, const void *data, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CSOWRITE, dbn, task)) == S_OKAY)
        stat = dcsowrite(set, field, data, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_csstat(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CSSTAT, dbn, task)) == S_OKAY)
        stat = dcsstat(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_ctbpath(const DB_TCHAR *ctb, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_CTBPATH, VOID_DB, task)) == S_OKAY)
        stat = dctbpath(ctb, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_ctscm(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CTSCM, dbn, task)) == S_OKAY)
        stat = dctscm(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_ctsco(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CTSCO, dbn, task)) == S_OKAY)
        stat = dctsco(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_ctscr(DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CTSCR, dbn, task)) == S_OKAY)
        stat = dctscr(timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_curkey(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_CURKEY, dbn, task)) == S_OKAY)
        stat = dcurkey(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dbdpath(const DB_TCHAR *path, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBDPATH, VOID_DB, task)) == S_OKAY)
        stat = ddbdpath(path, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dberr(int errnum, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBERR, VOID_DB, task)) == S_OKAY)
        stat = dberr(errnum);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dbfpath(const DB_TCHAR *path, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBFPATH, VOID_DB, task)) == S_OKAY)
        stat = ddbfpath(path, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dblog(const DB_TCHAR *log, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBLOG, VOID_DB, task)) == S_OKAY)
        stat = ddblog(log, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dbnum(const DB_TCHAR *dbn, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBNUM, VOID_DB, task)) == S_OKAY)
        stat = ddbnum(dbn, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dbstat(int type, int idx, void *buf, int buflen, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBSTAT, VOID_DB, task)) == S_OKAY)
        stat = ddbstat(type, idx, buf, buflen, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dbtaf(const DB_TCHAR *taf, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBTAF, VOID_DB, task)) == S_OKAY)
        stat = ddbtaf(taf, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dbtmp(const DB_TCHAR *tmp, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBTMP, VOID_DB, task)) == S_OKAY)
        stat = ddbtmp(tmp, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dbuserid(const DB_TCHAR *id, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBUSERID, VOID_DB, task)) == S_OKAY)
        stat = ddbuserid(id, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_decode_dba(DB_ADDR dba, short *file, DB_ADDR *slot)
{
    *file = (short) (FILEMASK & (dba >> FILESHIFT));
    *slot = (DB_ADDR) ADDRMASK & dba;
    return S_OKAY;
}

int EXTERNAL_FCN d_def_opt(unsigned long options, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DEF_OPT, VOID_DB, task)) == S_OKAY)
        stat = ddef_opt(options, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_delete(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_DELETE, dbn, task)) == S_OKAY)
        stat = ddelete(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_destroy(const DB_TCHAR *dbname, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DESTROY, VOID_DB, task)) == S_OKAY)
        stat = ddestroy(dbname, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_discon(int nset, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_DISCON, dbn, task)) == S_OKAY)
        stat = ddiscon(nset, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_disdel(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_DISDEL, dbn, task)) == S_OKAY)
        stat = ddisdel(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_encode_dba(short file, DB_ADDR slot, DB_ADDR *dba)
{
    *dba = (((F_ADDR)file & FILEMASK) << FILESHIFT) | (ADDRMASK & slot);
    return S_OKAY;
}

int EXTERNAL_FCN d_fillnew(int nrec, const void *recval, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_FILLNEW, dbn, task)) == S_OKAY)
        stat = dfillnew(nrec, recval, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_findco(int nset, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_FINDCO, dbn, task)) == S_OKAY)
        stat = dfindco(nset, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_findfm(int nset, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_FINDFM, dbn, task)) == S_OKAY)
        stat = dfindfm(nset, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_findlm(int nset, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_FINDLM, dbn, task)) == S_OKAY)
        stat = dfindlm(nset, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_findnm(int nset, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_FINDNM, dbn, task)) == S_OKAY)
        stat = dfindnm(nset, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_findpm(int nset, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_FINDPM, dbn, task)) == S_OKAY)
        stat = dfindpm(nset, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_fldnum(int *index, long fld, DB_TASK *task, int dbn)
{
    FIELD_ENTRY  *fld_ptr;
    RECORD_ENTRY *rec_ptr;
    int rec;
    int stat;

    if ((stat = db_enter(D_FLDNUM, dbn, task)) == S_OKAY)
        stat = nfld_check(fld, &rec, index, &rec_ptr, &fld_ptr, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_freeall(DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_FREEALL, VOID_DB, task)) == S_OKAY)
        stat = dfreeall(task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_gtscm(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_GTSCM, dbn, task)) == S_OKAY)
        stat = dgtscm(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_gtsco(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_GTSCO, dbn, task)) == S_OKAY)
        stat = dgtsco(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_gtscr(DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_GTSCR, dbn, task)) == S_OKAY)
        stat = dgtscr(timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_gtscs(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_GTSCS, dbn, task)) == S_OKAY)
        stat = dgtscs(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_iclose(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_ICLOSE, dbn, task)) == S_OKAY)
        stat = diclose(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_initfile(FILE_NO fno, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_INITFILE, dbn, task)) == S_OKAY)
        stat = dinitfile(fno, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_initialize(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_INITIALIZE, dbn, task)) == S_OKAY)
        stat = dinitialize(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_internals(DB_TASK *task, int topic, int id, int elem, void *ptr, unsigned size)
{
    int stat;

    if ((stat = db_enter(D_INTERNALS, VOID_DB, task)) == S_OKAY)
        stat = dinternals(task, topic, id, elem, ptr, size);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_iopen(const DB_TCHAR *dbnames, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_IOPEN, VOID_DB, task)) == S_OKAY)
        stat = dopen(dbnames, DB_TEXT(""), INCR_OPEN, NULL, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_iopen_sg(const DB_TCHAR *dbnames, const SG *sg, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_IOPEN_SG, VOID_DB, task)) == S_OKAY)
        stat = dopen(dbnames, DB_TEXT(""), INCR_OPEN, sg, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_ismember(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_ISMEMBER, dbn, task)) == S_OKAY)
        stat = dismember(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_isowner(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_ISOWNER, dbn, task)) == S_OKAY)
        stat = disowner(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keybuild(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYBUILD, dbn, task)) == S_OKAY)
        stat = dkeybuild(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keydel(long field, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYDEL, dbn, task)) == S_OKAY)
        stat = dkeydel(field, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keyexist(long field, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYEXIST, dbn, task)) == S_OKAY)
        stat = dkeyexist(field, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keyfind(long field, const void *fldval, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYFIND, dbn, task)) == S_OKAY)
        stat = dkeyfind(field, fldval, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keyfree(long key, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYFREE, dbn, task)) == S_OKAY)
        stat = dkeyfree(key, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keyfrst(long field, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYFRST, dbn, task)) == S_OKAY)
        stat = dkeyfrst(field, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keylast(long field, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYLAST, dbn, task)) == S_OKAY)
        stat = dkeylast(field, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keylock(long key, DB_TCHAR *lock_type, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYLOCK, dbn, task)) == S_OKAY)
        stat = dkeylock(key, lock_type, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keylstat(long key, DB_TCHAR *lstat, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYLSTAT, dbn, task)) == S_OKAY)
        stat = dkeylstat(key, lstat, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keynext(long field, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYNEXT, dbn, task)) == S_OKAY)
        stat = dkeynext(field, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keyprev(long field, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYPREV, dbn, task)) == S_OKAY)
        stat = dkeyprev(field, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keyread(void *key_val, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_KEYREAD, VOID_DB, task)) == S_OKAY)
        stat = dkeyread(key_val, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_keystore(long field, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_KEYSTORE, dbn, task)) == S_OKAY)
        stat = dkeystore(field, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_lmclear(
    const DB_TCHAR *username,
    const DB_TCHAR *lockmgr,
    LMC_AVAIL_FCN  *avail,
    DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_LMCLEAR, VOID_DB, task)) == S_OKAY)
        stat = dlmclear(username, lockmgr, avail, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_lmstat(DB_TCHAR *userid, int *user_stat, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_LMSTAT, VOID_DB, task)) == S_OKAY)
        stat = dlmstat(userid, user_stat, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_lock(int count, LOCK_REQUEST *lrpkt, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_LOCK, dbn, task)) == S_OKAY)
        stat = dlock(count, lrpkt, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_lockcomm(LMC_AVAIL_FCN *avail, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_LOCKCOMM, VOID_DB, task)) == S_OKAY)
        stat = dlockcomm(avail, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_lockmgr(const DB_TCHAR *id, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_LOCKMGR, VOID_DB, task)) == S_OKAY)
        stat = dlockmgr(id, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_locktimeout(int rs, int ws, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_LOCKTIMEOUT, VOID_DB, task)) == S_OKAY)
        stat = dlocktimeout(rs, ws, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_makenew(int nrec, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_MAKENEW, dbn, task)) == S_OKAY)
        stat = dmakenew(nrec, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_mapchar(unsigned char inchar, unsigned char outchar, const char *sort_str, unsigned char subsort, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_MAPCHAR, VOID_DB, task)) == S_OKAY)
        stat = dmapchar(inchar, outchar, sort_str, subsort, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_members(int set, long *tot, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_MEMBERS, dbn, task)) == S_OKAY)
        stat = dmembers(set, tot, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_off_opt(unsigned long options, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_OFF_OPT, VOID_DB, task)) == S_OKAY)
        stat = doff_opt(options, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_on_opt(unsigned long options, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_ON_OPT, VOID_DB, task)) == S_OKAY)
        stat = don_opt(options, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_open(const DB_TCHAR *dbnames, const DB_TCHAR *openmode, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_OPEN, VOID_DB, task)) == S_OKAY)
        stat = dopen(dbnames, openmode, FIRST_OPEN, NULL, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_open_sg(const DB_TCHAR *dbnames, const DB_TCHAR *openmode, const SG *sg, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_OPEN_SG, VOID_DB, task)) == S_OKAY)
        stat = dopen(dbnames, openmode, FIRST_OPEN, sg, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_rdcurr(DB_ADDR **currbuff, int *currsize, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RDCURR, dbn, task)) == S_OKAY)
        stat = drdcurr(currbuff, currsize, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dbini(const DB_TCHAR *path, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_DBINI, VOID_DB, task)) == S_OKAY)
        stat = ddbini(path, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_dbver(DB_TCHAR *fmt, DB_TCHAR *buf, int buflen)
{
    return ddbver(fmt, buf, buflen);
}

int EXTERNAL_FCN d_recfree(int rec, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECFREE, dbn, task)) == S_OKAY)
        stat = drecfree(rec, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_recfrst(int rec, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECFRST, dbn, task)) == S_OKAY)
        stat = drecfrst(rec, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_reclast(int rec, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECLAST, dbn, task)) == S_OKAY)
        stat = dreclast(rec, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_reclock(int rec, DB_TCHAR *lock_type, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECLOCK, dbn, task)) == S_OKAY)
        stat = dreclock(rec, lock_type, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_reclstat(int rec, DB_TCHAR *lstat, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECLSTAT, dbn, task)) == S_OKAY)
        stat = dreclstat(rec, lstat, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_recnext(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECNEXT, dbn, task)) == S_OKAY)
        stat = drecnext(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_recnum(int *index, int rec, DB_TASK *task, int dbn)
{
    RECORD_ENTRY *rec_ptr;
    int stat;

    if ((stat = db_enter(D_RECNUM, dbn, task)) == S_OKAY)
        stat = nrec_check(rec, index, &rec_ptr, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_recover(const DB_TCHAR *logfile, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_RECOVER, VOID_DB, task)) == S_OKAY)
        stat = drecover(logfile, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_recprev(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECPREV, dbn, task)) == S_OKAY)
        stat = drecprev(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_recread(void *rec, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECREAD, dbn, task)) == S_OKAY)
        stat = drecread(rec, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_recset(int rec, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECSET, dbn, task)) == S_OKAY)
        stat = drecset(rec, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_recstat(DB_ADDR dba, DB_ULONG rts, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECSTAT, dbn, task)) == S_OKAY)
        stat = drecstat(dba, rts, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_recwrite(const void *rec, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RECWRITE, dbn, task)) == S_OKAY)
        stat = drecwrite(rec, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_renclean(DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_RENCLEAN, VOID_DB, task)) == S_OKAY)
        stat = drenclean(task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_renfile(const DB_TCHAR *dbn, FILE_NO fno, const DB_TCHAR *fnm, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_RENFILE, VOID_DB, task)) == S_OKAY)
        stat = drenfile(dbn, fno, fnm, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_rerdcurr(DB_ADDR **currbuff, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RERDCURR, dbn, task)) == S_OKAY)
        stat = drerdcurr(currbuff, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_rlbclr(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RLBCLR, dbn, task)) == S_OKAY)
        stat = drlbclr(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_rlbset(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RLBSET, dbn, task)) == S_OKAY)
        stat = drlbset(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_rlbtst(DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_RLBTST, dbn, task)) == S_OKAY)
        stat = drlbtst(task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_set_dberr(ERRORPROC func, DB_TASK *task)
{
    int stat;
    
    if ((stat = db_enter(D_SET_DBERR, VOID_DB, task)) == S_OKAY)
        task->error_func = func;
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setdb(int dbn, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_SETDB, VOID_DB, task)) == S_OKAY)
        stat = dsetdb(dbn, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setfiles(int num, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_SETFILES, VOID_DB, task)) == S_OKAY)
        stat = dsetfiles(num, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setfree(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETFREE, dbn, task)) == S_OKAY)
        stat = dsetfree(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setkey(long field, const void *fldvalue, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETKEY, dbn, task)) == S_OKAY)
        stat = dsetkey(field, fldvalue, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setlock(int set, DB_TCHAR *lock_type, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETLOCK, dbn, task)) == S_OKAY)
        stat = dsetlock(set, lock_type, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setlstat(int set, DB_TCHAR *lstat, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETLSTAT, dbn, task)) == S_OKAY)
        stat = dsetlstat(set, lstat, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setmm(int sett, int sets, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETMM, dbn, task)) == S_OKAY)
        stat = dsetmm(sett, sets, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setmo(int setm, int seto, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETMO, dbn, task)) == S_OKAY)
        stat = dsetmo(setm, seto, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setmr(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETMR, dbn, task)) == S_OKAY)
        stat = dsetmr(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setnum(int *index, int set, DB_TASK *task, int dbn)
{
    SET_ENTRY *set_ptr;
    int stat;

    if ((stat = db_enter(D_SETNUM, dbn, task)) == S_OKAY)
        stat = nset_check(set, index, &set_ptr, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setom(int nseto, int nsetm, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETOM, dbn, task)) == S_OKAY)
        stat = dsetom(nseto, nsetm, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setoo(int nsett, int nsets, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETOO, dbn, task)) == S_OKAY)
        stat = dsetoo(nsett, nsets, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setor(int nset, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETOR, dbn, task)) == S_OKAY)
        stat = dsetor(nset, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setpages(int dbpgs, int ixpgs, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_SETPAGES, VOID_DB, task)) == S_OKAY)
        stat = dsetpages(dbpgs, ixpgs, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setrm(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETRM, dbn, task)) == S_OKAY)
        stat = dsetrm(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_setro(int set, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_SETRO, dbn, task)) == S_OKAY)
        stat = dsetro(set, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_stscm(int set, DB_ULONG timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_STSCM, dbn, task)) == S_OKAY)
        stat = dstscm(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_stsco(int set, DB_ULONG timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_STSCO, dbn, task)) == S_OKAY)
        stat = dstsco(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_stscr(DB_ULONG timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_STSCR, dbn, task)) == S_OKAY)
        stat = dstscr(timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_stscs(int set, DB_ULONG timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_STSCS, dbn, task)) == S_OKAY)
        stat = dstscs(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_timeout(int secs, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_TIMEOUT, VOID_DB, task)) == S_OKAY)
        stat = dtimeout(secs, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_trabort(DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_TRABORT, VOID_DB, task)) == S_OKAY)
        stat = dtrabort(task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_trbegin(const DB_TCHAR *tid, DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_TRBEGIN, VOID_DB, task)) == S_OKAY)
        stat = dtrbegin(tid, task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_trbound(DB_TASK *task)
{
    return dtrbound(task);
}

int EXTERNAL_FCN d_trend(DB_TASK *task)
{
    int stat;

    if ((stat = db_enter(D_TREND, VOID_DB, task)) == S_OKAY)
        stat = dtrend(task);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_trlog(int fno, long pno, const char *page, int psz, DB_TASK *task)
{
    return dtrlog(fno, pno, page, psz, task);
}

int EXTERNAL_FCN d_trmark(DB_TASK *task)
{
    return dtrmark(task);
}

int EXTERNAL_FCN d_utscm(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_UTSCM, dbn, task)) == S_OKAY)
        stat = dutscm(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_utsco(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_UTSCO, dbn, task)) == S_OKAY)
        stat = dutsco(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_utscr(DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_UTSCR, dbn, task)) == S_OKAY)
        stat = dutscr(timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_utscs(int set, DB_ULONG *timestamp, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_UTSCS, dbn, task)) == S_OKAY)
        stat = dutscs(set, timestamp, task, dbn);
    return db_exit(stat, task);
}

int EXTERNAL_FCN d_wrcurr(DB_ADDR *currbuff, DB_TASK *task, int dbn)
{
    int stat;

    if ((stat = db_enter(D_WRCURR, dbn, task)) == S_OKAY)
        stat = dwrcurr(currbuff, task, dbn);
    return db_exit(stat, task);
}

