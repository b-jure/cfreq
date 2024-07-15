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


#endif
