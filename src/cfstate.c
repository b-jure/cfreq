#define _DEFAULT_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cfstate.h"
#include "cfconf.h"
#include "cferror.h"
#include "cfalloc.h"
#include "cfbuffer.h"




/* -------------------------------------------------------------------------
 * Resolve filepaths
 * ------------------------------------------------------------------------- */

/* add external filepath to 'flocks' */
void cfreqS_addfilelock(CFThread *th, const char *filepath, cf_byte lock) {
	cfreq_State *cfs = th->cfs;
	char *dupfp = cf_strdup(th, filepath); /* might be in buffer */

	lockstatemutex(th);
	checkdead(th);
	cfreqA_growarray(th, cfs->flocks, cfs->sizefl, cfs->nflocks);
	cfs->flocks[cfs->nflocks++] = (FileLock){ .filepath = dupfp, .lock = lock };
	unlockstatemutex(th);
}


static void adddir(CFThread *th, DIR *dir) {
	checkdeadpre(th, closedir(dir));
	cfreqA_growarray(th, th->dirs, th->sizedirs, th->ndirs);
	th->dirs[th->ndirs++] = dir;
}


/* traverse directory */
static void traversedir(CFThread *th, const char *dirpath) {
	struct stat st;
	struct dirent *entry;
	cf_byte root;

	DIR *dir = opendir(dirpath);
	if (dir == NULL)
		cfreqE_errorf(th, "opendir(%S): %S", dirpath, strerror(errno));
	adddir(th, dir);
	cf_assert(buflast(&th->buf) == '\0');
	(void)bufpop(&th->buf); /* pop null terminator */
	root = !(buflast(&th->buf) != CF_PATHSEP);
	if (cf_likely(!root)) /* '/' not root ? */
		c2buff(th, &th->buf, CF_PATHSEP); /* add path separator */
	for (errno = 0; (entry = readdir(dir)) != NULL; errno = 0) {
		size_t len = strlen(entry->d_name);
		str2buff(th, &th->buf, entry->d_name, len);
		c2buff(th, &th->buf, '\0');
		if (lstat(th->buf.str, &st) < 0)
			cfreqE_errorf(th, "lstat(%S): %S", th->buf.str, strerror(errno));
		if (S_ISREG(st.st_mode)) {
			cfreqS_addfilelock(th, th->buf.str, 1);
		} else if (S_ISDIR(st.st_mode)) {
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
				traversedir(th, th->buf.str);
		}
		cf_assert(buflast(&th->buf) == '\0');
		(void)bufpop(&th->buf); /* pop null terminator */
		th->buf.len -= len; /* remove prev 'entry->d_name' */
	}
	int olderrno = errno;
	if (cf_unlikely(closedir(th->dirs[--th->ndirs]) < 0))
		cfreqE_errnoerror(th, "closedir", errno);
	if (cf_unlikely(olderrno != 0))
		cfreqE_errnoerror(th, "readdir", olderrno);
	cf_assert((!root) == (buflast(&th->buf) == CF_PATHSEP));
	th->buf.len -= !root;
	c2buff(th, &th->buf, '\0'); /* null terminate */
}


/* add filepath to 'flocks' */
static void addfilepath(CFThread *th, const char *filepath) {
	struct stat st;

	cf_logf("T%lu is traversing '%s'", th->thread, filepath);
	initbuf(th, &th->buf);
	if (lstat(filepath, &st) < 0)
		cfreqE_errorf(th, "lstat(%S): %S", filepath, strerror(errno));
	if (S_ISREG(st.st_mode)) {
		cfreqS_addfilelock(th, filepath, 1);
	} else if (S_ISDIR(st.st_mode)) {
		str2buff(th, &th->buf, filepath, strlen(filepath));
		c2buff(th, &th->buf, '\0');
		traversedir(th, filepath);
	}
	freebuf(th, &th->buf);
}


/* fill flocks */
static void addfilepaths(cfreq_State *cfs, CFThread *th) {
	lockstatemutex(th);
	for (size_t i = 0; i < cfs->nflocks && !th->dead; i++) {
		FileLock *fl = &cfs->flocks[i];
		if (!fl->lock) { /* if not locked */
			fl->lock = 1;
			unlockstatemutex(th);
			addfilepath(th, fl->filepath);
			lockstatemutex(th);
		}
	}
}



/* -------------------------------------------------------------------------
 * Read file (count)
 * ------------------------------------------------------------------------- */

typedef struct FileBuf {
	int n;
	FILE *fp;
	cf_byte buf[BUFSIZ];
	cf_byte *current;
	CFThread *th; /* for errors */
} FileBuf;


static inline void openfb(FileBuf *fb, const char *filepath, CFThread *th) {
	fb->n = 0;
	fb->current = fb->buf;
	fb->th = th;
	fb->fp = fopen(filepath, "r");
	if (cf_unlikely(fb->fp == NULL))
		cfreqE_errorf(th, "fopen(%S): %S", filepath, strerror(errno));
}


static inline void closefb(FileBuf *fb) {
	if (cf_unlikely(fclose(fb->fp) == EOF))
		cfreqE_errnoerror(fb->th, "fclose", errno);
}


static const cf_byte *fillfilebuf(FileBuf *fb) {
	cf_assert(fb->n == 0 && fb->current != NULL);
	if (feof(fb->fp)) return (fb->current = NULL);
	fb->n = fread(fb->buf, 1, sizeof(fb->buf), fb->fp);
	if (fb->n > 0) return (fb->current = fb->buf);
	return (fb->current = NULL);
}


static inline int fbgetc(FileBuf *fb) {
	if (fb->n > 0) return (fb->n--, *fb->current++);
	else if (fb->current == NULL) return EOF;
	else return (fillfilebuf(fb), fbgetc(fb));
}


/* combine counts from 'worker' thread into main thread table */
static void addcountstomt(cfreq_State *cfs, CFThread *worker) {
	CFThread *mt = MT_(cfs);
	cf_assert(!th->mainthread);
	for (size_t i = 0; i < CFREQ_TABLESIZE; i++) { /* add counts to state table */
		if (cf_unlikely(mt->counts[i] > MAX_CFSIZE - worker->counts[i])) {
			unlockstatemutex(worker);
			cfreqE_errorf(worker, "character count overflow, limit is '%N'", MAX_CFSIZE);
		}
		mt->counts[i] += worker->counts[i];
	}
}


/* count chars in regular file */
static void countfile(CFThread *th, const char *filepath) {
	FileBuf fb;
	int c;

	cf_logf("T%lu counting '%s'", th->thread, filepath);
	openfb(&fb, filepath, th); /* open file buffer */
	while ((c = fbgetc(&fb)) != EOF)
		th->counts[(cf_byte)c]++;
	closefb(&fb); /* close file buffer */
}


/* count characters */
static void countallfiles(cfreq_State *cfs, CFThread *th)
{
	lockstatemutex(th);
	for (size_t i = 0; i < cfs->nflocks && !th->dead; i++) {
		FileLock *fl = &cfs->flocks[i];
		if (!fl->lockcnt) { /* if not locked */
			fl->lockcnt = 1;
			unlockstatemutex(th);
			countfile(th, fl->filepath);
			checkdead(th);
			lockstatemutex(th);
		}
	}
}



/* -------------------------------------------------------------------------
 * Thread pool (worker threads)
 * ------------------------------------------------------------------------- */

/* allocate new worker thread */
static CFThread *newworker(cfreq_State *cfs) {
	/* deadthread is used in case worker allocation fails */
	CFThread deadthread = { .dead = 1, .cfs = cfs };
	CFThread *w = cfreqA_malloc(&deadthread, sizeof(*w));
	w->cfs = cfs;
	w->thread = pthread_self();
	w->buf.str = NULL; w->buf.len = 0; w->buf.size = 0;
	w->dirs = NULL; w->ndirs = 0; w->sizedirs = 0;
	w->mainthread = 0;
	w->dead = 0;
	w->statelock = 0;
	memset(w->counts, 0, sizeof(w->counts));
	return w;
}


/* add worker thread into thread pool */
static void addtothreadpool(cfreq_State *cfs, CFThread *th) {
	cf_assert(!th->mainthread);
	cf_assert(th->thread == pthread_self());
	lockstatemutex(th);
	cfreqA_growarray(th, cfs->workerthreads, cfs->sizeth, cfs->nthreads);
	cfs->workerthreads[cfs->nthreads++] = th;
	cfs->threadsact++;
	unlockstatemutex(th);
	pthread_cond_signal(&cfs->statecond); /* signal main thread */
}


/* close worker thread open dirs */
static void closedirs(CFThread *th) {
	if (th->dirs) {
		while (th->ndirs--)
			closedir(th->dirs[th->ndirs]); /* ignore failures */
		cfreqA_free(th->cfs, th->dirs, th->sizedirs * sizeof(th->dirs[0]));
		th->sizedirs = 0;
		th->dirs = NULL;
	}
}


/* clean up worker memory */
static void cleanupworker(cfreq_State *cfs, CFThread *worker) {
	cf_assert(!worker->mainthread);
	if (!worker->dead) {
		freebuf(worker, &worker->buf);
		closedirs(worker);
		worker->dead = 1; /* mark it as dead */
		cfs->threadsact--;
	}
}


/* entry for worker threads */
static void *workerfn(void *userdata) {
	cfreq_State *cfs = (cfreq_State *)userdata;
	CFThread *worker = newworker(cfs);
	addtothreadpool(cfs, worker);
	addfilepaths(cfs, worker); /* traverse all filepaths */
	countallfiles(cfs, worker); /* start counting */
	lockstatemutex(worker);
	addcountstomt(cfs, worker);
	cleanupworker(cfs, worker);
	unlockstatemutex(worker);
	return NULL;
}


/* count threaded */
void cfreqS_countthreaded(cfreq_State *cfs, size_t nthreads)
{
	CFThread *mt = MT_(cfs);
	CFThread th;
	int res;

	cfs->errworker = 0;
	/* thread pool must be empty */
	cf_assert(nthreads > 0 && cfs->sizeth == 0 && cfs->threadsact == 0);
	cf_logf("starting %zu threads", nthreads);
	for (size_t i = 0; i < nthreads; i++) {
		res = pthread_create(&th.thread, NULL, workerfn, cfs);
		if (cf_unlikely(res != 0))
			cfreqE_pthreaderror(mt, "pthread_create", res);
		cf_logf("T%lu started", th.thread);
	}
	lockstatemutex(mt);
	cf_log("waiting until threads are initialized");
	while (nthreads != cfs->nthreads) { /* wait until threads are registered */
		if (cf_unlikely(cfs->errworker)) { /* worker errored ? */
			unlockstatemutex(mt);
			goto workerror;
		}
		waitstatecond(mt);
	}
	unlockstatemutex(mt);
	for (size_t i = 0; i < nthreads; i++) { /* join on all threads */
		CFThread *worker = cfs->workerthreads[i];
		cf_logf("[%zu] MT joining on T%lu", i, worker->thread);
		res = pthread_join(worker->thread, NULL);
		if (cf_unlikely(res != 0))
			cfreqE_pthreaderror(mt, "pthread_join", res);
	}
	if (cf_unlikely(cfs->errworker)) {
workerror:
		cfreqE_error(mt, "worker thread had an error");
	}
	cf_log("all threads returned");
}


/* count on a single thread (mainthread) */
void cfreqS_count(cfreq_State *cfs) {
	cfs->errworker = 0;
	addfilepaths(cfs, MT_(cfs));
	countallfiles(cfs, MT_(cfs));
}



/* -------------------------------------------------------------------------
 * Reset/Cleanup state
 * ------------------------------------------------------------------------- */

/* free and reset the 'flocks' array */
static void resetfilelocks(cfreq_State *cfs) {
	cf_assert(MT_(cfs)->statelock);
	for (size_t i = 0; i < cfs->nflocks; i++) {
		char *filepath = cfs->flocks[i].filepath;
		cfreqA_free(cfs, filepath, strlen(filepath) + 1);
	}
	cfreqA_free(cfs, cfs->flocks, cfs->sizefl * sizeof(cfs->flocks[0]));
	cfs->flocks = NULL; cfs->sizefl = 0; cfs->nflocks = 0;
}


/* close main thread open dirs */
void cfreqS_closemtdirs(CFThread *mt) {
	closedirs(mt);
}


/* free 'threads' */
static void cancelworkerthreads(cfreq_State *cfs) {
	for (size_t i = 0; i < cfs->nthreads; i++) {
		CFThread *worker = cfs->workerthreads[i];
		cf_assert(!th->statelock);
		if (!worker->dead) {
			int res = pthread_cancel(worker->thread);
			if (cf_unlikely(res != 0))
				cfreqE_pthreaderror(MT_(cfs), "pthread_cancel", res);
			res = pthread_join(worker->thread, NULL);
			if (cf_unlikely(res != 0))
				cfreqE_pthreaderror(MT_(cfs), "pthread_join", res);
			cleanupworker(cfs, worker);
		}
		cf_assert(th->sizedirs == 0);
		cfreqA_free(cfs, worker, sizeof(*worker));
	}
	cf_assert(cfs->threadsact == 0);
	if (cfs->sizeth > 0)
		cfreqA_free(cfs, cfs->workerthreads,
				cfs->sizeth * sizeof(cfs->workerthreads[0]));
	cfs->workerthreads = NULL; cfs->nthreads = 0; cfs->sizeth = 0;
}


/* stops the count canceling all the worker threads */
void cfreqS_resetstate(cfreq_State *cfs) {
	CFThread *mt = MT_(cfs);
	if (cfs->nthreads == 0) /* no worker threads ? */
		closedirs(mt);
	else
		cancelworkerthreads(cfs);
	resetfilelocks(cfs);
	memset(mt->counts, 0, sizeof(mt->counts));
}


/* free worker thread which is also the caller */
cf_noret cfreqS_freeworker(cfreq_State *cfs, CFThread *th) {
	cf_assert(!th->mainthread);
	cf_assert(th->thread == pthread_self());
	lockstatemutex(th);
	cleanupworker(cfs, th);
	unlockstatemutex(th);
	pthread_exit(NULL);
}
