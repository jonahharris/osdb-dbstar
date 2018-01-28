#define T_WHILEOK 257
#define T_EXIT 258
#define T_PRINT 259
#define T_INPUT 260
#define T_ABORT 261
#define T_SCHEMA 262
#define T_CURRENCY 263
#define T_REWIND 264
#define T_IDENT 265
#define T_STRING 266
typedef union {
     STRTOK tstr;
     NUMTOK tnum;
} YYSTYPE;
extern YYSTYPE yylval;
