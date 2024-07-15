#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <assert.h>

#include "cfconf.h"
#include "cfstate.h"
#include "cferror.h"



/* Parsed args go here */
typedef struct CliArgs {
	int nthreads; /* number of threads '-t' */
} CliArgs;



static int clamptocorecount(unsigned long usercnt) {
#if defined(__linux__) || defined(_AIX) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
	int ncpu = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/sysctl.h>
	int mib[4];
	int ncpu;
	size_t len = sizeof(ncpu); 

	mib[0] = CTL_HW; /* Generic CPU, I/O */
	mib[1] = HW_NCPU; /* The number of cpus */
	sysctl(mib, 2, &ncpu, &len, NULL, 0);
#elif defined(__hpux)
#include <sys/mpctl.h>
	int ncpu = mpctl(MPC_GETNUMSPUS, NULL, NULL);
#elif defined(__sgi)
#include <unistd.h>
	int ncpu = sysconf(_SC_NPROC_ONLN);
#else
	int ncpu = 1;
#endif
	return (ncpu >= 1 ? ncpu : 1);
}


static int threadcnt(CFThread *mt, const char *tcnt) {
	static int ncpu = 0;
	char *endp;
	errno = 0;
	unsigned long cnt = strtoul(tcnt, &endp, 10);

	if (errno == ERANGE)

	if (endp != NULL || cnt < 1)
		cfE_error(mt, "%s", strerror(errno));
	if (ncpu == 0) {
		ncpu = clamptocorecount(cnt);
		return (ncpu > cnt ? cnt : ncpu);
	} else {
		return ncpu;
	}
}


static void parseargs(CFThread *mt, CliArgs *ctx, int argc, char **argv) {
	const char *arg;

	while (argc-- > 0) {
		arg = *argv++;
		switch (arg[0]) {
		case '-':
			switch(arg[1]) {
			case 't':
				if (argc-- >= 0)
					ctx->nthreads = threadcnt(mt, *argv++);
				else
					cfE_error(mt, "option '-t' is missing thread count");
				break;
			default:
				cfE_error(mt, "invalid option '-%c'", arg[1]);
			}
			break;
		default:
			cfS_addfilelock(mt, arg);
			break;
		}
	}
}


static void countfromfile(const char *filepath, int inmainth) {
	FILE *fp = fopen(filepath, "r");
	unsigned char buff[BUFSIZ];
	size_t n;

	if (!fp) {
		const char *err = strerror(errno);
		if (inmainth) 
			diemtf("%s", err);
		else 
			diethf("%s", err);
	}
	logmsg("reading '%s'...", filepath);
	while ((n = fread(buff, sizeof(buff[0]), BUFSIZ, fp)) > 0) {
		if (n != BUFSIZ && ferror(fp))
			goto error;
		for (size_t i = 0; i < n; i++)
			freqtable[buff[i]]++;
	}
	if (ferror(fp))
error:
		die(inmainth, "'fread' failed while reading '%s'", filepath);
}


/* count frequencies single thread */
static inline void count(void) {
	struct stat st;

	for (int i = 0; i < nfile_locks; i++) {
		const char *filename = file_locks[i].filepath;
		stat(filename, &st);
		countfromfile(file_locks[i].filepath, 1);
	}
}


/*
 * Recursive function for traversing directory.
 */
static void traversedirlocked(const char *dirname, int inmainth) {
	struct dirent *entry;
	struct stat st;
	DIR *dir;

	dir = opendir(dirname);
	if (!dir)
		dieunlock(inmainth, "opendir: %s", strerror(errno));
	for (errno = 0; (entry = readdir(dir)); errno = 0) {
		if (stat(entry->d_name, &st) < 0)
			dieunlock(inmainth, "stat: %s", strerror(errno));
		switch (st.st_mode & S_IFMT) {
		case S_IFLNK: case S_IFREG:
			break;
		case S_IFDIR:
			traversedirlocked(dirname, inmainth);
			break;
		default:
			dieunlock(inmainth, "invalid file type '%o'", st.st_mode & S_IFMT);
		}
	}
	if (errno != 0)
		dieunlock(inmainth, "readdir: %s", strerror(errno))
}


/* 
 * Recursive directory traversal.
 */
static void traversedir(TCtx *ctx, const char *dirname) {
	struct dirent *entry;
	struct stat st; /* use stat, 'dirent->d_type' might not be supported */
	DIR *dir;

	if (!(dir = opendir(dirname))) {
		diethf("opendir: %s", strerror(errno));
	} else {
		growdirarray(ctx);
		ctx->dirs[ctx->ndirs++] = dir;
	}
	for (errno = 0; (entry = readdir(dir)); errno = 0) {
		if (stat(entry->d_name, &st) < 0)
			dieunlock(ctx->mainthread, "stat: %s", strerror(errno));
		switch (st.st_mode & S_IFMT) {
		case S_IFLNK: case S_IFREG:
			pthread_mutex_lock(&global_mutex);
			growarray(file_locks, size_file_locks, nfile_locks, ctx->holdinglock,
					  ctx->mainthread);
			file_locks[nfile_locks].filepath = entry->d_name;
			file_locks[nfile_locks++].lock = 0;
			pthread_mutex_unlock(&global_mutex);
			break;
		case S_IFDIR:
			traversedir(ctx, dirname, inmainth);
			break;
		default:
			dieunlock(inmainth, "invalid file type '%o'", st.st_mode & S_IFMT);
		}
	}
	if (errno != 0)
		dieunlock(inmainth, "readdir: %s", strerror(errno));
}


/* 
 * Count frequencies multi-threaded 
 */
static void readfilelock(FileLock *fl, int inmainth) {
	const char *filename = fl->filepath;
	struct stat st;

	if (stat(filename, &st) < 0)
		die(inmainth, "stat: %s", strerror(errno));
	switch (st.st_mode & S_IFMT) {
	case S_IFLNK: case S_IFREG:
		countfromfile(filename, inmainth);
		break;
	case S_IFDIR:
		traversedir(filename, inmainth);
		break;
	default:
		die(inmainth, "invalid file type '%o'", st.st_mode & S_IFMT);
	}
}


/* auxiliary to 'printfreqtable' */
static int printspecial(size_t *freqtable) {
	const char *special[] = {
		"NULL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK",
		"\\a", "\\b", "\\t", "\\n", "\\v", "\\f", "\\r",
		"SO", "SI", "DLE", "DC1", "DC2", "DC3",	"DC4",
		"NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC",
		"FS", "GS", "RS", "US"
	};
	int i;

	for (i = 0; i < sizeof(special)/sizeof(special[0]); i++)
		fwritemsg(stdout, "'%-4s' : '%zu'\n", special[i], freqtable[i]);
	return i;
}


/* print 'freqtable' */
static void printfreqtable(size_t *freqtable) {
	for (int i = printspecial(); i < FREQSIZE; i++)
			fprintf(stdout, "'%-4c' : '%zu'\n", i, freqtable[i]);
}


/*
 * Thread entry function.
 * Internally this gets called after the thread
 * has been placed into the threadpool and the 'userdata'
 * is the corresponding 'CFThread' from the thread pool.
 */
void *threadentry(void *userdata) {
	CFThread *th = (CFThread *)userdata;
	FileLock *fl = NULL;
	pthread_t thread = pthread_self();

	logmsg("Thread %d start", pthread_self());
	for (;;) {
		pthread_mutex_lock(&global_mutex);
		if (nfile_locks > 0) {
			fl = &file_locks[--nfile_locks];
			fl->lock = 1; /* set lock */
			pthread_mutex_unlock(&global_mutex);
			readfilelock(fl, 0);
		} else {
			active_threads--;
			pthread_cond_wait(&global_condition, &global_mutex);
			active_threads++;
		}
	}
	logmsg("Thread %d exit", pthread_self());
	return NULL;
}


/*
 * Run all threads and count frequency.
 */
void runthreads(int threadcnt) {
	pthread_t thread;
	int i;

	pthread_mutex_init(&global_mutex, NULL);
	pthread_cond_init(&global_condition, NULL);
	nthreads = threadcnt;
	active_threads = threadcnt;
	for (i = 0; i < threadcnt; i++) {
		if (pthread_create(&thread, NULL, threadentry, &i) < 0)
			diemtf("%s", strerror(errno));
	}
	for (i = 0; i < nthreads; i++)
		pthread_join(threads[i].thread, NULL);
	if (thread_had_error)
		diemt("error in thread");
}


/* entry */
int main(int argc, char **argv) {
	CliArgs cliargs = { 0 };

	CFThread mt = cfS_new(threadentry);
	if (cf_unlikely(mt == NULL)) {
		fputs("cfreq: can't create state\n", stderr);
		exit(EXIT_FAILURE);
	}
	parseargs(mt, &cliargs, --argc, ++argv);
	if (cliargs.nthreads > 0) {
		cfS_newthreadpool(cliargs.nthreads);
	} else {
	}
	printfreqtable(crS_getfreqtable(mt));
	mainthreadcleanup();
	return 0;
}
