#ifndef CFERROR_H
#define CFERROR_H

#include "cfstate.h"


/* error message format */
#define PROG_NAME		"cfreq"
#define ERRMSG(msg)		PROG_NAME ": " msg "\n"


cf_noret cfreqE_error(CFThread *th, const char *fmt, ...);
cf_noret cfreqE_memerror(CFThread *th);

#endif
