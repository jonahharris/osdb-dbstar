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

#include <stdio.h>
#include "db.star.h"
#include "tims.h"

char *getstring(char *, int);
extern DB_TASK *Currtask;

/* List authors
*/
int list_authors()
{
    int status;
    char start[SIZEOF_NAME], name[SIZEOF_NAME];

    printf("start name: ");
    getstring(start,SIZEOF_NAME);
    if (start[0] != '\0')
    {
        /* scan for first name */
        for (status = d_findfm(AUTHOR_LIST, Currtask, CURR_DB); status == S_OKAY;
             status = d_findnm(AUTHOR_LIST, Currtask, CURR_DB))
        {
            d_crread(NAME, name, Currtask, CURR_DB);
            if (strcmp(start, name) <= 0)
                break;
        }
    }
    else
        status = d_findfm(AUTHOR_LIST, Currtask, CURR_DB);

    while (status == S_OKAY)
    {
        d_crread(NAME, name, Currtask, CURR_DB);
        printf("   %s\n", name);
        status = d_findnm(AUTHOR_LIST, Currtask, CURR_DB);
    }
    printf("--- press <enter> to continue");
    getstring(name,SIZEOF_NAME);
    return (0);
}

