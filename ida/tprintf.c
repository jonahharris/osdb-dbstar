/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, ida utility                                       *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

/* -------------------------------------------------------------------------
    IDA - Terminal Printf Function
--------------------------------------------------------------------------*/
/***************************************************************************
NAME
    tprintf - formatted terminal output conversion

SYNOPSIS

    tprintf(format [, arg]...)
    char *format;

DESCRIPTION
    tprintf places output on the standard output stream, stdout, just like
    printf with additional formatting codes to take advantage of the screen
    control capabilities provided by most CRT terminals. If tprintf is called
    using only printf formatting codes then it will function precisely like
    printf. Hence, tprintf can be used instead of printf when only terminal
    output is to be used.

    tprintf formatting codes are as follows:

    @@   - '@' character                   @I   - Insert line
    @S   - Standout mode on                @i   - Insert character
    @s   - Standout mode off               @D   - Delete line
    @c   - Clear display screen            @d   - Delete character
    @H   - Cursor to home                  @R   - Reverse video on
    @h   - Cursor left                     @r   - Reverse video off
    @j   - Cursor down                     @U   - Start underscore
    @k   - Cursor up                       @u   - Stop underscore
    @l   - Cursor right                    @B   - Blink on
    @E   - Erase to end of screen          @b   - Blink off
    @e   - Erase to end of line            @O   - Turn cursor character on
                                           @o   - Turn cursor character off
    @m   - Move cursor (literal)
             @mrrcc   where rr is the row number and cc is the
                      column number both in decimal with a leading
                      zero if necessary.

             Example: tprintf("@m0324: row 3, column 24");

             Note: The row and column numbers begin at zero. Hence @m0000
             is the home position for the cursor.

    @M   - Move cursor (variable).

             This form requires that the rou and column be specified as the next
             two arguments in the tprinf argument list.

             Example:  row = 3;
                       col = 24;
                       tprintf("@M: row 3, column 24",row,col);

    SEE ALSO
             printf(3s)

***************************************************************************/

#include "db.star.h"

#ifdef QNX

/*************************** QNX VERSION *****************************/

void sscopy( char *, char * );
char *tgoto( char *, int, int );

/* ********************** LOCAL VARIABLE DECLARATIONS **************** */
#define NOCODES 95

struct scr_entry {
    char *code;
    char *control;
};

struct scr_entry scr_table[] = {

/*      ' '          '!'          '"'          '#'          '$'      */
    {NULL,NULL}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL},

/*      '%'          '&'          '''          '('          ')'      */
    {NULL,NULL}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL},

/*      '*'          '+'          ','          '-'          '.'      */
    {NULL,NULL}, {NULL,NULL}, {NULL,NULL}, {"GH","-" }, {NULL,NULL},

/*      '/'          '0'          '1'          '2'          '3'      */
    {NULL,NULL}, {NULL,NULL}, {"G3","+" }, {"GU","+" }, {"G4","+" },

/*      '4'          '5'          '6'          '7'          '8'      */
    {"GR","+" }, {"GM","+" }, {"GL","+" }, {NULL,"+" }, {NULL,"+" },

/*      '9'          ':'          ';'          '<'          '='      */
    {NULL,"+" }, {NULL,NULL}, {NULL,NULL}, {"AL","<" }, {NULL,NULL},

/*      '>'          '?'          '@'          'A'          'B'      */
    {"AR",">" }, {NULL,NULL}, {NULL,"@"},  {NULL,NULL}, {"vs",NULL},

/*      'C'          'D'          'E'            'F'          'G'      */
    {NULL,NULL}, {"dl",NULL}, {"cd","\033J"}, {NULL,NULL}, {NULL,NULL},

/*      'H'          'I'          'J'          'K'          'L'      */
    {"ho",NULL}, {"al",NULL}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL},

/*      'M'            'N'          'O'          'P'          'Q'      */
    {"cm","\033="}, {NULL,NULL}, {"CO",NULL}, {NULL,NULL}, {NULL,NULL},

/*      'R'             'S'            'T'          'U'          'V'      */
    {"rv","\033("}, {"so","\033<"}, {NULL,NULL}, {"us",NULL}, {NULL,NULL},

/*      'W'          'X'          'Y'          'Z'          '['      */
    {NULL,NULL}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL},

/*      '\'          ']'          '^'          '_'          '`'      */
    {NULL,NULL}, {NULL,NULL}, {"AU","^" }, {NULL,NULL}, {NULL,NULL},

/*      'a'          'b'            'c'               'd'            'e'      */
    {NULL,NULL}, {"ve",NULL}, {"cl","\033H\033J"}, {"dc",NULL}, {"ce","\033K"},

/*      'f'          'g'          'h'          'i'          'j'      */
    {NULL,NULL}, {NULL,NULL}, {"bc","\b"}, {"ic",NULL}, {"do",NULL},

/*      'k'          'l'          'm'            'n'          'o'      */
    {"up",NULL}, {"nd",NULL}, {"cm","\033="}, {NULL,NULL}, {"CF",NULL},

/*      'p'          'q'            'r'             's'          't'      */
    {NULL,NULL}, {NULL,NULL}, {"re","\033)"}, {"se","\033>"}, {NULL,NULL},

/*      'u'          'v'          'w'          'x'          'y'      */
    {"ue",NULL}, {"AD","v" }, {NULL,NULL}, {NULL,NULL}, {NULL,NULL},

/*      'z'          '{'          '|'          '}'          '~'      */
    {NULL,NULL}, {NULL,NULL}, {"GV","|" }, {NULL,NULL}, {NULL,NULL}
};

int litmode = 0;             /* literal interp. mode flag */
int ldmode  = 0;             /* line drawing mode flag */
static int initialized = 0;  /* non-zero once initerm has been called */

/****************************************************************************/

int tprintf( char *format, ... )
{
    va_list argptr;
    char *tgoto();
    char fmt[256];
    register int i;
    register char *fin, *fout;
    register char *sptr;
    char fcode, *cp;

    va_start( argptr, format );
    fin = format;

    /* scan format copying characters into 'fmt' until an '@', '%', or '\0' */

    while ( *fin )
    {
        fout = fmt;

        while ( (*fin != '%') && *fin )
        {
            if ( *fin != '@' )
            {
                *fout++ = *fin++;
            }
            else
            { 
                ++fin;
                i = (int)(*fin - ' ');
                if (scr_table[i].control)
                {
                    if ( *fin == 'M' || *fin == 'm')
                    {
                        register int row, col;
                        if ( *fin == 'M' )
                        {           
                            row = *(va_arg( argptr, int * ));
                            col = *(va_arg( argptr, int * ));
                        }
                        else
                        { 
                            row = 10*(*(fin+1)-'0') + (*(fin+2)-'0');
                            col = 10*(*(fin+3)-'0') + (*(fin+4)-'0');
                            fin += 4;
                        }

                        sptr = fout; 
                        sscopy(fout, tgoto(scr_table[i].control, col, row));
                        fout += strlen(sptr);
                    }
                    else
                    {
                        sscopy(fout, scr_table[i].control);
                        fout += strlen(scr_table[i].control);
                    }
                }
                fin++;
            }
        }

        if ( *fin == '%' )
        {
            *fout++ = *fin++;
            while((*fin == '-')||(*fin == '.')||(*fin >= '0'&&*fin <= '9'))
                *fout++ = *fin++;
            fcode = *fout++ = *fin++;
            *fout = '\0';
            switch( fcode )
            {
                case 'l':
                    fcode = *fout++ = *fin++;
                    *fout = '\0';
                    if ( fcode == 'd' )
                        printf( fmt, va_arg( argptr, long ) );
                    else
                        printf( "Unknown format code = l%c\n", fcode );
                    break;
                case 'd':
                    printf( fmt, va_arg( argptr, int ) );
                    break;
                case 's':
                    printf( fmt, va_arg( argptr, char * ) );
                    break;
                case 'c':
                    cp = va_arg( argptr, char * );
                    printf( fmt, *cp );
                    break;
                default:
                    printf( "Unknown format code = %c\n", fcode );
            }
        }
        else
        {
            if ( fout )
            {
                *fout = '\0';
                printf(fmt);
            }
        }
    }

    fflush(stdout);
    va_end( argptr );

    return 0;
}

/****************************************************************************/
void sscopy(char *s1, char *s2) /* Special string copy function to allow printing '%' */
{
    while ( *s1++ = *s2 )
    {
        if ( *s2++ == '%' )
            *s1++ = '%';
    }
}

/****************************************************************************/
char *tgoto(char *st, int c, int r)
{
    static char rcstr[10];

    sprintf(rcstr, "\033=%c%c", r +' ', c +' ');
        return (rcstr);
}


#else


/****************************** UNIX VERSION ******************************/

/************************** EXTERNAL FUNCTIONS ****************************/

extern int tgetent();
extern int tgetflag();
extern int tgetnum();
extern char *tgetstr();
extern char *tgoto();
extern int tputs();
extern char *fgetlr();

/**************************** LOCAL FUNCTIONS *****************************/

int scopy(char *, char * );

/*************************** LOCAL  VARIABLES *****************************/

#define NOCODES 95

struct scr_entry
{
    char *code;
    char *control;
};

struct scr_entry scr_table[] = {

/*      ' '          '!'          '"'          '#'          '$'      */
    {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},

/*      '%'          '&'          '''          '('          ')'      */
    {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},

/*      '*'          '+'          ','          '-'          '.'      */
    {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {"GH", "-"}, {NULL, NULL},

/*      '/'          '0'          '1'          '2'          '3'      */
    {NULL, NULL}, {NULL, NULL}, {"G3", "+"}, {"GU", "+"}, {"G4", "+"},

/*      '4'          '5'          '6'          '7'          '8'      */
    {"GR", "+"}, {"GM", "+"}, {"GL", "+"}, {NULL, "+"}, {NULL, "+"},

/*      '9'          ':'          ';'          '<'          '='      */
    {NULL, "+"}, {NULL, NULL}, {NULL, NULL}, {"AL", "<"}, {NULL, NULL},

/*      '>'          '?'          '@'          'A'          'B'      */
    {"AR", ">"}, {NULL, NULL}, {NULL, "@"}, {NULL, NULL}, {"vs", NULL},

/*      'C'          'D'          'E'          'F'          'G'      */
    {NULL, NULL}, {"dl", NULL}, {"cd", NULL}, {NULL, NULL}, {NULL, NULL},

/*      'H'          'I'          'J'          'K'          'L'      */
    {"ho", NULL}, {"al", NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},

/*      'M'          'N'          'O'          'P'          'Q'      */
    {"cm", NULL}, {NULL, NULL}, {"CO", NULL}, {NULL, NULL}, {NULL, NULL},

/*      'R'          'S'          'T'          'U'          'V'      */
    {"so", NULL}, {"us", NULL}, {NULL, NULL}, {"us", NULL}, {NULL, NULL},

/*      'W'          'X'          'Y'          'Z'          '['      */
    {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},

/*      '\'          ']'          '^'          '_'          '`'      */
    {NULL, NULL}, {NULL, NULL}, {"AU", "^"}, {NULL, NULL}, {NULL, NULL},

/*      'a'          'b'          'c'          'd'          'e'      */
    {NULL, NULL}, {"ve", NULL}, {"cl", NULL}, {"dc", NULL}, {"ce", NULL},

/*      'f'          'g'          'h'          'i'          'j'      */
    {NULL, NULL}, {NULL, NULL}, {"bc", "\b"}, {"ic", NULL}, {"do", NULL},

/*      'k'          'l'          'm'          'n'          'o'      */
    {"up", NULL}, {"nd", NULL}, {"cm", NULL}, {NULL, NULL}, {"CF", NULL},

/*      'p'          'q'          'r'          's'          't'      */
    {NULL, NULL}, {NULL, NULL}, {"se", NULL}, {"ue", NULL}, {NULL, NULL},

/*      'u'          'v'          'w'          'x'          'y'      */
    {"ue", NULL}, {"AD", "v"}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},

/*      'z'          '{'          '|'          '}'          '~'      */
    {NULL, NULL}, {NULL, NULL}, {"GV", "|"}, {NULL, NULL}, {NULL, NULL}
};

int litmode = 0;                       /* literal interp. mode flag */
int ldmode = 0;                        /* line drawing mode flag */
static int initialized = 0;            /* non-zero once initerm has been
                                        * called */

/* ===================================================================
    Initialize terminal screen table
*/
static void initerm()
{
    char bp[1024];
    char sbuf[80];
    char *term, *q, *p;
    register int i;

    term = getenv("TERM");
    switch (tgetent(bp, term))
    {
        case -1:
            fprintf(stderr, "unable to open termcap\n");
            exit(1);
        case 0:
            fprintf(stderr, "no termcap entry for terminal %s\n", term);
            exit(1);
    }
    /* Load screen control table */
    for (i = 0; i < NOCODES; ++i)
    {
        p = sbuf;
        if (scr_table[i].code)
        {
            if (q = tgetstr(scr_table[i].code, &p))
            {
                if ((scr_table[i].control = (char *)malloc(strlen(q) + 1)) == NULL)
                {
                    fprintf(stderr, "tprintf: no memory for screen tables\n");
                    exit(1);
                }
                for (p = q; (*p >= '0' && *p <= '9'); ++p)
                {
                    if (*(p + 1) == '*')
                    {
                        p += 2;
                        break;
                    }
                }
                strcpy(scr_table[i].control, p);
            }
        }
    }
}

/* ===================================================================
    Formatted terminal output conversion
*/
int tprintf(char *format, ...)
{
    va_list argptr;
    char fmt[256];
    char fcode;
    char *cp;
    int *ip;
    register int i;
    register char *fin, *fout;
    register char *sptr;

    if (!initialized)
    {
        initerm();
        initialized = 1;
    }
    va_start(argptr, format);
    fin = format;
    while (*fin)
    {
        /* scan format copying characters into 'fmt' until an '@', '%', or a
         * '\0' is seen. */
        fout = fmt;
        while ((*fin != '%') && *fin)
        {
            if (*fin != '@')
            {
                *fout++ = *fin++;
            }
            else
            {
                ++fin;
                i = (int) (*fin - ' ');
                if (scr_table[i].control)
                {
                    if (*fin == 'M' || *fin == 'm')
                    {
                        register int row, col;

                        if (*fin == 'M')
                        {                    /* variable cursor movement */
                            ip = va_arg(argptr, int *);
                            row = *ip;
                            ip = va_arg(argptr, int *);
                            col = *ip;
                        }
                        else
                        {
                            row = 10 * (*(fin + 1) - '0') + (*(fin + 2) - '0');
                            col = 10 * (*(fin + 3) - '0') + (*(fin + 4) - '0');
                            fin += 4;
                        }
                        sptr = fout;
                        scopy(fout, tgoto(scr_table[i].control, col, row));
                        fout += strlen(sptr);
                    }
                    else
                    {
                        scopy(fout, scr_table[i].control);
                        fout += strlen(scr_table[i].control);
                    }
                }
                fin++;
            }
        }
        if (*fin == '%')
        {
            /* copy % spec into fmt */
            *fout++ = *fin++;             /* copy % sign */
            while ((*fin == '-') ||
                     (*fin == '.') ||
                     (*fin >= '0' && *fin <= '9'))
                *fout++ = *fin++;          /* copy field spec */
            fcode = *fout++ = *fin++;     /* copy format code */
            *fout = '\0';
            switch (fcode)
            {
                case 'l':
                    fcode = *fout++ = *fin++;
                    *fout = '\0';
                    if (fcode == 'd')
                        printf(fmt, va_arg(argptr, long));
                    else
                        printf("Unknown format code = l%c\n", fcode);
                    break;
                case 'd':
                    printf(fmt, va_arg(argptr, int));
                    break;
                case 's':
                    printf(fmt, va_arg(argptr, char *));
                    break;
                case 'c':
                    cp = va_arg(argptr, char *);
                    printf(fmt, *cp);
                    break;
                default:
                    printf("Unknown format code = %c\n", fcode);
            }
        }
        else if (fout)
        {
            *fout = '\0';
            printf(fmt);
        }
    }
    fflush(stdout);

    return 0;
}

/* ===================================================================
    Special string copy function
*/
int scopy(char *s1, char *s2)
{
    while (*s1++ = *s2)
    {
        if (*s2++ == '%')
            *s1++ = '%';
    }

    return 0;
}

#endif /* QNX */


