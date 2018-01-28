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
    IDA - Text Input Functions
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "internal.h"
#include "ida.h"
#include "keyboard.h"

/* ========================================================================
    Read text line
*/
int rdtext(char *txt)
{
    register int i;
    char c;

    for (i = 0; (c = tgetch()) != '\n';)
    {
        if (c >= ' ')
        {
            tprintf("%c", &c);
            txt[i++] = c;
            txt[i] = '\0';
        }
        else if ((c == '\b') && (i > 0))
        {
            tprintf("\b \b");
            txt[--i] = '\0';
        }
        else if ((c == K_ESC) || (c == K_CANCEL) || (c == K_BREAK))
        {
            return (-1);
        }
        else if (task->ctbl_activ && task->country_tbl[(unsigned char) c].out_chr)
        {
            tprintf("%c", &c);
            txt[i++] = c;
            txt[i] = '\0';
        }

    }
    return (i);
}

/* ========================================================================
    Read database address
*/
int rd_dba(DB_ADDR *dba)
{
    char *rec, txt[21];
    short rt;
    register char *tp;
    int i;
    FILE_NO  fno;
    F_ADDR   sno;
    PGZERO   pgz;

    tprintf("@m0200@eenter database address: ");
    if (rdtext(txt) <= 0)
        return (-1);

    for (tp = txt; *tp && (! isdigit(*tp)); ++tp)
        ;
    if (sscanf(tp, "%d", &i) != 1)
        return (0);
    fno = (FILE_NO) i;
    while (*tp && isdigit(*tp))
        ++tp;
    while (*tp && (! isdigit(*tp)))
        ++tp;
    if (sscanf(tp, "%ld", &sno) != 1)
        return (0);

    if ((fno < 0) || (fno >= task->size_ft))
        return (0);

    d_internals(task, TOPIC_PGZERO_TABLE, 0, fno, &pgz, sizeof(pgz));
    if ((sno < (F_ADDR) 1) || (sno >= pgz.pz_next))
        return (0);

    d_encode_dba(fno, sno, dba);

    if (dio_read(*dba, &rec, NOPGHOLD , task) == S_OKAY)
    {
        memcpy(&rt, rec, sizeof(short));
        rt &= ~RLBMASK;
        if (dio_release(*dba, NOPGFREE , task) != S_OKAY)
            return (task->db_status);
        if (rt < 0 || rt > task->size_rt)
            return (0);
    }
    if (dio_release(*dba, NOPGFREE , task) != S_OKAY)
        return (task->db_status);
    return (1);
}

/* ========================================================================
    Print user error message
*/
void usererr(char *msg)
{
    tprintf("@m2300@e@R%s -- press any key to continue@r", msg);
    tgetch();
    tprintf("@m2300@e");
}


