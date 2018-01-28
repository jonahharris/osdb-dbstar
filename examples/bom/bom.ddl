/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, BOM example database, network model               *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

database bom {
    data file "bom.d01" contains item;
    data file "bom.d02" contains bill;
    key file  "bom.k01" contains id_code;

    record item {
        unique key char id_code[16];
        char description[58];
        double cost;
    }
    record bill {
        double quantity;
        long level;
    }

    set bom {
        order last;
        owner item;
        member bill;
    }
    set where_used {
        order last;
        owner item;
        member bill;
    }
}
