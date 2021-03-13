// This "setproctitile" function is port from nginx to keepalived of FADC by me
// ----------------------------------------------------------------------------

/*
 * Soft:        Keepalived is a failover program for the LVS project
 *              <www.linuxvirtualserver.org>. It monitor & manipulate
 *              a loadbalanced server pool using multi-layer checks.
 *
 * Part:        Set process title. This code is highly inspired from nginx
 *
 * Author:      Yu Kou, <ykou@fortinet.com>
 *
 *              This program is distributed in the hope that it will be useful, 
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *              See the GNU General Public License for more details.
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Copyright (C) 2001-2014 Yu Kou, <ykou@fortinet.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>	// for MIN and MAX
#ifdef PRSETNAME
#include <sys/prctl.h>
#endif

extern char **environ;

static char *argv_last = NULL;
static char **proc_argv = NULL;
static int succeed = 0;

int init_setproctitle(char ***argv_p)
{
	char *p;
	size_t size = 0;
	unsigned int i;

	char **new_argv;
	size_t argv_size = 0, argv_cnt;

	proc_argv = *argv_p;

	for (i = 0; environ[i]; i++) {
		size += strlen(environ[i]) + 1;
	}

	p = malloc(size);
	if (p == NULL) {
		return -1;
	}

	argv_last = proc_argv[0];

	for (i = 0; proc_argv[i]; i++) {
		if (argv_last == proc_argv[i]) {
			argv_last = proc_argv[i] + strlen(proc_argv[i]) + 1;
		}
		argv_size += strlen(proc_argv[i]) + 1;
	}
	argv_cnt = i;

	for (i = 0; environ[i]; i++) {
		if (argv_last == environ[i]) {

			size = strlen(environ[i]) + 1;
			argv_last = environ[i] + size;

			memcpy(p, environ[i], size);
			environ[i] = p;
			p += size;
		}
	}

	argv_last--;

	new_argv = malloc(argv_cnt * sizeof(*new_argv));
	if (new_argv == NULL)
		return -1;

	p = malloc(argv_size);
	if (p == NULL)
		return -1;

	for (i = 0; i < argv_cnt; i++) {
		size = strlen(proc_argv[i]) + 1;
		memcpy(p, proc_argv[i], size);
		new_argv[i] = p;
		p += size;
	}

	*argv_p = new_argv;

	succeed = 1;
	return 0;
}


void setproctitle(char *title)
{
	char *p;
	int len;

	if (!proc_argv || !argv_last || !succeed || !title || !strlen(title))
		return;

	proc_argv[1] = NULL;
	p = proc_argv[0];

#define PROCTITLE_PREFIX "keepalived: "
#if defined(ADD_PREFIX) && defined(PROCTITLE_PREFIX)
	len = MIN(argv_last - p, sizeof(PROCTITLE_PREFIX) - 1);
	memcpy(p, PROCTITLE_PREFIX, len);
	p += len;
#endif

	/* Changing argv[0] only affects /proc/self/cmdline, but doesn't affect /proc/self/comm */
#ifndef ARGV0_NOCHANGE
	len = MIN(argv_last - p, strlen(title));
	memcpy(p, title, len);
	p += len;

	if (argv_last - p > 0) {
		memset(p, 0, argv_last - p);
	}
#endif

#ifdef PRSETNAME
	/* PR_SET_NAME only affects /proc/self/comm, but doesn't affect /proc/self/cmdline */
	if (0 != prctl(PR_SET_NAME, title, 0, 0, 0)) {
		perror("prctl");
		exit(1);
	}
#endif
}
