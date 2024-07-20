/*****************************************
 * Copyright (C) 2024 Jure B.
 * Refer to 'cfreq.h' for license details.
 *****************************************/

#ifndef CFERROR_H
#define CFERROR_H

#include "cfstate.h"


#define cfreqE_error(th,err) \
	cfreqE_error_(th, ERRMSG("[%D:%S]: " err), CFREQ_SRCINFO)

#define cfreqE_errorf(th,fmt,...) \
	cfreqE_error_(th, ERRMSG("[%D:%S]: " fmt), CFREQ_SRCINFO, __VA_ARGS__)


#define cfreqE_pthreaderror(th,name,c) \
	cfreqE_pthreaderror_(th, name, c, CFREQ_SRCINFO)

#define cfreqE_errnoerror(th,name,c) \
	cfreqE_errnoerror_(th, name, c, CFREQ_SRCINFO)


/* private ('_' at the end) */
cf_noret cfreqE_error_(CFThread *th, const char *fmt, ...);
cf_noret cfreqE_pthreaderror_(CFThread *th, const char *pthread_fn, int code, ...);
cf_noret cfreqE_errnoerror_(CFThread *th, const char *errno_fn, int code, ...);

cf_noret cfreqE_memerror(CFThread *th);

#endif
