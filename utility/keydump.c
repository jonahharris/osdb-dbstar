/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, keydump utility                                   *
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

    keydump.c - db.* key file formatted dump utility

    This utility will create a formatted dump of the contents of each
    node in a key file (named on the ddlp command line).

-----------------------------------------------------------------------*/

#define MOD keydump
#include "db.star.h"
#include "version.h"

#define KEYDUMP
#include "cnvfld.h"

#define MAXTEXT 1600


/* ********************** TYPE DEFINITIONS *************************** */

/* node number of root */
#define ROOT_ADDR (F_ADDR) 1


/* ********************** LOCAL FUNCTION DECLARATIONS **************** */

static void pr_keyfile_header(FILE_NO, DB_TASK *);
static int  pr_node(FILE_NO, F_ADDR, int *, int *, DB_BOOLEAN, DB_TASK *);
static void p_hex(char *, int);
static void usage(void);


/* List the valid key file names
*/
static void list_key_files(DB_TASK *task)
{
    int i;

    vftprintf(stderr, DB_TEXT("Valid key file names are:\n"));
    for (i = 0; i < task->size_ft; i++)
    {
        if (task->file_table[i].ft_type == 'k')
            vftprintf(stderr, DB_TEXT("\t%s\n"), task->file_table[i].ft_name);
    }
}


void EXTERNAL_FCN keydump_dberr(int errnum, DB_TCHAR *msg)
{
    vtprintf(DB_TEXT("\n*** Database error %d - %s\n"),
        errnum, msg);
    vtprintf(DB_TEXT("\terrno = %d\n"), errno);

}


int MAIN(int argc, DB_TCHAR *argv[])
{
    DB_TCHAR    dbfname[FILENMLEN];
    DB_TCHAR   *keyfile = NULL;
    DB_TCHAR   *dbname = NULL;
    int         i, status, num_keys = 0;
    int        *key_fld = NULL;
    int        *key_len = NULL;
    DB_BOOLEAN  hex = FALSE;
    FILE_NO     fno;
    F_ADDR      page, pz_next;
    DB_TASK    *task;
    SG         *sg = NULL;
#if defined(SAFEGARDE)
    DB_TCHAR   *cp;
    DB_TCHAR   *password;
    int         mode;
#endif

    vtprintf(DBSTAR_UTIL_DESC(DB_TEXT("Key File Dump")));

    /* process the options */
    if (argc < 2)
    {
        usage();
        return(1);
    }

    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] == DB_TEXT('-'))
        {
            switch (vtotlower(argv[i][1]))
            {
                case DB_TEXT('h'):
                    hex = TRUE;
                    break;

                case DB_TEXT('s'):
                    if (vtotlower(argv[i][2]) != DB_TEXT('g') || i == argc - 1)
                    {
                        usage();
                        return 1;
                    }

#if defined(SAFEGARDE)
                    if ((cp = vtstrchr(argv[++i], DB_TEXT(','))) != NULL)
                        *cp++ = DB_TEXT('\0');

                    if (cp)
                    {
                        if (vtstrcmp(argv[i], "low") == 0)
                            mode = LOW_ENC;
                        else if (vtstrcmp(argv[i], "med") == 0)
                            mode = MED_ENC;
                        else if (vtstrcmp(argv[i], "high") == 0)
                            mode = HIGH_ENC;
                        else
                        {
                            usage();
                            return 1;
                        }

                        password = cp;
                    }
                    else
                    {
                        mode = MED_ENC;
                        password = argv[i];
                    }

                    break;
#else
                    vftprintf(stderr, DB_TEXT("SafeGarde is not available in this version\n"));
                    return 1;
#endif

                default:
                    usage();
                    return(1);
            }
        }
        else if (dbname == NULL)
            dbname = argv[i];
        else
            keyfile = argv[i];
    }

    if (! dbname)
    {
        vftprintf(stderr, DB_TEXT("A database name must be supplied.\n"));
        usage();
        return(1);
    }

    if ((status = d_opentask(&task)) == S_OKAY)
    {
#if defined(SAFEGARDE)
        if (mode != NO_ENC && (sg = sg_create(mode, password)) == NULL)
        {
            vftprintf(stderr, DB_TEXT("Failed to create SafeGarde context\n"));
            return 1;
        }
#endif

        if ((status = d_set_dberr(keydump_dberr, task)) == S_OKAY)
        {
            if ((status = d_open_sg(dbname, DB_TEXT("o"), sg, task)) == S_OKAY)
            {
                /* make sure all components are present */
                if (! keyfile)
                {
                    vftprintf(stderr, DB_TEXT("A key file name must be supplied.\n"));
                    list_key_files(task);   /* database must be open for this */
                }
                else
                {
                    /* look up file table entry */
                    con_dbf(dbfname, keyfile, dbname,
                              get_element(task->dbfpath, 0), task);

                    for (fno = 0; fno < task->size_ft; ++fno)
                    {
                        if (psp_fileNameCmp(task->file_table[fno].ft_name, dbfname) == 0)
                            break;
                    }
                    if (fno == task->size_ft)
                    {
                        vftprintf(stderr, DB_TEXT("File '%s' not found in database %s\n"),
                                     keyfile, dbname);
                        list_key_files(task);
                    }
                    else
                    {
                        if (task->file_table[fno].ft_type != 'k')
                        {
                            vftprintf(stderr, DB_TEXT("File '%s' is not a key file\n"),
                                         keyfile);
                            list_key_files(task);
                        }
                        else
                        {
                            /* build arrays of key indexes in fld table, and sizes */
                            for (i = 0; i < (int) task->size_fd; ++i)
                            {
                                if (task->field_table[i].fd_key != NOKEY)
                                    ++num_keys;
                            }
                            key_len = (int *) psp_cGetMemory(num_keys *
                                    sizeof(int), 0);
                            key_fld = (int *) psp_cGetMemory(num_keys *
                                    sizeof(int), 0);
                            if (!key_len || !key_fld)
                            {
                                vftprintf(stderr, DB_TEXT("unable to allocate memory for keys\n"));
                            }
                            else
                            {
                                for (i = 0; i < (int) task->size_fd; ++i)
                                {
                                    if (task->field_table[i].fd_key != NOKEY)
                                    {
                                        key_len[task->field_table[i].fd_keyno] = task->field_table[i].fd_len;
                                        key_fld[task->field_table[i].fd_keyno] = i;
                                    }
                                }

                                /* find out how many nodes there are, and dump them */
                                if ((status = dio_pzread(fno, task)) == S_OKAY)
                                {
                                    pz_next = task->pgzero[fno].pz_next;
                                    pr_keyfile_header(fno, task);

                                    /* print each node */
                                    for (page = ROOT_ADDR; page < pz_next; ++page)
                                    {
                                        status = pr_node(fno, page, key_fld, key_len,
                                                              hex, task);
                                        if (status != S_OKAY)
                                            break;
                                    }
                                }
                                psp_freeMemory(key_len, 0);
                                psp_freeMemory(key_fld, 0);
                            }
                        }
                    }
                }
                d_close(task);
            }
        }
        d_closetask(task);
    }
    if (status != S_OKAY)
    {
        switch (status)
        {
            case S_INVDB:
                vftprintf(stderr, DB_TEXT("Unable to locate database: %s\n"), dbname);
                break;

            case S_INCOMPAT:
                vftprintf(stderr, DB_TEXT("Incompatible dictionary file. Re-run ddlp.\n"));
                break;

            default:
                vftprintf(stderr, DB_TEXT("Error %d opening database.\n"), status);
                break;
        }
        return(1);
    }
    return(0);
}

/* Print key dump header info
*/
static void pr_keyfile_header(FILE_NO fno, DB_TASK *task)
{
    DB_TCHAR buf[80];

    d_dbver(DB_TEXT("%V"), buf, sizeof(buf) / sizeof(DB_TCHAR));
    vtprintf(DB_TEXT("%s\n"), buf);
    vtprintf(DB_TEXT("Key File Dump Utility\n"));
    d_dbver(DB_TEXT("%C"), buf, sizeof(buf) / sizeof(DB_TCHAR));
    vtprintf(DB_TEXT("%s\n"), buf);
    vtprintf(DB_TEXT("------------------------------------"));
    vtprintf(DB_TEXT("------------------------------------\n"));
    vtprintf(DB_TEXT("KEY FILE      : %s\n"), task->file_table[fno].ft_name);
    vtprintf(DB_TEXT("DATABASE      : %s\n"), task->curr_db_table->db_name);
    vtprintf(DB_TEXT("PAGE SIZE     : %d\n"), (int) task->file_table[fno].ft_pgsize);
    vtprintf(DB_TEXT("SLOT SIZE     : %d\n"), (int) task->file_table[fno].ft_slsize);
    vtprintf(DB_TEXT("SLOTS PER PAGE: %d\n"), (int) task->file_table[fno].ft_slots);
    vtprintf(DB_TEXT("NEXT PAGE     : %ld (%08lx)\n"), task->pgzero[fno].pz_next, task->pgzero[fno].pz_next);
    vtprintf(DB_TEXT("DELETE CHAIN  : %ld (%08lx)\n"), task->pgzero[fno].pz_dchain, task->pgzero[fno].pz_dchain);
    vtprintf(DB_TEXT("------------------------------------"));
    vtprintf(DB_TEXT("------------------------------------\n\n\n"));
}


/* Print node
*/
static int pr_node(
    FILE_NO fno,
    F_ADDR page,
    int *key_fld,
    int *key_len,
    DB_BOOLEAN hex,
    DB_TASK *task)
{
    NODE       *node;
    KEY_TYPE    key;
    F_ADDR      orphan;
    DB_ADDR     dba;
    int         slot, fld, kt, status;
    int         slot_len = task->file_table[fno].ft_slsize;
    DB_TCHAR    text[MAXTEXT];

    status = dio_get(fno, page, (char **) &node, NOPGHOLD, task);
    if (status != S_OKAY)
        return status;

    if (node->used_slots == 0)
    {
        memcpy(&orphan, node->slots, sizeof(F_ADDR));
        vtprintf(DB_TEXT("NODE : %ld (%08lx)  DELETED,  NEXT NODE: %ld (%08lx)\n\n\n"),
                 page, page, orphan, orphan);
    }
    else
    {
        vtprintf(DB_TEXT("NODE : %ld (%08lx)\n"), page, page);
        vtprintf(DB_TEXT("SLOTS: %d\n"), node->used_slots);
        vtprintf(DB_TEXT("TIME : %s"), vtctime(&(node->last_chgd)));
        for (slot = 0; slot < node->used_slots; ++slot)
        {
            memcpy(&key, node->slots + slot * slot_len, slot_len);
            memcpy(&dba, key.ks.data + key_len[key.ks.keyno], sizeof(DB_ADDR));
            vtprintf(DB_TEXT("SLOT:%3d  CHILD: %6ld (%08lx)  PREFIX: %3d  "),
                     slot, key.ks.child, key.ks.child, key.ks.keyno);
            vtprintf(DB_TEXT("LENGTH: %3d  DB_ADDR: %08lx"),
                     key_len[key.ks.keyno], dba);
            fld = key_fld[key.ks.keyno];
            if (hex)
            {
                if (task->field_table[fld].fd_type == COMKEY)
                {
                    kt = task->field_table[fld].fd_ptr;
                    while (task->key_table[kt].kt_key == fld)
                    {
                        p_hex(key.ks.data + task->key_table[kt].kt_ptr,
                              task->field_table[task->key_table[kt].kt_field].fd_len);
                        kt++;
                    }
                    vtprintf(DB_TEXT("\n"));
                }
                else
                {
                    p_hex(key.ks.data, key_len[key.ks.keyno]);
                    vtprintf(DB_TEXT("\n"));
                }
            }
            else
            {
                if (task->field_table[fld].fd_type == COMKEY)
                {
                    kt = task->field_table[fld].fd_ptr;
                    while (task->key_table[kt].kt_key == fld)
                    {
                        fldtotxt(task->key_table[kt].kt_field,
                                 key.ks.data + task->key_table[kt].kt_ptr,
                                 text, 0, task);
                        vtprintf(DB_TEXT("\n          %s"), text);
                        kt++;
                    }
                    vtprintf(DB_TEXT("\n"));
                }
                else
                {
                    fldtotxt(fld, key.ks.data, text, 0, task);
                    vtprintf(DB_TEXT("\n          %s\n"), text);
                }
            }
        }
        memcpy(&orphan, node->slots + slot * slot_len, sizeof(F_ADDR));
        vtprintf(DB_TEXT("SLOT:%3d  CHILD: %6ld (%08lx)\n\n\n"), slot, orphan, orphan);
    }
    return(0);
}


static void p_hex(char *s, int len)
{
    char ch;
    int i;

    for (i = 0; i < len; ++i)
    {
        if (i % 36 == 0)
            vtprintf(DB_TEXT("\n          "));
        ch = s[i];
        vtprintf(DB_TEXT("%02x "), 0xFF & ch);
    }
}


static void usage()
{
    vftprintf(stderr, DB_TEXT("usage: keydump [-h] dbname keyfile\n"));
    vftprintf(stderr, DB_TEXT("   where   -h prints fields in hexidecimal notation\n"));
    vftprintf(stderr, DB_TEXT("Default is formatted field output.\n"));
    vftprintf(stderr, DB_TEXT("To see list of key files, omit keyfile from command ddlp_g.line.\n"));
}


VXSTARTUP("keydump", 0)

