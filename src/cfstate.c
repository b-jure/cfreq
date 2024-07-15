#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "cfstate.h"
#include "cferror.h"
#include "cfalloc.h"
#include "cfconf.h"



/* 
 * Create new state, and return mainthread if
 * state was built successfully. 
 */
CFThread *cfS_new(cf_ThreadFn fn) {
	GState *gs;

	gs = malloc(sizeof(*gs));
	if (cf_unlikely(gs == NULL))
		return NULL;
	gs->mainthread = (CFThread){ 
		.mainthread = 1,
		.thread = pthread_self(),
		.gs = gs 
	};
	gs->workerthreads = NULL;
	gs->flocks = NULL;
	gs->threadfn = fn;
	gs->sizeth = 0;
	gs->nthreads = 0;
	gs->threadsact = 0;
	gs->sizefl = 0;
	gs->nflocks = 0;
	memset(gs->freqtable, 0, FREQSIZE);
	pthread_mutex_init(&gs->statemutex, NULL);
	pthread_cond_init(&gs->statecond, NULL);
	gs->therror = 0;
	return &gs->mainthread;
}


void cfS_addfilelock(CFThread *mt, const char *filepath) {
	GState *gs;

	cf_assert(mt->mainthread);
	cfS_lockstate(mt);
	gs = G(mt);
	cfA_growarray(mt, gs->flocks, gs->sizefl, gs->nflocks);
	gs->flocks[gs->nflocks++] = (FileLock){ .filepath = filepath, .lock = 0 };
	cfS_unlockstate(mt);
}


size_t *cfS_getfreqtable(CFThread *mt) {
	return G(mt)->freqtable;
}


/* initialize worker thread */
static void initworker(CFThread *th, GState *gs) {
	cf_assert(!th->mainthread);
	th->gs = gs;
	th->thread = pthread_self();
	th->dirs = NULL;
	th->ndirs = 0;
	th->sizedirs = 0;
	th->mainthread = 0;
	th->statelock = 0;
}


/* entry for worker threads */
static void *workerentry(void *userdata) {
	CFThread th;
	CFThread *ud;
	GState *gs = (GState *)userdata;

	cf_assert(gs->threadfn != NULL);
	initworker(&th, gs);
	cfS_lockstate(&th);
	cfA_growarray(&th, gs->workerthreads, gs->sizeth, gs->nthreads);
	gs->workerthreads[gs->nthreads++] = th;
	gs->threadsact++;
	ud = &gs->workerthreads[gs->nthreads - 1];
	cfS_unlockstate(&th);
	gs->threadfn(ud); /* call user entry */
	cfS_lockstate(&th);
	gs->threadsact--;
	cfS_unlockstate(&th);
	return NULL;
}


/* create new thread pool */
void cfS_newthreadpool(GState *gs, size_t nthreads) {
	CFThread th; /* unused */

	/* thread pool must be empty */
	cf_assert(nthreads > 0);
	cf_assert(gs->threads == NULL && gs->nthreads == 0 && gs->sizeth ==  0);
	for (size_t i = 0; i < nthreads; i++) {
		if (cf_unlikely(pthread_create(&th.thread, NULL, workerentry, gs) < 0))
			cfE_error(&gs->mainthread, "%s", strerror(errno));
	}
}


/* close worker thread open dirs */
static inline void closedirs(CFThread *th) {
	if (th->dirs) {
		while (th->ndirs--)
			closedir(th->dirs[th->ndirs]); /* ignore failures */
		cfA_free(th->dirs); /* free 'dirs' */
		th->sizedirs = 0;
		th->dirs = NULL;
	}
	cf_assert(th->dirs == NULL && th->sizedirs == 0 && th->ndirs == 0);
}


/* free thread which is also the current thread running */
cf_noret cfS_freethread(CFThread *th) {
	GState *gs = th->gs;
	CFThread *threads = gs->workerthreads;

	cf_assert(!th->mainthread); /* can't be mainthread */
	cf_assert(th->thread == pthread_self()); /* this thread is current thread */
	cfS_lockstate(th);
	cf_assert(!th->dead); /* can't already be dead */
	closedirs(th);
	th->dead = 1; /* mark it as dead */
	gs->threadsact--;
	cfS_unlockstate(th);
	pthread_exit(NULL);
}


/* free 'threads' */
static void freethreadpool(GState *gs) {
	for (size_t i = 0; i < gs->nthreads; i++) {
		CFThread *th = &gs->workerthreads[i];
		cf_assert(!th->statelock);
		if (!th->dead) {
			closedirs(th);
			th->dead = 1;
			pthread_cancel(th->thread);
			gs->threadsact--;
		}
		cf_assert(th->sizedirs == 0 && th->ndirs == 0);
	}
	cf_assert(gs->threadsact == 0);
	if (gs->workerthreads)
		cfA_free(&gs->workerthreads);
}


/* free global state */
void cfS_free(GState *gs) {
	cfS_lockstate(&gs->mainthread);
	freethreadpool(gs);
	cfA_free(gs->flocks);
	cfS_unlockstate(&gs->mainthread);
	pthread_mutex_destroy(&gs->statemutex);
	pthread_cond_destroy(&gs->statecond);
	cfA_free(gs); /* finally free state */
}
