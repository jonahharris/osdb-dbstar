/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, BOM example database, relational model            *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

database rbom {
    data file "rbom.d01" contains ritem;
    data file "rbom.d02" contains rbill;
    key file  "rbom.k01" contains rid_code;
    key file  "rbom.k02" contains rbom, rwhere_used;

    record ritem {
        unique key char rid_code[16];
        char rdescription[58];
        double rcost;
        long rcomponent_count;
    }
    record rbill {
        char rparent[16];
        char rcomponent[16];
        long rsequence;
        double rquantity;
        long rlevel;

        compound key rbom {
            rparent;
            rsequence;
        }
        compound key rwhere_used {
            rcomponent;
            rsequence;
        }
    }
}
