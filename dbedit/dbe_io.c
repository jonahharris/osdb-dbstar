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

    dbe_io.c - DBEDIT, program input & ouput functions

    dbe_getline, dbe_out:
    Terminal input and output functions for whole program.

    dbe_open, dbe_read, dbe_write etc:
    File input and output functions for current file. There can only be
    one current file and one current record - all disk IO by functions
    here refers to current file.
    Note that disk IO in the verify command, in DBE_VRFY.C, is performed
    independently, because it involves simultaneous access to more than
    one file. Otherwise all disk IO uses the functions in this file.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "dbe_type.h"
#include "dbe_io.h"
#include "dbe_err.h"
#include "dbe_ext.h"

/* ********************** LOCAL VARIABLES **************************** */
static FILE_NO file_no;           /* current file number in task->file_table */
static FILE *input;               /* current input file */
static PAGE_ENTRY pg0;            /* edit hex mode, page zero buffer */


/* Initialize static variables - can't have global var initializers
   on some systems (VxWorks)
*/
void dbe_ioinit()
{
    file_no = -1;
    input = STDIN;
    memset(&pg0, 0, sizeof(PAGE_ENTRY));
}


/* Output a string - ALL OUTPUT TO SCREEN goes through this function
*/
int dbe_out(DB_TCHAR *string, FILE *channel)
{
    vftprintf(channel, DB_TEXT("%s"), string);
    fflush(channel);
    return 0;
}


/* Get a line of input - ALL KEYBOARD INPUT goes through this function
*/
int dbe_getline(
    DB_TCHAR   *prompt,
    DB_TCHAR   *line,
    int         linelen)
{
    int         i,
                error;
    DB_TINT     c = 0;

    error = 0;
    dbe_out(prompt, STDOUT);
    for (i = 0; i < linelen - 1;)
    {
        c = vgettc(input);
        if ((c == DB_TEXT('\n')) || (c == DB_TEXT('\r')))
            break;
        if (c == DB_TEOF)
        {
            line[i] = 0;
            dbe_out(line, STDOUT);
            in_close();
        }
        else
        {
            line[i++] = (DB_TCHAR) c;
        }
    }
    line[i] = 0;
    if (i >= linelen - 1)
    {
        while ((c != DB_TEXT('\n')) && (c != DB_TEXT('\r')))
        {
            c = vgettc(input);
            if (c == DB_TEOF)
            {
                dbe_out(line, STDOUT);
                in_close();
            }
        }
        error = ERR_LEN;
    }
    if (input != STDIN)
    {
        if (c == DB_TEXT('\r'))
        {
            if ((c = vgettc(input)) != DB_TEXT('\n'))
                vungettc(c, input);
        }

        dbe_out(line, STDOUT);
        dbe_out(DB_TEXT("\n"), STDOUT);
    }
    return (error);
}


/* Open input file (source command)
*/
int in_open(DB_TCHAR *filename)
{
    in_close();
    if ((input = vtfopen(filename,
                                unicode ? DB_TEXT("rb") : DB_TEXT("r"))) == NULL)
    {
        input = STDIN;
        return (ERR_INP);
    }
    return (0);
}


/* Close input file (end of source file) and restore standard input
*/
int in_close()
{
    if (input == STDIN)
        return (0);
    fclose(input);
    input = STDIN;
    fflush(input);
    return (0);
}


/* Calculate physical position in file of a database address (any file)
*/
F_ADDR phys_addr(DB_ADDR dba, DB_TASK *task)
{
    short          fno;              /* file number */
    unsigned long  offset,           /* offset of slot in page */
                   bits,             /* bits needed for addr storage */
                   slot_len,
                   slots_per_pg,
                   pgsize;
    DB_ADDR        sno;              /* slot number */
    F_ADDR         page,             /* page slot is on */
                   addr;             /* physical address of slot in file */

    d_decode_dba(dba, &fno, &sno);
    pgsize         = task->file_table[fno].ft_pgsize;
    slot_len       = task->file_table[fno].ft_slsize;
    slots_per_pg   = task->file_table[fno].ft_slots;
    page = (sno - 1L) / slots_per_pg + 1L;
    offset = slot_len * ((sno - 1L) % slots_per_pg) + PGHDRSIZE;
    for (sno = page, bits = 0; sno; bits++)
        sno >>= 1;
    for (sno = pgsize; sno; bits++)
        sno >>= 1;
    if (bits > sizeof(long) * BITS_PER_BYTE)
        return ((F_ADDR) -1);
    addr = page * pgsize + offset;
    return (addr);
}


/* Calculate the database address occupying a specified physical address
*/
DB_ADDR phys_to_dba(FILE_NO fno, F_ADDR curr_pos, DB_TASK *task)
{
    F_ADDR            pageno;
    unsigned long     offset,
                            slots;
    DB_ULONG          rno;
    DB_ADDR           dba;

    pageno = curr_pos / task->file_table[fno].ft_pgsize;
    offset = curr_pos % task->file_table[fno].ft_pgsize;

    if (pageno < 1)
    {
        pageno = 1;
        offset = PGHDRSIZE;
    }
    if (offset >= PGHDRSIZE)
    {
        offset -= PGHDRSIZE;
        slots = task->file_table[fno].ft_slots;
        rno = offset / task->file_table[fno].ft_slsize;
        if (rno < slots)
        {
            rno += (pageno - 1L) * slots + 1L;
            d_encode_dba(fno, rno, &dba);
            return (dba);
        }
    }
    return ((DB_ADDR) -1);
}


/* Open a file, and make it current file
*/
int dbe_open(FILE_NO fno, DB_TASK *task)
{
    int error, stat;

    if ((fno < 0) || (fno >= task->size_ft))
        return (BAD_FILE);
    if (task->file_table[fno].ft_type == 'k')
        return (BAD_DFIL);
    if (fno == file_no)
        return (0);
    if (changed)
    {
        if ((error = dbe_write(task)) < 0)
            return (error);
        changed = 0;
    }
    dbe_close(task);

    /* Read page zero values - dio_pzread calls dio_open */
    if ((stat = dio_pzread(fno, task)) != S_OKAY)
        return ((stat == S_NOFILE || stat == S_EACCESS) ?
                 ERR_OPEN : ERR_READ);

    file_no = fno;
    return (0);
}


/* Read record from current file (into global structure, slot)
*/
int dbe_read(DB_ADDR dba, DB_TASK *task)
{
    int         error, slot_len;
    short       fno;
    DB_ULONG    recNum;
    char       *p = NULL;

    if ((error = dbe_chkdba(dba, task)) < 0)
        return (error);
    if (changed)
    {
        if ((error = dbe_write(task)) < 0)
            return (error);
        changed = 0;
    }
    d_decode_dba(dba, &fno, &recNum);
    slot_len = task->file_table[fno].ft_slsize;

    if (dio_read(dba, &p, NOPGHOLD, task) != S_OKAY)
        return (ERR_READ);

    if (p)
    {
        memset(slot.buffer, 0, slot.size);
        memcpy(slot.buffer, p, slot_len);
    }
    else
        return (ERR_READ);

    return (0);
}


/* Write current record to current file (from global structure, slot)
*/
int dbe_write(DB_TASK *task)
{
    int         error,
                stat,
                slot_len;
    char        *p = NULL;
    short       fno;
    DB_ULONG    recNum;

    if ((error = dbe_chkdba(task->curr_rec, task)) < 0)
        return (ERR_WRIT);

    d_decode_dba(task->curr_rec, &fno, &recNum);
    slot_len = task->file_table[fno].ft_slsize;

    if (dio_read(task->curr_rec, &p, PGHOLD, task) != S_OKAY)
        return (ERR_READ);

    if (p)
        memcpy(p, slot.buffer, slot_len);
    else
        return (ERR_READ);

    if ((stat = dio_write(task->curr_rec, PGFREE, task)) == S_OKAY)
    {
        if ((stat = dio_flush(task)) == S_OKAY)
            stat = dio_clear(ALL_DBS, task);
    }
    if (stat != S_OKAY)
        return (ERR_WRIT);

    return (0);
}


/* Close current file, and make current file NULL
*/
int dbe_close(DB_TASK *task)
{
    if (file_no != -1)
        dio_close(file_no, task);
    file_no = -1;
    return (0);
}


/* Check that dba is a valid database address in current file
*/
int dbe_chkdba(DB_ADDR dba, DB_TASK *task)
{
    short    fno;
    DB_ULONG rno;

    d_decode_dba(dba, &fno, &rno);

    if (fno != file_no)
        return (ERR_CFIL);
    if ((fno < 0) || (fno >= task->size_ft) || (rno <= 0))
        return (BAD_DBA);
    if ((F_ADDR) rno >= task->pgzero[fno].pz_next)
    {
        if (((rno - task->pgzero[fno].pz_next) / task->file_table[fno].ft_slots) >= 2L)
            return (WARN_DBA);
    }

    return (0);
}


/* Return dchain value from current file
*/
int read_dchain(F_ADDR *ptr, DB_TASK *task)
{
    if (file_no == -1)
        return (ERR_CFIL);
    *ptr = task->pgzero[file_no].pz_dchain;
    return (0);
}


/* Update dchain value in current file
*/
int write_dchain(F_ADDR value, DB_TASK *task)
{
    if (file_no == -1)
        return (ERR_CFIL);
    task->pgzero[file_no].pz_dchain = value;
    task->pgzero[file_no].pz_modified = TRUE;
    if (dio_flush(task) != S_OKAY)
        return (ERR_WRIT);
    return (0);
}


/* Return nextslot value from current file
*/
int read_nextslot(DB_ADDR *ptr, DB_TASK *task)
{
    if (file_no == -1)
        return (ERR_CFIL);
    *ptr = task->pgzero[file_no].pz_next;
    return (0);
}


/* Update nextslot value in current file
*/
int write_nextslot(DB_ADDR value, DB_TASK *task)
{
    if (file_no == -1)
        return (ERR_CFIL);
    task->pgzero[file_no].pz_next = value;
    task->pgzero[file_no].pz_modified = TRUE;
    if (dio_flush(task) != S_OKAY)
        return (ERR_WRIT);
    return (0);
}


/* Read page from current file (edit hex)
*/
int dbe_xread(F_ADDR addr, DBE_PAGE *page, DB_TASK *task)
{
    int stat = S_OKAY;
    F_ADDR filesize, pagesize, pg_no;
    PAGE_ENTRY *pg_ptr = NULL;

    if (file_no == -1)
        return (ERR_CFIL);
    if (dio_open(file_no, task) != S_OKAY)
        return (ERR_READ);
    filesize = psp_fileLength(task->file_table[file_no].ft_desc);
    if (addr >= filesize)
        return (ERR_FPOS);

    pagesize = task->file_table[file_no].ft_pgsize;
    pg_no = addr / pagesize;

    /* if dio_getpg fails, whatever was in page before may be invalid */
    memset(page, 0, sizeof(DBE_PAGE));

    if (pg_no == 0)
    {
        /* can't use dio_getpg to read page zero, so read into pg0 */
        pg_ptr = &pg0;

        /* Either page zero has already been accessed since hex edit mode
           was entered, and it's already in pg0.buff (therefore, do nothing),
           or else no buffer has been allocated yet (so allocate buffer and
           read page zero into it).
        */
        if (!(pg_ptr->buff_size))
        {
            if (!(pg_ptr->buff = malloc(pagesize)))
                return (ERR_MEM);
            pg_ptr->buff_size = (short) pagesize;
            stat = dio_readfile(file_no, 0, pg_ptr->buff, pagesize, 0, NULL, task);
        }
    }
    else
    {
        /* get page from cache or database */
        stat = dio_getpg(file_no, pg_no, task->db_tag, 
                           &task->last_datapage, &pg_ptr, task);
    }
    if (stat != S_OKAY)
        return (errno == EOF ? ERR_EOF : ERR_READ);

    page->start  = pg_no * pagesize;
    page->end    = page->start + pagesize - 1;
    page->pg_ptr = pg_ptr;

    return (0);
}


/* Write page zero to current file (edit hex), if modified
*/
int dbe_xwrite(DB_TASK *task)
{
    int error = 0;
    F_ADDR size;

    if (file_no == -1)
        return (ERR_CFIL);
    if (dio_open(file_no, task) != S_OKAY)
        return (ERR_READ);

    size = task->file_table[file_no].ft_pgsize;

    /* was page zero of file ever read, in hex mode? */
    if (pg0.buff_size)
    {
        /* was it modified? */
        if (pg0.modified)
        {
            /* write it back to database */
            if (dio_writefile(file_no, 0, pg0.buff, size, 0, task) != S_OKAY)
                error = ERR_WRIT;
        }
        /* free the page buffer */
        free(pg0.buff);
    }

    /* reset for next time edit hex is invoked */
    memset(&pg0, 0, sizeof(PAGE_ENTRY));

    return (error);
}


