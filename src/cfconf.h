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


/* enables internal asserts */
#if defined(CF_ASSERT)
#undef NDBG
#include <assert.h>
#define cf_assert(e)		assert(e)
#else
#define cf_assert(e)		((void)0)
#endif


/* enable logging */
#if defined(CF_LOG)
#define cf_log(msg)					fputs("cfreq: " msg ".\n", stderr)
#define cf_logf(fmt, ...)			fprintf(stderr, "cfreq: " fmt ".\n", __VA_ARGS__)
#else
#define cf_log(msg)					((void)0)
#define cf_logf(fmt, ...)			((void)0)
#endif


/* branch reordering */
#if defined(__GNUC__)
#define cf_likely(e)		__builtin_expect((e) != 0, 1)
#define cf_unlikely(e)		__builtin_expect((e) != 0, 0)
#else
#define cr_likely(e)		e
#define cr_unlikely(e)		e
#endif


/* signature for functions that do not return to caller */
#if defined(__GNUC__)
#define cf_noret			__attribute__((noreturn)) void
#else
#define cf_noret			void
#endif


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
