/*
 * Copyright (c) 1987, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * @(#)strcasecmp.c	8.1 (Berkeley) 6/4/93
 * $FreeBSD: src/lib/libc/string/strcasecmp.c,v 1.2.8.1 2001/07/09 23:30:03 obrien Exp $
 * $DragonFly: src/lib/libc/string/strcasecmp.c,v 1.7 2005/04/28 19:40:51 joerg Exp $
 */

#include <sys/types.h>
#include <string.h>
#include <ctype.h>

int
strcasecmp(const char *s1, const char *s2)
{
	while (tolower((u_char)*s1) == tolower((u_char)*s2++))
		if (*s1++ == '\0')
			return (0);
	return (tolower((u_char)*s1) - tolower((u_char)*--s2));
}

int
strncasecmp(const char *s1, const char *s2, size_t n)
{
	if (n != 0) {
		do {
			if (tolower((u_char)*s1) != tolower((u_char)*s2++))
				return (tolower((u_char)*s1) - tolower((u_char)*--s2));
			if (*s1++ == '\0')
				break;
		} while (--n != 0);
	}
	return (0);
}
