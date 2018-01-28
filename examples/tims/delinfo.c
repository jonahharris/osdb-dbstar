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

/* Delete technical information records from TIMS database
*/
int del_info()
{
    int status;
    struct info irec;
    long count;
    char id[SIZEOF_ID_CODE], name[SIZEOF_NAME];

    /* find info to be deleted */
    printf("id_code: ");
    getstring(id,SIZEOF_ID_CODE);
    if (d_keyfind(ID_CODE, id, Currtask, CURR_DB) == S_NOTFOUND)
    {
        printf("id_code %s not on file\n", id);
        return (0);
    }
    d_recread(&irec, Currtask, CURR_DB);

    /* get author name */
    d_findco(HAS_PUBLISHED, Currtask, CURR_DB);
    d_crread(NAME, name, Currtask, CURR_DB);

    /* confirm delete request */
    printf("author: %s\n", name);
    printf("title : %s\n", irec.info_title);
    printf("delete (y/n)? ");
    getstring(id,SIZEOF_ID_CODE);
    if (id[0] != 'Y' && id[0] != 'y')
        return (0);

    /* disconnect any listed articles */
    d_setom(ARTICLE_LIST, HAS_PUBLISHED, Currtask, CURR_DB);
    for (status = d_findfm(ARTICLE_LIST, Currtask, CURR_DB); status == S_OKAY;
         status = d_discon(ARTICLE_LIST, Currtask, CURR_DB))
        ;

    /* disconnect and delete borrowers */
    d_setom(LOANED_BOOKS, HAS_PUBLISHED, Currtask, CURR_DB);
    while (d_findfm(LOANED_BOOKS, Currtask, CURR_DB) == S_OKAY)
    {
        d_discon(LOANED_BOOKS, Currtask, CURR_DB);
        d_setmr(LOAN_HISTORY, Currtask, CURR_DB);
        d_discon(LOAN_HISTORY, Currtask, CURR_DB);
        d_delete(Currtask, CURR_DB);
    }

    /* disconnect and delete abstract */
    d_setom(ABSTRACT, HAS_PUBLISHED, Currtask, CURR_DB);
    while (d_findfm(ABSTRACT, Currtask, CURR_DB) == S_OKAY)
    {
        d_discon(ABSTRACT, Currtask, CURR_DB);
        d_delete(Currtask, CURR_DB);
    }

    /* disconnect and delete intersect and (possibly) key word */
    d_setom(INFO_TO_KEY, HAS_PUBLISHED, Currtask, CURR_DB);
    while (d_findfm(INFO_TO_KEY, Currtask, CURR_DB) == S_OKAY)
    {
        d_discon(INFO_TO_KEY, Currtask, CURR_DB);
        d_setmr(KEY_TO_INFO, Currtask, CURR_DB);
        d_discon(KEY_TO_INFO, Currtask, CURR_DB);
        d_delete(Currtask, CURR_DB);
        d_members(KEY_TO_INFO, &count, Currtask, CURR_DB);
        if (count == 0L)
        {
            /* delete key word */
            d_setro(KEY_TO_INFO, Currtask, CURR_DB);
            d_delete(Currtask, CURR_DB);
        }
    }

    /* disconnect info record from author and delete */
    d_discon(HAS_PUBLISHED, Currtask, CURR_DB);
    d_delete(Currtask, CURR_DB);

    /* delete author too, if he has no other pubs */
    d_members(HAS_PUBLISHED, &count, Currtask, CURR_DB);
    if (count == 0L)
    {
        d_setmo(AUTHOR_LIST, HAS_PUBLISHED, Currtask, CURR_DB);
        d_discon(AUTHOR_LIST, Currtask, CURR_DB);
        d_delete(Currtask, CURR_DB);
    }
    return (0);
}


