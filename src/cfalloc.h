#ifndef CFALLOC_H
#define CFALLOC_H


#include "cfstate.h"


/* grow generic array */
#define cfA_growarray(th,b,sz,n) \
	((b) = cfA_growarray_(th, b, &(sz), n, sizeof(*b)))


/* free memory */
#define cfA_free(p)		cfA_saferealloc(NULL, p, 0);


void *cfA_saferealloc(CFThread *th, void *p, size_t nsize);
void *cfA_growarray_(CFThread *th, void *block, size_t *sizep, size_t nelems,
					 size_t elemsize);

#endif
