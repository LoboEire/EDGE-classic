#pragma once

#include <stdint.h>
#include <stdlib.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#  define EPI_THREAD_WIN32
#elif defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
#  define EPI_THREAD_NONE
#else
#  define EPI_THREAD_POSIX
#endif

#if defined(EPI_THREAD_WIN32)
#include <windows.h>
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0600
#define EPI_THREAD_WIN32_CONDVAR
#endif
#elif defined(EPI_THREAD_POSIX)
#include <pthread.h>
#include <time.h>
#endif

namespace epi
{

typedef int (*ThreadFunc)(void *data);

#if defined(EPI_THREAD_WIN32)

typedef HANDLE           Thread;
typedef CRITICAL_SECTION Mutex;
typedef volatile LONG    AtomicInt;

#if defined(EPI_THREAD_WIN32_CONDVAR)
typedef CONDITION_VARIABLE Cond;
#else
struct Cond
{
    LONG   waiters_count;
    Mutex  waiters_count_lock;
    HANDLE sema;
    HANDLE waiters_done;
    int    was_broadcast;
};
#endif

struct ThreadArg
{
    ThreadFunc fn;
    void      *data;
};

static DWORD WINAPI ThreadProc(LPVOID arg)
{
    ThreadArg *a = (ThreadArg *)arg;
    int        r = a->fn(a->data);
    free(a);
    return (DWORD)r;
}

static inline Thread ThreadCreate(ThreadFunc fn, void *data)
{
    ThreadArg *a = (ThreadArg *)malloc(sizeof(ThreadArg));
    a->fn        = fn;
    a->data      = data;
    return CreateThread(NULL, 0, ThreadProc, a, 0, NULL);
}

static inline void ThreadJoin(Thread t)
{
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
}

static inline void MutexInit(Mutex *m)    { InitializeCriticalSection(m); }
static inline void MutexDestroy(Mutex *m) { DeleteCriticalSection(m); }
static inline void MutexLock(Mutex *m)    { EnterCriticalSection(m); }
static inline void MutexUnlock(Mutex *m)  { LeaveCriticalSection(m); }

#if defined(EPI_THREAD_WIN32_CONDVAR)

static inline void CondInit(Cond *c)    { InitializeConditionVariable(c); }
static inline void CondDestroy(Cond *c) { (void)c; }

static inline void CondWait(Cond *c, Mutex *m)
{
    SleepConditionVariableCS(c, m, INFINITE);
}

static inline int32_t CondWaitTimeout(Cond *c, Mutex *m, int32_t timeout_ms)
{
    return SleepConditionVariableCS(c, m, (DWORD)timeout_ms) ? 1 : 0;
}

static inline void CondSignal(Cond *c)    { WakeConditionVariable(c); }
static inline void CondBroadcast(Cond *c) { WakeAllConditionVariable(c); }

#else

static inline void CondInit(Cond *c)
{
    c->waiters_count = 0;
    c->was_broadcast = 0;
    c->sema          = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
    c->waiters_done  = CreateEvent(NULL, FALSE, FALSE, NULL);
    MutexInit(&c->waiters_count_lock);
}

static inline void CondDestroy(Cond *c)
{
    CloseHandle(c->sema);
    CloseHandle(c->waiters_done);
    MutexDestroy(&c->waiters_count_lock);
}

static inline int32_t CondWaitDeadline(Cond *c, Mutex *m, DWORD timeout_ms)
{
    MutexLock(&c->waiters_count_lock);
    c->waiters_count++;
    MutexUnlock(&c->waiters_count_lock);

    MutexUnlock(m);
    DWORD result = WaitForSingleObject(c->sema, timeout_ms);

    MutexLock(&c->waiters_count_lock);
    c->waiters_count--;
    int32_t last_waiter = c->was_broadcast && c->waiters_count == 0;
    MutexUnlock(&c->waiters_count_lock);

    if (last_waiter)
        SetEvent(c->waiters_done);

    MutexLock(m);

    return result == WAIT_OBJECT_0 ? 1 : 0;
}

static inline void CondWait(Cond *c, Mutex *m) { CondWaitDeadline(c, m, INFINITE); }

static inline int32_t CondWaitTimeout(Cond *c, Mutex *m, int32_t timeout_ms)
{
    return CondWaitDeadline(c, m, (DWORD)timeout_ms);
}

static inline void CondSignal(Cond *c)
{
    MutexLock(&c->waiters_count_lock);
    int32_t have_waiters = c->waiters_count > 0;
    MutexUnlock(&c->waiters_count_lock);

    if (have_waiters)
        ReleaseSemaphore(c->sema, 1, NULL);
}

static inline void CondBroadcast(Cond *c)
{
    MutexLock(&c->waiters_count_lock);
    int32_t have_waiters = 0;
    if (c->waiters_count > 0)
    {
        c->was_broadcast = 1;
        have_waiters     = 1;
    }

    if (have_waiters)
    {
        ReleaseSemaphore(c->sema, c->waiters_count, NULL);
        MutexUnlock(&c->waiters_count_lock);
        WaitForSingleObject(c->waiters_done, INFINITE);
        c->was_broadcast = 0;
    }
    else
    {
        MutexUnlock(&c->waiters_count_lock);
    }
}

#endif

static inline int32_t AtomicLoad(AtomicInt *a)
{
    return (int32_t)InterlockedExchangeAdd(a, 0L);
}

static inline void AtomicStore(AtomicInt *a, int32_t v)
{
    InterlockedExchange(a, (LONG)v);
}

static inline int32_t AtomicAdd(AtomicInt *a, int32_t d)
{
    return (int32_t)InterlockedExchangeAdd(a, (LONG)d);
}

static inline int32_t AtomicCAS(AtomicInt *a, int32_t expected, int32_t desired)
{
    return InterlockedCompareExchange(a, (LONG)desired, (LONG)expected) == (LONG)expected ? 1 : 0;
}

#elif defined(EPI_THREAD_NONE)

typedef int     Thread;
typedef int     Mutex;
typedef int     Cond;
typedef int32_t AtomicInt;

static inline Thread ThreadCreate(ThreadFunc fn, void *data)
{
    fn(data);
    return 0;
}

static inline void ThreadJoin(Thread t) { (void)t; }

static inline void MutexInit(Mutex *m) { (void)m; }
static inline void MutexDestroy(Mutex *m) { (void)m; }
static inline void MutexLock(Mutex *m) { (void)m; }
static inline void MutexUnlock(Mutex *m) { (void)m; }

static inline void CondInit(Cond *c) { (void)c; }
static inline void CondDestroy(Cond *c) { (void)c; }

static inline void CondWait(Cond *c, Mutex *m) { (void)c; (void)m; }

static inline int32_t CondWaitTimeout(Cond *c, Mutex *m, int32_t timeout_ms)
{
    (void)c;
    (void)m;
    (void)timeout_ms;
    return 1;
}

static inline void CondSignal(Cond *c) { (void)c; }
static inline void CondBroadcast(Cond *c) { (void)c; }

static inline int32_t AtomicLoad(AtomicInt *a) { return *a; }

static inline void AtomicStore(AtomicInt *a, int32_t v) { *a = v; }

static inline int32_t AtomicAdd(AtomicInt *a, int32_t d)
{
    int32_t old = *a;
    *a += d;
    return old;
}

static inline int32_t AtomicCAS(AtomicInt *a, int32_t expected, int32_t desired)
{
    if (*a == expected)
    {
        *a = desired;
        return 1;
    }
    return 0;
}

#else

typedef pthread_t       Thread;
typedef pthread_mutex_t Mutex;
typedef pthread_cond_t  Cond;
typedef int             AtomicInt;

struct ThreadArg
{
    ThreadFunc fn;
    void      *data;
};

static void *ThreadProc(void *arg)
{
    ThreadArg *a = (ThreadArg *)arg;
    a->fn(a->data);
    free(a);
    return NULL;
}

static inline Thread ThreadCreate(ThreadFunc fn, void *data)
{
    ThreadArg *a = (ThreadArg *)malloc(sizeof(ThreadArg));
    a->fn        = fn;
    a->data      = data;
    pthread_t t;
    pthread_create(&t, NULL, ThreadProc, a);
    return t;
}

static inline void ThreadJoin(Thread t) { pthread_join(t, NULL); }

static inline void MutexInit(Mutex *m)    { pthread_mutex_init(m, NULL); }
static inline void MutexDestroy(Mutex *m) { pthread_mutex_destroy(m); }
static inline void MutexLock(Mutex *m)    { pthread_mutex_lock(m); }
static inline void MutexUnlock(Mutex *m)  { pthread_mutex_unlock(m); }

static inline void CondInit(Cond *c)    { pthread_cond_init(c, NULL); }
static inline void CondDestroy(Cond *c) { pthread_cond_destroy(c); }

static inline void CondWait(Cond *c, Mutex *m) { pthread_cond_wait(c, m); }

static inline int32_t CondWaitTimeout(Cond *c, Mutex *m, int32_t timeout_ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec  += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L)
    {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    }
    return pthread_cond_timedwait(c, m, &ts) == 0 ? 1 : 0;
}

static inline void CondSignal(Cond *c)    { pthread_cond_signal(c); }
static inline void CondBroadcast(Cond *c) { pthread_cond_broadcast(c); }

static inline int32_t AtomicLoad(AtomicInt *a)
{
    return __atomic_load_n(a, __ATOMIC_ACQUIRE);
}

static inline void AtomicStore(AtomicInt *a, int32_t v)
{
    __atomic_store_n(a, v, __ATOMIC_RELEASE);
}

static inline int32_t AtomicAdd(AtomicInt *a, int32_t d)
{
    return __atomic_fetch_add(a, d, __ATOMIC_RELAXED);
}

static inline int32_t AtomicCAS(AtomicInt *a, int32_t expected, int32_t desired)
{
    return __atomic_compare_exchange_n(a, &expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
}

#endif

} // namespace epi
