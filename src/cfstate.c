#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "cfstate.h"
#include "cferror.h"
#include "cfalloc.h"



/* size of state */
#define STATESIZE		sizeof(cfreq_State)



/* 
 * Create new state, and return mainthread if
 * state was built successfully. 
 */
cfreq_State *cfreqS_new(cfreq_fRealloc frealloc, void *ud, cfreq_fError ferror) {
	cfreq_State *cfs;

	cfs = frealloc(NULL, ud, 0, STATESIZE);
	if (cf_unlikely(cfs == NULL))
		return NULL;
	cfs->mainthread = (CFThread){ 
		.mainthread = 1,
		.thread = pthread_self(),
		.cfs = cfs 
	};
	cfs->workerthreads = NULL;
	cfs->flocks = NULL;
	cfs->frealloc = frealloc;
	cfs->ud = ud;
	cfs->ferror = ferror;
	cfs->sizeth = 0;
	cfs->nthreads = 0;
	cfs->threadsact = 0;
	cfs->sizefl = 0;
	cfs->nflocks = 0;
	memset(cfs->freqtable, 0, CFREQ_TABLESIZE);
	pthread_mutex_init(&cfs->statemutex, NULL);
	pthread_cond_init(&cfs->statecond, NULL);
	cfs->therror = 0;
	return cfs;
}


/* add filepath to 'flocks' */
void cfreqS_addfilelock(CFThread *th, const char *filepath) {
	cfreq_State *cfs = th->cfs;

	cfreqS_lockstate(th);
	cfreqA_growarray(th, cfs->flocks, cfs->sizefl, cfs->nflocks);
	cfs->flocks[cfs->nflocks++] = (FileLock){ .filepath = filepath, .lock = 0 };
	cfreqS_unlockstate(th);
}


/* get table of character occurrence counts */
size_t *cfreqS_getfreqtable(cfreq_State *cfs) {
	return cfs->freqtable;
}


/* initialize worker thread */
static void initworker(CFThread *th, cfreq_State *cfs) {
	cf_assert(!th->mainthread);
	th->cfs = cfs;
	th->thread = pthread_self();
	th->dirs = NULL;
	th->ndirs = 0;
	th->sizedirs = 0;
	th->mainthread = 0;
	th->statelock = 0;
}


/* traverse dirpath */
static void traversedir(CFThread *th, const char *dirpath) {
	struct stat st;
	struct dirent *de;

	DIR *dir = opendir(dirpath);
	if (dir == NULL)
		cfreqE_error(th, ERRMSG("opendir - %S"), strerror(errno));
	for (errno = 0; (de = readdir(dir)) != NULL; errno = 0) {
		if (stat(de->d_name, &st) < 0)
			cfreqE_error(th, ERRMSG("stat - %S"), strerror(errno));
		switch (st.st_mode & S_IFMT) {
		case S_IFREG: case S_IFLNK:
			cfreqS_addfilelock(th, de->d_name);
			break;
		case S_IFDIR:
			if (strcmp(de->d_name, "..") && strcmp(de->d_name, ".")) {

			}
			/* FALLTHRU */
		default: break; /* ignore */
		}
	}
	if (errno != 0)
		cfreqE_error(th, ERRMSG("readdir - %S"), strerror(errno));
}


/* traverse filepath */
static void traverse(CFThread *th, const char *filepath) {
	cfreq_State *cfs = S_(th);
	struct stat st;

	cf_assert(th->statelock); /* locked in 'fillflocks' */
	cfreqS_unlockstate(th);
	if (stat(filepath, &st) < 0)
		cfreqE_error(th, ERRMSG("stat - %S"), strerror(errno));
	switch (st.st_mode & S_IFMT) {
	case S_IFREG: case S_IFLNK: /* regular file or symlink? */
		cfreqS_addfilelock(th, filepath);
		break;
	case S_IFDIR: /* directory? */
		traversedir(th, filepath);
		break;
	default: break;
	}
}


/* fill flocks */
static void fillflocks(CFThread *th) {
	cfreq_State *cfs = S_(th);

	cfreqS_lockstate(th);
	for (size_t i = 0; i < cfs->nflocks; i++) {
		FileLock *fl = &cfs->flocks[i];
		if (!fl->lock) {
			fl->lock = 1;
			traverse(th, fl->filepath);
		}
	}
}


/* count characters */
static void countc(CFThread *th) {
}


/* entry for worker threads */
static void *workerentry(void *userdata) {
	cfreq_State *cfs = (cfreq_State *)userdata;
	CFThread *worker;
	CFThread th;

	cf_assert(cfs->threadfn != NULL);
	initworker(&th, cfs);
	cfreqS_lockstate(&th);
	cfreqA_growarray(&th, cfs->workerthreads, cfs->sizeth, cfs->nthreads);
	cfs->workerthreads[cfs->nthreads++] = th;
	cfs->threadsact++;
	worker = &cfs->workerthreads[cfs->nthreads - 1];
	cfreqS_unlockstate(worker);
	fillflocks(worker); /* first get all the files */
	countc(worker); /* now start counting */
	cfreqS_lockstate(worker);
	cfs->threadsact--;
	cfreqS_unlockstate(worker);
	return NULL;
}


/* create new thread pool */
void cfreqS_newthreadpool(cfreq_State *cfs, size_t nthreads) {
	CFThread th; /* unused */

	/* thread pool must be empty */
	cf_assert(nthreads > 0);
	cf_assert(cfs->threads == NULL && cfs->nthreads == 0 && cfs->sizeth ==  0);
	for (size_t i = 0; i < nthreads; i++) {
		if (cf_unlikely(pthread_create(&th.thread, NULL, workerentry, cfs) < 0))
			cfreqE_error(&cfs->mainthread, ERRMSG("pthread_create - %S"), strerror(errno));
	}
}


/* close worker thread open dirs */
static inline void closedirs(CFThread *th) {
	if (th->dirs) {
		while (th->ndirs--)
			closedir(th->dirs[th->ndirs]); /* ignore failures */
		cfreqA_free(th->cfs, th->dirs, th->ndirs * sizeof(th->dirs[0]));
		th->sizedirs = 0;
		th->dirs = NULL;
	}
	cf_assert(th->dirs == NULL && th->sizedirs == 0 && th->ndirs == 0);
}


/* free thread which is also the current thread running */
cf_noret cfreqS_freeworker(CFThread *th) {
	cfreq_State *cfs = th->cfs;
	CFThread *threads = cfs->workerthreads;

	cf_assert(!th->mainthread); /* can't be mainthread */
	cf_assert(th->thread == pthread_self()); /* this thread is current thread */
	cfreqS_lockstate(th);
	cf_assert(!th->dead); /* can't already be dead */
	closedirs(th);
	th->dead = 1; /* mark it as dead */
	cfs->threadsact--;
	cfreqS_unlockstate(th);
	pthread_exit(NULL);
}


/* free 'threads' */
static void cancelworkerthreads(cfreq_State *cfs) {
	cf_assert(cfs->mainthread.statelock);
	for (size_t i = 0; i < cfs->nthreads; i++) {
		CFThread *th = &cfs->workerthreads[i];
		cf_assert(!th->statelock);
		if (!th->dead) {
			closedirs(th);
			th->dead = 1;
			pthread_cancel(th->thread);
			cfs->threadsact--;
		}
		cf_assert(th->sizedirs == 0 && th->ndirs == 0);
	}
	cf_assert(cfs->threadsact == 0);
	if (cfs->workerthreads)
		cfreqA_free(cfs, cfs->workerthreads,
					cfs->sizeth * sizeof(cfs->workerthreads[0]));
	cfs->workerthreads = NULL; cfs->nthreads = 0; cfs->sizeth = 0;
}


/* free and reset the 'flocks' array */
static void resetfilelocks(cfreq_State *cfs) {
	cf_assert(cfs->mainthread.statelock);
	cfreqA_free(cfs, cfs->flocks, cfs->sizefl * sizeof(cfs->flocks[0]));
	cfs->flocks = NULL; cfs->sizefl = 0; cfs->nflocks = 0;
}


/* stops the count canceling all the worker threads */
void cfreqS_stopcount(cfreq_State *cfs) {
	cfreqS_lockstate(&cfs->mainthread);
	cancelworkerthreads(cfs);
	resetfilelocks(cfs);
	memset(cfs->freqtable, 0, sizeof(cfs->freqtable));
	cfreqS_unlockstate(&cfs->mainthread);
}


/* free global state */
void cfreqS_free(cfreq_State *cfs) {
	cfreqS_stopcount(cfs); /* mainthread takes lock here */
	pthread_mutex_destroy(&cfs->statemutex);
	pthread_cond_destroy(&cfs->statecond);
	cfs->frealloc(cfs, cfs->ud, STATESIZE, 0);
}
