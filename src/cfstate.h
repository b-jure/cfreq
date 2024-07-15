#ifndef CFSTATE_H
#define CFSTATE_H


#include <dirent.h>
#include <pthread.h>

#include "cfreq.h"



/* entry for threads */
typedef void *(*cf_ThreadFn)(void *userdata);



/* get state */
#define S_(th)		((th)->cfs)



/* get lock on 'statemutex' */
#define cfreqS_lockstate(th) \
	{ if (!(th)->statelock) { \
	  while (pthread_mutex_trylock(&S_(th)->statemutex) < 0) \
		  pthread_cond_wait(&S_(th)->statecond, &S_(th)->statemutex); \
	  (th)->statelock = 1; }}


/* release 'statemutex' lock and broadcast it */
#define cfreqS_unlockstate(th) \
	{ if ((th)->statelock) { (th)->statelock = 0; \
	  pthread_mutex_unlock(&S_(th)->statemutex); \
	  pthread_cond_broadcast(&S_(th)->statecond); }}



/* check if worker thread errored */
#define cfreqS_workererror(th)			(S_(th)->therror)



/* thread context */
typedef struct CFThread {
	struct cfreq_State *cfs;
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
typedef struct cfreq_State {
	CFThread mainthread;
	CFThread *workerthreads; /* thread pool */
	FileLock *flocks; /* filepaths with a flag acting as a lock */
	cfreq_fRealloc frealloc; /* allocator */
	void *ud; /* userdata for 'frealloc' */
	cfreq_fError ferror; /* error writer (print) */
	cfreq_fPanic fpanic; /* panic handler */
	size_t sizeth; /* size of 'threads' */
	size_t nthreads; /* number of elements in 'threads' */
	size_t threadsact; /* number of active threads */
	size_t sizefl; /* size of 'flocks' */
	volatile size_t nflocks; /* number of elements in 'flocks' */
	size_t freqtable[CFREQ_TABLESIZE]; /* character frequencies table */
	pthread_mutex_t statemutex; /* state access mutex */
	pthread_cond_t statecond;
	volatile cf_byte therror; /* true if any of the threads errored */
} cfreq_State;


cfreq_State *cfreqS_new(cfreq_fRealloc fn, void *userdata, cfreq_fError fnerr);
void cfreqS_newthreadpool(cfreq_State *cfs, size_t nthreads);
void cfreqS_addfilelock(CFThread *th, const char *filepath);
size_t *cfreqS_getfreqtable(cfreq_State *cfs);
cf_noret cfreqS_freeworker(CFThread *th);
void cfreqS_free(cfreq_State *cfs);

#endif
