#include <string.h>
#include <pthread.h>

#include "cfreq.h"
#include "cfstate.h"
#include "cferror.h"



CFREQ_API cfreq_State *
cfreq_newstate(cfreq_fRealloc frealloc, void *ud) {
	cfreq_State *cfs;

	cfs = frealloc(NULL, ud, 0, STATESIZE);
	if (cf_unlikely(cfs == NULL))
		return NULL;
	*MT_(cfs) = (CFThread){ 
		.mainthread = 1,
		.thread = pthread_self(),
		.cfs = cfs 
	};
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
	memset(cfs->freqtable, 0, sizeof(cfs->freqtable));
	pthread_mutex_init(&cfs->statemutex, NULL);
	pthread_cond_init(&cfs->statecond, NULL);
	cfs->errworker = 0;
	return cfs;
}


CFREQ_API void 
cfreq_free(cfreq_State *cfs) {
	CFThread *mt = MT_(cfs);
	lockstatemutex(mt);
	cfreqS_resetstate(cfs);
	unlockstatemutex(mt);
	int res = pthread_mutex_destroy(&cfs->statemutex);
	if (cf_unlikely(res != 0))
		cfreqE_pthreaderror(mt, "pthread_mutex_destory", res);
	res = pthread_cond_destroy(&cfs->statecond);
	if (cf_unlikely(res != 0))
		cfreqE_pthreaderror(mt, "pthread_cond_destroy", res);
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
cfreq_count(cfreq_State *cfs, size_t nthreads, size_t dest[CFREQ_TABLESIZE]) {
	nthreads = (nthreads == 0 ? 1 : nthreads);
	if (nthreads == 1)
		cfreqS_count(cfs);
	else
		cfreqS_countthreaded(cfs, nthreads);
	memcpy(dest, cfs->freqtable, CFREQ_TABLESIZE);
	cfreqS_resetstate(cfs);
}
