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

/*----------------------------------------------------------------
    Technical Information Management System (TIMS) Database
----------------------------------------------------------------*/
database tims {
    data file "tims.d01" contains system, key_word, intersect;
    data file "tims.d02" contains author, borrower, info, infotext;
    key  file "tims.k01" contains id_code;
    key  file "tims.k02" contains friend, kword;

    record author {
        char name[32];               /* author's name: "last, first" */
    }                                /* or editor's name */
    record info {
        unique key char id_code[16]; /* dewey dec. or own coding tech. */
        char info_title[80];         /* title of book, article, mag. */
        char publisher[32];          /* name of publisher - prob. coded */
        char pub_date[12];           /* date of publication  
                                        (e.g. most recent copyright) */
        short info_type;             /* 0 = book, 1 = magazine, 2 = article */
    }
    record borrower {
        key char friend[32];         /* name of borrower */
        long date_borrowed;          /* dates are stored initially as */
        long date_returned;          /* numeric YYMMDD (e.g. 870226) */
    }
    record infotext {
        char line[80];               /* line of abstract text */
    }
    record key_word {
        unique key char kword[32];   /* subject key words or classification */
    }
    record intersect {
        short int_type;              /* copy of info_type to save I/O */
    }                                /* when looking only for, say, books */
    set author_list {
        order ascending;
        owner system;
        member author by name;
    }
    set has_published {
        order ascending;
        owner author;
        member info by info_title;
    }
    set article_list {
        order last;
        owner info;
        member info;
    }
    set loaned_books {
        order last;
        owner info;
        member borrower;
    }
    set abstract {
        order last;
        owner info;
        member infotext;
    }
    set key_to_info {
        order last;
        owner key_word;
        member intersect;
    }
    set info_to_key {
        order last;
        owner info;
        member intersect;
    }
    set loan_history {
        order last;
        owner system;
        member borrower;
    }
}

