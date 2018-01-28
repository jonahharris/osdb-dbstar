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
    IDA - main program
--------------------------------------------------------------------------*/

#include "db.star.h"
#include "ida_d.h"
#include "ida_t.h"
#include "ida.h"

/************************** EXTERNAL FUNCTIONS ****************************/
extern char dbname[];

extern char tgetch();
extern int db_glob_init;
DB_TASK *task;

void usage(void);

/*************************** GLOBAL VARIABLES *****************************/
int *keyfields;
int tot_keys = 0;
char **keynames;



void EXTERNAL_FCN idaDBErr(int err, char *pszErr)
{

    if (err < 0)
    {
        char *p, pos[10];
        int line;

        /* how many lines? */
        for (line=22, p=pszErr; (p=strchr(p,'\n')) != NULL; ++p)
            --line;

        sprintf(pos, "@m%2.2d00@E", line);
        tprintf(pos); tprintf("@E@R"); tprintf(pszErr);
        tprintf("\n@Rpress <RETURN> to continue@r");
        tgetch();
        tprintf(pos);
    }

    return;
}


/* ========================================================================
    Build list of key names and numbers
*/
void bld_keys()
{
    char buf[70];
    int i;

    for (tot_keys = i = 0; i < task->size_fd; ++i)
    {
        /* count total number of key fields */
        if (task->field_table[i].fd_key != NOKEY)
            ++tot_keys;
    }
    if (tot_keys)
    {
        keyfields = (int *) calloc(tot_keys, sizeof(int));
        keynames = (char **) calloc(tot_keys, sizeof(char *));
        if (keyfields == NULL)
        {
            dberr(S_NOMEMORY);
            return;
        }
        for (tot_keys = i = 0; i < task->size_fd; ++i)
        {
            if (task->field_table[i].fd_key != NOKEY)
            {
                sprintf(buf, "%s.%s",
                        task->record_names[task->field_table[i].fd_rec], task->field_names[i]);
                if (!(keynames[tot_keys] = (char *) malloc(strlen(buf) + 1)))
                {
                    dberr(S_NOMEMORY);
                    return;
                }
                strcpy(keynames[tot_keys], buf);
                keyfields[tot_keys++] = i;
            }
        }
    }
    else
    {
        keyfields = NULL;
    }
}

/* ========================================================================
    Display a reverse-video centered title string
*/
void title(int row, char *str)
{
#define WIDTH 40
    int  col = (80 - WIDTH)/2;
    char buf[8 + WIDTH + 3];
    int  len, pad;

    sprintf(buf, "@m%2.2d%2.2d@R%*c@r", row, col, WIDTH, ' ');

    while (isspace(*str)) ++str;
    len = strlen(str);
    if (len > WIDTH) len = WIDTH;
    while (isspace(str[len])) --len;
    pad = (WIDTH - len)/2;
    strncpy(buf+8+pad, str, len);
    tprintf(buf);
}

/* ========================================================================
    IDA Main Program
*/
int main(int argc, char *argv[])
{
    char           chFlag;
    char           access[5];
    int            i;
    int            j;
    char           version[80];
    char          *v, *p;
    char          *lm_type = NULL;
    LMC_AVAIL_FCN *avail = NULL;
    SG            *sg = NULL;
#if defined(SAFEGARDE)
    char          *cp;
    char          *password = NULL;
    int            mode = NO_ENC;
#endif

    /* exclusive access is default */
    strcpy(access, "x");

    /* process option switches */
    for (i = 1; i < argc && argv[i][0] == '-'; ++i)
    {
        switch (tolower(argv[i][1]))
        {
            case 'h':
            case '?':
                usage();
                return 1;

            case 's':
                if (tolower(argv[i][2]) == 'g')
                {
#if defined(SAFEGARDE)
                    if (i == argc - 1)
                    {
                        fprintf(stderr, "Password required\n");
                        usage();
                        return 1;
                    }

                    if ((cp = strchr(argv[++i], ',')) != NULL)
                        *cp++ = '\0';

                    if (cp)
                    {
                        if (stricmp(argv[i], "low") == 0)
                            mode = LOW_ENC;
                        else if (stricmp(argv[i], "med") == 0)
                            mode = MED_ENC;
                        else if (stricmp(argv[i], "high") == 0)
                            mode = HIGH_ENC;
                        else
                        {
                            fprintf(stderr, "Invalid encryptoin mode\n");
                            usage();
                            return 1;
                        }

                        password = cp;
                    }
                    else
                    {
                        mode = MED_ENC;
                        password = argv[i];
                    }
#else
                    vtprintf(DB_TEXT("SafeGarde is not available in this version\n"));
                    return 1;
#endif
                    break;
                }
                /* Fall through */
            case 'x':
            case 'o':   strcpy(access, &argv[i][1]);   break;
            case 't':   d_on_opt(TXTEST, task);          break;
            case 'm':
                switch (tolower(argv[i][2]))
                {
                    case 'n':
                        lm_type = "";
                        break;

                    case 't':
                        lm_type = "TCP";
                        break;

                    case 'p':
                        lm_type = "IP";
                        break;

                    default :
                        usage();
                        return 1;
                }

                break;

            default:
	        vtprintf(DB_TEXT("Invalid option: %s\n"), argv[i]);
                usage();
                return 1;
        }
    }

    if (i < argc)
        strcpy(dbname, argv[i]);

    if (d_opentask(&task) != S_OKAY)
        return 1;

    ioc_on();
    tprintf("@c");
    title(5, "'IDA'");
    title(6, "");
    d_dbver("%V", version, sizeof(version));
    title(7, version);
    title(8, "Interactive Database Access Utility");
    title(9, "");
    d_dbver("%C", v = version, sizeof(version));
    i = 10;
    while ((p = strchr(v, ',')) != NULL)
    {
        *p = '\0';
        title(i++, v);
        v = ++p;
    }
    title(i++, v);
    fflush(stdout);

    title(21, "press any key to continue");
    tgetch();
    tprintf("@c");

    d_on_opt(READNAMES, task);
    d_set_dberr((ERRORPROC)idaDBErr, task);

    if (!lm_type)
        lm_type = "TCP";

    d_lockcomm(psp_lmcFind(lm_type), task);

#if defined(SAFEGARDE)
    if (mode && (sg = sg_create(mode, password)) == NULL)
    {
        fprintf(stderr, "Failed to create SafeGarde context\n");
        d_closetask(task);
        return 1;
    }
#endif

    if (dbname[0])
    {
        for (i = menu_table[DB_MENU].first_cmd; cmd_table[i].next; ++i)
        {
            /* find X_exit in r_menu and change to Quit */
            if (strcmp(cmd_table[i].word, "X_exit") == 0)
            {
                cmd_table[i].word = "Quit";
                break;
            }
        }

        if (d_open_sg(dbname, access, sg, task) == S_OKAY)
        {
            bld_keys();
            menu(DB_MENU);
            d_close(task);
        }
        else
        {
            if (task->db_status == S_UNAVAIL)
                usererr("database currently unavailable");
        }
    }
    else
    {
        if (initFromIniFile(task) != S_OKAY)
        {
            d_closetask(task);
            return 1;
        }

        dblist();         /* build list of database names */
        menu(MAIN_MENU);
        d_close(task);
    }

#if defined(SAFEGARDE)
    sg_delete(sg);
#endif
    d_closetask(task);
    tprintf("@m2300\n\n");
    ioc_off();

    return 0;
}

void usage(void)
{
    fprintf(stderr, "usage: ida [-t] [-m{n|t|p}] [-{s|x|o}] [-sg [<mode>,]<passwd] [dbname]\n");
    fprintf(stderr, "where:  -t  turns on TXTEXT mode for debugging\n");
    fprintf(stderr, "        -m  specifies the lock manager communication type\n");
    fprintf(stderr, "             n - None (single user mode only)\n");
    fprintf(stderr, "             t - TCP/IP\n");
    fprintf(stderr, "             p - IP (Unix Domain sockets) (where available)\n");
    fprintf(stderr, "        -s  open the database in shared mode\n");
    fprintf(stderr, "        -x  open the database in exclusive mode\n");
    fprintf(stderr, "        -o  open the database in one user mode\n");
    fprintf(stderr, "        -sg specifies the encryptiong information\n");
    fprintf(stderr, "             <mode> may be 'low', 'med' (default), or 'high'\n");
}


