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
    IDA - General Purpose List Selection Control Function
--------------------------------------------------------------------------*/
#include "db.star.h"
#include "ida.h"
#include "keyboard.h"

/**************************** LOCAL VARIABLES *****************************/
#define UP -1
#define DOWN -2
#define RETN -3
#define ESC -4

static int first_entry;
static int zero = 0;

static int getkey(void);
static void disp_fld(char **, int, int, int);

/* ========================================================================
    Display field
*/
static void disp_fld(char **list, int fno, int row, int reverse)
{
    if (reverse)
    {
        tprintf("@M@e@R%4d.", &row, &zero, fno + first_entry);
        tprintf(" %s@r@M", list[fno], &row, &zero);
    }
    else
    {
        tprintf("@M@e%4d.", &row, &zero, fno + first_entry);
        tprintf(" %s@M", list[fno], &row, &zero);
    }
}

/* ========================================================================
    Select from list
*/
int list_selection(
    int topln,                          /* top line of list display (0-20) */
    int entries,                        /* number of entries in list */
    char **list,                        /* list of selections (strings) */
    int entry_no,                       /* starting entry number */
    int last_sel,                       /* last selected field */
    int disp_list)                      /* list display flag */
{
    register int key, top, bot, fno, i;

    first_entry = entry_no;
    top = 0;
    bot = 20 - topln;
    if (last_sel > 0 && last_sel < entries)
        fno = last_sel;
    else
        fno = 0;

    if (fno < top || fno > bot)
    {
        top = fno;
        bot = top + 20 - topln;
    }
    if (disp_list)
    {
        for (i = top; i <= bot && i < entries; ++i)
            disp_fld(list, i, i - top + topln, 0);
    }
    disp_fld(list, fno, fno - top + topln, 1);

    while ((key = getkey()) == UP || key == DOWN || key == ESC || key >= 0)
    {
        disp_fld(list, fno, fno - top + topln, 0);
        if (key >= 0 && key != fno && key < entries)
        {
            fno = key;
            if (fno < top || fno > bot)
            {
                top = fno;
                bot = top + 20 - topln;
                tprintf("@M@E", &topln, &zero);
                for (i = top; i <= bot && i < entries; ++i)
                    disp_fld(list, i, i - top + topln, 0);
            }
        }
        else if (key == UP)
        {                                /* move up a field */
            if (fno && --fno < top)
            {                             /* scroll display down */
                --top;
                --bot;
                tprintf("@M", &topln, &zero);
                for (i = top; i <= bot && i < entries; ++i)
                    disp_fld(list, i, i - top + topln, 0);
                tprintf("@m2100@E");
            }
        }
        else if (key == DOWN)
        {                                /* move down a field */
            if (fno < entries - 1 && ++fno > bot)
            {                             /* scroll display up */
                ++top;
                ++bot;
                tprintf("@M", &topln, &zero);
                for (i = top; i <= bot && i < entries; ++i)
                    disp_fld(list, i, i - top + topln, 0);
                tprintf("@m2100@E");
            }
        }
        else if (key == ESC)
        {
            return (-1);
        }
        disp_fld(list, fno, fno - top + topln, 1);
    }
    return (fno);
}

/* ========================================================================
    Get key
*/
static int getkey()
{
    register char c;

    c = tgetch();
    if (c >= '0' && c <= '9')
    {
        register int i;

        i = 0;
        while (c != '\n' && c != '\r')
        {
            printf("%c", c);
            i = 10 * i + c - '0';
            c = tgetch();
        }
        return (i - first_entry);
    }
    switch (c)
    {
        case K_UP:
        case 'u':
            return (UP);
        case K_DOWN:
        case 'd':
            return (DOWN);
                case '\n':
        case '\r':
            return (RETN);
        default:
            return (ESC);
    }
}


