/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, dal utility                                       *
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

    dalexec.c - Function to execute a compiled DAL command.

    This function will interpret and execute one DAL command.  The
    command may be a WHILEOK loop, and thus may be any number of
    commands.

-----------------------------------------------------------------------*/

/* ********************** INCLUDE FILES ****************************** */
#include "db.star.h"
#include "daldef.h"
#include "dalvar.h"

#define DAL
#include "getnames.h"

static char get_fdtype(int, DB_TASK *);


int dalexec(INST *pi)
{
    int f, stat, setnum1, setnum2, recnum, intval, intval2, ndx, ndx2;
    int fldnum, fldnum2;
    DB_TCHAR *lptr, *lptr2;
    char *recptr, *dbaptr, *fldptr, *intptr, *ctptr, *ctptr2, *ctptr3;
    DB_TASK *task = DalDbTask;    /* required for macros, e.g. task->size_rt */
#if defined(UNICODE)
    char     cbuffer[FILENMLEN];
#else
    wchar_t  wbuffer[FILENMLEN];
#endif

    stat = S_OKAY;

    /* search for the function name */
    for (f = 0; f < nfcns; f++)
    {
        if (vtstrcmp(pi->i_name, DB_TEXT("print")) == 0 ||
            vtstrcmp(pi->i_name, DB_TEXT("input")) == 0)
        {
            stat = dalio(pi);
            return (stat);
        }
        if (vtstrcmp(pi->i_name, DB_TEXT("currency")) == 0)
        {
            stat = dalcurr(stdout);
            return (stat);
        }
        if (vtstrcmp(pi->i_name, DB_TEXT("rewind")) == 0)
        {
            stat = dal_rewind(pi);
            return (stat);
        }
        if (vtstrncmp(pi->i_name, fcns[f].f_name, vtstrlen(fcns[f].f_name)) == 0)
        {
            switch (fcns[f].f_fcntype)
            {
                case N_TSK:
                    if (vtstrcmp(fcns[f].f_name, DB_TEXT("clo")) == 0)
                    {
                        freevar();
                    }

                    stat = (*((int (EXTERNAL_FCN *) (DB_TASK *)) fcns[f].fcn)) (DalDbTask);
                    break;

                case N_TSK_DBN:
                    if (vtstrcmp(fcns[f].f_name, DB_TEXT("clo")) == 0)
                    {
                        /* free the schema values arrays */
                        freevar();
                    }

                    stat = (*((int (EXTERNAL_FCN *) (DB_TASK *, int)) fcns[f].fcn)) (DalDbTask, CURR_DB);
                    break;

                case I_TSK:
                    vstscanf(pi->i_p1, DB_TEXT("%d"), &intval);

                    stat = (*((int (EXTERNAL_FCN *) (int, DB_TASK *)) fcns[f].fcn)) (intval, DalDbTask);
                    break;

                case I_TSK_DBN:
                    vstscanf(pi->i_p1, DB_TEXT("%d"), &intval);

                    stat = (*((int (EXTERNAL_FCN *) (int, DB_TASK *, int)) fcns[f].fcn)) (intval, DalDbTask, CURR_DB);
                    break;

                case I_I_TSK:
                    vstscanf(pi->i_p1, DB_TEXT("%d"), &intval);
                    vstscanf(pi->i_p2, DB_TEXT("%d"), &intval2);

                    stat = (*((int (EXTERNAL_FCN *) (int, int, DB_TASK *)) fcns[f].fcn)) (intval, intval2, DalDbTask);
                    break;

                case L_TSK:
                    if ((lptr = (DB_TCHAR *)findvar(LITERAL, pi->i_p1, &ndx)) == NULL)
                    {
                        dalerror(DB_TEXT("Paramter 1 is invalid character string"));
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (DB_TCHAR *, DB_TASK *)) fcns[f].fcn)) (lptr, DalDbTask);
                    break;

                case L_L_SG_TSK:
                    if ((lptr = (DB_TCHAR *)findvar(LITERAL, pi->i_p1, &ndx)) == NULL)
                    {
                        dalerror(DB_TEXT("Paramter 1 is invalid character string"));
                        break;
                    }
                    if ((lptr2 = (DB_TCHAR *)findvar(LITERAL, pi->i_p2, &ndx2)) == NULL)
                    {
                        dalerror(DB_TEXT("Paramter 2 is invalid character string"));
                        break;
                    }

                    /* d_open is the only function with L_L_SG_TSK parameters */
                    stat = d_on_opt(READNAMES, DalDbTask);
                    if (stat == S_OKAY)
                        stat = (*((int (EXTERNAL_FCN *) (DB_TCHAR *, DB_TCHAR *, SG *, DB_TASK *)) fcns[f].fcn)) (lptr, lptr2, dal_sg, DalDbTask);
                    break;

                case S_TSK_DBN:
                    setnum1 = getset(pi->i_p1, DalDbTask);
                    if (setnum1 >= 0)
                        stat = (*((int (EXTERNAL_FCN *) (int, DB_TASK *, int)) fcns[f].fcn)) (set_const(setnum1), DalDbTask, CURR_DB);
                    else
                    {
                        stat = S_INVSET;
                        dalerror(DB_TEXT("Paramter 1 is invalid set name"));
                    }
                    break;

                case F_TSK_DBN:
                    fldnum = (int) getfld(pi->i_p1, -1, DalDbTask);
                    if (fldnum < 0)
                    {
                        dalerror(DB_TEXT("Parameter 1 is invalid field name\n"));
                        stat = S_INVFLD;
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (long, DB_TASK *, int)) fcns[f].fcn)) (fld_const(fldnum, DalDbTask), DalDbTask, CURR_DB);
                    break;

                case R_TSK_DBN:
                    if (vtstrlen(pi->i_f1))
                    {
                        dalerror(DB_TEXT("Parameter 1 - Field name not allowed"));
                        stat = DAL_ERR;
                        break;
                    }
                    recnum = getrec(pi->i_p1, DalDbTask);
                    if (recnum < 0)
                    {
                        stat = S_INVREC;
                        dalerror(DB_TEXT("Parameter 1 is invalid record name"));
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (int, DB_TASK *, int)) fcns[f].fcn)) (rec_const(recnum), DalDbTask, CURR_DB);
                    break;

                case S_S_TSK_DBN:
                    setnum1 = getset(pi->i_p1, DalDbTask);
                    setnum2 = getset(pi->i_p2, DalDbTask);
                    if (setnum1 < 0)
                    {
                        stat = S_INVSET;
                        dalerror(DB_TEXT("Parameter 1 is invalid set name"));
                        break;
                    }
                    else if (setnum2 < 0)
                    {
                        stat = S_INVSET;
                        dalerror(DB_TEXT("Parameter 2 is invalid set name"));
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (int, int, DB_TASK *, int)) fcns[f].fcn)) (set_const(setnum1),
                            set_const(setnum2), DalDbTask, CURR_DB);
                    break;

                    /* Field type, Literal */
                case S_L_TSK_DBN:
                    setnum1 = getset(pi->i_p1, DalDbTask);
                    if (setnum1 < 0)
                    {
                        stat = S_INVSET;
                        dalerror(DB_TEXT("Parameter 1 is invalid set name"));
                        break;
                    }
                    if ((lptr = (DB_TCHAR *)findvar(LITERAL, pi->i_p2, &ndx2)) == NULL)
                    {
                        dalerror(DB_TEXT("Paramter 2 is invalid character string"));
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (int, DB_TCHAR *, DB_TASK *, int)) fcns[f].fcn)) (set_const(setnum1), lptr, DalDbTask, CURR_DB);
                    break;

                case FP_TSK:
                    if (vtstrlen(pi->i_f1))
                    {
                        if ((recptr = findvar(RECORD, pi->i_p1, &ndx)) == NULL)
                        {
                            dalerror(DB_TEXT("Record variable does not exist!"));
                            stat = DAL_ERR;
                            break;
                        }
                        fldnum = (int) getfld(pi->i_f1, ndx, DalDbTask);
                        if (fldnum < 0)
                        {
                            stat = S_INVFLD;
                            dalerror(DB_TEXT("Parameter 1 contains invalid field name"));
                            break;
                        }
                        fldptr = recptr + task->field_table[fldnum].fd_ptr -
                            task->record_table[ndx].rt_data;
                    }
                    else
                    {
                        if ((fldptr = findvar(FIELD, pi->i_p1, &ndx)) == NULL)
                        {
                            if ((fldptr = findvar(LITERAL, pi->i_p1, &ndx)) == NULL)
                                fldptr = addvar(LITERAL, -1, pi->i_p1, 240);
                        }
                    }

                    stat = (*((int (EXTERNAL_FCN *) (char *, DB_TASK *)) fcns[f].fcn)) (fldptr, DalDbTask);
                    break;

                case RP_TSK_DBN:
                    if (vtstrlen(pi->i_f1))
                    {
                        dalerror(DB_TEXT("Field name not allowed"));
                        stat = DAL_ERR;
                        break;
                    }
                    if (d_crtype(&recnum, DalDbTask, CURR_DB) != S_OKAY)
                    {
                        stat = S_NOCR;
                        dalerror(DB_TEXT("No current record"));
                        break;
                    }
                    recnum -= RECMARK;
                    if ((recptr = findvar(RECORD, pi->i_p1, &ndx)) == NULL)
                        recptr = addvar(RECORD, recnum, pi->i_p1,
                                             task->record_table[recnum].rt_len);
                    else if (recnum != ndx)
                    {
                        dalerror(DB_TEXT("Record of same name and different type exists!"));
                        stat = DAL_ERR;
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (char *, DB_TASK *, int)) fcns[f].fcn)) (recptr, DalDbTask, CURR_DB);
                    break;

                case DP_TSK_DBN:
                    if ((dbaptr = findvar(DBAPTR, pi->i_p1, &ndx)) == NULL)
                        dbaptr = addvar(DBAPTR, -1, pi->i_p1, sizeof(DB_ADDR));

                    stat = (*((int (EXTERNAL_FCN *) (char *, DB_TASK *, int)) fcns[f].fcn)) (dbaptr, DalDbTask, CURR_DB);
                    break;

                case IP_TSK_DBN:
                    if ((intptr = findvar(INTPTR, pi->i_p1, &ndx)) == NULL)
                        intptr = addvar(INTPTR, -1, pi->i_p1, sizeof(int));

                    stat = (*((int (EXTERNAL_FCN *) (char *, DB_TASK *, int)) fcns[f].fcn)) (intptr, DalDbTask, CURR_DB);
                    break;

                case S_IP_TSK_DBN:
                    setnum1 = getset(pi->i_p1, DalDbTask);
                    if (setnum1 < 0)
                    {
                        stat = S_INVSET;
                        dalerror(DB_TEXT("Paramter 1 is invalid set name"));
                        break;
                    }
                    if ((intptr = findvar(INTPTR, pi->i_p2, &ndx)) == NULL)
                        intptr = addvar(INTPTR, -1, pi->i_p2, sizeof(int));

                    stat = (*((int (EXTERNAL_FCN *) (int, char *, DB_TASK *, int)) fcns[f].fcn)) (set_const(setnum1), intptr, DalDbTask, CURR_DB);
                    break;

                case S_DP_TSK_DBN:
                    setnum1 = getset(pi->i_p1, DalDbTask);
                    if (setnum1 < 0)
                    {
                        stat = S_INVSET;
                        dalerror(DB_TEXT("Paramter 1 is invalid set name"));
                        break;
                    }
                    if ((dbaptr = findvar(DBAPTR, pi->i_p2, &ndx)) == NULL)
                        dbaptr = addvar(DBAPTR, -1, pi->i_p2, sizeof(DB_ADDR));

                    stat = (*((int (EXTERNAL_FCN *) (int, char *, DB_TASK *, int)) fcns[f].fcn)) (set_const(setnum1), dbaptr, DalDbTask, CURR_DB);
                    break;

                case R_RP:
                    if (vtstrlen(pi->i_f2))
                    {
                        dalerror(DB_TEXT("Parameter 2 - Field name not allowed"));
                        stat = DAL_ERR;
                        break;
                    }
                    recnum = getrec(pi->i_p1, DalDbTask);
                    if (recnum < 0)
                    {
                        stat = S_INVREC;
                        dalerror(DB_TEXT("Parameter 1 is invalid record name"));
                        break;
                    }
                    if ((recptr = findvar(RECORD, pi->i_p2, &ndx)) == NULL)
                        recptr = addvar(RECORD, recnum, pi->i_p2,
                                        task->record_table[recnum].rt_len);
                    else if (recnum != ndx)
                    {
                        dalerror(DB_TEXT("Record of same name and different type exists!"));
                        stat = DAL_ERR;
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (int, char *)) fcns[f].fcn)) (rec_const(recnum), recptr);
                    break;

                case R_RP_TSK_DBN:
                    if (vtstrlen(pi->i_f2))
                    {
                        dalerror(DB_TEXT("Parameter 2 - Field name not allowed"));
                        stat = DAL_ERR;
                        break;
                    }
                    recnum = getrec(pi->i_p1, DalDbTask);
                    if (recnum < 0)
                    {
                        stat = S_INVREC;
                        dalerror(DB_TEXT("Parameter 1 is invalid record name"));
                        break;
                    }
                    if ((recptr = findvar(RECORD, pi->i_p2, &ndx)) == NULL)
                        recptr = addvar(RECORD, recnum, pi->i_p2,
                                        task->record_table[recnum].rt_len);
                    else if (recnum != ndx)
                    {
                        dalerror(DB_TEXT("Record of same name and different type exists!"));
                        stat = DAL_ERR;
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (int, char *, DB_TASK *, int)) fcns[f].fcn)) (rec_const(recnum), recptr, DalDbTask, CURR_DB);
                    break;

                case F_FP_TSK_DBN:
                    fldnum = (int) getfld(pi->i_p1, -1, DalDbTask);
                    if (fldnum < 0)
                    {
                        stat = S_INVFLD;
                        dalerror(DB_TEXT("Parameter 1 is invalid field name"));
                        break;
                    }
                    if (vtstrlen(pi->i_f2))
                    {
                        if ((recptr = findvar(RECORD, pi->i_p2, &ndx)) == NULL)
                        {
                            dalerror(DB_TEXT("Record variable does not exist!"));
                            stat = DAL_ERR;
                            break;
                        }
                        fldnum2 = (int) getfld(pi->i_f2, ndx, DalDbTask);
                        if (fldnum2 < 0)
                        {
                            stat = S_INVFLD;
                            dalerror(DB_TEXT("Parameter 2 contains invalid field name"));
                            break;
                        }
                        fldptr = recptr + task->field_table[fldnum2].fd_ptr -
                            task->record_table[ndx].rt_data;
                    }
                    else
                    {
                        if ((fldptr = findvar(FIELD, pi->i_p2, &ndx)) == NULL)
                        {
                            if ((fldptr = findvar(LITERAL, pi->i_p2, &ndx)) == NULL)
                            {
                                fldptr = addvar(FIELD, fldnum, pi->i_p2,
                                                task->field_table[fldnum].fd_len);
                            }
#if defined(UNICODE)
                            else if (get_fdtype(fldnum, task) == CHARACTER)
                            {
                                /* field is char, literal is wchar_t */
                                memset(cbuffer, 0, sizeof(cbuffer));
                                wtoa((wchar_t *) fldptr, cbuffer, sizeof(cbuffer));
                                cbuffer[sizeof(cbuffer) - 1] = '\0';
                                fldptr = cbuffer;
#else
                            else if (get_fdtype(fldnum, task) == WIDECHAR)
                            {
                                /* field is wchar_t, literal is char */
                                memset(wbuffer, 0, sizeof(wbuffer));
                                atow(fldptr, wbuffer, sizeof(wbuffer) / sizeof(wchar_t));
                                wbuffer[(sizeof(wbuffer)/sizeof(wchar_t))-1] = 0;
                                fldptr = (char *)wbuffer;
#endif
                            }
                        }
                        else if (fldnum != ndx)
                        {
                            dalerror(DB_TEXT("Field of same name and different type exists!"));
                            stat = DAL_ERR;
                            break;
                        }
                    }

                    stat = (*((int (EXTERNAL_FCN *) (long, char *, DB_TASK *, int)) fcns[f].fcn)) (fld_const(fldnum, DalDbTask), fldptr, DalDbTask, CURR_DB);
                    break;

                case S_F_FP_TSK_DBN:
                    setnum1 = getset(pi->i_p1, DalDbTask);
                    if (setnum1 < 0)
                    {
                        stat = S_INVSET;
                        dalerror(DB_TEXT("Paramter 1 is invalid set name"));
                        break;
                    }
                    fldnum = (int) getfld(pi->i_p2, -1, DalDbTask);
                    if (fldnum < 0)
                    {
                        stat = S_INVFLD;
                        dalerror(DB_TEXT("Parameter 2 is invalid field name"));
                        break;
                    }
                    if (vtstrlen(pi->i_f3))
                    {
                        if ((recptr = findvar(RECORD, pi->i_p3, &ndx)) == NULL)
                        {
                            dalerror(DB_TEXT("Record variable does not exist!"));
                            stat = DAL_ERR;
                            break;
                        }
                        fldnum2 = (int) getfld(pi->i_f3, -1, DalDbTask);
                        if (fldnum2 < 0)
                        {
                            stat = S_INVFLD;
                            dalerror(DB_TEXT("Parameter 2 contains invalid field name"));
                            break;
                        }
                        fldptr = recptr + task->field_table[fldnum].fd_ptr -
                            task->record_table[ndx].rt_data;
                    }
                    else
                    {
                        if ((fldptr = findvar(FIELD, pi->i_p3, &ndx)) == NULL)
                        {
                            if ((fldptr = findvar(LITERAL, pi->i_p3, &ndx)) == NULL)
                                fldptr = addvar(FIELD, fldnum, pi->i_p3,
                                                task->field_table[fldnum].fd_len);
                        }
                        else if (fldnum != ndx)
                        {
                            dalerror(DB_TEXT("Field of same name and different type exists!"));
                            stat = DAL_ERR;
                            break;
                        }
                    }

                    stat = (*((int (EXTERNAL_FCN *) (int, long, char *, DB_TASK *, int)) fcns[f].fcn))
                        (set_const(setnum1), fld_const(fldnum, DalDbTask), fldptr, DalDbTask, CURR_DB);
                    break;

                case U:
                    vtprintf(DB_TEXT("Unimplemented function - %s\n"), pi->i_name);
                    stat = S_OKAY;
                    break;

                    /* Field type, Literal */
                case F_L_TSK_DBN:
                    fldnum = (int) getfld(pi->i_p1, -1, DalDbTask);
                    if (fldnum < 0)
                    {
                        stat = S_INVFLD;
                        dalerror(DB_TEXT("Parameter 1 is invalid field name"));
                        break;
                    }
                    if ((lptr = (DB_TCHAR *)findvar(LITERAL, pi->i_p2, &ndx2)) == NULL)
                    {
                        dalerror(DB_TEXT("Paramter 2 is invalid character string"));
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (long, DB_TCHAR *, DB_TASK *, int)) fcns[f].fcn)) (fld_const(fldnum, DalDbTask), lptr, DalDbTask, CURR_DB);
                    break;

                    /* Integer, Lock Packet */
                case I_LP_TSK_DBN:
                    dalerror(DB_TEXT("Function is not implemented"));
                    break;

                    /* Record type, Literal */
                case R_L_TSK_DBN:
                    recnum = getrec(pi->i_p1, DalDbTask);
                    if (recnum < 0)
                    {
                        stat = S_INVREC;
                        dalerror(DB_TEXT("Parameter 1 is invalid record name"));
                        break;
                    }
                    if ((lptr = (DB_TCHAR *)findvar(LITERAL, pi->i_p2, &ndx2)) == NULL)
                    {
                        dalerror(DB_TEXT("Paramter 2 is invalid character string"));
                        break;
                    }

                    stat = (*((int (EXTERNAL_FCN *) (int, DB_TCHAR *, DB_TASK *, int)) fcns[f].fcn)) (rec_const(recnum), lptr, DalDbTask, CURR_DB);
                    break;

                    /* Currency buffer Pointer, Integer Pointer */
                case CP_IP_TSK_DBN:
                    dalerror(DB_TEXT("Function is not implemented"));
                    break;

                    /* d_renfile function */
                case REN_TSK:
                    dalerror(DB_TEXT("Function is not implemented"));
                    break;

                case CT:
                    if ((ctptr = findvar(CLOCK_T, pi->i_p1, &ndx)) == NULL)
                    {
                        ctptr = addvar(CLOCK_T, -1, pi->i_p1, sizeof(clock_t));
                    }

                    stat = (*((int (EXTERNAL_FCN *) (clock_t *)) fcns[f].fcn)) ((clock_t *)ctptr);
                    break;

                case CT_CT_CT:
                    if ((ctptr = findvar(CLOCK_T, pi->i_p1, &ndx)) == NULL)
                    {
                        dalerror(DB_TEXT("Parameter 1 is not valid"));
                    }
                    if ((ctptr2 = findvar(CLOCK_T, pi->i_p2, &ndx)) == NULL)
                    {
                        dalerror(DB_TEXT("Parameter 2 is not valid"));
                    }
                    if ((ctptr3 = findvar(CLOCK_T, pi->i_p3, &ndx)) == NULL)
                    {
                        ctptr3 = addvar(CLOCK_T, -1, pi->i_p3, sizeof(clock_t));
                    }

                    stat = (*((int (EXTERNAL_FCN *) (clock_t *, clock_t *, clock_t *)) fcns[f].fcn)) ((clock_t *)ctptr, (clock_t *)ctptr2, (clock_t *)ctptr3);
                    break;

                    /* Currency buffer */
                case C_TSK_DBN:
                    dalerror(DB_TEXT("Function is not implemented"));
                    break;
            }
            if (!batch && stat > 0)
            {
                switch (stat)
                {
                    case S_EOS:
                        vtprintf(DB_TEXT("End of set\n"));
                        break;
                    case S_NOTFOUND:
                        vtprintf(DB_TEXT("Record not found\n"));
                        break;
                    case S_DUPLICATE:
                        vtprintf(DB_TEXT("Duplicate key\n"));
                        break;
                    case S_KEYSEQ:
                        vtprintf(DB_TEXT("Field type used out of sequence in d_keynext\n"));
                        break;
                    case S_UNAVAIL:
                        vtprintf(DB_TEXT("Database is unavailable\n"));
                        break;
                    case S_DELETED:
                        vtprintf(DB_TEXT("Record deleted since last access\n"));
                        break;
                    case S_UPDATED:
                        vtprintf(DB_TEXT("Record updated since last access\n"));
                        break;
                    case S_LOCKED:
                        vtprintf(DB_TEXT("Current record is locked\n"));
                        break;
                    case S_UNLOCKED:
                        vtprintf(DB_TEXT("Current record is not locked\n"));
                        break;
                }
            }

            return (stat);
        }
    }
    if (f == nfcns)
    {
        vftprintf(stderr, DB_TEXT("No such function - %s\n"), pi->i_name);
        return (DAL_ERR);
    }
    return (stat);
}

static char get_fdtype(int fld, DB_TASK *task)
{
    char fd_type = task->field_table[fld].fd_type;

    if (fd_type == COMKEY || fd_type == GROUPED)
    {
        /* If the field is a compound key or a struct, then get the type
            of its first element
        */
        if (fd_type == COMKEY)
            fld = task->key_table[task->field_table[fld].fd_ptr].kt_field;
        else
            fld++;
        fd_type = task->field_table[fld].fd_type;
    }
    return fd_type;
}


