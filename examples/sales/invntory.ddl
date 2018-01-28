/*----------------------------------------------------------------------
   db_QUERY inventory example database
----------------------------------------------------------------------*/
database invntory 
{

   data file "invntory.d00" contains outlet;
   data file "invntory.d01" contains product;
   data file "invntory.d02" contains on_hand;

   key  file "invntory.k03" contains prod_id;
   key  file "invntory.k04" contains loc_id;

   record product 
   {
      long             price[3];
      long             cost;
      unique key short prod_id;
      char             prod_desc[40];
   }
   record outlet 
   {
      short             region;
      unique key char   loc_id[4];
      char              city[18];
      char              state[3];
   }
   record on_hand 
   {
      long quantity;
   }
   set distribution 
   {
      order    last;
      owner    product;
      member   on_hand;
   }
   set inventory 
   {
      order    last;
      owner    outlet;
      member   on_hand;
   }
}

