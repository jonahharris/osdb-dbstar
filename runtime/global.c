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

PSP_SEM task_sem;

#define LOCALE_SIZE 256

static int      initialized = 0;
static DB_TCHAR locale[LOCALE_SIZE];

/*
    The functions below initialize and delete the critical sections
    which protect db.*'s global variables. Under VxWorks and Solaris they
    are called in dt_opentask and dt_closetask when the first / last task
    is opened / closed. Under Win32 they are called when a process attaches
    to / detaches from the db.* DLL (which guarantees that they will get
    called, even if the application never calls dt_closetask()).
*/

void dbInit(int where)
{
    if (!initialized)
    {
        if (where == FROM_RUNTIME)
            psp_enterCritSec();

        if (!initialized && psp_init() == PSP_OKAY)
        {
            if ((task_sem = psp_syncCreate(PSP_MUTEX_SEM)) != NO_PSP_SEM) {
                initialized = where;
                psp_localeGet(locale, LOCALE_SIZE);
                psp_localeSet(DB_TEXT(""));
            }
        }

        if (where == FROM_RUNTIME)
            psp_exitCritSec();
    }
}

void dbTerm(int where)
{
    if (initialized == where)
    {
        if (where == FROM_RUNTIME)
            psp_enterCritSec();

        if (initialized) {
            psp_localeSet(locale);
            psp_syncDelete(task_sem);
            psp_term();
            initialized = 0;
        }

        if (where == FROM_RUNTIME)
            psp_exitCritSec();
    }
}

