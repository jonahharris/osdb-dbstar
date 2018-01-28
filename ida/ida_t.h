/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

#include "mdl.h"

extern void db_access();
extern void initdb();
extern void closedb();
extern void showpar();
extern void oofcn();
extern void osfcn();
extern void oefcn();
extern void clear_exit();
extern void pdfcn();
extern void pffcn();
extern void pufcn();
extern void ppfcn();
extern void pmfcn();
extern void plfcn();
extern void pcfcn();
extern void clear_exit();
extern void rgfcn();
extern void refcn();
extern void rmfcn();
extern void rdfcn();
extern void rsfcn();
extern void rffcn();
extern void rkffcn();
extern void rklfcn();
extern void rnfcn();
extern void rpfcn();
extern void rrsfcn();
extern void rrffcn();
extern void rrlfcn();
extern void rrnfcn();
extern void rrpfcn();
extern void ssfcn();
extern void sofcn();
extern void sffcn();
extern void snfcn();
extern void slfcn();
extern void spfcn();
extern void scfcn();
extern void sdfcn();
extern void stfcn();
extern void cofcn();
extern void cmfcn();
extern void crfcn();
extern void cafcn();
extern void cdfcn();
extern void cdxfcn();
extern void tbfcn();
extern void tefcn();
extern void tafcn();
extern void lrfcn();
extern void lsfcn();
extern void lkfcn();
extern void lcfcn();
extern void ltfcn();
extern void mlfcn();
extern void frfcn();
extern void fsfcn();
extern void fkfcn();
extern void fafcn();
extern void fcfcn();
extern void mffcn();
extern void mlfcn();
extern void clear_exit();
extern void rsnfcn();
extern void rsffcn();
extern void rssfcn();
extern void clear_exit();
extern void ssnfcn();
extern void ssffcn();
extern void sssfcn();
extern void clear_exit();
extern void corfcn();
extern void coofcn();
extern void comfcn();
extern void cocfcn();
extern void cmrfcn();
extern void cmofcn();
extern void cmmfcn();
extern void cmcfcn();
extern void crofcn();
extern void crmfcn();
extern void crcfcn();
extern void cdnfcn();
extern void cdpfcn();
extern void cdsfcn();
extern void cdxfcn();
extern void ctrfcn();
extern void ctofcn();
extern void ctmfcn();
extern void ctsfcn();
extern void clear_exit();
extern void rrsnfcn();
extern void rrsffcn();
extern void rrssfcn();
extern void clear_exit();
extern void edit_rec();
extern void create_rec();
extern void next_rec();
extern void prev_rec();
extern void write_rec();
extern void store_key();
extern void del_key();
extern void setor();
extern void scfcn();
extern void clear_exit();

int no_of_menus = 22;
int root_menu = -1;

MENU menu_table[] = {
    {"IDA - db.* Interactive Database Access Utility", 0},
    {"Open a db.* database", 6},
    {"Database Access Commands", 10},
    {"Set db.* Operational Parameters", 18},
    {"Record Manipulation Functions", 26},
    {"Scan and view record based on key", 33},
    {"Scan and view records based on database address", 40},
    {"Set Manipulation Functions", 46},
    {"Currency Table Manipulation Functions", 56},
    {"Transaction Processing Functions", 63},
    {"Multiuser Set/Record Lock Functions", 67},
    {"Multiuser Set/Record Free Locks Functions", 74},
    {"Miscellaneous IDA Functions", 80},
    {"Scan and View Keys", 83},
    {"Scan All Members of Set", 87},
    {"Change Current Owner of Set", 91},
    {"Change Current Member of Set", 96},
    {"Change Current Record", 101},
    {"Currency Table Display", 105},
    {"Timestamp Currency Functions", 109},
    {"Scan and View Records", 114},
    {"Display/Edit Record", 118}
};

COMMAND cmd_table[] = {
    {1, 5, 0, "Open", "Open a db.* database", 1, NULL, 1},
    {2, 0, 6, "Access", "Access opened db.* database", -1, db_access, 1},
    {3, 1, 14, "Initialize", "Initialize a db.* database", -1, initdb, 1},
    {4, 2, 26, "Close", "Close the opened db.* database", -1, closedb, 1},
    {5, 3, 33, "Parameters", "Set db.* operational parameters", 3, showpar, 1},
    {0, 4, 45, "Quit", "Quit Interactive Database Access utility", -1, NULL, 3},
    {7, 9, 0, "One_user", "Open for single user access", -1, oofcn, 0},
    {8, 6, 10, "Shared", "Open for shared access", -1, osfcn, 0},
    {9, 7, 18, "Exclusive", "Open for exclusive access", -1, oefcn, 0},
    {6, 8, 29, "X_exit", "Exit to main menu", -1, NULL, 0},
    {11, 17, 0, "Record", "Record manipulation functions", 4, NULL, 1},
    {12, 10, 8, "Set", "Set navigation/manipulation functions", 7, NULL, 1},
    {13, 11, 13, "Currency", "Currency table manipulation functions", 8, NULL, 1},
    {14, 12, 23, "Transaction", "Transaction functions", 9, NULL, 1},
    {15, 13, 36, "Lock", "Database set/record file lock functions", 10, NULL, 1},
    {16, 14, 42, "Free", "Database set/record free file lock functions", 11, NULL, 1},
    {17, 15, 48, "Miscellaneous", "Miscellaneous IDA functions", 12, NULL, 1},
    {10, 16, 63, "X_exit", "Exit to main menu", -1, clear_exit, 0},
    {19, 25, 0, "Dictionary_path", "Set DBDPATH environment variable", -1, pdfcn, 1},
    {20, 18, 17, "Files_path", "Set DBFPATH environment variable", -1, pffcn, 1},
    {21, 19, 29, "Userid", "Set db.* User Identifier", -1, pufcn, 1},
    {22, 20, 37, "Pages", "Set number of virtual memory pages", -1, ppfcn, 1},
    {23, 21, 44, "Max_files", "Set maximum number of open db.* files", -1, pmfcn, 1},
    {24, 22, 55, "Logging", "Toggle transaction logging option", -1, plfcn, 1},
    {25, 23, 64, "Chain", "Toggle reuse of delete chain slots option", -1, pcfcn, 1},
    {18, 24, 71, "X_exit", "Exit parameters menu", -1, clear_exit, 0},
    {27, 32, 0, "Keyscan", "Scan and view record based on key", 5, NULL, 1},
    {28, 26, 9, "Get", "Get record by database address", -1, rgfcn, 1},
    {29, 27, 14, "Enter", "Enter new records", -1, refcn, 1},
    {30, 28, 21, "Modify", "Modify current record", -1, rmfcn, 1},
    {31, 29, 29, "Delete", "Delete current record", -1, rdfcn, 1},
    {32, 30, 37, "Recscan", "Scan and view records based on database address", 6, NULL, 1},
    {26, 31, 46, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {34, 39, 0, "Scan", "Scan and view records based on key", -1, rsfcn, 1},
    {35, 33, 6, "Keyfind", "Find record by key", -1, rffcn, 1},
    {36, 34, 15, "First", "Find first record by key", -1, rkffcn, 1},
    {37, 35, 22, "Last", "Find last record by key", -1, rklfcn, 1},
    {38, 36, 28, "Next", "Find next record by key", -1, rnfcn, 1},
    {39, 37, 34, "Previous", "Find previous record by key", -1, rpfcn, 1},
    {33, 38, 44, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {41, 45, 0, "Scan", "Scan and view records on database address", -1, rrsfcn, 1},
    {42, 40, 6, "First", "Find first record by database address", -1, rrffcn, 1},
    {43, 41, 13, "Last", "Find last record by database address", -1, rrlfcn, 1},
    {44, 42, 19, "Next", "Find next record by database address", -1, rrnfcn, 1},
    {45, 43, 25, "Previous", "Find previous record by database address", -1, rrpfcn, 1},
    {40, 44, 35, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {47, 55, 0, "Scan", "Scan and view set", -1, ssfcn, 0},
    {48, 46, 6, "Owner", "Find current owner", -1, sofcn, 1},
    {49, 47, 13, "First", "Find first member", -1, sffcn, 1},
    {50, 48, 20, "Next", "Find next member", -1, snfcn, 1},
    {51, 49, 26, "Last", "Find last member", -1, slfcn, 1},
    {52, 50, 32, "Previous", "Find previous member", -1, spfcn, 1},
    {53, 51, 42, "Connect", "Connect current record to set", -1, scfcn, 1},
    {54, 52, 51, "Disconnect", "Disconnect current member from set", -1, sdfcn, 1},
    {55, 53, 63, "Total", "Display total members of set", -1, stfcn, 1},
    {46, 54, 70, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {57, 62, 0, "Owner", "Change current owner of a set", -1, cofcn, 1},
    {58, 56, 7, "Member", "Change current member of a set", -1, cmfcn, 1},
    {59, 57, 15, "Record", "Change current record", -1, crfcn, 1},
    {60, 58, 23, "Auto_set", "Toggle automatic set connection on/off", -1, cafcn, 1},
    {61, 59, 33, "Display", "Display Currency table", -1, cdfcn, 1},
    {62, 60, 42, "Timestamp", "Test current timestamps", 19, NULL, 0},
    {56, 61, 53, "X_exit", "Exit to parent menu", -1, cdxfcn, 0},
    {64, 66, 0, "Begin", "Begin transaction", -1, tbfcn, 0},
    {65, 63, 7, "End", "End transaction", -1, tefcn, 0},
    {66, 64, 12, "Abort", "Abort transaction", -1, tafcn, 0},
    {63, 65, 19, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {68, 73, 0, "Record", "Lock record type", -1, lrfcn, 1},
    {69, 67, 8, "Set", "Lock set type", -1, lsfcn, 1},
    {70, 68, 13, "Key", "Lock key type", -1, lkfcn, 1},
    {71, 69, 18, "Current", "Set record lock bit of current record", -1, lcfcn, 1},
    {72, 70, 27, "Timeout", "Set lock request timeout value", -1, ltfcn, 1},
    {73, 71, 36, "Display", "Display record/set lock statuses", -1, mlfcn, 1},
    {67, 72, 45, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {75, 79, 0, "Record", "Free record lock", -1, frfcn, 1},
    {76, 74, 8, "Set", "Free set lock", -1, fsfcn, 1},
    {77, 75, 13, "Key", "Free key lock", -1, fkfcn, 1},
    {78, 76, 18, "All", "Free all set and record locks", -1, fafcn, 0},
    {79, 77, 23, "Current", "Clear record lock bit of current record", -1, fcfcn, 1},
    {74, 78, 32, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {81, 82, 0, "Files", "Display database file statuses", -1, mffcn, 1},
    {82, 80, 7, "Locks", "Display set/record lock statuses", -1, mlfcn, 1},
    {80, 81, 14, "X_exit", "Exit to parent menu", -1, clear_exit, 0},
    {84, 86, 0, "Next", "Display next page of keys", -1, rsnfcn, 1},
    {85, 83, 6, "First", "Display first page of keys", -1, rsffcn, 1},
    {86, 84, 13, "Select", "Select key from current page", -1, rssfcn, 2},
    {83, 85, 21, "X_exit", "Exit to keyscan menu", -1, clear_exit, 0},
    {88, 90, 0, "Next", "Display next page of member records", -1, ssnfcn, 1},
    {89, 87, 6, "First", "Display first page of member records", -1, ssffcn, 1},
    {90, 88, 13, "Select", "Select member record from current page", -1, sssfcn, 2},
    {87, 89, 21, "X_exit", "Exit to access menu", -1, clear_exit, 2},
    {92, 95, 0, "Record", "to current record", -1, corfcn, 0},
    {93, 91, 8, "Owner", "to current owner of a set", -1, coofcn, 0},
    {94, 92, 15, "Member", "to current member of a set", -1, comfcn, 0},
    {95, 93, 23, "Change", "to database address", -1, cocfcn, 0},
    {91, 94, 31, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {97, 100, 0, "Record", "to current record", -1, cmrfcn, 0},
    {98, 96, 8, "Owner", "to current owner of a set", -1, cmofcn, 0},
    {99, 97, 15, "Member", "to current member of a set", -1, cmmfcn, 0},
    {100, 98, 23, "Change", "to database address", -1, cmcfcn, 0},
    {96, 99, 31, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {102, 104, 0, "Owner", "to current owner of a set", -1, crofcn, 0},
    {103, 101, 7, "Member", "to current member of a set", -1, crmfcn, 0},
    {104, 102, 15, "Change", "to database address", -1, crcfcn, 0},
    {101, 103, 23, "X_exit", "Exit to parent menu", -1, NULL, 0},
    {106, 108, 0, "Next", "Display next page", -1, cdnfcn, 1},
    {107, 105, 6, "Previous", "Display previous page", -1, cdpfcn, 1},
    {108, 106, 16, "Select", "Select currency record", -1, cdsfcn, 0},
    {105, 107, 24, "X_exit", "Exit to parent menu", -1, cdxfcn, 0},
    {110, 113, 0, "Record", "Test timestamp status of current record", -1, ctrfcn, 1},
    {111, 109, 8, "Owner", "Test timestamp status of current owner", -1, ctofcn, 1},
    {112, 110, 15, "Member", "Test timestamp status of current member", -1, ctmfcn, 1},
    {113, 111, 23, "Set", "Test timestamp status of current set", -1, ctsfcn, 1},
    {109, 112, 28, "X_exit", "Exit to parent menu", -1, clear_exit, 0},
    {115, 117, 0, "Next", "Display next page of records", -1, rrsnfcn, 1},
    {116, 114, 6, "First", "Display first page of records", -1, rrsffcn, 1},
    {117, 115, 13, "Select", "Select records from current page", -1, rrssfcn, 2},
    {114, 116, 21, "X_exit", "Exit to recscan menu", -1, clear_exit, 0},
    {119, 127, 0, "Edit", "Edit record", -1, edit_rec, 1},
    {120, 118, 6, "Init", "Initialize new record", -1, create_rec, 1},
    {121, 119, 12, "Next", "Display next page of data fields", -1, next_rec, 1},
    {122, 120, 18, "Prev", "Display previous page of data fields", -1, prev_rec, 1},
    {123, 121, 24, "Write", "Write record contents", -1, write_rec, 1},
    {124, 122, 31, "Store_key", "Store optional key", -1, store_key, 1},
    {125, 123, 42, "Delete_key", "Delete optional key", -1, del_key, 1},
    {126, 124, 54, "Owner", "Set owner of set to current record", -1, setor, 1},
    {127, 125, 61, "Connect", "Connect record to set", -1, scfcn, 1},
    {118, 126, 70, "X_exit", "Exit to parent menu", -1, clear_exit, 0}
};


