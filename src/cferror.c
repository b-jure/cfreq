#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "cferror.h"


/* name of the program (binary) */
#define PROGNAME	"cfreq"



static inline void writevferror(const char *fmt, va_list ap) {
	fputs(PROGNAME ": ", stderr);
	vfprintf(stderr, fmt, ap);
	fflush(stderr);
}


static inline void writeferror(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	writevferror(fmt, ap);
	va_end(ap);
}


cf_noret cfE_error(CFThread *th, const char *fmt, ...) {
	va_list ap;
	cf_byte ismainth;

	va_start(ap, fmt);
	writevferror(fmt, ap);
	va_end(ap);
	if (!th->mainthread) {
		th->gs->therror = 1; /* no lock required here */
		cfS_freethread(th);
	} else { /* full cleanup */
		cfS_free(th->gs);
		exit(EXIT_FAILURE);
	}
	cf_assert(0); /* UNREACHED */
}


cf_noret cfE_memerror(CFThread *th) {
	cfE_error(th, "out of memory");
}
