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
static void traversedir(CFThread *th) {
	struct stat st;
	struct dirent *entry;

	DIR *dir = opendir(th->buf.str);
	if (dir == NULL)
		cfreqE_errnoerror(th, "opendir", errno);
	adddir(th, dir);
	th->buf.len--; /* pop null terminator */
	c2buff(th, &th->buf, CF_PATHSEP); /* add path separator */
	for (errno = 0; (entry = readdir(dir)) != NULL; errno = 0) {
		size_t len = strlen(entry->d_name) + 1;
		str2buff(th, &th->buf, entry->d_name, len);
		if (stat(th->buf.str, &st) < 0)
			cfreqE_errorf(th, "stat(%S): %S", th->buf.str, strerror(errno));
		if (S_ISREG(st.st_mode)) {
			cfreqS_addfilelock(th, th->buf.str, 1);
		} else if (S_ISDIR(st.st_mode)) {
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
				traversedir(th);
				break;
			}
		}
		th->buf.len -= len; /* remove previous 'entry->d_name' */
	}
	int olderrno = errno;
	if (cf_unlikely(closedir(th->dirs[--th->ndirs]) < 0))
		cfreqE_errnoerror(th, "closedir", errno);
	if (cf_unlikely(olderrno != 0))
		cfreqE_errnoerror(th, "readdir", olderrno);
	buffpoppath(&th->buf, 1); /* pop current directory + file sep */
}


/* add filepath to 'flocks' */
static void addfilepath(CFThread *th, const char *filepath) {
	struct stat st;

	initbuf(th, &th->buf);
	if (stat(filepath, &st) < 0)
		cfreqE_errorf(th, "stat(%S): %S", strerror(errno));
	if (S_ISREG(st.st_mode)) {
		cfreqS_addfilelock(th, filepath, 1);
	} else if (S_ISDIR(st.st_mode)) {
		str2buff(th, &th->buf, filepath, strlen(filepath) + 1);
		traversedir(th);
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
		cfreqE_errnoerror(fb->th, "fopen", errno);
}


static inline void closefb(FileBuf *fb) {
	if (cf_unlikely(fclose(fb->fp) == EOF))
		cfreqE_errnoerror(fb->th, "fclose", errno);
}


static const cf_byte *fillfilebuf(FileBuf *fb) {
	cf_assert(fb->n == 0 && fb->current != NULL);
	if (feof(fb->fp)) return (fb->current = NULL);
	fb->n = fread(fb->buf, 1, sizeof(fb->buf), fb->fp);
	if (fb->n == 0) return (fb->current = NULL);
	return (fb->current = fb->buf);
}


static inline int fbgetc(FileBuf *fb) {
	if (fb->n > 0) return (fb->n--, *fb->current++);
	else if (fb->current == NULL) return EOF;
	else return (fillfilebuf(fb), fbgetc(fb));
}


static void addtables(CFThread *th, size_t *dest, size_t *src, size_t size) {
	lockstatemutex(th);
	for (size_t i = 0; i < size; i++) { /* add counts to state table */
		if (cf_unlikely(dest[i] > MAX_CFSIZE - src[i])) {
			unlockstatemutex(th);
			cfreqE_errorf(th, "character count overflow, limit is '%N'", MAX_CFSIZE);
		}
		dest[i] += src[i];
	}
	unlockstatemutex(th);
}


/* count chars in regular file */
static void countfile(cfreq_State *cfs, CFThread *th, const char *filepath)
{
	CFREQTABLE(filetable) = { 0 };
	FileBuf fb;
	struct stat st;
	int c;

	if (cf_unlikely(stat(filepath, &st) < 0))
		cfreqE_errnoerror(th, "stat", errno);
	if (!S_ISREG(st.st_mode)) /* not a regular file ? */
		return; /* nothing to count */
	openfb(&fb, filepath, th); /* open file buffer */
	while ((c = fbgetc(&fb)) != EOF)
		filetable[(cf_byte)c]++;
	closefb(&fb); /* close file buffer */
	addtables(th, cfs->freqtable, filetable, CFREQ_TABLESIZE);
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
			countfile(cfs, th, fl->filepath);
		}
	}
}



/* -------------------------------------------------------------------------
 * Thread pool (worker threads)
 * ------------------------------------------------------------------------- */

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


static CFThread *addworker(cfreq_State *cfs, CFThread *th) {
	cf_assert(!th->mainthread);
	cf_assert(th->thread == pthread_self());
	lockstatemutex(th);
	cfreqA_growarray(th, cfs->workerthreads, cfs->sizeth, cfs->nthreads);
	CFThread *worker = &cfs->workerthreads[cfs->nthreads++];
	*worker = *th;
	cfs->threadsact++;
	unlockstatemutex(worker);
	pthread_cond_signal(&cfs->statecond); /* signal main thread */
	return worker;
}


/* entry for worker threads */
static void *workerfn(void *userdata) {
	cfreq_State *cfs = (cfreq_State *)userdata;
	CFThread th;

	initworker(&th, cfs);
	CFThread *worker = addworker(cfs, &th);
	addfilepaths(cfs, worker); /* first get all the files */
	countallfiles(cfs, worker); /* now start counting */
	lockstatemutex(worker);
	cfs->threadsact--;
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
	for (size_t i = 0; i < nthreads; i++) {
		res = pthread_create(&th.thread, NULL, workerfn, cfs);
		if (cf_unlikely(res != 0))
			cfreqE_pthreaderror(mt, "pthread_create", res);
	}
	lockstatemutex(mt);
	while (nthreads != cfs->nthreads) { /* wait until threads are registered */
		if (cf_unlikely(cfs->errworker)) { /* worker errored ? */
			unlockstatemutex(mt);
			goto workerror;
		}
		waitstatecond(mt);
	}
	unlockstatemutex(mt);
	for (size_t i = 0; i < nthreads; i++) { /* join on all threads */
		res = pthread_join(cfs->workerthreads[i].thread, NULL);
		if (cf_unlikely(res != 0))
			cfreqE_pthreaderror(mt, "pthread_join", res);
	}
	if (cf_unlikely(cfs->errworker)) {
workerror:
		cfreqE_error(mt, "worker thread had an error");
	}
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


/* close main thread open dirs */
void cfreqS_closemtdirs(CFThread *mt) {
	closedirs(mt);
}


/* free 'threads' */
static void cancelworkerthreads(cfreq_State *cfs) {
	for (size_t i = 0; i < cfs->nthreads; i++) {
		CFThread *th = &cfs->workerthreads[i];
		cf_assert(!th->statelock);
		if (!th->dead) {
			freebuf(th, &th->buf);
			closedirs(th);
			th->dead = 1;
			int res = pthread_cancel(th->thread);
			if (cf_unlikely(res != 0))
				cfreqE_pthreaderror(MT_(cfs), "pthread_cancel", res);
			res = pthread_join(th->thread, NULL);
			if (cf_unlikely(res != 0))
				cfreqE_pthreaderror(MT_(cfs), "pthread_join", res);
			cfs->threadsact--;
		}
		cf_assert(th->sizedirs == 0);
	}
	cf_assert(cfs->threadsact == 0);
	if (cfs->sizeth > 0)
		cfreqA_free(cfs, cfs->workerthreads,
					cfs->sizeth * sizeof(cfs->workerthreads[0]));
	cfs->workerthreads = NULL; cfs->nthreads = 0; cfs->sizeth = 0;
}


/* stops the count canceling all the worker threads */
void cfreqS_resetstate(cfreq_State *cfs) {
	if (cfs->nthreads == 0) /* no multithreading ? */
		closedirs(MT_(cfs));
	else
		cancelworkerthreads(cfs);
	resetfilelocks(cfs);
	memset(cfs->freqtable, 0, sizeof(cfs->freqtable));
}


/* free worker thread which is also the caller */
cf_noret cfreqS_freeworker(cfreq_State *cfs, CFThread *th) {
	cf_assert(!th->mainthread);
	cf_assert(th->thread == pthread_self());
	lockstatemutex(th);
	if (!th->dead) {
		freebuf(th, &th->buf);
		closedirs(th);
		th->dead = 1; /* mark it as dead */
		cfs->threadsact--;
	}
	unlockstatemutex(th);
	pthread_exit(NULL);
}
