/***************************************************************************
 *                                                                         *
 * db.linux                                                                *
 * open source platform support package for Linux (tm)                     *
 *                                                                         *
 * Copyright (c) 2000 Centura Software Corporation. All rights reseved.    *
 *                                                                         *
 * Use of this software, whether in source code format, or in executable,  *
 * binary object code form, is governed by the CENTURA OPEN SOURCE LICENSE *
 * which is fully described in the LICENSE.TXT file, included within this  *
 * distribution of source code files.                                      *
 *                                                                         *
 **************************************************************************/

/* pspunix.c - Contains UNIX specific functions */
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <mntent.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "psp.h"
#include "pspint.h"

typedef struct DEVICE_LIST_ {
   struct DEVICE_LIST_ *next;
   dev_t                device;
   char                 *name;
} DEVICE_LIST;

DEVICE_LIST *deviceHead = NULL;
PSP_SEM      deviceSem = NO_PSP_SEM;

/* ==========================================================================
    OS Specific initialization
*/
int psp_osInit(
    void)
{
	deviceHead = NULL;
    if ((deviceSem = psp_syncCreate(PSP_MUTEX_SEM)) == NO_PSP_SEM)
        return PSP_FAILED;

    signal(SIGPIPE, SIG_IGN);

    return PSP_OKAY;
}

/* ==========================================================================
   OS Specific termination
*/
void psp_osTerm(
    void)
{
    psp_syncDelete(deviceSem);
}

/* ==========================================================================
    Open a file
*/
short psp_flOpen(
    const char    *name,
    unsigned int   flags,
    unsigned short xflags,
    PSP_FDESC     *desc)
{
    if ((*desc = open(name, flags, 0666)) == -1) {
        *desc = 0;
        return errno;
    }

    return 0;
}

/* ==========================================================================
    Close a file
*/
void psp_flClose(
    PSP_FDESC desc)
{
    close(desc);
}

/* ==========================================================================
    Seek to a specified address in a file
*/
void psp_flSeek(
    PSP_FDESC desc,
    size_t    addr,
    int       origin)
{
    lseek(desc, addr, origin);
}

/* ==========================================================================
    Write to a file
*/
size_t psp_flWrite(
    PSP_FDESC   desc,
    const void *buf,
    size_t      size)
{
    size_t  bytes;

    errno = 0;

    bytes = write(desc, buf, size);
    return (bytes <= 0 && errno) ? -1 : bytes;
}

/* ==========================================================================
    Read from a file
*/
size_t psp_flRead(
    PSP_FDESC desc,
    void     *buf,
    size_t    size)
{
    size_t bytes;

    errno = 0;

    bytes = read(desc, buf, size);
    return (bytes <= 0 && errno) ? -1 : bytes;
}

/* ==========================================================================
    Write to a specified address in a file
*/
size_t psp_flSeekWrite(
    PSP_FDESC   desc,
    size_t      addr,
    const void *buf,
    size_t      size)
{
    size_t bytes;

    if (addr != (size_t) -1) {
        if (lseek(desc, addr, SEEK_SET) == -1)
             return -1;
    }

    errno = 0;

    bytes = write(desc, buf, size);
    return (bytes <= 0 && errno) ? -1 : bytes;
}

/* ===========================================================================
    Read from a specified address in a file
*/
size_t psp_flSeekRead(
    PSP_FDESC desc,
    size_t    addr,
    void     *buf,
    size_t    size)
{
    size_t bytes;

    if (addr != (size_t) -1) {
        if (lseek(desc, addr, SEEK_SET) == -1)
            return -1;
    }

    errno = 0;
    bytes = read(desc, buf, size);
    return (bytes != size && errno) ? -1 : bytes;
}

/* ===========================================================================
    Synchronize writes to a file
*/
void psp_flSync(
    PSP_FDESC desc)
{
    fsync(desc);
}

/* ===========================================================================
    Find the current size of a file
*/
size_t psp_flSize(
    PSP_FDESC desc)
{
    size_t cur  = lseek(desc, 0, SEEK_CUR);
    size_t size = lseek(desc, 0, SEEK_END);

    lseek(desc, cur, SEEK_SET);
    return size;
}

/* ===========================================================================
    Set the current size of a file
*/
int psp_flSetSize(
    PSP_FDESC desc,
    size_t    size)
{
    size_t ii;
    char   buf[10240];

    lseek(desc, 0, SEEK_END);
    memset(buf, 0, sizeof(buf));
    for (ii = 0; ii < size / sizeof(buf); ii++) {
        if (write(desc, buf, sizeof(buf)) < (int) sizeof(buf))
            return -1;
    }

    if (size % sizeof(buf)) {
        if (write(desc, buf, size % sizeof(buf)) < (int) (size % sizeof(buf)))
            return -1;
    }

    return 0;
}

/* ===========================================================================
    Lock a file
*/
short psp_flLock(
    PSP_FDESC desc)
{
    if ((flock(desc, LOCK_EX|LOCK_NB) != 0) && (errno == EWOULDBLOCK ||
            errno == EACCES))
        return PSP_FAILED;

    return PSP_OKAY;
}

/* ===========================================================================
    Lock a file
*/
void psp_flUnlock(
    PSP_FDESC desc)
{
    flock(desc, LOCK_UN|LOCK_NB);
}

/* ===========================================================================
    Get last access time for a file
*/
long psp_flLastAccess(
    PSP_FDESC desc)
{
    struct stat stbuf;

    if (fstat(desc, &stbuf) < 0)
        return -1;

    return stbuf.st_atime;
}

/* ===========================================================================
    Get modification time for a file
*/
long psp_flModTime(
    PSP_FDESC desc)
{
    struct stat stbuf;

    if (fstat(desc, &stbuf) < 0)
        return -1;

    return stbuf.st_mtime;
}

/* ===========================================================================
   See if the specified string matches the given file name.  On case sensitive
   file systems (UNIX) this should be a case sensitive.  Other wise it should
   be case insensitive
*/
int psp_fileNameCmp(
    const char *str,
    const char *file)
{
    return strcmp(str, file);
}

/* ===========================================================================
   See if the specified string matches the given file name.  On case sensitive
   file systems (UNIX) this should be a case sensitive.  Other wise it should
   be case insensitive
*/
int psp_fileNamenCmp(
    const char *str,
    const char *file,
    size_t      len)
{
    return strncmp(str, file, len);
}

/* ===========================================================================
   Rename a file
*/
int psp_fileRename(
    const char *src,
    const char *dest)
{
    return rename(src, dest);
}

/* ===========================================================================
    Delete a file
*/
int psp_fileRemove(
    const char *name)
{
    return remove(name);
}

#if 0
/* ==========================================================================
   Return the size of a file
*/
size_t psp_fileSize(
    const char *name)
{
    struct stat stbuf;

    if (stat(name, &stbuf) < 0)
        return 0;

    return stbuf.st_size;
}
#endif

/* ==========================================================================
   Validate the existence of a file
*/
int psp_fileValidate(
    const char *name)
{
    return !access(name, 0);
}

#define INI_BUF_SIZE 1024
typedef struct {
    PSP_FH fh;
    char  *buf;
} INI_ENTRY;

/* convert a string to an integer, handling octal, decimal, and hex */
static int strtoi(
    const char *s)
{
    int  d;
    int  v = 0;
    int  neg = 0;
    int  base = 10;

    while (isspace(*s))
        ++s;

    if (*s == '-') {
        neg = 1;
        ++s;
    }

    if (*s == '0') {
        if (neg)
            return v;  /* can't negate octal or hex numbers */

        base = 8;
        if (*++s == 'x' || *s == 'X') {
            base = 16;
            ++s;
        }
    }

    for ( ; *s; s++) {
        if (v > INT_MAX / base)
            break;

        if ((d = *s - '0') < 0 || d > 9) {
            if ((d = toupper(*s) - 'A') < 0 || d > 5)
                break;

            if ((d += 10) >= base)
                break;
        }

        v = v * base + d;
    }

    if (neg)
        return -v;

    return v;
}


/* ==========================================================================
   Open an INI file
*/
PSP_INI psp_iniOpen(
    const DB_TCHAR *name)
{
    INI_ENTRY *ini;
    
    if ((ini = psp_getMemory(sizeof(INI_ENTRY), 0)) == NULL)
        return NULL;

    ini->fh = psp_fileOpen(name, O_RDONLY, PSP_FLAG_DENYNO | PSP_FLAG_STREAM);
    if (ini->fh == NULL) {
        psp_freeMemory(ini, 0);
        return NULL;
    }

    if ((ini->buf = psp_getMemory(INI_BUF_SIZE, 0)) == NULL) {
        psp_fileClose(ini->fh);
        psp_freeMemory(ini, 0);
    }

    return (PSP_INI) ini;
}

/* ==========================================================================
   Close an INI file
*/
void psp_iniClose(
    PSP_INI ini)
{
    if (ini) {
        psp_fileClose(((INI_ENTRY *) ini)->fh);
        psp_freeMemory(((INI_ENTRY *) ini)->buf, 0);
        psp_freeMemory(ini, 0);
    }
}

#define INI_BUF_SIZE 1024
static char *readINI(
    INI_ENTRY  *ini,
    const char *section,
    const char *key)
{
    size_t len;
    char  *cp;
    char  *np;

    psp_fileSeek(ini->fh, 0);
    len = strlen(section);
    while ((cp = psp_fileGets(ini->fh, ini->buf, INI_BUF_SIZE)) != NULL) {
        if (cp[0] == '[' && strnicmp(cp + 1, section, len) == 0 &&
                cp[len + 1] == ']')
            break;
    }

    if (!cp)
        return NULL;

    /* Find the key */
    len = strlen(key);
    while ((cp = psp_fileGets(ini->fh, ini->buf, INI_BUF_SIZE)) != NULL) {
        if (cp[0] == '[')
            return NULL; /* start of new section, key not found */

        if (strnicmp(cp, key, len) == 0) {
            if ((cp = strchr(cp + len, '=')) == NULL)
                return NULL;  /* No value specified */

            while (isspace(*(++cp)))
                ;

            /* trim any trailing newlien */
            if ((np = strchr(cp, '\n')) != NULL)
                *np = '\0';

            return cp;
        }
    }

    return NULL;
}

/* ==========================================================================
   Read a short value from an INI file
*/
short psp_iniShort(
    PSP_INI         ini,
    const DB_TCHAR *section,
    const DB_TCHAR *key,
    short           def)
{
    char *str;

    if (ini && (str = readINI((INI_ENTRY *) ini, section, key)) != NULL)
        return (short) strtoi(str);

    return def;
}

/* ==========================================================================
   Read a short value from an INI file
*/
size_t psp_iniString(
    PSP_INI         ini,
    const DB_TCHAR *section,
    const DB_TCHAR *key,
    const DB_TCHAR *def,
    DB_TCHAR       *buf,
    size_t          len)
{
    char *str;

    if (ini && (str = readINI((INI_ENTRY *) ini, section, key)) != NULL)
        strncpy(buf, str, len);
    else
        strncpy(buf, def, len);

    buf[len - 1] ='\0';

    return strlen(buf);
}

/* ==========================================================================
    Suspend the current thread for the specified # of milliseconds
*/
void psp_sleep(
    unsigned long millisecs)
{
    struct timespec tm1;
    struct timespec tm2;

    tm1.tv_sec = millisecs / 1000;
    tm1.tv_nsec = (millisecs % 1000) * 1000000;
    while (nanosleep(&tm1, &tm2) != 0)
         memcpy(&tm1, &tm2, sizeof(struct timespec));
}

/* ===========================================================================
    Return the process id
*/
unsigned int psp_get_pid(
    void)
{
    return (unsigned int) getpid();
}

static int pfd[2];
/* ===========================================================================
    Handle a SIGCHLD message
*/
void sigchld_handler(
    int sig)
{
    int stat;

    wait(&stat);

    if (WIFEXITED(stat)) {
	fprintf(stderr, "Lock manager startup aborted with a status of %d.",
		WEXITSTATUS(stat));
        exit(WEXITSTATUS(stat));
    }

    if (WIFSIGNALED(stat)) {
	fprintf(stderr, "Lock manager terminated by internal error or"
		"external signal (%d).\n", WTERMSIG(stat));
        exit(-WTERMSIG(stat));
    }

    fprintf(stderr, "Unexpected status signal from child process.\n");
    exit(99);
}

/* ===========================================================================
    Fork the process to begin daemon mode
*/
void psp_daemonInit(
    DB_TCHAR *name)
{
    pipe(pfd);

    if (fork()) {
	/* parent waits for child to start up and then exits */
	signal(SIGCHLD, sigchld_handler);
	close(pfd[1]);

	read(pfd[0], &pfd[1], 1);

	/* pipe has now closed. */
	printf("Lock manager '%s' started successfully.\n", name);
	close(pfd[0]);
	exit(0);
    }

    /* child continues here */
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);

    setsid();
    write(pfd[1], &pfd[0], sizeof(int));
    close(pfd[0]);
    close(pfd[1]);
}

/* ===========================================================================
    Terminate running as a daemone.  Nothing to do on Unix
*/
void psp_daemonTerm(
    void)
{
}

/* ===========================================================================
   Return a pointer to the first character of the file name portion of the
   supplied path
*/
char *psp_pathGetFile(
    const char *path)
{
    char *cp;

    if ((cp = strrchr(path, DIRCHAR)) == NULL)
        return (char *) path;

    return cp + 1;
}

/* ===========================================================================
   Determine if the path is only a subdirectory (does not contain a file name)
*/
int psp_pathIsDir(
    const char *path)
{
    size_t len;

    if (!path || !*path)
        return -1;

    len = strlen(path) - 1;
    if (path[len] == DIRCHAR)
        return 1;

    return 0;
}

/* ===========================================================================
   Split a path up into the subdirectory and file name components
*/
void psp_pathSplit(
    const char *path,
    char      **dir,
    char      **file)
{
    char *cp;

    cp = strrchr(path, DIRCHAR);
    if (file) {
        if (cp && *(cp + 1))
           *file = psp_strdup(cp + 1, 0);
        else
           *file = psp_strdup(path, 0);
    }

    if (dir) {
        if (cp) {
            *(cp + 1) = '\0';
            *dir = psp_strdup(path, 0);
        }
        else
            *dir = NULL;
    }
}

/* ===========================================================================
   Return the path without any drive specifier
*/
char *psp_pathStripDrive(
    const char *path)
{
    return (char *) path;
}

static char *dbtmp = "/tmp/";

/* ===========================================================================
   Return the default temporary path
*/
char *psp_pathDefTmp(
    void)
{
    return dbtmp;
}

typedef struct {
    DIR  *entry;
    char *pattern;
} DIR_ENTRY;

/* ===========================================================================
   Open a directory for searching
*/
PSP_DIR psp_pathOpen(
    const char *path,
    const char *pattern)
{
    DIR_ENTRY *dir;

    if ((dir = psp_getMemory(sizeof(DIR_ENTRY), 0)) == NULL)
        return NULL;

    if ((dir->entry = opendir(path)) == NULL) {
        psp_freeMemory(dir, 0);
        return NULL;
    }

    if ((dir->pattern = psp_strdup(pattern, 0)) == NULL) {
        psp_freeMemory(dir->entry, 0);
        psp_freeMemory(dir, 0);
    }

    return dir;
}

static int match(
    const char *str,
    const char *pattern)
{
    char p;
    char s;

    while ((p = *pattern++) != '\0') {
        if (p == '*')
        {
            if ((p = *pattern++) == '\0')
                return 1;

            while (*str) {
                s = *str++;
                if (toupper(s) == toupper(p) && match(str, pattern))
                    return 1;
            }

            return 0;
        }

        if ((s = *str++) == '\0')
            return 0;

        if (p != '?' && toupper(p) != toupper(s))
            return 0;
    }

    if (*str == '\0')
        return 1;

    return 0;
}

/* ===========================================================================
   Find next occurrence of requested name in path
*/
char *psp_pathNext(
    PSP_DIR dir)
{
    size_t         len;
    struct dirent *ent;
    DIR_ENTRY     *d;

    if (!dir)
        return NULL;

    d = (DIR_ENTRY *) dir;
    for ( ; ; ) {
        if ((ent = readdir(d->entry)) == NULL)
            return NULL;

        if (match(ent->d_name, d->pattern))
            break;
    }

    return psp_strdup(ent->d_name, 0);
}

/* ===========================================================================
   Close directory searching
*/
void psp_pathClose(
    PSP_DIR dir)
{
    DIR_ENTRY *d;

    if (dir) {
        d = (DIR_ENTRY *) dir;

        closedir(d->entry);

        psp_freeMemory(d->pattern, 0);
        psp_freeMemory(d, 0);
    }
}

/* ===========================================================================
   Return a random (hopefully) seed
*/
long psp_seed(
    void)
{
    return clock();
}

/* ===========================================================================
   Return the current time in seconds
*/
long psp_timeSecs(
    void)
{
    return time(NULL);
}

/* ===========================================================================
   Return the current time in milliseconds
*/
long psp_timeMilliSecs(
    void)
{
   struct timeval tv;

   gettimeofday(&tv, NULL);
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* ===========================================================================
   Return the error number from the last call
*/
int psp_errno(
    void)
{
    return errno;
}

/* ===========================================================================
   Return the contents of an environment variable
*/
char *psp_getenv(
    const char *var)
{
    return getenv(var);
}

/* ===========================================================================
    Generate a unique name for a device
*/
static char *get_device(
    dev_t       device,
    const char *src)
{
    char          *seppos;
    char          *name;
    char          *host;
    struct mntent *fstab;
    struct stat    stbuf;
    FILE          *mnt;

    if ((mnt = setmntent(MNTTAB, "r")) == NULL)
        return NULL;

    /* search in /etc/[fstab|checklist] */
    while ((fstab = getmntent(mnt)) != NULL) {
        if (stat(fstab->mnt_dir, &stbuf) != -1 && stbuf.st_dev == device)
            break;
    }

    endmntent(mnt);

    if (fstab == NULL)
        return NULL;

    name = psp_getMemory(FILEIDLEN, 0);
    host = psp_getMemory(FILEIDLEN, 0);

    if (strncmp(fstab->mnt_fsname, "/dev", 4) == 0) { /* device is local */
        if (gethostname(host, FILEIDLEN) < 0) {
            psp_freeMemory(host, 0);
            psp_freeMemory(name, 0);
            return NULL;
        }

        if (fstab->mnt_dir[0] == DIRCHAR && fstab->mnt_dir[1] == '\0') {
            /* we need /path_prefix, as that is what will be supplied in
                mnttab on a remote machine mounting this filesystem. */
            char  prefix[FILENMLEN];
            char *cp;

            if (src[0] == DIRCHAR)
                strcpy(prefix, src);
            else {
                getcwd(prefix, FILENMLEN - 2);
                strcat(prefix, "/");
                if (src[0] == '.') {
                    if (src[1] == '.') {
                        if ((cp = strrchr(prefix, DIRCHAR)) != NULL)
                            *cp = '\0';
                    }

                    strcat(prefix, &src[2]);
                }
                else
                    strcat(prefix, src);
            }

            if ((cp = strchr(&prefix[1], DIRCHAR)) != NULL)
                *cp = '\0';

            strncpy(name, prefix, FILEIDLEN);
        }
        else
            strncpy(name, fstab->mnt_dir, FILEIDLEN);
    }
    else if ((seppos = strchr(fstab->mnt_fsname, ':')) != NULL) { /* host:fsname */
        *seppos = '\0';
        strncpy(host, fstab->mnt_fsname, FILEIDLEN);
        strncpy(name, seppos + 1, FILEIDLEN);
    }
    else if ((seppos = strchr(fstab->mnt_fsname, '@')) != NULL) { /* fsname@host */
        *seppos = '\0';
        strncpy(host, seppos + 1, FILEIDLEN);
        strncpy(name, fstab->mnt_fsname, FILEIDLEN);
    }
    else { /* can't understand fsname in fstab */
        psp_freeMemory(name, 0);
        psp_freeMemory(host, 0);
        return NULL;
    }

    name[FILEIDLEN - 1] = '\0';       /* convert device name to fsname@host */
    strcat(name, "@");
    host[FILEIDLEN - strlen(name) - 1] = '\0';
    strcat(name, host);

    psp_freeMemory(host, 0);

    return name;
}

/* ===========================================================================
    Find the device in list of devices already discovered or search the mount
    table
*/
static char *find_device(
    dev_t       device,
    const char *src)
{
    char        *name;
    DEVICE_LIST *p;

    if (!psp_inited)
        return NULL;

    psp_syncEnterExcl(deviceSem);

    /* search linked list of known devices */
    for (p = deviceHead; p; p = p->next) {
        if (device == p->device)
            break;
    }

    if (p)
        return p->name;

    /* generate name for this device */
    if ((name = get_device(device, src)) == NULL)
        return NULL;

    /* add this device to the linked list */
    p = psp_getMemory(sizeof(DEVICE_LIST), 0);
    p->device = device;
    p->name = name;

    p->next = deviceHead;
    deviceHead = p;

    psp_syncExitExcl(deviceSem);

    return p->name;
}

/* ===========================================================================
   Return a string that completely and uniquely defines a file (the "truename"
   of the file) so that the lock manager can correctly handle lock contention.
   On UNIX this means the device name on a host with the inode for that file
   in the following format:  <devicepath>@<hostname>:<inode>
*/
char *psp_truename(
    const char *src,
    PSP_MEMTAG   tag)
{
    char        truename[FILEIDLEN];
    struct stat stbuf;
    char       *name;
    dev_t       device;

    if (stat(src, &stbuf) == -1)
        return NULL;

    if ((name = find_device(stbuf.st_dev, src)) == NULL)
        return NULL;

    sprintf(truename, "%s:%ld", name, stbuf.st_ino);

    return psp_strdup(truename, tag);
}

/* ===========================================================================
   Return the default lock manager name
*/
char *psp_defLockmgr(
    void)
{
    return "lockmgr";
}

/* ===========================================================================
   Return the validity of the give lock manager name
*/
int psp_validLockmgr(
    const char *name)
{
    char c;

    if (!name)
        return PSP_FAILED;

    while ((c = *name++) != '\0') {
        if (!(isalnum(c) || c == '_' || c == ':' || c == '.'))
            return 0;
    }

    return 1;
}

static INTERRUPT_FCN *interrupt_fcn = NULL;

static void handler(
    int sig)
{
    signal(SIGINT, SIG_IGN);

    if (interrupt_fcn)
        (*interrupt_fcn)();

    signal(SIGINT, handler);
}

/* ===========================================================================
   Setup a function for handling interrupts
*/
int psp_handleInterrupt(
    INTERRUPT_FCN *fcn)
{
    if (!fcn) {
        signal(SIGINT, SIG_IGN);
        return PSP_OKAY;
    }

    interrupt_fcn = fcn;
    signal(SIGINT, handler);
    return PSP_OKAY;
}

/* ===========================================================================
   Get the current locale setting
*/
void psp_localeGet(
    DB_TCHAR *str,
    size_t    len)
{
    /* Not currently supported on UNIX */
}

/* ===========================================================================
   Set the current locale setting
*/
void psp_localeSet(
    const DB_TCHAR *str)
{
    /* Not currently supported on UNIX */
}

/* ===========================================================================
   Prints an error to the screen and waits for input
*/
void psp_errPrint(
    const char *msg)
{
    char ch;

    printf("%s\n", msg);
    printf("press <return> to continue ");
    do
        ch = (char) getchar();
    while (ch != '\n' && ch != EOF);
}

