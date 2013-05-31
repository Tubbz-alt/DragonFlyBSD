/*
 * Copyright (c) 1980, 1993
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
 * @(#) Copyright (c) 1980, 1993 The Regents of the University of California.  All rights reserved.
 * @(#)swapon.c	8.1 (Berkeley) 6/5/93
 * $FreeBSD: src/sbin/dumpon/dumpon.c,v 1.10.2.2 2001/07/30 10:30:05 dd Exp $
 * $DragonFly: src/sbin/dumpon/dumpon.c,v 1.5 2006/03/25 07:44:14 dillon Exp $
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fstab.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sysexits.h>

void	usage(void) __dead2;

int
main(int argc, char **argv)
{
	int ch, verbose, rv;
	struct stat stab;
	int mib[2];
	char *path;

	verbose = rv = 0;
	while ((ch = getopt(argc, argv, "v")) != -1) {
		switch((char)ch) {
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argv += optind;

	if (argv[0] == NULL || argv[1])
		usage();

	path = argv[0];
	if (strcmp(path, "off") == 0) {
		stab.st_rdev = NODEV;
	} else {
		path = getdevpath(path, 0);
		rv = stat(path, &stab);
		if (rv)
			err(EX_OSFILE, "%s", path);

		if (!S_ISCHR(stab.st_mode)) {
			errx(EX_USAGE,
			     "%s: must specify a character disk device",
			     path);
		}
	}

	mib[0] = CTL_KERN;
	mib[1] = KERN_DUMPDEV;

	rv = sysctl(mib, 2, NULL, NULL, &stab.st_rdev, sizeof stab.st_rdev);
	if (rv) {
		err(EX_OSERR, "sysctl: kern.dumpdev");
	}

	if (verbose) {
		if (stab.st_rdev == NODEV) {
			printf("dumpon: crash dumps disabled\n");
		} else {
			printf("dumpon: crash dumps to %s (%lu, %#lx)\n",
			       path,
			       (unsigned long)major(stab.st_rdev),
			       (unsigned long)minor(stab.st_rdev));
		}
	}

	return 0;
}

void
usage(void)
{
	fprintf(stderr,
		"usage: dumpon [-v] special_file\n"
		"       dumpon [-v] off\n");
	exit(EX_USAGE);
}
