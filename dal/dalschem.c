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

    dalschem.c - Print part of the schema.

    This function will print schema information in response to
    the 'schema' command in DAL.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "daldef.h"
#include "dalvar.h"

#define DAL
#include "getnames.h"

static void prfield(int, int, DB_TASK *);


int dalschem(DB_TCHAR *name)
{
    int recnum, setnum, i, j;
    int fldnum;
    DB_TASK *task = DalDbTask;    /* required for macros, e.g. task->size_rt */

    if (vtstrlen(name) == 0)
    {
        /* print all record names */
        vtprintf(DB_TEXT("Database record types:\n"));
        for (i = 0; i < task->size_rt; i++)
            vtprintf(DB_TEXT("   %s\n"), task->record_names[i]);
        vtprintf(DB_TEXT("\nDatabase set types:\n"));
        for (i = 0; i < task->size_st; i++)
            vtprintf(DB_TEXT("   %s\n"), task->set_names[i]);
        return (0);
    }

    /* first check the records */
    if ((recnum = getrec(name, task)) != -1)
    {
        /* print record information */
        vtprintf(DB_TEXT("Record type: %s\n"), task->record_names[recnum]);
        for (i = task->record_table[recnum].rt_fields;
             i < task->record_table[recnum].rt_fields + task->record_table[recnum].rt_fdtot;
             i++)
        {
            prfield(3, i, task);
        }
    }
    else if ((fldnum = (int) getfld(name, -1, task)) != -1)
    {
        /* print field information */
        prfield(0, fldnum, task);
    }
    else if ((setnum = getset(name, task)) != -1)
    {
        /* print set information */
        vtprintf(DB_TEXT("Set type: %s, order: "), task->set_names[setnum]);
        switch (task->set_table[setnum].st_order)
        {
            case FIRST:
                vtprintf(DB_TEXT("FIRST"));
                break;
            case LAST:
                vtprintf(DB_TEXT("LAST"));
                break;
            case ASCENDING:
                vtprintf(DB_TEXT("ASCENDING"));
                break;
            case DESCENDING:
                vtprintf(DB_TEXT("DESCENDING"));
                break;
            case NEXT:
                vtprintf(DB_TEXT("NEXT"));
                break;
        }
        vtprintf(DB_TEXT("\n   Owner record type: %s\n"),
                    task->record_names[task->set_table[setnum].st_own_rt]);
        vtprintf(DB_TEXT("   Member record type(s):\n"));
        for (i = task->set_table[setnum].st_members;
             i < task->set_table[setnum].st_members + task->set_table[setnum].st_memtot;
             i++)
        {
            vtprintf(DB_TEXT("      %s"), task->record_names[task->member_table[i].mt_record]);
            if (task->set_table[setnum].st_order == 'a' ||
                 task->set_table[setnum].st_order == 'd')
            {
                vtprintf(DB_TEXT(", sorted by: "));
                for (j = task->member_table[i].mt_sort_fld;
                     j < task->member_table[i].mt_sort_fld + task->member_table[i].mt_totsf;
                     j++)
                    vtprintf(DB_TEXT("%s\n"), task->field_names[task->sort_table[j].se_fld]);
            }
            else
                vtprintf(DB_TEXT("\n"));
        }
    }
    else
        vtprintf(DB_TEXT("%s not found in schema\n"), name);

    return (0);
}


static void prfield(int offset, int fldnum, DB_TASK *task)
{
    int i;

    for (i = 0; i < offset; i++)
        vtprintf(DB_TEXT(" "));

    vtprintf(DB_TEXT("Field: %s, type: "), task->field_names[fldnum]);
    switch (task->field_table[fldnum].fd_type)
    {
        case CHARACTER:
            vtprintf(DB_TEXT("CHARACTER"));
            break;
        case WIDECHAR:
            vtprintf(DB_TEXT("WIDECHAR"));
            break;
        case SHORTINT:
            vtprintf(DB_TEXT("SHORT INTEGER"));
            break;
        case REGINT:
            vtprintf(DB_TEXT("INTEGER"));
            break;
        case LONGINT:
            vtprintf(DB_TEXT("LONG INTEGER"));
            break;
        case FLOAT:
            vtprintf(DB_TEXT("FLOAT"));
            break;
        case DOUBLE:
            vtprintf(DB_TEXT("DOUBLE"));
            break;
        case DBADDR:
            vtprintf(DB_TEXT("DB_ADDR"));
            break;
        case GROUPED:
            vtprintf(DB_TEXT("GROUP"));
            break;
    }
    vtprintf(DB_TEXT(", length: %d\n"), task->field_table[fldnum].fd_len);

    if (task->field_table[fldnum].fd_key != 'n')
    {
        for (i = 0; i < offset; i++)
            vtprintf(DB_TEXT(" "));

        vtprintf(DB_TEXT("   field has %s key, contained in %s\n"),
            task->field_table[fldnum].fd_type == 'u' ? DB_TEXT("UNIQUE") : DB_TEXT("NON-UNIQUE"),
            task->file_table[task->field_table[fldnum].fd_keyfile].ft_name);
    }
}


