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

    dbe_err.c - DBEDIT, error message handling

    This file contains the functions dbe_err, which displays the error
    message corresponding to the error number passed to it.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_err.h"
#include "dbe_io.h"
#include "dbe_ext.h"


/* Display error message
*/
int dbe_err(
    int error,         /* Error string number - errors < 0, warnings > 0 */
    DB_TCHAR *string)  /* Append this string to error message, if not null */
{
    DB_TCHAR errstr[2 * LINELEN], *p;

    switch (error)
    {
        case 0:        return(0);

#if defined(UNICODE)
        case USAGE:    p = L"usage: dbedit [-u] database\n"
                       L"  where -u sepcifies Unicode input files for source commands";
#else
        case USAGE:    p = "usage: dbedit database\n";
#endif
                       break;

        case BAD_COM:  p = DB_TEXT("Invalid command");                       break;
        case BAD_OPT:  p = DB_TEXT("Invalid command option");                break;
        case BAD_TOK:  p = DB_TEXT("Invalid token");                         break;
        case UNX_END:  p = DB_TEXT("Unexpected end of string");              break;
        case UNX_REC:  p = DB_TEXT("Unexpected record name");                break;
        case UNX_SET:  p = DB_TEXT("Unexpected set name");                   break;
        case UNX_FILE: p = DB_TEXT("Unexpected file name");                  break;
        case UNX_FLD:  p = DB_TEXT("Unexpected field name");                 break;
        case UNX_KEY:  p = DB_TEXT("Unexpected key name");                   break;
        case UNX_NUM:  p = DB_TEXT("Unexpected number");                     break;
        case UNX_HEX:  p = DB_TEXT("Unexpected hex number");                 break;
        case UNX_OPT:  p = DB_TEXT("Unexpected command option");             break;
        case UNX_DBA:  p = DB_TEXT("Unexpected database address");           break;
        case UNX_TOK:  p = DB_TEXT("Unexpected token");                      break;
        case ERR_OVF:  p = DB_TEXT("Command line overflow");                 break;
        case ERR_OPEN: p = DB_TEXT("Unable to open database");               break;
        case ERR_VER:  p = DB_TEXT("Incompatible database version");         break;
        case ERR_MEM:  p = DB_TEXT("Unable to allocate sufficient memory");  break;
        case ERR_CREC: p = DB_TEXT("No current record");                     break;
        case ERR_CFIL: p = DB_TEXT("No current file");                       break;
        case ERR_READ: p = DB_TEXT("Read error, file");                      break;
        case ERR_OPN:  p = DB_TEXT("Open error, file");                      break;
        case ERR_NREC: p = DB_TEXT("No more records, file");                 break;
        case ERR_PREC: p = DB_TEXT("No previous record, file");              break;
        case ERR_OSET: p = DB_TEXT("Current record is not owner of set");    break;
        case ERR_MSET: p = DB_TEXT("Current record is not member of set");   break;
        case ERR_RFLD: p = DB_TEXT("Field is not in current record");        break;
        case BAD_DBA:  p = DB_TEXT("Invalid database address");              break;
        case BAD_FILE: p = DB_TEXT("Invalid file number");                   break;
        case BAD_NUM:  p = DB_TEXT("Invalid number");                        break;
        case BAD_TYPE: p = DB_TEXT("Invalid record type");                   break;
        case ERR_RFIL: p = DB_TEXT("Record type is not in current file");    break;
        case BAD_HEX:  p = DB_TEXT("Invalid hexadecimal number");            break;
        case ERR_WRIT: p = DB_TEXT("Write error, file");                     break;
        case ERR_NOPT: p = DB_TEXT("Record has no optional keys");           break;
        case BAD_KWD:  p = DB_TEXT("Non-unique keyword specification:");     break;
        case BAD_DFIL: p = DB_TEXT("File is not a data file");               break;
        case ERR_NMEM: p = DB_TEXT("No more records in set");                break;
        case ERR_PMEM: p = DB_TEXT("No previous record in set");             break;
        case ERR_LEN:  p = DB_TEXT("Command line too long");                 break;
        case BAD_STR:  p = DB_TEXT("Invalid string");                        break;
        case ERR_FPOS: p = DB_TEXT("Invalid file position");                 break;
        case ERR_EOF:  p = DB_TEXT("End of file");                           break;
        case ERR_SNF:  p = DB_TEXT("String not found");                      break;
        case BAD_BASE: p = DB_TEXT("Invalid base");                          break;
        case ERR_ADDR: p = DB_TEXT("File address overflow");                 break;
        case ERR_INP:  p = DB_TEXT("Cannot open input file");                break;
        case BAD_MEMP: p = DB_TEXT("invalid member pointer:");               break;

        case BAD_LAST:
            p = DB_TEXT("invalid last pointer in owner's set pointers:");
            break;

        case BAD_FRST:
            p = DB_TEXT("invalid first pointer in owner's set pointers:");
            break;

        case ERR_CSET:
            p = DB_TEXT("Current record is neither owner nor member of set");
            break;

        case BAD_OWNP:
            p = DB_TEXT("invalid owner pointer in member set pointers:");
            break;

        case BAD_OWNT:
            p = DB_TEXT("owner record type is invalid for set, owner type:");
            break;

        case BAD_MEMT:
            p = DB_TEXT("member record type is invalid for set, member type:");
            break;

        case ERR_PNN:
            p = DB_TEXT("first member record does not have NULL previous pointer");
            break;

        case ERR_NNN:
            p = DB_TEXT("last member record does not have NULL next pointer");
            break;

        case ERR_MEMP:
            p = DB_TEXT("member's member pointers do not point to correct owner");
            break;

        case ERR_PREV:
            p = DB_TEXT("member's previous pointer does not point to previous member");
            break;

        case ERR_PN:
            p = DB_TEXT("member has null previous pointer, but is not first member of set");
            break;

        case ERR_NN:
            p = DB_TEXT("member has null next pointer, but is not last member of set");
            break;

        case WARN_DBA:
            p = DB_TEXT("Warning: address is more than two pages beyond end of file");
            break;

        default:
            p = DB_TEXT("Unrecognised error");
            break;
    }
    vtstrcpy(errstr, p);
    if (string != NULL)
    {
        vtstrcat(errstr, DB_TEXT(" "));
        vtstrcat(errstr, string);
    }
    vtstrcat(errstr, DB_TEXT("\n"));
    dbe_out(errstr, STDERR);
    return 0;
}

void EXTERNAL_FCN dbe_dberr(int n, DB_TCHAR *msg)
{
    DB_TCHAR errstr[2 * LINELEN];

    vstprintf(errstr, DB_TEXT("%s (errnum = %d)\n"), msg, n);
    dbe_out(errstr, STDERR);
}


