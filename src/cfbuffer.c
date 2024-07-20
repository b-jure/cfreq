/*****************************************
 * Copyright (C) 2024 Jure B.
 * Refer to 'cfreq.h' for license details.
 *****************************************/

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
void cfreqB_init(CFThread *th, Buffer *buf) {
	buf->size = 0;
	buf->len = 0;
	buf->str = NULL;
	ensurebuf(th, buf, MINBUFSIZE);
}


/* conversion for integers */
void cfreqB_addint(CFThread *th, Buffer *buf, int i) {
	char temp[MAXNUMDIGITS];
	int cnt = snprintf(temp, sizeof(temp), "%d", i);
	ensurebuf(th, buf, cnt);
	memcpy(&buf->str[buf->len], temp, cnt);
	addlen2buff(buf, cnt);
}


/* conversion for size_t  */
void cfreqB_addsizet(CFThread *th, Buffer *buf, size_t n) {
	char temp[MAXNUMDIGITS];
	int cnt = snprintf(temp, sizeof(temp), "%lu", n);
	ensurebuf(th, buf, cnt);
	memcpy(&buf->str[buf->len], temp, cnt);
	addlen2buff(buf, cnt);
}


/* conversion for strings */
void cfreqB_addstring(CFThread *th, Buffer *buf, const char *str, size_t len) 
{
	ensurebuf(th, buf, len);
	memcpy(&buf->str[buf->len], str, len);
	addlen2buff(buf, len);
}


/* conversion for chars */
void cfreqB_addchar(CFThread *th, Buffer *buf, int c) {
	ensurebuf(th, buf, 1);
	buf->str[buf->len++] = c;
}


/* free buffer memory */
void cfreqB_free(CFThread *th, Buffer *buf) {
	if (buf->size > 0) {
		cf_assert(buf->str != NULL);
		cfreqA_freearray(th, buf->str, buf->size);
	}
	buf->str = NULL; buf->size = buf->len = 0;
}


/* duplicate strings */
char *cfreqB_strdup(CFThread *th, const char *str) {
	cf_assert(str != NULL);
	size_t len = strlen(str);
	char *new = cfreqA_malloc(th, len + 1);
	memcpy(new, str, len);
	new[len] = '\0';
	return new;
}
