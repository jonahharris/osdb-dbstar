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

#include "db.star.h"
#include "internal.h"


/* ======================================================================
    Return a value from an internal variable
*/
int INTERNAL_FCN dinternals(
    DB_TASK      *task,
    int           topic,
    int           id,
    int           elem,
    void         *ptr,
    unsigned      size)
{
    int  stat;
    long num;

    if (ptr == NULL)
        return (dberr(S_INVPTR));

    switch (topic)
    {
        case TOPIC_GLOBALS:
            switch (id)
            {
                case ID_DB_STATUS:
                    memcpy(ptr, &task->db_status, size);
                    break;

                case ID_DBOPEN:
                    memcpy(ptr, &task->dbopen, size);
                    break;

                case ID_DBUSERID:
                    vtstrncpy(ptr, task->dbuserid, size / sizeof(DB_TCHAR));
                    break;

                case ID_DBDPATH:
                    vtstrncpy(ptr, task->dbdpath, size / sizeof(DB_TCHAR));
                    break;

                case ID_DBFPATH:
                    vtstrncpy(ptr, task->dbfpath, size / sizeof(DB_TCHAR));
                    break;

                case ID_DBNAME:
                    vtstrncpy(ptr, task->curr_db_table->db_name, size / sizeof(DB_TCHAR));
                    break;

                case ID_CACHE_OVFL:
                    memcpy(ptr, &task->cache_ovfl, size);
                    break;

                case ID_NO_OF_KEYS:
                    memcpy(ptr, &task->no_of_keys, size);
                    break;

                case ID_KEY_INFO:
                    if (task->key_info == NULL)
                        break;

                    memcpy(ptr, &task->key_info[elem], size);
                    break;

                case ID_KEY_TYPE:
                    memcpy(ptr, &task->key_type, size);
                    break;

                case ID_LMC_STATUS:
                    break;

                case ID_DBOPTIONS:
                    memcpy(ptr, &task->dboptions, size);
                    break;

                case ID_OV_INITADDR:
                    memcpy(ptr, &task->ov_initaddr, size);
                    break;

                case ID_OV_ROOTADDR:
                    memcpy(ptr, &task->ov_rootaddr, size);
                    break;

                case ID_OV_NEXTADDR:
                    memcpy(ptr, &task->ov_nextaddr, size);
                    break;

                case ID_ROOT_IX:
                    if (task->root_ix == NULL)
                        break;

                    memcpy(ptr, &task->root_ix[elem], size);
                    break;

                case ID_REN_LIST:
                    memcpy(ptr, &task->ren_list, size);
                    break;

                case ID_PAGE_SIZE:
                    memcpy(ptr, &task->page_size, size);
                    break;

                case ID_CURR_REC:
                    memcpy(ptr, &task->curr_rec, size);
                    break;

                case ID_OV_FILE:
                    memcpy(ptr, &task->ov_file, size);
                    break;

                case ID_SIZE_FT:
                    memcpy(ptr, &task->size_ft, size);
                    break;

                case ID_SIZE_RT:
                    memcpy(ptr, &task->size_rt, size);
                    break;

                case ID_SIZE_ST:
                    memcpy(ptr, &task->size_st, size);
                    break;

                case ID_SIZE_MT:
                    memcpy(ptr, &task->size_mt, size);
                    break;

                case ID_SIZE_SRT:
                    memcpy(ptr, &task->size_srt, size);
                    break;

                case ID_SIZE_FD:
                    memcpy(ptr, &task->size_fd, size);
                    break;

                case ID_SIZE_KT:
                    memcpy(ptr, &task->size_kt, size);
                    break;

                case ID_LOCK_LVL:
                    memcpy(ptr, &task->lock_lvl, size);
                    break;

                case ID_LOCK_STACK:
                    memcpy(ptr, task->lock_stack, size);
                    break;

                case ID_SK_LIST:
                    memcpy(ptr, &task->sk_list, size);
                    break;

                case ID_ERROR_FUNC:
                    memcpy(ptr, &task->error_func, size);
                    break;

                case ID_COUNTRY_TBL:
                    if (task->country_tbl == NULL)
                        break;

                    memcpy(ptr, &task->country_tbl, size);
                    break;

                case ID_CTBL_ACTIV:
                    memcpy(ptr, &task->ctbl_activ, size);
                    break;

                case ID_CTBPATH:
                    vtstrncpy(ptr, task->ctbpath, size / sizeof(DB_TCHAR));
                    break;

                case ID_CR_TIME:
                    memcpy(ptr, &task->cr_time, size);
                    break;

                case ID_CO_TIME:
                    if (task->co_time == NULL)
                        break;
                    memcpy(ptr, &task->co_time[elem], size);
                    break;

                case ID_CM_TIME:
                    if (task->cm_time == NULL)
                        break;
                    memcpy(ptr, &task->cm_time[elem], size);
                    break;

                case ID_CS_TIME:
                    if (task->cs_time == NULL)
                        break;
                    memcpy(ptr, &task->cs_time[elem], size);
                    break;

                case ID_DB_TSRECS:
                    memcpy(ptr, &task->db_tsrecs, size);
                    break;

                case ID_DB_TSSETS:
                    memcpy(ptr, &task->db_tssets, size);
                    break;

                case ID_CURR_DB:
                    memcpy(ptr, &task->curr_db, size);
                    break;

                case ID_CURR_DB_TABLE:
                    memcpy(ptr, task->curr_db_table, size);
                    break;

                case ID_SET_DB:
                    memcpy(ptr, &task->set_db, size);
                    break;

                case ID_NO_OF_DBS:
                    memcpy(ptr, &task->no_of_dbs, size);
                    break;

                case ID_RN_TABLE:
                    if (task->rn_table == NULL)
                        break;

                    memcpy(ptr, &task->rn_table[elem], size);
                    break;

                case ID_CURR_RN_TABLE:
                    memcpy(ptr, &task->curr_rn_table, size);
                    break;

                case ID_TRANS_ID:
                    vtstrncpy(ptr, task->trans_id, size / sizeof(DB_TCHAR));
                    break;

                case ID_DBLOG:
                    vtstrncpy(ptr, task->dblog, size / sizeof(DB_TCHAR));
                    break;

                case ID_DBWAIT_TIME:
                    memcpy(ptr, &task->dbwait_time, size);
                    break;

                case ID_DB_TIMEOUT:
                    memcpy(ptr, &task->db_timeout, size);
                    break;

                case ID_DB_LOCKMGR:
                    memcpy(ptr, &task->db_lockmgr, size);
                    break;
                case ID_READLOCKSECS:
                     memcpy(ptr, &task->readlocksecs,size);
                     break;

                case ID_WRITELOCKSECS:
                     memcpy(ptr, &task->writelocksecs,size);
                     break;

                case ID_KEYL_CNT:
                    memcpy(ptr, &task->keyl_cnt, size);
                    break;

                case ID_LP_SIZE:
                    memcpy(ptr, &task->lp_size, size);
                    break;

                case ID_FP_SIZE:
                    memcpy(ptr, &task->fp_size, size);
                    break;

                case ID_LOCK_PKT:
                    if (task->lock_pkt == NULL)
                        break;

                    memcpy(ptr, &task->lock_pkt[elem], size);
                    break;

                case ID_FREE_PKT:
                    if (task->free_pkt == NULL)
                        break;

                    memcpy(ptr, &task->free_pkt[elem], size);
                    break;

                case ID_FILE_REFS:
                    if (task->file_refs == NULL)
                        break;

                    memcpy(ptr, &task->file_refs[elem], size);
                    break;

                case ID_SESSION_ACTIVE:
                    memcpy(ptr, &task->session_active, size);
                    break;

                case ID_MAX_FILES:
                    num = psp_fileHandleLimit();
                    memcpy(ptr, &num, size);
                    break;

                case ID_MAX_CACHEPGS:
                    if (task->cache == NULL)
                        break;

                    memcpy(ptr, &task->cache->db_pgtab.pgtab_sz, size);
                    break;

                case ID_MAX_OVIXPGS:
                {
                    if (task->cache == NULL)
                        break;

                    memcpy(ptr, &task->cache->ix_pgtab.pgtab_sz, size);
                    break;
                }

                case ID_CACHE_LOOKUP:
                {
                    int tag;
                    unsigned long count = 0;

                    if (task->cache == NULL)
                        break;

                    for (tag=0; tag < task->cache->max_tags; tag++)
                        count += task->cache->tag_table[tag].lookups;

                    memcpy(ptr, &count, size);
                    break;
                }

                case ID_CACHE_HITS:
                {
                    int tag;
                    unsigned long count = 0;

                    if (task->cache == NULL)
                        break;

                    for (tag=0; tag < task->cache->max_tags; tag++)
                        count += task->cache->tag_table[tag].hits;

                    memcpy(ptr, &count, size);
                    break;
                }

                case ID_LOCKCOMM:
                    memcpy(ptr, psp_lmcName(task->lmc), size);
                    break;

                case ID_DBTAF:
                    vtstrncpy(ptr, task->dbtaf, size / sizeof(DB_TCHAR));
                    break;

                case ID_LOCKMGRN:
                    if (task->lockmgrn)
                        vtstrncpy(ptr, task->lockmgrn, size / sizeof(DB_TCHAR));
                    else
                        ((DB_TCHAR *) ptr)[0] = 0;
                    break;

                default:
                    dberr(S_INVID);
                    break;
            }
            break;                  /* end of TOPIC_GLOBAL */

        case TOPIC_TASK:
            memcpy(ptr, task, size);
            break;

        case TOPIC_PGZERO_TABLE:
            if (task->pgzero == NULL)
                break;

            if (elem >= task->size_ft)
                break;      /* need to check because of call to dio_pzread() */

            size = min(size, PGZEROSZ);   /* give only what's on disk */
            stat = task->pgzero[elem].pz_next == 0L;
            if (stat)
                dio_pzread((FILE_NO)elem, task);        /* do a dirty read */

            memcpy(ptr, &task->pgzero[elem], size);
            if (stat)
                task->pgzero[elem].pz_next = 0L;    /* clean up */

            break;

        case TOPIC_FILE_TABLE:
            if (task->file_table == NULL)
                break;

            memcpy(ptr, &task->file_table[elem], size);
            break;

        case TOPIC_RECORD_TABLE:
            if (task->record_table == NULL)
                break;

            memcpy(ptr, &task->record_table[elem], size);
            break;

        case TOPIC_SET_TABLE:
            if (task->set_table == NULL)
                break;

            memcpy(ptr, &task->set_table[elem], size);
            break;

        case TOPIC_MEMBER_TABLE:
            if (task->member_table == NULL)
                break;

            memcpy(ptr, &task->member_table[elem], size);
            break;

        case TOPIC_SORT_TABLE:
            if (task->sort_table == NULL)
                break;

            memcpy(ptr, &task->sort_table[elem], size);
            break;

        case TOPIC_FIELD_TABLE:
            if (task->field_table == NULL)
                break;

            memcpy(ptr, &task->field_table[elem], size);
            break;

        case TOPIC_KEY_TABLE:
            if (task->key_table == NULL)
                break;

            memcpy(ptr, &task->key_table[elem], size);
            break;

        case TOPIC_DB_TABLE:
            if (task->db_table == NULL)
                break;

            memcpy(ptr, &task->db_table[elem], size);
            break;

        case TOPIC_RECORD_NAMES:
            if (task->record_names == NULL || elem >= task->size_rt)
                break;

            vtstrncpy(ptr, task->record_names[elem], size / sizeof(DB_TCHAR));
            break;

        case TOPIC_SET_NAMES:
            if (task->set_names == NULL || elem >= task->size_st)
                break;

            vtstrncpy(ptr, task->set_names[elem], size / sizeof(DB_TCHAR));
            break;

        case TOPIC_FIELD_NAMES:
            if (task->field_names == NULL || elem >= task->size_fd)
                break;

            vtstrncpy(ptr, task->field_names[elem], size / sizeof(DB_TCHAR));
            break;

        case TOPIC_REC_LOCK_TABLE:
            if (task->rec_locks == NULL)
                break;

            memcpy(ptr, &task->rec_locks[elem], size);
            break;

        case TOPIC_SET_LOCK_TABLE:
            if (task->set_locks == NULL)
                break;

            memcpy(ptr, &task->set_locks[elem], size);
            break;

        case TOPIC_KEY_LOCK_TABLE:
            if (task->key_locks == NULL)
                break;

            memcpy(ptr, &task->key_locks[elem], size);
            break;

        case TOPIC_APP_LOCKS_TABLE:
            if (task->app_locks == NULL)
                break;

            memcpy(ptr, &task->app_locks[elem], size);
            break;

        case TOPIC_EXCL_LOCKS_TABLE:
            if (task->excl_locks == NULL)
                break;

            memcpy(ptr, &task->excl_locks[elem], size);
            break;

        case TOPIC_KEPT_LOCKS_TABLE:
            if (task->kept_locks == NULL)
                break;

            memcpy(ptr, &task->kept_locks[elem], size);
            break;

        case TOPIC_CURR_OWNER_TABLE:
            if (task->curr_own == NULL)
                break;

            memcpy(ptr, &task->curr_own[elem], size);
            break;

        case TOPIC_CURR_MEMBER_TABLE:
            if (task->curr_mem == NULL)
                break;

            memcpy(ptr, &task->curr_mem[elem], size);
            break;

        default:
            dberr(S_INVID);
            break;
    }

    return (task->db_status);
}


