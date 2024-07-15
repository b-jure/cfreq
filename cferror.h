#ifndef CFERROR_H
#define CFERROR_H

#include "cfstate.h"

cf_noret cfE_error(CFThread *th, const char *fmt, ...);
cf_noret cfE_memerror(CFThread *th);

#endif
