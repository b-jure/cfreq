#ifndef CFREADER_H
#define CFREADER_H

#include "cfreq.h"


#define cfreq_getc(r) \
	((r)->n-- > 0 ? ((cf_byte)*(r)->buff++) : cfreq_fill(r))


#define CFEOF		(-1)


typedef struct Reader {
    size_t n; /* unread bytes */
    const char* buff; /* position in buffer */
    cfreq_fRead fread; /* reader function */
    void* userdata; /* user data for 'cfreq_fRead' */
    cfreq_State* cfs; /* 'cfreq_State' for 'cfreq_fRead' */
} Reader;


void cfR_init(cfreq_State* cfs, Reader* r, cfreq_fRead fread, void* ud);
int cfR_fill(Reader* r);
size_t cfR_readn(Reader* r, size_t n);

#endif
