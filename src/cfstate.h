#ifndef CFSTATE_H
#define CFSTATE_H


#include <dirent.h>
#include <pthread.h>

#include "cfconf.h"



/* entry for threads */
typedef void *(*cf_ThreadFn)(void *userdata);



/* size of 'freqtable' in global state */
#define FREQSIZE			UCHAR_MAX



/* get global state */
#define G(th)		((th)->gs)



/* get lock on 'statemutex' */
#define cfS_lockstate(th) \
	{ if (!(th)->statelock) { \
	  while (pthread_mutex_trylock(&G(th)->statemutex) < 0) \
		  pthread_cond_wait(&G(th)->statecond, &G(th)->statemutex); \
	  (th)->statelock = 1; }}


/* release 'statemutex' lock and broadcast it */
#define cfS_unlockstate(th) \
	{ if ((th)->statelock) { (th)->statelock = 0; \
	  pthread_mutex_unlock(&G(th)->statemutex); \
	  pthread_cond_broadcast(&G(th)->statecond); }}



/* check if worker thread errored */
#define cfS_workererror(th)			(G(th)->therror)



/* thread context */
typedef struct CFThread {
	struct GState *gs;
	pthread_t thread; /* the thread itself */
	DIR **dirs; /* open directory streams */
	size_t ndirs; /* number of elements in 'dirs' */
	size_t sizedirs; /* size of 'dirs' */
	cf_byte mainthread; /* true if this is mainthread */
	cf_byte dead; /* true if thread is dead */
	cf_byte statelock;
} CFThread;



/* filenames with lock flag */
typedef struct FileLock {
	const char *filepath;
	volatile int lock; /* to prevent race */
} FileLock;



/* global state */
typedef struct GState {
	CFThread mainthread;
	CFThread *workerthreads; /* thread pool */
	FileLock *flocks; /* filepaths with a flag acting as a lock */
	cf_ThreadFn threadfn; /* entry function for threads */
	size_t sizeth; /* size of 'threads' */
	size_t nthreads; /* number of elements in 'threads' */
	size_t threadsact; /* number of active threads */
	size_t sizefl; /* size of 'flocks' */
	size_t nflocks; /* number of elements in 'flocks' */
	size_t freqtable[FREQSIZE]; /* character frequencies table */
	pthread_mutex_t statemutex; /* state access mutex */
	pthread_cond_t statecond;
	volatile cf_byte therror; /* true if any of the threads errored */
} GState;


CFThread *cfS_new(cf_ThreadFn fn);
void cfS_newthreadpool(GState *gs, size_t newthreads);
void cfS_addfilelock(CFThread *th, const char *filepath);
size_t *cfS_getfreqtable(CFThread *mt);
cf_noret cfS_freethread(CFThread *th);
void cfS_freethfrommt(CFThread *mt, CFThread *th);
void cfS_free(GState *gs);

#endif
