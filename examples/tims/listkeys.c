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

/* List key words
*/
int list_keys()
{
    int status;
    char key[SIZEOF_KWORD];

    printf("start key: ");
    getstring(key,SIZEOF_KWORD);
    if (key[0] != '\0')
    {
        if ((status = d_keyfind(KWORD, key, Currtask, CURR_DB)) == S_NOTFOUND)
            status = d_keynext(KWORD, Currtask, CURR_DB);
    }
    else
        status = d_keyfrst(KWORD, Currtask, CURR_DB);

    /* scan thru keys */
    while (status == S_OKAY)
    {
        d_keyread(key, Currtask);
        printf("   %s\n", key);
        status = d_keynext(KWORD, Currtask, CURR_DB);
    }
    printf("--- press <enter> to continue");
    getstring(key,SIZEOF_KWORD);
    return (0);
}

