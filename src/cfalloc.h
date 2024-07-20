/*****************************************
 * Copyright (C) 2024 Jure B.
 * Refer to 'cfreq.h' for license details.
 *****************************************/

#ifndef CFALLOC_H
#define CFALLOC_H


#include "cfstate.h"


/* grow generic array */
#define cfreqA_growarray(th,b,sz,n) \
	((b) = cfreqA_growarray_(th, b, &(sz), n, sizeof(*b)))

/* ensure array space */
#define cfreqA_ensurearray(th,b,sz,n,e) \
	((b) = cfreqA_ensurearray_(th, b, &(sz), n, e, sizeof(*b)))

/* free array */
#define cfreqA_freearray(th,b,sz) \
	cfreqA_free(S_(th), b, (sz) * sizeof(*(b)))


void *cfreqA_malloc(CFThread *th, size_t size);
void cfreqA_free(cfreq_State *cfs, void *block, size_t osize);
void *cfreqA_realloc(CFThread *th, void *p, size_t osize, size_t nsize);
void *cfreqA_growarray_(CFThread *th, void *block, size_t *sizep, size_t nelems,
						size_t elemsize);
void *cfreqA_ensurearray_(CFThread *th, void *block, size_t *sizep,
						  size_t nelems, size_t ensure, size_t elemsize);

#endif
