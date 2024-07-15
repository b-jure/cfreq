#include <stdlib.h>
#include <stdio.h>

#include "cfalloc.h"
#include "cfstate.h"
#include "cferror.h"



/* minimum array size */
#define ARRSIZEMIN		8

/* maximum array size */
#define ARRSIZEMAX		MAX_CFSIZE




void *cfA_saferealloc(CFThread *th, void *p, size_t nsize) 
{
	if (nsize == 0) {
		free(p);
		return NULL;
	}
	if (!(p = realloc(p, nsize)))
		cfE_memerror(th);
	return p;
}


void *cfA_growarray_(CFThread *th, void *block, size_t *sizep, size_t nelems, size_t elemsize)
{
	int size;

	size = *sizep;
	if (nelems + 1 <= size)
		return block;
	if (cf_unlikely(size >= ARRSIZEMAX / 2)) {
		if (cf_unlikely(size >= ARRSIZEMAX))
			cfE_error(th, "array size limit reached '%zu'", ARRSIZEMAX);
		size = ARRSIZEMAX;
		cf_assert(size >= ARRSIZEMIN);
	} else {
		size <<= 1;
		if (size < ARRSIZEMIN)
			size = ARRSIZEMIN;
	}
	block = cfA_saferealloc(th, block, size * elemsize);
	*sizep = size;
	return block;
}


