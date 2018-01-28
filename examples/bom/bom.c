/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, BOM example application, network model            *
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
#include "bom.h"

void build_bill();
double get_cost();
void random_id(char*);

long current_level, max_level, max_members;
double rolled_up_cost;
char response[20];
time_t start_time, end_time, elapsed_time;
DB_TASK *Currtask;

struct bill Bill;    /* global BILL record */
struct item Item;    /* global ITEM record */


int main(void)
{
    printf("\nNetwork model BOM benchmark\n");

    Item.id_code[0] = '\0';
    Item.cost = 1.0L;

    Bill.quantity = 1.0L;

    current_level = 0;

    printf("\nEnter number of levels: ");
    fgets(response, sizeof(response), stdin);
    max_level = atol(response);

    printf("\nEnter number of members per level: ");
    fgets(response, sizeof(response), stdin);
    max_members = atoi(response);

    /* create a database task, open the database, and initialize it */
    d_opentask(&Currtask);
    d_setpages(32, 8, Currtask);
    d_open("bom", "o", Currtask);
    d_initialize(Currtask, CURR_DB);    /* erase all old data */

    printf("building bill file\n");
    time(&start_time);

    /* create top level ITEM record */
    strcpy(Item.id_code, "AAAAAAAAAAAAAAA");
    if (d_fillnew(ITEM, &Item, Currtask, CURR_DB) != S_OKAY) {
        printf("duplicate part %s\n", Item.id_code);
    }

    /* recursive call to build multi-level bill */
    build_bill();

    time(&end_time);
    elapsed_time = end_time - start_time;
    printf("time to build file was %ld seconds\n", elapsed_time);

    printf("rolling up cost\n");
    time(&start_time);

    /* find the top level ITEM record */
    d_keyfind(ID_CODE, "AAAAAAAAAAAAAAA", Currtask, CURR_DB);

    /* recursive call to calculate cost of whole component tree */
    rolled_up_cost = get_cost();

    time(&end_time);
    elapsed_time = end_time - start_time;
    printf("total rolled up cost = %10.2f\n", rolled_up_cost);
    printf("time to compute cost was %ld seconds\n",elapsed_time);

    /* close the database and task */
    d_close(Currtask);
    d_closetask(Currtask);

    return (0);
}

/* build_bill - recursive routine to build one level of a bill by
   adding components to a parent. Assumes that parent is current
   record. References global variables 'current_level' and 'max_level'.
*/
void build_bill()
{
    DB_ADDR bom_owner;
    DB_ADDR bom_member;
    int i;

    current_level++;

    /* get current owner and member of BOM set - restore these before
       the function returns, so that the currency will be correct for
       the previous level of recursion
    */
    d_csoget(BOM, &bom_owner, Currtask, CURR_DB);
    d_csmget(BOM, &bom_member, Currtask, CURR_DB);

    /* make the current record the current owner of BOM set */
    d_setor(BOM, Currtask, CURR_DB);

    for (i=0; i<max_members; i++) {

        /* create new ITEM record, for component item */
        random_id(Item.id_code);
        if (d_fillnew(ITEM, &Item, Currtask, CURR_DB) != S_OKAY) {    /* new component ITEM record */
            printf("duplicate part %s\n", Item.id_code);
        }
        d_setor(WHERE_USED, Currtask, CURR_DB);

        /* create new BILL record */
        Bill.level = current_level;
        d_fillnew(BILL, &Bill, Currtask, CURR_DB);

        /* connect new BILL record to parent ITEM and component ITEM,
           to form a linkage between the two items, in a many-to-many
           relationship
        */
        d_connect(BOM, Currtask, CURR_DB);
        d_connect(WHERE_USED, Currtask, CURR_DB);

        /* if we are not at the bottom level, recurse down another level */
        if (current_level < max_level) {

            /* make the new component ITEM record the current record,
               so that the recursive call to this function will connect
               further components to it
            */
            d_setro(WHERE_USED, Currtask, CURR_DB);
            build_bill();
        }
    }
    current_level--;

    /* restore original currency */
    d_csoset(BOM, &bom_owner, Currtask, CURR_DB);
    d_csmset(BOM, &bom_member, Currtask, CURR_DB);

    return;
}

/* get_cost - recursive routine to roll up cost from lower levels of bill.
   Assumes that item to be costed is the current record. The costs are found
   only at the lowest levels of the bill.
*/
double get_cost()
{
    DB_ADDR bom_owner;
    DB_ADDR bom_member;
    double total_cost;    /* for this item and below */
    long member_count;
    struct bill Bill_local;

    /* get current owner and member of BOM set - restore these before
       the function returns, so that the currency will be correct for
       the previous level of recursion
    */
    d_csoget(BOM, &bom_owner, Currtask, CURR_DB);
    d_csmget(BOM, &bom_member, Currtask, CURR_DB);

    /* make the current record the current owner of BOM set */
    d_setor(BOM, Currtask, CURR_DB);

    /* get number of components connected to this ITEM record */
    d_members(BOM, &member_count, Currtask, CURR_DB);

    /* no members - we are at the bottom level now */
    if (member_count == 0) {

        /* read the current ITEM to get cost */
        d_recread(&Item, Currtask, CURR_DB);

        /* restore original currency */
        d_csoset(BOM, &bom_owner, Currtask, CURR_DB);
        d_csmset(BOM, &bom_member, Currtask, CURR_DB);

        /* return cost from the ITEM record just read */
        return Item.cost;
    }

    /* there is at least one member, so go down a level */
    total_cost = 0.0L;

    /* cycle through all components */
    while (member_count--) {

        /* find next member - if there's no current member this will
           find the first member
        */
        d_findnm(BOM, Currtask, CURR_DB);

        /* read the bill rec to get quanity */
        d_recread(&Bill_local, Currtask, CURR_DB);

        /* find its owner thru WHERE_USED set - this is a component ITEM */
        d_findco(WHERE_USED, Currtask, CURR_DB);

        /* recursive call - get cost of all components connected to this one */
        total_cost += get_cost() * Bill_local.quantity;
    }

    /* restore original currency */
    d_csoset(BOM, &bom_owner, Currtask, CURR_DB);
    d_csmset(BOM, &bom_member, Currtask, CURR_DB);

    return total_cost;
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
