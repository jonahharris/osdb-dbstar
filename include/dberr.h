/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database kernel                                             *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

#ifndef DBERR_H
#define DBERR_H

/* dberr error/status messages */
static DB_TCHAR *user_error[] =
{
/*  -1 S_DBOPEN         */ DB_TEXT("database not opened"),
/*  -2 S_INVSET         */ DB_TEXT("invalid set"),
/*  -3 S_INVREC         */ DB_TEXT("invalid record"),
/*  -4 S_INVDB          */ DB_TEXT("can not open dictionary"),
/*  -5 S_INVFLD         */ DB_TEXT("invalid field name"),
/*  -6 S_INVADDR        */ DB_TEXT("invalid database address"),
/*  -7 S_NOCR           */ DB_TEXT("no current record"),
/*  -8 S_NOCO           */ DB_TEXT("set has no current owner"),
/*  -9 S_NOCM           */ DB_TEXT("set has no current member"),
/* -10 S_KEYREQD        */ DB_TEXT("key value required"),
/* -11 S_BADTYPE        */ DB_TEXT("invalid lock value"),
/* -12 S_HASMEM         */ DB_TEXT("record is owner of non-empty set(s)"),
/* -13 S_ISMEM          */ DB_TEXT("record is member of set(s)"),
/* -14 S_ISOWNED        */ DB_TEXT("member already owned"),
/* -15 S_ISCOMKEY       */ DB_TEXT("field is a compound key"),
/* -16 S_NOTCON         */ DB_TEXT("record not connected to set"),
/* -17 S_NOTKEY         */ DB_TEXT("field is not a valid key"),
/* -18 S_INVOWN         */ DB_TEXT("record not legal owner of set"),
/* -19 S_INVMEM         */ DB_TEXT("record not legal member of set"),
/* -20 S_SETPAGES       */ DB_TEXT("must call d_setpages() before opening the database"),
/* -21 S_INCOMPAT       */ DB_TEXT("incompatible dictionary file"),
/* -22 S_DELSYS         */ DB_TEXT("illegal attempt to delete/add system record"),
/* -23 S_NOTFREE        */ DB_TEXT("illegal attempt to lock locked set/record"),
/* -24 S_NOTLOCKED      */ DB_TEXT("attempt to access unlocked set/record"),
/* -25 S_TRANSID        */ DB_TEXT("transaction ID not supplied"),
/* -26 S_TRACTIVE       */ DB_TEXT("transaction already active"),
/* -27 S_TRNOTACT       */ DB_TEXT("transaction not active"),
/* -28 S_BADPATH        */ DB_TEXT("directory string is invalid"),
/* -29 S_TRFREE         */ DB_TEXT("cannot free locks within a transaction"),
/* -30 S_RECOVERY       */ DB_TEXT("automatic recovery about to occur"),
/* -31 S_NOTRANS        */ DB_TEXT("cannot update database outside a transaction"),
/* -32 S_EXCLUSIVE      */ DB_TEXT("exclusive access required"),
/* -33 S_STATIC         */ DB_TEXT("locks not allowed on static files"),
/* -34 S_USERID         */ DB_TEXT("unspecified user id"),
/* -35 S_NAMELEN        */ DB_TEXT("database path or file name too long"),
/* -36 S_RENAME         */ DB_TEXT("invalid file number was passed to d_renfile"),
/* -37 S_NOTOPTKEY      */ DB_TEXT("field is not an optional key field"),
/* -38 S_BADFIELD       */ DB_TEXT("field is not defined in current record type"),
/* -39 S_COMKEY         */ DB_TEXT("record/field has/in a compound key"),
/* -40 S_INVNUM         */ DB_TEXT("invalid record or set number"),
/* -41 S_TIMESTAMP      */ DB_TEXT("record/set not timestamped"),
/* -42 S_BADUSERID      */ DB_TEXT("bad DBUSERID (too long or contains non-alphanumeric)"),
/* -43                  */ DB_TEXT(""),
/* -44 S_INVENCRYPT     */ DB_TEXT("Invalid encryption key"),
/* -45                  */ DB_TEXT(""),
/* -46 S_NOTYPE         */ DB_TEXT("no current record type"),
/* -47 S_INVSORT        */ DB_TEXT("invalid country table sort string"),
/* -48 S_DBCLOSE        */ DB_TEXT("database not closed"),
/* -49 S_INVPTR         */ DB_TEXT("invalid pointer"),
/* -50 S_INVID          */ DB_TEXT("invalid internal ID"),
/* -51 S_INVLOCK        */ DB_TEXT("invalid lockmgr communication type"),
/* -52 S_INVTASK        */ DB_TEXT("invalid task"),
/* -53 S_NOLOCKCOMM     */ DB_TEXT("Lock Manager Communication not initialized"),
/* -54 S_NOTIMPLEMENTED */ DB_TEXT("option is not implemented in this version")
};

/* dberr system error messages */
static DB_TCHAR *system_error[] =
{
/* -900 S_NOSPACE        */ DB_TEXT("out of disk space"),
/* -901 S_SYSERR         */ DB_TEXT("system error"),
/* -902 S_FAULT          */ DB_TEXT("page fault"),
/* -903 S_NOWORK         */ DB_TEXT("cannot access dbQuery dictionary"),
/* -904 S_NOMEMORY       */ DB_TEXT("out of memory"),
/* -905 S_NOFILE         */ DB_TEXT("error opening data or key file"),
/* -906 S_DBLACCESS      */ DB_TEXT("unable to open TAF or DBL or LOG file"),
/* -907 S_DBLERR         */ DB_TEXT("error reading/writing TAF or DBL file"),
/* -908 S_BADLOCKS       */ DB_TEXT("inconsistent database locks"),
/* -909 S_RECLIMIT       */ DB_TEXT("file record limit exceeded"),
/* -910 S_KEYERR         */ DB_TEXT("key file inconsistency"),
/* -911                  */ DB_TEXT(""),
/* -912 S_FSEEK          */ DB_TEXT("file seek error"),
/* -913 S_LOGIO          */ DB_TEXT("error reading/writing LOG file"),
/* -914 S_READ           */ DB_TEXT("error reading from a data or key file"),
/* -915 S_NETSYNC        */ DB_TEXT("lock manager synchronization error"),
/* -916 S_DEBUG          */ DB_TEXT("debug check interrupt"),
/* -917 S_NETERR         */ DB_TEXT("network communications error"),
/* -918                  */ DB_TEXT(""),
/* -919 S_WRITE          */ DB_TEXT("error writing to a data or key file"),
/* -920 S_NOLOCKMGR      */ DB_TEXT("no lock manager is installed"),
/* -921 S_DUPUSERID      */ DB_TEXT("DBUSERID is already being used"),
/* -922 S_LMBUSY         */ DB_TEXT("the lock manager table(s) are full"),
/* -923 S_DISCARDED      */ DB_TEXT("attempt to lock discarded memory"),
/* -924                  */ DB_TEXT(""),
/* -925 S_LMCERROR       */ DB_TEXT(""), /* network layer error */
/* -926                  */ DB_TEXT(""),
/* -927                  */ DB_TEXT(""),
/* -928                  */ DB_TEXT(""),
/* -929                  */ DB_TEXT(""),
/* -930                  */ DB_TEXT(""),
/* -931                  */ DB_TEXT(""),
/* -932                  */ DB_TEXT(""),
/* -933                  */ DB_TEXT(""),
/* -934                  */ DB_TEXT(""),
/* -935 S_TAFCREATE      */ DB_TEXT("Failed to create taf file"),
/* -936                  */ DB_TEXT(""),
/* -937                  */ DB_TEXT(""),
/* -938                  */ DB_TEXT(""),
/* -939 S_READONLY       */ DB_TEXT("unable to update file due to READONLY option"),
/* -940 S_EACCESS        */ DB_TEXT("file in use"),
/* -941                  */ DB_TEXT(""),
/* -942                  */ DB_TEXT(""),
/* -943 S_RECFAIL        */ DB_TEXT("recovery failed"),
/* -944 S_TAFSYNC        */ DB_TEXT("TAF-lockmgr synchronization error"),
/* -945 S_TAFLOCK        */ DB_TEXT("Failed to lock TAF file"),
/* -946                  */ DB_TEXT(""),
/* -947 S_REENTER        */ DB_TEXT("db.* entered re-entrantly")
};

/* dberr internal error messages */
static DB_TCHAR *internal_error[] =
{
/* -9001                 */ DB_TEXT(""),
/* -9002                 */ DB_TEXT(""),
/* -9003 SYS_BADTREE     */ DB_TEXT("b-tree malformed"),
/* -9004 SYS_KEYEXIST    */ DB_TEXT("key value already exists"),
/* -9005                 */ DB_TEXT(""),
/* -9006 SYS_LOCKARRAY   */ DB_TEXT("lock packet array exceeded"),
/* -9007                 */ DB_TEXT(""),
/* -9008 SYS_BADFREE     */ DB_TEXT("attempt to free empty table"),
/* -9009 SYS_BADOPTKEY   */ DB_TEXT("calculating optkey index"),
/* -9010                 */ DB_TEXT(""),
/* -9011 SYS_IXNOTCLEAR  */ DB_TEXT("ix-cache not reset after trans"),
/* -9012 SYS_INVLOGPAGE  */ DB_TEXT("invalid page in log file"),
/* -9013 SYS_INVFLDTYPE  */ DB_TEXT("illegal field type"),
/* -9014 SYS_INVSORT     */ DB_TEXT("illegal sort ordering"),
/* -9015                 */ DB_TEXT(""),
/* -9016 SYS_INVPGTAG    */ DB_TEXT("invalid page tag"),
/* -9017 SYS_INVHOLD     */ DB_TEXT("bad hold count"),
/* -9018 SYS_HASHCYCLE   */ DB_TEXT("cycle detected in hash chain"),
/* -9019 SYS_INVLRU      */ DB_TEXT("invalid lru page"),
/* -9020 SYS_INVPAGE     */ DB_TEXT("invalid cache page"),
/* -9021 SYS_INVPGCOUNT  */ DB_TEXT("bad page tag page count"),
/* -9022 SYS_INVPGSIZE   */ DB_TEXT("invalid cache page size"),
/* -9023 SYS_PZACCESS    */ DB_TEXT("invalid access to page zero"),
/* -9024 SYS_BADPAGE     */ DB_TEXT("wrong page"),
/* -9025 SYS_INVEXTEND   */ DB_TEXT("illegal attempt to extend file"),
/* -9026                 */ DB_TEXT(""),
/* -9027 SYS_PZNEXT      */ DB_TEXT("bad pznext"),
/* -9028 SYS_DCHAIN      */ DB_TEXT("bad dchain"),
/* -9029 SYS_EOF         */ DB_TEXT("attempt to write past EOF"),
/* -9030 SYS_FILEMODIFIED*/ DB_TEXT("locked file was modified by another user")
};

#endif


