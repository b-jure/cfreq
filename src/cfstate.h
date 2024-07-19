#ifndef CFSTATE_H
#define CFSTATE_H


#include <dirent.h>
#include <pthread.h>

#include "cfreq.h"



/* size of state */
#define STATESIZE		sizeof(cfreq_State)



/* get state */
#define S_(th)		((th)->cfs)

/* get main thread */
#define MT_(cfs)		(&(cfs)->mainthread)



/* lock 'statemutex' */
#define lockstatemutex(th) \
	{ if (!(th)->statelock) { \
	      pthread_mutex_lock(&S_(th)->statemutex); \
		  (th)->statelock = 1; } }


/* unlock 'statemutex' */
#define unlockstatemutex(th) \
	{ if ((th)->statelock) { \
	      (th)->statelock = 0; \
	      pthread_mutex_unlock(&S_(th)->statemutex); } }


/* wait on condition variable 'cond' */
#define waitcond(th, cond) \
	{ cf_assert((th)->statelock); (th)->statelock = 0; \
	  pthread_cond_wait(cond, &S_(th)->statemutex); \
	  (th)->statelock = 1; }



/* exit from thread if it is dead, do 'pre' before exiting */
#define checkdeadpre(th,pre) \
	if ((th)->dead) { cf_assert(!(th)->mainthread); \
		pre; unlockstatemutex(th); pthread_exit(NULL); }



/* check if thread is dead, if so then exit */
#define checkdead(th)		checkdeadpre(th, (void)0)


/* check if worker thread errored */
#define errorworker(th)			(S_(th)->errworker)


/* table for counting char occurrences of 8-bit ASCII */
#define CFREQTABLE(name)		size_t name[CFREQ_TABLESIZE]




/* buffer */
typedef struct Buffer { /* functions in 'cfbuffer.c' */
	char *str;
	size_t len;
	size_t size;
} Buffer;



/* thread context */
typedef struct CFThread {
	struct cfreq_State *cfs; /* state */
	CFREQTABLE(counts); /* character counts */
	pthread_t thread; /* the thread itself */
	Buffer buf; /* buffer for filepaths */
	DIR **dirs; /* open directory streams */
	size_t ndirs; /* number of elements in 'dirs' */
	size_t sizedirs; /* size of 'dirs' */
	cf_byte mainthread; /* true if this is mainthread */
	cf_byte dead; /* true if thread is dead */
	cf_byte statelock; /* true if holding lock on state mutex */
} CFThread;



/* filenames with lock flag */
typedef struct FileLock {
	char *filepath;
	volatile cf_byte lock; /* traverse lock */
	volatile cf_byte lockcnt; /* count lock */
} FileLock;



/* global state */
struct cfreq_State {
	CFThread mainthread;
	CFThread **workerthreads; /* thread pool */
	FileLock *flocks; /* filepaths with a flag acting as a lock */
	cfreq_fRealloc frealloc; /* allocator */
	void *ud; /* userdata for 'frealloc' */
	cfreq_fError ferror; /* error writer (print) */
	cfreq_fPanic fpanic; /* panic handler */
	size_t sizeth; /* size of 'threads' */
	size_t nthreads; /* number of elements in 'threads' */
	size_t threadsact; /* number of active threads */
	size_t sizefl; /* size of 'flocks' */
	size_t condvar; /* condition variable (for synchronization)  */
	volatile size_t nflocks; /* number of elements in 'flocks' */
	pthread_mutex_t statemutex; /* state access mutex */
	pthread_cond_t statecond; /* state condition */
	pthread_cond_t workercond; /* state condition */
	volatile cf_byte errworker; /* true if any of the worker threads errored */
};



void cfreqS_count(cfreq_State *cfs);
void cfreqS_countthreaded(cfreq_State *cfs, size_t nthreads);
void cfreqS_resetstate(cfreq_State *cfs);
void cfreqS_closemtdirs(CFThread *mt);
void cfreqS_addfilelock(CFThread *th, const char *filepath, cf_byte lock);
cf_noret cfreqS_freeworker(cfreq_State *cfs, CFThread *th);

#endif
