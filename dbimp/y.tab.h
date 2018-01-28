#define T_DATABASE 257
#define T_FOR 258
#define T_ON 259
#define T_FIELD 260
#define T_CONNECT 261
#define T_END 262
#define T_RECORD 263
#define T_CREATE 264
#define T_UPDATE 265
#define T_FIND 266
#define T_NUMBER 267
#define T_IDENT 268
#define T_STRING 269
typedef union {
    STRTOK tstr; 
    NUMTOK tnum;
} YYSTYPE;
extern YYSTYPE yylval;
