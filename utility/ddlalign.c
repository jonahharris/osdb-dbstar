/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ddlp utility                                      *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/*-----------------------------------------------------------------------

    ddlalign.c  -- DDLP data field alignment computation module.

    This module deals with the alignment of data fields within
    records to match that performed by the C compiler.  The
    appropriate boundary conditions produced by the C compiler
    are discovered using simple address arithmetic on each of the
    structures declared below.  The alignment table contains one
    entry per data type and is filled by function init_align.

    Function calc_align is called by the DDLP to update a record or
    struct offset for a given data type.

-----------------------------------------------------------------------*/


#include "db.star.h"
#include "parser.h"
#include "ddldefs.h"

#define elem_sizeof(s, m) (size_t)(sizeof(((s *)0)->m))

/* ********************** LOCAL VARIABLE DECLARATIONS **************** */
/*lint -e612 */

struct fs { char f1; float f2;   };   /* Some compilers may warn   */
struct ds { char d1; double d2;  };   /* that these structures are */
struct cs { char c1; char c2;    };   /* sensitive to packing.     */
struct ws { char w1; wchar_t w2;   }; /* DO NOT CHANGE THAT!!!     */
struct ss { char s1; short s2;   };   /* These are supposed to     */
struct is { char i1; int i2;     };   /* determine the packing     */
struct ls { char l1; long l2;    };   /* method used by the        */
struct as { char a1; DB_ADDR a2; };   /* compiler                  */
struct sts {
    char st1;
    struct { char st2; } st3;
    struct struct_array { char st4; } st5[2];
};

struct stws {
    char stw1;
    struct { wchar_t st2; } st3;
    struct wstruct_array { wchar_t st4; } st5[2];
};

struct stss {
    char sts1;
    struct { short st2; } st3;
    struct sstruct_array { short st4; } st5[2];
};

struct stis {
    char sti1;
    struct { int st2; } st3;
    struct istruct_array { int st4; } st5[2];
};

struct stds {
    char std1;
    struct { double st2; } st3;
    struct dstruct_array { double st4; } st5[2];
};

/*lint +e612 */

typedef struct {
    unsigned char type;  /* field type */
    unsigned short val;  /* alignment value */
} ALIGN_S;

typedef struct      /* struct padding boundaries */
{
    unsigned short pad[MAX_ALIGNS];          /* non-arrayed */
    unsigned short array_pad[MAX_ALIGNS];    /* arrayed */
} STR_S;

static ALIGN_S align[12] = {
    { CHARACTER, 0 },
    { WIDECHAR,  0 },
    { SHORTINT,  0 },
    { REGINT,    0 },
    { LONGINT,   0 },
    { DBADDR,    0 },
    { FLOAT,     0 },
    { DOUBLE,    0 },
    { GROUPED,   0 },
    { GROUPED,   0 },
    { GROUPED,   0 },
    { GROUPED,   0 }
};

/* Initialize field alignment table
*/
void init_align(void)
{
    align[0].val  = (unsigned short) offsetof(struct cs, c2);
    align[1].val  = (unsigned short) offsetof(struct ws, w2);
    align[2].val  = (unsigned short) offsetof(struct ss, s2);
    align[3].val  = (unsigned short) offsetof(struct is, i2);
    align[4].val  = (unsigned short) offsetof(struct ls, l2);
    align[5].val  = (unsigned short) offsetof(struct as, a2);
    align[6].val  = (unsigned short) offsetof(struct fs, f2);
    align[7].val  = (unsigned short) offsetof(struct ds, d2);

    align[8].val  = (unsigned short) offsetof(struct sts, st3);
    align[9].val  = (unsigned short) offsetof(struct stss, st3);
    align[10].val = (unsigned short) offsetof(struct stis, st3);
    align[11].val = (unsigned short) offsetof(struct stds, st3);

    ddlp_g.str_pad[CHR_ALIGN]       = elem_sizeof(struct sts, st3);
    ddlp_g.str_array_pad[CHR_ALIGN] = sizeof(struct struct_array);

    ddlp_g.str_pad[SHT_ALIGN]       = elem_sizeof(struct stss, st3);
    ddlp_g.str_array_pad[SHT_ALIGN] = sizeof(struct sstruct_array);

    ddlp_g.str_pad[REG_ALIGN]       = elem_sizeof(struct stis, st3);
    ddlp_g.str_array_pad[REG_ALIGN] = sizeof(struct istruct_array);

    ddlp_g.str_pad[DBL_ALIGN]       = elem_sizeof(struct stds, st3);
    ddlp_g.str_array_pad[DBL_ALIGN] = sizeof(struct dstruct_array);
}


/* Calculate new aligned offset
*/
short calc_align(
    int offset,              /* current offset value */
    unsigned char fd_type,   /* field type */
    unsigned short str_flg)  /* Set if structure contains non-char fields */
{
    unsigned short i, j = 0;

    /* lookup data type in align table */
    while (align[j].type != fd_type)
        j++;

    /* Do special alignment for structures based on the value of str_flg
       If str_flg is DBL_ALIGN (3) then there are doubles in the structure
       and we may need to do double-word alignment (SPARC).  If str_flg is
       REG_ALIGN (2) then we are using standard word alignment.  If str_flg
       is SHT_ALIGN (1) then there are shorts and characters only and we
       use 2-byte alignment.  If str_flg is CHR_ALIGN (1) then only
       characters are in the structure and we can use 1-byte alignment.
    */
    if (fd_type == GROUPED)
        j += (unsigned short) str_flg;

    i = align[j].val;

    /* force offset to appropriate boundary */
    if (i) /* if i == 0 then we have a problem */
        offset += ((i - (offset % i)) % i);

    return ((short) offset);
}


