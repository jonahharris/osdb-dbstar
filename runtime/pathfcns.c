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

/*-----------------------------------------------------------------------

    This file contains the functions con_dbd and con_dbf which construct
    the full path for the dictionary and data files.  It provides a uniform
    method of dealing with how to construct the full paths given the schema
    path, d_open path, and the paths provided in DBDPATH and DBFPATH.  This
    function does not depend upon the runtime and can be included with
    utilities that do not call the runtime.

    This file also contains the function get_element which extracts a single
    element out of the possibly multiple-element DBDPATH or DBFPATH.  Each
    element is separated by a semacolon.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */

#include "db.star.h"

/* ********************** FUNCTION PROTOTYPES ************************ */

/* TRUE if path is absolute */
static int INTERNAL_FCN isabs(const DB_TCHAR *);


/* ======================================================================
    Construct the full path to the dictionary.
*/
int INTERNAL_FCN con_dbd(
    DB_TCHAR *path_str,  /* (output) string to receive the full     */
                                         /* path to the dictionary                  */
    DB_TCHAR *dbname,    /* contains the filename of the dictionary */
                                         /* preceded optionally by a path, and      */
                                         /* optionally (DOS only) by a drive spec.  */
    DB_TCHAR *dbdpath,   /* contains one element of the environment */
                                         /* variable DBDPATH. NULL means no DBDPATH */
                                         /* defined.                                */
    DB_TASK *task)
{
/*
    RETURNS: task->db_status, S_OKAY means no errors
    ASSUMES: That the string dbdpath contains enough room
             to add a DIRCHAR if it isn't in there.
*/

    /* We stop constructing the string when we find an absolute path */
    if (isabs(dbname))
        vtstrcpy(path_str, dbname);
    else
    {
        /* Was dbdpath was defined?  Make sure it ends with a DIRCHAR or ':' */
        if (dbdpath)
        {
            int i = vtstrlen(dbdpath);
            if (!psp_pathIsDir(dbdpath))
            {
                dbdpath[i++] = DIRCHAR;
                dbdpath[i] = DB_TEXT('\0');
            }

            if (i >= FILENMLEN - 4)
                return (dberr(S_NAMELEN));

            /* Now construct the path with dbdpath + dbname */
            vtstrcpy(path_str, dbdpath);
            if (vtstrlen(path_str) +
                vtstrlen(psp_pathStripDrive(dbname)) >= FILENMLEN - 4)
                return (dberr(S_NAMELEN));

            vtstrcat(path_str, psp_pathStripDrive(dbname));
        }
        else
            vtstrcpy(path_str, dbname);  /* dbdpath not defined */
    }

    /* Now add the .dbd extension */
    vtstrcat(path_str, DB_TEXT(".dbd"));
    return (task->db_status);
}


/* ======================================================================
    Construct full path to data/key files
*/
int EXTERNAL_FCN con_dbf(
    DB_TCHAR *path_str,  /* (output) receives full path to dictionary */
    DB_TCHAR *dbfile,    /* path to database file (defn in schema)    */
    DB_TCHAR *dbname,    /* d_open dbname argument - path to dictnry  */
    DB_TCHAR *dbfpath,   /* element from DBFPATH, or NULL for no path */
    DB_TASK *task)
{
/*
    RETURNS: task->db_status, S_OKAY means no error.
    ASSUMES: None.
*/
    DB_TCHAR filespec[FILENMLEN];           /* Scratch work space */

    /* Stop construction when we get to an absolute path.  If we prepend
       then remove the previous drive specifier
    */
    vtstrcpy(path_str, dbfile);
    if (isabs(dbfile))
        return (task->db_status);

    /* Add only the drive specification and directory component of the path
       supplied in dbname
    */
    psp_pathSplit(dbname, &dbname, NULL);
    if (dbname)
    {
        if (vtstrlen(path_str) + vtstrlen(dbname) >= FILENMLEN)
        {
            psp_freeMemory(dbname, 0);
            return (dberr(S_NAMELEN));
        }

        vtstrcpy(filespec, dbname);             /* Copy to working space */
        psp_freeMemory(dbname, 0);
        vtstrcat(filespec, psp_pathStripDrive(path_str));
        vtstrcpy(path_str, filespec);           /* Place in o/p string */
        if (isabs(filespec))
            return (task->db_status);
    }

    /* Now add the path specification from DBFPATH, if defined */
    if (dbfpath != NULL)
    {
        int i = vtstrlen(dbfpath);               /* Make sure it ends with DIRCHAR */
        if (i && (dbfpath[i - 1] != DIRCHAR) && (dbfpath[i - 1] != DB_TEXT(';')))
        {
            dbfpath[i++] = DIRCHAR;
            dbfpath[i] = DB_TEXT('\0');
        }

        if (vtstrlen(path_str) + vtstrlen(dbfpath) >= FILENMLEN)
            return (dberr(S_NAMELEN));

        vtstrcpy(filespec, dbfpath);
        vtstrcat(filespec, psp_pathStripDrive(path_str));
        vtstrcpy(path_str, filespec);
    }

    return (task->db_status);
}


/* ======================================================================
    extracts a single element from DBxPATH type variables
*/
DB_TCHAR *EXTERNAL_FCN get_element(
    const DB_TCHAR *dbxpath,
    int num)                          /* Element to extract (0 = first) */
{
/*
    RETURNS: Pointer to string with element.  NULL if no such element
             exists.  If there are not 'num' elements in the string, then
             if there is only one element then return the only element else
             return NULL.
    ASSUMES: dbxpath cannot exceed 2*FILENMLEN chars.
             DBxPATH, elements separated by semicolons,
             NULL causes get_element to return NULL.
             A NULL string also causes a NULL return.

*/

    static DB_TCHAR element[2 * FILENMLEN + 1];      /* return value */
    int             i, manyElementsInPath;
    const DB_TCHAR *dbx_ptr;

    if (!dbxpath || !*dbxpath)
        return NULL;                   /* NULL string? */

    dbx_ptr = dbxpath;
    manyElementsInPath = 0;
    do
    {
        i = 0; element[i] = DB_TEXT('\0');
        if (!*dbx_ptr)
        {
            /* we are at the end of dbxpath */
            if (manyElementsInPath)
                element[0] = DB_TEXT('\0');
            else
                vtstrcpy(element, dbxpath);

            break;
        }

        while (*dbx_ptr && *dbx_ptr != DB_TEXT(';'))
            element[i++] = *dbx_ptr++;

        element[i] = DB_TEXT('\0');
        if (*dbx_ptr == DB_TEXT(';'))
        {
            manyElementsInPath = 1;
            dbx_ptr++;                 /* move past it */
        }
    } while (num-- > 0);

    if ((i = vtstrlen(element)) == 0)
        return NULL;

    if (!psp_pathIsDir(element))
    {
        element[i++] = DIRCHAR;
        element[i] = DB_TEXT('\0');
    }

    return element;
}


/* ======================================================================
    Returns TRUE of path is absolute
*/
static int INTERNAL_FCN isabs(const DB_TCHAR *path_str)
{
/*
    RETURNS: TRUE if path is absolute
    ASSUMES: None.  Path to test, NULL returns FALSE.
*/
    DB_TCHAR *path;              /* Points to path w/o drive spec */

    if (path_str == NULL)
        return FALSE;

    path = psp_pathStripDrive(path_str);

    if (path[0] != DIRCHAR)
        return FALSE;

    return TRUE;
}


/* ======================================================================
    Set Country Table path
*/
int INTERNAL_FCN dctbpath(const DB_TCHAR *ctb, DB_TASK *task)
{
    if (task->dbopen)
        return (dberr(S_DBOPEN));
        
    if (ctb && *ctb)
    {
        int i = vtstrlen(ctb);
        if (!psp_pathIsDir(ctb))
        {
            /* need to add DIRCHAR to the end of the path */
            if (i < DB_PATHLEN - 2)
            {
                vtstrcpy(task->ctbpath, ctb);
                task->ctbpath[i++] = DIRCHAR;
                task->ctbpath[i] = DB_TEXT('\0');
            }
            else
                return (dberr(S_NAMELEN));
        }
        else
        {
            /* already has the directory char at the end of path */
            if (i < DB_PATHLEN - 1)
                vtstrcpy(task->ctbpath, ctb);
            else
                return (dberr(S_NAMELEN));
        }
    }

    return (task->db_status);
}



