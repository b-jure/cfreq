#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdint.h>
#include <setjmp.h>
#include <ctype.h>

#include "cfreq.h"
<<<<<<< HEAD
=======
#include "cfstate.h"
#include "cferror.h"
>>>>>>> 710ce35c58b15374dfa376ec2d3b48f21d0fd05a



/* Parsed args go here */
typedef struct CliArgs {
	cfreq_State *cfs;
	int nthreads; /* number of threads '-t' */
} CliArgs;



/* defer with status 'c' */
#define cfdefer(c) \
	{ status = c; goto defer; }



/* write error */
#define werror(err) \
	werrorfmt(err, CFREQ_SRCINFO)


/* write formatted error */
#define werrorf(fmt, ...) \
	werrorfmt(fmt, CFREQ_SRCINFO, __VA_ARGS__)


/*
 * Write to 'stderr'; expects in order: line, source
 * and rest of the args.
 */
static void werrorfmt(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	int line = va_arg(ap, int);
	const char *src = va_arg(ap, const char *);
	fprintf(stderr, PROG_NAME ": [%d:%s]: ", line, src);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	fflush(stderr);
	va_end(ap);
}


/* clamp provided thread count */
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
<<<<<<< HEAD
	return (ncpu >= (long int)usercnt ? (long int)usercnt : ncpu);
=======
	return (ncpu >= usercnt ? usercnt : ncpu);
>>>>>>> 710ce35c58b15374dfa376ec2d3b48f21d0fd05a
}


/* option -t */
static int threadcnt(const char *tcnt) {
	static int ncpu = 0;
	char *endp;
	errno = 0;
	unsigned long cnt = strtoul(tcnt, &endp, 10);

<<<<<<< HEAD
	if (errno == ERANGE || endp != NULL || cnt < 1)
		werror("invalid thread count (in option '-t')");
=======
	if (errno == ERANGE)
		cfreqE_error(mt, ERRMSG("strtoul - %S"), strerror(errno));
	if (endp != NULL || cnt < 1)
		cfreqE_error(mt, ERRMSG("invalid thread count (in option '-t')"));
>>>>>>> 710ce35c58b15374dfa376ec2d3b48f21d0fd05a
	if (ncpu == 0) {
		ncpu = clamptocorecount(cnt);
		return (ncpu > (int)cnt ? (int)cnt : ncpu);
	} else {
		return ncpu;
	}
}


/* parse cli arguments into 'cliargs' */
<<<<<<< HEAD
static int parseargs(CliArgs *cliargs, int argc, char **argv) {
	cf_byte nomoreopts = 0;
	int nofile = 1;
=======
static void parseargs(CFThread *mt, CliArgs *cliargs, int argc, char **argv) {
	cf_byte nomoreopts = 0;
>>>>>>> 710ce35c58b15374dfa376ec2d3b48f21d0fd05a

	while (argc-- > 0) {
		const char *arg = *argv++;
		switch (arg[0]) {
		case '-':
			if (nomoreopts) 
				goto addfilepath;
			switch(arg[1]) {
<<<<<<< HEAD
			case '-': 
				nomoreopts = 1;
				break;
			case 't':
				if (arg[2] != '\0') {
					arg = &arg[2];
				} else if (argc-- > 0) {
					arg = *argv++;
				} else {
					werror("option 't' is missing thread count");
					return 1;
				}
				cliargs->nthreads = threadcnt(arg);
				break;
			default:
				werrorf("invalid option '-%c'", arg[1]);
				return 1;
=======
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
>>>>>>> 710ce35c58b15374dfa376ec2d3b48f21d0fd05a
			}
			break;
		default:
addfilepath:
<<<<<<< HEAD
			nofile = 0; /* got a file */
			cfreq_addfilepath(cliargs->cfs, arg);
			break;
		}
	}
	if (nofile)
		werror("no files provided");
	return nofile;
=======
			cfreqS_addfilelock(mt, arg);
			break;
		}
	}
>>>>>>> 710ce35c58b15374dfa376ec2d3b48f21d0fd05a
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
	unsigned int i;

	for (i = 0; i < sizeof(special)/sizeof(special[0]); i++)
		fprintf(stdout, "%-4s : %zu\n", special[i], freqtable[i]);
	return i;
}


/* print 'freqtable' */
static void printfreqtable(size_t *freqtable) {
	for (int i = printspecial(freqtable); i < CFREQ_TABLESIZE; i++)
			fprintf(stdout, "%-4c : %zu\n", isgraph(i) ? i : '?', freqtable[i]);
	fflush(stdout);
}


static void *cfrealloc(void *block, void *ud, size_t os, size_t ns) {
	(void)(ud); /* unused */
	(void)(os); /* unused */

	if (ns == 0) {
		free(block);
		return NULL;
	}
	return realloc(block, ns);
}


<<<<<<< HEAD
/* error writer */
static void cferror(cfreq_State *cfs, const char *msg) {
	(void)(cfs); /* unused */
	fputs(msg, stderr);
	fflush(stderr);
}


/* error recovery */
static jmp_buf jmp;


/* panic handler */
static cf_noret cfpanic(cfreq_State *cfs) {
	(void)(cfs); /* unused */
	longjmp(jmp, 1);
}


/* cfreq */
=======
/* entry */
>>>>>>> 710ce35c58b15374dfa376ec2d3b48f21d0fd05a
int main(int argc, char **argv) {
	int status = EXIT_SUCCESS;
	char **argsstart = ++argv; /* prevent clobber warning ('setjmp') (GCC) */

<<<<<<< HEAD
	cfreq_State *cfs = cfreq_newstate(cfrealloc, NULL);
	if (cf_unlikely(cfs == NULL)) {
		werror("can't create state");
		cfdefer(EXIT_FAILURE);
	}
	cfreq_seterror(cfs, cferror);
	cfreq_setpanic(cfs, cfpanic);
	if (setjmp(jmp) == 0) {
		size_t outtab[CFREQ_TABLESIZE] = { 0 };
		CliArgs cliargs = { .cfs = cfs };
		if (parseargs(&cliargs, --argc, argsstart) != 0)
			cfdefer(EXIT_FAILURE);
		cfreq_count(cfs, cliargs.nthreads, outtab);
		printfreqtable(outtab);
	} else { /* return from panic handler */
		status = EXIT_FAILURE;
=======
	CFThread *mt = cfreqS_new(threadentry);
	if (cf_unlikely(mt == NULL)) {
		writeerror("can't create state");
		exit(EXIT_FAILURE);
	}
	parseargs(mt, &cliargs, --argc, ++argv);
	if (cliargs.nthreads > 0) {
		cfreqS_newthreadpool(mt, cliargs.nthreads);
	} else {
>>>>>>> 710ce35c58b15374dfa376ec2d3b48f21d0fd05a
	}
defer:
	cfreq_free(cfs);
	return status;
}
