#pragma once

#include <stdint.h>
#include <stdlib.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#  define EPI_THREAD_WIN32
#elif defined(__EMSCRIPTEN__)
#  ifndef __EMSCRIPTEN_PTHREADS__
#    error "epi_thread.h requires -pthread when targeting Emscripten"
#  endif
#  define EPI_THREAD_POSIX
#else
#  define EPI_THREAD_POSIX
#endif

#if defined(EPI_THREAD_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#endif

namespace epi
{

typedef int (*ThreadFunc)(void *data);

#if defined(EPI_THREAD_WIN32)

typedef HANDLE             Thread;
typedef CRITICAL_SECTION   Mutex;
typedef CONDITION_VARIABLE Cond;
typedef volatile LONG      AtomicInt;

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
