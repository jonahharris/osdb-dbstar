/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, keypack utility                                   *
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

    packfile.c - packing algorithm for keypack

-----------------------------------------------------------------------*/

#include "db.star.h"
#include "keypack.h"


/* ************************* LOCAL DEFINITIONS *********************** */
struct page_zero
{
    F_ADDR dchain;            /* delete chain pointer */
    F_ADDR next;              /* next page or record slot */
    long timestamp;           /* file's timestamp value */
    time_t cdate;             /* creation date,time */
    time_t bdate;             /* date/time of last backup */
    DB_TCHAR vdb_id[48];      /* db.* id mark */
};

/* node number of root */
#define ROOT_ADDR (F_ADDR) 1

/* null childe node pointer */
#define NULL_NODE (F_ADDR) -1

/* Leaf (Base) level */
#define LEAF_NODE 0
#define LEAF_LVL  0

/* lvls structure */
typedef struct
{
    long last_node;           /* last node written on this level */
    NODE *node_ptr;           /* pointer to current node on this level */
}  LVL_NODE;

/* general file-specific data */
typedef struct
{
    FILE_NO fno;              /* packed file number in file table */
    PSP_FH pkf;               /* packed file handle */
    int max_slots;            /* number of slots to fill */
    short root_lvl;           /* lvls level containing root node */
    LVL_NODE *lvls;           /* b-tree levels node buffer */
    long next_node;           /* node # of last node written to disk */

    int buff_max_nodes;       /* max number of output buffer nodes */
    int buff_curr_node;       /* current node in disk buffer */
    int buff_size;            /* size of buffer in bytes */
    char *buff;               /* buffer for output file */

}  PACKFILE_DATA;

/* ************************* LOCAL FUNCTIONS ************************* */
static int add_slot(int, KEY_SLOT *, PACKFILE_DATA *, DB_TASK *);
static int packcheck(FILE_NO, DB_TCHAR *, char *, size_t, KEYPACK_OPTS *, KEYPACK_STATS *, DB_TASK *);

/* =====================================================================
    Pack an db.* key file
*/
int packfile(
    FILE_NO fno,
    DB_TCHAR *out_file,
    int unused_slots,
    KEYPACK_OPTS *popts,
    KEYPACK_STATS *pstats,
    DB_TASK *task)
{
    int status = S_OKAY;
    int keystat;
    F_ADDR j;
    short k, slot, lvl;
    int slots;                          /* number of slots / node */
    int node_sz;                        /* key page (node) size */
    int slot_sz;                        /* slot size */
    int max_lvls;                       /* max. number of levels in b-tree */
    struct page_zero pz;                /* page zero of the B-Tree */
    KEY_SLOT *slot_ptr = NULL;          /* slot found in cache to be added */
    NODE *node_ptr = NULL;              /* pointer node in lvls */
    DB_ADDR dba;                        /* used in key scan */
    F_ADDR page;                        /* used in key scan */
    KEY_INFO *curkey_ptr = NULL;        /* used in key scan */
    F_ADDR *next_node_ptr = NULL;
    PACKFILE_DATA *pk = NULL;

    unsigned int left_used_slots, right_used_slots;
    NODE *parent_node_ptr = NULL;
    NODE *rt_child_node_ptr = NULL;
    NODE *lf_child_node_ptr = NULL;
    KEY_SLOT *parent_slot_ptr = NULL;
    KEY_SLOT *rt_child_slot_ptr = NULL;
    KEY_SLOT *lf_child_slot_ptr = NULL;
    KEY_SLOT *tmp_slot_ptr = NULL;
    F_ADDR tmp;                         /* needed for memcpy() */


    /* initialize variables */
    slots = task->file_table[fno].ft_slots;
    slot_sz = task->file_table[fno].ft_slsize;
    node_sz = task->file_table[fno].ft_pgsize;

    /* fully packed nodes only allowed for static files */
    if (unused_slots == 0 && task->file_table[fno].ft_flags != STATIC)
    {
        vtprintf(DB_TEXT("\n%s must contain only static data to fully pack\n"),
                 task->file_table[fno].ft_name);
        return (S_STATIC);
    }

    /* the number of unused slots per node should be less than 1/2 of node */
    if (unused_slots > slots / 2)
    {
        vtprintf(DB_TEXT("\n%s too many free slots specified\n"),
                 task->file_table[fno].ft_name);
        return (S_FAULT);
    }

    /* determine max depth of B-Tree */
    tmp = MAXRECORDS / slots;
    for (j = slots, max_lvls = 2; j < MAXRECORDS; ++max_lvls)
    {
        /* test whether overflow would occur on next iteration */
        if (j > tmp)
            j = MAXRECORDS;
        else
            j *= slots;
    }

    /* allocate needed buffers */
    if (pk = (PACKFILE_DATA *) psp_cGetMemory(sizeof(PACKFILE_DATA), 0))
    {
        pk->fno = fno;
        pk->max_slots = slots - unused_slots;
        pk->next_node = 2L;
        pk->buff_max_nodes = (int) (65536L / (long) node_sz);

        pk->lvls = (LVL_NODE *) psp_cGetMemory(max_lvls * sizeof(LVL_NODE), 0);
        if (pk->lvls)
        {
            for (lvl = 0; lvl < max_lvls; ++lvl)
            {
                pk->lvls[lvl].node_ptr = (NODE *) psp_cGetMemory(node_sz, 0);
                if (!pk->lvls[lvl].node_ptr)
                    break;
            }
            if (lvl == max_lvls)
            {
                if (lf_child_node_ptr = (NODE *) psp_cGetMemory(node_sz, 0))
                {
                    if (tmp_slot_ptr = (KEY_SLOT *) psp_cGetMemory(slot_sz, 0))
                    {
                        while (pk->buff_max_nodes > 0 &&
                                !(pk->buff = psp_cGetMemory(pk->buff_max_nodes *
                                node_sz, 0)))
                            pk->buff_max_nodes--;
                        pk->buff_size = (size_t) pk->buff_max_nodes * node_sz;
                    }
                }
            }
        }
    }
    if (!pk || !pk->buff)
        status = S_NOMEMORY;

    /* open packed key file and set file pointer to node #2 */
    if (status == S_OKAY)
    {
        if (!(pk->pkf = psp_fileOpen(out_file, O_CREAT | O_RDWR, PSP_FLAG_DENYNO)))
            status = S_NOFILE;
        else
            psp_fileSeek(pk->pkf, (off_t) (2 * node_sz));
    }

    /* scan in order each key in key file and store in packed file */
    for (k = 0; k < task->size_fd && status == S_OKAY; ++k)
    {
        if (task->field_table[k].fd_key != 'n' && task->field_table[k].fd_keyfile == fno)
        {
            task->db_status = S_OKAY;
            if ((status = key_init(k, task)) == S_OKAY)
            {
                for (keystat = key_boundary(KEYFRST, &dba, task);
                     keystat == S_OKAY && status == S_OKAY;
                     keystat = key_scan(KEYNEXT, &dba, task))
                {
                    curkey_ptr = &task->key_info[task->field_table[k].fd_keyno];
                    page = curkey_ptr->node_path[curkey_ptr->level].node;
                    slot = curkey_ptr->node_path[curkey_ptr->level].slot;
                    if ((status = dio_get(fno, page, (char **) &node_ptr,
                                                 NOPGHOLD, task)) == S_OKAY)
                    {
                        slot_ptr = (KEY_SLOT *) (node_ptr->slots + slot * slot_sz);
                        status = add_slot(LEAF_NODE, slot_ptr, pk, task);
                    }
                }
            }
        }
    }
    if (status == S_OKAY)
    {
        /* flush disk buffer */
        if (pk->buff_curr_node)
            if ( psp_fileWrite(pk->pkf, pk->buff, pk->buff_curr_node * node_sz)
                 != pk->buff_curr_node*node_sz )
                status = S_BADWRITE;
    }

    /* Balance tree, insuring all node !root have >= m/2 slots used */
    if (status == S_OKAY)
    {
        for (lvl = pk->root_lvl; lvl > LEAF_LVL && status == S_OKAY; --lvl)
        {
            memcpy(&parent_node_ptr, &(pk->lvls[lvl].node_ptr),
                   sizeof(NODE *));
            parent_slot_ptr = (KEY_SLOT *)(parent_node_ptr->slots + 
                              (parent_node_ptr->used_slots-1) * slot_sz);

            rt_child_node_ptr = pk->lvls[lvl-1].node_ptr;
            rt_child_slot_ptr = (KEY_SLOT *)rt_child_node_ptr->slots;

            psp_fileSeek(pk->pkf, (off_t)(pk->lvls[lvl-1].last_node * node_sz));

            if ( psp_fileRead(pk->pkf,
                    (char *)lf_child_node_ptr, node_sz) != node_sz )
                 status = S_BADREAD;
            if (status != S_OKAY)
                break;

            lf_child_slot_ptr = (KEY_SLOT *)lf_child_node_ptr->slots;

            if ((double) pk->max_slots / 2.0 ==
                 (double) (right_used_slots = pk->max_slots / 2))
                left_used_slots = right_used_slots;
            else
                left_used_slots = right_used_slots + 1;

            if (rt_child_node_ptr->used_slots < pk->max_slots / 2)
            {
                lf_child_node_ptr->used_slots = (short) left_used_slots;

                psp_fileSeek(pk->pkf, (off_t)(pk->lvls[lvl-1].last_node * node_sz));

                if ( psp_fileWrite(pk->pkf,
                        (char *)lf_child_node_ptr, node_sz) != node_sz )
                    status = S_BADWRITE;
                if (status != S_OKAY)
                    break;

                /* store the last used slot of the parent node except for the
                 * child node pointer,  this slot will be added to the right
                 * child and the left childs next node pointer will be used
                 * for this slots child node pointer */

                memcpy(&tmp_slot_ptr->keyno, &parent_slot_ptr->keyno,
                       slot_sz - sizeof(F_ADDR));

                /* copy the left child's m/2 slot to last used slot in parent
                 * except for the child node pointer, it has become the next
                 * node pointer for left child, the parents child node pointer
                 * is valid for the slot from the left child */

                memcpy(&parent_slot_ptr->keyno,
                       ((char*)&lf_child_slot_ptr->keyno)
                            + left_used_slots*slot_sz,
                       slot_sz - sizeof(F_ADDR));

                /* move right child's present slots to the end of node, remember
                 * this node is less than half full, there is not next node
                 * pointer for nodes currently in the lvls */

                memcpy(((char*)rt_child_slot_ptr)+right_used_slots*slot_sz,
                          rt_child_slot_ptr,
                          rt_child_node_ptr->used_slots*slot_sz);

                /* copy what was the parents last used slot as right childs
                 * m/2 slot, at this time there is still no child node pointer
                 * for this slot */

                memcpy(((char *)&rt_child_slot_ptr->keyno)
                            + (right_used_slots-1) * slot_sz,
                          &tmp_slot_ptr->keyno,
                          slot_sz - sizeof(F_ADDR));

                /* copy left child's lower half to right childs first half, note
                 * that this opperation adds the child node pointer to the m/2
                 * slot which was added above */

                memcpy(rt_child_slot_ptr, 
                          ((char*)lf_child_slot_ptr)+(left_used_slots+1)*slot_sz,
                          (right_used_slots - 1) * slot_sz + sizeof(F_ADDR));

                /* update right child's used slot counter */
                rt_child_node_ptr->used_slots += (short) right_used_slots;
            }
        }
    }

    if (status == S_OKAY)
    {
        psp_fileSeek(pk->pkf, (off_t)(pk->next_node * node_sz));

        /* flush out node in lvls */
        for (lvl = LEAF_LVL; lvl < pk->root_lvl && status == S_OKAY; ++lvl)
        {
            if (!pk->lvls[lvl].node_ptr->used_slots)
                continue;

            node_ptr = pk->lvls[lvl].node_ptr;

            time(&node_ptr->last_chgd);

            next_node_ptr = (F_ADDR *) (node_ptr->slots + 
                                                 node_ptr->used_slots * slot_sz);

            if (lvl == LEAF_LVL)
                tmp = NULL_NODE;
            else
                tmp = pk->next_node - 1;
            memcpy(next_node_ptr, &tmp, sizeof(F_ADDR));

            if ( psp_fileWrite(pk->pkf, (char *) node_ptr, node_sz) != node_sz )
                status = S_BADWRITE;

            pk->lvls[lvl].last_node = pk->next_node++;
        }
    }

    if (status == S_OKAY)
    {
        /* update and write root node */
        node_ptr = pk->lvls[pk->root_lvl].node_ptr;
        next_node_ptr = (F_ADDR *) (node_ptr->slots + 
                                             node_ptr->used_slots * slot_sz);

        /* then next node for the root node will be the last node written out */
        if (lvl == LEAF_LVL)
            tmp = NULL_NODE;
        else
            tmp = pk->next_node - 1;
        memcpy(next_node_ptr, &tmp, sizeof(F_ADDR));
        time(&node_ptr->last_chgd);

        psp_fileSeek(pk->pkf, (off_t)node_sz);

        if ( psp_fileWrite(pk->pkf, (char *) node_ptr, node_sz) != node_sz )
            status = S_BADWRITE;
        if (status == S_OKAY)
        {
            /* write page zero */
            pz.dchain = NULL_NODE;
            pz.next = pk->next_node;
            pz.timestamp = 0L;
            pz.bdate = 0L;
            time(&pz.cdate);
            d_dbver(DB_TEXT("%V"), pz.vdb_id, 48);
            psp_fileSeek(pk->pkf, 0);
            if (psp_fileWrite(pk->pkf, (char *) &pz,
                    sizeof(struct page_zero)) != sizeof(struct page_zero) )
                status = S_BADWRITE;
            psp_fileClose(pk->pkf);
        }
    }

    if (status == S_OKAY)
    {
        status = packcheck(fno, out_file, pk->buff, pk->buff_size,
                                 popts, pstats, task);
    }

    /* free allocated memory */
    if (pk)
    {
        if (pk->lvls)
        {
            if (lf_child_node_ptr)
            {
                if (tmp_slot_ptr)
                {
                    if (pk->buff)
                        psp_freeMemory((char *) pk->buff, 0);

                    psp_freeMemory((char *) tmp_slot_ptr, 0);
                }

                psp_freeMemory((char *) lf_child_node_ptr, 0);
            }

            for (lvl = 0; lvl < max_lvls; ++lvl)
            {
                if (pk->lvls[lvl].node_ptr)
                    psp_freeMemory((char *) pk->lvls[lvl].node_ptr, 0);
            }

            psp_freeMemory((char *) pk->lvls, 0);
        }

        psp_freeMemory(pk, 0);
    }

    if (status != S_OKAY)
        dberr(status);

    return (status);
}

/* =====================================================================
    Add slot to key file node
*/
static int add_slot(
    int lvl,
    KEY_SLOT *slot_ptr,
    PACKFILE_DATA *pk,
    DB_TASK *task)
{
    int status;
    int node_sz;
    int slot_sz;
    NODE *curr_node_ptr;
    KEY_SLOT *buff_slot_ptr;
    F_ADDR *child_node_ptr;
    F_ADDR *next_node_ptr;
    F_ADDR tmp;                   /* needed for memcpy() */

    curr_node_ptr = pk->lvls[lvl].node_ptr;
    slot_sz = task->file_table[pk->fno].ft_slsize;
    node_sz = task->file_table[pk->fno].ft_pgsize;

    /* 1st use of new lvls will have last_chgd == 0 */
    if (!curr_node_ptr->last_chgd)
    {
        /* mark new root level */
        pk->root_lvl = (short) lvl;

        /* make update time non-zero (the time will go in later) */
        curr_node_ptr->last_chgd = pk->next_node;
    }

    /* In an db.* B-Tree, the child node ptr will also serve as
       the next node ptr in the first unused slot.  Here we are initializing
       both the child node ptr and the next node pointer
    */

    next_node_ptr = child_node_ptr = 
            (F_ADDR *) (curr_node_ptr->slots +
            curr_node_ptr->used_slots * slot_sz);

    /* check and, if full, output node */
    if (curr_node_ptr->used_slots == pk->max_slots)
    {
        /* new slot goes into the next higher level (the parent node) */
        if ((status = add_slot(lvl + 1, slot_ptr, pk, task)) != S_OKAY)
            return (status);

        time(&curr_node_ptr->last_chgd);

        /* update next node (the current node is the next node) */
        if (lvl == LEAF_LVL)
            tmp = NULL_NODE;
        else
            tmp = pk->next_node + 1;
        memcpy(next_node_ptr, &tmp, sizeof(F_ADDR));

        if (pk->buff_max_nodes > 0)
        {
            if (pk->buff_curr_node < pk->buff_max_nodes)
            {
                /* copy filled node into output buffer */
                memcpy(pk->buff + pk->buff_curr_node * node_sz,
                    curr_node_ptr, node_sz);
                ++(pk->buff_curr_node);
            }
            else
            {
                /* buffer is full - write to disk */
                if ( psp_fileWrite(pk->pkf, pk->buff, pk->buff_curr_node * node_sz) !=
                      pk->buff_curr_node*node_sz )
                    status = S_BADWRITE;
                if (status != S_OKAY)
                    return (status);
                memcpy(pk->buff, curr_node_ptr, node_sz);
                pk->buff_curr_node = 1;
            }
        }
        else if ((psp_fileWrite(pk->pkf, (char *)curr_node_ptr, node_sz)) != node_sz)
            return (S_BADWRITE);
        
        pk->lvls[lvl].last_node = pk->next_node++;
        curr_node_ptr->used_slots = 0;
    }
    else /* node is not full */
    {
        /* copy slot */
        buff_slot_ptr = (KEY_SLOT *) (curr_node_ptr->slots +
            curr_node_ptr->used_slots * slot_sz);
        memcpy(buff_slot_ptr, slot_ptr, slot_sz);

        /* update child node */
        if (lvl == LEAF_LVL)
            tmp = NULL_NODE;
        else
            tmp = pk->next_node;
        memcpy(child_node_ptr, &tmp, sizeof(F_ADDR));
        ++curr_node_ptr->used_slots;
    }
    return (S_OKAY);
}

/* ======================================================================
    Display packing results
*/
static int packcheck(
    FILE_NO fno,
    DB_TCHAR *out_file,
    char *buff,
    size_t buff_sz,
    KEYPACK_OPTS *popts,
    KEYPACK_STATS *pstats,
    DB_TASK *task)
{
    DB_TCHAR backfile[FILENMLEN];
    int page_sz;

    page_sz = task->file_table[fno].ft_pgsize;
    pstats->or_size = psp_fileLength(task->file_table[fno].ft_name);
    pstats->pk_size = psp_fileLength(out_file);
    pstats->or_total += pstats->or_size;
    pstats->pk_total += pstats->pk_size;

    vtprintf(DB_TEXT("  reduced %4.1f%%"),
                (1 - (double) pstats->pk_size / pstats->or_size) * 100);

    if (popts->backup)
    {
        vstprintf(backfile, DB_TEXT("%s%s"), popts->backpath, task->file_table[fno].ft_name);
        vtprintf(DB_TEXT("  backing up..."));
        if (psp_fileCopy(task->file_table[fno].ft_name, backfile) != PSP_OKAY)
        {
            vftprintf(stderr, DB_TEXT("unable to copy file %s to %s\n"),
                         task->file_table[fno].ft_name, backfile);
        }
    }
    if (!popts->keep)
    {
        dio_close(fno, task);
        psp_fileRemove(task->file_table[fno].ft_name);
        vtprintf(DB_TEXT("  replacing..."));
        if (psp_fileMove(out_file, task->file_table[fno].ft_name) != PSP_OKAY)
        {
            vftprintf(stderr, DB_TEXT("unable to move file %s to %s\n"),
                         out_file, task->file_table[fno].ft_name);
            return(-1);
        }
    }
    vtprintf(DB_TEXT("\nDone\n"));
    return(0);
}


