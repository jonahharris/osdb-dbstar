/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dbedit utility                                    *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* Error message numbers for DBEDIT - errors negative, warnings positive */

#define ERR_DBE -10000
#define WARN_DBE 10000

#define USAGE    ERR_DBE-1       /* Usage error message */

#define BAD_COM  ERR_DBE-1000    /* Invalid command */
#define BAD_OPT  ERR_DBE-1001    /* Invalid command option */
#define UNX_END  ERR_DBE-1002    /* Unexpected end of string */
#define UNX_REC  ERR_DBE-1003    /* Unexpected record name */
#define UNX_SET  ERR_DBE-1004    /* Unexpected set name */
#define UNX_FILE ERR_DBE-1005    /* Unexpected file name */
#define UNX_NUM  ERR_DBE-1006    /* Unexpected number */
#define UNX_HEX  ERR_DBE-1007    /* Unexpected hex number */
#define UNX_OPT  ERR_DBE-1008    /* Unexpected command option */
#define UNX_DBA  ERR_DBE-1009    /* Unexpected database address */
#define UNX_TOK  ERR_DBE-1010    /* Unexpected token */
#define UNX_FLD  ERR_DBE-1011    /* Unexpected field name */
#define UNX_KEY  ERR_DBE-1012    /* Unexpected key name */
#define ERR_OVF  ERR_DBE-1013    /* Command line overflow */
#define BAD_TOK  ERR_DBE-1014    /* Invalid token */
#define BAD_KWD  ERR_DBE-1015    /* Keyword ambiguous */

#define ERR_OPEN ERR_DBE-2000    /* Unable to open database */
#define ERR_VER  ERR_DBE-2001    /* Error reading object names */
#define ERR_MEM  ERR_DBE-2002    /* Unable to allocate sufficient memory */
#define ERR_CREC ERR_DBE-2003    /* No current record */
#define ERR_CFIL ERR_DBE-2004    /* No current file */
#define ERR_READ ERR_DBE-2005    /* Read error */
#define ERR_OPN  ERR_DBE-2006    /* Open error */
#define ERR_NREC ERR_DBE-2007    /* No more records */
#define ERR_PREC ERR_DBE-2008    /* No previous record */
#define ERR_OSET ERR_DBE-2009    /* No previous record */
#define ERR_MSET ERR_DBE-2010    /* No previous record */
#define ERR_RFLD ERR_DBE-2011    /* No previous record */
#define BAD_DBA  ERR_DBE-2012    /* Invalid database address */
#define BAD_FILE ERR_DBE-2013    /* Invalid file number */
#define BAD_NUM  ERR_DBE-2014    /* Invalid number */
#define BAD_TYPE ERR_DBE-2015    /* Invalid record type */
#define ERR_RFIL ERR_DBE-2016    /* Record type is not in current file */
#define BAD_HEX  ERR_DBE-2017    /* Invalid hexadecimal number */
#define ERR_WRIT ERR_DBE-2018    /* Write error */
#define ERR_NOPT ERR_DBE-2019    /* Record has no optional keys */
#define BAD_DFIL ERR_DBE-2020    /* File is not a data file */
#define ERR_NMEM ERR_DBE-2021    /* No more records in set */
#define ERR_PMEM ERR_DBE-2022    /* No previous record in set */
#define ERR_LEN  ERR_DBE-2023    /* Command line too long */
#define BAD_STR  ERR_DBE-2024    /* Invalid string */
#define ERR_FPOS ERR_DBE-2025    /* Invalid file position */
#define ERR_EOF  ERR_DBE-2026    /* End of file */
#define ERR_SNF  ERR_DBE-2027    /* String not found */
#define BAD_BASE ERR_DBE-2028    /* Invalid base */
#define BAD_LAST ERR_DBE-2029    /* Verify error */
#define BAD_FRST ERR_DBE-2030    /* Verify error */
#define ERR_CSET ERR_DBE-2031    /* Verify error */
#define BAD_MEMP ERR_DBE-2032    /* Verify error */
#define BAD_OWNP ERR_DBE-2033    /* Verify error */
#define BAD_OWNT ERR_DBE-2034    /* Verify error */
#define BAD_MEMT ERR_DBE-2035    /* Verify error */
#define ERR_PNN  ERR_DBE-2036    /* Verify error */
#define ERR_NNN  ERR_DBE-2037    /* Verify error */
#define ERR_MEMP ERR_DBE-2038    /* Verify error */
#define ERR_PREV ERR_DBE-2039    /* Verify error */
#define ERR_PN   ERR_DBE-2040    /* Verify error */
#define ERR_NN   ERR_DBE-2041    /* Verify error */
#define ERR_ADDR ERR_DBE-2042    /* File address overflow */
#define ERR_INP  ERR_DBE-2043    /* Cannot open input file */

#define WARN_DBA WARN_DBE+1000   /* Database address warning */


