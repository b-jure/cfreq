#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>

#include "cfreq.h"



/* ascii txt + count */
typedef struct Tuple {
	const char *asciitxt;
	size_t count;
} Tuple;

/* character counts and their ASCII txt */
static Tuple cfcounts[CFREQ_TABLESIZE] = {
	{"NULL",0},{"SOH",0},{"STX",0},{"ETX",0},{"EOT",0},{"ENQ",0},{"ACK",0},
	{"\\a",0},{"\\b",0},{"\\t",0},{"\\n",0},{"\\v",0},{"\\f",0},{"\\r",0},
	{"SO",0},{"SI",0},{"DLE",0},{"DC1",0},{"DC2",0},{"DC3",0},{"DC4",0},
	{"NAK",0},{"SYN",0},{"ETB",0},{"CAN",0},{"EM",0},{"SUB",0},{"ESC",0},
	{"FS",0},{"CFS}",0},{"RS",0},{"US",0},{"SPACE",0},{"!",0},{"\"",0},{"#",0},
	{"$",0},{"%",0},{"&",0},{"'",0},{"(",0},{")",0},{"*",0},{"+",0},{",",0},
	{"-",0},{".",0},{"/",0},{"0",0},{"1",0},{"2",0},{"3",0},{"4",0},{"5",0},
	{"6",0},{"7",0},{"8",0},{"9",0},{":",0},{";",0},{"<",0},{"=",0},{">",0},
	{"?",0},{"@",0},{"A",0},{"B",0},{"C",0},{"D",0},{"E",0},{"F",0},{"G",0},
	{"H",0},{"I",0},{"J",0},{"K",0},{"L",0},{"M",0},{"N",0},{"O",0},{"P",0},
	{"Q",0},{"R",0},{"S",0},{"T",0},{"U",0},{"V",0},{"W",0},{"X",0},{"Y",0},
	{"Z",0},{"[",0},{"\\",0},{"]",0},{"^",0},{"_",0},{"`",0},{"a",0},{"b",0},
	{"c",0},{"d",0},{"e",0},{"f",0},{"g",0},{"h",0},{"i",0},{"j",0},{"k",0},
	{"l",0},{"m",0},{"n",0},{"o",0},{"p",0},{"q",0},{"r",0},{"s",0},{"t",0},
	{"u",0},{"v",0},{"w",0},{"x",0},{"y",0},{"z",0},{"{",0},{"|",0},{"}",0},
	{"~",0},{"DEL",0},
};



/* Parsed args go here */
typedef struct CliArgs {
	cfreq_State *cfs;
	int nthreads; /* number of threads '-t' */
	cf_byte ascii128; /* true if using 7 bit ASCII */
	cf_byte clock; /* use clock (time the execution) */
	cf_byte nums; /* show only numbers (counts) */
	char sort; /* sort == 0 (no sort), sort == -1 (rev) sort == 1 (sort) */
} CliArgs;




/* defer with status 'c' */
#define cfdefer(c) \
	{ status = c; goto defer; }



/* write error */
#define cfprinterror(err) \
	cffwritef(stderr, PROG_NAME ":[%d:%s]: " err ".\n", CFREQ_SRCINFO)

/* write formatted error */
#define cfprinterrorf(fmt, ...) \
	cffwritef(stderr, PROG_NAME ":[%d:%s]: " fmt ".\n", CFREQ_SRCINFO, __VA_ARGS__)

/* write to 'stdout' formatted */
#define cfprintf(fmt, ...)		cffwritef(stdout, fmt, __VA_ARGS__)



/* write to file stream 'fp' */
static void cffwritef(FILE *fp, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	fflush(fp);
}



/* get sort specifier */
static int sortspecifier(const char *sspec) {
	if (!strcmp("-1", sspec))
		return -1;
	else if (!strcmp("0", sspec))
		return 0;
	else if (!strcmp("1", sspec))
		return 1;
	cfprinterror("invalid sort specifier (try {-1,0,1})");
	return -2; /* error */
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
	return (ncpu >= (long int)usercnt ? (long int)usercnt : ncpu);
}


/* option -t */
static int threadcnt(const char *tcnt) {
	char *endp;
	errno = 0;
	unsigned long cnt = strtoul(tcnt, &endp, 10);

	if (errno == ERANGE || *endp != '\0' || cnt < 1) {
		cfprinterror("invalid thread count (in option '-t')");
		return -1;
	}
	return clamptocorecount(cnt);
}



/* check if we have more opts */
#define jmpifhaveopt(arg,i,l)		if (arg[++i] != '\0') goto l


/* parse cli arguments into 'cliargs' */
static int parseargs(CliArgs *cliargs, int argc, char **argv) {
	size_t i;
	int nofile = 1;
	cf_byte nomoreopts = 0;

	while (argc-- > 0) {
		const char *arg = *argv++;
		switch (arg[0]) {
		case '-':
			if (nomoreopts) 
				goto addfilepath;
			i = 1;
moreopts:
			switch(arg[i]) {
			case '-':  /* '--' */
				nomoreopts = 1;
				break;
			case 'n': /* show only numbers (counts ) */
				cliargs->nums = 1;
				jmpifhaveopt(arg, i, moreopts);
				break;
			case 'c': /* cpu clock */
				cliargs->clock = 1;
				jmpifhaveopt(arg, i, moreopts);
				break;
			case '7': /* 7-bit ascii */
				cliargs->ascii128 = 1;
				jmpifhaveopt(arg, i, moreopts);
				break;
			case '8': /* 8-bit ascii */
				cliargs->ascii128 = 0;
				if (arg[++i] != '\0') goto moreopts;
				break;
			case 's': /* sort */
				if (arg[++i] != '\0') {
					arg = &arg[i];
				} else if (argc-- > 0) {
					arg = *argv++;
				} else {
					cfprinterror("option 's' is missing sort specifier");
					return 1;
				}
				cliargs->sort = sortspecifier(arg);
				if (cliargs->sort < -1) return 1;
				break;
			case 't': /* thread count */
				if (arg[++i] != '\0') {
					arg = &arg[i];
				} else if (argc-- > 0) {
					arg = *argv++;
				} else {
					cfprinterror("option 't' is missing thread count");
					return 1;
				}
				cliargs->nthreads = threadcnt(arg);
				if (cliargs->nthreads < 0) return 1;
				break;
			default:
				cfprinterrorf("unknown option '-%c'", arg[i]);
				return 1;
			}
			break;
		default:
addfilepath:
			nofile = 0; /* got a file */
			cfreq_addfilepath(cliargs->cfs, arg);
			break;
		}
	}
	if (nofile)
		cfprinterror("no files provided");
	return nofile;
}


/* 'cfqsort' cmp fn */
typedef cf_byte (*cfCmpFn)(size_t lhs, size_t rhs);


/* comparison funcs */
static inline cf_byte cfqsortcmp(size_t lhs, size_t rhs) { return lhs >= rhs; }
static inline cf_byte cfqsortcmpr(size_t lhs, size_t rhs) { return lhs < rhs; }


/* auxiliary swap function for 'cfqsort' */
static inline void cfswap(Tuple *a, Tuple *b) {
	Tuple temp = *a;
	*a = *b;
	*b = temp;
}


/* quicksort */
static void cfqsort(Tuple *v, size_t start, size_t end, cfCmpFn fcmp) {
	if (start >= end) return;
	cfswap(&v[start], &v[(start + end) / 2]);
	size_t last = start;
	for (size_t i = last + 1; i <= end; i++)
		if ((*fcmp)(v[start].count, v[i].count))
			cfswap(&v[++last], &v[i]);
	cfswap(&v[start], &v[last]);
	if (last > start) cfqsort(v, start, last - 1, fcmp);
	cfqsort(v, last + 1, end, fcmp);
}


static void sortcfcounts(char sort) {
	if (sort == 0) return;
	cfCmpFn cmp = (sort < 0 ? cfqsortcmpr : cfqsortcmp);
	cfqsort(cfcounts, 0, CFREQ_TABLESIZE - 1, cmp);
}


/* print 'freqtable' */
static void printcfcounts(cf_byte all, cf_byte noascii) {
	for (size_t i = 0; i < CFREQ_TABLESIZE; i++) {
		Tuple *t = &cfcounts[i];
		if (!t->asciitxt && !all) continue;
		if (!noascii)
			cfprintf("%-5s  %zu\n", (t->asciitxt ? t->asciitxt : "?"), t->count);
		else
			cfprintf("%zu\n", t->count);
	}
}


/* allocator */
static void *cfrealloc(void *block, void *ud, size_t os, size_t ns) {
	(void)(ud); /* unused */
	(void)(os); /* unused */

	if (ns == 0) {
		free(block);
		return NULL;
	}
	return realloc(block, ns);
}


/* error writer */
static void cferror(cfreq_State *cfs, const char *msg) {
	(void)(cfs); /* unused */
	fputs(msg, stderr);
	fflush(stderr);
}


/* panic handler */
static cf_noret cfpanic(cfreq_State *cfs) {
	cfreq_free(cfs);
	exit(EXIT_FAILURE);
}


/* set 'cfcounts' tuple.counts */
static inline void setcfcounts(size_t *counts) {
	for (size_t i = 0; i < CFREQ_TABLESIZE; i++)
		cfcounts[i].count = counts[i];
}


/* print how much time it took to count */
static inline void printelapsed(cf_byte print, time_t diff) {
	if (print) cfprintf("\nTook ~ [%g seconds]\n", diff / (double)CLOCKS_PER_SEC);
}


/* cfreq */
int main(int argc, char **argv) {
	size_t counts[CFREQ_TABLESIZE] = { 0 };
	int status = EXIT_SUCCESS;

	cfreq_State *cfs = cfreq_newstate(cfrealloc, NULL);
	if (cfs == NULL) {
		cfprinterror("failed creating state");
		exit(EXIT_FAILURE);
	}
	cfreq_seterror(cfs, cferror);
	cfreq_setpanic(cfs, cfpanic);
	CliArgs cliargs = { .cfs = cfs };
	if (parseargs(&cliargs, --argc, ++argv) != 0)
		cfdefer(EXIT_FAILURE);
	time_t tick = clock();
	cfreq_count(cfs, cliargs.nthreads, counts);
	time_t tock = clock();
	setcfcounts(counts);
	sortcfcounts(cliargs.sort);
	printcfcounts(!cliargs.ascii128, cliargs.nums);
	printelapsed(cliargs.clock, tock - tick);
defer:
	cfreq_free(cfs);
	return status;
}
