/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, TIMS example application                          *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/*
 *    See section 4.5.3 of the User's Guide for database design and
 *    application requirements.
*/

#include <stdio.h>
#include <errno.h>

#include "db.star.h"

int loan_book(void);
int return_book(void);
int list_loaners(void);
int del_info(void);
int ent_info(void);
int by_key(void);
int by_author(void);
int list_authors(void);
int list_keys(void);
char *getstring(char *, int);

DB_TASK *Currtask;


/* Technical Information Management System
*/
int main(void)
{
    char cmd[20];                    /* command entry string */

    if (d_opentask(&Currtask) != S_OKAY
     || d_open("tims", "o", Currtask) != S_OKAY)
    {
        printf("errno: %d\n", errno);
        perror("Failed to open database");
        return (0);
    }

    for (;;)
    {
        /* display command menu */
        printf("\nTIMS Commands:\n");
        printf("   1 - Display list of key words\n");
        printf("   2 - Display list of authors\n");
        printf("   3 - List publications by key word\n");
        printf("   4 - List publications by author\n");
        printf("   5 - Enter technical information\n");
        printf("   6 - Delete technical information\n");
        printf("   7 - Loan book\n");
        printf("   8 - Return loaned book\n");
        printf("   9 - List borrowed books\n");
        printf("   q - Quit\n");
        printf("enter command: ");
        getstring(cmd, sizeof(cmd));
        switch (cmd[0])
        {
            case 'q':
            case 'Q':   d_close(Currtask);
                        d_closetask(Currtask);
                        return (0);

            case '1':   list_keys();      break;
            case '2':   list_authors();   break;
            case '3':   by_key();         break;
            case '4':   by_author();      break;
            case '5':   ent_info();       break;
            case '6':   del_info();       break;
            case '7':   loan_book();      break;
            case '8':   return_book();    break;
            case '9':   list_loaners();   break;
            default:
                printf("*** incorrect command -- re-enter\n");
                break;
        }
    }
    return  (0);
}


/* safe gets() */
char *getstring(char *s, int size)
{
    int len;

    if (fgets(s, size, stdin) == NULL)
        return NULL;
    
    len = strlen(s);
    if (s[len-1] == '\n')
        s[len-1] = '\0';
    
    return s;
}
