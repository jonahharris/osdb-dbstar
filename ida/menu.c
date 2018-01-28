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
    MDLP - Menu Control Function
--------------------------------------------------------------------------*/
#include "keyboard.h"
#include "db.star.h"
#include "mdl.h"
#include "ida.h"

/************************** EXTERNAL FUNCTIONS ****************************/

/************************** EXTERNAL VARIABLES ****************************/
/* declared in ida_t.h */
extern MENU menu_table[];              /* menu definition table */
extern COMMAND cmd_table[];            /* command definition table */
extern int root_menu;                  /* root menu number */

/*************************** LOCAL  VARIABLES *****************************/
#define NEXT 'n'
#define PREV 'p'

/*************************** LOCAL  FUNCTIONS *****************************/
static void bad_cmd(void);

/* ========================================================================
    Display menu
*/
static void show_menu(
    int mno)                            /* menu number */
{
    register int i, j;

    tprintf("@m0000@S%s@s@e\n@e", menu_table[mno].title);
    i = j = menu_table[mno].first_cmd;
    do
    {
        tprintf("%s  ", cmd_table[i].word);
        i = cmd_table[i].next;
    } while (i != j);
    tprintf("\r@R%s@r\n%s@e", cmd_table[j].word, cmd_table[j].desc);
}

/* ========================================================================
    Move current command selection
*/
static void ida_select(
    int *cmd,                           /* current cmd selection */
    char norp)                          /* norp = 'n' => move right,
                                         * norp = 'p' => move left */
{
    int rn, cn;

    rn = 1;
    cn = cmd_table[*cmd].colpos;
    tprintf("@M@r%s", &rn, &cn, cmd_table[*cmd].word);
    if (norp == NEXT)
        *cmd = cmd_table[*cmd].next;
    else if (norp == PREV)
        *cmd = cmd_table[*cmd].prev;
    cn = cmd_table[*cmd].colpos;
    tprintf("@M@R%s@r\n%s@e", &rn, &cn, cmd_table[*cmd].word,
            cmd_table[*cmd].desc);
}

/* ========================================================================
    Find selected command entry
*/
static int find_cmd(
    char ch,                            /* typed command selection */
    int mno,                            /* current menu being processed */
    int *cmd)                           /* current command selection */
{
    register int i, j;
    register char cw;
    int rn, cn;

    if (ch == '\n')
        return (1);
    i = j = menu_table[mno].first_cmd;
    if (isupper(ch))
        ch = (char) tolower(ch);
    do
    {
        cw = cmd_table[i].word[0];
        if (isupper(cw))
            cw = (char) tolower(cw);
        if (ch == cw)
        {
            rn = 1;
            cn = cmd_table[*cmd].colpos;
            tprintf("@M%s", &rn, &cn, cmd_table[*cmd].word);
            *cmd = i;
            cn = cmd_table[*cmd].colpos;
            tprintf("@M@R%s@r\n%s@e", &rn, &cn, cmd_table[*cmd].word,
                      cmd_table[*cmd].desc);
            return (1);
        }
        i = cmd_table[i].next;
    } while (i != j);
    return (0);
}

/* ========================================================================
    Invalid user command selection
*/
static void bad_cmd()
{
    putchar('\007');
}

/* ========================================================================
    Check for selection movement command
*/
static int movemnt(char ch)
{
    switch (ch)
    {
        case K_FTAB:
        case K_RIGHT:
        case K_LEFT:
        case K_CANCEL:
        case K_ESC:
        case K_BREAK:
        case ' ':
        case '\b':
            return (1);
        default:
            return (0);
    }
}

/* ========================================================================
    db.* Menu Processor
*/
int menu(
    int mno)                            /* Menu number to be processed */
{
    int cmd;                            /* current command selection */
    char ch;                            /* typed character */

    if (root_menu < 0)                  /* top level menu */
    {
        root_menu = mno;
    }

    for (;;)
    {
        show_menu(mno);
        cmd = menu_table[mno].first_cmd;
        while (movemnt(ch = tgetch()))
        {
            switch (ch)
            {
                case K_RIGHT:
                case K_FTAB:
                case ' ':
                    ida_select(&cmd, NEXT);
                    break;
                case K_LEFT:
                case '\b':
                    ida_select(&cmd, PREV);
                    break;
                case K_CANCEL:
                case K_ESC:
                    if (mno != root_menu)
                        return (4);
                case K_BREAK:
                    if (mno != root_menu)
                        return (2);
            }
        }
        if (find_cmd(ch, mno, &cmd))
        {
            if (cmd_table[cmd].fcn)
                (*(cmd_table[cmd].fcn)) ();
            if (cmd_table[cmd].action >= 0)
                switch (menu(cmd_table[cmd].action))
                {
                    case 2:
                        if (mno != root_menu)
                            return (2);
                        break;
                    case 3:
                        if (mno != root_menu)
                            return (3);
                    case 4:
                        /* user pressed <esc> key */
                        continue;
                }
        }
        else
            bad_cmd();
        if (cmd_table[cmd].finish == 3)
            break;
        else if (cmd_table[cmd].finish != 1)
        {
            if (mno == root_menu)
            {
                root_menu = -1;
            }
            return (cmd_table[cmd].finish);
        }
    }
    return 0;
}


