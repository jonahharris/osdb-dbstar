/***************************************************************************
 *                                                                         *
 * db.*                                                                    *
 * open source database, lock manager console utility                      *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reserved.   *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      * 
 *                                                                         *
 **************************************************************************/

#include "db.star.h"
#include "dbtype.h"

#include <unistd.h>
#include <signal.h>
#include "console.h"
#include "ida.h"

#define G_TOP        12
#define UR_TOP        2
#define FR_TOP        2
#define US_TOP        9
#define FS_TOP        3

#define U_NAME_COL    2
#define U_PEND_COL   21
#define U_TIME_COL   26
#define U_STAT_COL   31
#define U_REC_COL    41
#define U_LOG_COL    46

#define F_NUM_COL     0
#define F_NAME_COL    5

#define F_NAME_LEN   24
#define F_STAT_COL   30
#define F_NUML_COL   36
#define F_HOLD_COL   39
#define F_PEND_COL   57

#define G_QITMS_XY    6, 64
#define G_MSGSR_XY    8, 64
#define G_MSGSS_XY    9, 64
#define G_GRANT_XY    6, 11
#define G_REJCT_XY    7, 11
#define G_TMOUT_XY    8, 11
#define G_TOTAL_XY    9, 11
#define G_FREED_XY    9, 27
#define G_NUSER_XY   11, 06

#define US_NAME_XY    0, 12
#define US_TOUT_XY    0, 75
#define US_PID_XY     1, 12
#define US_TIME_XY    1, 75
#define US_TAF_XY     2, 12
#define US_STAT_XY    4, 12
#define US_REC_XY     5, 12
#define US_LOG_XY     6, 12
#define US_LOCK_XY    8,  4
#define US_PEND_XY    8, 14
#define US_OPEN_XY    8, 24
#define US_NET1_XY    4, 50
#define US_NET2_XY    5, 50
#define US_NET3_XY    6, 50

#define FS_NAME_XY    0, 16
#define FS_LOCK_XY    2,  4
#define FS_REQ_XY     2, 13
#define FS_OPEN_XY    2, 23

#define MAXROWS 50
#define CMD_LINE 23


char *commands[] = {
    "USERS",
    "USER",
    "STATUS",
    "SHUTDOWN",
    "REFRESH",
    "QUIT",
    "FILES",
    "FILE",
    "KILL",
    "HELP",
    "EXIT",
    NULL
};

enum cmd {
    CMD_USERS,
    CMD_USER,
    CMD_STATUS,
    CMD_SHUTDOWN,
    CMD_REFRESH,
    CMD_QUIT,
    CMD_FILES,
    CMD_FILE,
    CMD_KILL,
    CMD_HELP,
    CMD_EXIT,
    CMD_DEFAULT
};

enum outline
{
    NO_OUTLINE,
    GENERAL_STATUS,
    USER_REPORT,
    FILE_REPORT,
    USER_STATUS,
    FILE_STATUS
};

typedef struct USERLINE_S
{
    int  clr;
    int  pend;
    int  time;
    char rec[4];
    char stat[9];
    char name[17];
    char log[17];
} USERLINE;

typedef struct FILELINE_S
{
    int  clr;
    int  num;
    int  numl;
    char stat;
    char name[25];
    char hold[17];
    char pend[33];
} FILELINE;

typedef struct GENLINE_S
{
    int clr;
    char name[5][16];
} GENLINE;

typedef struct GENSCREEN_S
{
    long qitms;
    long msgsr;
    long msgss;
    long grant;
    long rejct;
    long tmout;
    long total;
    long freed;
    int  clr;
    int  nuser;
    GENLINE *lines;
} GENSCREEN;

typedef struct USERDATALINE_S
{
    int  clr;
    char lock;
    char req;
    char name[45];
} USERDATALINE;

typedef struct USERDATASCREEN_S
{
    long pid;
    int  clr;
    int  timeout;
    int  timer;
    int  numlocks;
    int  numreq;
    int  numopen;
    char name[17];
    char status[9];
    char recover[4];
    char logfile[FILENMLEN];
    char taffile[FILENMLEN];
    USERDATALINE *lines;
} USERDATASCREEN;

typedef struct FILEDATALINE_S
{
    int  clr;
    char lock;
    char req;
    char name[17];
} FILEDATALINE;

typedef struct FILEDATASCREEN_S
{
    int  clr;
    int  numlock;
    int  numreq;
    int  numopen;
    char name[FILENMLEN];
    FILEDATALINE *lines;
} FILEDATASCREEN;

/*** Local Functions ***/

static void repaint(int);
static void general_status(short);
static void user_report(short);
static void file_report(short);
static void user_status(short);
static void file_status(short);
static void help_func(short);
static void print_gs_outline(void);
static void print_ur_outline(void);
static void print_fr_outline(void);
static void print_us_outline(void);
static void print_fs_outline(void);
static void print_gs_data(short);
static void print_ur_data(short);
static void print_fr_data(short);
static void print_us_data(short, unsigned short);
static void print_fs_data(short, unsigned short);
static void print_user_line(short, unsigned short);
static void print_file_line(short, unsigned short);
static void clear_line(short, int *);
static void show_help(void);
static int  get_command();
static int  do_command(char *);
static int  good_char(char);
static void print_command_line(void);
static void locate_to_cmnd_line(void);
static int  is_blank(char *);
static void str_test(short, short, char *, char *, int, int);
static void long_test(short, short, long *, long, int);
static void int_test(short, short, int *, int, int);
static void spc_test(short, short, int *, int, int);
static void char_test(short, short, char *, char, int);

static void vioPrint(int, int, char *);
static void vioPutChar(short);
static void vioLocate(int, int);
static void vioClear(void);
static void vioClearLine(int);

/*** Local Data ***/

#define MAXCOMMAND  22
static enum outline curr_outline = NO_OUTLINE;
static char  command[MAXCOMMAND + 2];
static void  (*report_func)(short) = general_status;
static void  (*temp_func)(short);

static int             vio_ok = 0;
static short           top_line = 1;
static char            user_to_view[16];
static unsigned short  file_to_view;


static USERLINE       *user_line  = NULL;
static FILELINE       *file_line  = NULL;
static GENSCREEN      *general    = NULL;
static USERDATASCREEN *user_data  = NULL;
static FILEDATASCREEN *file_data  = NULL;
static short          *files_open = NULL;
static short          *users_open = NULL;

/**************************************************************************/

void error_msg(char *msg)
{
    if (vio_ok)
    {
        vioClearLine((short) (CMD_LINE + 1));
        vioPrint((short) (CMD_LINE + 1), 0, msg);
    }
    else
        printf(msg);
}

/**************************************************************************/

static int good_char(char key)
{
    if (isalnum(key)
     || (key == ' ') || (key == '?') || (key == '_') || (key == '$'))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**************************************************************************/

#define MAXRESULT 18

static int is_blank(char *data)
{
    while (*data)
    {
        if (*data++ != ' ')
            return 0;
    }

    return 1;
}

/**************************************************************************/

static void str_test(
    short x,
    short y,
    char *prev_data,
    char *new_data,
    int   clear,
    int   len)
{
    char temp[65];
    int old_len = strlen(prev_data);
    int data_len = strlen(new_data);

    len = min(len, (int) sizeof(temp)-1);
    data_len = min(data_len, len);
    len = max(data_len, old_len);

    if (clear || strncmp(new_data, prev_data, len))
    {
        memcpy(temp, new_data, data_len);
        memset(temp + data_len, ' ', len - data_len);
        temp[len] = '\0';
        vioPrint(x, y, temp);
        temp[data_len] = '\0';
        strcpy(prev_data, temp);
    }
}

/**************************************************************************/

static void long_test(
    short x,
    short y,
    long *prev_data,
    long  new_data,
    int   clear)
{
    char temp[16];

    if (clear || (new_data != *prev_data))
    {
        sprintf(temp, "%-7ld", new_data);
        vioPrint(x, y, temp);
        *prev_data = new_data;
    }
}

/**************************************************************************/

static void int_test(
    short x,
    short y,
    int  *prev_data,
    int   new_data,
    int   clear)
{
    char temp[10];

    if (clear || (new_data != *prev_data))
    {
        sprintf(temp, "%-5d", new_data);
        vioPrint(x, y, temp);
        *prev_data = new_data;
    }
}

/**************************************************************************/

static void spc_test(
    short x,
    short y,
    int  *prev_data,
    int   new_data,
    int   clear)
{
    char temp[10];

    if (clear || (new_data != *prev_data))
    {
        sprintf(temp, "(%d)_", new_data);
        vioPrint(x, y, temp);
        *prev_data = new_data;
    }
}

/**************************************************************************/

static void char_test(
    short x,
    short y,
    char *prev_data,
    char  new_data,
    int   clear)
{
    char temp[2] = " ";

    if (clear || (new_data != *prev_data))
    {
        temp[0] = new_data;
        vioPrint(x, y, temp);
        *prev_data = new_data;
    }
}

#define  keyBackSpace 0x08
#define  keyEsc       0x1B
#define  keyHome      0x47
#define  keyEnter     0x0A
#define  keyUp        0x48
#define  keyDown      0x45
#define  keyLeft      0x44
#define  keyRight     0x43
#define  keyEnd       0x4F
#define  keyPgUp      0x41
#define  keyPgDn      0x42
#define  keyFFFF      0xFFFF
#define  keyEscape    0x1b
#define  keyExtended  0xe0

/**************************************************************************/

static int get_command()
{
    unsigned short key = 0, row;
    int stat, extended = 0;
    static int next_char;
    static int first_time = 1;

    if (first_time)
    {
        memset(command, 0, sizeof(command));
        next_char = 0;
        first_time = 0;
        repaint(0);
    }

    if (report_func == help_func)
    {
        getchar();
        first_time = 1;

        for (row = 11; row <= (unsigned short)(CMD_LINE + 1); ++row)
            vioClearLine(row);

        curr_outline = NO_OUTLINE;

        report_func = temp_func;
        if (report_func)
            (*report_func)(0);

        print_command_line();
    }
    else
    {
        key = (short) getchar();
        if (key != keyFFFF)
            alarm(0);

        extended = 0;
        if ((key == keyEscape) || (key == keyExtended))
        {
            extended = 1;
            getchar();                    /* Ignore second char */
            key = (short) getchar();
        }
    }

    if (key)
    {
        if ((key == keyEnter) && (next_char > 0))
        {
            stat = do_command(command);
            memset(command, 0, sizeof(command));
            if (stat != CMD_HELP)
            {
                first_time = TRUE;
                print_command_line();
            }
            next_char = 0;
            return stat;
        }

        if ( extended &&
             ( (key == keyPgDn) || (key == keyPgUp) ||
               (key == keyHome) || (key == keyEnd) ||
               (key == keyLeft) || (key == keyRight) ) )
        {
            if (report_func)
                (*report_func)(key);

            return CMD_DEFAULT;
        }

        if (key == keyEsc)
        {
            memset(command, 0, sizeof(command));
            next_char = 0;
            print_command_line();
            return CMD_DEFAULT;
        }

        if (key == keyBackSpace)
        {
            if (next_char > 0)
            {
                command[--next_char] = '\0';

                vioPutChar(key);
                vioPutChar(' ');
                vioPutChar(key);
            }
        }
        else
        {
            if ((next_char < MAXCOMMAND) && (!extended) && good_char((char) key))
            {
                command[next_char++] = (char) key;
                command[next_char]   = '\0';
                vioPutChar(key);
            }
        }
    }

    return CMD_DEFAULT;
}

/**************************************************************************/

static int do_command(char *command)
{
    register enum cmd i;
    int n;
    char *p;

    /* skip leading spaces */
    while (*command == ' ')
        ++command;

    for (i = 0; commands[i]; i++)
        if (strnicmp(commands[i], command, strlen(commands[i])) == 0)
            break;

    command[0] = '\0';

    switch (i)
    {
        case CMD_FILE:
            if (! is_blank(command + 4))
            {
                p = command + 4;
                
                while (*p == ' ')
                    p++;

                file_to_view = atoi(p);

                file_status(-1);
            }
            break;

        case CMD_FILES:
            file_report(-1);
            break;

        case CMD_HELP:
            show_help();
            break;

        case CMD_KILL:
            if (!is_blank(command + 4))
            {
                p = command + 4;

                while (*p == ' ')
                    p++;

                kill_user(p);
            }
            break;

        case CMD_QUIT:
        case CMD_EXIT:
            report_func = NULL;
            return CMD_QUIT;

        case CMD_REFRESH:
            if (strlen(command) > 7 && !is_blank(command + 7))
            {
                n = atoi(command + 7);
                if (n >= 2 && n <= 59)
                    refresh_secs = n;
            }
            break;

        case CMD_SHUTDOWN:
            kill_lockmgr();
            report_func = NULL;
            return CMD_QUIT;

        case CMD_STATUS:
            general_status(0);
            break;

        case CMD_USERS:
            user_report(-1);
            break;

        case CMD_USER:
            if (!is_blank(command + 4))
            {
                p = command + 4;

                while (*p == ' ')
                    p++;

                strcpy(user_to_view, p);
                while (user_to_view[strlen(user_to_view) - 1] == ' ')
                    user_to_view[strlen(user_to_view) - 1] = '\0';

                user_status(-1);
            }
            break;

        default:
            return CMD_DEFAULT;
    }

    return i;
}

/**************************************************************************/

static void clear_line(short row, int *clear)
{
    if (!*clear)
    {
        *clear = TRUE;
        vioClearLine(row);
    }
}


/**************************************************************************/

static void print_gs_outline()
{
    if (curr_outline != GENERAL_STATUS)
    {
        vioClear();
        vioPrint(0, (short) (40 - (strlen(title1) / 2)), title1);
        vioPrint(1, (short) (40 - (strlen(title2) / 2)), title2);
        vioPrint(2,  0, "Name:");
        vioPrint(2,  8, lockmgrn);
        vioPrint(6, 43, "Receive Queue items:");
        vioPrint(8, 54, "Requests:");
        vioPrint(9, 55, "Replies:");
        vioPrint(4,  0, "Locks");
        vioPrint(5,  0, "_______________________________");
        vioPrint(6,  2, "Granted:");
        vioPrint(7,  1, "Rejected:");
        vioPrint(8,  0, "Timed Out:");
        vioPrint(9,  4, "Total:");
        vioPrint(9, 20, "Freed:");
        vioPrint(11, 0, "Users    _______________________________________________________________");

        curr_outline = GENERAL_STATUS;
        general->clr = TRUE;
    }
}

/**************************************************************************/

static void print_gs_data(short scroll)
{
    unsigned short col, row = G_TOP, user = 0;
    unsigned short num_users = GetNumUsers();
    int   user_no = 0;
    char  temp_s[16];
    int   clear = general->clr, done = FALSE;

    long_test(G_QITMS_XY, &general->qitms, GetNumQItems(), clear);
    long_test(G_MSGSR_XY, &general->msgsr, GetMsgsReceived(), clear);
    long_test(G_MSGSS_XY, &general->msgss, GetMsgsSent(), clear);
    long_test(G_GRANT_XY, &general->grant, GetLocksGranted(), clear);
    long_test(G_REJCT_XY, &general->rejct, GetLocksRejected(), clear);
    long_test(G_TMOUT_XY, &general->tmout, GetLocksTimedOut(), clear);
    long_test(G_TOTAL_XY, &general->total, GetTotalLocks(), clear);
    long_test(G_FREED_XY, &general->freed, GetLocksFreed(), clear);

    do
    {
        clear = general->lines[row - G_TOP].clr | general->clr;
        for (col = 0; (col < 5); col++, user++)
        {
            strcpy(temp_s, "               ");

            if (!done)
            {
                while ((user < num_users) && !IsUserActive(user))
                    user++;

                if (user < num_users)
                {
                    sprintf(temp_s, "%-15.15s", GetUserId(user));
                    user_no++;
                }
                else
                    done = TRUE;
            }

            if (clear || strcmp(temp_s, general->lines[row - G_TOP].name[col]))
            {
                vioPrint(row, (short) (col * 16), temp_s);
                strcpy(general->lines[row - G_TOP].name[col], temp_s);
            }

            general->lines[row - G_TOP].clr = FALSE;
        }

        row++;

        if (done && general->lines[row - G_TOP].name[col][0] == '\0')
            break;

    } while ((user < num_users) && (row < CMD_LINE));

    if (row >= CMD_LINE)
    {
        while (user < num_users)
        {
            if (IsUserActive(user++))
                user_no++;
        }
    }

    spc_test(G_NUSER_XY, &general->nuser, user_no, general->clr);

    while (row < CMD_LINE)
    {
        clear_line(row, &general->lines[row - G_TOP].clr);
        row++;
    }

    general->clr = FALSE;

    locate_to_cmnd_line();
}

/**************************************************************************/

static void general_status(short scroll)
{
    report_func = general_status;

    top_line = G_TOP;
    get_tables();
    print_gs_outline();
    print_gs_data(scroll);
}

/**************************************************************************/

static void print_ur_outline()
{
    unsigned short i;

    if (curr_outline != USER_REPORT)
    {
        vioClear();
        vioPrint(0, 2,
            "User Name          Q    TO   Status    Rec  Log File");
        vioPrint(1, 2,
            "_________________  ___  ___  ________  ___  __________");

        curr_outline = USER_REPORT;
        for (i = 0; i < (unsigned short)(CMD_LINE - UR_TOP); i++)
            user_line[i].clr = TRUE;
    }
}

/**************************************************************************/

static void print_user_line(short row, unsigned short user)
{
    int tmp = row - UR_TOP;
    int clear = user_line[tmp].clr;

    str_test(row, U_NAME_COL, user_line[tmp].name,  GetUserId(user),        clear, 16);
    int_test(row, U_PEND_COL, &user_line[tmp].pend, GetUserPending(user),   clear);
    int_test(row, U_TIME_COL, &user_line[tmp].time, GetUserTimer(user),     clear);
    str_test(row, U_STAT_COL, user_line[tmp].stat,  GetUserStatus(user),    clear, 8);
    str_test(row, U_REC_COL,  user_line[tmp].rec,   GetUserRecover(user),   clear, 3);
    str_test(row, U_LOG_COL,  user_line[tmp].log,   GetUserLogFile(user),   clear, 16);

    user_line[tmp].clr = FALSE;
}

/**************************************************************************/

static void print_ur_data(short scroll)
{
    unsigned short    user;
    static short      top = 0;
    short             prev_top = top;
    unsigned short    row,
                      num_users = GetNumUsers();

    if (scroll == -1)
    {
        top = 0;
    }
    else
    {
        if (scroll == keyPgDn)
        {
            top = CMD_LINE - 2;
            if (top > (short) (num_users - CMD_LINE + 2))
                top = num_users - CMD_LINE + 2;
        }
        else
        {
            if (scroll == keyPgUp)
                top -= CMD_LINE - 2;
        }

        if (scroll == keyEnd)
            top = num_users - CMD_LINE + 2;

        if ((scroll == keyHome) || (top < 0))
            top = 0;

        if (scroll && (top == prev_top))
            return;
    }

    for ( user = top, row = UR_TOP;
            (user < num_users) && (row < CMD_LINE);
            user++)
    {
        print_user_line(row++, user);
    }

    while (row < (unsigned short)(CMD_LINE - 1))
    {
        clear_line(row, &user_line[row - UR_TOP].clr);
        row++;
    }

    locate_to_cmnd_line();
}

/**************************************************************************/

static void user_report(short scroll)
{
    report_func = user_report;

    top_line = UR_TOP;
    get_tables();
    print_ur_outline();
    print_ur_data(scroll);
}

/**************************************************************************/

static void print_fr_outline()
{
    unsigned short i;

    if (curr_outline != FILE_REPORT)
    {
        vioClear();
        vioPrint(0, 0,
            "Num  Device@Host:Inode        Lock  #  Holding lock      Waiting");
        vioPrint(1, 0,
            "___  ________________________ ____  _  ________________  ____________________");

        curr_outline = FILE_REPORT;
        for (i = 0; i < (unsigned short)(CMD_LINE - FR_TOP); i++)
            file_line[i].clr = TRUE;
    }
}

static void print_file_line(short row, unsigned short file)
{
    unsigned tmp = row - FR_TOP;
    int clear = file_line[tmp].clr;

    int_test(row, F_NUM_COL,   &file_line[tmp].num,  file, clear);
    str_test(row, F_NAME_COL,  file_line[tmp].name,  GetFileName(file, FALSE),
            clear, F_NAME_LEN);
    char_test(row, F_STAT_COL, &file_line[tmp].stat, GetLockStat(file), clear);
    int_test(row, F_NUML_COL,  &file_line[tmp].numl, GetNumLocks(file), clear);
    str_test(row, F_HOLD_COL,  file_line[tmp].hold,  GetUserWLock(file), clear,
            16);
    str_test(row, F_PEND_COL,  file_line[tmp].pend,  GetPendLocks(file), clear,
            30);

    file_line[tmp].clr = FALSE;
}

/**************************************************************************/

static void print_fr_data(short scroll)
{
    static unsigned short row;
    static short top  = 0;
    short prev_top = top;
    unsigned short num_files = GetNumFiles();
    register unsigned short file;

    if (scroll == -1)
    {
        top = 0;
    }
    else
    {
        if (scroll == keyPgDn)
        {
            top += CMD_LINE - 2;
            if (top > (short) (num_files - CMD_LINE + 2))
                top = num_files - CMD_LINE + 2;
        }
        else
        {
            if (scroll == keyPgUp)
                top -= CMD_LINE - 2;
        }

        if (scroll == keyEnd)
            top = num_files - CMD_LINE + 2;

        if (scroll == keyHome || top < 0)
            top = 0;

        if ((scroll > 0) && (top == prev_top))
            return;
    }

    for (file = top, row = 2; (file < num_files) && row < CMD_LINE; file++)
    {
        if (IsFileActive(file))
            print_file_line(row++, file);
    }

    while (row < (unsigned short)(CMD_LINE - 1))
    {
        clear_line(row, &file_line[row - FR_TOP].clr);
        row++;
    }

    locate_to_cmnd_line();
}

/**************************************************************************/

static void file_report(short scroll)
{
    report_func = file_report;

    top_line = FR_TOP;
    get_tables();
    print_fr_outline();
    print_fr_data(scroll);
}

/**************************************************************************/

static void print_us_outline()
{
    if (curr_outline != USER_STATUS)
    {
        vioClear();
        vioPrint(0, 1,  "User Name:");
        vioPrint(0, 66, "Timeout:");
        vioPrint(1, 68, "Timer:");
        vioPrint(2, 2,  "Taf File:");
        vioPrint(3, 5, "___________________________________________________________________");
        vioPrint(4, 4,  "Status:");
        vioPrint(5, 0,  "Recovering:");
        vioPrint(6, 2,  "Log File:");
        vioPrint(8, 0,  "Lock(0)___Pend(0)___Open(0)_______________________________________________");

        curr_outline = USER_STATUS;
        user_data->clr = TRUE;
    }
}

/**************************************************************************/

static void print_us_data(short scroll, unsigned short user)
{
    static short   top = 0,
                   taf_left = 0,
                   log_left = 0;
    short          prev_top = top,
                   prev_taf = taf_left,
                   prev_log = log_left;
    char           taffile[FILENMLEN];
    char           logfile[FILENMLEN];
    char           temp_s[40];
    char           temp_c;
    unsigned short file,
                   numfiles,
                   num_files_open,
                   row,
                   i;
    int            clear = user_data->clr;

    numfiles = GetNumFiles();
    strcpy(taffile, GetUserTAF(user));
    strcpy(logfile, GetUserLogFile(user));

    for (num_files_open = 0, file = 0; file < numfiles; file++)
    {
        if (UserHasOpen(user, file))
            files_open[num_files_open++] = (short) file;
    }

    if (scroll == -1)
    {
        top = 0;
        taf_left = 0;
        log_left = 0;
    }
    else
    {
        switch (scroll)
        {
            case keyPgDn :
                top += CMD_LINE - 9;
                if (top > (short) (num_files_open - CMD_LINE + 9))
                    top = num_files_open - CMD_LINE + 9;
                break;

            case keyPgUp :
                top -= CMD_LINE - 9;
                if (top < 0)
                    top = 0;
                break;

            case keyLeft :
                taf_left -= 16;
                if (taf_left < 0)
                    taf_left = 0;

                log_left -= 16;
                if (log_left < 0)
                    log_left = 0;
                break;

            case keyRight :
                if (strlen(taffile) > 35)
                {
                    taf_left += 16;
                    if (taf_left > (short) (strlen(taffile) - 35))
                        taf_left = (short) (strlen(taffile) - 35);
                }

                if (strlen(logfile) > 35)
                {
                    log_left += 16;
                    if (log_left > (short) (strlen(logfile) - 35))
                        log_left = (short) (strlen(logfile) - 35);
                }
                break;

            case keyEnd :
                top = num_files_open - CMD_LINE + 9;
                if (top < 0)
                    top = 0;
                break;

            case keyHome:
                top = 0;
                break;
        }

        if (  (scroll > 0) && (top == prev_top) && (taf_left == prev_taf) &&
                (log_left == prev_log))
        {
            return;
        }
    }

    str_test(US_NAME_XY, user_data->name,        GetUserId(user),        clear, 16);
    str_test(US_TAF_XY,  user_data->taffile,     taffile + taf_left,     clear, 35);
    int_test(US_TOUT_XY, &user_data->timeout,    GetUserTimeOut(user),   clear);
    int_test(US_TIME_XY, &user_data->timer,      GetUserTimer(user),     clear);
    str_test(US_STAT_XY, user_data->status,      GetUserStatus(user),    clear, 8);
    str_test(US_REC_XY,  user_data->recover,     GetUserRecover(user),   clear, 3);
    str_test(US_LOG_XY,  user_data->logfile,     logfile + log_left,     clear, 35);
    spc_test(US_LOCK_XY, &user_data->numlocks,   UserNumLock(user),      clear);
    spc_test(US_PEND_XY, &user_data->numreq,     UserNumReq(user),       clear);
    spc_test(US_OPEN_XY, &user_data->numopen,    UserNumOpen(user),      clear);

    for (row = US_TOP, i = top; i < num_files_open && row < CMD_LINE; i++, row++)
    {
        file = files_open[i];

        clear = user_data->lines[row - US_TOP].clr | user_data->clr;

        temp_c = GetLockStat(file);

        if (!UserHasLock(user, file) || (temp_c == 'f'))
            temp_c = ' ';

        char_test(row, 1, &user_data->lines[row - US_TOP].lock, temp_c, clear);
        char_test(row, 11, &user_data->lines[row - US_TOP].req,
            UserWaitingLock(user, file), clear);

        sprintf(temp_s, "(%d) %-36s", file, GetFileName(file, FALSE));
        str_test(row, 20, user_data->lines[row - US_TOP].name, temp_s,
            clear, 45);

        user_data->lines[row - US_TOP].clr = FALSE;
    }

    while (row < (unsigned short)(CMD_LINE - 1))
    {
        clear_line(row, &user_data->lines[row - US_TOP].clr);
        row++;
    }

    user_data->clr = FALSE;

    locate_to_cmnd_line();
}

/**************************************************************************/

static void user_status(short scroll)
{
    unsigned short user;
    unsigned short num_users = GetNumUsers();

    report_func = user_status;

    top_line = US_TOP;
    for (user = 0; user < num_users; user++)
    {
        if (strcmp(user_to_view, GetUserId(user)) == 0)
            break;
    }

    if (user == num_users)
    {
        general_status(0);
        return;
    }

    get_user_info(user);
    print_us_outline();
    print_us_data(scroll, user);
}

/**************************************************************************/

static void print_fs_outline()
{
    if (curr_outline != FILE_STATUS)
    {
        vioClear();
        vioPrint(0, 5, "File Name:");
        vioPrint(2, 0, "Lock(0)__Pend(0)___Open(0)___________________________________________________");

        curr_outline = FILE_STATUS;
        file_data->clr = TRUE;
    }
}

/**************************************************************************/

static void print_fs_data(short scroll, unsigned short file)
{
    static short top = 0, nm_left = 0;
    short prev_top = top, prev_nm = nm_left;
    char name[FILENMLEN];
    char temp_c;
    unsigned short i, user, numusers = GetNumUsers();
    unsigned short num_users_open, row;
    int clear = file_data->clr;

    for (num_users_open = 0, user = 0; user < numusers; user++)
    {
        if (FileOpenByUser(file, user))
            users_open[num_users_open++] = (short) user;
    }

    strcpy(name, GetFileName(file, TRUE));

    if (scroll == -1)
    {
        top = 0;
        nm_left = 0;
    }
    else
    {
        switch (scroll)
        {
            case keyPgDn :
                top += CMD_LINE - 3;
                if (top > (short) (num_users_open - CMD_LINE + 3))
                    top = num_users_open - CMD_LINE + 3;
                break;

            case keyPgUp :
                top -= CMD_LINE - 3;
                if (top < 0)
                    top = 0;
                break;

            case keyLeft :
                nm_left -= 30;
                if (nm_left < 0)
                    nm_left = 0;
                break;

            case keyRight :
                if (strlen(name) > 60)
                {
                    nm_left += 30;
                    if (nm_left > (short) (strlen(name) - 60))
                        nm_left = (short) (strlen(name) - 60);
                }
                break;

            case keyEnd :
                top = num_users_open - CMD_LINE + 3;
                if (top < 0)
                    top = 0;
                break;

            case keyHome:
                top = 0;
                break;
        }

        if ((scroll > 0) && (top == prev_top) && (nm_left == prev_nm))
            return;
    }

    str_test(FS_NAME_XY, file_data->name,     name + nm_left,      clear, 60);
    spc_test(FS_OPEN_XY, &file_data->numopen, FileNumOpen(file),   clear);
    spc_test(FS_LOCK_XY, &file_data->numlock, FileNumLock(file),   clear);
    spc_test(FS_REQ_XY,  &file_data->numreq,  FileNumReq(file),    clear);

    for ( row = FS_TOP, i = top;
          (i < num_users_open) && (row < CMD_LINE);
          i++, row++)
    {
        user = users_open[i];

        clear = file_data->lines[row - FS_TOP].clr | file_data->clr;

        if (FileLockedByUser(file, user))
            temp_c = GetLockStat(file);
        else
            temp_c = ' ';

        char_test(row, 1, &file_data->lines[row - FS_TOP].lock, temp_c, clear);
        char_test(row, 11, &file_data->lines[row - FS_TOP].req,
            FileRequestedByUser(file, user), clear);
        str_test(row, 19, file_data->lines[row - FS_TOP].name,
            GetUserId(user), clear, 16);

        file_data->lines[row - FS_TOP].clr = FALSE;
    }

    while (row < (unsigned short)(CMD_LINE - 1))
    {
        clear_line(row, &file_data->lines[row - FS_TOP].clr);
        row++;
    }

    file_data->clr = FALSE;

    locate_to_cmnd_line();
}

/**************************************************************************/

static void file_status(short scroll)
{
    unsigned short numfiles = GetNumFiles();

    report_func = file_status;

    top_line = FS_TOP;
    if (file_to_view >= numfiles)
    {
        general_status(0);
        return;
    }

    get_file_info(file_to_view);
    print_fs_outline();
    print_fs_data(scroll, file_to_view);
}

/**************************************************************************/

static void help_func(short scroll)
{
    return;
}

/**************************************************************************/

static void show_help()
{
    register unsigned short row;
    static char *help_lines[] =
    {
        "FILE <num>     View more information on file <num>.", 
        "FILES          Display the open file list, lock status and locking user.",
        "KILL <name>    Remove a user from the Lock Manager.",
        "REFRESH <num>  Change the refresh delay to <num> seconds.  (default 2)",
        "SHUTDOWN       Shut down the Lock Manager and exit.",
        "STATUS         Display the Lock Manager general status report.",
        "USER <name>    View more information on a user.",
        "USERS          Display report on users and transaction logfiles.",
        "QUIT/EXIT      Exit the remote console program.",
        "HELP           Display this list.",
        ""
    };

    temp_func = report_func;
    report_func = help_func;

#if defined(SOLARIS) || defined(ISOLARIS) || defined(UNIXWARE)
    vioPrint((short) (top_line - 2), 0,
#else  
    vioPrint((short) (top_line - 1), 0,
#endif 
        "Lock Manager Commands ______________________________________________");

    for (row = top_line; help_lines[row - top_line][0]; row++)
    {
        vioClearLine(row);
        vioPrint(row, 2, help_lines[row - top_line]);
    }

    while (row <= (unsigned short)(CMD_LINE + 1))
        vioClearLine(row++);

    vioPrint((short) (CMD_LINE + 1), 0, "Press any key to continue...            ");
    vioLocate((short) (CMD_LINE + 1), 28);

    return;
}

/**************************************************************************/

static void locate_to_cmnd_line()
{
    vioLocate(CMD_LINE, (short) (9 + strlen(command)));
}

/**************************************************************************/

static void print_command_line()
{
    vioClearLine((short) (CMD_LINE + 1));
    vioPrint(CMD_LINE, 0, "Command [....................]");
    vioPrint(CMD_LINE, 9, command);
}

/**************************************************************************/

void console()
{
    while (get_command() != CMD_QUIT);
}

/**************************************************************************/

int get_display_memory()
{
    user_line = (USERLINE *) psp_cGetMemory((CMD_LINE - UR_TOP) * sizeof(USERLINE), 0);
    if (!user_line) goto cleanup;

    file_line = (FILELINE *) psp_cGetMemory((CMD_LINE - FR_TOP) * sizeof(FILELINE), 0);
    if (!file_line) goto cleanup;

    general = (GENSCREEN *) psp_cGetMemory(sizeof(GENSCREEN), 0);
    if (!general) goto cleanup;

    general->lines = (GENLINE *) psp_cGetMemory((CMD_LINE - G_TOP) * sizeof(GENLINE), 0);
    if (!general->lines) goto cleanup;

    user_data = (USERDATASCREEN *) psp_cGetMemory(sizeof(USERDATASCREEN), 0);
    if (!user_data) goto cleanup;

    user_data->lines = (USERDATALINE *) psp_cGetMemory((CMD_LINE - US_TOP) * sizeof(USERDATALINE), 0);
    if (!user_data->lines) goto cleanup;

    file_data = (FILEDATASCREEN *) psp_cGetMemory(sizeof(FILEDATASCREEN), 0);
    if (!file_data) goto cleanup;

    file_data->lines = (FILEDATALINE *) psp_cGetMemory((CMD_LINE - FS_TOP) * sizeof(FILEDATALINE), 0);
    if (!file_data->lines) goto cleanup;

    files_open = (short *) psp_cGetMemory(512 * sizeof(short), 0);
    if (!files_open) goto cleanup;

    users_open = (short *) psp_cGetMemory(512 * sizeof(short), 0);
    if (!users_open) goto cleanup;

    return S_OKAY;

cleanup:
    free_display_memory();
    return S_NOMEMORY;
}

/**************************************************************************/

void free_display_memory()
{
    if (user_line)
    {
        psp_freeMemory(user_line, 0);
        user_line = NULL;
    }
    if (file_line)
    {
        psp_freeMemory(file_line, 0);
        file_line = NULL;
    }
    if (general)
    {
        if (general->lines)
            psp_freeMemory(general->lines, 0);
        psp_freeMemory(general, 0);
        general = NULL;
    }
    if (user_data)
    {
        if (user_data->lines)
            psp_freeMemory(user_data->lines, 0);
        psp_freeMemory(user_data, 0);
        user_data = NULL;
    }
    if (file_data)
    {
        if (file_data->lines)
            psp_freeMemory(file_data->lines, 0);
        psp_freeMemory(file_data, 0);
        file_data = NULL;
    }
    if (files_open)
    {
        psp_freeMemory(files_open, 0);
        files_open = NULL;
    }
    if (users_open)
    {
        psp_freeMemory(users_open, 0);
        users_open = NULL;
    }
}

/**************************************************************************/

void repaint(int sig)
{
    if (report_func)
    {
        (*report_func)(0);
        print_command_line();
    }

    /* reinstall signal handler */
    signal(SIGALRM, repaint);
    alarm(refresh_secs);
}

/**************************************************************************/

void vioInit()
{
    if (!vio_ok)
    {
        ioc_on();
        vioClear();
        vioLocate(0, 0);
        vio_ok = 1;
    }
}

void vioTerm()
{
    if (vio_ok)
    {
        vioClear();
        ioc_off();
        vio_ok = 0;
    }
}

static void vioPrint(int x, int y, char *string)
{
    tprintf("@M%s", &x, &y, string);
}

static void vioPutChar(short c)
{
    putchar(c);
}

static void vioLocate(int x, int y)
{
    tprintf("@M", &x, &y);
}

static void vioClear()
{
    tprintf("@m0000@E");
}

static void vioClearLine(int row)
{
    int col_zero = 0;
    tprintf("@M@e", &row, &col_zero);
}



