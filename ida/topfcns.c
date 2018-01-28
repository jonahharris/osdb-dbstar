/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* -------------------------------------------------------------------------
    IDA - Top Level Menu Functions.
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "ddlnms.h"
#include "ida_d.h"
#include "ida.h"

/************************** EXTERNAL VARIABLES ****************************/
extern int dbs_in_dir;
extern char **dbnames;

/*************************** GLOBAL VARIABLES *****************************/
char dbname[80] = "";

static int select_db(void);
static void opendb(char *);

/* =========================================================================
    Open database
*/
static void opendb(char *type)
{
    int db;

    if ((db = select_db()) != -1)
    {
        if (task->dbopen)
            d_close(task);
        if (d_open(dbnames[db], type, task) == S_OKAY)
        {
            bld_keys();
            strcpy(dbname, dbnames[db]);
        }
        else if (task->db_status == S_UNAVAIL)
            usererr("database currently unavailable");
    }
}

/* =========================================================================
    Open exclusive database
*/
void oefcn()
{
    opendb("x");
}

/* =========================================================================
    Open shared database
*/
void osfcn()
{
    opendb("s");
}

/* =========================================================================
    Open single user database
*/
void oofcn()
{
    opendb("o");
}

/* =========================================================================
    Access database
*/
void db_access()
{
    if (!task->dbopen)
        usererr("database not opened");
    else
        menu(DB_MENU);
}

/* =========================================================================
    Initialize database
*/
void initdb()
{
    register int c;

    if (!task->dbopen)
    {
        usererr("database must first be opened");
        return;
    }
    tprintf("@m0400@EInitialization of database: %s\n\n", dbname);
    tprintf("This will @SERASE@s entire database, are you sure? ");
    c = tgetch();
    if (c == 'y' || c == 'Y')
    {
        tprintf("yes");
        fflush(stdout);
        if (d_initialize(task, CURR_DB) == S_OKAY)
            usererr("database initialized");
    }
    else
    {
        tprintf("no");
        fflush(stdout);
    }
    tprintf("@c");
}

/* =========================================================================
    Close database
*/
void closedb()
{
    d_close(task);
}

/* =========================================================================
    Select database name
*/
static int select_db()
{
    int i;

    if (dbs_in_dir)
    {
        tprintf("@m0400@ESELECT DATABASE:\n\n");
        i = list_selection(6, dbs_in_dir, dbnames, 1, 0, 1);
        tprintf("@m0300@E");
        return (i);
    }
    else
    {
        usererr("no databases found");
        return (-1);
    }
}


