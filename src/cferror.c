#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "cferror.h"
#include "cfbuffer.h"




/* panic */
static cf_noret cfreq_panic(cfreq_State *cfs, CFThread *th) {
	if (th->mainthread) {
		freebuf(th, &th->buf);
		cfreqS_closemtdirs(th);
		if (cfs->fpanic) {
			unlockstatemutex(th); /* unlock if locked */
			cfs->fpanic(cfs);
		}
		abort();
	} else {
		th->cfs->errworker = 1; /* no lock required here */
		cfreqS_freeworker(cfs, th);
	}
}


/* write format */
static inline void Ewritevf(cfreq_State *cfs, CFThread *th, const char *fmt,
							va_list ap) 
{
	const char *end;
	Buffer buf;

	initbuf(th, &buf);
	while ((end = strchr(fmt, '%')) != NULL) {
		str2buff(th, &buf, fmt, end - fmt);
		switch (end[1]) {
		case '%': {
			c2buff(th, &buf, '%');
			break;
		}
		case 'D': {
			int i = va_arg(ap, int);
			int2buff(th, &buf, i);
			break;
		}
		case 'N': {
			size_t n = va_arg(ap, size_t);
			size2buff(th, &buf, n);
			break;
		}
		case 'S': {
			const char *str = va_arg(ap, const char *);
			if (str == NULL)
				str2buff(th, &buf, "(null)", sizeof("(null)") - 1);
			else
				str2buff(th, &buf, str, strlen(str));
			break;
		}
		case 'C': {
			int c = va_arg(ap, int);
			c2buff(th, &buf, c);
			break;
		}
		default:
			freebuf(th, &buf);
			cfreqE_errorf(th, "unkown format specifier '%%%C'", end[1]);
		}
		fmt = end + 2; /* '%' + specifier */
	}
	str2buff(th, &buf, fmt, strlen(fmt));
	c2buff(th, &buf, '\0'); /* null terminate */
	cfs->ferror(cfs, buf.str);
	freebuf(th, &buf);
}


static void Ewritef(cfreq_State *cfs, CFThread *th, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	Ewritevf(cfs, th, fmt, ap);
	va_end(ap);
}


cf_noret cfreqE_error_(CFThread *th, const char *fmt, ...) {
	cfreq_State *cfs = S_(th);
	va_list ap;

	if (cfs->ferror) {
		va_start(ap, fmt);
		Ewritevf(cfs, th, fmt, ap);
		va_end(ap);
	}
	cfreq_panic(S_(th), th);
}


/* memory error */
cf_noret cfreqE_memerror(CFThread *th) {
	cfreq_State *cfs = S_(th);

	if (cfs->ferror)
		cfs->ferror(cfs, ERRMSG("out of memory"));
	cfreq_panic(cfs, th);
}


/* pthread error code to text */
static const char *pthreadcodetxt(int code) {
	/* Add more codes depending on which 'pthread'
	 * functions are checked for errors */
	switch (code) {
	case ESRCH: return "ESRCH";
	case EINVAL: return "EINVAL";
	case EDEADLK: return "EDEADLK";
	default: return "?";
	}
}


/* error for 'pthread_' functions that return errno code */
cf_noret cfreqE_pthreaderror_(CFThread *th, const char *pthread_fn, int code, ...) {
	cfreq_State *cfs = S_(th);

	if (cfs->ferror) {
		va_list ap;
		va_start(ap, code);
		int line = va_arg(ap, int);
		const char *file = va_arg(ap, const char *);
		Ewritef(cfs, th, ERRMSG("[%D:%S]: %S -> %S"),
				line, file, pthread_fn, pthreadcodetxt(code));
		va_end(ap);
	}
	cfreq_panic(cfs, th);
}


/* error for functions that set 'errno' */
cf_noret cfreqE_errnoerror_(CFThread *th, const char *errno_fn, int code, ...) {
	cfreq_State *cfs = S_(th);

	if (cfs->ferror) {
		va_list ap;
		va_start(ap, code);
		int line = va_arg(ap, int);
		const char *file = va_arg(ap, const char *);
		Ewritef(cfs, th, ERRMSG("[%D:%S]: %S (%S)"),
				line, file, errno_fn, strerror(code));
		va_end(ap);
	}
	cfreq_panic(cfs, th);
}
