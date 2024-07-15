#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cferror.h"
#include "cfalloc.h"



/* ensure buffer space */
#define ensurebuf(th,buf,n) \
	cfreqA_ensurearray(th, (buf)->str, (buf)->size, (buf)->len, n)


/* free buffer */
#define freebuf(th,buf)		cfreqA_freearray(th, (buf)->str, (buf)->size)


/* increase buffer length */
#define addlen2buff(buf,l)		((buf)->len += (l))




/* for number to string conversion */
#define MAXNUMDIGITS		44




/* panic */
static cf_noret cfreq_panic(CFThread *th, cfreq_State *cfs) {
	if (th->mainthread) {
		if (cfs->fpanic) {
			cfs->fpanic();
		} else {
			cfreqS_free(cfs);
			abort();
		}
	} else {
		th->cfs->therror = 1; /* no lock required here */
		cfreqS_freeworker(th);
	}
}


/* buffer for writting errors */
typedef struct Buffer {
	char *str;
	size_t len;
	size_t size;
} Buffer;


static inline void num2buff(CFThread *th, Buffer *buf, size_t n) {
	char temp[MAXNUMDIGITS] = { 0 };
	int cnt = snprintf(temp, sizeof(temp), "%lu", n);

	ensurebuf(th, buf, cnt);
	memcpy(buf->str, temp, cnt);
	addlen2buff(buf, cnt);
}


static inline void str2buff(CFThread *th, Buffer *buf, const char *str,
							size_t len) 
{
	ensurebuf(th, buf, len);
	memcpy(buf->str, str, len);
	addlen2buff(buf, len);
}


static inline void c2buff(CFThread *th, Buffer *buf, int c) {
	ensurebuf(th, buf, 1);
	buf->str[buf->len] = c;
	addlen2buff(buf, 1);
}


static inline void Ewritevf(CFThread *th, const char *fmt, va_list ap) {
	cfreq_State *cfs = th->cfs;
	const char *end;
	Buffer buf = { 0 };

	while ((end = strchr(fmt, '%')) != NULL) {
		str2buff(th, &buf, fmt, end - fmt);
		switch (end[1]) {
		case '%': {
			c2buff(th, &buf, '%');
			break;
		}
		case 'N': {
			size_t n = va_arg(ap, size_t);
			num2buff(th, &buf, n);
			break;
		}
		case 'S': {
			const char *str = va_arg(ap, const char *);
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
			cfreqE_error(th, ERRMSG("unknown format specifier '%%%C'"), end[1]);
		}
		fmt = end + 2; /* '%' + specifier */
	}
	str2buff(th, &buf, fmt, strlen(fmt));
	c2buff(th, &buf, '\0'); /* null terminate */
	S_(th)->ferror(S_(th), buf.str);
	freebuf(th, &buf);
}


cf_noret cfreqE_error(CFThread *th, const char *fmt, ...) {
	va_list ap;
	cf_byte ismainth;

	va_start(ap, fmt);
	Ewritevf(th, fmt, ap);
	va_end(ap);
	cfreq_panic(th, th->cfs);
	cf_assert(0); /* UNREACHED */
}


cf_noret cfreqE_memerror(CFThread *th) {
	cfreqE_error(th, ERRMSG("out of memory"));
}
