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
#include "options.h"


int Db_task_count = 0;
TASK **Db_task_table = NULL;    /* database task table */
extern PSP_SEM task_sem;

/* ======================================================================
*/
int EXTERNAL_FCN d_opentask(DB_TASK **pTask)
{
    int    stat = S_OKAY;
    TASK  *task;
    TASK **temp_table;

    /*
        The task does not exist yet, so we can't call dberr() or set db_status
    */
    if (pTask == NULL)
        return S_INVPTR;

    if (!Db_task_count)
        dbInit(FROM_RUNTIME);

    *pTask = task = (TASK *) psp_cGetMemory(sizeof(TASK), 0);
    if (!task)
        return S_NOMEMORY;

    task->curr_db = -1;
    task->set_db = -1;

#if defined(DB_TRACE)
    task->db_trace = DEFAULT_TRACE_OPTS;
#endif

#if defined(DB_DEBUG)
    task->db_debug = DEFAULT_DEBUG_OPTS;
#endif

    task->dboptions = DEFAULT_OPTS;
    task->dbwait_time = 1;
    task->db_timeout = DB_TIMEOUT;
    task->nap_factor = DEF_FACTOR;
    /* task->q = NULL;   initialize db_QUERY pointer */

    psp_syncEnterExcl(task_sem);

    Db_task_count++;

    /* add new task to global task table */
    temp_table = (TASK **) psp_cGetMemory(Db_task_count * sizeof(TASK *), 0);
    if (temp_table == NULL)
    {
        psp_syncExitExcl(task_sem);
        stat = S_NOMEMORY;
        goto err;
    }

    if (Db_task_count > 1)
    {
        memcpy(temp_table, Db_task_table, (Db_task_count - 1) * sizeof(TASK *));
        psp_freeMemory(Db_task_table, 0);
    }

    Db_task_table = temp_table;
    Db_task_table[Db_task_count - 1] = task;

    psp_syncExitExcl(task_sem);

    /*
        OK, the task is set up enough to be used, so we can do a db_enter
        to switch to this task
    */
    if ((stat = db_enter(D_OPENTASK, VOID_DB, task)) == S_OKAY)
        create_cache(task);

    return db_exit(task->db_status, task);

err:
    db_exit(stat, task);

    psp_freeMemory(task, 0);

    return stat;
}


/* ======================================================================
*/
int EXTERNAL_FCN d_closetask(DB_TASK *task)
{
    int stat;
    int close_task_no;

    if (!task)
        return S_INVTASK;

    if ((stat = db_enter(D_CLOSETASK, VOID_DB, task)) != S_OKAY)
        return stat;

    if (task->dbopen > 0)
        stat = dberr(S_DBCLOSE);

    API_EXIT();

    if (stat)
        return stat;

    if (task->lmc)
        psp_lmcCleanup(task->lmc);

    close_task_no = ntask_check(task);

    remove_cache(task);
    if (task->dbtmp)
        psp_freeMemory(task->dbtmp, 0);

    if (task->lockmgrn)
        psp_freeMemory(task->lockmgrn, 0);

    if (task->enc_buff)
        psp_freeMemory(task->enc_buff, 0);

    psp_freeMemory(task, 0);

    /* can not access any part of the DB_TASK from here down */

    psp_syncEnterExcl(task_sem);

    if (--Db_task_count)
    {
        if (close_task_no < Db_task_count)
        {
            memmove(&Db_task_table[close_task_no],
                    &Db_task_table[close_task_no + 1],
                    (Db_task_count - close_task_no) * sizeof(TASK *));
        }
    }
    else
        psp_freeMemory(Db_task_table, 0);

    psp_syncExitExcl(task_sem);

    /* can not call db_exit() because the task no longer exists, so fake it */

    if (!Db_task_count)
        dbTerm(FROM_RUNTIME);

    return stat;
}

/* ======================================================================
*/
int INTERNAL_FCN create_cache(DB_TASK *task)
{
    CACHE  *new_cache;

    /* Allocate new cache */
    if ((new_cache = (CACHE *) psp_cGetMemory(sizeof(CACHE), 0)) == NULL)
        return (dberr(S_NOMEMORY));

    new_cache->db_pgtab.pgtab_sz = DEFDBPAGES;
    new_cache->ix_pgtab.pgtab_sz = DEFIXPAGES;

    task->cache = new_cache;

    return (task->db_status);
}

/* ======================================================================
    Remove a cache from Task.
*/
int INTERNAL_FCN remove_cache(DB_TASK *task)
{
    psp_freeMemory(task->cache, 0);

    return (task->db_status);
}

/* ======================================================================
    Returns task number, or S_INVTASK
*/
int INTERNAL_FCN ntask_check(DB_TASK *task)
{
    /* this function must not call dberr() or use db_global */

    int tx;

    psp_syncEnterExcl(task_sem);

    /* Task must be in task table */
    for (tx = 0; tx < Db_task_count; tx++)
    {
        if (task == Db_task_table[tx])
            break;
    }

    if (tx == Db_task_count)
        tx = S_INVTASK;

    psp_syncExitExcl(task_sem);

    return tx;
}


