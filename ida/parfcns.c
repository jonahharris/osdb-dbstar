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

/*--------------------------------------------------------------------------
    IDA - Parameter Control Functions
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "internal.h"
#include "ida.h"

void showpar(void);

/* ========================================================================
    Display current parameter settings
*/
void showpar()
{
    long max_files;

    d_internals(task, TOPIC_GLOBALS, ID_MAX_FILES, 0, &max_files, sizeof(max_files));

    tprintf("@m0400@E\n\n\n");
    tprintf("@SDatabase Dictionary Path - DBDPATH  :@s %s\n\n",
            task->dbdpath[0] ? task->dbdpath : ".");
    tprintf("@SDatabase Files Path      - DBFPATH  :@s %s\n\n",
            task->dbfpath[0] ? task->dbfpath : ".");
    tprintf("@SDatabase User Identifer  - DBUSERID :@s %s\n\n",
            task->dbuserid[0] ? task->dbuserid : "");
    tprintf("@SNumber of Virtual Memory Pages      :@s %d\n\n", task->cache->db_pgtab.pgtab_sz);
    tprintf("@SMaximum number of open files        :@s %ld\n\n", max_files);
    tprintf("@STransaction Logging                 :@s %s\n\n",
            (task->dboptions & TRLOGGING) ? "ON " : "OFF");
    tprintf("@SReuse of Delete Chain               :@s %s\n\n",
            (task->dboptions & DCHAINUSE) ? "ON " : "OFF");
}

/* ========================================================================
    Set DBDPATH environment variable
*/
void pdfcn()
{
    char dbd[80];
    int len;

    if (task->dbopen)
        usererr("database must be closed");
    else
    {
        tprintf("@m0200@eenter database directory path: ");
        switch (rdtext(dbd))
        {
            case -1:
                return;
            case 0:
                dbd[0] = '.';
                dbd[1] = DIRCHAR;
                dbd[2] = '\0';
                d_dbdpath(dbd, task);
                d_dbfpath(dbd, task);
                break;
            default:
                len = strlen(dbd);
                if (dbd[len - 1] != DIRCHAR)
                {
                    dbd[len] = DIRCHAR;
                    dbd[++len] = '\0';
                }
                d_dbdpath(dbd, task);
                d_dbfpath(dbd, task);
        }
        dblist();                        /* regenerate list of database
                                          * names */
    }
    showpar();
}

/* ========================================================================
    Set DBFPATH environment variable
*/
void pffcn()
{
    char dbf[80];
    int len;

    if (task->dbopen)
        usererr("database must be closed");
    else
    {
        tprintf("@m0200@eenter database files path: ");
        switch (rdtext(dbf))
        {
            case -1:
                return;
            case 0:
                dbf[0] = '.';
                dbf[1] = DIRCHAR;
                dbf[2] = '\0';
                d_dbfpath(dbf, task);
                break;
            default:
                len = strlen(dbf);
                if (dbf[len - 1] != DIRCHAR)
                {
                    dbf[len] = DIRCHAR;
                    dbf[++len] = '\0';
                }
                d_dbfpath(dbf, task);
        }
    }
    showpar();
}

/* ========================================================================
    Set database user id
*/
void pufcn()
{
    char dbid[80];

    if (task->dbopen)
        usererr("database must be closed");
    else
    {
        tprintf("@m0200@eenter database user id: ");
        switch (rdtext(dbid))
        {
            case -1:
                return;
            case 0:
                d_dbuserid("", task);
                break;
            default:
                d_dbuserid(dbid, task);
        }
    }
    showpar();
}

/* ========================================================================
    Set current number of virtual memory pages
*/
void ppfcn()
{
    char pgs_str[20];
    int pgs;

    if (task->dbopen)
        usererr("database must be closed");
    else
    {
        tprintf("@m0200@eenter number of pages: ");
        if (rdtext(pgs_str) <= 0)
            return;
        if (sscanf(pgs_str, "%d", &pgs) == 1)
        {
            d_setpages(pgs, (pgs >= 32 ? 8 : 4), task);
            showpar();
        }
        else
            usererr("invalid entry");
    }
}

/* ========================================================================
    Set maximum number of open db.* files
*/
void pmfcn()
{
    char files_str[20];
    int files;

    if (task->dbopen)
        usererr("database must be closed");
    else
    {
        tprintf("@m0200@eenter maximum number of files: ");
        if (rdtext(files_str) <= 0)
            return;
        if (sscanf(files_str, "%d", &files) == 1)
        {
            d_setfiles(files, task);
            showpar();
        }
        else
            usererr("invalid entry");
    }
}

/* ========================================================================
    Toggle transaction logging flag
*/
void plfcn()
{
    if (task->dbopen)
        usererr("database must be closed");
    else
    {
        if (task->dboptions & TRLOGGING)
        {
            d_off_opt(TRLOGGING, task);
            tprintf("@m1738OFF");
        }
        else
        {
            d_on_opt(TRLOGGING, task);
            tprintf("@m1738ON ");
        }
    }
}

/* ========================================================================
    Toggle dchain use flag
*/
void pcfcn()
{
    if (task->dbopen)
        usererr("database must be closed");
    else
    {
        if (task->dboptions & DCHAINUSE)
        {
            d_off_opt(DCHAINUSE, task);
            tprintf("@m1938OFF");
        }
        else
        {
            d_on_opt(DCHAINUSE, task);
            tprintf("@m1938ON ");
        }
    }
}


