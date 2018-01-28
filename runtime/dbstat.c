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
#include "rntmint.h"

int INTERNAL_FCN ddbstat(int type, int idx, void *buf, int buflen,
                         DB_TASK *task)
{
#ifdef DBSTAT
    switch (type)
    {
        case GEN_STAT:
        {
            /* Some total fields are not updated on-line for performance. But they
                need to be saved to accumulate stats across d_close/d_open.
            */
            if (task->cache->db_pgtab.pg_table)
            {
                sync_MEM_STATS(&task->gen_stats.dbmem_stats, &task->cache->db_pgtab.mem_stats);
                sync_CACHE_STATS(&task->gen_stats.db_stats, &task->cache->db_pgtab.cache_stats);
            }

            if (task->cache->ix_pgtab.pg_table)
            {
                sync_MEM_STATS(&task->gen_stats.ixmem_stats, &task->cache->ix_pgtab.mem_stats);
                sync_CACHE_STATS(&task->gen_stats.ix_stats, &task->cache->ix_pgtab.cache_stats);
            }

            task->gen_stats.files_open = task->cnt_open_files;

            memcpy(buf, &task->gen_stats, min((int) sizeof(GEN_STATS), buflen));
            break;
        }

        case TAG_STAT:
        {
            TAG_STATS tag_stats;

            if (idx >= task->cache->num_tags)
                return (dberr(S_INVID));

            tag_stats.lookups   = task->cache->tag_table[idx].lookups;
            tag_stats.hits      = task->cache->tag_table[idx].hits;
            tag_stats.num_pages = task->cache->tag_table[idx].num_pages;
            memcpy(buf, &tag_stats, min((int) sizeof(TAG_STATS), buflen));
            break;
        }

        case FILE_STAT:
        {
            if (idx >= task->size_ft)
                return (dberr(S_INVID));

            memcpy(buf, &task->file_stats[idx], sizeof(FILE_STATS));
            break;
        }

        case MSG_STAT:
        {
            if (idx >= L_LAST)
                return (dberr(S_INVID));

            memcpy(buf, &task->msg_stats[idx], sizeof(MSG_STATS));
            break;
        }

        default:
            return (dberr(S_INVID));
    }

    return (task->db_status);
#else
    return (dberr(S_NOTIMPLEMENTED));
#endif
}

#ifdef DBSTAT

void sync_MEM_STATS(MEM_STATS *dest, MEM_STATS *src)
{
    dest->mem_used = src->mem_used;
    if (dest->max_mem < src->max_mem)
        dest->max_mem = src->max_mem;

    dest->allocs += src->allocs; src->allocs = 0;
}

void sync_CACHE_STATS(CACHE_STATS *dest, CACHE_STATS *src)
{
    dest->lookups += src->lookups; src->lookups = 0;
    dest->hits += src->hits;       src->hits = 0;
    dest->num_pages = src->num_pages;
}

void INTERNAL_FCN STAT_mem_alloc(PAGE_TABLE *cache, size_t size)
{
    cache->mem_stats.allocs++;
    cache->mem_stats.mem_used += size;
    if (cache->mem_stats.mem_used > cache->mem_stats.max_mem)
        cache->mem_stats.max_mem = cache->mem_stats.mem_used;
}

static void INTERNAL_FCN _STAT_lookups(CACHE_STATS *cache_stats)
{
    if (cache_stats->lookups > LONG_MAX)
    {
        cache_stats->lookups >>= 1;
        cache_stats->hits >>= 1;
    }

    cache_stats->lookups++;
}

void INTERNAL_FCN STAT_lookups(FILE_NO fno, DB_TASK *task)
{
    if (fno != task->ov_file)
    {
        _STAT_lookups(&task->file_stats[fno].cache_stats);
        _STAT_lookups(&task->cache->db_pgtab.cache_stats);
    }
    else
        _STAT_lookups(&task->cache->ix_pgtab.cache_stats);
}

void INTERNAL_FCN STAT_hits(FILE_NO fno, DB_TASK *task)
{
    if (fno != task->ov_file)
    {
        task->file_stats[fno].cache_stats.hits++;
        task->cache->db_pgtab.cache_stats.hits++;
    }
    else
        task->cache->ix_pgtab.cache_stats.hits++;
}

void INTERNAL_FCN STAT_pages(FILE_NO fno, short num, DB_TASK *task)
{
    if (fno != task->ov_file)
    {
        task->file_stats[fno].cache_stats.num_pages += num;
        task->cache->db_pgtab.cache_stats.num_pages += num;
    }
    else
        task->cache->ix_pgtab.cache_stats.num_pages += num;
}

void INTERNAL_FCN STAT_file_open(FILE_NO fno, DB_TASK *task)
{
    task->file_stats[fno].file_opens++;
    task->gen_stats.file_opens++;
}

void INTERNAL_FCN STAT_pz_read(FILE_NO fno, size_t amt, DB_TASK *task)
{
    task->file_stats[fno].pz_stats.read_count++;
    task->file_stats[fno].pz_stats.read_bytes += amt;
    task->gen_stats.pz_stats.read_count++;
    task->gen_stats.pz_stats.read_bytes += amt;
}

void INTERNAL_FCN STAT_pz_write(FILE_NO fno, size_t amt, DB_TASK *task)
{
    task->file_stats[fno].pz_stats.write_count++;
    task->file_stats[fno].pz_stats.write_bytes += amt;
    task->gen_stats.pz_stats.write_count++;
    task->gen_stats.pz_stats.write_bytes += amt;
}

void INTERNAL_FCN STAT_pg_read(FILE_NO fno, size_t amt, DB_TASK *task)
{
    task->file_stats[fno].pg_stats.read_count++;
    task->file_stats[fno].pg_stats.read_bytes += amt;
    task->gen_stats.pg_stats.read_count++;
    task->gen_stats.pg_stats.read_bytes += amt;
}

void INTERNAL_FCN STAT_pg_write(FILE_NO fno, size_t amt, DB_TASK *task)
{
    task->file_stats[fno].pg_stats.write_count++;
    task->file_stats[fno].pg_stats.write_bytes += amt;
    task->gen_stats.pg_stats.write_count++;
    task->gen_stats.pg_stats.write_bytes += amt;
}

void INTERNAL_FCN STAT_rlb_read(FILE_NO fno, size_t amt, DB_TASK *task)
{
    task->file_stats[fno].rlb_stats.read_count++;
    task->file_stats[fno].rlb_stats.read_bytes += amt;
    task->gen_stats.rlb_stats.read_count++;
    task->gen_stats.rlb_stats.read_bytes += amt;
}

void INTERNAL_FCN STAT_rlb_write(FILE_NO fno, size_t amt, DB_TASK *task)
{
    task->file_stats[fno].rlb_stats.write_count++;
    task->file_stats[fno].rlb_stats.write_bytes += amt;
    task->gen_stats.rlb_stats.write_count++;
    task->gen_stats.rlb_stats.write_bytes += amt;
}

void INTERNAL_FCN STAT_new_page(FILE_NO fno, DB_TASK *task)
{
    task->file_stats[fno].new_pages++;
    task->gen_stats.new_pages++;
}

void INTERNAL_FCN STAT_log_open(DB_TASK *task)
{
    task->gen_stats.log_opens++;
}

void INTERNAL_FCN STAT_log_read(size_t amt, DB_TASK *task)
{
    task->gen_stats.log_stats.read_count++;
    task->gen_stats.log_stats.read_bytes += amt;
}

void INTERNAL_FCN STAT_log_write(size_t amt, DB_TASK *task)
{
    task->gen_stats.log_stats.write_count++;
    task->gen_stats.log_stats.write_bytes += amt;
}

void INTERNAL_FCN STAT_taf_open(DB_TASK *task)
{
    task->gen_stats.taf_opens++;
}

void INTERNAL_FCN STAT_taf_read(size_t amt, DB_TASK *task)
{
    task->gen_stats.taf_stats.read_count++;
    task->gen_stats.taf_stats.read_bytes += amt;
}

void INTERNAL_FCN STAT_taf_write(size_t amt, DB_TASK *task)
{
    task->gen_stats.taf_stats.write_count++;
    task->gen_stats.taf_stats.write_bytes += amt;
}

void INTERNAL_FCN STAT_dbl_open(DB_TASK *task)
{
    task->gen_stats.dbl_opens++;
}

void INTERNAL_FCN STAT_dbl_read(size_t amt, DB_TASK *task)
{
    task->gen_stats.dbl_stats.read_count++;
    task->gen_stats.dbl_stats.read_bytes += amt;
}

void INTERNAL_FCN STAT_dbl_write(size_t amt, DB_TASK *task)
{
    task->gen_stats.dbl_stats.write_count++;
    task->gen_stats.dbl_stats.write_bytes += amt;
}

void INTERNAL_FCN STAT_max_open(int num, DB_TASK *task)
{
    if (num > task->gen_stats.max_files_open)
        task->gen_stats.max_files_open = num;
}

void INTERNAL_FCN STAT_trbegin(DB_TASK *task)
{
    task->gen_stats.trbegins++;
}

void INTERNAL_FCN STAT_trend(DB_TASK *task)
{
    task->gen_stats.trends++;
    if (task->cache_ovfl)
        task->gen_stats.trovfl++;
}

void INTERNAL_FCN STAT_trabort(DB_TASK *task)
{
    task->gen_stats.traborts++;
}

void INTERNAL_FCN STAT_lock(FILE_NO fno, int type, DB_TASK *task)
{
    ((DB_ULONG *)&task->file_stats[fno].lock_stats)[type]++;
    ((DB_ULONG *)&task->gen_stats.lock_stats)[type]++;
}

void INTERNAL_FCN STAT_send_msg(int mtype, size_t msglen, size_t tot_pkts,
                                DB_TASK *task)
{
    MSG_STATS *mstat = &task->msg_stats[mtype];
    size_t len = msglen;
    size_t pkts = tot_pkts;

#ifdef DB_DEBUG
    if (mtype > L_LAST)
    {
        dberr(S_SYSERR);
        return;
    }
#endif

    mstat->msg_count++;
    mstat->send_packets += pkts;
    mstat->send_bytes += len;

    mstat = &task->gen_stats.msg_stats;
    mstat->msg_count++;
    mstat->send_packets += pkts;
    mstat->send_bytes += len;
}

void INTERNAL_FCN STAT_recv_msg(int mtype, size_t msglen, size_t tot_pkts,
                                DB_TASK *task)
{
    MSG_STATS *mstat = &task->msg_stats[mtype];
    size_t len = msglen;
    size_t pkts = tot_pkts;

#ifdef DB_DEBUG
    if (mtype > L_LAST)
    {
        dberr(S_SYSERR);
        return;
    }
#endif

    mstat->recv_packets += pkts;
    mstat->recv_bytes += len;

    mstat = &task->gen_stats.msg_stats;
    mstat->recv_packets += pkts;
    mstat->recv_bytes += len;
}

#endif


