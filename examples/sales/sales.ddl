/*----------------------------------------------------------------------
   db_QUERY sales example database
----------------------------------------------------------------------*/
database sales 
{
   data file "sales.d00" contains customer, sales_order, salesperson;
   data file "sales.d01" contains item;
   data file "sales.d02" contains note;
   data file "sales.d03" contains text20;
   data file "sales.d04" contains text50;
   data file "sales.d05" contains text80;

   key  file "sales.k06" contains cust_id, sale_id, ord_num;
   key  file "sales.k07" contains order_key, note_id;

   record customer 
   {
      unique key char cust_id[4];
                 char company[31];
                 char contact[31];
                 char street[31];
                 char city[18];
                 char state[3];
                 char zip[6];
                 char phone[13];
   }
   record sales_order 
   {
                 double    amount;
                 float     tax;
                 long      ord_date;
      unique key short     ord_num;
      compound key         order_key 
      {
        ord_date desc;
        ord_num;
      }
   }
   record item 
   {
      short prod_id;
      short quantity;
   }
   record salesperson 
   {
      float             commission;
      short             region;
      db_addr           office;
      unique key char   sale_id[4];
      char              sale_name[31];
   }
   record note 
   {
      long     note_date;
      key char note_id[9];
   }
   record text20 
   {
      char t20[21];
   }
   record text50 
   {
      char t50[51];
   }
   record text80 
   {
      char t80[81];
   }
   set manages 
   {
      order    ascending;
      owner    salesperson;
      member   salesperson by sale_name;
   }
   set accounts 
   {
      order    first;
      owner    salesperson;
      member   customer;
   }
   set purchases 
   {
      order    first;
      owner    customer;
      member   sales_order;
   }
   set line_items 
   {
      order    last;
      owner    sales_order;
      member   item;
   }
   set tickler 
   {
      order    descending;
      owner    salesperson;
      member   note by note_date;
   }
   set actions 
   {
      order    descending;
      owner    customer;
      member   note by note_date;
   }
   set comments 
   {
      order    last;
      owner    note;
      member   text20;
      member   text50;
      member   text80;
   }
}

