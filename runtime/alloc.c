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

static int INTERNAL_FCN NewInit(ll_elem **, DB_TASK *);

/*-----------------------------------------------------------------------
    Uninstrumented dynamic memory allocation functions
-----------------------------------------------------------------------*/

/* ======================================================================
    Resets pointer to current element and checks for an empty list
*/
DB_BOOLEAN INTERNAL_FCN ll_access(llist *ll)
{
    ll->curr = NULL;
    return (DB_BOOLEAN) (ll->head != NULL && ll->tail != NULL);
}

/* ======================================================================
    Appends item to list
*/
int INTERNAL_FCN ll_append(llist *ll, char *data, DB_TASK * task)
{
    ll_elem *item;

    if (NewInit(&item, task) != S_OKAY)
        return task->db_status;

    if (ll->head == NULL)  /* Empty list */
        ll->curr = ll->head = ll->tail = item;
    else
    {
        ll->tail->next = item;
        ll->curr = ll->tail = item;
    }

    ll->curr->data = data;
    return task->db_status;
}

/* ======================================================================
    Finds the first element of a list and returns its data
*/
char *INTERNAL_FCN ll_first(llist *ll)
{
    if (ll->head == NULL)
        return NULL;

    ll->curr = ll->head;
    return ll->curr->data;
}

/* ======================================================================
    Frees a list
*/
void INTERNAL_FCN ll_free(llist *ll, DB_TASK *task)
{
    ll_elem *curr;
    ll_elem *next;

    for (curr = ll->head; curr != NULL; curr = next)
    {
        next = curr->next;
        psp_freeMemory((void *) curr, 0);
    }

    ll->head = NULL;
    ll->tail = NULL;
    ll->curr = NULL;
}

/* ======================================================================
    Finds the next element and returns its data
*/
char *INTERNAL_FCN ll_next(llist *ll)
{
    ll_elem *next;

    if (ll->curr == NULL)
        return ll_first(ll);

    if (ll->curr->next == NULL)
        return NULL;

    next = ll->curr->next;
    ll->curr = next;
    return ll->curr->data;
}

/* ======================================================================
    Prepends (stacks) item
*/
int INTERNAL_FCN ll_prepend(llist *ll, char *data, DB_TASK *task)
{
    ll_elem *item;

    if (NewInit(&item, task) != S_OKAY)
        return task->db_status;

    if (ll->head == NULL)
    {
        /* Empty list */
        ll->curr = ll->head = ll->tail = item;
    }
    else
    {
        item->next = ll->head;
        ll->curr = ll->head = item;
    }

    ll->curr->data = data;
    return task->db_status;
}

/* ======================================================================
    Allocates and initializes a new list element
*/
static int INTERNAL_FCN NewInit(ll_elem * *pNew, DB_TASK *task)
{
    *pNew = (ll_elem *) psp_getMemory(sizeof(ll_elem), 0);
    if (*pNew == NULL)
        return (dberr(S_NOMEMORY));

    memset(*pNew, '\0', sizeof(ll_elem));
    return task->db_status;
}


/* ======================================================================
*/
void INTERNAL_FCN ll_deaccess(llist *ll)
{
    ll->curr = NULL;
}
