/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)assert.c	8.1 (Berkeley) 6/4/93
 * $FreeBSD: src/lib/libc/gen/assert.c,v 1.8 2007/01/09 00:27:53 imp Exp $
 * $DragonFly: src/lib/libc/gen/assert.c,v 1.5 2005/04/26 10:41:57 joerg Exp $
 */

#include <sys/syslog.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void
__assert(const char *func, const char *file, int line, const char *failedexpr)
{
	if (func == NULL) {
		fprintf(stderr,
		     "Assertion failed: (%s), file %s, line %d.\n", failedexpr,
		     file, line);
	} else {
		fprintf(stderr,
		     "Assertion failed: (%s), function %s, file %s, line %d.\n",
		     failedexpr, func, file, line);
	}
	abort();
	/* NOTREACHED */
}

enum {
	DIAGASSERT_ABORT =	1<<0,
	DIAGASSERT_STDERR =	1<<1,
	DIAGASSERT_SYSLOG =	1<<2
};

static int	diagassert_flags = -1;

void
__diagassert(const char *file, int line, const char *function,
	     const char *failedexpr)
{
	char buf[1024];

	if (diagassert_flags == -1) {
		const char *p;

		diagassert_flags = DIAGASSERT_SYSLOG | DIAGASSERT_ABORT;

		for (p = getenv("LIBC_DIAGASSERT"); p && *p; p++) {
			switch (*p) {
			case 'a':
				diagassert_flags |= DIAGASSERT_ABORT;
				break;
			case 'A':
				diagassert_flags &= ~DIAGASSERT_ABORT;
				break;
			case 'e':
				diagassert_flags |= DIAGASSERT_STDERR;
				break;
			case 'E':
				diagassert_flags &= ~DIAGASSERT_STDERR;
				break;
			case 'l':
				diagassert_flags |= DIAGASSERT_SYSLOG;
				break;
			case 'L':
				diagassert_flags &= ~DIAGASSERT_SYSLOG;
				break;
			}
		}
	}

	snprintf(buf, sizeof(buf),
		 "assertion \"%s\" failed: file \"%s\", line %d, function \"%s\"",
		 failedexpr, file, line, function);
	if (diagassert_flags & DIAGASSERT_STDERR)
		fprintf(stderr, "%s: %s\n", getprogname(), buf);
	if (diagassert_flags & DIAGASSERT_SYSLOG)
		syslog(LOG_DEBUG | LOG_USER, "%s", buf);
	if (diagassert_flags & DIAGASSERT_ABORT)
		abort();
}
