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

static struct info irec;
static struct author arec;

static int enter_key_words(void);
static int enter_abstract(void);
static int get_info(void);
char *getstring(char *, int);

/* Enter technical information records into TIMS database
*/
int ent_info()
{
    int status;
    char s[SIZEOF_NAME];  /* generic string variable */

    /* enter tech info into TIMS database */
    while (get_info() != EOF)
    {
        /* see if author exists */
        for (status = d_findfm(AUTHOR_LIST, Currtask, CURR_DB); status == S_OKAY;
             status = d_findnm(AUTHOR_LIST, Currtask, CURR_DB))
        {
            d_crread(NAME, s, Currtask, CURR_DB);
            if (strcmp(arec.name, s) == 0)
                break;                        /* author record on file */
        }
        if (status == S_EOS)
        {
            /* author not on file -- create record and connect to author list */
            d_fillnew(AUTHOR, &arec, Currtask, CURR_DB);
            d_connect(AUTHOR_LIST, Currtask, CURR_DB);
        }
        /* make author current owner of has_published set */
        d_setor(HAS_PUBLISHED, Currtask, CURR_DB);

        /* create new tech. info record */
        if (d_fillnew(INFO, &irec, Currtask, CURR_DB) == S_DUPLICATE)
            printf("duplicate id_code: %s\n", irec.id_code);
        else
        {
            /* connect to author record */
            d_connect(HAS_PUBLISHED, Currtask, CURR_DB);

            /* set current owner for key words and abstract */
            d_setor(INFO_TO_KEY, Currtask, CURR_DB);
            d_setor(ABSTRACT, Currtask, CURR_DB);

            enter_key_words();

            enter_abstract();
        }
    }
    return (0);
}



/* Enter any key words
*/
static int enter_key_words()
{
    char s[SIZEOF_KWORD];

    for (;;)
    {
        printf("key word: ");
        if (getstring(s,SIZEOF_KWORD) == NULL || s[0] == '\0')
            break;
        /* see if key word record exists */
        if (d_keyfind(KWORD, s, Currtask, CURR_DB) == S_NOTFOUND)
        {
            /* create new key word record */
            d_fillnew(KEY_WORD, s, Currtask, CURR_DB);
        }
        d_setor(KEY_TO_INFO, Currtask, CURR_DB);

        /* create intersection record */
        d_fillnew(INTERSECT, &irec.info_type, Currtask, CURR_DB);
        d_connect(KEY_TO_INFO, Currtask, CURR_DB);
        d_connect(INFO_TO_KEY, Currtask, CURR_DB);
    }
    return (0);
}


/* Enter abstract description
*/
static int enter_abstract()
{
    char text_line[SIZEOF_LINE];

    for (;;)
    {
        printf("abstract: ");
        if (getstring(text_line,SIZEOF_LINE) == NULL || text_line[0] == '\0')
            return (0);
        d_fillnew(INFOTEXT, text_line, Currtask, CURR_DB);
        d_connect(ABSTRACT, Currtask, CURR_DB);
    }
    return (0);
}



/* Fill irec, arec with info data from user
*/
static int get_info()
{
    char txt[40];

    printf("author   : ");
    if (getstring(arec.name,SIZEOF_NAME) == NULL || arec.name[0] == '\0')
        return (EOF);
    else
    {
        for (;;)
        {
            printf("id_code  : ");
            getstring(irec.id_code,SIZEOF_ID_CODE);
            printf("title    : ");
            getstring(irec.info_title,SIZEOF_INFO_TITLE);
            printf("publisher: ");
            getstring(irec.publisher,SIZEOF_PUBLISHER);
            printf("pub. date: ");
            getstring(irec.pub_date,SIZEOF_PUB_DATE);
            for (;;)
            {
                printf("info type: ");
                getstring(txt,sizeof(txt));
                sscanf(txt, "%hd", (short *) &irec.info_type);
                if (irec.info_type >= 0 && irec.info_type <= 2)
                    break;
                printf("invalid info type - correct types are:\n");
                printf("0 - book, 1 = magazine, 2 = article\n");
            }
            printf("enter data (y/n)? ");
            getstring(txt,sizeof(txt));
            if (txt[0] == 'y' || txt[0] == 'Y')
                return (0);
        }
    }
}

