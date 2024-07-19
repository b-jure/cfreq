#include <string.h>

#include "cfbuffer.h"
#include "cfalloc.h"



/* ensure buffer space */
#define ensurebuf(th,buf,n) \
	cfreqA_ensurearray(th, (buf)->str, (buf)->size, (buf)->len, n)


/* increase buffer length */
#define addlen2buff(buf,l)		((buf)->len += (l))



/* for number to string conversion */
#define MAXNUMDIGITS		44


/* initial size for buffers */
#define MINBUFSIZE			64



/* initialize buffer */
void initbuf(CFThread *th, Buffer *buf) {
	buf->size = 0;
	buf->len = 0;
	buf->str = NULL;
	ensurebuf(th, buf, MINBUFSIZE);
}


/* conversion for integers */
void int2buff(CFThread *th, Buffer *buf, int i) {
	char temp[MAXNUMDIGITS];
	int cnt = snprintf(temp, sizeof(temp), "%d", i);
	ensurebuf(th, buf, cnt);
	memcpy(&buf->str[buf->len], temp, cnt);
	addlen2buff(buf, cnt);
}


/* conversion for size_t  */
void size2buff(CFThread *th, Buffer *buf, size_t n) {
	char temp[MAXNUMDIGITS];
	int cnt = snprintf(temp, sizeof(temp), "%lu", n);
	ensurebuf(th, buf, cnt);
	memcpy(&buf->str[buf->len], temp, cnt);
	addlen2buff(buf, cnt);
}


/* conversion for strings */
void str2buff(CFThread *th, Buffer *buf, const char *str, size_t len) 
{
	ensurebuf(th, buf, len);
	memcpy(&buf->str[buf->len], str, len);
	addlen2buff(buf, len);
}


/* conversion for chars */
void c2buff(CFThread *th, Buffer *buf, int c) {
	ensurebuf(th, buf, 1);
	buf->str[buf->len++] = c;
}


/* pop last filepath segment up until CF_PATHSEP */
void buffpoppath(Buffer *buf, cf_byte popsep) {
	char *sep = NULL;

	for (size_t i = 0; i < buf->len; i++) {
		if (buf->str[i] == CF_PATHSEP)
			sep = &buf->str[i];
	}
	if (sep && sep - buf->str > 1)
		buf->len = sep - buf->str + !popsep;
}


/* free buffer memory */
void freebuf(CFThread *th, Buffer *buf) {
	if (buf->size > 0) {
		cf_assert(buf->str != NULL);
		cfreqA_freearray(th, buf->str, buf->size);
	}
	buf->str = NULL; buf->size = buf->len = 0;
}


/* duplicate strings */
char *cf_strdup(CFThread *th, const char *str) {
	cf_assert(str != NULL);
	size_t len = strlen(str);
	char *new = cfreqA_malloc(th, len + 1);
	memcpy(new, str, len);
	new[len] = '\0';
	return new;
}
