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

/*-------------------------------------------------------------------------
    IDA - field text edit functions
-------------------------------------------------------------------------*/
#include "db.star.h"
#include "ida.h"
#include "keyboard.h"

/************************** EXTERNAL FUNCTIONS ****************************/
extern char tgetch();

/************************** EXTERNAL VARIABLES ****************************/
extern char fldtxt[];

/************************** FORWARD REFERENCES ****************************/
static int mvcl(void), mvcr(void), mvbeg(void), mvend(void), mvfbw(void),
           mvfw(void), mvbbw(void), mvbw(void), findr(void), findl(void),
           repf(void), search(void), next(void), apndend(void), repch(void),
           reptxt(void), delr(void), setdel(void), undo(void), pr_text(void),
           ins(void), apndnxt(void), delete(int);

/*************************** LOCAL  VARIABLES *****************************/
#define DWIDTH 72                      /* width of display field */
#define MAXCHG 200                     /* maximum changed text which can
                                        * be undone */
static char chgbuf[MAXCHG];            /* text change buffer */
static int chgpos;                     /* position of last change */
static char chgcmd;                    /* last change command */
static int chglen = 0;                 /* number of chars changed */
static int lp;                         /* position at left end */
static int rp;                         /* position at right end */
static int cp;                         /* current position in fldtxt */
static int len;                        /* current length of fldtxt string */
static char cc;                        /* current command character */
static char lc = '\0';                 /* last command char */
static char fc = 'f';                  /* last find command */
static char sc = '\0';                 /* last search char */
static char srch[81];                  /* search string */
static int rep;                        /* repeat count */
static int refresh;                    /* display refresh flag */
static int deleting = 0;               /* delete mode flag */
static int dp;                         /* delete start position */

/* edit command table */
struct command
{
    int (*fcn) (void);                 /* function which processes command */
    char key;                          /* command char */
};

/* char move commands must all be listed first */
#define MOVECMDS 26
#define THRUCMDS 18
static struct command cmds[] = {
/* movement commands follow */
    {mvcl, '\b'},                       /* move char left */
    {mvcl, 'h'},                        /* move char left */
    {mvcl, K_LEFT},                     /* move char left */
    {mvcr, 'l'},                        /* move char right */
    {mvcr, ' '},                        /* move char right */
    {mvcr, K_RIGHT},                    /* move char right */
    {mvbeg, '^'},                       /* move to beginning of text */
    {mvbeg, K_HOME},                    /* move to beginning of text */
    {mvfbw, 'W'},                       /* move forward to major word */
    {mvfbw, K_F3},                      /* move forward to major word */
    {mvfw, 'w'},                        /* move forward to minor word */
    {mvfw, K_F4},                       /* move forward to minor word */
    {mvbbw, 'B'},                       /* move backward to major word */
    {mvbbw, K_F5},                      /* move backward to major word */
    {mvbw, 'b'},                        /* move backward to minor word */
    {mvbw, K_F6},                       /* move backward to minor word */
    {search, '/'},                      /* search for string match */
    {next, 'n'},                        /* repeat last search */
/* delele thru movement commands follow */
    {mvend, '$'},                       /* move to end of text */
    {mvend, K_END},                     /* move to end of text */
    {findr, 'f'},                       /* find char to right */
    {findr, '+'},                       /* find char to right */
    {findl, 'F'},                       /* find char to left */
    {findl, '-'},                       /* find char to left */
    {repf, ';'},                        /* repeat last find */
    {repf, K_F2},                       /* repeat last find */
/* change commands follow */
    {ins, 'i'},                         /* insert */
    {ins, K_INS},                       /* insert */
    {apndnxt, 'a'},                     /* append next */
    {apndnxt, K_F10},                   /* append next */
    {apndend, 'A'},                     /* append at end */
    {apndend, K_F9},                    /* append at end */
    {repch, 'r'},                       /* replace char */
    {repch, K_F8},                      /* replace char */
    {reptxt, 'R'},                      /* replace text */
    {reptxt, K_F7},                     /* replace text */
    {delr, 'x'},                        /* delete char right */
    {delr, K_DEL},                      /* delete char right */
    {setdel, 'd'},                      /* set delete mode */
    {undo, 'u'},                        /* undo last change */
    {undo, K_F1},                       /* undo last change */
    {NULL, '\0'}
};

/* ================================================================
    Edit data field contents
*/
int ed_field(int create)
{
    register int i;
    int rn, cn;

    len = strlen(fldtxt);
    cp = 0;
    lp = 0;
    chglen = 0;
    if ((rp = lp + DWIDTH - 1) > len - 1)
        rp = len - 1;
    if (create)
        untgetch('i');
    else
        tprintf("@m0060@SMode:@s COMMAND");

    tprintf("@m0500@e@SEDIT:@s");
    pr_text();
    for (;;)
    {
        cc = tgetch();

        /* check for repitition number */
        for (rep = 0; cc >= '0' && cc <= '9';)
        {
            rep = 10 * rep + (int) (cc - '0');
            cc = tgetch();
        }
        if (rep == 0)
            rep = 1;

        /* lookup command */
        for (i = 0; cmds[i].key && cmds[i].key != cc; ++i)
            ;                             /* find command character in cmds
                                           * table */

        /* process edit command */
        switch (cc)
        {
            case K_CANCEL:
            case K_BREAK:
            case K_ESC:
                return (0);
            case '\n':
            case K_FTAB:
            case K_DOWN:
                return (1);
            case K_UP:
                return (-1);
            default:
                if (cmds[i].key)
                {
                    if (!deleting)
                    {
                        if ((*(cmds[i].fcn)) ())
                            return (1);
                    }
                    else if (i < MOVECMDS)
                    {
                        if ((*(cmds[i].fcn)) ())
                            return (1);
                        delete(i);
                    }
                    else
                    {
                        deleting = 0;
                        beep();
                    }
                }
                else
                    beep();
        }
        /* update display */
        if (!refresh && cp >= lp && cp <= rp)
        {
            rn = 5;
            cn = cp - lp + 6;
            tprintf("@M", &rn, &cn);
        }
        else
            pr_text();

        /* remember last command */
        lc = cc;
    }
}

/* ================================================================
    Move left one character
*/
static int mvcl()
{
    if (cp)
        cp = (cp - rep > 0) ? cp - rep : 0;
    return (0);
}

/* ================================================================
    Move right one character
*/
static int mvcr()
{
    if (cp < len - 1)
        cp = (cp + rep < len) ? cp + rep : len - 1;
    return (0);
}

/* ================================================================
    Move to beginning of text
*/
static int mvbeg()
{
    cp = 0;
    return (0);
}

/* ================================================================
    Move to end of text
*/
static int mvend()
{
    cp = len - 1;
    return (0);
}

/* ================================================================
    Find character to right
*/
static int findr()
{
    register int i;

    if (fc)
        sc = tgetch();
    fc = 'f';
    i = cp;
    while (rep-- && ++i < len)
    {
        while (i < len && fldtxt[i] != sc)
            ++i;
    }
    if (fldtxt[i] == sc)
        cp = i;
    else
        beep();
    return (0);
}

/* ================================================================
    Find character to left
*/
static int findl()
{
    register int i;

    if (fc)
        sc = tgetch();
    fc = 'F';
    i = cp;
    while (rep-- && --i >= 0)
    {
        while (i >= 0 && fldtxt[i] != sc)
            --i;
    }
    if (fldtxt[i] == sc)
        cp = i;
    else
        beep();
    return (0);
}

/* ================================================================
    Repeat last find
*/
static int repf()
{
    if (fc == 'F')
    {
        fc = '\0';                       /* signals repeat */
        findl();
    }
    else if (fc == 'f')
    {
        fc = '\0';                       /* signals repeat */
        findr();
    }
    return (0);
}

/* ================================================================
    Forward major word
*/
static int mvfbw()
{
    register int i;

    i = cp;
    while (rep-- && ++i < len)
    {
        /* find first blank */
        while (i < len && fldtxt[i] != ' ')
            ++i;
        /* find first non-blank after first blank */
        while (i < len && fldtxt[i] == ' ')
            ++i;
    }
    if (i >= len)
        beep();
    else
        cp = i;
    return (0);
}

/* ================================================================
    Forward minor word
*/
static int mvfw()
{
    register int i;

    i = cp;
    while (rep-- && ++i < len)
    {
        /* find first blank */
        while (i < len && isalpha(fldtxt[i]))
            ++i;
        /* find first non-blank after first blank */
        while (i < len && !isalpha(fldtxt[i]))
            ++i;
    }
    if (i >= len)
        beep();
    else
        cp = i;
    return (0);
}

/* ================================================================
    Backward major word
*/
static int mvbbw()
{
    register int i;

    i = cp - 1;
    while (rep-- && i >= 0)
    {
        /* find first blank */
        while (i >= 0 && fldtxt[i] != ' ')
            --i;
        /* find first non-blank after first blank */
        while (i >= 0 && fldtxt[i] == ' ')
            --i;
        /* find first blank */
        while (i >= 0 && fldtxt[i] != ' ')
            --i;
    }
    if (fldtxt[++i] != ' ')
        cp = i;
    else
        beep();
    return (0);
}

/* ================================================================
    Backward minor word
*/
static int mvbw()
{
    register int i;

    i = cp - 1;
    while (rep-- && i >= 0)
    {
        /* find first non-alpha */
        while (i >= 0 && isalpha(fldtxt[i]))
            --i;
        /* find first alpha after first blank */
        while (i >= 0 && !isalpha(fldtxt[i]))
            --i;
        /* find first non-alpha */
        while (i >= 0 && isalpha(fldtxt[i]))
            --i;
    }
    if (isalpha(fldtxt[++i]))
        cp = i;
    else
        beep();
    return (0);
}

/* ================================================================
    Search for string match
*/
static int search()
{
    char txt[81];
    register int i, sl;

    tprintf("@m2300@E/");
    if (rdtext(txt) > 0)
        strcpy(srch, txt);

    if ((sl = strlen(srch)) != 0)
    {
        for (i = cp + 1; i <= len - sl; ++i)
            if (strncmp(fldtxt + i, srch, sl) == 0)
            {
                cp = i;
                return (0);
            }
    }
    return (0);
}

/* ================================================================
    Repeat last search
*/
static int next()
{
    untgetch('\n');
    untgetch('/');
    return (0);
}

/* ================================================================
    Insert new text
*/
static int ins()
{
    register int i;
    register char sc;
    int rn, cn;

    rn = 5;
    cn = cp - lp + 6;
    tprintf("@m0060@SMode:@s INSERT @M", &rn, &cn);
    chgpos = cp;
    chglen = 0;
    chgcmd = 'i';
    for (;;)
    {
        sc = tgetch();
        if (sc == K_ESC || sc == K_BREAK || sc == K_CANCEL || sc == '\n')
        {
            if (chglen == MAXCHG)
                chglen = 0;
            if (cp)
                --cp;
            fldtxt[len] = '\0';
            tprintf("@m0060@SMode:@s COMMAND");
            return ((sc == K_ESC || sc == K_BREAK || sc == K_CANCEL) ? 0 : 1);
        }
        if (sc == '\b' && chglen)
        {
            for (i = --cp; i < len - 1; ++i)
                fldtxt[i] = fldtxt[i + 1];
            --chglen;
            --len;
        }
        else if (sc >= ' ' && sc <= '~' && cp < MAXSZ)
        {
            for (i = len; i > cp; --i)
                fldtxt[i] = fldtxt[i - 1];
            fldtxt[cp++] = sc;
            if (chglen < MAXCHG)
                chgbuf[chglen++] = sc;
            ++len;
        }
        else
        {
            if (task->ctbl_activ && task->country_tbl[(unsigned char) sc].out_chr &&
                 cp < MAXSZ)
            {
                for (i = len; i > cp; --i)
                    fldtxt[i] = fldtxt[i - 1];
                fldtxt[cp++] = sc;
                if (chglen < MAXCHG)
                    chgbuf[chglen++] = sc;
                ++len;
            }
        }
        pr_text();
    }
}

/* ================================================================
    Append at next position
*/
static int apndnxt()
{
    register int i;

    if (len == 0)
        return (ins());

    if (cp == len - 1)
    {
        /* add space to end */
        fldtxt[len++] = ' ';
        fldtxt[len] = '\0';
    }
    ++cp;
    pr_text();
    i = ins();
    if (cp == len - 2)
    {
        fldtxt[--len] = '\0';
        refresh = 1;
    }
    return (i);
}

/* ================================================================
    Append at end of text
*/
static int apndend()
{
    untgetch('a');
    untgetch('$');
    return (0);
}

/* ================================================================
    Replace char
*/
static int repch()
{
    char rc;

    rc = tgetch();
    chgbuf[0] = fldtxt[cp];
    chglen = 1;
    chgpos = cp;
    chgcmd = 'r';
    fldtxt[cp] = rc;
    tprintf("%c", &rc);
    return (0);
}

/* ================================================================
    Replace text
*/
static int reptxt()
{
    register char rc;
    int rn, cn;

    rn = 5;
    cn = cp - lp + 6;
    tprintf("@m0060@SMode:@s REPLACE@M", &rn, &cn);
    chgpos = cp;
    chglen = 0;
    chgcmd = 'r';
    for (;;)
    {
        rc = tgetch();
        if (rc == K_ESC || rc == K_BREAK || rc == K_CANCEL || rc == '\n')
        {
            if (chglen == MAXCHG)
                chglen = 0;
            if (cp)
                --cp;
            tprintf("@m0060@SMode:@s COMMAND");
            return ((rc == K_ESC || rc == K_BREAK || rc == K_CANCEL) ? 0 : 1);
        }
        if (rc == '\b' && chglen)
        {
            fldtxt[--cp] = chgbuf[--chglen];
        }
        else if (rc >= ' ' && rc <= '~' && cp < len)
        {
            if (task->ctbl_activ && task->country_tbl[(unsigned char) sc].out_chr)
                if (chglen < MAXCHG)
                    chgbuf[chglen++] = fldtxt[cp];
            fldtxt[cp++] = rc;
        }
        else
        {
            if (task->ctbl_activ && task->country_tbl[(unsigned char) rc].out_chr &&
                 cp < len)
            {
                if (chglen < MAXCHG)
                    chgbuf[chglen++] = fldtxt[cp];
                fldtxt[cp++] = rc;
            }
        }
        pr_text();
    }
}

/* ================================================================
    Delete character under cursor (right)
*/
static int delr()
{
    register int i;

    if (rep > len - cp)
        rep = len - cp;
    if (len)
    {
        for (chglen = 0; chglen < rep && rep < MAXCHG; ++chglen)
            chgbuf[chglen] = fldtxt[cp + chglen];
        chgbuf[chglen] = '\0';
        chgcmd = 'x';
        chgpos = cp;
        for (i = cp; i <= len - rep; ++i)
            fldtxt[i] = fldtxt[i + rep];
        if (cp >= (len = len - rep))
        {
            cp = len - 1;
            rp = cp;
            if ((lp = rp - DWIDTH + 1) < 0)
                lp = 0;
        }
        refresh = 1;
    }
    return (0);
}

/* ================================================================
    Set delete mode
*/
static int setdel()
{
    dp = cp;
    deleting = 1;
    return (0);
}

/* ================================================================
    Delete text between dp and cp
*/
static int delete(int cmd)
{
    if (cp < dp)
    {
        rep = dp - cp;
    }
    else if (cp > dp)
    {
        rep = cp - dp;
        if (cmd >= THRUCMDS)
            ++rep;
        cp = dp;
    }
    else
        return (0);
    deleting = 0;
    return (delr());
}

/* ================================================================
    Undo last change
*/
static int undo()
{
    register int i;
    register int rc;

    if (chglen)
    {
        switch (chgcmd)
        {
            case 'x':
                for (i = len + chglen; i >= chgpos + chglen; --i)
                    fldtxt[i] = fldtxt[i - chglen];
                for (i = 0; i < chglen; ++i)
                    fldtxt[chgpos + i] = chgbuf[i];
                len = len + chglen;
                rp = lp + DWIDTH - 1;
                if (rp > len - 1)
                    rp = len - 1;
                cp = chgpos;
                chgcmd = 'i';
                refresh = 1;
                break;
            case 'i':
                rep = chglen;
                cp = chgpos;
                delr();
                break;
            case 'r':
                for (i = 0; i < chglen; ++i)
                {
                    rc = fldtxt[chgpos + i];
                    fldtxt[chgpos + i] = chgbuf[i];
                    chgbuf[i] = (char) rc;
                }
                cp = chgpos;
                refresh = 1;
                break;
        }
    }
    return (0);
}

/* ================================================================
    Print field text from specified column
*/
static int pr_text()
{
    register int i;
    int rn, cn;

    if (rp > len - 1)
    {
        rp = len - 1;
        if ((lp = rp - DWIDTH + 1) < 0)
            lp = 0;
    }
    if (len == 0)
    {
        cp = lp = rp = 0;
    }
    else if (len <= DWIDTH)
    {
        lp = 0;
        rp = len - 1;
    }
    else if (cp < lp)
    {
        lp = cp;
        rp = lp + DWIDTH - 1;
    }
    else if (cp > rp)
    {
        rp = (cp < len) ? cp : len - 1;
        lp = rp - DWIDTH + 1;
    }
    tprintf("@o");

    if (lp)
        tprintf("@m0505@S<@s");
    else
        tprintf("@m0505@S>@s");

    for (i = lp; len && i <= rp; ++i)
        tprintf("%c", fldtxt + i);

    if (rp == 0 || rp == len - 1)
        tprintf("@S<@s@e");
    else
        tprintf("@S>@s@e");

    rn = 5;
    cn = cp - lp + 6;
    tprintf("@M@O", &rn, &cn);

    refresh = 0;
    return 0;
}

/* ================================================================
    Beep for users attention
*/
void beep()
{
    putchar('\7');
}


