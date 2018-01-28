/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dal utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/*-----------------------------------------------------------------------

    getnames.c - Functions to retrieve record, set and field names.

    These functions return a record, set or field table entry based
    on the name sent in as a parameter.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "getnames.h"

int getrec(DB_TCHAR *name, DB_TASK *task)
{
    register int i;
    DB_TCHAR caps[50];

    for (i = 0; i < (int) vtstrlen(name); i++)
        if (name[i] >= DB_TEXT('a') && name[i] <= DB_TEXT('z'))
            caps[i] = name[i] - DB_TEXT(' ');
        else
            caps[i] = name[i];
    caps[i] = 0;

    for (i = 0; i < task->size_rt; i++)
    {
        if (vtstrcmp(caps, task->record_names[i]) == 0)
            return (i);
    }
    return (-1);
}

int getset(DB_TCHAR *name, DB_TASK *task)
{
    register int i;
    DB_TCHAR caps[50];

    for (i = 0; i < (int) vtstrlen(name); i++)
        if (name[i] >= DB_TEXT('a') && name[i] <= DB_TEXT('z'))
            caps[i] = name[i] - DB_TEXT(' ');
        else
            caps[i] = name[i];
    caps[i] = 0;

    for (i = 0; i < task->size_st; i++)
    {
        if (vtstrcmp(caps, task->set_names[i]) == 0)
            return (i);
    }
    return (-1);
}

long getfld(DB_TCHAR *name, int rec, DB_TASK *task)
{
    register int i;
    int lower, upper;
    DB_TCHAR caps[50];

    for (i = 0; i < (int) vtstrlen(name); i++)
        if (name[i] >= DB_TEXT('a') && name[i] <= DB_TEXT('z'))
            caps[i] = name[i] - DB_TEXT(' ');
        else
            caps[i] = name[i];
    caps[i] = 0;

    if (rec == -1)
    {
        lower = 0;
        upper = task->size_fd;
    }
    else
    {
        lower = task->record_table[rec].rt_fields;
        upper = lower + task->record_table[rec].rt_fdtot;
    }

    for (i = lower; i < upper; i++)
    {
        if (vtstrcmp(caps, task->field_names[i]) == 0)
        {
            return ((long) i);
        }
    }
    return ((long) -1);
}

int rec_const(int rec)
{
    return (rec + RECMARK);
}

int set_const(int set)
{
    return (set + SETMARK);
}

long fld_const(int fld, DB_TASK *task)
{
    long rt, fd;

    rt = (long) task->field_table[fld].fd_rec;
    fd = rt * (long) FLDMARK + (long) (fld - task->record_table[rt].rt_fields);
    return (fd);
}


