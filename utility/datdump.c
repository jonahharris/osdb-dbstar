/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, datdump utility                                   *
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

    datdump.c - db.* Data File Dump Utility

    To execute:
        datdump [-h] [-x] [-f] [r#] [-sg [<mode>,]<passwd>] dbname datfile

    Where:
        -h      = Print header info only (no field info)
        -x      = Print hex dump only
        -f      = Print full formatted dump (no hex dump)
        -r#     = Print only the record at slot dddd
        -sg     = Specify encryption information
        dbname  = name of database
        datfile = name of data file to be dumped

-----------------------------------------------------------------------*/

#define MOD datdump
#include "db.star.h"
#include "version.h"

#define DATDUMP
#include "cnvfld.h"

#define MAXTEXT 1600


/* ********************** LOCAL FUNCTION DECLARATIONS **************** */

static F_ADDR phys_addr(FILE_NO, F_ADDR, DB_TASK *);
static void pr_fields(short, char *, DB_TASK *);
static void pr_filehdr(FILE_NO, DB_TCHAR *, DB_TCHAR *, DB_TASK *);
static void pr_hex(short, F_ADDR, char *, DB_TASK *);
static void pr_rechdr(short, short, DB_BOOLEAN, DB_BOOLEAN, char *, DB_TASK *);
static int  pr_record(FILE_NO, F_ADDR, DB_BOOLEAN, DB_BOOLEAN, DB_BOOLEAN, DB_TASK *);
static int  tstopt(int, char *, DB_TASK *);
static void usage(void);


/* List the valid data file names *****************************************
*/
static void list_data_files(DB_TASK *task)
{
    int i;

    vftprintf(stderr, DB_TEXT("Valid data file names are:\n"));
    for (i = 0; i < task->size_ft; i++)
    {
        if (task->file_table[i].ft_type == 'd')
            vftprintf(stderr, DB_TEXT("\t%s\n"), task->file_table[i].ft_name);
    }
}


void EXTERNAL_FCN datdump_dberr(int errnum, DB_TCHAR *errmsg)
{
    vtprintf(DB_TEXT("\n%s (errnum = %d)\n"), errmsg, errnum);
}


/* db.* Data File Dump Utility ******************************
*/
int MAIN(int argc, DB_TCHAR *argv[])
{
    DB_TCHAR    dbfname[FILENMLEN];
    DB_TCHAR   *datfile;
    DB_TCHAR   *dbname;
    DB_BOOLEAN  formatted;
    DB_BOOLEAN  header;
    DB_BOOLEAN  hex;
    FILE_NO     fno;
    F_ADDR      rno;
    F_ADDR      pz_next;
    int         i;
    int         retcode = 1;
    int         status = S_OKAY;
    DB_TASK    *task;
    SG         *sg = NULL;
#if defined(SAFEGARDE)
    DB_TCHAR   *cp;
    DB_TCHAR   *password = NULL;
    int         mode = NO_ENC;
#endif

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Data File Dump")));

    /* process the options */
    if (argc < 2)
    {
        usage();
        return(1);
    }

    dbname = NULL; 
    datfile = NULL;
    rno = 0L;
    formatted = header = hex = TRUE;

    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] == DB_TEXT('-'))
        {
            switch (vtotlower(argv[i][1]))
            {
                case DB_TEXT('x'):
                    formatted = header = FALSE;
                    break;

                case DB_TEXT('h'):
                    formatted = hex = FALSE;
                    break;

                case DB_TEXT('f'):
                    hex = FALSE;
                    break;

                case DB_TEXT('r'):
                    rno = vttol(&argv[i][2]);
                    break;

                case DB_TEXT('s'):
                    if (vtotlower(argv[i][2]) != DB_TEXT('g') || i == argc - 1)
                    {
                        usage();
                        return 1;
                    }

#if defined(SAFEGARDE)
                    if ((cp = vtstrchr(argv[++i], DB_TEXT(','))) != NULL)
                        *cp++ = DB_TEXT('\0');

                    if (cp)
                    {
                        if (vtstricmp(argv[i], DB_TEXT("low")) == 0)
                            mode = LOW_ENC;
                        else if (vtstricmp(argv[i], DB_TEXT("med")) == 0)
                            mode = MED_ENC;
                        else if (vtstricmp(argv[i], DB_TEXT("high")) == 0)
                            mode = HIGH_ENC;
                        else
                        {
                            usage();
                            return 1;
                        }

                        password = cp;
                    }
                    else
                    {
                        mode = MED_ENC;
                        password = argv[i];
                    }

                    break;
#else
                    vftprintf(stderr, DB_TEXT("SafeGarde is not available in this version\n"));
                    usage();
                    return 1;
#endif

                default:
                    usage();
                    return(1);
            }
        }
        else if (dbname == NULL)
            dbname = argv[i];
        else
            datfile = argv[i];
    }

    if (! dbname)
    {
        vftprintf(stderr, DB_TEXT("A database name must be supplied.\n"));
        usage();
        return(1);
    }

    if ((status = d_opentask(&task)) == S_OKAY)
    {
#if defined(SAFEGARDE)
	if (mode != NO_ENC && (sg = sg_create(mode, password)) == NULL)
	{
	    vftprintf(stderr, DB_TEXT("A SafeGarde context could not be created.\n"));
	    return 1;
	}
#endif

        if ((status = d_set_dberr(datdump_dberr, task)) == S_OKAY)
        {
            if ((status = d_on_opt(READNAMES, task)) == S_OKAY)
            {
                if ((status = d_open_sg(dbname, DB_TEXT("o"), sg, task)) == S_OKAY)
                {
                    /* make sure all components are present */
                    if (! datfile)
                    {
                        vftprintf(stderr, DB_TEXT("A data file name must be supplied.\n"));
                        list_data_files(task);   /* database must be open for this */
                    }
                    else
                    {
                        /* look up file table entry */
                        con_dbf(dbfname, datfile, dbname,
                                  get_element(task->dbfpath, 0), task);

                        for (fno = 0; fno < task->size_ft; ++fno)
                        {
                            if (psp_fileNameCmp(task->file_table[fno].ft_name,
                                    dbfname) == 0)
                                break;
                        }
                        if (fno == task->size_ft)
                        {
                            vftprintf(stderr, DB_TEXT("File '%s' not found in database %s\n"),
                                      datfile, dbname);
                            list_data_files(task);
                        }
                        else
                        {
                            if (task->file_table[fno].ft_type != 'd')
                            {
                                vftprintf(stderr, DB_TEXT("File '%s' is not a data file\n"),
                                          datfile);
                                list_data_files(task);
                            }
                            else
                            {
                                if ((status = dio_pzread(fno, task)) == S_OKAY)
                                {
                                    pz_next = task->pgzero[fno].pz_next;

                                    if (rno > 0L && rno < pz_next)
                                    {
                                        retcode = pr_record(fno, rno, formatted,
                                                                  header, hex, task);
                                    }
                                    else if (rno == 0L)
                                    {
                                        pr_filehdr(fno, datfile, dbname, task);
    
                                        /* read each record in data file */
                                        for (rno = 1L; rno < pz_next; ++rno)
                                        {
                                            if ((retcode = pr_record(fno, rno, formatted,
                                                                            header, hex, task)))
                                                break;
                                        }
                                        if (rno == pz_next)
                                        retcode = 0;
                                    }
                                    else
                                    {
                                        vftprintf(stderr, DB_TEXT("invalid record slot number\n"));
                                    }
                                }
                            }
                        }
                    }
                    d_close(task);
                }
            }
        }
        d_closetask(task);
    }
    if (status != S_OKAY)
    {
        switch (status)
        {
            case S_INVDB:
                vftprintf(stderr, DB_TEXT("Unable to locate database: %s\n"), dbname);
                break;
    
            case S_INCOMPAT:
                vftprintf(stderr, DB_TEXT("Incompatible dictionary file. Re-run ddlp.\n"));
                break;

            default:
                vftprintf(stderr, DB_TEXT("Error %d opening database.\n"), status);
                break;
        }
    }
    return(retcode);
}


/* Print record contents ***************************************************
*/
static int pr_record(
    FILE_NO fno,
    F_ADDR rno,
    DB_BOOLEAN formatted,
    DB_BOOLEAN header,
    DB_BOOLEAN hex,
    DB_TASK *task)
{
    F_ADDR       addr;
    DB_ADDR      dba;
    short     rid;
    short     rtn;
    DB_BOOLEAN   rlb;
    DB_BOOLEAN   deleted;
    char        *rec;

    d_encode_dba(fno, rno, &dba);

    /* read record */
    if ( dio_read(dba, &rec, NOPGHOLD, task) != S_OKAY )
        return(1);

    /* get and interpret record identification number */
    memcpy(&rid, rec, sizeof(short));
    rtn = rid;
    if (rtn < 0)
    {
        deleted = TRUE;
        rtn = (short) ~rtn;
    }
    else
    {
        deleted = FALSE;
    }
    if (rtn & RLBMASK)
    {
        rlb = TRUE;
        rtn &= ~RLBMASK;
    }
    else
    {
        rlb = FALSE;
    }

    /* print first line of record dump */
    addr = phys_addr(fno, rno, task);
    vtprintf(DB_TEXT("SLOT: %-7ld  ADDR: %lxH    TYPE: %s\n"),
             rno, addr, task->record_names[rtn]);

    if (header || formatted)
    {
        /* print formatted header info */
        pr_rechdr(rid, rtn, rlb, deleted, rec, task);
    }
    if (formatted)
    {
        /* print field contents */
        pr_fields(rtn, rec, task);
    }
    if (hex)
    {
        /* print hex dump */
        pr_hex(rtn, addr, rec, task);
    }
    vtprintf(DB_TEXT("\n"));
    
    return(0);
}


/* Print data file header info *********************************************
*/
static void pr_filehdr(
    FILE_NO fno,
    DB_TCHAR *datfile,
    DB_TCHAR *dbname,
    DB_TASK *task)
{
    DB_TCHAR buf[80];

    d_dbver(DB_TEXT("%V"), buf, sizeof(buf) / sizeof(DB_TCHAR));
    vtprintf(DB_TEXT("%s\n"), buf);
    vtprintf(DB_TEXT("Data File Dump Utility\n"));
    d_dbver(DB_TEXT("%C"), buf, sizeof(buf) / sizeof(DB_TCHAR));
    vtprintf(DB_TEXT("%s\n"), buf);
    vtprintf(DB_TEXT("------------------------------------"));
    vtprintf(DB_TEXT("------------------------------------\n"));
    vtprintf(DB_TEXT("DATA FILE     : %s\n"), datfile);
    vtprintf(DB_TEXT("DATABASE      : %s\n"), dbname);
    vtprintf(DB_TEXT("TIMESTAMP     : %lu\n"), task->pgzero[fno].pz_timestamp);
    vtprintf(DB_TEXT("PAGE SIZE     : %d\n"), task->file_table[fno].ft_pgsize);
    vtprintf(DB_TEXT("SLOT SIZE     : %d\n"), task->file_table[fno].ft_slsize);
    vtprintf(DB_TEXT("SLOTS PER PAGE: %d\n"), task->file_table[fno].ft_slots);
    vtprintf(DB_TEXT("NEXT SLOT     : %ld (%08lX)\n"), task->pgzero[fno].pz_next, task->pgzero[fno].pz_next);
    vtprintf(DB_TEXT("DELETE CHAIN  : %ld (%08lX)\n"), task->pgzero[fno].pz_dchain, task->pgzero[fno].pz_dchain);
    vtprintf(DB_TEXT("------------------------------------"));
    vtprintf(DB_TEXT("------------------------------------\n\n\n"));
}

/* Print record header *****************************************************
*/
static void pr_rechdr(
    short rid,
    short rtn,
    DB_BOOLEAN rlb,
    DB_BOOLEAN deleted,
    char *rec,
    DB_TASK *task)
{
    DB_BOOLEAN     printed;
    DB_ADDR        dba;
    long           cts, uts;
    register int   fld, set, mbr;
    short          fileO, fileF, fileL;
    DB_ULONG       slotO, slotF, slotL;
    SET_PTR        sp;
    MEM_PTR        mp;

    /* get database address */
    memcpy(&dba, rec + sizeof(short), sizeof(DB_ADDR));

    /* print general info */
    vtprintf(DB_TEXT("   RID: %04hX    RLB: %s   "),
        rid, (rlb ? "set  " : "clear"));

    if (deleted)
    {
        vtprintf(DB_TEXT("DELETED, NEXT AT %lu\n"), dba);
        return;
    }
    d_decode_dba(dba, &fileO, &slotO);
    vtprintf(DB_TEXT("DBA: [%03d:%07lu]   "), fileO, slotO);

    /* print any timestamp info */
    if (task->record_table[rtn].rt_flags & TIMESTAMPED)
    {
        /* get and print timestamps */
        memcpy(&cts, rec + RECCRTIME, sizeof(long));
        memcpy(&uts, rec + RECUPTIME, sizeof(long));
        vtprintf(DB_TEXT("CTIME: %06lu   UTIME: %06lu"), cts, uts);
    }
    vtprintf(DB_TEXT("\n"));

    /* print any optional key settings */
    for ( printed = FALSE, fld = task->record_table[rtn].rt_fields;
          fld < task->record_table[rtn].rt_fields + task->record_table[rtn].rt_fdtot;
          ++fld)
    {
        if (task->field_table[fld].fd_flags & OPTKEYMASK)
        {
            if (!printed)
            {
                vtprintf(DB_TEXT("\n   STORED OPT KEYS: "));
                printed = TRUE;
            }
            if (tstopt(fld, rec, task) == S_DUPLICATE)
                vtprintf(DB_TEXT("%s  "), task->field_names[fld]);
        }
    }
    if (printed)
        vtprintf(DB_TEXT("\n"));

    /* print any set pointers */
    for (printed = FALSE, set = 0; set < task->size_st; ++set)
    {
        if (task->set_table[set].st_own_rt == rtn)
        {
            /* print set pointer */
            if (! printed)
            {
                /* print set pointer header */
                vtprintf(
                    DB_TEXT("\n   SET POINTER              COUNT   FIRST          LAST"));
                if (task->set_table[set].st_flags & TIMESTAMPED)
                    vtprintf(DB_TEXT("           UTIME"));
                vtprintf(DB_TEXT("\n"));
                printed = TRUE;
            }
            memcpy(&sp, rec + task->set_table[set].st_own_ptr, SETPSIZE);
            d_decode_dba(sp.first, &fileF, &slotF);
            d_decode_dba(sp.last, &fileL, &slotL);
            vtprintf(DB_TEXT("   %3d %-20.20s %06lu  [%03d:%07lu]  [%03d:%07lu]"),
                set, task->set_names[set], sp.total, fileF, slotF, fileL, slotL);
            if (task->set_table[set].st_flags & TIMESTAMPED)
                vtprintf(DB_TEXT("  %06lu"), sp.timestamp);
            vtprintf(DB_TEXT("\n"));
        }
    }

    /* print any member pointers */
    for (printed = FALSE, set = 0; set < task->size_st; ++set)
    {
        for ( mbr = task->set_table[set].st_members;
              mbr < task->set_table[set].st_members + task->set_table[set].st_memtot;
              ++mbr)
        {
            if (task->member_table[mbr].mt_record == rtn)
            {
                /* print member pointer */
                if (! printed)
                {
                    /* print member pointer header */
                    vtprintf(
                        DB_TEXT("\n   MEMBER POINTER           OWNER          PREVIOUS       NEXT\n"));
                    printed = TRUE;
                }
                memcpy(&mp, rec + task->member_table[mbr].mt_mem_ptr, MEMPSIZE);
                d_decode_dba(mp.owner, &fileO, &slotO);
                d_decode_dba(mp.prev, &fileF, &slotF);
                d_decode_dba(mp.next, &fileL, &slotL);
                vtprintf(
                    DB_TEXT("   %3d %-20.20s [%03d:%07lu]  [%03d:%07lu]  [%03d:%07lu]\n"),
                    set, task->set_names[set],
                    fileO, slotO, fileF, slotF, fileL, slotL);
            }
        }
    }
}

/* Print the field contents ************************************************
*/
static void pr_fields(short rtn, char *rec, DB_TASK *task)
{
    int fld;
    DB_TCHAR buf[MAXTEXT];

    if (task->record_table[rtn].rt_fdtot <= 0)
        return;

    vtprintf(DB_TEXT("\n   FIELD                    CONTENTS\n"));
    
    /* for each field type in record */
    for ( fld = task->record_table[rtn].rt_fields;
          fld < task->record_table[rtn].rt_fields + task->record_table[rtn].rt_fdtot;
          ++fld)
    {
        if (task->field_table[fld].fd_flags & STRUCTFLD)
            continue;
        vtprintf(DB_TEXT("  %4d %-20.20s "), fld, task->field_names[fld]);
        fldtotxt(fld, rec + task->field_table[fld].fd_ptr, buf, 0, task);
        vtprintf(DB_TEXT("%s\n"), buf);
    }
}

/* Print hex dump of record contents ***************************************
*/
static void pr_hex(short rtn, F_ADDR addr, char *rec, DB_TASK *task)
{
    DB_TCHAR       hln[50];
    DB_TCHAR       cln[30];
    int            p;
    register int   i;

    for (hln[0] = cln[0] = 0, i = 0; i < task->record_table[rtn].rt_len; ++i)
    {
        p = i % 12;
        if (p == 0)
        {
            /* print previous line */
            vtprintf(DB_TEXT("%-45.45s  %s\n"), hln, cln);

            /* print new address */
            vstprintf(hln, DB_TEXT("   %05lX:  "), addr + i);
        }
        vstprintf(hln + (p * 3) + 10, DB_TEXT("%02X "), (0xFF & (int) rec[i]));
        vstprintf(cln + (p * 2), DB_TEXT(" %c"),
            (rec[i] < ' ' || rec[i] > '~') ? '.' : rec[i]);
    }
    vtprintf(DB_TEXT("%-45.45s  %s\n"), hln, cln);
}

/* Calculate a physical address from a database address ********************
*/
static F_ADDR phys_addr(FILE_NO fno, F_ADDR rno, DB_TASK *task)
{
    F_ADDR page, offset, addr;

    page = (rno - 1L) /  + 1L;
    offset = (F_ADDR) task->file_table[fno].ft_slsize *
             ((rno - 1L) % (F_ADDR) task->file_table[fno].ft_slots) + PGHDRSIZE;
    addr = page * (F_ADDR) task->file_table[fno].ft_pgsize + offset;
    return (addr);
}

/* Test the optional key field "stored" bit ********************************
*/
static int tstopt(
    int field,                       /* Field table index of optional key */
    char *rec,                       /* Pointer to record */
    DB_TASK *task)
{
    int offset;                      /* offset to the bit map */
    int keyndx;                      /* index into bit map of this key */
    int byteno, bitno;               /* position within bit map of this key */

    /* calculate the position to the bit map */
    offset = task->record_table[task->field_table[field].fd_rec].rt_flags & TIMESTAMPED ?
             RECHDRSIZE + 2 * sizeof(long) : RECHDRSIZE;

    /* extract the index into the bit map of this key */
    keyndx = (((task->field_table[field].fd_flags & OPTKEYMASK) >> OPTKEYSHIFT)
              & OPTKEYNDX) - 1;
    if (keyndx < 0)
        return (dberr(S_SYSERR));

    /* determine which byte, and which bit within the byte */
    byteno = keyndx / BITS_PER_BYTE;
    bitno = keyndx % BITS_PER_BYTE;

    /* extract the bit */
    if (*(rec + offset + byteno) & (1 << (BITS_PER_BYTE - bitno - 1)))
        return (S_DUPLICATE);
    else
        return (S_OKAY);
}

/* Print usage message and exit ********************************************
*/
static void usage()
{
    vftprintf(stderr, DB_TEXT("usage: datdump [-x] [-f] [-h] [-rN] [-sg [<mode>,]<password>] dbname filename\n"));
    vftprintf(stderr, DB_TEXT("    where   -x  selects hex output only (no formatted output)\n"));
    vftprintf(stderr, DB_TEXT("            -f  selects formatted output only (no hex output)\n"));
    vftprintf(stderr, DB_TEXT("            -h  selects header info only (no field info)\n"));
    vftprintf(stderr, DB_TEXT("            -r  prints record at slot N only\n"));
    vftprintf(stderr, DB_TEXT("            -sg specifies the encryption information for the database\n"));
    vftprintf(stderr, DB_TEXT("                <mode> can be 'low', 'med' (default), or 'high'\n"));
    vftprintf(stderr, DB_TEXT("Default is both formatted and hex print of entire file.\n"));
    vftprintf(stderr, DB_TEXT("To see list of data files, omit filename from command line.\n"));
}

VXSTARTUP("datdump", 0)

