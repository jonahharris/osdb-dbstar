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

/* pspposix.c - Contains Posix specific functions */
#include <signal.h>
#include <sys/time.h>

#include "psp.h"
#include "pspint.h"
#if defined(HPUX) || defined(AIX) || defined(BSDI) || defined(LINUX) || defined(UNIXWARE)
#define MILLISEC   1000
#define MICROSEC   1000000
#define NANOSEC    1000000000
#endif

#if defined(BSDI)
#define ETIME ETIMEDOUT
#endif

#define PSP_SEM_ENTRY_SIZE 1024
#define PSP_SEM_TABLE_SIZE  128

typedef struct {
    pthread_mutex_t mutex;
    int             id_set;
    pthread_t       id;
} PSP_MUTEX_ENTRY;

typedef struct {
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
    short           flag;
} PSP_EVENT_ENTRY;

typedef struct {
    union {
        PSP_MUTEX_ENTRY m;
        PSP_EVENT_ENTRY e;
    } obj;
    short                  type;
    short                  avail;
    struct PSP_SEM_TABLE_ *owner;
} PSP_SEM_ENTRY;

typedef struct PSP_SEM_TABLE_ {
    PSP_SEM_ENTRY *entry;
    short          avail;
} PSP_SEM_TABLE;

static PSP_SEM_TABLE    sem_table[PSP_SEM_TABLE_SIZE];
static pthread_mutex_t  semTabMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  globalMutex = PTHREAD_MUTEX_INITIALIZER;

static void semDelete(PSP_SEM_ENTRY *);

#define SEM_END   -1
#define SEM_INUSE -2


/* ==========================================================================
   Initialize synchronization functions
*/
int psp_syncInit(
    void)
{
    memset(sem_table, 0, sizeof(sem_table));
    return PSP_OKAY;
}

/* ==========================================================================
   Shutdown synchronization functions
*/
void psp_syncShutdown(
    void)
{
    int            ii;
    int            jj;
    PSP_SEM_TABLE *p;
    PSP_SEM_ENTRY *sem;

    for (ii = 0, p = sem_table; ii < PSP_SEM_TABLE_SIZE; ii++, p++) {
        if (p->entry) {
            sem = p->entry;
            for (jj = 0; jj < PSP_SEM_ENTRY_SIZE; jj++, sem++) {
                if (sem->avail == SEM_INUSE)
                    semDelete(sem);
            }
                    
            psp_freeMemory(p->entry, 0);
        }
    }
}

/* ==========================================================================
   Cleanup synchronization functions
*/
void psp_syncTerm(
    void)
{
}

/* ==========================================================================
    Create a synchronization object
*/
PSP_SEM psp_syncCreate(
    short type)
{
    short          avail;
    int            ii;
    int            jj;
    PSP_SEM_TABLE *p;
    PSP_SEM_ENTRY *sem = NULL;

    if (!psp_inited)
        return NULL; 

    pthread_mutex_lock(&semTabMutex);

    for (ii = 0, p = sem_table; ii < PSP_SEM_TABLE_SIZE; ii++, p++) {
        if (!p->entry) {
            sem = psp_cGetMemory(sizeof(PSP_SEM_ENTRY) * PSP_SEM_ENTRY_SIZE, 0);

            for (jj = 0; jj < PSP_SEM_ENTRY_SIZE; jj++) {
                sem[jj].avail = jj + 1;
                sem[jj].owner = p;
            }

            sem[jj - 1].avail = SEM_END;
            p->entry = sem;
            p->avail = 0;
        }

        if ((avail = p->avail) != SEM_END) {
            sem = &p->entry[avail];
            p->avail = sem->avail;
            sem->avail = SEM_INUSE;
            break;
        }
    }

    pthread_mutex_unlock(&semTabMutex);

    if (!sem) {
        /* TBD: Log error about insufficient number of semaphores */
        printf("Insufficient number of semaphores\n");
        return NO_PSP_SEM;
    }

    switch (sem->type = type) {
        case PSP_MUTEX_SEM:
            pthread_mutex_init(&sem->obj.m.mutex, NULL);
            sem->obj.m.id_set = 0;
            break;

        case PSP_EVENT_SEM:
            pthread_mutex_init(&sem->obj.e.mutex, NULL);
            pthread_cond_init(&sem->obj.e.cond, NULL);
            sem->obj.e.flag = 1;
            break;

        default:
            pthread_mutex_unlock(&semTabMutex);
            /* TBD:  Log out error about invalid semaphore type */
            printf("Invalid semaphore type\n");
            return NO_PSP_SEM;
    }

    pthread_mutex_unlock(&semTabMutex);

    return sem;
}

static void semDelete(
    PSP_SEM_ENTRY *sem)
{
    switch (sem->type) {
        case PSP_MUTEX_SEM:
            pthread_mutex_destroy(&sem->obj.m.mutex);
            break;

        case PSP_EVENT_SEM:
            pthread_mutex_destroy(&sem->obj.e.mutex);
            pthread_cond_destroy(&sem->obj.e.cond);
            break;

        default:
            break;
    }
}

/* ==========================================================================
    Delete a synchronization object
*/
void psp_syncDelete(
    PSP_SEM sem)
{
    PSP_SEM_ENTRY *entry = (PSP_SEM_ENTRY *) sem;
    PSP_SEM_TABLE *owner = entry->owner;

    semDelete(entry);
    entry->type = PSP_UNDEF_SEM;

    pthread_mutex_lock(&semTabMutex);

    /* put semaphore entry on available semaphore list */
    entry->avail = owner->avail;
    owner->avail = (short) (entry - owner->entry);

    pthread_mutex_unlock(&semTabMutex);
}

/* ==========================================================================
    Obtain exclusive access to a mutex
*/
void psp_syncEnterExcl(
    PSP_SEM sem)
{
    PSP_MUTEX_ENTRY *m = &((PSP_SEM_ENTRY *) sem)->obj.m;

    if (((PSP_SEM_ENTRY *) sem)->type != PSP_MUTEX_SEM) {
        /* TBD: Log out error */
        printf("Invalid mutex\n");
    }        

    if (m->id_set & pthread_equal(m->id, pthread_self()))
        return;

    pthread_mutex_lock(&m->mutex);
    m->id_set = 1;
    m->id     = pthread_self();
}

/* ==========================================================================
    Release exclusive access to a mutex
*/
void psp_syncExitExcl(
    PSP_SEM sem)
{
    PSP_MUTEX_ENTRY *m = &((PSP_SEM_ENTRY *) sem)->obj.m;

    if (((PSP_SEM_ENTRY *) sem)->type != PSP_MUTEX_SEM) {
        /* TBD: Log out error */
        printf("Invalid mutex\n");
    }        

    m->id_set = 0;
    m->id = 0;
    pthread_mutex_unlock(&m->mutex);
}

/* ==========================================================================
    Wait for an event
*/
short psp_syncWait(
    PSP_SEM sem,
    long    timeout)
{
    PSP_EVENT_ENTRY *e = &((PSP_SEM_ENTRY *) sem)->obj.e;
    short           *flag = &e->flag;
    int              ret = 0;
    struct timespec  tm;
    struct timeval   tmval;

    if (((PSP_SEM_ENTRY *) sem)->type != PSP_EVENT_SEM) {
        /* TBD: Log out error */
        printf("Invalid mutex\n");
    }        

    pthread_mutex_lock(&e->mutex);

    while (!*flag && ret != ETIME && ret != ETIMEDOUT) {
        if (timeout == PSP_INDEFINITE_WAIT)
            pthread_cond_wait(&e->cond, &e->mutex);
        else {
            gettimeofday(&tmval, NULL);
            tm.tv_sec = tmval.tv_sec + (timeout / MILLISEC);
            tm.tv_nsec = (timeout % MILLISEC) * (NANOSEC / MILLISEC) +
                    tmval.tv_usec * (NANOSEC / MICROSEC);

            if (tm.tv_nsec > NANOSEC) {
                tm.tv_sec += tm.tv_nsec / NANOSEC;
                tm.tv_nsec %= NANOSEC;
            }

            ret = pthread_cond_timedwait(&e->cond, &e->mutex, &tm);
#if defined(HPUX)
            if ((ret == -1) && (errno == EAGAIN))
                ret = ETIME;
#endif
        }
    }

    pthread_mutex_unlock(&e->mutex);

    return ((ret == ETIME || ret == ETIMEDOUT) ? PSP_TIMEOUT : PSP_OKAY);
}

/* ==========================================================================
    Set an event to the unsignalled state
*/
void psp_syncStart(
    PSP_SEM sem)
{
    if (((PSP_SEM_ENTRY *) sem)->type != PSP_EVENT_SEM) {
        /* TBD: Log out error */
        printf("Invalid mutex\n");
    }        

    ((PSP_SEM_ENTRY *) sem)->obj.e.flag = 0;
}

/* ==========================================================================
    Set an event to the signalled state
*/
void psp_syncResume(
    PSP_SEM sem)
{
    PSP_EVENT_ENTRY *e = &((PSP_SEM_ENTRY *) sem)->obj.e;

    if (((PSP_SEM_ENTRY *) sem)->type != PSP_EVENT_SEM) {
        /* TBD: Log out error */
        printf("Invalid mutex\n");
    }        

    pthread_mutex_lock(&e->mutex);
    e->flag = 1;
    pthread_cond_broadcast(&e->cond);
    pthread_mutex_unlock(&e->mutex);
}

/* ==========================================================================
    Get exclusive access to the global mutex
*/
void psp_enterCritSec(
    void)
{
    pthread_mutex_lock(&globalMutex);
}

/* ==========================================================================
    Release exclusive access to the global mutex
*/
void psp_exitCritSec(
    void)
{
    pthread_mutex_unlock(&globalMutex);
}

/* ==========================================================================
    Create a thread
*/
short psp_threadBegin(
    void       (*fcn)(void *),
    unsigned int stacksize,
    void        *arg)
{
    pthread_t      tid;
    pthread_attr_t attr;
    int            ret;
#if defined(LINUX)
    sigset_t       orgset;
    sigset_t       newset;
#endif

    if (!psp_inited)
        return PSP_FAILED;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#if defined(LINUX)
    __sigemptyset(&newset);
    __sigaddset(&newset, SIGINT);
    __sigaddset(&newset, SIGTERM);
    __sigaddset(&newset, SIGCHLD);
    pthread_sigmask(SIG_BLOCK, &newset, &orgset);
#else
#if defined(AIX)
    if (stacksize && stacksize < PTHREAD_STACK_MIN)
        stacksize = PTHREAD_STACK_MIN;
#endif

    pthread_attr_setstacksize(&attr, stacksize);
#endif

    ret = pthread_create(&tid, &attr, (void *(*)(void *))fcn, arg);

#if defined(LINUX)
    pthread_sigmask(SIG_SETMASK, &orgset, NULL);
#endif
    pthread_attr_destroy(&attr);

    if (ret != 0)
        return PSP_FAILED;

    return (ret == 0) ? PSP_OKAY : PSP_FAILED;
}

/* ==========================================================================
    End a thread
*/
void psp_threadEnd(
    void)
{
    pthread_exit(NULL);
}

/* ==========================================================================
    Return the id of the current thread
*/
psp_thread_t psp_threadId(
    void)
{
    return pthread_self();
}
