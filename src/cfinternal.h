#ifndef CFINTERNAL_H
#define CFINTERNAL_H

#include "cfreq.h"


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

#endif
