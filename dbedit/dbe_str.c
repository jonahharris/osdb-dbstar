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

    dbe_str.c - DBEDIT, program text strings

    This file contains the function dbe_getstr, through which all dbedit
    functions get all required text strings for titles, prompts etc.
    Apart from help text (in DBE_HELP.C) and error messages (in
    DBE_ERR.C) all text is located in this file.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_str.h"


/* Get prompt / title string ***********************************************
*/
DB_TCHAR *dbe_getstr(int n)
{
    DB_TCHAR *p = NULL;

    switch (n)
    {
        case M_DBEDIT:    p = DB_TEXT("Database Edit Utility\n");         break;

    /* General */
        case M_TAB:       p = DB_TEXT("        ");            break;
        case M_PROMPT:    p = DB_TEXT("dbedit> ");            break;
        case M_FLAGFF:    p = DB_TEXT("SORTFLD");             break;
        case M_FLAGFS:    p = DB_TEXT("STRUCTFLD");           break;
        case M_FLAGFU:    p = DB_TEXT("UNSIGNEDFLD");         break;
        case M_FLAGFO:    p = DB_TEXT("Opt Key ");            break;
        case M_FLAGFC:    p = DB_TEXT("COMKEYED");            break;
        case M_FLAGRT:    p = DB_TEXT("TIMESTAMPED");         break;
        case M_FLAGRS:    p = DB_TEXT("STATIC");              break;
        case M_FLAGRL:    p = DB_TEXT("LOCAL");               break;
        case M_FLAGRC:    p = DB_TEXT("COMKEYED");            break;
        case M_FLAGST:    p = DB_TEXT("TIMESTAMPED");         break;
        case M_FLAGLB:    p = DB_TEXT(" - RLB");              break;
        case M_FLAGDE:    p = DB_TEXT(" - DELETED");          break;
        case M_FLAGKS:    p = DB_TEXT(" - STORED");           break;
        case M_FLAGKN:    p = DB_TEXT(" - NOT STORED");       break;
        case M_CREC:      p = DB_TEXT("Current record: ");    break;
        case M_CFILE:     p = DB_TEXT("Current file:   ");    break;

    /* Strings for disp command */
        case M_DISPRT:    p = DB_TEXT("Record Type");                  break;
        case M_DISPDA:    p = DB_TEXT("Database Address");             break;
        case M_DISPCU:    p = DB_TEXT("Creation/Update Timestamp");    break;
        case M_DISPOK:    p = DB_TEXT("Stored Optional Keys");         break;
        case M_DISPSO:    p = DB_TEXT("Set Owner Pointers (C/F/L)");   break;
        case M_DISPSM:    p = DB_TEXT("Set Member Pointers (O/P/N)");  break;
        case M_DISPFD:    p = DB_TEXT("Field ");                       break;

    /* Strings for show command */
        case M_SHOWFL:    p = DB_TEXT("Files:\n");                  break;
        case M_SHOWRD:    p = DB_TEXT("Records:\n");                break;
        case M_SHOWRT:    p = DB_TEXT("Record Type: ");             break;
        case M_SHOWRC:    p = DB_TEXT("\nContained in file: ");     break;
        case M_SHOWRL:    p = DB_TEXT("\nRecord Length: ");         break;
        case M_SHOWRO:    p = DB_TEXT("\nOffset to Data: ");        break;
        case M_SHOWRG:    p = DB_TEXT("\nFlags: ");                 break;
        case M_SHOWRF:    p = DB_TEXT("Fields:\n");                 break;
        case M_SHOWSL:    p = DB_TEXT("Sets:\n");                   break;
        case M_SHOWST:    p = DB_TEXT("Set Type: ");                break;
        case M_SHOWSD:    p = DB_TEXT("\nOrder: ");                 break;
        case M_SHOWSF:    p = DB_TEXT("\n        By Field(s): ");   break;
        case M_SHOWSO:    p = DB_TEXT("\nOwner: ");                 break;
        case M_SHOWSM:    p = DB_TEXT("\nMember(s): ");             break;
        case M_SHOWSG:    p = DB_TEXT("\nFlags: ");                 break;
        case M_SHOWKL:    p = DB_TEXT("Keys:\n");                   break;
        case M_SHOWFF:    p = DB_TEXT("    Flags: ");               break;

    /* Strings for verify command */
        case M_VRFYDE:    p = DB_TEXT("\n*** DATABASE ERROR ***\n");   break;
        case M_VRFYST:    p = DB_TEXT("*** in set ");                  break;
        case M_VRFYMA:    p = DB_TEXT(", member database address: ");  break;
        case M_VRFYOA:    p = DB_TEXT(", owner database address: ");   break;
        case M_VRFYAM:    p = DB_TEXT(", actual: ");                   break;
        case M_VRFYCO:    p = DB_TEXT("\nVerify completed\n");         break;
        case M_VRFY1E:    p = DB_TEXT(" error was detected\n");        break;
        case M_VRFYER:    p = DB_TEXT(" errors were detected\n");      break;
        case M_VRFYMM:    p = DB_TEXT("mismatch in membership count, set pointers: ");
                                break;

    /* Strings for edit command */
        case M_EDITCV:    p = DB_TEXT("Current value is ");                     break;
        case M_EDITNV:    p = DB_TEXT("New value? ");                           break;
        case M_HEX:       p = DB_TEXT("hex> ");                                 break;
        case M_MOVE:      p = DB_TEXT("Change current record to here? (y/n) "); break;

    /* Set Ordering */
        case M_SETFST:    p = DB_TEXT("First");         break;
        case M_SETLST:    p = DB_TEXT("Last");          break;
        case M_SETASC:    p = DB_TEXT("Ascending");     break;
        case M_SETDSC:    p = DB_TEXT("Descending");    break;
        case M_SETNXT:    p = DB_TEXT("Next");          break;

    /* Field types */
        case M_UNSIGN:    p = DB_TEXT("unsigned ");     break;
        case M_SHORT:     p = DB_TEXT("short ");        break;
        case M_INT:       p = DB_TEXT("int ");          break;
        case M_LONG:      p = DB_TEXT("long ");         break;
        case M_CHAR:      p = DB_TEXT("char ");         break;
        case M_WIDECHAR:  p = DB_TEXT("wchar_t ");      break;
        case M_FLOAT:     p = DB_TEXT("float ");        break;
        case M_DOUBLE:    p = DB_TEXT("double ");       break;
        case M_STRUCT:    p = DB_TEXT("struct {");      break;
        case M_ADDR:      p = DB_TEXT("db_addr ");      break;
        case M_COMP:      p = DB_TEXT("compound ");     break;
        case M_OPT:       p = DB_TEXT("optional ");     break;
        case M_UNIQ:      p = DB_TEXT("unique ");       break;
        case M_KEY:       p = DB_TEXT("key ");          break;
        case M_ASC:       p = DB_TEXT(" ascending");    break;
        case M_DESC:      p = DB_TEXT(" descending");   break;
        default:          break;
    }

    return (p);
}


