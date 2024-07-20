/* ----------------------------------------------------------------------------------------------
 * Copyright (C) 2024 Jure B.
 *
 * This file is part of cfreq.
 * cfreq is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * cfreq is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with cfreq.
 * If not, see <https://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------------------------*/

#ifndef CFREQ_H
#define CFREQ_H

#include <stdio.h>

#include "cfconf.h"


/* error message format */
#define PROG_NAME		"cfreq"
#define ERRMSG(msg)		PROG_NAME ": " msg "\n"


/* source information */
#define CFREQ_SRCINFO		__LINE__, __FILE__


/* size of table holding character occurrences */
#define CFREQ_TABLESIZE		(UCHAR_MAX + 1)



/* state */
typedef struct cfreq_State cfreq_State;


/* byte */
typedef unsigned char cf_byte;


/* error writer (debug) */
typedef void (*cfreq_fError)(cfreq_State *cfs, const char *msg);

/* allocavarstor */
typedef void *(*cfreq_fRealloc)(void *block, void *ud, size_t os, size_t ns);

/* panic handler */
typedef cf_noret (*cfreq_fPanic)(cfreq_State *cfs);



/* create/free state */
CFREQ_API cfreq_State *cfreq_newstate(cfreq_fRealloc frealloc, void *ud);
CFREQ_API void cfreq_free(cfreq_State *cfs);

/* set handlers for panic and error writing */
CFREQ_API cfreq_fPanic cfreq_setpanic(cfreq_State *cfs, cfreq_fPanic fpanic);
CFREQ_API cfreq_fError cfreq_seterror(cfreq_State *cfs, cfreq_fError ferror);

/* add filepath to count */
CFREQ_API void cfreq_addfilepath(cfreq_State *cfs, const char *filepath);

/* counts characters and copy over the count into 'dest' */
CFREQ_API void cfreq_count(cfreq_State *cfs, size_t nthreads, size_t dest[CFREQ_TABLESIZE]);

#endif
