#include "cfreader.h"


/* initializer reader */
void cfR_init(cfreq_State *cfs, Reader *r, cfreq_fRead fread, void *ud)
{
	r->n = 0;
	r->buff = NULL;
	r->fread = fread;
	r->userdata = ud;
	r->cfs = cfs;
}


/* 
 * Invoke 'cfreq_fRead' returning the first character or 'CFEOF' (-1).
 * 'cfreq_fRead' should set the 'size' to the amount of bytes
 * reader read and return the pointer to the start of that
 * buffer. 
 */
int cfR_fill(Reader *r)
{
	cfreq_State *cfs;
	size_t size;
	const char *buff;

	cfs = r->cfs;
	buff = r->fread(cfs, r->userdata, &size);
	if (buff == NULL || size == 0)
		return CFEOF;
	r->buff = buff;
	r->n = size - 1;
	return *r->buff++;
}


/* 
 * Read 'n' bytes from 'Reader' returning
 * count of unread bytes or 0 if all bytes were read. 
 */
size_t cfR_readn(Reader *r, size_t n)
{
	size_t min;

	while (n) {
		if (r->n == 0) {
			if (cfR_fill(r) == CFEOF)
				return n;
			r->n++; /* cfR_fill decremented it */
			r->buff--; /* restore that character */
		}
		min = (r->n <= n ? r->n : n);
		r->n -= min;
		r->buff += min;
		n -= min;
	}
	return 0;
}
