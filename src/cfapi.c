/*****************************************
 * Copyright (C) 2024 Jure B.
 * Refer to 'cfreq.h' for license details.
 *****************************************/

#include <string.h>
#include <pthread.h>

#include "cfreq.h"
#include "cfstate.h"


static void initmt(cfreq_State *cfs, CFThread *mt) {
	mt->cfs = cfs;
	memset(mt->counts, 0, sizeof(mt->counts));
	mt->thread = pthread_self();
	mt->buf.str = NULL; mt->buf.size = mt->buf.len = 0;
	mt->dirs = NULL; mt->sizedirs = 0;
	mt->mainthread = 1;
	mt->dead = mt->statelock = 0;
}


CFREQ_API cfreq_State *
cfreq_newstate(cfreq_fRealloc frealloc, void *ud) {
	cfreq_State *cfs;

	cfs = frealloc(NULL, ud, 0, STATESIZE);
	if (cf_unlikely(cfs == NULL))
		return NULL;
	initmt(cfs, MT_(cfs));
	cfs->workerthreads = NULL;
	cfs->flocks = NULL;
	cfs->frealloc = frealloc;
	cfs->ud = ud;
	cfs->ferror = NULL;
	cfs->sizeth = 0;
	cfs->nthreads = 0;
	cfs->threadsact = 0;
	cfs->sizefl = 0;
	cfs->nflocks = 0;
	cfs->condvar = 0;
	pthread_mutex_init(&cfs->statemutex, NULL);
	pthread_cond_init(&cfs->statecond, NULL);
	pthread_cond_init(&cfs->workercond, NULL);
	cfs->errworker = 0;
	cfs->log = 0;
	return cfs;
}


CFREQ_API void 
cfreq_free(cfreq_State *cfs) {
	CFThread *mt = MT_(cfs);
	lockstatemutex(mt);
	cfreqS_resetstate(cfs);
	unlockstatemutex(mt);
	/* ignore errors, state closing can't fail */
	pthread_mutex_destroy(&cfs->statemutex);
	pthread_cond_destroy(&cfs->statecond);
	pthread_cond_destroy(&cfs->workercond);
	cfs->frealloc(cfs, cfs->ud, STATESIZE, 0);
}


CFREQ_API cfreq_fPanic 
cfreq_setpanic(cfreq_State *cfs, cfreq_fPanic fpanic) {
	cfreq_fPanic old = cfs->fpanic;
	cfs->fpanic = fpanic;
	return old;
}


CFREQ_API cfreq_fError 
cfreq_seterror(cfreq_State *cfs, cfreq_fError ferror) {
	cfreq_fError old = cfs->ferror;
	cfs->ferror = ferror;
	return old;
}


CFREQ_API void 
cfreq_addfilepath(cfreq_State *cfs, const char *filepath) {
	cf_assert(filepath != NULL);
	cfreqS_addfilelock(MT_(cfs), filepath, 0);
}


CFREQ_API void
cfreq_count(cfreq_State *cfs, size_t nthreads, size_t dest[CFREQ_TABLESIZE],
			int log) 
{
	CFThread *mt = MT_(cfs);
	cfs->log = log;
	nthreads = (nthreads == 0 ? 1 : nthreads);
	cf_logf("counting with %zu thread/s", nthreads);
	if (nthreads == 1) 
		cfreqS_count(cfs);
	else 
		cfreqS_countthreaded(cfs, nthreads);
	cf_assert(mt->statelock);
	cf_log("copying over the total count");
	memcpy(dest, mt->counts, sizeof(mt->counts));
	cfreqS_resetstate(cfs);
	unlockstatemutex(mt);
}
