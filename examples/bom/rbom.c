/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, BOM example application, relational model         *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

#include <db.star.h>
#include "rbom.h"

void rbuild_bill(char*);
double rget_cost(char*);
void random_id(char*);

long current_level, max_level, max_members;
double rolled_up_cost;
char response[20];
time_t start_time, end_time, elapsed_time;
DB_TASK *Currtask;

struct rbill RBill;
struct ritem RItem;


int main(void)
{
    printf("\nRelational model BOM benchmark\n");

    RItem.rid_code[0] = '\0';
    RItem.rcost = 1.0L;

    RBill.rquantity = 1.0L;

    current_level = 0;

    printf("\nEnter number of levels: ");
    fgets(response, sizeof(response), stdin);
    max_level = atoi(response);

    printf("\nEnter number of members per level: ");
    fgets(response, sizeof(response), stdin);
    max_members = atoi(response);

    /* create a database task, open the database, and initialize it */
    d_opentask(&Currtask);
    d_setpages(32, 8, Currtask);
    d_open("rbom", "o", Currtask);
    d_initialize(Currtask, CURR_DB);    /* erase all old data */

    printf("building bill file\n");
    time(&start_time);

    /* create top level ITEM record */
    strcpy(RItem.rid_code, "AAAAAAAAAAAAAAA");
    RItem.rcomponent_count = max_members;
    if (d_fillnew(RITEM, &RItem, Currtask,  CURR_DB) != S_OKAY) {
        printf("duplicate part %s\n", RItem.rid_code);
    }

    /* recursive call to build multi-level bill */
    rbuild_bill("AAAAAAAAAAAAAAA");

    time(&end_time);
    elapsed_time = end_time - start_time;
    printf("time to build file was %ld seconds\n", elapsed_time);

    printf("rolling up cost\n");
    time(&start_time);

    /* recursive call to calculate cost of whole component tree */
    rolled_up_cost = rget_cost("AAAAAAAAAAAAAAA");

    time(&end_time);
    elapsed_time = end_time - start_time;
    printf("total rolled up cost = %10.2f\n", rolled_up_cost);
    printf("time to compute cost was %ld seconds\n",elapsed_time);

    /* close the database and task */
    d_close(Currtask);
    d_closetask(Currtask);

    return (0);
}

/* rbuild_bill - recursive routine to build one level of a bill by
   adding components to a parent. References global variables
   'current_level' and 'max_level'.
*/
void rbuild_bill(char* parent)
{
    long i;
    char id_code[16];

    current_level++;

    for (i=0; i<max_members; i++) {

        /* create an RITEM record which will be a component of the parent
           RITEM record whose id code was passed into this function
        */
        random_id(RItem.rid_code);
        if (current_level < max_level) {
            RItem.rcomponent_count = max_members;
        }
        else {
            /* this is the bottom of the tree */
            RItem.rcomponent_count = 0;
        }
        if (d_fillnew(RITEM, &RItem, Currtask, CURR_DB) != S_OKAY) {
            printf("duplicate part %s\n", RItem.rid_code);
        }

        /* create an RBILL record, with relational references to the parent
           and component RITEM records - this RBILL record provides a
           linkage in a many-to-many relationship
        */
        strcpy(RBill.rparent, parent);
        strcpy(RBill.rcomponent, RItem.rid_code);
        RBill.rsequence = i;
        RBill.rlevel = current_level;
        d_fillnew(RBILL, &RBill, Currtask, CURR_DB);

        /* if we are not at the bottom of the tree, attach further 
           components to this component RITEM, by recursively calling
           this function
        */
        if (current_level < max_level) {
            strcpy(id_code, RItem.rid_code);
            rbuild_bill(id_code);
        }
    }
    current_level--;
    return;
}

/* rget_cost - recursive routine to roll up cost from lower levels of bill.
   The costs are stored only at the lowest levels of the bill.
*/
double rget_cost(char* parent)
{
    double total_cost;    /* for this item and below */
    int component_count;
    struct rbom Rbom, Rbom_save;
    struct rbill RBill_local;

    /* get the current key value for the RBOM key - on exit, this function
       repositions to this value, so that calls to d_keynext in the previous
       level of recursion will find the right value
    */
    d_keyread(&Rbom_save, Currtask);

    /* use RID_CODE key to find the parent item, then read the whole record */
    d_keyfind(RID_CODE, parent, Currtask, CURR_DB);
    d_recread(&RItem, Currtask, CURR_DB);

    component_count = RItem.rcomponent_count;
    if (component_count == 0) {

        /* no components for this parent - return the cost of this
           bottom-level item 
        */
        return RItem.rcost;
    }

    /* there is at least one component, so go down a level - use RBOM key
       to find the first RBILL record that refers to the parent item
    */
    strcpy(Rbom.rparent, parent);
    Rbom.rsequence = 0;
    d_keyfind(RBOM, &Rbom, Currtask, CURR_DB);  /* find first bill record */
    total_cost = 0.0L;

    /* use RBOM key to go through all RBILL records that refer to the
       parent item
    */
    for ( ; ; ) {

        /* read bill rec to get component ID */
        d_recread(&RBill_local, Currtask, CURR_DB);

        /* this component may also be a parent - call this function
           recursively to get costs for the component tree attached
           to the RITEM record
        */
        total_cost += rget_cost(RBill_local.rcomponent) * RBill_local.rquantity;

        if(--component_count == 0) break;

        /* find next bill record */
        d_keynext(RBOM, Currtask, CURR_DB);
    }

    /* reposition to start position in RBOM key */
    d_keyfind(RBOM, &Rbom_save, Currtask, CURR_DB);

    return total_cost;    /* for everything below this item */
}


void random_id(char* string)    /* generates 15-character alpha part id */
{
    int i, j;
    for (i=0; i<15; i++) {
        do {
            j = toupper(rand() & 127);
        } while (j < 'A' || j > 'Z');
        string[i] = j;
    }
    string[i] = '\0';
}
