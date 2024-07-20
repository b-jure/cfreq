/*****************************************
 * Copyright (C) 2024 Jure B.
 * Refer to 'cfreq.h' for license details.
 *****************************************/

#ifndef CFBUFFER_H
#define CFBUFFER_H


#include "cfreq.h"
#include "cfstate.h"


#define bufpop(b)		((b)->str[--(b)->len])
#define buflast(b)		((b)->str[(b)->len - 1])


void cfreqB_init(CFThread *th, Buffer *buf);
void cfreqB_free(CFThread *th, Buffer *buf);
void cfreqB_addint(CFThread *th, Buffer *buf, int i);
void cfreqB_addsizet(CFThread *th, Buffer *buf, size_t n);
void cfreqB_addstring(CFThread *th, Buffer *buf, const char *str, size_t len);
void cfreqB_addchar(CFThread *th, Buffer *buf, int c);
char *cfreqB_strdup(CFThread *th, const char *str);

#endif
