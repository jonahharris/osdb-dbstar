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

/* ======================================================================
    Check a field for permission to change it
*/
int INTERNAL_FCN r_chkfld(
    short         field,     /* task->field_table entry number */
    FIELD_ENTRY  *fld_ptr,   /* corresponds to field */
    char         *rec,       /* pointer to record slot */
    const char   *data,      /* ptr to data area containing fld contents */
    DB_TASK      *task)
{
    DB_ADDR       dba;
    long          fld;
    short         rn;
    char         *fptr;
    char          ckey[MAXKEYSIZE];
    short         i;
    FIELD_ENTRY  *sfld_ptr;
    RECORD_ENTRY *rec_ptr;
    int           cmp;

    memcpy(&rn, rec, sizeof(short));
    rn &= ~RLBMASK;                     /* mask off rlb */
    if (rn != NUM2EXT(fld_ptr->fd_rec, rt_offset))
        return (dberr(S_INVFLD));

    rec_ptr = &task->record_table[fld_ptr->fd_rec];
    fld = FLDMARK * rn + field - rec_ptr->rt_fields;

    if (fld_ptr->fd_type == COMKEY)
    {
        /* build compound key value. NOTE: cflag MUST be the same here as
           for the call to key_bldcom in recwrite, which calls this
           function.
        */
        fptr = rec + rec_ptr->rt_data;
        key_bldcom(field, fptr, ckey, FALSE, task);
        fptr = ckey;
    }
    else
        fptr = rec + fld_ptr->fd_ptr;

    /* do nothing unless the new value is different */
    if (fld_ptr->fd_key != NOKEY)
        cmp = fldcmp(fld_ptr, data, fptr, task);
    else if (fld_ptr->fd_type == CHARACTER && fld_ptr->fd_dim[1] == 0)
        cmp = strncmp(data, fptr, fld_ptr->fd_len);
    else if (fld_ptr->fd_type == WIDECHAR && fld_ptr->fd_dim[1] == 0)
    {
        cmp = vwcsncmp((const wchar_t *)data, (const wchar_t *)fptr,
              fld_ptr->fd_len / sizeof(wchar_t));
    }
    else
        cmp = memcmp(data, fptr, fld_ptr->fd_len);

    if (cmp == 0)
        return (task->db_status);

    /* if this is a unique key field, make sure the key does not
       already exist
    */
    if (fld_ptr->fd_key == UNIQUE)
    {
        dba = task->curr_rec;

        /* If the key field is not optional, or optional and stored */
        if (!(fld_ptr->fd_flags & OPTKEYMASK) || r_tstopt(fld_ptr, rec, task))
        {
            if (dkeyfind(fld, data, task, task->curr_db) == S_OKAY)
            {
                /* another record is already using this key value */
                task->db_status = S_DUPLICATE;
            }
            else if (task->db_status == S_NOTFOUND)
                task->db_status = S_OKAY;
        }

        task->curr_rec = dba;
        if (task->db_status == S_DUPLICATE)
            return task->db_status;
    }

    /* if field is grouped, call r_chkfld for 1st entry of each sub-field */
    if (fld_ptr->fd_type == GROUPED)
    {
        for ( i = (short) (field + 1), sfld_ptr = fld_ptr + 1;
              (i < task->size_fd) && (sfld_ptr->fd_flags & STRUCTFLD);
              ++i, ++sfld_ptr)
        {
            fptr = (char *) data + (sfld_ptr->fd_ptr - fld_ptr->fd_ptr);
            if (r_chkfld(i, sfld_ptr, rec, fptr, task) != S_OKAY)
                return task->db_status;
        }
    }

    return (task->db_status);
}


/* ======================================================================
    Delete the current record
*/
int INTERNAL_FCN r_delrec(
    short    rt,               /* record table index */
    DB_ADDR  db_addr,
    DB_TASK *task)
{
    char         *rec;                   /* ptr to record slot */
    char         *fptr;                  /* field data ptr */
    char          ckey[MAXKEYSIZE];      /* compound key data */
    DB_ULONG      timestamp;
    short         fno;
    DB_ULONG      rno;
    int           stat;
    int           fld;
    RECORD_ENTRY *rec_ptr;
    FIELD_ENTRY  *fld_ptr;

    if (dio_read(db_addr, &rec, PGHOLD, task) != S_OKAY)
        return task->db_status;

    rec_ptr = &task->record_table[rt];

    /* remove any key fields from the key files */
    for ( fld = rec_ptr->rt_fields, fld_ptr = &task->field_table[fld];
          (fld < task->size_fd) && (fld_ptr->fd_rec == rt);
          ++fld, ++fld_ptr)
    {
        if (fld_ptr->fd_key == NOKEY)
            continue;

        if (fld_ptr->fd_type == COMKEY)
        {
            key_bldcom(fld, rec + rec_ptr->rt_data, ckey, TRUE, task);
            fptr = ckey;
        }
        else
            fptr = rec + fld_ptr->fd_ptr;

        /* delete the key if it exists */
        if ((!(fld_ptr->fd_flags & OPTKEYMASK) || r_tstopt(fld_ptr, rec, task)) &&
            key_delete(fld, fptr, db_addr, task) != S_OKAY)
        {
            stat = task->db_status;
            if (dio_release(db_addr, PGFREE, task) != S_OKAY)
                return task->db_status;

            return (task->db_status = stat);
        }
    }

    DECODE_DBA(db_addr, &fno, &rno);
    fno = (FILE_NO) NUM2INT(fno, ft_offset);

    /* update timestamp, if necessary */
    if (rec_ptr->rt_flags & TIMESTAMPED)
    {
        timestamp = dio_pzgetts(fno, task);
        memcpy(rec + RECCRTIME, &timestamp, sizeof(DB_ULONG));
        memcpy(rec + RECUPTIME, &timestamp, sizeof(DB_ULONG));
    }

    if (dio_write(db_addr, PGFREE, task) != S_OKAY)
        return task->db_status;

    /* place this record onto the delete chain */
    dio_pzdel(fno, rno, task);

    return task->db_status;
}


/* ======================================================================
    Get data field from record
*/
int INTERNAL_FCN r_gfld(
    FIELD_ENTRY     *fld_ptr,
    char            *rec,
    char            *data,
    DB_TASK *task)
{
    short        rn;
    int          kt_lc;      /* loop control */
    FIELD_ENTRY *kfld_ptr;
    KEY_ENTRY   *key_ptr;

    memcpy(&rn, rec, sizeof(short));
    if (rn < 0)
        return (task->db_status = S_DELETED);

    if (rn & RLBMASK)
    {
        rn &= ~RLBMASK;                  /* mask off rlb */
        task->rlb_status = S_LOCKED;
    }
    else
        task->rlb_status = S_UNLOCKED;

    rn += task->curr_db_table->rt_offset;

    if (fld_ptr->fd_rec != rn)
        return (dberr(S_INVFLD));

    switch (fld_ptr->fd_type)
    {
        case COMKEY:
            /* clear compound key data area */
            memset(data, '\0', fld_ptr->fd_len);

            /* copy each field of compound key to data area */
            for ( kt_lc = task->size_kt - fld_ptr->fd_ptr,
                  key_ptr = &task->key_table[fld_ptr->fd_ptr]; (--kt_lc >= 0) &&
                  (&task->field_table[key_ptr->kt_key] == fld_ptr); ++key_ptr)
            {
                kfld_ptr = &task->field_table[key_ptr->kt_field];
                memcpy(data + key_ptr->kt_ptr, rec + kfld_ptr->fd_ptr,
                          kfld_ptr->fd_len);
            }

            break;

        default:
            memcpy(data, rec + fld_ptr->fd_ptr, fld_ptr->fd_len);
            break;
    }

    return (task->db_status);
}


/* ======================================================================
    Get member pointer from record
*/
int INTERNAL_FCN r_gmem(
    int      set,       /* set table entry number */
    char    *rec,       /* pointer to record */
    MEM_PTR *mem_addr,  /* pointer to member pointer */
    DB_TASK *task)
{
    short         rt;
    int           mem;
    int           memtot;
    SET_ENTRY    *set_ptr;
    MEMBER_ENTRY *mem_ptr;

    /* search member list of set for record */
    set_ptr = &task->set_table[set];
    memcpy(&rt, rec, sizeof(short));
    rt &= ~RLBMASK;
    for ( mem = set_ptr->st_members, memtot = mem + set_ptr->st_memtot,
          mem_ptr = &task->member_table[mem]; mem < memtot; ++mem, ++mem_ptr)
    {
        if (NUM2EXT(mem_ptr->mt_record, rt_offset) == rt)
        {
            /* have found correct member record */
            memcpy(mem_addr, rec + mem_ptr->mt_mem_ptr, MEMPSIZE);
            return (task->db_status);
        }
    }

    /* this record is not member of set */
    return (dberr(S_INVMEM));
}


/* ======================================================================
    Get set pointer from record
*/
int INTERNAL_FCN r_gset(
    int      set,         /* set table entry number */
    char    *rec,         /* pointer to record */
    SET_PTR *setptr,      /* pointer to set pointer */
    DB_TASK *task)
{
    short      rt;
    int        len;
    SET_ENTRY *set_ptr;

    set_ptr = &task->set_table[set];
    memcpy(&rt, rec, sizeof(short));
    if (NUM2EXT(set_ptr->st_own_rt, rt_offset) == (rt & ~RLBMASK))
    {
        if (set_ptr->st_flags & TIMESTAMPED)
            len = SETPSIZE;
        else
            len = SETPSIZE - sizeof(DB_ULONG);

        memcpy(setptr, rec + set_ptr->st_own_ptr, len);
        return (task->db_status);
    }

    return (dberr(S_INVOWN));
}


/* ======================================================================
    Put data field into record
*/
int INTERNAL_FCN r_pfld(
    short         field,     /* task->field_table entry number */
    FIELD_ENTRY  *fld_ptr,   /* corresponds to field */
    char         *rec,       /* pointer to existing record */
    const char   *data,      /* ptr to new fld contents */
    DB_ADDR      *db_addr,
    DB_TASK      *task)
{
    FILE_ENTRY   *file_ptr;
    int           file;
    DB_ADDR       mdba;
    DB_ADDR       odba;
    DB_ADDR       dba;
    int           cmp;
    int           set;
    int           sn;
    MEM_PTR       memp;
    DB_ADDR      *co_ptr;
    DB_ADDR      *cm_ptr;
    unsigned long co_ts = 0;
    unsigned long cm_ts = 0;                 /* save the timestamps */
    int           s;
    int           strfld;
    short         i;
    const char   *tfptr;
    FIELD_ENTRY  *sfld_ptr = NULL;
    SORT_ENTRY   *srt_ptr;
    char         *fptr;

    fptr = rec + fld_ptr->fd_ptr;
    if (fld_ptr->fd_type == CHARACTER && fld_ptr->fd_dim[1] == 0)
        cmp = strncmp(fptr, data, fld_ptr->fd_len);
    else if (fld_ptr->fd_type == WIDECHAR && fld_ptr->fd_dim[1] == 0)
    {
        cmp = vwcsncmp((const wchar_t *)fptr, (const wchar_t *)data,
                fld_ptr->fd_len / sizeof(wchar_t));
    }
    else
        cmp = memcmp(fptr, data, fld_ptr->fd_len);

    if (cmp == 0)
        return task->db_status;

    memcpy(&dba, db_addr, DB_ADDR_SIZE);

    /* if this field is part of an ordered set, check locks */
    if ((task->dbopen == 1) && cmp && (fld_ptr->fd_flags & SORTFLD))
    {
        for (s = 0, srt_ptr = task->sort_table; s < task->size_srt; ++s, ++srt_ptr)
        {
            if (srt_ptr->se_fld != field)
                continue;

            sn = srt_ptr->se_set;
            if (r_gmem(sn, rec, &memp, task) != S_OKAY)
                return task->db_status;

            if (null_dba(memp.owner))
                continue;
        
            /* Owner record's file */
            file = task->record_table[task->set_table[sn].st_own_rt].rt_file;
            file_ptr = &task->file_table[file];

            /* check shared access privileges */
            if ((!task->app_locks[file]) && !task->excl_locks[file] &&
                !(file_ptr->ft_flags & STATIC))
                return (dberr(S_NOTLOCKED));
        }
    }

    /* if this is a key field, change the key file also */
    if (cmp && fld_ptr->fd_key != NOKEY && (!(fld_ptr->fd_flags & OPTKEYMASK) ||
        r_tstopt(fld_ptr, rec, task)))
    {
        /* delete the old key and insert the new one */
        if (key_delete(field, fptr, dba, task) == S_OKAY)
        {
            if (key_insert(field, data, dba, task) != S_OKAY)
                return (task->db_status);
        }
        else
        {
            if (task->db_status == S_NOTFOUND)
                dberr(S_KEYERR);
            return (task->db_status);
        }
    }

    /* if subfield of struct field, check to see if struct is a key */
    if (task->struct_key_chk && fld_ptr->fd_flags & STRUCTFLD)
    {
        strfld = field - 1;
        for (sfld_ptr = &task->field_table[strfld];
             sfld_ptr->fd_type != GROUPED;
             --sfld_ptr)
        {
            /* find struct field */
            --strfld;
        }

        /* make sure it is stored */
        if (cmp && sfld_ptr->fd_key != NOKEY &&
            (!(sfld_ptr->fd_flags & OPTKEYMASK) ||
            r_tstopt(sfld_ptr, rec, task)))
        {
            /* delete the old struct key */
            if (key_delete(strfld, rec + sfld_ptr->fd_ptr, dba, task) != S_OKAY)
                return task->db_status;
        }
        else
            strfld = -1;
    }
    else
        strfld = -1;

    /* copy data into record area */
    switch (fld_ptr->fd_type)
    {
        case CHARACTER:
            if (fld_ptr->fd_dim[1])
                memcpy(fptr, data, fld_ptr->fd_len);
            else if (fld_ptr->fd_dim[0])
                strncpy(fptr, data, fld_ptr->fd_len);
            else
                *fptr = (char) *data;

            break;

        case WIDECHAR:
            if (fld_ptr->fd_dim[0] > 0 && fld_ptr->fd_dim[1] == 0)
            {
                vwcsncpy((wchar_t *)fptr, (wchar_t *)data,
                        fld_ptr->fd_len / sizeof(wchar_t));
            }
            else
                memcpy(fptr, data, fld_ptr->fd_len);

            break;

        case GROUPED:
            if (! fld_ptr->fd_dim[0])
            {
                /* non-arrayed structure */
                task->struct_key_chk = 0;
                for (i = (short) (field + 1), sfld_ptr = fld_ptr + 1;
                     (i < task->size_fd) && (sfld_ptr->fd_flags & STRUCTFLD);
                     ++i, ++sfld_ptr)
                {
                    tfptr = (char *) data + sfld_ptr->fd_ptr - fld_ptr->fd_ptr;
                    if (r_pfld(i, sfld_ptr, rec, tfptr, &dba, task) != S_OKAY)
                        break;
                }

                task->struct_key_chk = 1;
                if (task->db_status != S_OKAY)
                    return task->db_status;

                break;
            }
            /* arrayed struct fall-thru to a full field copy */

        default:
            memcpy(fptr, data, fld_ptr->fd_len);
            break;
    }

    /* if this field is part of an ordered set, reconnect */
    if (cmp && (fld_ptr->fd_flags & SORTFLD))
    {
        for (s = 0, srt_ptr = task->sort_table; s < task->size_srt; ++s, ++srt_ptr)
        {
            if (srt_ptr->se_fld != field)
                continue;
        
            sn = srt_ptr->se_set;
            if (r_gmem(sn, rec, &memp, task) != S_OKAY)
                return task->db_status;

            if (null_dba(memp.owner))
                continue;
        
            /* save currency */
            odba = *(co_ptr = &task->curr_own[sn]);
            mdba = *(cm_ptr = &task->curr_mem[sn]);
            if (task->db_tssets)
                co_ts = task->cs_time[sn];

            if (task->db_tsrecs)
                cm_ts = task->cm_time[sn];

            *co_ptr = memp.owner;
            *cm_ptr = dba;

            /* calculate set constant */
            set = NUM2EXT(sn + SETMARK, st_offset);

            /* disconnect from prior order set and reconnect
               in new order
            */
            if (ddiscon(set, task, task->curr_db) < S_OKAY)
                return task->db_status;  /* S_EOS is good, others not possible */

            task->db_status = S_OKAY;

            if (dconnect(set, task, task->curr_db) != S_OKAY)
                return task->db_status;

            /* reset currency */
            *co_ptr = odba;
            *cm_ptr = mdba;
            if (task->db_tssets)
                task->cs_time[sn] = co_ts;

            if (task->db_tsrecs)
                task->cm_time[sn] = cm_ts;
        }
    }

    if (cmp && (strfld >= 0))
    {
        /* insert the new struct key */
        if (key_insert(strfld, rec + sfld_ptr->fd_ptr, dba, task) != S_OKAY)
            return task->db_status;
    }

    return task->db_status;
}


/* ======================================================================
    Put member pointer into record
*/
int INTERNAL_FCN r_pmem(
    int      set,            /* set table entry number */
    char    *rec,            /* pointer to record */
    char    *mem_addr,       /* pointer to member pointer */
    DB_TASK *task)
{
    short         rt;
    int           mem;
    int           memtot;
    SET_ENTRY    *set_ptr;
    MEMBER_ENTRY *mem_ptr;

    /* search member list of set for record */
    set_ptr = &task->set_table[set];
    memcpy(&rt, rec, sizeof(short));
    rt &= ~RLBMASK;
    for (mem = set_ptr->st_members, memtot = mem + set_ptr->st_memtot,
         mem_ptr = &task->member_table[mem]; mem < memtot; ++mem, ++mem_ptr)
    {
        if (NUM2EXT(mem_ptr->mt_record, rt_offset) == rt)
        {
            /* have found correct member record */
            memcpy(rec + mem_ptr->mt_mem_ptr, mem_addr, MEMPSIZE);
            return (task->db_status);
        }
    }

    /* this record is not member of set */
    return (dberr(S_INVMEM));
}


/* ======================================================================
    Put set pointer into record
*/
int INTERNAL_FCN r_pset(
    int      set,          /* set table entry number */
    char    *rec,          /* pointer to record */
    char    *setptr,       /* pointer to set pointer */
    DB_TASK *task)
{
    short      rt;
    int        len;
    SET_ENTRY *set_ptr;

    set_ptr = &task->set_table[set];
    memcpy(&rt, rec, sizeof(short));
    if (NUM2EXT(set_ptr->st_own_rt, rt_offset) == (rt & ~RLBMASK))
    {
        if (set_ptr->st_flags & TIMESTAMPED)
            len = SETPSIZE;
        else
            len = SETPSIZE - sizeof(DB_ULONG);

        memcpy(rec + set_ptr->st_own_ptr, setptr, len);
        return (task->db_status);
    }

    return (dberr(S_INVOWN));
}


/* ======================================================================
    Set the current set member from record
*/
int INTERNAL_FCN r_smem(
    DB_ADDR *db_addr,
    int      set,
    DB_TASK *task)
{
    int     nset;
    MEM_PTR mem;
    char   *ptr;
    DB_ADDR dba;
    int     stat;

    memcpy(&dba, db_addr, DB_ADDR_SIZE);

    /* make sure record is owned */
    if (dio_read(dba, &ptr, NOPGHOLD, task) != S_OKAY)
        return task->db_status;

    if ((stat = r_gmem(set, ptr, &mem, task)) != S_OKAY)
    {
        if (dio_release(dba, NOPGFREE, task) != S_OKAY)
            return task->db_status;

        return (task->db_status = stat);
    }

    if (null_dba(mem.owner))
        return (dberr(S_NOTCON));

    task->curr_own[set] = mem.owner;

    if (dio_release(dba, NOPGFREE, task) != S_OKAY)
        return task->db_status;

    /* ownership okay, set the member */
    task->curr_mem[set] = dba;

    nset = NUM2EXT(set + SETMARK, st_offset);

    /* set timestamps */
    if (task->db_tsrecs)
    {
        dutsco(nset, &task->co_time[set], task, task->curr_db);
        dutscm(nset, &task->cm_time[set], task, task->curr_db);
    }

    if (task->db_tssets)
        dutscs(nset, &task->cs_time[set], task, task->curr_db);

    return (task->db_status);
}

/* ======================================================================
    Set the optional key field "stored" bit
*/
int INTERNAL_FCN r_setopt(
    FIELD_ENTRY *fld_ptr,    /* field table entry of optional key */
    char        *rec,        /* Pointer to record */
    DB_TASK     *task)
{
    int offset;                      /* offset to the bit map */
    int keyndx;                      /* index into bit map of this key */
    int byteno, bitno;               /* position within bit map of this key */

    /* calculate the position to the bit map */
    offset = (task->record_table[fld_ptr->fd_rec].rt_flags & TIMESTAMPED) ?
            (RECHDRSIZE + 2 * sizeof(long)) : RECHDRSIZE;

    /* extract the index into the bit map of this key */
    keyndx = (((fld_ptr->fd_flags & OPTKEYMASK) >> OPTKEYSHIFT) & OPTKEYNDX) - 1;
    if (keyndx < 0)
        return (dberr(SYS_BADOPTKEY));

    /* determine which byte, and which bit within the byte */
    byteno = keyndx / BITS_PER_BYTE;
    bitno = keyndx - byteno * BITS_PER_BYTE;

    /* set the bit */
    rec[byteno + offset] |= 1 << (BITS_PER_BYTE - bitno - 1);

    return (task->db_status);
}

/* ======================================================================
    Clear the optional key field "stored" bit
*/
int INTERNAL_FCN r_clropt(
    FIELD_ENTRY *fld_ptr,    /* Field table entry of optional key */
    char        *rec,        /* Pointer to record */
    DB_TASK     *task)
{
    int offset;                      /* offset to the bit map */
    int keyndx;                      /* index into bit map of this key */
    int byteno, bitno;               /* position within bit map of this key */

    /* calculate the position to the bit map */
    offset = (task->record_table[fld_ptr->fd_rec].rt_flags & TIMESTAMPED) ?
            (RECHDRSIZE + 2 * sizeof(long)) : RECHDRSIZE;

    /* extract the index into the bit map of this key */
    keyndx = (((fld_ptr->fd_flags & OPTKEYMASK) >> OPTKEYSHIFT) & OPTKEYNDX) - 1;
    if (keyndx < 0)
        return (dberr(SYS_BADOPTKEY));

    /* determine which byte, and which bit within the byte */
    byteno = keyndx / BITS_PER_BYTE;
    bitno = keyndx - byteno * BITS_PER_BYTE;

    /* clear the bit */
    rec[byteno + offset] &= ~(1 << (BITS_PER_BYTE - bitno - 1));

    return S_OKAY;
}

/* ======================================================================
    Test the optional key field "stored" bit
*/
int EXTERNAL_FCN r_tstopt(
    FIELD_ENTRY *fld_ptr,    /* Field table entry of optional key */
    char        *rec,        /* Pointer to record */
    DB_TASK     *task)
{
    int offset;                      /* offset to the bit map */
    int keyndx;                      /* index into bit map of this key */
    int byteno, bitno;               /* position within bit map of this key */

    /* calculate the position to the bit map */
    offset = (task->record_table[fld_ptr->fd_rec].rt_flags & TIMESTAMPED) ?
            (RECHDRSIZE + 2 * sizeof(long)) : RECHDRSIZE;

    /* extract the index into the bit map of this key */
    keyndx = (((fld_ptr->fd_flags & OPTKEYMASK) >> OPTKEYSHIFT) & OPTKEYNDX) - 1;
    if (keyndx < 0)
        return (dberr(SYS_BADOPTKEY));

    /* determine which byte, and which bit within the byte */
    byteno = keyndx / BITS_PER_BYTE;
    bitno = keyndx - byteno * BITS_PER_BYTE;

    /* extract the bit */
    if (rec[byteno + offset] & (1 << (BITS_PER_BYTE - bitno - 1)))
        return (task->db_status = S_DUPLICATE);

    return (task->db_status);
}


