#ifndef CFREQ_H
#define CFREQ_H

#include <stdio.h>

#include "cfconf.h"


/* size of table holding character occurrences */
#define CFREQ_TABLESIZE		UCHAR_MAX



/* state */
typedef struct cfreq_State cfreq_State;


/* byte */
typedef unsigned char cf_byte;


/* error writer */
typedef void (*cfreq_fError)(cfreq_State *cfs, const char *msg);

/* file reader */
typedef const char *(*cfreq_fRead)(cfreq_State *cfs, void *userdata, size_t *szread);

/* allocator */
typedef void *(*cfreq_fRealloc)(void *block, void *ud, size_t os, size_t ns);

/* panic handler */
typedef cf_noret (*cfreq_fPanic)(void);



/* create new state */
cfreq_State *cfreq_newstate(cfreq_fRealloc frealloc, void *ud, cfreq_fError ferror);

/* free state */
void cfreq_free(cfreq_State *cfs);


/* set panic handler, return old one (or NULL) */
cfreq_fPanic cfreq_setpanic(cfreq_State *cfs, cfreq_fPanic fpanic);


/* add filepath to count */
void cfreq_addfilepath(cfreq_State *cfs, const char *filepath);


/* counts character occurrences from the already provided filepaths */
void cfreq_count(cfreq_State *cfs, cfreq_fRead fn, void *userdata, size_t nthreads);

/* get character counts (of 8 bit ASCII chars) into 'dest' */
void cfreq_getcount(cfreq_State *cfs, size_t dest[CFREQ_TABLESIZE]);


/* resets count cancels all worker threads and removes all filepaths */
void cfreq_resetcount(cfreq_State *cfs);

#endif
