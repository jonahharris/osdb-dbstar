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

    ddltable.c - db.* DDL processor table handling routines

    This module contains functions that are called during the parsing
    of a db.* ddlp schema.  They store and validate the data as it
    is recognized by the parser.

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "version.h"
#include "parser.h"
#include "ddldefs.h"

#define FATAL        1
#define NOT_FATAL    0

/* ********************** GLOBAL VARIABLE DECLARATIONS *************** */
/* timestamped record list */
static ID_INFO            *trlist = NULL;
static ID_INFO            *trlast = NULL;
static int                 ts_all_recs = FALSE;

/* timestamped set list */
static ID_INFO            *tslist = NULL;
static ID_INFO            *tslast = NULL;
static int                 ts_all_sets = FALSE;

/* constant list */
static CONST_INFO         *const_list = NULL;
static CONST_INFO         *const_last = NULL;

/* type list */
static TYPE_INFO          *type_list = NULL;
static TYPE_INFO          *type_last = NULL;

static short               key_prefix = 0;


/* ********************** LOCAL FUNCTION DECLARATIONS ************* */

static int add_base_type (DB_TCHAR *, short, char, int);
static void free_elems (ELEM_INFO *);
static void free_idlist (ID_INFO *);
static void pr_hdr (void);

/* ********************** LOCAL VARIABLE DECLARATIONS **************** */
static int            lines,
                      page;
static unsigned short str_flg;       /* Set if struct contains non-chars */

static int            SystemSetDynamic = FALSE;
static int            InSystemSet = FALSE;
static short          max_page_size = 0;


/* Initialize variables, etc. */
void tableInit (void)
{
     trlist      = NULL;
     trlast      = NULL;
     ts_all_recs = FALSE;
     tslist      = NULL;
     tslast      = NULL;
     ts_all_sets = FALSE;
     const_list  = NULL;
     const_last  = NULL;
     type_list   = NULL;
     type_last   = NULL;
     key_prefix  = 0;
     InSystemSet = FALSE;
     SystemSetDynamic = FALSE;
}


int vstricmp(const char *s1, const char *s2)
{
    int result, c1, c2;

    if (s1 == NULL)
    {
        if (s2 == NULL)
            return 0;
        else
            return -1;
    }
    else if (s2 == NULL)
    {
        return 1;
    }

    do
    {
        c1 = (int) ((unsigned char) (islower(*s1) ? toupper(*s1) : *s1));
        c2 = (int) ((unsigned char) (islower(*s2) ? toupper(*s2) : *s2));
        result = c1 - c2;    /* arithmetic over-/under-flow not possible */
        s1++;
        s2++;
    } while (c1 && c2 && !result);

    return (result);
}


/* Store timestamped record list
*/
void ts_recs(ID_INFO *id_list)
{
    ID_INFO *p;

    if (id_list)
    {
        if (trlist == NULL)
            trlist = id_list;
        else
            trlast->next_id = id_list;

        for (p = id_list; p; p = p->next_id)
        {
            if (ddlp_g.x_flag)
                add_xref(NULL, p->id_name, 'r', p->id_line);
            trlast = p;
        }
    }
    else
    {
        ts_all_recs = TRUE;
    }
}



/* Store timestamped set list
*/
void ts_sets(ID_INFO *id_list)
{
    ID_INFO *p;

    if (id_list)
    {
        if (tslist == NULL)
            tslist = id_list;
        else
            tslast->next_id = id_list;

        for (p = id_list; p; p = p->next_id)
        {
            if (ddlp_g.x_flag)
                add_xref(NULL, p->id_name, 's', p->id_line);
            tslast = p;
        }
    }
    else
    {
        ts_all_sets = TRUE;
    }
}


/* Initialize type list
*/
void init_lists()
{
    trlist =       trlast =    NULL;
    tslist =       tslast =    NULL;
    ts_all_recs =  FALSE;
    ts_all_sets =  FALSE;
    const_list =   const_last =   NULL;
    type_list =    type_last =    NULL;
    ddlp_g.elem_list =    ddlp_g.elem_last =    NULL;
    ddlp_g.file_list =    ddlp_g.file_last =    NULL;
    ddlp_g.rec_list =     ddlp_g.last_rec =     NULL;
    ddlp_g.field_list =   ddlp_g.last_fld =     ddlp_g.cur_struct = NULL;
    ddlp_g.key_list =     ddlp_g.last_key =     NULL;
    ddlp_g.set_list =     ddlp_g.last_set =     NULL;
    ddlp_g.mem_list =     ddlp_g.last_mem =     NULL;
    ddlp_g.sort_list =    ddlp_g.last_sort =    NULL;
    key_prefix = 0;

    add_base_type(DB_TEXT("DB_ADDR"),         sizeof(DB_ADDR),  'd', 0);
    add_base_type(DB_TEXT("char"),            sizeof(char),     'c', 0);
    add_base_type(DB_TEXT("wchar_t"),         sizeof(wchar_t),  'C', 0);
    add_base_type(DB_TEXT("short"),           sizeof(short),    's', 0);
    add_base_type(DB_TEXT("long"),            sizeof(long),     'l', 0);
    add_base_type(DB_TEXT("float"),           sizeof(float),    'f', 0);
    add_base_type(DB_TEXT("double"),          sizeof(double),   'F', 0);
    add_base_type(DB_TEXT("unsigned char"),   sizeof(char),     'c', 1);
    add_base_type(DB_TEXT("unsigned short"),  sizeof(short),    's', 1);
    add_base_type(DB_TEXT("unsigned long"),   sizeof(long),     'l', 1);

    add_base_type(DB_TEXT("int"),             sizeof(int),      'i', 0);
    add_base_type(DB_TEXT("unsigned int"),    sizeof(int),      'i', 1);
}

/* Add a structure to the type list
*/
int add_struct_type(DB_TCHAR *name)
{
    TYPE_INFO *p = type_list;

    while (p)
    {
        if (vtstrcmp(p->type_name, name) == 0)
            return (0);
        p = p->next_type;
    }

    if ((p = (TYPE_INFO *) psp_cGetMemory(sizeof(TYPE_INFO), 0)) == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (0);
    }

    if (type_last == NULL)
        type_list = p;
    else
        type_last->next_type = p;

    type_last = p;
    p->next_type = NULL;
    vtstrcpy(p->type_name, name);
    p->elem = ddlp_g.elem_list;
    ddlp_g.elem_list = NULL;
    ddlp_g.elem_last = NULL;
    return(1);
}

/* Add a type to the type list *********************************************
*/
static int add_base_type(DB_TCHAR *name, short size, char ch, int sign)
{
    TYPE_INFO *p = type_list;
    int dim;

    while (p)
    {
        if (vtstrcmp(p->type_name, name) == 0)
            return (0);
        p = p->next_type;
    }

    if ((p = (TYPE_INFO *) psp_cGetMemory(sizeof(TYPE_INFO), 0)) == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (0);
    }

    if (type_last == NULL)
        type_list = p;
    else
        type_last->next_type = p;

    type_last = p;
    p->next_type = NULL;
    vtstrcpy(p->type_name, name);
    for (dim = 0; dim < MAXDIMS; dim++)
        p->dims[dim] = 0;
    p->type_size = size;
    p->type_char = ch;
    p->type_sign = sign;
    p->elem = NULL;
    return (1);
}

/* Add a type to the type list *********************************************
*/
int add_type(DB_TCHAR *name, TYPE_INFO *ti)
{
    TYPE_INFO *p = type_list;
    int dim, tot_dims;

    while (p)
    {
        if (vtstrcmp(p->type_name, name) == 0)
            return (0);
        p = p->next_type;
    }

    if ((p = (TYPE_INFO *) psp_cGetMemory(sizeof(TYPE_INFO), 0)) == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (0);
    }

    if (type_last == NULL)
        type_list = p;
    else
        type_last->next_type = p;

    type_last = p;
    p->next_type = NULL;
    vtstrcpy(p->type_name, name);
    memset(p->dims, 0, MAXDIMS * sizeof(short));
    for (tot_dims = 0; (tot_dims < MAXDIMS) && ti->dims[tot_dims]; tot_dims++)
        p->dims[tot_dims] = ti->dims[tot_dims];
    for (dim = 0; (dim < MAXDIMS) && ddlp_g.fd_entry.fd_dim[dim]; dim++, tot_dims++)
    {
        if (tot_dims >= MAXDIMS)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("too many array dimensions, maximum is %d"), MAXDIMS);
            dderror(ddlp_g.msg, ddlp_g.line);
            break;
        }
        p->dims[tot_dims] = ddlp_g.fd_entry.fd_dim[dim];
    }
    if (ti->elem)
    {
        p->elem = ti->elem;
        p->elem->use_count++;
    }
    else
    {
        p->type_size = ti->type_size;
        p->type_char = ti->type_char;
        p->type_sign = ti->type_sign;
    }
    return (1);
}

/* Find a constant in the constant list */
TYPE_INFO *find_type(DB_TCHAR *name, int sign)
{
    TYPE_INFO  *p = type_list;
    DB_TCHAR    tempbuff[NAMELEN + 10];

    tempbuff[0] = 0;
    if (sign)
        vtstrcpy(tempbuff, DB_TEXT("unsigned "));
    vtstrcat(tempbuff, name);

    if (vtstrcmp(DB_TEXT("db_addr"), tempbuff) == 0)
    {
        return(type_list);
    }
    else
    {
        while (p)
        {
            if (vtstrcmp(p->type_name, tempbuff) == 0)
                return (p);
            p = p->next_type;
        }
        return (NULL);
    }
}

void del_type(DB_TCHAR *name)
{
    TYPE_INFO *p = type_list, *q = NULL;

    while (p && (vtstrcmp(p->type_name, name) != 0))
    {
        q = p;
        p = p->next_type;
    }

    if (p)
    {
        if (q)
        {
            q->next_type = p->next_type;
            if (q->next_type == NULL)
                type_last = q;
        }
        else
        {
            type_list = p->next_type;
            if (type_list == NULL)
                type_last = NULL;
        }

        if (p->elem->use_count-- == 0)
            free_elems(p->elem);

        psp_freeMemory(p, 0);
    }
}

/* Add a constant to the constant list */
int add_const(DB_TCHAR *name, short value)
{
    CONST_INFO *p = const_list;

    while (p)
    {
        if (vtstrcmp(p->const_name, name) == 0)
        {
            p->value = value;
            return (0);
        }
        p = p->next_const;
    }
    
    if ((p = (CONST_INFO *) psp_cGetMemory(sizeof(CONST_INFO), 0)) == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (0);
    }

    if (const_last == NULL)
        const_list = p;
    else
        const_last->next_const = p;

    const_last = p;
    p->next_const = NULL;
    vtstrcpy(p->const_name, name);
    p->value = value;
    return (1);
}

/* Find a constant in the constant list */
CONST_INFO *find_const(DB_TCHAR *name, short *value)
{
    CONST_INFO *p = const_list;

    while (p)
    {
        if (vtstrcmp(p->const_name, name) == 0)
        {
            *value = p->value;
            return (p);
        }
        p = p->next_const;
    }
    return (NULL);
}

/* Add a structure element to the element list
*/
int add_elem(DB_TCHAR *name, char key, TYPE_INFO *ti, short *dims, CONST_INFO **ci)
{
    ELEM_INFO *p = ddlp_g.elem_list;
    unsigned short i, j;

    if (ddlp_g.s_flag)
        vtstrlwr(name);

    while (p)
    {
        if (vtstrcmp(p->field_name, name) == 0)
            return (0);
        p = p->next_elem;
    }

    if ((p = (ELEM_INFO *) psp_cGetMemory(sizeof(ELEM_INFO), 0)) == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (0);
    }

    if (ddlp_g.elem_last == NULL)
        ddlp_g.elem_list = p;
    else
        ddlp_g.elem_last->next_elem = p;

    ddlp_g.elem_last = p;
    p->next_elem = NULL;
    vtstrcpy(p->field_name, name);
    p->field_type = ti;
    p->use_count = 0;
    p->key = key;
    for (i = 0; ti && (i < MAXDIMS) && ti->dims[i]; i++)
        p->dims[i] = ti->dims[i];
    for (j = 0; i < MAXDIMS; i++, j++)
    {
        p->dims[i] = dims[j];
        p->dim_const[i] = ci[j];
    }
    return (1);
}

static void free_elems(ELEM_INFO *elem)
{
    ELEM_INFO *p = elem, *q;

    while (p)
    {
        q = p->next_elem;
        psp_freeMemory(p, 0);
        p = q;
    }
}

/* Add fields for a predefined structure
*/
void add_struct_fields(DB_TCHAR *rec_name, int line_num, ELEM_INFO *elem)
{
    int i;

    while (elem)
    {
        ddlp_g.fd_entry.fd_len = elem->field_type->type_size;
        ddlp_g.fd_entry.fd_type = elem->field_type->type_char;
        ddlp_g.fd_entry.fd_flags = (short) (elem->field_type->type_sign ? UNSIGNEDFLD : 0);
        ddlp_g.fd_entry.fd_key = elem->key;
        for (i = 0; i < MAXDIMS; i++)
        {
            ddlp_g.fd_entry.fd_dim[i] = elem->dims[i];
            if (ddlp_g.fd_entry.fd_dim[i])
                ddlp_g.fd_entry.fd_len *= ddlp_g.fd_entry.fd_dim[i];
        }
        add_field(rec_name, elem->field_name, line_num, elem->dim_const,
                elem->field_type, NULL);
        elem = elem->next_elem;
    }
}

/* Find file associated with specified record or field name
*/
static struct file_info *find_file(DB_TCHAR *rname, DB_TCHAR *name, short *fno)
{
    struct file_info *p;
    ID_INFO *q;

    for (*fno = 0, p = ddlp_g.file_list; p; p = p->next_file, ++(*fno))
    {
        for (q = p->name_list; q; q = q->next_id)
        {
            if (vtstrcmp(name, q->id_name) == 0)
            {
                if (rname)
                {
                    if (vtstrcmp(rname, q->id_rec) == 0)
                    {
                        /* match */
                        return (p);
                    }
                    else if (vtstrlen(q->id_rec) == 0)
                    {
                        /* be ready to check for future match */
                        vtstrcpy(q->id_rec, rname);
                        return (p);
                    }
                }
                else
                {
                    return (p);
                }
            }
        }
    }

    return (NULL);
}


/* Find record name in record list
*/
struct record_info *find_rec(DB_TCHAR *rname, short *rno)
{
    struct record_info *p;

    for (*rno = 0, p = ddlp_g.rec_list; p; p = p->next_rec, ++(*rno))
    {
        if (vtstrcmp(rname, p->rec_name) == 0)
            break;
    }
    return (p);
}


/* Find field name in field list
*/
static struct field_info *find_fld(DB_TCHAR *rname, DB_TCHAR *fname, short *fno)
{
    struct field_info *p;
    short rno;

    if (rname)
    {
        if (find_rec(rname, &rno) == NULL)
            return (struct field_info *) NULL;
    }
    else
    {
        rno = -1;
    }

    for (*fno = 0, p = ddlp_g.field_list; p; p = p->next_field, ++(*fno))
    {
        if (vtstrcmp(fname, p->fld_name) == 0)
        {
            if (rno >= 0)
            {
                if (rno == p->fd.fd_rec)
                    break;
            }
            else
            {
                break;
            }
        }
    }
    return (p);
}


/* Find set name in set list
*/
static struct set_info *find_set(DB_TCHAR *sname)
{
    struct set_info *p;

    for (p = ddlp_g.set_list; p; p = p->next_set)
    {
        if (vtstrcmp(p->set_name, sname) == 0)
            break;
    }
    return (p);
}


static int check_dup_name(
    DB_TCHAR *name,   /* name to check */
    DB_TCHAR *rname,  /* record name if this is a field, NULL otherwise */
    int line_num,     /* ddlp_g.line for error report */
    int fatal)        /* 1 if FATAL error, 0 otherwise */
{
    struct record_info *rec;
    struct set_info *set;
    struct field_info *field;
    short rno;
    short fno;

    /* Is it a Set name */
    for (set = ddlp_g.set_list; set; set = set->next_set)
    {
        if (vtstrcmp(set->set_name, name) == 0)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("set type %s previously defined"), name);
            if (fatal)
                ddlp_abort(ddlp_g.msg);
            else
                dderror(ddlp_g.msg, line_num);
            return (1);
        }
    }

    /* It it a record name */
    for (rec = ddlp_g.rec_list; rec; rec = rec->next_rec)
    {
        if (vtstrcmp(rec->rec_name, name) == 0)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("record type %s is already defined"), name);
            if (fatal)
                ddlp_abort(ddlp_g.msg);
            else
                dderror(ddlp_g.msg, line_num);
            return (1);
        }
    }

    /* Is it a Field name */
    /* if the ddlp_g.d_flag is not set then check all
       records because no dups are allowed
    */
    if (!ddlp_g.d_flag)
    {
        for ( fno = 0, field = ddlp_g.field_list;
              field;
              field = field->next_field, ++fno)
        {
            if (vtstrcmp(name, field->fld_name) == 0)
            {
                vstprintf(ddlp_g.msg, DB_TEXT("field '%s' is already defined"), name);
                if (fatal)
                    ddlp_abort(ddlp_g.msg);
                else
                    dderror(ddlp_g.msg, line_num);
                return (1);
            }
        }
    }
    else
    {
        /* we only need to check the given record name, otherwise OK */
        if (rname)
        {
            if (find_rec(rname, &rno) != NULL)
            {
                for ( fno = 0, field = ddlp_g.field_list;
                      field;
                      field = field->next_field, ++fno)
                {
                    if (vtstrcmp(name, field->fld_name) == 0)
                    {
                        if (rno == field->fd.fd_rec)
                        {
                            vstprintf(ddlp_g.msg,
                                DB_TEXT("field '%s' is already defined for this record"),
                                name);
                            if (fatal)
                                ddlp_abort(ddlp_g.msg);
                            else
                                dderror(ddlp_g.msg, line_num);
                            return (1);
                        }
                    }
                }
            }
        }
    }

    return (0);
}


/* Copy a file name, checking its format and length
*/
void cpfile(DB_TCHAR *ft_name, DB_TCHAR *str, int line_num)
{

    /* check the length */
    if (vtstrlen(str) > FILENMLEN - 1)
    {
        vtstrncpy(ft_name, str, FILENMLEN);
        ft_name[FILENMLEN-1] = '\0';
        vstprintf(ddlp_g.msg, DB_TEXT("file id/name %s is too long"), str);
        dderror(ddlp_g.msg, line_num);
        return;
    }
    vtstrcpy(ft_name, str);               /* unix */
}


/* Add file to file_info
*/
struct file_info *add_file(DB_TCHAR *fileid, ID_INFO *idlist, int line_num)
{
    short i;
    struct file_info *p;
    ID_INFO *q;

    if (ddlp_g.s_flag)
        vtstrlwr(fileid);

    if (ddlp_g.tot_files >= (FILEMASK + 1))
    {
        ddlp_abort(DB_TEXT("too many files defined in database"));
        return (NULL);
    }

    /* check to see if file already defined */
    for (p = ddlp_g.file_list; p; p = p->next_file)
    {
        if (fileid && vtstrcmp(p->fileid, fileid) == 0)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("file id %s has been previously defined"), fileid);
            dderror(ddlp_g.msg, line_num);
            return (NULL);
        }
        if (vtstrcmp(p->ft.ft_name, ddlp_g.ft_entry.ft_name) == 0)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("file name %s has been previously defined"), ddlp_g.ft_entry.ft_name);
            dderror(ddlp_g.msg, line_num);
            return (NULL);
        }
    }

    /* verify that rec/field names in idlist are not in other files */
    for (q = idlist; q; q = q->next_id)
    {
        if (find_file(q->id_rec, q->id_name, &i))
        {
            vstprintf(ddlp_g.msg, DB_TEXT("record/field %s is contained in another file"),
                q->id_name);
            dderror(ddlp_g.msg, line_num);
            return (NULL);
        }
        if (ddlp_g.x_flag)
        {
            add_xref(q->id_rec, q->id_name,
                (char) ((ddlp_g.ft_entry.ft_type == 'd') ? 'r' : 'f'), q->id_line);
        }
    }

    /* allocate space and fill file info */
    p = (struct file_info *) psp_cGetMemory(sizeof(struct file_info), 0);
    if (p == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (NULL);
    }

    if (ddlp_g.file_last == NULL)
        ddlp_g.file_list = p;
    else
        ddlp_g.file_last->next_file = p;
    ddlp_g.file_last = p;
    p->next_file = NULL;
    p->name_list = idlist;
    if (fileid)
        vtstrcpy(p->fileid, fileid);
    else
        p->fileid[0] = 0;
    memcpy(&(p->ft), &ddlp_g.ft_entry, sizeof(FILE_ENTRY));
    p->ft.ft_status = 'c';


    ++ddlp_g.tot_files;
    return (p);
}


/* Add record definition
*/
struct record_info *add_record(DB_TCHAR *rname, int line_num)
{
    short fno;
    struct file_info *f;
    struct record_info *p;

    if (ddlp_g.s_flag)
        vtstrlwr(rname);

    /* Ensure record not previously defined */
    check_dup_name(rname, NULL, line_num, FATAL);

    /* Ensure record contained in specified file */
    if ((f = find_file(NULL, rname, &fno)) == NULL)
    {
        vstprintf(ddlp_g.msg, DB_TEXT("record type %s is not contained in a file"), rname);
        ddlp_abort(ddlp_g.msg);
        return (NULL);
    }
    if (f->ft.ft_type != 'd')
    {
        vstprintf(ddlp_g.msg,
            DB_TEXT("record type %s needs to be contained in a data file, not a key file"),
            rname);
        ddlp_abort(ddlp_g.msg);
        return (NULL);
    }

    /* allocate space and initialize record */
    p = (struct record_info *) psp_cGetMemory(sizeof(struct record_info), 0);
    if (p == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (NULL);
    }
    if (ddlp_g.last_rec == NULL)
        ddlp_g.rec_list = p;
    else
        ddlp_g.last_rec->next_rec = p;
    ddlp_g.last_rec = p;
    vtstrncpy(p->rec_name, rname, NAMELEN - 1);
    p->rec_name[NAMELEN - 1] = 0;
    memcpy(&(p->rt), &ddlp_g.rt_entry, sizeof(RECORD_ENTRY));
    p->rt.rt_data = 0;
    p->rt.rt_len = 0;
    p->rt.rt_fdtot = (short) (vtstricmp(rname, DB_TEXT("system")) ? 0 : -1);
    p->rt.rt_fields = ddlp_g.tot_fields;
    p->rt.rt_file = fno;

    if (ts_all_recs)
        p->rt.rt_flags |= TIMESTAMPED;

    p->totown = 0;
    p->totmem = 0;
    p->next_rec = NULL;
    p->rec_num = ddlp_g.tot_records++;
    p->om_rec_type = NORMAL;
    vtstrcpy(p->om_class_type, DB_TEXT("StoreObj"));
    p->om_polymorph_member_count = 0;
    p->om_set_memberof_count = 0;
    *(p->om_default_set) = 0;
    p->om_key_count = 0;
    if (ddlp_g.x_flag)
        add_xref(NULL, rname, 'r', line_num);
    return (p);
}


/* Add field to field info
*/
struct field_info *add_field(
    DB_TCHAR *rname,
    DB_TCHAR *name,
    int line_num,
    CONST_INFO *ci[],
    TYPE_INFO *ti,
    OM_INFO *om)
{
    short key_file, i;
    struct field_info *p;
    struct file_info *f;

    if (ddlp_g.s_flag)
    {
        vtstrlwr(rname);
        vtstrlwr(name);
    }

    /* Ensure that field has not been previously defined */
    if (name && check_dup_name(name, rname, line_num, NOT_FATAL))
        return (NULL);

    if (name)
    {
        f = find_file(rname, name, &key_file);
        if (ddlp_g.fd_entry.fd_key != NOKEY && (f == NULL || f->ft.ft_type != 'k'))
        {
            vstprintf(ddlp_g.msg, DB_TEXT("key field %s is not contained in a key file"), name);
            dderror(ddlp_g.msg, line_num);
            return (NULL);
        }
        else if (ddlp_g.fd_entry.fd_key == NOKEY && f && f->ft.ft_type == 'k')
        {
            vstprintf(ddlp_g.msg, DB_TEXT("non-key field %s was contained in key file %s"),
                name, f->ft.ft_name);
            dderror(ddlp_g.msg, line_num);
        }
        else if (f && !(ddlp_g.fd_entry.fd_key != NOKEY && f->ft.ft_type == 'k'))
        {
            vstprintf(ddlp_g.msg, DB_TEXT("field and record have the same name %s"), name);
            dderror(ddlp_g.msg, line_num);
        }
        if (ddlp_g.x_flag)
            add_xref(rname, name, 'f', line_num);
    }
    p = (struct field_info *) psp_cGetMemory(sizeof(struct field_info), 0);
    if (p == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (NULL);
    }
    if (ddlp_g.last_fld == NULL)
        ddlp_g.field_list = p;
    else
        ddlp_g.last_fld->next_field = p;
    ddlp_g.last_fld = p;

    if (name)
    {
        vtstrncpy(p->fld_name, name, NAMELEN - 1);
        p->fld_name[NAMELEN - 1] = 0;
    }
    else
    {
        p->fld_name[0] = 0;
    }

    for (i = 0; i < MAXDIMS; i++)
        p->dim_const[i] = ci[i];
    p->object_info = om;
    memcpy(&(p->fd), &ddlp_g.fd_entry, sizeof(FIELD_ENTRY));

    p->fd.fd_keyfile = (short) ((ddlp_g.fd_entry.fd_key != NOKEY) ? key_file : 0);
    p->fd.fd_keyno = (short) ((ddlp_g.fd_entry.fd_key != NOKEY) ? key_prefix++ : 0);
    p->fd.fd_rec = (short) (ddlp_g.tot_records - 1);
    p->next_field = NULL;

    /* calculate number of array elements */
    for (i = 0; i < MAXDIMS; ++i)
    {
        p->fd.fd_dim[i] = ddlp_g.fd_entry.fd_dim[i];
        p->dim_const[i] = ci[i];
    }

    if (ti && ti->type_name && vtstrcmp(ti->type_name, DB_TEXT("_s_temp_")))
        p->type = ti;
    else
        p->type = NULL;

    if (!name)
    {
        if (ddlp_g.cur_struct)
        {
            dderror(DB_TEXT("nested structures are not allowed"), line_num);
        }
        else
        {
            ddlp_g.cur_struct = p;
            p->fd.fd_len = 0;
            p->fd.fd_type = GROUPED;
            str_flg = CHR_ALIGN;            /* Assume struct has only chars */
        }
    }
    else if (ddlp_g.cur_struct)
    {
        p->fd.fd_flags |= STRUCTFLD;
        switch (max((1 << str_flg), (ddlp_g.fd_entry.fd_type == CHARACTER ?
                1 : ddlp_g.fd_entry.fd_len)))
        {
            case 1: str_flg = CHR_ALIGN; break;
            case 2: str_flg = SHT_ALIGN; break;
            case 4: str_flg = REG_ALIGN; break;
            case 8: str_flg = DBL_ALIGN; break;
        }
    }

    if (p->fd.fd_type == 'k')
    {
        p->fd.fd_ptr = ddlp_g.tot_comkeyflds;
        p->fd.fd_len = 0;
        ddlp_g.last_rec->rt.rt_flags |= COMKEYED;
    }
    else
    {
        if (!ddlp_g.cur_struct && name)
        {
            /* calculate offset to start of field */
            p->fd.fd_ptr = calc_align(ddlp_g.last_rec->rt.rt_len, ddlp_g.fd_entry.fd_type, str_flg);

            /* update record length */
            ddlp_g.last_rec->rt.rt_len = (short) (p->fd.fd_ptr + ddlp_g.fd_entry.fd_len);
        }

        /* increment total fields in record */
        ++(ddlp_g.last_rec->rt.rt_fdtot);
    }
    ++ddlp_g.tot_fields;
    return (p);
}


/* Add compound key specification
*/
void add_key(DB_TCHAR *rname, DB_TCHAR *name, int line_num)
{
    short fldno;
    struct field_info *fp;
    struct ddlkey_info *kp;

    if (ddlp_g.s_flag)
    {
        vtstrlwr(rname);
        vtstrlwr(name);
    }

    /* make sure compound key is not contained in compound key */
    if (vtstrcmp(ddlp_g.last_fld->fld_name, name) == 0)
    {
        vstprintf(ddlp_g.msg,
            DB_TEXT("recursive definition for compound key '%s' is not allowed"),
            name);
        dderror(ddlp_g.msg, line_num);
        return;
    }

    /* look up field name */
    if ((fp = find_fld(rname, name, &fldno)) == NULL)
    {
        vstprintf(ddlp_g.msg, DB_TEXT("compound key element field %s is not defined"), name);
        dderror(ddlp_g.msg, line_num);
        return;
    }

    /* ensure field is defd in current record */
    if (fp->fd.fd_rec != ddlp_g.tot_records - 1)
    {
        vstprintf(ddlp_g.msg,
            DB_TEXT("compound key element field '%s' is not defined in this record"),
            name);
        dderror(ddlp_g.msg, line_num);
        return;
    }

    /* struct fields are not allowed */
    if (fp->fd.fd_type == 'g')
    {
        vstprintf(ddlp_g.msg,
            DB_TEXT("'struct' compound key fields not allowed, field '%s'"), name);
        dderror(ddlp_g.msg, line_num);
        return;
    }

    /* add key field to end of ddlp_g.key_list */
    kp = (struct ddlkey_info *) psp_cGetMemory(sizeof(struct ddlkey_info), 0);
    if (kp == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return;
    }
    if (ddlp_g.last_key == NULL)
        ddlp_g.key_list = kp;
    else
        ddlp_g.last_key->next_key = kp;
    memcpy(&(kp->kt), &ddlp_g.kt_entry, sizeof(KEY_ENTRY));
    ddlp_g.last_key = kp;
    kp->kt.kt_key = (short) (ddlp_g.tot_fields - 1);
    kp->kt.kt_field = fldno;
    fp->fd.fd_flags |= COMKEYED;

    /* calculate key field offset */
    kp->kt.kt_ptr = calc_align(ddlp_g.last_fld->fd.fd_len, fp->fd.fd_type, 0);

    /* update length of compound key */
    ddlp_g.last_fld->fd.fd_len = (unsigned short)(kp->kt.kt_ptr + fp->fd.fd_len);

    ++ddlp_g.tot_comkeyflds;
    if (ddlp_g.x_flag)
        add_xref(rname, name, 'f', line_num);
    return;
}



/* Close structure declaration
*/
void close_struct(DB_TCHAR *rname, DB_TCHAR *name, int line_num)
{
    short key_file;
    struct file_info *f;
    int elts, pad;
    int n;
    struct field_info *p;

    if (ddlp_g.s_flag)
    {
        vtstrlwr(rname);
        vtstrlwr(name);
    }

    if (!ddlp_g.cur_struct)
    {
        ddlp_abort(DB_TEXT("system error: no struct defined"));
        return;
    }

    /* calculate number of array elements */
    for (elts = 1, n = 0; n < MAXDIMS; ++n)
    {
        if (ddlp_g.cur_struct->fd.fd_dim[n])
            elts = elts * ddlp_g.cur_struct->fd.fd_dim[n];
    }

    /* Now compute field offsets inside structure */
    ddlp_g.cur_struct->fd.fd_ptr = calc_align(ddlp_g.last_rec->rt.rt_len,
        ddlp_g.cur_struct->fd.fd_type, str_flg);
    p = ddlp_g.cur_struct->next_field;
    while (p && (p->fd.fd_flags & STRUCTFLD) != 0)
    {
        /* calculate offset to start of field */
        p->fd.fd_ptr = calc_align(ddlp_g.last_rec->rt.rt_len, p->fd.fd_type, str_flg);

        if (p->fd.fd_ptr < ddlp_g.cur_struct->fd.fd_ptr)
            p->fd.fd_ptr = ddlp_g.cur_struct->fd.fd_ptr;

        /* update record length */
        ddlp_g.last_rec->rt.rt_len = (short) (p->fd.fd_ptr + p->fd.fd_len);
        ddlp_g.cur_struct->fd.fd_len = (short) (p->fd.fd_ptr + p->fd.fd_len -
                ddlp_g.cur_struct->fd.fd_ptr);
        p = p->next_field;
    }

    /* align structure as necessary */
    pad = elts > 1 ? ddlp_g.str_array_pad[str_flg] : ddlp_g.str_pad[str_flg];

    ddlp_g.cur_struct->fd.fd_len += (short)
        ((pad - (ddlp_g.cur_struct->fd.fd_len % pad)) % pad);
    ddlp_g.cur_struct->fd.fd_len *= (short) elts;

    /* update record length */
    ddlp_g.last_rec->rt.rt_len = (short)
        (ddlp_g.cur_struct->fd.fd_ptr + ddlp_g.cur_struct->fd.fd_len);

    vtstrncpy(ddlp_g.cur_struct->fld_name, name, NAMELEN - 1);
    ddlp_g.cur_struct->fld_name[NAMELEN - 1] = '\0';
    f = find_file(rname, name, &key_file);
    if (ddlp_g.cur_struct->fd.fd_key != NOKEY)
    {
        /* Ensure that key field contained in a key file */
        if ((f == NULL) || (f->ft.ft_type != 'k'))
        {
            vstprintf(ddlp_g.msg, DB_TEXT("field '%s' is not contained in a key file"),
                name);
            dderror(ddlp_g.msg, line_num);
        }
        ddlp_g.cur_struct->fd.fd_keyfile = key_file;
    }
    else
    {
        if (ddlp_g.cur_struct->fd.fd_key == NOKEY && f)
        {
            vstprintf(ddlp_g.msg, DB_TEXT("non-key field '%s' was contained in key file '%s'"),
                name, f->ft.ft_name);
            dderror(ddlp_g.msg, line_num);
        }
        ddlp_g.cur_struct->fd.fd_keyfile = 0;
    }
    ddlp_g.cur_struct = NULL;
    if (ddlp_g.x_flag)
        add_xref(rname, name, 'f', line_num);
    return;
}


/* Add set to set_info table
*/
struct set_info *add_set(DB_TCHAR *name, DB_TCHAR *owner, int line_num)
{
    struct set_info *p;
    struct record_info *r;
    short ownno;

    if (ddlp_g.s_flag)
        vtstrlwr(name);

    /* Ensure set not already defined */
    check_dup_name(name, NULL, line_num, FATAL);

    /* Ensure that owner record is defined */
    if ((r = find_rec(owner, &ownno)) == NULL)
    {
        if (vtstricmp(owner, DB_TEXT("system")) == 0)
        {
            /* create system record */
            memset(&ddlp_g.rt_entry, 0, sizeof(RECORD_ENTRY));
            r = add_record(DB_TEXT("system"), line_num);
        }
        else
        {
            vstprintf(ddlp_g.msg, DB_TEXT("owner of set '%s' is not defined"), name);
            dderror(ddlp_g.msg, line_num);
            return (NULL);
        }
    }

    if (vtstricmp(owner, DB_TEXT("system")) == 0)
        InSystemSet = TRUE;
    else
        InSystemSet = FALSE;

    p = (struct set_info *) psp_cGetMemory(sizeof(struct set_info), 0);
    if (p == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (NULL);
    }
    if (ddlp_g.last_set == NULL)
        ddlp_g.set_list = p;
    else
        ddlp_g.last_set->next_set = p;
    ddlp_g.last_set = p;
    vtstrncpy(p->set_name, name, NAMELEN - 1);
    p->set_name[NAMELEN - 1] = 0;
    memcpy(&(p->st), &ddlp_g.st_entry, sizeof(SET_ENTRY));
    p->st.st_own_rt = ownno;
    p->st.st_own_ptr = 0;
    p->st.st_members = ddlp_g.tot_members;
    p->st.st_memtot = 0;
    p->set_rec = r;
    p->om_set_type = NORMAL;

    if (ts_all_sets)
        p->st.st_flags |= TIMESTAMPED;

    p->next_set = NULL;
    ++(r->totown);
    ++ddlp_g.tot_sets;
    if (ddlp_g.x_flag)
        add_xref(NULL, name, 's', line_num);
    return (p);
}


/* Add set member
*/
struct member_info *add_member(DB_TCHAR *name, ID_INFO *by_list, int line_num)
{
    struct member_info *p;
    struct record_info *r;
    short rno, fno;

    if (ddlp_g.s_flag)
        vtstrlwr(name);

    /* Ensure that member record is defined */
    if ((r = find_rec(name, &rno)) == NULL)
    {
        vstprintf(ddlp_g.msg, DB_TEXT("member record type %s is not defined"), name);
        dderror(ddlp_g.msg, line_num);
        return (NULL);
    }

    /* there will be no ddlp_g.last_set if the owner record type was in error */
    if (!ddlp_g.last_set)
        return (NULL);
    p = (struct member_info *) psp_cGetMemory(sizeof(struct member_info), 0);
    if (p == NULL)
    {
        ddlp_abort(DB_TEXT("out of memory"));
        return (NULL);
    }
    if (ddlp_g.last_mem == NULL)
        ddlp_g.mem_list = p;
    else
        ddlp_g.last_mem->next_mbr = p;
    ddlp_g.last_mem = p;
    memcpy(&(p->mt), &ddlp_g.mt_entry, sizeof(MEMBER_ENTRY));
    p->mt.mt_record = rno;
    p->mt.mt_mem_ptr = 0;
    p->next_mbr = NULL;
    p->mem_rec = r;

    ++(ddlp_g.last_set->st.st_memtot);
    ++ddlp_g.tot_members;
    if (ddlp_g.x_flag)
        add_xref(NULL, name, 'r', line_num);
    if (by_list)
    {
        /* process sort field list */
        struct field_info *f;
        struct sort_info *s;
        ID_INFO *idlist = by_list;

        p->mt.mt_sort_fld = ddlp_g.tot_sort_fields;
        while (by_list)
        {
            f = find_fld(name, by_list->id_name, &fno);
            if (f == NULL)
            {
                vstprintf(ddlp_g.msg, DB_TEXT("sort field %s is not defined"), by_list->id_name);
                dderror(ddlp_g.msg, line_num);
                goto next_by_list;
            }
            if (f->fd.fd_rec != rno)
            {
                vstprintf(ddlp_g.msg, DB_TEXT("sort field %s is not contained in record %s"),
                    by_list->id_name, r->rec_name);
                dderror(ddlp_g.msg, line_num);
                goto next_by_list;
            }
            if (f->fd.fd_type == 'k')
            {
                vstprintf(ddlp_g.msg, DB_TEXT("sort field %s cannot be a compound key"),
                    by_list->id_name);
                dderror(ddlp_g.msg, line_num);
                goto next_by_list;
            }
            if (ddlp_g.x_flag)
                add_xref(name, by_list->id_name, 'f', by_list->id_line);
            s = (struct sort_info *) psp_cGetMemory(sizeof(struct sort_info), 0);
            if (s == NULL)
            {
                ddlp_abort(DB_TEXT("out of memory"));
                return (NULL);
            }
            if (ddlp_g.last_sort == NULL)
                ddlp_g.sort_list = s;
            else
                ddlp_g.last_sort->next_sort = s;
            ddlp_g.last_sort = s;
            f->fd.fd_flags |= SORTFLD;
            ddlp_g.tot_sort_fields++;
            s->sort_field.se_fld = fno;
            s->sort_field.se_set = (short) (ddlp_g.tot_sets - 1);
            s->next_sort = NULL;
            ++(p->mt.mt_totsf);
next_by_list:
            by_list = by_list->next_id;
        }
        free_idlist(idlist);

        if (ddlp_g.last_set->st.st_memtot > 1)
        /*  Check to make sure that the field lists used to sort each member
            match in types, lengths, and dimensions. */
        {
            struct member_info *q = ddlp_g.mem_list;
            struct sort_info *si;
            struct field_info *fd1, *fd2;
            int i;

            for (i = 0; i < ddlp_g.last_set->st.st_members; i++)
                q = q->next_mbr;

            if (q->mt.mt_totsf != p->mt.mt_totsf)
            {
                dderror(
                    DB_TEXT("Sort field lists do not match in number of fields"),
                    line_num);
                return(NULL);
            }

            for (i = 0; i < p->mt.mt_totsf; i++)
            {
                int sj;
                unsigned int uj;

                fd1 = fd2 = ddlp_g.field_list;
                si = ddlp_g.sort_list;
                for (sj = 0; sj < p->mt.mt_sort_fld + i; sj++)
                    si = si->next_sort;

                for (sj = 0; sj < si->sort_field.se_fld; sj++)
                    fd1 = fd1->next_field;

                si = ddlp_g.sort_list;
                for (sj = 0; sj < q->mt.mt_sort_fld + i; sj++)
                    si = si->next_sort;

                for (sj = 0; sj < si->sort_field.se_fld; sj++)
                    fd2 = fd2->next_field;

                if (fd1->fd.fd_type != fd2->fd.fd_type)
                {
                    dderror(
                        DB_TEXT("Sort field lists do not match in type of fields"),
                        line_num);
                    return(NULL);
                }

                if (fd1->fd.fd_len != fd2->fd.fd_len)
                {
                    dderror(
                        DB_TEXT("Sort field lists do not match in length of fields"),
                        line_num);
                    return(NULL);
                }

                for (uj = 0; uj < MAXDIMS; uj++)
                {
                    if (fd1->fd.fd_dim[uj] != fd2->fd.fd_dim[uj])
                    {
                        dderror(
                            DB_TEXT("Sort field lists do not match in dimensions"),
                            line_num);
                        return(NULL);
                    }
                }
            }
        }
    }
    else
    {
        p->mt.mt_totsf = 0;
        p->mt.mt_sort_fld = 0;
    }
    ++(r->totmem);

    if (InSystemSet)
    {
        if (!(r->rt.rt_flags & STATIC))
            SystemSetDynamic = TRUE;
    }

    return (p);
}


/* Finish up by computing owner & member pointer offsets, etc
*/
void finish_up (void)
{
    struct record_info *r;
    struct set_info *s;
    struct member_info *m;
    struct file_info *f;
    struct field_info *df, *pf = NULL;
    ID_INFO *id;
    short offset;
    short rno, fno;
    int maxp, maxi, keys, n_optkeys;

    int InSystemFile;
    int MarkStatic;
    struct file_info *SystemFile = NULL;
    struct record_info *SystemRec = NULL;

    /* mark timestamped records */
    if (!ts_all_recs)
    {
        for (id = trlist; id; id = id->next_id)
        {
            r = find_rec(id->id_name, &rno);
            if (r)
            {
                r->rt.rt_flags |= TIMESTAMPED;
            }
            else
            {
                vstprintf(ddlp_g.msg, DB_TEXT("record type %s is not defined"), id->id_name);
                dderror(ddlp_g.msg, id->id_line);
            }
        }
    }
    /* mark timestamped sets */
    if (!ts_all_sets)
    {
        for (id = tslist; id; id = id->next_id)
        {
            s = find_set(id->id_name);
            if (s)
            {
                s->st.st_flags |= TIMESTAMPED;
            }
            else
            {
                vstprintf(ddlp_g.msg, DB_TEXT("set type %s is not defined"), id->id_name);
                dderror(ddlp_g.msg, id->id_line);
            }
        }
    }

    /* scan file list to ensure that everything has been defined */
    for (max_page_size = 0, f = ddlp_g.file_list; f; f = f->next_file)
    {
        InSystemFile = FALSE;
        MarkStatic = FALSE;

        /* compute max. page size */
        if (f->ft.ft_pgsize > max_page_size)
            max_page_size = f->ft.ft_pgsize;

        /* check each name contained in file */
        for (id = f->name_list; id; id = id->next_id)
        {
            if (f->ft.ft_type == 'd')
            {
                r = find_rec(id->id_name, &rno);
                if (r)
                {
                    if (r->rt.rt_fdtot == -1)
                    {                          /* Is system record. */
                        InSystemFile = TRUE;

                        SystemFile = f;
                        SystemRec = r;
                    }
                }
                else
                {
                    vstprintf(ddlp_g.msg, DB_TEXT("data file %s contains undefined record %s"),
                        f->ft.ft_name, id->id_name);
                    ddlp_abort(ddlp_g.msg);
                    return;
                }
            }
            else
            {
                df = find_fld(id->id_rec, id->id_name, &fno);
                if (df)
                {
                    for ( rno = 0, r = ddlp_g.rec_list;
                            df->fd.fd_rec != rno;
                            r = r->next_rec, ++rno)
                    {
                            ; /* find key field's record */
                    }
                }
                else
                {
                    vstprintf(ddlp_g.msg, DB_TEXT("key file %s contains undefined key field %s"),
                        f->ft.ft_name, id->id_name);
                    ddlp_abort(ddlp_g.msg);
                    return;
                }
            }

            /* check and mark any static files */
            if (r && ((r->rt.rt_flags & STATIC) || (f->ft.ft_flags & STATIC)))
            {
                if (!(f->ft.ft_flags & STATIC) && id == f->name_list)
                {
                    /* okay on first record (File not static, but record is.) */
                    f->ft.ft_flags |= STATIC;
                }
                else if (!(r->rt.rt_flags & STATIC) ||
                            !(f->ft.ft_flags & STATIC))
                {
                    if (InSystemFile)
                    {
                        /* We're in a static file. */
                        if (!SystemSetDynamic)
                        {
                            /* And all are static members. */
                            MarkStatic = TRUE;
                            continue;
                        }
                    }

                    /* but not okay on any others */
                    vstprintf(ddlp_g.msg, DB_TEXT("file '%s' contains static and non-static data"),
                        f->ft.ft_name);
                    ddlp_abort(ddlp_g.msg);
                    return;
                }
            }
        }

        if (MarkStatic && SystemFile && SystemRec)
        {
            /* Mark the file and system record as static */
            SystemFile->ft.ft_flags |= STATIC;
            SystemRec->rt.rt_flags |= STATIC;
        }
    }

    /* for each record compute pointer/data offsets */
    for (rno = 0, r = ddlp_g.rec_list; r; r = r->next_rec, ++rno)
    {
        offset = RECHDRSIZE;
        if (r->rt.rt_flags & TIMESTAMPED)
            offset += 2 * sizeof(DB_ULONG);

        /* Count the number of optional keys in record */
        for (n_optkeys = 0, df = ddlp_g.field_list; df; df = df->next_field)
        {
            if (df->fd.fd_rec == rno && (df->fd.fd_flags & OPTKEYMASK))
                ++n_optkeys;
        }

        /* For every eight optional keys, leave one byte */
        if (n_optkeys)
            offset += (short) (((n_optkeys - 1) / BITS_PER_BYTE) + 1);

        for (s = ddlp_g.set_list; s; s = s->next_set)
        {
            /* compute owner pointer offset */
            if (s->st.st_own_rt == rno)
            {
                s->st.st_own_ptr = offset;
                offset += (short)(2 * DB_ADDR_SIZE + sizeof(F_ADDR));
                if (s->st.st_flags & TIMESTAMPED)
                    offset += sizeof(DB_ULONG);
            }
        }
        for (m = ddlp_g.mem_list; m; m = m->next_mbr)
        {
            /* compute member pointer offsets */
            if (m->mt.mt_record == rno)
            {
                m->mt.mt_mem_ptr = offset;
                offset += (short)(3 * DB_ADDR_SIZE);
            }
        }

        /* data starts after all pointers */
        r->rt.rt_data = offset;

        /* update record length */
        r->rt.rt_len += offset;

        keys = 0;
        for (df = ddlp_g.field_list; df; df = df->next_field)
        {
            /* update data field offsets */
            if (df->fd.fd_rec == rno && df->fd.fd_type == 'k')
            {
                keys = 1;
            }
            else if (df->fd.fd_rec == rno)
            {
                df->fd.fd_ptr += offset;
                if (df->fd.fd_key != NOKEY)
                    keys = 1;
            }
        }
    }

    /* compute page record slots for each file */
    for (fno = 0, f = ddlp_g.file_list; f; f = f->next_file, ++fno)
    {
        maxi = 0;
        if (f->ft.ft_type == DATA)
        {
            struct record_info *p = NULL;

            for (r = ddlp_g.rec_list; r; r = r->next_rec)
            {
                if (r->rt.rt_file == fno)
                {
                    /* find size of largest rec in file */
                    if (r->rt.rt_len > maxi)
                    {
                        maxi = r->rt.rt_len;
                        p = r;
                    }
                    else
                    {
                        if (r->rt.rt_len <= 0)
                        {
                            vstprintf(ddlp_g.msg,
                                DB_TEXT("record type '%s' is too large"), r->rec_name);
                            ddlp_abort(ddlp_g.msg);
                            return;
                        }
                    }
                }
            }
            if (f->ft.ft_pgsize == 0)
            {
                f->ft.ft_pgsize = (short)
                    (((maxi - 1 + PGHDRSIZE) / 512 + 1) * 512);
                if (f->ft.ft_pgsize < 1024)
                    f->ft.ft_pgsize = 1024;
                if (f->ft.ft_pgsize > max_page_size)
                    max_page_size = f->ft.ft_pgsize;
            }
            else if (maxi > f->ft.ft_pgsize - PGHDRSIZE)
            {
                vstprintf(ddlp_g.msg, DB_TEXT("record type '%s' is too large"), p->rec_name);
                ddlp_abort(ddlp_g.msg);
                return;
            }
            f->ft.ft_slsize = (short) (maxi = maxi + (maxi % 2));
            f->ft.ft_slots = (short) ((f->ft.ft_pgsize - PGHDRSIZE) / maxi);
        }
        else
        {
            /* key file */
            for (df = ddlp_g.field_list; df; df = df->next_field)
            {
                if ((df->fd.fd_key != NOKEY) && (df->fd.fd_keyfile == fno) &&
                    (df->fd.fd_len > maxi))
                {
                    maxi = df->fd.fd_len;
                    pf = df;
                }
            }

            /* If either of these equations change, MAXKEYDATA in qdefns.h
               will need to be changed also.
            */
            /* upd time   filled slots   orphaned child    */
            maxp = f->ft.ft_pgsize - PGHDRSIZE - sizeof(short) -
                sizeof(F_ADDR);

            /*      child node ptr   key prefix         database addr */
            maxi += sizeof(F_ADDR) + sizeof(short) + DB_ADDR_SIZE;
            if (maxi > MAXKEYSIZE)   /*  IF MAXKEYSIZE INCREASES > 256, PUT */
            {                        /*  IN A CHANGE TO COMPUTE PAGE SIZE   */
                vstprintf(ddlp_g.msg, DB_TEXT("key field '%s' is too large"), pf->fld_name);
                ddlp_abort(ddlp_g.msg);
                return;
            }
            f->ft.ft_slsize = (short) (maxi = maxi + (maxi % 2));
            if ((f->ft.ft_slots = (short) (maxp / maxi)) < 4)
            {
                vstprintf(ddlp_g.msg, DB_TEXT("key file '%s' has less than 4 slots per page"),
                    f->ft.ft_name);
                ddlp_abort(ddlp_g.msg);
                return;
            }
        }
    }
}


/* Write header from .ddl into .h
*/
void write_header()
{
    FILE *dfile;
    DB_TINT ch = 0;
    DB_TCHAR key[10], tempbuff[9];
    int i = 0, j, ok = 0, start_of_line = TRUE;

    vtstrcpy(key, DB_TEXT("DATABASE"));
    tempbuff[8] = 0;

    if ((dfile = vtfopen(ddlp_g.ddlfile, ddlp_g.u_flag ? DB_TEXT("rb") : DB_TEXT("r"))) == NULL)
    {
        vstprintf(ddlp_g.msg,
            DB_TEXT("Error (errno=%d): unable to open database ddlp_g.schema file\n    '%s'"),
            errno, ddlp_g.ddlfile);
        ddlp_abort(ddlp_g.msg);
        return;
    }

    while (! ok && (ch != DB_TEOF)) /* in case we can't find 'database' word */
    {
        ch = vgettc(dfile);
        if (vistalpha(ch) && start_of_line)
        {
            tempbuff[i++] = (DB_TCHAR) ch;
            if (i == 8)
            {
                if (vtstricmp(tempbuff, key) == 0)
                {
                    ok = 1;
                    continue;
                }
                for (j = 0; j < i; j++)
                    vputtc((DB_TINT) tempbuff[j], ddlp_g.ifile);
                i = 0;
                start_of_line = FALSE;
            }
        }
        else
        {
            for (j = 0; j < i; j++)
                vputtc((DB_TINT) tempbuff[j], ddlp_g.ifile);
            if (!vistspace(ch))
                start_of_line = FALSE;
            i = 0;

            vputtc(ch, ddlp_g.ifile);
            if (ch == (DB_TINT) DB_TEXT('/'))
            {
                ch = vgettc(dfile);
                if (ch == (DB_TINT) DB_TEXT('*'))
                {
                    vputtc(ch, ddlp_g.ifile);
                    do
                    {
                        while ((ch = vgettc(dfile)) != (DB_TINT) DB_TEXT('*'))
                            vputtc(ch, ddlp_g.ifile);
                        vputtc(ch, ddlp_g.ifile);
                        if ((ch = vgettc(dfile)) != (DB_TINT) DB_TEXT('/'))
                            vungettc(ch, dfile);
                    } while (ch != (DB_TINT) DB_TEXT('/'));
                    vputtc(ch, ddlp_g.ifile);
                }
                else if (ch == (DB_TINT) DB_TEXT('/'))
                {
                    vputtc(ch, ddlp_g.ifile);
                    while ((ch = vgettc(dfile)) != (DB_TINT) DB_TEXT('\n'))
                        vputtc(ch, ddlp_g.ifile);
                    vputtc(ch, ddlp_g.ifile);
                    start_of_line = TRUE;
                }
            }
            else
            {
                if (ch == (DB_TINT) DB_TEXT('\n'))
                    start_of_line = TRUE;
            }
        }
    }

    /* If we can't find 'database' in ddlp_g.schema file */
    if (ch == DB_TEOF && ! ok)
    {
        vstprintf(ddlp_g.msg,
            DB_TEXT("Error: unable to find 'database' keyword in ddlp_g.schema file\n'%s'"),
            ddlp_g.ddlfile);
        ddlp_abort(ddlp_g.msg);
        return;
    }

    /* close the input file */
    fclose(dfile);
}


/* create record struct declarations */
void pr_structs (void)
{
    int i, n;
    struct record_info *r;
    struct field_info *d;

    for (i = 0, d = ddlp_g.field_list, r = ddlp_g.rec_list; r; r = r->next_rec, ++i)
    {
        /* generate output only if not system record, and fields are present */
        if (vtstricmp(r->rec_name, DB_TEXT("system")) == 0 || r->rt.rt_fdtot <= 0)
            continue;

        vftprintf(ddlp_g.ifile, DB_TEXT("struct %s {\n"), r->rec_name);
        while (d && d->fd.fd_rec == i)
        {
            if (d->fd.fd_type != 'k')
                d = pr_decl(d);
            d = d->next_field;
        }
        if (ddlp_g.cur_struct)
        {
            vftprintf(ddlp_g.ifile, DB_TEXT("   } %s"), ddlp_g.cur_struct->fld_name);
            for (n = 0; ((n < MAXDIMS) && (ddlp_g.cur_struct->fd.fd_dim[n])); ++n)
            {
                if (ddlp_g.cur_struct->dim_const[n])
                    vftprintf(ddlp_g.ifile, DB_TEXT("[%s]"), ddlp_g.cur_struct->dim_const[n]->const_name);
                else
                    vftprintf(ddlp_g.ifile, DB_TEXT("[%d]"), ddlp_g.cur_struct->fd.fd_dim[n]);
            }
            vftprintf(ddlp_g.ifile, DB_TEXT(";\n"));
            ddlp_g.cur_struct = NULL;
        }
        vftprintf(ddlp_g.ifile, DB_TEXT("};\n\n"));
    }
}

/* create compound key struct declarations */
static void pr_compound_keys (void)
{
    int i, j;
    struct record_info *r;
    struct field_info *d, *cf;
    struct ddlkey_info *k;

    for (i = 0, d = ddlp_g.field_list, k = ddlp_g.key_list; d; d = d->next_field, ++i)
    {
        if (d->fd.fd_type != 'k')
            continue;

        if (!ddlp_g.d_flag)
        {
            vftprintf(ddlp_g.ifile, DB_TEXT("struct %s {\n"), d->fld_name);
        }
        else
        {
            for (j = 0, r = ddlp_g.rec_list; j < d->fd.fd_rec; r = r->next_rec, ++j)
                ;
            vftprintf(ddlp_g.ifile, DB_TEXT("struct %s_%s {\n"), r->rec_name, d->fld_name);
        }

        while (k && k->kt.kt_key == i)
        {
            int nFld;

            /* find field entry */
            for ( nFld = 0, cf = ddlp_g.field_list; 
                    nFld != k->kt.kt_field;
                    cf = cf->next_field, ++nFld)
            {
                ;
            }
            pr_decl(cf);
            k = k->next_key;
        }
        vftprintf(ddlp_g.ifile, DB_TEXT("};\n\n"));
    }
}


void write_dm_header (void)
{
    DB_TCHAR temp[FILENMLEN];

    /* construct include file name */
    vtstrncpy(ddlp_g.ifn, ddlp_g.db_name, FILENMLEN - 4);
    ddlp_g.ifn[FILENMLEN - 4] = 0;
    vtstrcat(ddlp_g.ifn, DB_TEXT(".h"));

    /* open database include file */
    if ((ddlp_g.ifile = vtfopen(ddlp_g.ifn, ddlp_g.u_flag ? DB_TEXT("wb") : DB_TEXT("w"))) == NULL)
    {
        vstprintf(ddlp_g.msg, DB_TEXT("Error (errno=%d): unable to create include file '%s'"),
            errno, ddlp_g.ifn);
        ddlp_abort(ddlp_g.msg);
        return;
    }

    vtstrupr(vtstrcpy(temp, ddlp_g.db_name));
    vftprintf(ddlp_g.ifile, DB_TEXT("#ifndef %s_H\n#define %s_H\n\n"), temp, temp);
    vftprintf(ddlp_g.ifile, DB_TEXT("/* db.* %s */\n\n"), DBSTAR_VERSION);

    write_header();
    if (ddlp_g.abort_flag)
        return;

    vftprintf(ddlp_g.ifile,
        DB_TEXT("\n/* database %s record/key structure declarations */\n\n"), ddlp_g.db_name);

    /* create record struct declarations */
    pr_structs();

    /* create compound key struct declarations */
    pr_compound_keys();

    /* print constants */
    pr_constants();

    vftprintf(ddlp_g.ifile, DB_TEXT("\n#endif    /* %s_H */\n"), temp);
    fclose(ddlp_g.ifile);
}


/* Write out database tables
*/
void write_tables (void)
{
    struct file_info *f;
    struct record_info *r;
    struct field_info *d;
    struct set_info *s;
    struct member_info *m;
    struct sort_info *o;
    struct ddlkey_info *k;
    DB_TCHAR tobjname[NAMELEN + 1];  /* add one to handle '\n' */
    PSP_FH tfile;                   /* table file descr */
    DB_TCHAR tfn[FILENMLEN];         /* table file name */
    int v = 300;
    char *ver = dbd_VERSION_300;
#if defined(UNICODE)
    char cobjname[NAMELEN + 1];
#endif

    if (!ddlp_g.db_name[0])
        /* Parsing error! But UNIX open() will create ".dbd" successfully, */
        /* so we need to stop here before open_x() is called */
    {
        vstprintf(ddlp_g.msg, DB_TEXT("Parsing Error: unable to create dictionary file '%s'"),
            tfn);
        ddlp_abort(ddlp_g.msg);
        return;
    }

    /* generate table file name */
    vtstrncpy(tfn, ddlp_g.db_name, FILENMLEN - 6);
    vtstrcat(tfn, DB_TEXT(".dbd"));

    /* unlink it if it exists */
    psp_fileRemove(tfn);

    /* open output file */
    tfile = psp_fileOpen(tfn, O_RDWR | O_CREAT, 0);
    if (!tfile)
    {
        vstprintf(ddlp_g.msg, DB_TEXT("Error (errno=%d): unable to create dictionary file '%s'"),
            psp_errno(), tfn);
        ddlp_abort(ddlp_g.msg);
        return;
    }

    /* determine format (v3.01 if new keywords used, otherwise v3.00) */
    /* if any filenames are longer than 47 chars, use db.* 5.0 format  */
    for (f = ddlp_g.file_list; f; f = f->next_file)
    {
        if (vtstrlen(f->ft.ft_name) > V3_FILENMLEN - 1)
        {
            v = 500;
            ver = dbd_VERSION_500;
            break;
        }
        if (f->ft.ft_initial + f->ft.ft_next + f->ft.ft_pctincrease)
        {
            v = 301;
            ver = dbd_VERSION_301;
        }
    }

#if defined(UNICODE)
    if (ddlp_g.u_flag)
    {
        v = -500;
        ver = dbd_VERSION_u500;
    }
#endif

    /* output dictionary version */
    psp_fileWrite(tfile, ver, DBD_COMPAT_LEN);

    /* output page size */
    psp_fileWrite(tfile, (char *)&max_page_size, sizeof(short));

    /* output file sizes */
    psp_fileWrite(tfile, &ddlp_g.tot_files, sizeof(short));
    psp_fileWrite(tfile, &ddlp_g.tot_records, sizeof(short));
    psp_fileWrite(tfile, &ddlp_g.tot_fields, sizeof(short));
    psp_fileWrite(tfile, &ddlp_g.tot_sets, sizeof(short));
    psp_fileWrite(tfile, &ddlp_g.tot_members, sizeof(short));
    psp_fileWrite(tfile, &ddlp_g.tot_sort_fields, sizeof(short));
    psp_fileWrite(tfile, &ddlp_g.tot_comkeyflds, sizeof(short));


    /* output file table */
    switch (v)
    {
        case 301:
        {
            V301_FILE_ENTRY fe;
            for (f = ddlp_g.file_list; f; f = f->next_file)
            {
                memset(&fe, 0, sizeof(V301_FILE_ENTRY));
                ttoa(f->ft.ft_name, fe.v3_ft_name, V3_FILENMLEN);
                fe.v3_ft_desc    = 0;
                fe.v3_ft_status  = 'c';
                fe.v3_ft_type    = f->ft.ft_type;
                fe.v3_ft_slots   = f->ft.ft_slots;
                fe.v3_ft_slsize  = f->ft.ft_slsize;
                fe.v3_ft_pgsize  = f->ft.ft_pgsize;
                fe.v3_ft_flags   = f->ft.ft_flags;
                fe.v3_ft_pctincrease = f->ft.ft_pctincrease;
                fe.v3_ft_initial = f->ft.ft_initial;
                fe.v3_ft_next    = f->ft.ft_next;
                psp_fileWrite(tfile, &fe, sizeof(V301_FILE_ENTRY));
            }
            break;
        }

        case 500:
        {
            V500_FILE_ENTRY fe;
            for (f = ddlp_g.file_list; f; f = f->next_file)
            {
                memset(&fe, 0, sizeof(V500_FILE_ENTRY));
                ttoa(f->ft.ft_name, fe.v5_ft_name, FILENMLEN);
                fe.v5_ft_desc    = 0;
                fe.v5_ft_status  = 'c';
                fe.v5_ft_type    = f->ft.ft_type;
                fe.v5_ft_slots   = f->ft.ft_slots;
                fe.v5_ft_slsize  = f->ft.ft_slsize;
                fe.v5_ft_pgsize  = f->ft.ft_pgsize;
                fe.v5_ft_flags   = f->ft.ft_flags;
                fe.v5_ft_pctincrease = f->ft.ft_pctincrease;
                fe.v5_ft_initial = f->ft.ft_initial;
                fe.v5_ft_next    = f->ft.ft_next;
                psp_fileWrite(tfile, &fe, sizeof(V500_FILE_ENTRY));
            }
            break;
        }

        case -500:
            for (f = ddlp_g.file_list; f; f = f->next_file)
                psp_fileWrite(tfile, &f->ft, sizeof(FILE_ENTRY));

            break;

        default:
        {
            V300_FILE_ENTRY fe;
            for (f = ddlp_g.file_list; f; f = f->next_file)
            {
                memset(&fe, 0, sizeof(V300_FILE_ENTRY));
                ttoa(f->ft.ft_name, fe.v3_ft_name, V3_FILENMLEN);
                fe.v3_ft_desc    = 0;
                fe.v3_ft_status  = 'c';
                fe.v3_ft_type    = f->ft.ft_type;
                fe.v3_ft_slots   = f->ft.ft_slots;
                fe.v3_ft_slsize  = f->ft.ft_slsize;
                fe.v3_ft_pgsize  = f->ft.ft_pgsize;
                fe.v3_ft_flags   = f->ft.ft_flags;
                psp_fileWrite(tfile, &fe, sizeof(V300_FILE_ENTRY));
            }
            break;
        }
    }

    /* output record table */
    for (r = ddlp_g.rec_list; r; r = r->next_rec)
        psp_fileWrite(tfile, &(r->rt), sizeof(RECORD_ENTRY));

    /* output field table */
    for (d = ddlp_g.field_list; d; d = d->next_field)
        psp_fileWrite(tfile, &(d->fd), sizeof(FIELD_ENTRY));

    /* output set table */
    for (s = ddlp_g.set_list; s; s = s->next_set)
        psp_fileWrite(tfile, &(s->st), sizeof(SET_ENTRY));

    /* output member table */
    for (m = ddlp_g.mem_list; m; m = m->next_mbr)
        psp_fileWrite(tfile, &(m->mt), sizeof(MEMBER_ENTRY));

    /* output sort table */
    for (o = ddlp_g.sort_list; o; o = o->next_sort)
        psp_fileWrite(tfile, &(o->sort_field), sizeof(SORT_ENTRY));

    /* output compound key table */
    for (k = ddlp_g.key_list; k; k = k->next_key)
        psp_fileWrite(tfile, &(k->kt), sizeof(KEY_ENTRY));

    if (!ddlp_g.n_flag)
    {
        /* output name lists */

        /* records */
        for (r = ddlp_g.rec_list; r; r = r->next_rec)
        {
            vtstrupr(vtstrcpy(tobjname, r->rec_name));
#if defined(UNICODE)
            if (v > 0)
            {
                wtoa(tobjname, cobjname, sizeof(cobjname));
                strcat(cobjname, "\n");
                psp_fileWrite(tfile, cobjname, strlen(cobjname));
                continue;
            }
#endif
            vtstrcat(tobjname, DB_TEXT("\n"));
            psp_fileWrite(tfile, tobjname, vtstrlen(tobjname) * sizeof(DB_TCHAR));
        }

        /* fields */
        for (d = ddlp_g.field_list; d; d = d->next_field)
        {
            vtstrupr(vtstrcpy(tobjname, d->fld_name));
#if defined(UNICODE)
            if (v > 0)
            {
                wtoa(tobjname, cobjname, sizeof(cobjname));
                strcat(cobjname, "\n");
                psp_fileWrite(tfile, cobjname, strlen(cobjname));
                continue;
            }
#endif
            vtstrcat(tobjname, DB_TEXT("\n"));
            psp_fileWrite(tfile, tobjname, vtstrlen(tobjname) * sizeof(DB_TCHAR));
        }
        
        /* sets */
        for (s = ddlp_g.set_list; s; s = s->next_set)
        {
            vtstrupr(vtstrcpy(tobjname, s->set_name));
#if defined(UNICODE)
            if (v > 0)
            {
                /* non-Unicode dbd - convert names to ansi */
                wtoa(tobjname, cobjname, sizeof(cobjname));
                strcat(cobjname, "\n");
                psp_fileWrite(tfile, cobjname, strlen(cobjname));
                continue;
            }
#endif
            vtstrcat(tobjname, DB_TEXT("\n"));
            psp_fileWrite(tfile, tobjname, vtstrlen(tobjname) * sizeof(DB_TCHAR));
        }
    }

#if defined(SAFEGARDE)
    if (ddlp_g.sg)
    {
        char data[16];

        (*ddlp_g.sg->set)(ddlp_g.sg->data, data);
        psp_fileWrite(tfile, data, sizeof(data));
    }
#endif

    psp_fileClose(tfile);

    /* write out header file */
    write_dm_header();
}

/* Print constants
*/
void pr_constants (void)
{
    int                  i,
                         j,
                         recIndex;
    long                 fldnum;
    struct file_info    *f;
    struct record_info  *r;
    struct field_info   *d;
    struct set_info     *s;
    DB_TCHAR             temp_rec[FILENMLEN],
                         temp[FILENMLEN];

    vftprintf(ddlp_g.ifile, DB_TEXT("/* record, field and set table entry definitions */\n"));

    /* output file constants */
    vftprintf(ddlp_g.ifile, DB_TEXT("\n/* File Id Constants */\n"));
    for (i = 0, f = ddlp_g.file_list; f; f = f->next_file, ++i)
    {
        vtstrupr(vtstrcpy(temp, f->fileid));
        if (f->fileid[0])
            vftprintf(ddlp_g.ifile, DB_TEXT("#define %s %d\n"), temp, i);
    }

    /* output record constants */
    vftprintf(ddlp_g.ifile, DB_TEXT("\n/* Record Name Constants */\n"));
    for (i = 0, r = ddlp_g.rec_list; r; r = r->next_rec, ++i)
    {
        vtstrupr(vtstrcpy(temp, r->rec_name));
        if (vtstricmp(r->rec_name, DB_TEXT("system")) != 0)
            vftprintf(ddlp_g.ifile, DB_TEXT("#define %s %d\n"), temp, i + RECMARK);
        else
            r->om_rec_type = SYSTEM;
    }
    
    /* output field constants */
    vftprintf(ddlp_g.ifile, DB_TEXT("\n/* Field Name Constants */\n"));
    
    /* Need to start recIndex at -1 so that first field in the first
        record is initialized correctly.
    */
    recIndex = -1;
    fldnum = 0;
    for (d = ddlp_g.field_list; d; d = d->next_field)
    {
        if (d->fd.fd_rec == recIndex)
        {
            /* same record, just get the next number */
            fldnum++;
        }
        else
        {
            /* different record, reset numbers */
            recIndex = d->fd.fd_rec;
            fldnum = FLDMARK * recIndex;
            r = NULL;
        }

        vtstrupr(vtstrcpy(temp, d->fld_name));
        if (! ddlp_g.d_flag)
            vftprintf(ddlp_g.ifile, DB_TEXT("#define %s %ldL\n"), temp, fldnum);
        else
        {
            if (r == NULL)
            {
                for (j = recIndex, r = ddlp_g.rec_list; j; j--)
                    r = r->next_rec;
            }

            vtstrupr(vtstrcpy(temp_rec, r->rec_name));
            vftprintf(ddlp_g.ifile, DB_TEXT("#define %s_%s %ldL\n"), temp_rec, temp, fldnum);
        }
    }

    /* output set constants */
    vftprintf(ddlp_g.ifile, DB_TEXT("\n/* Set Name Constants */\n"));

    for (i = 0, s = ddlp_g.set_list; s; s = s->next_set, ++i)
    {
        vtstrupr(vtstrcpy(temp, s->set_name));
        vftprintf(ddlp_g.ifile, DB_TEXT("#define %s %d\n"), temp, i + SETMARK);
    }

    if (ddlp_g.z_flag)  /* do not output sizes */
        return;

    /* output field sizes */
    vftprintf(ddlp_g.ifile, DB_TEXT("\n/* Field Sizes */\n"));
    for (d = ddlp_g.field_list; d; d = d->next_field)
    {
        if ((d->fd.fd_type == 'g') || (d->fd.fd_type == 'k'))
            continue;

        vtstrupr(vtstrcpy(temp, d->fld_name));
        if (! ddlp_g.d_flag)
        {
            vftprintf(ddlp_g.ifile, DB_TEXT("#define SIZEOF_%s %d\n"), temp, d->fd.fd_len);
        }
        else
        {
            for (j = d->fd.fd_rec, r = ddlp_g.rec_list; j; j--)
                r = r->next_rec;

            vtstrupr(vtstrcpy(temp_rec, r->rec_name));
            vftprintf(ddlp_g.ifile, DB_TEXT("#define SIZEOF_%s_%s %d\n"), temp_rec, temp,
                    d->fd.fd_len);
        }
    }
}

/* Print field declaration
*/
static void pr_initializer(struct field_info *d)
{
    int n;
    struct field_info *cf;

    if (d->fd.fd_type == 'g')
    {
        ddlp_g.cur_struct = d;
        for ( cf = d->next_field;
                cf->fd.fd_flags & STRUCTFLD;
                cf = cf->next_field)
        {
            pr_initializer(cf);
        }

        ddlp_g.cur_struct = NULL;
    }
    else
    {
        vftprintf(ddlp_g.ifile, DB_TEXT(", "));
        if (ddlp_g.cur_struct)
        {
            vftprintf(ddlp_g.ifile, DB_TEXT("%s"), ddlp_g.cur_struct->fld_name);
            for (n = 0; ((n < MAXDIMS) && (ddlp_g.cur_struct->fd.fd_dim[n])); ++n)
                vftprintf(ddlp_g.ifile, DB_TEXT("[0]"));
            vftprintf(ddlp_g.ifile, DB_TEXT("."));
        }

        vftprintf(ddlp_g.ifile, DB_TEXT("%s"), d->fld_name);

        switch (d->fd.fd_type)
        {
            case 'c':
                vftprintf(ddlp_g.ifile, DB_TEXT("((char *) '\\0')")); 
                break;

            case 'C':
                vftprintf(ddlp_g.ifile, DB_TEXT("(_TEXT('\\0'))")); 
                break;

            case 's':
            case 'i':
            case 'l':
            case 'd':
                vftprintf(ddlp_g.ifile, DB_TEXT("(0)"));
                break;

            case 'f':
            case 'F':
                vftprintf(ddlp_g.ifile, DB_TEXT("(0.0)"));
                break;
        }
    }
}

/* Print field declaration
*/
struct field_info *pr_decl(struct field_info *d)
{
    DB_TCHAR decl[80];
    int n;
    DB_TCHAR *p;

    if (d->fd.fd_flags & STRUCTFLD)
    {
        if (ddlp_g.cur_struct)
            vftprintf(ddlp_g.ifile, DB_TEXT("   "));
    }
    else if (ddlp_g.cur_struct)
    {
        vftprintf(ddlp_g.ifile, DB_TEXT("   } %s"), ddlp_g.cur_struct->fld_name);
        for (n = 0; ((n < MAXDIMS) && (ddlp_g.cur_struct->fd.fd_dim[n])); ++n)
        {
            if (ddlp_g.cur_struct->dim_const[n])
                vftprintf(ddlp_g.ifile, DB_TEXT("[%s]"), ddlp_g.cur_struct->dim_const[n]->const_name);
            else
                vftprintf(ddlp_g.ifile, DB_TEXT("[%d]"), ddlp_g.cur_struct->fd.fd_dim[n]);
        }
        
        vftprintf(ddlp_g.ifile, DB_TEXT(";\n"));
        ddlp_g.cur_struct = NULL;
    }

    vftprintf(ddlp_g.ifile, DB_TEXT("   "));
    if (d->fd.fd_type == 'g')
    {
        if (d->type)
        {
            if (  (d->type->type_name[0] == DB_TEXT('_')) &&
                    (d->type->type_name[1] == DB_TEXT('s')) &&
                    (d->type->type_name[2] == DB_TEXT('_')) )
            {
                vftprintf(ddlp_g.ifile, DB_TEXT("struct %s %s;\n"), &(d->type->type_name[3]),
                        d->fld_name);
            }
            else
            {
                vftprintf(ddlp_g.ifile, DB_TEXT("%s %s;\n"), d->type->type_name, d->fld_name);
            }
            
            while (d->next_field && (d->next_field->fd.fd_flags & STRUCTFLD))
                d = d->next_field;
        }
        else
        {
            vftprintf(ddlp_g.ifile, DB_TEXT("struct {\n"));
            ddlp_g.cur_struct = d;
        }
    }
    else
    {
        if (d->type)
        {
            p = d->type->type_name;
            vtstrcpy(decl, p);
        }
        else
        {
            if (d->fd.fd_flags & UNSIGNEDFLD)
                vtstrcpy(decl, DB_TEXT("unsigned "));
            else
                decl[0] = 0;
            switch (d->fd.fd_type)
            {
                case 'c':   vtstrcat(decl, DB_TEXT("char "));     break;
                case 'C':   vtstrcat(decl, DB_TEXT("wchar_t "));  break;
                case 's':   vtstrcat(decl, DB_TEXT("short "));    break;
                case 'l':   vtstrcat(decl, DB_TEXT("long "));     break;
                case 'f':   vtstrcat(decl, DB_TEXT("float "));    break;
                case 'F':   vtstrcat(decl, DB_TEXT("double "));   break;
                case 'd':   vtstrcat(decl, DB_TEXT("DB_ADDR "));  break;
                case 'i':
                    p = DB_TEXT("int ");
                    vtstrcat(decl, p);
                    break;
            }
        }
        
        vftprintf(ddlp_g.ifile, DB_TEXT("%s %s"), decl, d->fld_name);
        for (n = 0; ((n < MAXDIMS) && (d->type->dims[n])); ++n)
            ;     /* skip those dimensions in the type */
            
        for ( ; ((n < MAXDIMS) && (d->fd.fd_dim[n])); ++n)
        {
            if (d->dim_const[n])
                vftprintf(ddlp_g.ifile, DB_TEXT("[%s]"), d->dim_const[n]->const_name);
            else
                vftprintf(ddlp_g.ifile, DB_TEXT("[%d]"), d->fd.fd_dim[n]);
        }
        
        vftprintf(ddlp_g.ifile, DB_TEXT(";\n"));
    }
    return d;
}


/* Print database table statistics
*/
void print_tables (void)
{
    struct file_info *f;
    struct record_info *r;
    int i;
    int free_space;

    lines = 55;
    page = 0;
    for (i = 0, f = ddlp_g.file_list; f; f = f->next_file, ++i)
    {
        if (f->ft.ft_type == 'd')
        {
            free_space = f->ft.ft_pgsize - PGHDRSIZE
                - f->ft.ft_slots * f->ft.ft_slsize;
        }
        else
        {
            free_space = f->ft.ft_pgsize - PGHDRSIZE - sizeof(short) -
                sizeof(F_ADDR) - f->ft.ft_slots * f->ft.ft_slsize;
        }
        if ((lines += 7) > 52)
            pr_hdr();

        vftprintf(ddlp_g.outfile, DB_TEXT("FILE: %s\n"), f->ft.ft_name);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Id  : %d\n"), i);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Type: %s\n"), (f->ft.ft_type == 'k' ? DB_TEXT("key") : DB_TEXT("data")));
        vftprintf(ddlp_g.outfile, DB_TEXT("   Size of record slots : %d\n"), f->ft.ft_slsize);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Record slots per page: %d\n"), f->ft.ft_slots);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Unused page space    : %d\n\n"), free_space);
    }
    for (i = 0, r = ddlp_g.rec_list; r; r = r->next_rec, ++i)
    {
        int j;

        /* Find file entry associated with this record */
        for (j = 0, f = ddlp_g.file_list; j != r->rt.rt_file; j++)
            f = f->next_file;

        if ((lines += 9) > 52)
            pr_hdr();

        vftprintf(ddlp_g.outfile, DB_TEXT("RECORD: %s\n"), r->rec_name);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Id  : %d\n"), i);
        vftprintf(ddlp_g.outfile, DB_TEXT("   File: %s [%d]\n"), f->ft.ft_name, r->rt.rt_file);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Total set pointers   : %d\n"), r->totown);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Total member pointers: %d\n"), r->totmem);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Offset to data       : %d\n"), r->rt.rt_data);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Size of record       : %d\n"), r->rt.rt_len);
        vftprintf(ddlp_g.outfile, DB_TEXT("   Unused slot space    : %d\n\n"), f->ft.ft_slsize - r->rt.rt_len);
    }
}


/* Print summary report header
*/
static void pr_hdr (void)
{
    time_t Clock;

    time(&Clock);
    vftprintf(ddlp_g.outfile, DB_TEXT("\f\n"));
    vftprintf(ddlp_g.outfile, DB_TEXT("%72s %2d\r"), DB_TEXT("Page"), ++page);

    vftprintf(ddlp_g.outfile,
        DB_TEXT("db.* %s, Summary of Database: %s\n"),
        DBSTAR_VERSION, ddlp_g.db_name);

    vftprintf(ddlp_g.outfile, DB_TEXT("%s\n"), vtctime(&Clock));
    lines = 0;
}


/* Free linked lists allocated from heap
*/
static void free_idlist (ID_INFO *list)
{
    ID_INFO *p, *next_id;

    for (p = list; p; p = next_id)
    {
        next_id = p->next_id;
        psp_freeMemory(p, 0);
    }
}

static void free_const_list (CONST_INFO *list)
{
    CONST_INFO *p, *next_const;

    for (p = list; p; p = next_const)
    {
        next_const = p->next_const;
        psp_freeMemory(p, 0);
    }
}

static void free_type_list (TYPE_INFO *list)
{
    TYPE_INFO *p, *next_type;

    for (p = list; p; p = next_type)
    {
        next_type = p->next_type;
        free_elems(p->elem);
        psp_freeMemory(p, 0);
    }
}

static void free_file_list (struct file_info *list)
{
    struct file_info *p, *next_file;

    for (p = list; p; p = next_file)
    {
        next_file = p->next_file;
        free_idlist(p->name_list);
        psp_freeMemory(p, 0);
    }
}

static void free_rec_list (struct record_info *list)
{
    struct record_info *p, *next_rec;

    for (p = list; p; p = next_rec)
    {
        next_rec = p->next_rec;
        psp_freeMemory(p, 0);
    }
}

static void free_field_list (struct field_info *list)
{
    struct field_info *p, *next_field;

    for (p = list; p; p = next_field)
    {
        next_field = p->next_field;
        if (p->object_info)
            psp_freeMemory(p->object_info, 0);

        psp_freeMemory(p, 0);
    }
}

static void free_key_list (struct ddlkey_info *list)
{
    struct ddlkey_info *p, *next_key;

    for (p = list; p; p = next_key)
    {
        next_key = p->next_key;
        psp_freeMemory(p, 0);
    }
}

static void free_set_list (struct set_info *list)
{
    struct set_info *p, *next_set;

    for (p = list; p; p = next_set)
    {
        next_set = p->next_set;
        psp_freeMemory(p, 0);
    }
}

static void free_member_list (struct member_info *list)
{
    struct member_info *p, *next_mbr;

    for (p = list; p; p = next_mbr)
    {
        next_mbr = p->next_mbr;
        psp_freeMemory(p, 0);
    }
}

static void free_sort_list (struct sort_info *list)
{
    struct sort_info *p, *next_sort;

    for (p = list; p; p = next_sort)
    {
        next_sort = p->next_sort;
        psp_freeMemory(p, 0);
    }
}

void free_lists (void)
{
    free_idlist(trlist);
    trlist = trlast = NULL;

    free_idlist(tslist);
    tslist = tslast = NULL;

    free_const_list(const_list);
    const_list = const_last = NULL;

    free_type_list(type_list);
    type_list = type_last = NULL;

    free_file_list(ddlp_g.file_list);
    ddlp_g.file_list = ddlp_g.file_last = NULL;

    free_rec_list(ddlp_g.rec_list);
    ddlp_g.rec_list = ddlp_g.last_rec = NULL;

    free_field_list(ddlp_g.field_list);
    ddlp_g.field_list = ddlp_g.last_fld = NULL;

    free_key_list(ddlp_g.key_list);
    ddlp_g.key_list = ddlp_g.last_key = NULL;

    free_set_list(ddlp_g.set_list);
    ddlp_g.set_list = ddlp_g.last_set = NULL;

    free_member_list(ddlp_g.mem_list);
    ddlp_g.mem_list = ddlp_g.last_mem = NULL;

    free_sort_list(ddlp_g.sort_list);
    ddlp_g.sort_list = ddlp_g.last_sort = NULL;
}


