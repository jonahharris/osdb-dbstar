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

/* Menu Definition Language Table type definitions */

typedef struct
{
    char *title;                        /* menu title */
    int first_cmd;                      /* index of first command in menu */
}  MENU;

typedef struct
{
    int next;                           /* index of next command entry */
    int prev;                           /* index of previous command entry */
    int colpos;                         /* column position for this command
                                         * word */
    char *word;                         /* command word */
    char *desc;                         /* command description */
    int action;                         /* action to be taken: -1 =>
                                         * nothing 0..no_of_menus-1 =>
                                         * process new menu */
    void (*fcn) (void);                 /* pointer to function to be called */
    int finish;                         /* where to go when done: 0 =
                                         * return to parent menu, 1 =
                                         * repeat this menu 2 = break to
                                         * root menu, 3 = exit menu
                                         * processor */
}  COMMAND;


