/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbedit utility                                    *
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

    dbe_help.c - DBEDIT, help command

    The function help() handles display of the help screens in both
    dbedit and hex mode. All help text is contained in this file.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_str.h"
#include "dbe_io.h"
#include "dbe_ext.h"

/* ********************** LOCAL VARIABLES **************************** */

/* Help text for dbedit mode */

DB_TCHAR help_1[] =

DB_TEXT("\n\n")
DB_TEXT("DBEDIT COMMANDS\n")
DB_TEXT("\n")
DB_TEXT("base 10/16 . . . . . . . . . . . . . . Output and interpret addresses and\n")
DB_TEXT("                                       counts as decimal / hex\n")
DB_TEXT("display [type/dba/ts/opt/set/mem/fld]. Display contents of current record\n")
DB_TEXT("edit type/dba/opt/first/last/  . . . . Edit contents of current record or\n")
DB_TEXT("     count/own/prev/next/              file\n")
DB_TEXT("     dchain/nextslot/hex\n")
DB_TEXT("exit . . . . . . . . . . . . . . . . . Exit from dbedit\n")
DB_TEXT("fields . . . . . . . . . . . . . . . . Restore fields (display command)\n")
DB_TEXT("goto first/last/own/prev/next/ . . . . Goto new address or file\n")
DB_TEXT("     prevrec/nextrec/file/[n:nnn]\n")
DB_TEXT("help/? . . . . . . . . . . . . . . . . Display list of commands\n")
DB_TEXT("nofields . . . . . . . . . . . . . . . Suppress fields (display command)\n")
DB_TEXT("notitles . . . . . . . . . . . . . . . Suppress titles (display command)\n")
DB_TEXT("reread . . . . . . . . . . . . . . . . Cancel changes to current record\n")
DB_TEXT("show [fld/file/key/record/set] . . . . Show database information\n")
DB_TEXT("source filename. . . . . . . . . . . . Read commands from file\n")
DB_TEXT("titles . . . . . . . . . . . . . . . . Restore titles (display command)\n")
DB_TEXT("verify setname . . . . . . . . . . . . Check consistency of set pointers\n\n");


/* Help text for hex mode */

DB_TCHAR help_2[] =

DB_TEXT("\n\n\n\n")
DB_TEXT("EDIT HEX COMMANDS\n")
DB_TEXT("\n")
DB_TEXT("+   Forwards 1 character         >   Forwards 1 line\n")
DB_TEXT("+N  Forwards hex N chars         >N  Forwards hex N lines\n")
DB_TEXT("-   Backwards 1 character        <   Backwards 1 line\n")
DB_TEXT("-N  Backwards hex N chars        <N  Backwards hex N lines\n")
DB_TEXT("\n")
DB_TEXT("=N  Move to position hex N bytes from start of file\n")
DB_TEXT("\n")
DB_TEXT(">>STRING  Search forwards for STRING (from current position)\n")
DB_TEXT("<<STRING  Search backwards for STRING (from current position)\n")
DB_TEXT("\n")
DB_TEXT("cancel        Cancel all edits since entry into edit hex\n")
DB_TEXT("end           End edit hex\n")
DB_TEXT("help/?        List edit hex commands\n")
DB_TEXT("print N       Print hex N lines (from current position)\n")
DB_TEXT("write STRING  Overwrite file contents (starting at current position)\n")
DB_TEXT("              with STRING\n")
DB_TEXT("\n")
DB_TEXT("NOTE: STRING may be specified in ascii, enclosed by double quotes,\n")
#if defined(UNICODE)
DB_TEXT("      as Unicode text, enclosed by double quotes and preceeded by L,\n")
#endif
DB_TEXT("      or as a sequence of hex bytes, e.g., \"string\" or 73 74 72 69 6E 67\n\n");


/* Display help text, dbedit or hex mode
*/
int help(int mode, DB_TASK *task)
{
    DB_TCHAR    line[LINELEN];
    short       fno;
    DB_ULONG    rno;

    d_decode_dba(task->curr_rec, &fno, &rno);

    if (mode)
    {
        dbe_out(help_2, STDOUT);
    }
    else
    {
        dbe_out(help_1, STDOUT);

        /* Display address of current record */
        vstprintf(line,
                  decimal ? DB_TEXT("%s[%d:%ld]\n") : DB_TEXT("%s[%x:%lx]\n"),
                  dbe_getstr(M_CREC), (int) fno, (long) rno);
        dbe_out(line, STDOUT);
    }

    /* Display current file number */
    vstprintf(line, DB_TEXT("%s%s (%d)\n"),
              dbe_getstr(M_CFILE), task->file_table[fno].ft_name, (int) fno);
    dbe_out(line, STDOUT);
    return (0);
}


