/*****************************************
 * Copyright (C) 2024 Jure B.
 * Refer to 'cfreq.h' for license details.
 *****************************************/

#ifndef CFCONF_H
#define CFCONF_H


/* common headers */
#include <stddef.h>
#include <stdint.h>
#include <limits.h>


/* path separator character */
#if defined(__linux__)
#define CF_PATHSEP			'/'
#elif defined(__WIN32)
#define CF_PATHSEP			'\\'
#endif


/* maximum memory size */
#define MAX_CFSIZE		SIZE_MAX


/* signature for core library functions */
#if !defined(CFREQ_API)
#define CFREQ_API			extern
#endif


/* buffer size when reading files */
#if !defined(CF_BUFSIZ)
#define CF_BUFSIZ			(4096 * 10)
#endif


#endif
