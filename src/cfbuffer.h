#ifndef CFBUFFER_H
#define CFBUFFER_H


#include "cfreq.h"
#include "cfstate.h"


void initbuf(CFThread *th, Buffer *buf);
void freebuf(CFThread *th, Buffer *buf);
void int2buff(CFThread *th, Buffer *buf, int i);
void size2buff(CFThread *th, Buffer *buf, size_t n);
void str2buff(CFThread *th, Buffer *buf, const char *str, size_t len);
void c2buff(CFThread *th, Buffer *buf, int c);
void buffpoppath(Buffer *buf, cf_byte popsep);
char *cf_strdup(CFThread *th, const char *str);

#endif
