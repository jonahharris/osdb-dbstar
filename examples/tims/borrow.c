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

/* Loan book
*/
int loan_book()
{
    char id[SIZEOF_ID_CODE];
    char date[20];
    struct borrower brec;

    printf("id_code of book to be loaned: ");
    if (getstring(id,SIZEOF_ID_CODE) && id[0] != '\0')
    {
        if (d_keyfind(ID_CODE, id, Currtask, CURR_DB) == S_NOTFOUND)
            printf("id_code %s not on file\n", id);
        else
        {
            d_setor(LOANED_BOOKS, Currtask, CURR_DB);
            printf("name of borrower: ");
            getstring(brec.friend,SIZEOF_FRIEND);
            printf("date borrowed   : ");
            getstring(date,sizeof(date));
            sscanf(date, "%ld", &brec.date_borrowed);
            brec.date_returned = 0L;
            d_fillnew(BORROWER, &brec, Currtask, CURR_DB);
            d_connect(LOANED_BOOKS, Currtask, CURR_DB);
            d_connect(LOAN_HISTORY, Currtask, CURR_DB);
        }
    }
    return (0);
}


/* Returned borrowed book
*/
int return_book()
{
    char id[SIZEOF_ID_CODE];
    char friend[SIZEOF_FRIEND];
    char date[20];
    long bdate;
    struct borrower brec;

    printf("id_code of returned book: ");
    if (getstring(id,SIZEOF_ID_CODE) && id[0] != '\0')
    {
        if (d_keyfind(ID_CODE, id, Currtask, CURR_DB) == S_NOTFOUND)
            printf("id_code %s not on file\n", id);
        else
        {
            d_setor(LOANED_BOOKS, Currtask, CURR_DB);
            printf("name of borrower: ");
            getstring(friend,SIZEOF_FRIEND);
            while (d_findnm(LOANED_BOOKS, Currtask, CURR_DB) == S_OKAY)
            {
                d_recread(&brec, Currtask, CURR_DB);
                if (strcmp(brec.friend, friend) == 0)
                {
                    if (brec.date_returned == 0L)
                    {
                        printf("book borrowed on: %ld\n", brec.date_borrowed);
                        printf("date returned   : ");
                        getstring(date,sizeof(date));
                        sscanf(date, "%ld", &bdate);
                        d_crwrite(DATE_RETURNED, &bdate, Currtask, CURR_DB);
                        return (0);
                    }
                }
            }
            printf("borrower not on file\n");
        }
    }
    return (0);
}


/* List loaned out books
*/
int list_loaners()
{
    int status;
    struct borrower brec;
    char title[80];

    for (status = d_findfm(LOAN_HISTORY, Currtask, CURR_DB); status == S_OKAY;
          status = d_findnm(LOAN_HISTORY, Currtask, CURR_DB))
    {
        d_recread(&brec, Currtask, CURR_DB);
        if (brec.date_returned == 0L)
        {
            d_findco(LOANED_BOOKS, Currtask, CURR_DB);
            d_crread(INFO_TITLE, title, Currtask, CURR_DB);
            printf("title: %s\n", title);
            printf("borrowed by %s on %ld\n\n", brec.friend, brec.date_borrowed);
        }
    }
    return (0);
}


