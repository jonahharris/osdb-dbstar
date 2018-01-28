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

/*--------------------------------------------------------------------------
    IDA - Terminal Character Input Function
--------------------------------------------------------------------------*/

#ifdef QNX
/***************************** QNX VERSION ****************************/

#include <conio.h>

#include "keyboard.h"

/* ********************** LOCAL VARIABLE DECLARATIONS **************** */

static char unget[20];
static char unptr=0;

/****************************************************************************/

char tgetch()       /* get next terminal input character */
{
     int i;
     char j;

     while (1)
     {
          if (unptr)
                i = unget[--unptr];
          else
                i = getch( );

          if ((i>=' ' && i<='~') || i=='\n')      /* return normal char */
                return ((char) i);

          if (i == 127)                           /* return backspace */
                return ('\b');

          if (i == 255 )
          {
                i = getch();
                j = 0;
                switch (i)
                {
                     case 0201:  j = K_F1; break;
                     case 0202:  j = K_F2; break;
                     case 0203:  j = K_F3; break;
                     case 0204:  j = K_F4; break;
                     case 0205:  j = K_F5; break;
                     case 0206:  j = K_F6; break;
                     case 0207:  j = K_F7; break;
                     case 0210:  j = K_F8; break;
                     case 0211:  j = K_F9; break;
                     case 0212:  j = K_F10; break;
                     case 0240:  j = K_HOME; break;
                     case 0241:  j = K_UP; break;
                     case 0242:  j = K_PGUP; break;
                     case 0244:  j = K_LEFT; break;
                     case 0246:  j = K_RIGHT; break;
                     case 0250:  j = K_END; break;
                     case 0251:  j = K_DOWN; break;
                     case 0252:  j = K_PGDN; break;
                     case 0253:  j = K_INS; break;
                     case 0254:  j = K_DEL; break;
                     case 0003:  j = K_BREAK; break;
                     case 0033:  j = K_ESC; break;
                     default:    break;
                }

                if (j)
                     return (j);
          }
     }
}

/****************************************************************************/

int untgetch(char ch)
{
    if (unptr < 18)
        unget[unptr++] = ch;

    return 0;
}

#else  /* QNX */


/****************************** UNIX VERSION ******************************/
#include <signal.h>
#include <unistd.h>

#include "db.star.h"
#include "ida.h"
#include "keyboard.h"

/**************************** EXTERNAL FUNCTIONS **************************/

extern int tgetent();
extern char *tgetstr();

/**************************** LOCAL VARIABLES *****************************/
static jmp_buf sjbuf;

typedef struct token
{
    struct token *t_next;               /* next token */
    char ch;                            /* valid input token (character) */
    int action;                         /* action associated with 'seen'
                                         * token: */
}  TOKEN;                               /* > 0 => next state, < 0 => return
                                         * keytab.kchar for keytab entry
                                         * -(action+1) */
typedef struct state
{
    struct state *s_next;               /* next state (used only during
                                         * table build) */
    int state_no;                       /* state number (used only during
                                         * table build) */
    int end_action;                     /* action associated with end of
                                         * input */
    TOKEN *token_list;                  /* list of valid token characters */
}  STATE;

static int state_no = 1;
static STATE start = {NULL, 0, 0, NULL};
static STATE *cur_state = &start;
static STATE **state_table;            /* state_table[state_no] points to
                                        * state entry */

static struct fcnkeys
{
    char kchar;                         /* returned character code */
    char kstr[9];                       /* code string from terminal */
    char *tcstr;                        /* termcap code string: note the
                                         * upper-case special codes */
}  keytab[] =

{
    {
        K_INS, "\001", "IN"
    },
    {
        K_HOME, "\002", "kh"
    },
    {
        K_END, "\003", "EN"
    },
    {
        K_UP, "\004", "ku"
    },
    {
        K_DOWN, "\005", "kd"
    },
    {
        K_LEFT, "\006", "kl"
    },
    {
        K_RIGHT, "\007", "kr"
    },
    {
        K_FTAB, "\011", NULL
    },
    {
        K_PGUP, "\013", "PU"
    },
    {
        K_PGDN, "\014", "PD"
    },
    {
        K_F1, "\016", "k0"
    },
    {
        K_F2, "\017", "k1"
    },
    {
        K_F3, "\020", "k2"
    },
    {
        K_F4, "\021", "k3"
    },
    {
        K_F5, "\022", "k4"
    },
    {
        K_F6, "\023", "k5"
    },
    {
        K_F7, "\024", "k6"
    },
    {
        K_F8, "\025", "k7"
    },
    {
        K_F9, "\026", "k8"
    },
    {
        K_F10, "\027", "k9"
    },
    {
        K_CANCEL, "\030", "CN"
    },
    {
        K_BREAK, "\032", "BR"
    },
    {
        K_ESC, "\033", NULL
    },
    {
        K_DEL, "\177", "DE"
    },
    {
        '\0', "", NULL
    }
};

/* put back input stack */
struct in_stack
{
    struct in_stack *nextch;
    char ch;
};
static struct in_stack *isp = NULL;
static int initialized = 0;

/* ========================================================================
    Locate input token in current state
*/
static TOKEN *loctok( STATE *sp, char ch )
{
    register TOKEN *tokp, *tokq;

    for (tokq = NULL, tokp = sp->token_list; tokp; tokp = tokp->t_next)
    {
        if (tokp->ch == ch)
            return (tokp);
        tokq = tokp;
    }
    if (!initialized)
    {
        tokp = (TOKEN *) calloc(1, sizeof(TOKEN));
        if (tokq)
            tokq->t_next = tokp;
        else
            sp->token_list = tokp;
        tokp->t_next = NULL;
        tokp->ch = ch;
        tokp->action = 0;
    }
    return (tokp);
}

/* ========================================================================
    Build terminal function key input state transition table
*/
static void bld_state_table()
{
    register TOKEN *tokp;
    register STATE *sp;
    register int i, cp, len;

    for (i = 0; keytab[i].kchar; ++i)
    {
        sp = &start;
        len = strlen(keytab[i].kstr);
        for (cp = 0; cp < len; ++cp)
        {
            tokp = loctok(sp, keytab[i].kstr[cp]);
            if (tokp->action > 0)
            {
                /* switch states */
                while (tokp->action > sp->state_no)
                    sp = sp->s_next;
            }
            else if (cp == len - 1)
            {
                tokp->action = -i - 1;
            }
            else
            {
                /* create new state */
                cur_state->s_next = (STATE *) calloc(1, sizeof(STATE));
                cur_state = cur_state->s_next;
                cur_state->end_action = tokp->action;
                tokp->action = cur_state->state_no = state_no++;
                cur_state->token_list = NULL;
                sp = cur_state;
            }
        }
    }
    state_table = (STATE **) calloc(state_no, sizeof(STATE *));
    for (i = 0, sp = &start; i < state_no; ++i)
    {
        state_table[i] = sp;
        sp = sp->s_next;
    }
}

/* ========================================================================
    Initialize terminal input
*/
static void tgetinit()
{
    char bp[1024];
    char sbuf[80];
    char *term, *p;
    register int i, j;

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
    for (i = 0; keytab[i].kchar; ++i)
    {
        p = sbuf;
        if (keytab[i].tcstr)
        {
            if (p = tgetstr(keytab[i].tcstr, &p))
            {
                for (j = 0; p[j]; ++j)
                    keytab[i].kstr[j] = (p[j] == '\r' ? '\n' : p[j]);
                keytab[i].kstr[j] = '\0';
            }
        }
    }
    bld_state_table();
    initialized = 1;
}

/* ========================================================================
    Wake up getchar()
*/
static void wakeup(int dummy)
{
    longjmp(sjbuf, 1);
}

/* ========================================================================
    Push character back onto input stack
*/
void untgetch( char ch )
{
    struct in_stack *tsp;

    tsp = (struct in_stack *) calloc(1, sizeof(struct in_stack));
    if (tsp)
    {
        tsp->nextch = isp;
        isp = tsp;
        isp->ch = ch;
    }
}

/* ========================================================================
    Get next terminal input character
*/
char tgetch()
{
    char chbuf[20];
    struct in_stack *tsp;
    int awake;
    char ch = 0;
    register TOKEN *tokp;
    register int i = 0;;

    if (!initialized)
        tgetinit();

    /* return any put back character */
    if (isp)
    {
        ch = isp->ch;
        tsp = isp;
        isp = isp->nextch;
        free(tsp);
        return (ch);
    }

    ch = getchar();
    /* return any normal character */
    if ((ch >= ' ' && ch <= '~') || ch == '\n' || ch == '\b')
        return (ch);

    /* possible function key */
    state_no = 0;
    chbuf[0] = ch;
    chbuf[1] = '\0';
    i = 1;
    awake = setjmp(sjbuf);
    while (!awake)
    {
        if (tokp = loctok(state_table[state_no], ch))
        {
            if (tokp->action > 0)
                /* switch states */
                state_no = tokp->action;
            else
                /* matched input sequence */
                return (keytab[-(tokp->action + 1)].kchar);
        }
        else                             /* character sequence not found */
            goto send_back;
        signal(SIGALRM, wakeup);         /* This is how we can tell */
        alarm(1);                        /* when there isn't any input char. */
        ch = getchar();                  /* The alarm will wake up this
                                          * getchar. */
        signal(SIGALRM, SIG_IGN);        /* we'll only return if a char is
                                          * avail. */
        chbuf[i++] = ch;
        chbuf[i] = '\0';
    }
    /* end of input */
    if (state_table[state_no]->end_action < 0)
        return (keytab[-(state_table[state_no]->end_action + 1)].kchar);

send_back:
    for (i = strlen(chbuf) - 1; i > 0; --i)
        /* put back extra characters */
        untgetch(chbuf[i]);
    return (chbuf[0]);
}

#endif /* QNX */


