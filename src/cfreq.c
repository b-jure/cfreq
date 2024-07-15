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

#include "cfreq.h"
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
	return (ncpu >= usercnt ? usercnt : ncpu);
}


static int threadcnt(CFThread *mt, const char *tcnt) {
	static int ncpu = 0;
	char *endp;
	errno = 0;
	unsigned long cnt = strtoul(tcnt, &endp, 10);

	if (errno == ERANGE)
		cfreqE_error(mt, ERRMSG("strtoul - %S"), strerror(errno));
	if (endp != NULL || cnt < 1)
		cfreqE_error(mt, ERRMSG("invalid thread count (in option '-t')"));
	if (ncpu == 0) {
		ncpu = clamptocorecount(cnt);
		return (ncpu > cnt ? cnt : ncpu);
	} else {
		return ncpu;
	}
}


/* parse cli arguments into 'cliargs' */
static void parseargs(CFThread *mt, CliArgs *cliargs, int argc, char **argv) {
	cf_byte nomoreopts = 0;

	while (argc-- > 0) {
		const char *arg = *argv++;
		switch (arg[0]) {
		case '-':
			if (nomoreopts) 
				goto addfilepath;
			switch(arg[1]) {
			case '-': nomoreopts = 1; return;
			case 't':
				if (arg[2] != '\0')
					arg = &arg[2];
				else if (argc-- > 0)
					arg = *argv++;
				else
					cfreqE_error(mt, ERRMSG("option '-t' is missing thread count"));
				cliargs->nthreads = threadcnt(mt, arg);
				break;
			default:
				cfreqE_error(mt, ERRMSG("invalid option '-%C'"), arg[1]);
			}
			break;
		default:
addfilepath:
			cfreqS_addfilelock(mt, arg);
			break;
		}
	}
}


/* auxiliary to 'printfreqtable' */
static int printspecial(size_t *freqtable) {
	const char *special[] = {
		"NULL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK",
		"\\a", "\\b", "\\t", "\\n", "\\v", "\\f", "\\r",
		"SO", "SI", "DLE", "DC1", "DC2", "DC3",	"DC4",
		"NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC",
		"FS", "cfs", "RS", "US"
	};
	int i;

	for (i = 0; i < sizeof(special)/sizeof(special[0]); i++)
		fprintf(stdout, "'%-4s' : '%zu'\n", special[i], freqtable[i]);
	return i;
}


/* print 'freqtable' */
static void printfreqtable(size_t *freqtable) {
	for (int i = printspecial(freqtable); i < FREQSIZE; i++)
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

	for (;;) {
		// TODO: read files, recurse through directories
	}
	return NULL;
}


/* entry */
int main(int argc, char **argv) {
	CliArgs cliargs = { 0 };

	CFThread *mt = cfreqS_new(threadentry);
	if (cf_unlikely(mt == NULL)) {
		writeerror("can't create state");
		exit(EXIT_FAILURE);
	}
	parseargs(mt, &cliargs, --argc, ++argv);
	if (cliargs.nthreads > 0) {
		cfreqS_newthreadpool(mt, cliargs.nthreads);
	} else {
	}
	printfreqtable(crS_getfreqtable(mt));
	mainthreadcleanup();
	return 0;
}
