/*----------------------------------------------------------------------
   exupd.c: Update example sales database

      -  create customer notes
      -  make sales manager set connections
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.star.h"
#include "sales.h"

char sid0[15] = "";  /* last salesperson id */
char cid0[15] = "";  /* last customer id */
char nid0[15] = "";  /* last note id */
long ndate0;         /* last note date */
char sid1[15] = "";  /* current salesperson id */
char cid1[15] = "";  /* current customer id */
char nid1[15] = "";  /* current note id */
long ndate1 = 0L;    /* current note date */
char text[128];      /* input text buffer */
struct note note_rec;

FILE *ft;

/* =====================================================================
*/
int main()
{
   int new_note, len, rec;
   DB_TASK *task;

   /* open database task */
   if ( d_opentask(&task) != S_OKAY )
      return 1;

   /* open sales database */
   if ( d_open("sales", "o", task) != S_OKAY )
      return 1;

   /* open ascii notes file */
   if ( ! (ft = fopen("notes.asc","r")) )
   {
      printf("unable to open notes file\n");
      return 1;
   }
   /* enter customer notes for db_QUERY example database */
   while ( fgets(text, 90, ft) )
   {
      new_note = 0;
      sscanf(text, "%s %s %s %ld", sid1, cid1, nid1, &ndate1);
      if ( strcmp(sid0, sid1) )
      {
         new_note = 1;
         strcpy(sid0, sid1);
         d_keyfind(SALE_ID, sid1, task, CURR_DB);
         d_setor(TICKLER, task, CURR_DB);
      }
      if ( strcmp(cid0, cid1) )
      {
         new_note = 1;
         strcpy(cid0, cid1);
         d_keyfind(CUST_ID, cid1, task, CURR_DB);
         d_setor(ACTIONS, task, CURR_DB);
      }
      if ( new_note || strcmp(nid0, nid1) || ndate0 != ndate1 )
      {
         strcpy(nid0, nid1);
         strcpy(note_rec.note_id, nid1);
         note_rec.note_date = ndate0 = ndate1;
         d_fillnew(NOTE, (char *)&note_rec, task, CURR_DB);
         d_setor(COMMENTS, task, CURR_DB);
         d_connect(TICKLER, task, CURR_DB);
         d_connect(ACTIONS, task, CURR_DB);
      }
      fgets(text, 90, ft);
      len = strlen(text);
      text[--len] = '\0';
      if ( len < 20 )
         rec = TEXT20;
      else
         if ( len < 50 )
            rec = TEXT50;
         else
            rec = TEXT80;

      d_fillnew(rec, text, task, CURR_DB);
      d_connect(COMMENTS, task, CURR_DB);
   }
   d_close(task);
   d_closetask(task);
   return 0;
}

