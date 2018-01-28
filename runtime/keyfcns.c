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

/*---------------------------------------------------------------------------

    db.* Key File/Field Manipulation Functions
    -----------------------------------------
    An implementation of the B-tree indexing method  described in
    "Sorting and Searching: The Art of Computer Programming, Vol III",
    Knuth, Donald E., Addison-Wesley, 1975. pp 473-480.

    A more detailed description of the generic algorithms can be found
    in "Fundamentals of Data Structures in Pascal", Horowitz & Sahni,
    Computer Science Press, 1984. pp 491-512.

    A tutorial survey of B-tree methods can be found in "The Ubiquitous
    B-Tree", Comer, D., ACM Computing Surveys, Vol 11, No. 2, June 1979.

---------------------------------------------------------------------------*/

#include "db.star.h"

/* Internal function prototypes */
static int  INTERNAL_FCN node_search(const char *, DB_ADDR *, NODE *, int *, int *, F_ADDR *, DB_TASK *);
static int  INTERNAL_FCN keycmp(const char *, KEY_SLOT *, DB_ADDR *, DB_TASK *);
static int  INTERNAL_FCN expand(const char *, DB_ADDR, F_ADDR, DB_TASK *);
static int  INTERNAL_FCN split_root(NODE *, DB_TASK *);
static int  INTERNAL_FCN split_node(F_ADDR, NODE *, DB_TASK *);
static int  INTERNAL_FCN delete(DB_TASK *);
static void INTERNAL_FCN open_slots(NODE *, int, int, DB_TASK *);
static void INTERNAL_FCN close_slots(NODE *, int, int, DB_TASK *);
static void INTERNAL_FCN key_found(DB_ADDR *, DB_TASK *);

/* Number of used slots plus orphan */
#define NODE_WIDTH(node)      ((node)->used_slots*task->slot_len + sizeof(F_ADDR))

/* Data definitions used for the B-tree algorithms */
#define ROOT_ADDR    1L             /* node number of root */
#define NULL_NODE    -1L            /* null node pointer */

/* last status value */
#define KEYEOF       -1
#define KEYINIT      0
#define KEYFOUND     1
#define KEYNOTFOUND  2
#define KEYREPOS     3

/* ======================================================================
    Open B-tree key field index processing
*/
int EXTERNAL_FCN key_open(DB_TASK *task)
{
    int           fd_lc;            /* loop control */
    F_ADDR        t;                /* total keys thru level l */
    F_ADDR        max;
    short         l;                /* level number */
    short         i;                /* field subscript */
    FIELD_ENTRY  *fld_ptr;
    KEY_INFO     *ki_ptr;
    FILE_ENTRY   *file_ptr;
    int           old_no_of_keys;

    /* child ptr      key number   */
    task->key_data = sizeof(F_ADDR) + sizeof(short);

    /* count total number of key fields */
    old_no_of_keys = task->no_of_keys;
    for (fd_lc = task->size_fd - task->old_size_fd,
         fld_ptr = &task->field_table[task->old_size_fd];
         --fd_lc >= 0; ++fld_ptr)
    {
        if (fld_ptr->fd_key != NOKEY)
            ++task->no_of_keys;
    }

    if (task->no_of_keys)
    {
        if (alloc_table((void **) &task->key_info, task->no_of_keys *
            sizeof(KEY_INFO), old_no_of_keys * sizeof(KEY_INFO),
            task) != S_OKAY)
            return task->db_status;

        for (i = task->old_size_fd,
             fld_ptr = &task->field_table[task->old_size_fd];
             i < task->size_fd; ++i, ++fld_ptr)
        {
            if (fld_ptr->fd_key != NOKEY)
            {
                ki_ptr = &task->key_info[fld_ptr->fd_keyno];
                ki_ptr->level = 0;
                ki_ptr->lstat = KEYINIT;
                ki_ptr->fldno = i;
                ki_ptr->keyfile = fld_ptr->fd_keyfile;
                ki_ptr->dba = NULL_DBA;
                file_ptr = &task->file_table[fld_ptr->fd_keyfile];
                ki_ptr->keyval = psp_getMemory(file_ptr->ft_slsize, 0);
                if (!ki_ptr->keyval)
                    return (dberr(S_NOMEMORY));

                /* compute maximum possible levels */
                max = MAXRECORDS / file_ptr->ft_slots;
                for (t = file_ptr->ft_slots, l = 1; t < MAXRECORDS; ++l)
                {
                    /* test whether overflow would occur on next iteration */
                    if (t > max)
                        t = MAXRECORDS;
                    else
                        t *= file_ptr->ft_slots;
                }

                ki_ptr->max_lvls = ++l;
                ki_ptr->node_path = (NODE_PATH *) psp_getMemory(l * sizeof(NODE_PATH), 0);

                if (!ki_ptr->node_path)
                    return (dberr(S_NOMEMORY));

                memset(ki_ptr->node_path, 0, l * sizeof(NODE_PATH));
            }
        }
    }        /* end of if (task->no_of_keys) */

    return (task->db_status);
}



/* ======================================================================
    Close key field processing
*/
void INTERNAL_FCN key_close(int dbn, DB_TASK *task)
{
    int           k;
    KEY_INFO     *ki_ptr;
    int           fd_lc;
    int           low, high, total;
    FIELD_ENTRY  *fld_ptr;

    if (task->key_info)
    {
        /* free memory allocated for key functions */
        if (dbn == ALL_DBS)
        {
            for (k = 0, ki_ptr = task->key_info; k < task->no_of_keys; ++k,
                    ++ki_ptr)
            {
                psp_freeMemory(ki_ptr->node_path, 0);
                psp_freeMemory(ki_ptr->keyval, 0);
            }

            psp_freeMemory(task->key_info, 0);

        }
        else
        {
            for ( fd_lc = low = high = 0, fld_ptr = task->field_table;
                  fd_lc < DB_REF(Size_fd) + DB_REF(fd_offset);
                  ++fd_lc, ++fld_ptr)
            {
                if (fld_ptr->fd_key != NOKEY)
                {
                    if (fd_lc < DB_REF(fd_offset))
                        low++;
                    else
                        high++;
                }
            }

            high += low;
            total = task->no_of_keys;

            for (k = low, ki_ptr = &task->key_info[low]; k < high; ++k, ++ki_ptr)
            {
                psp_freeMemory(ki_ptr->node_path, 0);
                psp_freeMemory(ki_ptr->keyval, 0);
            }

            free_table((void **) &task->key_info, low, high, total,
                       sizeof(KEY_INFO), task);
            task->no_of_keys -= (high - low);

            for (k = low, ki_ptr = &task->key_info[low]; k < task->no_of_keys;
                 ++k, ++ki_ptr)
            {
                ki_ptr->keyfile -= task->db_table[dbn].Size_ft;
                ki_ptr->fldno -= task->db_table[dbn].Size_fd;
            }
        }
    }
}


/* ======================================================================
    Initialize key function operation
*/
int EXTERNAL_FCN key_init(int field, DB_TASK *task)
{
    FIELD_ENTRY  *fld_ptr;
    FILE_ENTRY   *file_ptr;

    fld_ptr = &task->field_table[field];

    if (fld_ptr->fd_key == NOKEY)
        return (dberr(S_NOTKEY));

    task->fldno       = (short) field;
    task->cfld_ptr    = fld_ptr;
    task->keyno       = fld_ptr->fd_keyno;
    task->prefix      = (short) (task->keyno - task->curr_db_table->key_offset);
    task->key_len     = fld_ptr->fd_len;
    task->keyfile     = fld_ptr->fd_keyfile;
    file_ptr          = &task->file_table[task->keyfile];
    task->slot_len    = file_ptr->ft_slsize;
    task->max_slots   = file_ptr->ft_slots;
    task->mid_slot    = (short) (task->max_slots / 2);
    task->curkey      = &task->key_info[task->keyno];
    task->unique      = (fld_ptr->fd_key == UNIQUE);
    task->last_keyfld = task->fldno;

    return (task->db_status);
}



/* ======================================================================
    Reset task->key_info last status to reposition keys on file "fno"
*/
int INTERNAL_FCN key_reset(FILE_NO fno, DB_TASK *task)
{
    int        i;
    KEY_INFO  *ki_ptr;

    for (i = 0, ki_ptr = task->key_info; i < task->no_of_keys; ++i, ++ki_ptr)
    {
        if ((fno == task->size_ft || ki_ptr->keyfile == fno) &&
            (ki_ptr->lstat == KEYFOUND || ki_ptr->lstat == KEYNOTFOUND))
            ki_ptr->lstat = KEYREPOS;
    }

    return (task->db_status);
}



/* ======================================================================
    Locate proper key position on B-tree
*/
int EXTERNAL_FCN key_locpos(
    const char *key_val,         /* key search value */
    DB_ADDR    *dba,             /* database address of located key */
    DB_TASK    *task)
{
    NODE      *node;             /* pointer to current node */
    F_ADDR     child;            /* page number of child node */
    F_ADDR     pg;               /* page number of current node */
    F_ADDR     last_page = 0L;   /* last page number */
    int        stat;             /* saves node search status */
    int        slot, slot_pos;
    short      match_lvl;        /* lowest level with duplicate key */
    NODE_PATH *np_ptr;
    char      *node_slot_ptr;

    match_lvl = -1;

    for (task->curkey->level = 0, np_ptr = task->curkey->node_path,
         pg = ROOT_ADDR; ; ++task->curkey->level, ++np_ptr, pg = child)
    {
        /* sanity check for the b-tree being malformed */
        if (task->curkey->level > task->curkey->max_lvls)
            return (dberr(SYS_BADTREE));

        /* read in next node */
        if (last_page)
            dio_unget(task->keyfile, last_page, NOPGFREE, task);

        if (dio_get(task->keyfile, (last_page = pg), (char **) &node, NOPGHOLD,
                    task) != S_OKAY)
            return task->db_status;

        np_ptr->node = pg;
        if (node->used_slots == 0)
        {
            np_ptr->slot = 0;
            task->curkey->lstat = KEYEOF;
            dio_unget(task->keyfile, pg, NOPGFREE, task);

            return (task->db_status = S_NOTFOUND);
        }

        /* search node for key */
        stat = node_search(key_val, ((*dba == NULL_DBA) ? NULL : dba), node,
                           &slot, &slot_pos, &child, task);
        np_ptr->slot = (short) slot;

        node_slot_ptr = &node->slots[slot_pos];
        if (stat == S_OKAY)
        {
            if (task->unique || *dba != NULL_DBA)
                break;

            /* mark level as having matching key */
            match_lvl = task->curkey->level;

            /* save the key value */
            memcpy(&task->key_type, node_slot_ptr, task->slot_len);
        }

        /* if node_search failed, try next level in B-tree - reset db_status */
        if (task->db_status == S_NOTFOUND)
            task->db_status = S_OKAY;

        /* check for end of search */
        if (child == NULL_NODE)
            break;
    }

    if (match_lvl >= 0)
    {
        /* set to lowest duplicate key */
        task->curkey->level = match_lvl;
        stat = S_OKAY;
        task->curkey->lstat = KEYFOUND;
    }
    else if (stat == S_OKAY)
    {
        memcpy(&task->key_type, node_slot_ptr, task->slot_len);
        stat = S_OKAY;
        task->curkey->lstat = KEYFOUND;
    }
    else
    {
        /* key not found -- save key data at a positioned slot.  Note that
           the key data from the "previous" slot is saved but the slot
           number saved is still the "next" key.
        */
        if (np_ptr->slot > 0)
            node_slot_ptr -= task->slot_len;

        memcpy(&task->key_type, node_slot_ptr, task->slot_len);
        task->curkey->lstat = KEYNOTFOUND;
        stat = S_NOTFOUND;
    }

    key_found(*dba == NULL_DBA ? dba : NULL, task);   /* whether exact match or not */

    dio_unget(task->keyfile, last_page, NOPGFREE, task);

    return task->db_status = stat;
}



/* ======================================================================
    Search node for key value
*/
static int INTERNAL_FCN node_search(
    const char   *key_val,      /* key being searched */
    DB_ADDR      *dba,          /* dbaddr included in comparison if not null */
    NODE         *node,         /* node being searched */
    int          *slotno,       /* slot number of key position in node */
    int          *slot_offset,  /* slot position offset */
    F_ADDR       *child,        /* child ptr of located key */
    DB_TASK *task)
{
    int     cmp, i, l, u, slot_pos;
    char   *node_slot_ptr;

    /* perform binary search on node */
    l = 0;
    u = node->used_slots - 1;

    do
    {
        i = (l + u) / 2;
        slot_pos = i * task->slot_len;
        node_slot_ptr = &node->slots[slot_pos];
        cmp = keycmp(key_val, (KEY_SLOT *) node_slot_ptr, dba, task);
        if (cmp < 0)
            u = i - 1;
        else if (cmp > 0)
            l = i + 1;
        else if (i && !task->unique && !dba)
        {
            /* backup to lowest duplicate in node */
            while (keycmp(key_val, (KEY_SLOT *) (node_slot_ptr -= task->slot_len),
                   dba, task) == 0)
            {
                slot_pos -= task->slot_len;
                if (--i == 0)
                    goto have_slot;
            }

            node_slot_ptr += task->slot_len;
            goto have_slot;
        }
        else
            goto have_slot;

    } while (u >= l);

have_slot:
    if (cmp > 0)
    {
        /* always return next higher slot when not found */
        ++i;
        node_slot_ptr += task->slot_len;
        slot_pos += task->slot_len;
    }

    /* return child pointer from located slot */
    memcpy(child, node_slot_ptr, sizeof(F_ADDR));

    /* return slot number */
    *slotno = i;
    *slot_offset = slot_pos;

    return task->db_status = (cmp == 0 ? S_OKAY : S_NOTFOUND);
}



/* ======================================================================
    Compare key value
*/
static int INTERNAL_FCN keycmp(
    const char *key_val, /* key value */
    KEY_SLOT *slot,      /* pointer to key slot to be compared */
    DB_ADDR *dba,        /* dbaddr included in comparison if not null */
    DB_TASK *task)
{
/*
    returns < 0 if key_val < slot
            > 0 if key_val > slot
            = 0 if key_val == slot
*/
    int cmp;

    if ((cmp = SHORTcmp((char *) &task->prefix, (char *) &slot->keyno)) == 0 &&
        (cmp = fldcmp(task->cfld_ptr, key_val, slot->data, task)) == 0 && dba)
        cmp = ADDRcmp(dba, (DB_ADDR *) &slot->data[task->key_len]);

    return cmp;
}


/* ======================================================================
    Scan thru key field
*/
int EXTERNAL_FCN key_scan(
    int fcn,                        /* next or prev */
    DB_ADDR *dba,                   /* db address of scanned record */
    DB_TASK *task)
{
    F_ADDR     child;
    F_ADDR     last_page;
    int        stat;
    NODE      *node;
    NODE_PATH *np_ptr;
    char      *node_slot_ptr;

    /* If the last d_keyfind() returned S_NOTFOUND, the slot number
       associated with task->curkey is for the "next" key while the keyval
       is for the "previous" key!
    */

    /* locate next key on file */
    switch (task->curkey->lstat)
    {
        case KEYINIT:
        case KEYEOF:
            return key_boundary(((fcn == KEYNEXT) ? KEYFRST : KEYLAST), dba, task);
            /* break; */

        case KEYREPOS:
            /* Note that if (dkeyfind() == S_NOTFOUND) then this call to
               key_locpos() will reset the keyval AND slot to the "previous"
               key.  See comment at start of function.
            */
            stat = key_locpos(task->curkey->keyval, &task->curkey->dba, task);
            if (stat != S_OKAY)
                break;
            /* else FALL THROUGH */

        case KEYFOUND:
            if (fcn == KEYNEXT)
                ++task->curkey->node_path[task->curkey->level].slot;

            break;

        default:
            break;
    }

    np_ptr = &task->curkey->node_path[task->curkey->level];
    if (dio_get(task->keyfile, (last_page = np_ptr->node), (char **) &node,
                NOPGHOLD, task) != S_OKAY)
        return task->db_status;

    node_slot_ptr = &node->slots[np_ptr->slot * task->slot_len];
    memcpy(&child, node_slot_ptr, sizeof(F_ADDR));
    if (child == NULL_NODE)
    {
        if (fcn == KEYPREV)
        {
            --np_ptr->slot;
            node_slot_ptr -= task->slot_len;
        }

        while ((fcn == KEYNEXT && np_ptr->slot >= node->used_slots) ||
               (fcn == KEYPREV && np_ptr->slot < 0))
        {
            if (task->curkey->level <= 0)
            {
                /* return end of file */
                task->curkey->lstat = KEYEOF;
                dio_unget(task->keyfile, last_page, NOPGFREE, task);
                return (task->db_status = S_NOTFOUND);
            }

            --task->curkey->level;
            node_slot_ptr = NULL;
            dio_unget(task->keyfile, last_page, NOPGFREE, task);

            if (dio_get(task->keyfile, (last_page = (--np_ptr)->node),
                        (char **) &node, NOPGHOLD, task) != S_OKAY)
            {
                dio_unget(task->keyfile, last_page, NOPGFREE, task);
                return task->db_status;
            }

            if (fcn == KEYPREV)
                --np_ptr->slot;
        }

        if (node_slot_ptr == NULL)
            node_slot_ptr = &node->slots[np_ptr->slot * task->slot_len];
    }
    else
    {
        do
        {                                /* move down to leaf node */
            dio_unget(task->keyfile, last_page, NOPGFREE, task);
            if (dio_get(task->keyfile, (last_page = child), (char **) &node,
                        NOPGHOLD, task) != S_OKAY)
                return task->db_status;

            ++task->curkey->level;
            (++np_ptr)->node = child;
            if (fcn == KEYNEXT)
            {
                np_ptr->slot = 0;
                node_slot_ptr = node->slots;
            }
            else
            {
                np_ptr->slot = node->used_slots;
                node_slot_ptr = &node->slots[np_ptr->slot * task->slot_len];
            }

            memcpy(&child, node_slot_ptr, sizeof(F_ADDR));
        } while (child != NULL_NODE);
    }

    if (np_ptr->slot == node->used_slots)
    {
        --np_ptr->slot;
        node_slot_ptr -= task->slot_len;
    }

    memcpy(&task->key_type, node_slot_ptr, task->slot_len);
    if (task->key_type.ks.keyno == task->prefix)
    {
        key_found(dba, task);
        task->curkey->lstat = KEYFOUND;
        stat = S_OKAY;
    }
    else
    {
        task->curkey->lstat = KEYEOF;
        stat = S_NOTFOUND;
    }

    dio_unget(task->keyfile, last_page, NOPGFREE, task);

    return task->db_status = stat;
}


/* ======================================================================
    Key has been found.  Save appropriate information
*/
static void INTERNAL_FCN key_found(DB_ADDR *dba, DB_TASK *task)
{
    /* save key value and database address for possible repositioning */
    memcpy(task->curkey->keyval, task->key_type.ks.data, task->key_len);
    memcpy(&task->curkey->dba, &task->key_type.ks.data[task->key_len], sizeof(DB_ADDR));

    /* return found db addr */
    if (dba)
        memcpy(dba, &task->curkey->dba, sizeof(DB_ADDR));
}


/* ======================================================================
    Find key boundary
*/
int EXTERNAL_FCN key_boundary(
    int      fcn,                /* KEYFRST or KEYLAST */
    DB_ADDR *dba,                /* to get dba of first or last key */
    DB_TASK *task)
{
    F_ADDR     pg;               /* node number */
    F_ADDR     last_page = 0L;   /* last page obtained with a dio_get */
    NODE      *node;             /* pointer to node contents in cache */
    int        cmp = 0;          /* keycmp result */
    short      match_lvl;        /* lowest level containing matched key */
    short      lvl;              /* node_path level variable */
    int        slot;             /* slot position in node */
    int        stat;
    NODE_PATH *np_ptr;
    char      *node_slot_ptr;

    if (fcn == KEYFIND)
    {
        task->curkey->lstat = KEYINIT;
        return (task->db_status);
    }

    task->curkey->lstat = KEYNOTFOUND;

    /* traverse B-tree for first or last key with specified prefix */
    match_lvl = -1;
    pg = ROOT_ADDR;

    for (lvl = 0; ; ++lvl)
    {
        /* read next node */
        if (last_page)
            dio_unget(task->keyfile, last_page, NOPGFREE, task);

        if (dio_get(task->keyfile, (last_page = pg),
                    (char **) &node, NOPGHOLD, task) != S_OKAY)
            return task->db_status;

        if (node->used_slots == 0)
        {
            /* must not be anything on file */
            task->curkey->lstat = KEYEOF;
            dio_unget(task->keyfile, last_page, NOPGFREE, task);

            return (task->db_status = S_NOTFOUND);
        }

        if (fcn == KEYFRST)
        {
            for (slot = 0, node_slot_ptr = node->slots; slot < node->used_slots;
                    ++slot, node_slot_ptr += task->slot_len)
            {
                if ((cmp = SHORTcmp((char *) &task->prefix,
                    (char *) (node_slot_ptr + sizeof(F_ADDR)))) <= 0)
                    break;
            }
        }
        else
        {
            /* KEYLAST */
            for (slot = node->used_slots - 1,
                 node_slot_ptr = &node->slots[slot * task->slot_len]; slot >= 0;
                 --slot, node_slot_ptr -= task->slot_len)
            {
                if ((cmp = SHORTcmp((char *) &task->prefix,
                    (char *) (node_slot_ptr + sizeof(F_ADDR)))) >= 0)
                    break;
            }
        }

        /* save node path position */
        np_ptr = &task->curkey->node_path[lvl];
        np_ptr->node = pg;
        np_ptr->slot = (short) slot;

        if (cmp == 0)
        {
            /* save matched level & key value */
            match_lvl = lvl;
            memcpy(&task->key_type, node_slot_ptr, task->slot_len);
        }

        /* fetch appropriate child pointer */
        if (fcn == KEYLAST)
            node_slot_ptr += task->slot_len;

        memcpy(&pg, node_slot_ptr, sizeof(F_ADDR));
        if (pg == NULL_NODE)
            break;
    }

    if (match_lvl >= 0)
    {
        task->curkey->level = match_lvl;
        key_found(dba, task);
        task->curkey->lstat = KEYREPOS;
        stat = S_OKAY;
    }
    else
    {
        /* no keys on file with requested prefix */
        task->curkey->level = 0;
        task->curkey->lstat = KEYEOF;
        stat = S_NOTFOUND;
    }

    dio_unget(task->keyfile, last_page, NOPGFREE, task);

    return task->db_status = stat;
}


/* ======================================================================
    Insert key field into B-tree
*/
int EXTERNAL_FCN key_insert(
    int         fld,                /* key field number */
    const char *key_val,            /* key value */
    DB_ADDR     dba,                /* record's database address */
    DB_TASK    *task)
{
    int stat;

    /* initialize key function operation */
    if (key_init(fld, task) != S_OKAY)
        return task->db_status;

    /* locate insertion point */
    if (key_locpos(key_val, &dba, task) == S_NOTFOUND)
    {
        task->db_status = S_OKAY;

        /* expand node to include key */
        if ((stat = expand(key_val, dba, NULL_NODE, task)) == S_OKAY)
        {
            /* save key value and database address for possible repositioning */
            FIELD_ENTRY *kfld_ptr = &task->field_table[fld];
            if (kfld_ptr->fd_type == CHARACTER && kfld_ptr->fd_dim[1] == 0)
                strncpy(task->curkey->keyval, key_val, task->key_len);
            else if (kfld_ptr->fd_type == WIDECHAR && kfld_ptr->fd_dim[1] == 0)
            {
                vwcsncpy((wchar_t *)task->curkey->keyval, (wchar_t *)key_val,
                        task->key_len / sizeof(wchar_t));
            }
            else
                memcpy(task->curkey->keyval, key_val, task->key_len);

            task->curkey->dba = dba;

            /* reset key position */
            key_reset(task->curkey->keyfile, task);
        }

        task->db_status = stat;
    }
    else
    {
        if (task->db_status == S_OKAY)
            dberr(SYS_KEYEXIST);
    }

    return task->db_status;
}



/* ======================================================================
    Expand node for new key
*/
static int INTERNAL_FCN expand(
    const char *key_val,            /* key value */
    DB_ADDR     dba,                /* record's database address */
    F_ADDR      sibling,            /* page number of sibling node */
    DB_TASK    *task)
{
    F_ADDR     pg;
    int        slot_pos;
    NODE      *node;
    NODE_PATH *np_ptr;
    char      *node_slot_ptr;

    np_ptr = &task->curkey->node_path[task->curkey->level];

    if (dio_get(task->keyfile, pg = np_ptr->node,
                (char **) &node, PGHOLD, task) != S_OKAY)
        return task->db_status;

    if (node->used_slots >= task->max_slots)
    {
        dio_unget(task->keyfile, pg, PGFREE, task);
        return (dberr(S_KEYERR));
    }

    node_slot_ptr = &node->slots[slot_pos = np_ptr->slot * task->slot_len];
    open_slots(node, slot_pos, 1, task);

    /* copy sibling into opened slot's child pointer */
    memcpy(node_slot_ptr + task->slot_len, &sibling, sizeof(F_ADDR));

    /* copy keyno into current slot */
    memcpy(node_slot_ptr + sizeof(F_ADDR), &task->prefix, sizeof(short));

    node_slot_ptr += task->key_data;

    /* clear slot data area to zeros */
    memset(node_slot_ptr, 0, task->slot_len - task->key_data);

    /* copy keyval into current slot */
    if (task->cfld_ptr->fd_type == CHARACTER && task->cfld_ptr->fd_dim[1] == 0)
        strncpy(node_slot_ptr, key_val, task->key_len);
    else if (task->cfld_ptr->fd_type == WIDECHAR && task->cfld_ptr->fd_dim[1] == 0)
        vwcsncpy((wchar_t *)node_slot_ptr, (wchar_t *)key_val, task->key_len / sizeof(wchar_t));
    else
        memcpy(node_slot_ptr, key_val, task->key_len);

    /* copy database address into current slot */
    memcpy(node_slot_ptr + task->key_len, &dba, sizeof(DB_ADDR));

    if (node->used_slots == task->max_slots)
    {
        if (pg == ROOT_ADDR)
            split_root(node, task);
        else
            split_node(pg, node, task);
    }
    else
        dio_touch(task->keyfile, pg, PGFREE, task);

    return task->db_status;
}



/* ======================================================================
    Split node into two nodes
*/
static int INTERNAL_FCN split_node(
    F_ADDR   l_pg,                     /* left node's page number */
    NODE    *l_node,                   /* left node buffer */
    DB_TASK *task)
{
    F_ADDR        r_pg;
    DB_ADDR       dba;
    char         *key_val;
    NODE         *r_node;
    char         *l_node_slot_ptr;

    /* Save state of all global variables used in this (potentially) */
    /* recursively invoked procedure.                                */
    int           save_key_len;
    short         save_fldno;
    short         save_keyno;
    FIELD_ENTRY  *save_cfld_ptr;
    short         save_prefix;

    key_val = psp_getMemory(MAXKEYSIZE, 0);
    if (key_val == NULL)
        return (dberr(S_NOMEMORY));

    save_key_len = task->key_len;
    save_fldno = task->fldno;
    save_cfld_ptr = task->cfld_ptr;
    save_keyno = task->keyno;
    save_prefix = task->prefix;

    /* extract middle key */
    l_node_slot_ptr = &l_node->slots[task->mid_slot * task->slot_len];
    memcpy(&task->prefix, l_node_slot_ptr + sizeof(F_ADDR), sizeof(short));
    task->keyno = (short) (task->prefix + task->curr_db_table->key_offset);
    task->fldno = task->key_info[task->keyno].fldno;
    task->cfld_ptr = &task->field_table[task->fldno];
    task->key_len = task->cfld_ptr->fd_len;
    memcpy(key_val, l_node_slot_ptr + task->key_data, task->key_len);
    memcpy(&dba, l_node_slot_ptr + task->key_data + task->key_len, sizeof(DB_ADDR));

    /* divide left node */
    l_node->used_slots = task->mid_slot;

    /* allocate new right node */
    if ((dio_pzalloc(task->keyfile, &r_pg, task) == S_OKAY) &&
        (dio_get(task->keyfile, r_pg, (char **) &r_node, PGHOLD, task) == S_OKAY))
    {
        /* copy slots from left node at slot task->mid_slot+1 into right node */
        r_node->used_slots = (short) (task->max_slots - (task->mid_slot + 1));
        l_node_slot_ptr += task->slot_len;
        memcpy(r_node->slots, l_node_slot_ptr, NODE_WIDTH(r_node));

        dio_touch(task->keyfile, l_pg, PGFREE, task);
        dio_touch(task->keyfile, r_pg, PGFREE, task);

        --task->curkey->level;

        /* expand parent slot to include middle key and new right node ptr */
        expand(key_val, dba, r_pg, task);
    }
    
    psp_freeMemory(key_val, 0);
    task->key_len = save_key_len;
    task->fldno = save_fldno;
    task->cfld_ptr = save_cfld_ptr;
    task->keyno = save_keyno;
    task->prefix = save_prefix;

    return task->db_status;
}


/* ======================================================================
    Split root node
*/
static int INTERNAL_FCN split_root(NODE *node, DB_TASK *task)
{
    F_ADDR  l_pg, r_pg;
    NODE   *l_node,
           *r_node;
    int     slot_pos;
    char   *node_slot_ptr;

    /* allocate two new nodes */
    if ((dio_pzalloc(task->keyfile, &l_pg, task) != S_OKAY) ||
        (dio_get(task->keyfile, l_pg, (char **) &l_node, PGHOLD, task) != S_OKAY))
        return task->db_status;

    if ((dio_pzalloc(task->keyfile, &r_pg, task) != S_OKAY) ||
        (dio_get(task->keyfile, r_pg, (char **) &r_node, PGHOLD, task) != S_OKAY))
    {
        dio_unget(task->keyfile, l_pg, PGFREE, task);
        return task->db_status;
    }

    /* copy 0..task->max_slots/2-1 keys from root into left node */
    l_node->used_slots = task->mid_slot;
    slot_pos = task->mid_slot * task->slot_len;
    memcpy(l_node->slots, node->slots, NODE_WIDTH(l_node));

    /* copy task->max_slots/2+1..task->max_slots from root into right node */
    r_node->used_slots = (short) (task->max_slots - (task->mid_slot + 1));
    node_slot_ptr = &node->slots[slot_pos += task->slot_len];
    memcpy(r_node->slots, node_slot_ptr, NODE_WIDTH(r_node));

    /* copy task->mid_slot into slot[0] of root */
    memcpy(node->slots, node_slot_ptr - task->slot_len, task->slot_len);

    /* copy left page number into p[0] of root */
    memcpy(node->slots, &l_pg, sizeof(F_ADDR));

    /* copy right page number into p[1] of root */
    memcpy(&node->slots[task->slot_len], &r_pg, sizeof(F_ADDR));

    /* reset number of used slots in root */
    node->used_slots = 1;

    dio_touch(task->keyfile, l_pg, PGFREE, task);
    dio_touch(task->keyfile, r_pg, PGFREE, task);
    dio_touch(task->keyfile, ROOT_ADDR, PGFREE, task);

    return task->db_status;
}



/* ======================================================================
    Delete key from B-tree
*/
int INTERNAL_FCN key_delete(int fld, char const *key_val, DB_ADDR dba,
                                     DB_TASK *task)
{
    int stat;

    /* initialize key function operation */
    if (key_init(fld, task) != S_OKAY)
        return task->db_status;

    /* locate key to be deleted */
    if ((stat = key_locpos(key_val, &dba, task)) == S_OKAY)
    {
        if ((stat = delete(task)) == S_OKAY)
        {
            /* save key value and database address for possible repositioning */
            FIELD_ENTRY *kfld_ptr = &task->field_table[fld];
            if (kfld_ptr->fd_type == CHARACTER && kfld_ptr->fd_dim[1] == 0)
                strncpy(task->curkey->keyval, key_val, task->key_len);
            else if (kfld_ptr->fd_type == WIDECHAR && kfld_ptr->fd_dim[1] == 0)
                vwcsncpy((wchar_t *)task->curkey->keyval, (wchar_t *)key_val, task->key_len / sizeof(wchar_t));
            else
                memcpy(task->curkey->keyval, key_val, task->key_len);
            task->curkey->dba = dba;

            /* reset key position */
            key_reset(task->curkey->keyfile, task);
        }
    }

    return task->db_status = stat;
}



/* ======================================================================
    Delete key at current node_path position
*/
static int INTERNAL_FCN delete(DB_TASK *task)
{
    F_ADDR     pg, p_pg, l_pg, r_pg;
    F_ADDR     last_r_pg = 0L;
    int        amt, slot_pos, stat;
    NODE      *node;
    NODE      *p_node;
    NODE      *l_node;
    NODE      *r_node;
    NODE_PATH *np_ptr;
    char      *node_slot_ptr;
    char      *p_node_slot_ptr;
    char      *l_node_slot_ptr;
    char      *r_node_slot_ptr;

    np_ptr = &task->curkey->node_path[task->curkey->level];

    /* read node containing key to be deleted */
    if (dio_get(task->keyfile, pg = np_ptr->node,
                (char **) &node, PGHOLD, task) != S_OKAY)
        return task->db_status;

    /* copy pointer to right sub-tree */
    slot_pos = np_ptr->slot * task->slot_len;
    node_slot_ptr = &node->slots[slot_pos];
    memcpy(&r_pg, node_slot_ptr + task->slot_len, sizeof(F_ADDR));

    if (r_pg != NULL_NODE)
    {
        /* find leftmost descendent of right sub-tree */
        ++np_ptr->slot;
        do
        {
            if (last_r_pg)
                dio_unget(task->keyfile, last_r_pg, NOPGFREE, task);

            if ((stat = dio_get(task->keyfile, (last_r_pg = r_pg),
                                (char **) &r_node, NOPGHOLD, task)) != S_OKAY)
            {
                dio_unget(task->keyfile, pg, PGFREE, task);
                return task->db_status = stat;
            }

            ++task->curkey->level;
            ++np_ptr;
            np_ptr->node = r_pg;
            np_ptr->slot = 0;
            memcpy(&r_pg, r_node->slots, sizeof(F_ADDR));
        } while (r_pg != NULL_NODE);

        /* copy key from leaf into node */
        node_slot_ptr += sizeof(F_ADDR);
        r_node_slot_ptr = &r_node->slots[sizeof(F_ADDR)];
        memcpy(node_slot_ptr, r_node_slot_ptr, task->slot_len - sizeof(F_ADDR));
        dio_unget(task->keyfile, last_r_pg, NOPGFREE, task);
        dio_touch(task->keyfile, pg, PGFREE, task);

        /* set up to delete key from leaf */
        /* (this is more efficient than a recursive call) */
        slot_pos = 0;
        node_slot_ptr = node->slots;
        if (dio_get(task->keyfile, pg = np_ptr->node,
                    (char **) &node, PGHOLD, task) != S_OKAY)
            return task->db_status;
    }

shrink:                        /* delete key from leaf (shrink node ) */
    close_slots(node, slot_pos, 1, task);

    /* check if necessary to adjust nodes */
    if (task->curkey->level > 0 && node->used_slots < task->mid_slot)
    {
        /* read in parent node */
        if (dio_get(task->keyfile, p_pg = (np_ptr - 1)->node,
                    (char **) &p_node, PGHOLD, task) != S_OKAY)
            return task->db_status;

        slot_pos = (np_ptr - 1)->slot * task->slot_len;
        node_slot_ptr = &node->slots[slot_pos];
        if ((np_ptr - 1)->slot == p_node->used_slots)
        {
            /* pg is right node */
            r_pg = pg;
            r_node = node;

            /* parent slot position should bisect left & right nodes */
            --(np_ptr - 1)->slot;
            slot_pos -= task->slot_len;

            /* read left node */
            p_node_slot_ptr = &p_node->slots[slot_pos];
            memcpy(&l_pg, p_node_slot_ptr, sizeof(F_ADDR));
            if (dio_get(task->keyfile, l_pg, (char **) &l_node, PGHOLD, task) != S_OKAY)
                return task->db_status;
        }
        else
        {
            /* pg is left node */
            l_pg = pg;
            l_node = node;

            /* read right node */
            p_node_slot_ptr = &p_node->slots[slot_pos + task->slot_len];
            memcpy(&r_pg, p_node_slot_ptr, sizeof(F_ADDR));
            if (dio_get(task->keyfile, r_pg, (char **) &r_node, PGHOLD, task) != S_OKAY)
                return task->db_status;
        }

        if ((l_node->used_slots + r_node->used_slots + 1) < task->max_slots)
        {
            /* combine left and right nodes */
            if ((task->curkey->level == 1) && (p_node->used_slots == 1))
            {
                /* shrink down to root */
                /* copy right node data into root */
                p_node_slot_ptr = &p_node->slots[task->slot_len];
                memcpy(p_node_slot_ptr, r_node->slots, NODE_WIDTH(r_node));
                p_node->used_slots = (short) (r_node->used_slots + 1);
                r_node->used_slots = 0;
                dio_touch(task->keyfile, r_pg, PGFREE, task);

                /* copy left node data into root */
                open_slots(p_node, 0, l_node->used_slots, task);
                memcpy(p_node->slots, l_node->slots, NODE_WIDTH(l_node));
                l_node->used_slots = 0;
                dio_touch(task->keyfile, l_pg, PGFREE, task);

                dio_touch(task->keyfile, p_pg, PGFREE, task);

                /* free node pages */
                dio_pzdel(task->keyfile, r_pg, task);
                dio_pzdel(task->keyfile, l_pg, task);

                return task->db_status;
            }

            /* open space for l_node->used_slots+1 slots in right node */
            open_slots(r_node, 0, l_node->used_slots + 1, task);

            /* move left node slots into right node */
            amt = NODE_WIDTH(l_node);
            r_node_slot_ptr = r_node->slots;
            memcpy(r_node_slot_ptr, l_node->slots, amt);

            /* move parent slot data into right node */
            r_node_slot_ptr += amt;
            p_node_slot_ptr = &p_node->slots[slot_pos + sizeof(F_ADDR)];
            memcpy(r_node_slot_ptr, p_node_slot_ptr, task->slot_len - sizeof(F_ADDR));

            dio_touch(task->keyfile, r_pg, PGFREE, task);
            dio_touch(task->keyfile, l_pg, PGFREE, task);

            /* delete left node */
            l_node->used_slots = 0;
            if (dio_pzdel(task->keyfile, l_pg, task) != S_OKAY)
                return task->db_status;

            /* decrement level & make parent node current node */
            --task->curkey->level;
            --np_ptr;
            pg = p_pg;
            node = p_node;
            goto shrink;                  /* delete slot from parent */
        }

        /* acquire needed key from sibling */
        if ((l_node->used_slots + 1) < r_node->used_slots)
        {
            /* get key from right node */

            /* move parent slot to end of left node */
            l_node_slot_ptr = &l_node->slots[NODE_WIDTH(l_node)];
            p_node_slot_ptr = &p_node->slots[slot_pos + sizeof(F_ADDR)];
            memcpy(l_node_slot_ptr, p_node_slot_ptr, task->slot_len - sizeof(F_ADDR));

            ++l_node->used_slots;

            /* copy slot 0 child from right node to left node orphan */
            l_node_slot_ptr += task->slot_len - sizeof(F_ADDR);
            r_node_slot_ptr = r_node->slots;
            memcpy(l_node_slot_ptr, r_node_slot_ptr, sizeof(F_ADDR));

            /* move slot 0 of right node to parent */
            r_node_slot_ptr += sizeof(F_ADDR);
            memcpy(p_node_slot_ptr, r_node_slot_ptr, task->slot_len - sizeof(F_ADDR));

            /* delete slot 0 from right node */
            close_slots(r_node, 0, 1, task);
        }
        else if ((r_node->used_slots + 1) < l_node->used_slots)
        {
            /* get key from left node */

            /* open one slot at front of right node */
            open_slots(r_node, 0, 1, task);

            /* move parent slot to slot 0 of right node */
            r_node_slot_ptr = &r_node->slots[sizeof(F_ADDR)];
            p_node_slot_ptr = &p_node->slots[slot_pos + sizeof(F_ADDR)];
            memcpy(r_node_slot_ptr, p_node_slot_ptr, task->slot_len - sizeof(F_ADDR));

            /* move end slot of left node to parent */
            l_node_slot_ptr = &l_node->slots[NODE_WIDTH(l_node) - task->slot_len];
            memcpy(p_node_slot_ptr, l_node_slot_ptr, task->slot_len - sizeof(F_ADDR));

            /* move left orphan to child of slot 0 in right node */
            l_node_slot_ptr += task->slot_len - sizeof(F_ADDR);
            memcpy(r_node->slots, l_node_slot_ptr, sizeof(F_ADDR));

            /* delete end slot from left node */
            --l_node->used_slots;
        }

        dio_touch(task->keyfile, l_pg, PGFREE, task);
        dio_touch(task->keyfile, r_pg, PGFREE, task);
        dio_touch(task->keyfile, p_pg, PGFREE, task);
    }
    else
        dio_touch(task->keyfile, pg, PGFREE, task);

    return task->db_status;
}



/* ======================================================================
    Open n slots in node
*/
static void INTERNAL_FCN open_slots(NODE *node, int slot_pos, int n,
                                                DB_TASK *task)
{
    char *dst,
          *src;

    src = &node->slots[slot_pos];
    dst = src + n * task->slot_len;
    memmove(dst,src, NODE_WIDTH(node) - slot_pos);

    node->used_slots += (short) n;
}



/* ======================================================================
    Close n slots in node
*/
static void INTERNAL_FCN close_slots(NODE *node, int slot_pos, int n,
                                                 DB_TASK *task)
{
    char *dst,
          *src;

    node->used_slots -= (short) n;

    dst = &node->slots[slot_pos];
    src = dst + n * task->slot_len;
    memmove(dst, src, NODE_WIDTH(node) - slot_pos);
}


/* ======================================================================
    Read value of last key scanned
*/
int INTERNAL_FCN dkeyread(void *key_val, DB_TASK *task)
{
    float        fv;
    double       dv;
    int          kt_lc;                 /* loop control */
    int          entries, i;
    short        lstat;
    short       *dim_ptr;
    char        *fptr;
    char        *kptr;
    FIELD_ENTRY *fld_ptr;
    KEY_ENTRY   *key_ptr;

    key_init(task->last_keyfld, task);

    lstat = task->curkey->lstat;
    if (lstat != KEYFOUND && lstat != KEYNOTFOUND && lstat != KEYREPOS)
        return (task->db_status = S_KEYSEQ);

    /* clear key area */
    memset(key_val, '\0', task->cfld_ptr->fd_len);

    if (task->cfld_ptr->fd_type == COMKEY)
    {
        /* copy compound key fields */
        for (kt_lc = task->size_kt - task->cfld_ptr->fd_ptr,
             key_ptr = &task->key_table[task->cfld_ptr->fd_ptr];
             (--kt_lc >= 0) && (key_ptr->kt_key == task->fldno); ++key_ptr)
        {
            fld_ptr = &task->field_table[key_ptr->kt_field];
            fptr = task->key_type.ks.data + key_ptr->kt_ptr;
            kptr = (char *) key_val + key_ptr->kt_ptr;
            if (key_ptr->kt_sort == 'd')
            {
                entries = 1;
                for (i = 0, dim_ptr = fld_ptr->fd_dim;
                     (i < MAXDIMS) && *dim_ptr;
                     ++i, ++dim_ptr)
                {
                    entries *= *dim_ptr;
                }

                switch (fld_ptr->fd_type)
                {
                    case FLOAT:
                    {
                        float *float_fptr = (float *)fptr,
                              *float_kptr = (float *)kptr;

                        for(i = 0; i < entries; i++,float_fptr++,float_kptr++)
                        {
                            memcpy(&fv, float_fptr, sizeof(float));
                            fv = (float) 0.0 - fv;
                            memcpy( float_kptr, &fv, sizeof(float));
                        }
                        break;
                    }

                    case DOUBLE:
                    {
                        double *double_fptr = (double *)fptr,
                               *double_kptr = (double *)kptr;

                        for(i = 0; i < entries; i++,double_fptr++,double_kptr++)
                        {
                            memcpy(&dv, double_fptr, sizeof(double));
                            dv = (double) 0.0 - dv;
                            memcpy( double_kptr, &dv, sizeof(double));
                        }
                        break;
                    }
                    
                    case CHARACTER:
                        if (fld_ptr->fd_dim[0] > 1 && fld_ptr->fd_dim[1] == 0)
                            key_acpy(kptr, fptr, fld_ptr->fd_len);
                        else
                            key_cmpcpy(kptr, fptr, fld_ptr->fd_len);
                        break;

                    case WIDECHAR:
                        if (fld_ptr->fd_dim[0] > 1 && fld_ptr->fd_dim[1] == 0)
                        {
                            key_wacpy((wchar_t *)kptr, (wchar_t *)fptr,
                                      (short)(fld_ptr->fd_len / sizeof(wchar_t)));
                            break;
                        }

                    default:
                        key_cmpcpy(kptr, fptr, fld_ptr->fd_len);
                        break;
                }
            }
            else
                if (fld_ptr->fd_type == CHARACTER && fld_ptr->fd_dim[1] == 0)
                    strncpy(kptr, fptr, fld_ptr->fd_len);
                else if (fld_ptr->fd_type == WIDECHAR && fld_ptr->fd_dim[1] == 0)
                    vwcsncpy((wchar_t *)kptr, (wchar_t *)fptr, fld_ptr->fd_len / sizeof(wchar_t));
                else
                    memcpy(kptr, fptr, fld_ptr->fd_len);
        }
    }
    else
    {
        if (task->cfld_ptr->fd_type == CHARACTER && task->cfld_ptr->fd_dim[1] == 0)
            strncpy(key_val, task->key_type.ks.data, task->key_len);
        else if (task->cfld_ptr->fd_type == WIDECHAR && task->cfld_ptr->fd_dim[1] == 0)
            vwcsncpy((wchar_t *)key_val, (wchar_t *)task->key_type.ks.data, task->key_len / sizeof(wchar_t));
        else
            memcpy(key_val, task->key_type.ks.data, task->key_len);
    }

    return (task->db_status);
}



/* ======================================================================
    Build compound key value from record
*/
int EXTERNAL_FCN key_bldcom(
    int fld,            /* compound key field number */
    char *rec,          /* ptr to record data */
    char *key,          /* ptr to array to recv constructed key */
    int cflag,          /* TRUE to compliment compound descending keys */
    DB_TASK *task)
{
    float        fv;
    double       dv;
    int          kt_lc;         /* loop control */
    int          entries, i;
    short       *dim_ptr;
    short        header_size;

    char        *fptr;
    char        *tptr;
    FIELD_ENTRY *fld_ptr,
                *kfld_ptr;
    KEY_ENTRY   *key_ptr;

    /* clear key area */
    fld_ptr = &task->field_table[fld];
    memset(key, '\0', fld_ptr->fd_len);

    /* create compound key value */
    header_size = task->record_table[fld_ptr->fd_rec].rt_data;
    for (kt_lc = task->size_kt - fld_ptr->fd_ptr,
         key_ptr = &task->key_table[fld_ptr->fd_ptr];
         (--kt_lc >= 0) && (key_ptr->kt_key == fld); ++key_ptr)
    {
        kfld_ptr = &task->field_table[key_ptr->kt_field];
        fptr = rec + (kfld_ptr->fd_ptr - header_size);
        tptr = (char *)&key[key_ptr->kt_ptr];

        /* Complement descending keys if permitted (cflag) */
        if (cflag && (key_ptr->kt_sort == 'd'))
        {
            entries = 1;
            for (i = 0, dim_ptr = kfld_ptr->fd_dim;
                 (i < MAXDIMS) && *dim_ptr;
                 ++i, ++dim_ptr)
            {
                entries *= *dim_ptr;
            }

            switch (kfld_ptr->fd_type)
            {
                case FLOAT:
                {
                    float *float_fptr = (float *)fptr,
                          *float_tptr = (float *)tptr;

                    for (i = 0; i < entries; i++,float_fptr++,float_tptr++)
                    {
                        memcpy(&fv, float_fptr, sizeof(float));
                        fv = (float) 0.0 - fv;
                        memcpy( float_tptr, &fv, sizeof(float));
                    }

                    break;
                }

                case DOUBLE:
                {
                    double *double_fptr = (double *)fptr,
                           *double_tptr = (double *)tptr;

                    for(i = 0; i < entries; i++,double_fptr++,double_tptr++)
                    {
                        memcpy(&dv, double_fptr, sizeof(double));
                        dv = (double) 0.0 - dv;
                        memcpy( double_tptr, &dv, sizeof(double));
                    }

                    break;
                }
                
                case CHARACTER:
                    if (kfld_ptr->fd_dim[0] > 1 && kfld_ptr->fd_dim[1] == 0)
                        key_acpy(tptr, fptr, kfld_ptr->fd_len);
                    else
                        key_cmpcpy(tptr, fptr, kfld_ptr->fd_len);
                    break;

                case WIDECHAR:
                    if (kfld_ptr->fd_dim[0] > 1 && kfld_ptr->fd_dim[1] == 0)
                    {
                        key_wacpy((wchar_t *)tptr, (wchar_t *)fptr,
                                  (short)(kfld_ptr->fd_len / sizeof(wchar_t)));
                        break;
                    }

                default:
                    key_cmpcpy(tptr, fptr, kfld_ptr->fd_len);
                    break;
            }
        }
        else
        {
            if (kfld_ptr->fd_type == CHARACTER && kfld_ptr->fd_dim[1] == 0)
                strncpy(tptr, fptr, kfld_ptr->fd_len);
            else if (kfld_ptr->fd_type == WIDECHAR && kfld_ptr->fd_dim[1] == 0)
                vwcsncpy((wchar_t *)tptr, (wchar_t *)fptr, kfld_ptr->fd_len / sizeof(wchar_t));
            else
                memcpy(tptr, fptr, kfld_ptr->fd_len);
        }
    }

    return (task->db_status);
}



/* ======================================================================
    Complement and copy bytes
*/
void INTERNAL_FCN key_cmpcpy(char *s1, char *s2, short n)
{
    while (n--)
    {
        *s1++ = (char) ~(*s2++);
    }
}

/* ======================================================================
    Complement and copy bytes till 0 encounted [1358],[1387]
*/
void INTERNAL_FCN key_acpy(char *s1, char *s2, short n)
{
    while (n--)
    {
        if (! *s2)
        {
            *s1 = '\0';
             break;
        }

        *s1++ = (char) ~(*s2++);
    }
}

/* ======================================================================
    Same as above, but for widechar
*/
void INTERNAL_FCN key_wacpy(wchar_t *s1, wchar_t *s2, short n)
{
    wchar_t w;

    while (n--)
    {
        /*
            Use memcpy rather than dereferencing pointer, as Windows CE
            requires wchar_t variables to be on even byte addresses.
        */
        memcpy(&w, s2++, sizeof(wchar_t));
        if (!w)
        {
            memcpy(s1, &w, sizeof(wchar_t));
            break;
        }

        w = ~w;
        memcpy(s1++, &w, sizeof(wchar_t));
    }
}

/* ======================================================================
    Position all key on this record to point to this record
*/
int INTERNAL_FCN dcurkey(DB_TASK *task, int dbn)
{
    short         rt;
    int           i;
    char          ckey[MAXKEYSIZE];    /* compound key data */
    char         *rec;                 /* ptr to record slot */
    char         *fptr;                /* field data ptr */
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY  *fld_ptr;
    KEY_INFO     *ki_ptr;

    if (! task->curr_rec)
        return (dberr(S_NOCR));

    /* get the record type of the current record */
    if (dio_read(task->curr_rec, &rec, NOPGHOLD, task) != S_OKAY)
        return (task->db_status);

    memcpy(&rt, rec, sizeof(short));
    if (rt < 0)
        return (dberr(S_INVADDR));   /* record was deleted */

    rt &= ~RLBMASK;                     /* mask off rlb */
    rt += task->curr_db_table->rt_offset;

    rec_ptr = &task->record_table[rt];

    /* position any key fields from the key files */
    for (i = 0, ki_ptr = task->key_info; i < task->no_of_keys; ++i, ++ki_ptr)
    {
        fld_ptr = &task->field_table[ki_ptr->fldno];
        if (fld_ptr->fd_rec != rt)
            continue;

        /* Reset the key if it exists */
        if ((! (fld_ptr->fd_flags & OPTKEYMASK)) || r_tstopt(fld_ptr, rec, task))
        {
            ki_ptr->lstat = KEYREPOS;
            ki_ptr->dba = task->curr_rec;
            if (fld_ptr->fd_type == COMKEY)
            {
                key_bldcom(ki_ptr->fldno, rec + rec_ptr->rt_data, ckey, TRUE, task);
                fptr = ckey;
            }
            else
                fptr = rec + fld_ptr->fd_ptr;

            memcpy(ki_ptr->keyval, fptr, fld_ptr->fd_len);
        }
    }

    dio_release(task->curr_rec, NOPGFREE, task);
    return (task->db_status);
}


