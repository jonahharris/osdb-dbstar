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
#if !defined(NO_ERRSTR)
#include "dberr.h"
#endif


/* ======================================================================
    Database auto-recovery notification function
*/
void INTERNAL_FCN dbautorec(DB_TASK *task)
{
    /* Call dberr to display "recovery about to occur" message,
       and flush the error, otherwise it won't appear till after
       recovery is complete, when the d_ function returns.
    */
    dberr(S_RECOVERY);
    flush_dberr(task);
    task->db_status = S_OKAY;
}


/* ======================================================================
    Signal database error
*/
int EXTERNAL_FCN _dberr(int errnum, char *filename, int linenum, DB_TASK *task)
{
    if (errnum >= S_OKAY)
    {
        /* don't overwrite a previous error */
        if (task->db_status >= 0)
            task->db_status = errnum;
        return errnum;
    }

    /* S_RECFAIL is returned when an auto-recovery fails. The failure
       must be caused by some other error, such as an I/O error, so it
       is likely that db_status is already set. Report the previous
       error before storing the S_RECFAIL code.
    */
    if (errnum == S_RECFAIL)
        flush_dberr(task);

    /* don't overwrite a previous error */
    if (task->db_status < 0)
        return errnum;

    /* S_REENTER is returned when db.* is called reentrantly with the
       same task argument - could happen if the application's database
       error handler calls a d_ function. To avoid infinite recursion,
       this error is not reported through the error handler.
    */
    if (errnum == S_REENTER)
        return (task->db_status = errnum);
   
    if (!task->errnum)
    {
        /* save the error state */
        if ((errnum <= S_INTERNAL_FIRST) && (errnum >= S_INTERNAL_LAST))
        {
            task->err_ex = errnum;
            errnum = S_SYSERR;
        }
        task->errnum   = errnum;
        task->errno_c  = psp_errno();
        task->errfile = filename;
        task->errline  = linenum;
    }

    return (task->db_status = errnum);
}


/* ======================================================================
*/
void EXTERNAL_FCN flush_dberr(DB_TASK *task)
{
    DB_TCHAR msg_string[180];

    if (task->errnum >= 0)
        return;

    /* generate error message string */
    dberr_msg(msg_string, sizeof(msg_string) / sizeof(DB_TCHAR), task);

#ifdef DB_TRACE
    if (task->db_trace)
    {
        /* log the error to the trace file */
        db_printf(task, DB_TEXT("%s\n"), msg_string);
    }
#endif
    /* cannot call db.* reentrantly with same task */
    task->inDBSTAR = 1;     /* checked by db_enter */

    /* the handler must not do a throw/longjump, or reenter db.*
       with the current task, but can call db.* functions for a
       different task
    */
    if (task->error_func)
        task->error_func(task->errnum, msg_string);
    else  /* default error handler */
        psp_errPrint(msg_string);

    task->inDBSTAR = 0;

    task->errnum = 0;
    task->err_ex = 0;
    task->errno_c = 0;
    task->errfile = NULL;
    task->errline = 0;
    task->db_status = S_OKAY;
}



/* ======================================================================
    Construct an error message string
    (note: this may be called during db_exit() after task is no longer
     accessible)
*/

void EXTERNAL_FCN dberr_msg(
    DB_TCHAR *msg_string,
    int msg_len,
    DB_TASK *task)
{
    DB_STRING msg;
#define MESSAGE_LEN  80
    DB_TCHAR msgbuf[MESSAGE_LEN];
    DB_TCHAR *msg_type;
    DB_TCHAR errstr[10];
    int stat;

    if (task->errnum >= 0)
    {
        msg_type = DB_TEXT("Function Status Code");
        vtstrcpy(msgbuf, DB_TEXT(""));
    }
    else if (task->errnum == S_LMCERROR)
    {
        msg_type = DB_TEXT("LMC error");
        stat = psp_lmcErrstr(msgbuf, task->err_ex, MESSAGE_LEN, task->lmc);
        if (stat != S_OKAY)
            errstring(msgbuf, stat, MESSAGE_LEN, task);
    }
    else if ((task->errnum <= S_USER_FIRST) && (task->errnum >= S_USER_LAST))
    {
        msg_type = DB_TEXT("PROGRAMMER/USER error");
        errstring(msgbuf, task->errnum, MESSAGE_LEN, task);
    }
    else if (task->errnum == S_SYSERR)
    {
        msg_type = DB_TEXT("INTERNAL error");
        errstring(msgbuf, task->err_ex ? task->err_ex : S_SYSERR, MESSAGE_LEN, task);
    }
    else if ((task->errnum <= S_SYSTEM_FIRST) && (task->errnum >= S_SYSTEM_LAST))
    {
        msg_type = DB_TEXT("SYSTEM/OS error");
        errstring(msgbuf, task->errnum, MESSAGE_LEN, task);
    }
    else
    {
        msg_type = DB_TEXT("INTERNAL error");
        vstprintf(msgbuf, DB_TEXT("Unrecognized error %d"), task->errnum);
    }
    msgbuf[MESSAGE_LEN-1] = DB_TEXT('\0');    /* just in case */

    /* Message format:
            +-------------------------+
            | type: errnum            |
            | errmsg                  |
            | C errno = nn: strerror  |
            | FILE: filename(linenum) |
            | DLL: dll_name           |
            +-------------------------+
    */
    msg_string[0] = DB_TEXT('\0');
    STRinit(&msg, msg_string, msg_len);
    STRcpy(&msg, msg_type);

    vstprintf(errstr, DB_TEXT(": %d\n"), task->errnum);
    STRcat(&msg, errstr);
    STRcat(&msg, msgbuf);
    vstprintf(msgbuf, DB_TEXT("\nC errno = %d"), task->errno_c);
    STRcat(&msg, msgbuf);
    vstprintf(msgbuf, DB_TEXT(": ") DB_STRFMT, strerror(task->errno_c));
    STRcat(&msg, msgbuf);

    if (task->errfile != NULL && task->errline != 0)
    {
        vstprintf(msgbuf, DB_TEXT("\nFILE: ") DB_STRFMT DB_TEXT("(%d)"),
                task->errfile, task->errline);
        STRcat(&msg, msgbuf);
    }
}


/* ======================================================================
    Return error message string for specified error number
*/
int INTERNAL_FCN errstring(DB_TCHAR *msgbuf, int errnum, int buflen,
                           DB_TASK *task)
{
    DB_TCHAR *msg = NULL;

#if defined(NO_ERRSTR)
    if (errnum == S_RECOVERY)
        msg = DB_TEXT("automatic recovery about to occur");
    else
        msg = DB_TEXT("error message unavailable");
#else
    if (errnum <= S_USER_FIRST && errnum >= S_USER_LAST)
        msg = user_error[S_USER_FIRST - errnum];
    else if (errnum <= S_SYSTEM_FIRST && errnum >= S_SYSTEM_LAST)
        msg = system_error[S_SYSTEM_FIRST - errnum];
    else if (errnum <= S_INTERNAL_FIRST && errnum >= S_INTERNAL_LAST)
        msg = internal_error[S_INTERNAL_FIRST - errnum];
#endif

    if (msg)
        vstrnzcpy(msgbuf, msg, buflen);

    return S_OKAY;
}

