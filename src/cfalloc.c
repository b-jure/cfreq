#include <stdlib.h>
#include <stdio.h>

#include "cfalloc.h"
#include "cfstate.h"
#include "cferror.h"



/* minimum array size */
#define ARRSIZEMIN		8

/* maximum array size */
#define ARRSIZEMAX		MAX_CFSIZE



/* malloc */
void *cfreqA_malloc(CFThread *th, size_t size) {
	cfreq_State *cfs = th->cfs;
	void *block = cfs->frealloc(NULL, cfs->ud, 0, size);

	if (cf_unlikely(block == NULL))
		cfreqE_memerror(th);
	return block;
}


/* free */
void cfreqA_free(cfreq_State *cfs, void *block, size_t osize) {
	cfs->frealloc(block, cfs->ud, osize, 0);
}


/* safe realloc */
void *cfreqA_realloc(CFThread *th, void *block, size_t osize, size_t nsize) 
{
	cfreq_State *cfs = th->cfs;

	block = cfs->frealloc(block, cfs->frealloc, osize, nsize);
	if (cf_unlikely(block == NULL))
		cfreqE_memerror(th);
	return block;
}


/* ensure array has space for at least 'nelems' + 'ensure' elements */
void *cfreqA_ensurearray_(CFThread *th, void *block, size_t *sizep,
						  size_t nelems, size_t ensure, size_t elemsize)
{
	size_t size = *sizep;

	if (cf_unlikely(ARRSIZEMAX - nelems < ensure)) /* would overflow? */
		cfreqE_error(th, ERRMSG("array size limit reached (%N)"), ARRSIZEMAX);
	ensure += nelems;
	if (ensure <= size)
		return block;
	if (cf_unlikely(size >= ARRSIZEMAX / 2)) {
		if (cf_unlikely(size >= ARRSIZEMAX))
			cfreqE_error(th, ERRMSG("array size limit reached (%N)"), ARRSIZEMAX);
		size = ARRSIZEMAX;
		cf_assert(size >= ARRSIZEMIN);
	} else {
		do {
			size <<= 1;
		} while (size < ensure);
		if (cf_unlikely(size < ARRSIZEMIN))
			size = ARRSIZEMIN;
	}
	block = cfreqA_realloc(th, block, *sizep * elemsize, size * elemsize);
	*sizep = size;
	return block;
}


/* grow array if it has no space for one more element */
void *cfreqA_growarray_(CFThread *th, void *block, size_t *sizep, size_t nelems,
						size_t elemsize)
{
	size_t size = *sizep;

	if (nelems + 1 <= size)
		return block;
	if (cf_unlikely(size >= ARRSIZEMAX / 2)) {
		if (cf_unlikely(size >= ARRSIZEMAX))
			cfreqE_error(th, ERRMSG("array size limit reached (%N)"), ARRSIZEMAX);
		size = ARRSIZEMAX;
		cf_assert(size >= ARRSIZEMIN);
	} else {
		size <<= 1;
		if (size < ARRSIZEMIN)
			size = ARRSIZEMIN;
	}
	block = cfreqA_realloc(th, block, *sizep * elemsize, size * elemsize);
	*sizep = size;
	return block;
}
