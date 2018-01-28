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

extern DB_TASK *Currtask;

static char cmd[10];

static int pr_keywords(void);
static int pr_abstract(void);
char *getstring(char *, int);

/* Find publications by key word
*/
int by_key()
{
    int status;
    struct info irec;                /* info record variable */
    char name[SIZEOF_NAME];          /* author's name */
    char key[SIZEOF_KWORD];          /* key word */

    /* find key word record */
    printf("key word: ");
    getstring(key,SIZEOF_KWORD);
    if (d_keyfind(KWORD, key, Currtask, CURR_DB) == S_NOTFOUND)
        printf("no records found\n");
    else
    {
        /* scan thru key_to_info set */
        d_setor(KEY_TO_INFO, Currtask, CURR_DB);
        for (status = d_findfm(KEY_TO_INFO, Currtask, CURR_DB); status == S_OKAY;
             status = d_findnm(KEY_TO_INFO, Currtask, CURR_DB))
        {
            /* find current owner (info) of current record (intersect) */
            d_findco(INFO_TO_KEY, Currtask, CURR_DB);

            /* read contents of info record */
            d_recread(&irec, Currtask, CURR_DB);

            /* find author of info record */
            d_findco(HAS_PUBLISHED, Currtask, CURR_DB);
            d_crread(NAME, name, Currtask, CURR_DB);

            /* print results */
            printf("id_code: %s\n", irec.id_code);
            printf("author : %s\n", name);
            printf("title  : %s\n", irec.info_title);
            printf("publ.  : %s, %s\n", irec.publisher, irec.pub_date);
            pr_keywords();
            pr_abstract();
            printf("--- press <enter> to continue");
            getstring(cmd,sizeof(cmd));
        }
    }
    return (0);
}


/* Find publication by author
*/
int by_author()
{
    int status;
    int searchLen;
    struct info irec;                /* info record variable */
    char search[SIZEOF_NAME];        /* author to search for */
    char name[SIZEOF_NAME];

    /* find author record */
    printf("author: ");
    getstring(search,SIZEOF_NAME);
    searchLen = strlen(search);
    for (status = d_findfm(AUTHOR_LIST, Currtask, CURR_DB); status == S_OKAY;
         status = d_findnm(AUTHOR_LIST, Currtask, CURR_DB))
    {
        d_crread(NAME, name, Currtask, CURR_DB);
        if (strncmp(search, name, searchLen) == 0)
        {
            d_setor(HAS_PUBLISHED, Currtask, CURR_DB);
            for (status = d_findfm(HAS_PUBLISHED, Currtask, CURR_DB);
                 status == S_OKAY;
                 status = d_findnm(HAS_PUBLISHED, Currtask, CURR_DB))
            {
                d_recread(&irec, Currtask, CURR_DB);

                /* read and print info record */
                printf("id_code: %s\n", irec.id_code);
                printf("author : %s\n", name);
                printf("title  : %s\n", irec.info_title);
                printf("publ.  : %s, %s\n", irec.publisher, irec.pub_date);
                pr_keywords();
                pr_abstract();
                printf("--- press <enter> to continue");
                getstring(cmd,sizeof(cmd));
            }
        }
        else if (strcmp(search, name) < 0)
        {
            printf("author record not found\n");
            return (0);
        }
    }
    return (0);
}


/* Print key words
*/
static int pr_keywords()
{
    long count;                      /* number of info_to_key members */
    char key[SIZEOF_KWORD];          /* key word or phrase */
    DB_ADDR dba;                     /* db addr of key_to_info member */

    /* the current member of the has_published set is the info record whose
     * key words are to be listed */
    d_setom(INFO_TO_KEY, HAS_PUBLISHED, Currtask, CURR_DB);

    /* fetch number of members of info_to_key */
    d_members(INFO_TO_KEY, &count, Currtask, CURR_DB);

    /* list the key words, if any */
    if (count > 0L)
    {
        /* save current member of key_to_info because it's going to change and
         * we may be currently scanning through that set */
        d_csmget(KEY_TO_INFO, &dba, Currtask, CURR_DB);

        printf("key words:\n----------\n");
        /* find each intersect member record */
        while (d_findnm(INFO_TO_KEY, Currtask, CURR_DB) == S_OKAY)
        {
            /* find, read and print corresponding key_word */
            d_findco(KEY_TO_INFO, Currtask, CURR_DB);
            d_crread(KWORD, key, Currtask, CURR_DB);
            printf("   %s\n", key);
        }
        printf("\n");

        /* reset key_to_info current member and owner */
        if (dba)
            d_csmset(KEY_TO_INFO, &dba, Currtask, CURR_DB);
    }
    return (0);
}



/* Print abstract
*/
static int pr_abstract()
{
    long count;                      /* number of abstract members */
    char txt[80];                    /* line of abstract text */

    /* the current member of has_published is the info record whose abstract
     * is to be printed */
    d_setom(ABSTRACT, HAS_PUBLISHED, Currtask, CURR_DB);

    /* fetch number of lines in abstract */
    d_members(ABSTRACT, &count, Currtask, CURR_DB);

    /* print abstract, if one exists */
    if (count > 0)
    {
        printf("abstract:\n---------\n");

        /* find, read and print each abstract text line */
        while (d_findnm(ABSTRACT, Currtask, CURR_DB) != S_EOS)
        {
            d_csmread(ABSTRACT, LINE, txt, Currtask, CURR_DB);
            printf("  %s\n", txt);
        }
    }
    printf("\n");
    return (0);
}

