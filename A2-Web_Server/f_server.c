/*
 * Copyright (c) 2008 Bob Beck <beck@obtuse.com>
 *               2014 Stephen Just <sajust@ualberta.ca>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* server.c  - the "classic" example of a socket server */

/*
 * compile with gcc -o server server.c
 * or if you are on a crappy version of linux without strlcpy
 * thanks to the bozos who do glibc, do
 * gcc -c strlcpy.c
 * gcc -o server server.c strlcpy.o
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server_common.h"

int main(int argc,  char *argv[])
{
	struct sockaddr_in sockname, client;
	char *ep;
	socklen_t clientlen;
	int sd;
	u_short port;
	pid_t pid;
	u_long p;
	struct stat s;
	char *lf;
	FILE *logfile;

	if (argc != 4)
		usage();

	/*
	 * first, figure out what port we will listen on - it should
	 * be our first parameter.
	 */
	errno = 0;
        p = strtoul(argv[1], &ep, 10);
        if (*argv[1] == '\0' || *ep != '\0') {
		/* parameter wasn't a number, or was empty */
		fprintf(stderr, "%s - not a number\n", argv[1]);
		usage();
	}
        if ((errno == ERANGE && p == ULONG_MAX) || (p > USHRT_MAX)) {
		/* It's a number, but it either can't fit in an unsigned
		 * long, or is too big for an unsigned short
		 */
		fprintf(stderr, "%s - value out of range\n", argv[1]);
		usage();
	}
	/* now safe to do this */
	port = p;

	/*
	 * second, get the directory to serve
	 */
	if (*argv[2] == '\0') {
		/* parameter was empty */
		fprintf(stderr, "webroot empty\n");
		usage();
	}
	/* clean trailing slash - safe because strlen > 0 */
	if (strlen(argv[2]) > 1 && argv[2][strlen(argv[2])-1] == '/')
		argv[2][strlen(argv[2])-1] = '\0';
	if (stat(argv[2], &s) != 0) {
		if (errno == ENOENT) {
			fprintf(stderr, "Directory %s not found\n", argv[2]);
			usage();
		} else {
			fprintf(stderr, "Unkonwn error opening %s\n", argv[2]);
			usage();
		}
	}
	if (!S_ISDIR(s.st_mode)) {
		fprintf(stderr, "%s is not a directory\n", argv[2]);
		usage();
	}
	webroot = argv[2];
	fprintf(stdout, "Web root is %s\n", webroot);

	/*
	 * third, get the log file to use
	 */
	if (*argv[3] == '\0') {
		/* parameter was empty */
		fprintf(stderr, "logfile empty\n");
		usage();
	}
	logfile = fopen(argv[3], "a");
	if (logfile == NULL) {
		if (errno == EACCES) {
			fprintf(stderr, "Permission to file %s failed\n", argv[3]);
		} else if (errno == EISDIR) {
			fprintf(stderr, "%s is a directory\n", argv[3]);
		} else {
			fprintf(stderr, "Failed to open log file %s\n", argv[3]);
		}
		usage();
	}
	fclose(logfile);
	lf = argv[3];

	sd = bind_socket(sockname, port);

	/*
	 * we're now bound, and listening for connections on "sd" -
	 * each call to "accept" will return us a descriptor talking to
	 * a connected client
	 */


	sighandler_setup();
	if (pthread_mutex_init(&log_lock, NULL) != 0) {
		fprintf(stderr, "Failed to create mutex.\n");
		exit(1);
	}

	/*
	 * finally - the main loop.  accept connections and deal with 'em
	 */
	printf("Server up and listening for connections on port %u\n", port);

	if (daemon(0, 0) == -1) {
		printf("Failed to daemonize.\n");
		exit(1);
	}

	for(;;) {
		int clientsd;
		clientlen = sizeof(&client);
		clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");
		/*
		 * We fork child to deal with each connection, this way more
		 * than one client can connect to us and get served at any one
		 * time.
		 */

		pid = fork();
		if (pid == -1)
		     err(1, "fork failed");

		if(pid == 0) {
			do_request(clientsd, &client, lf);
			exit(0);
		}
		close(clientsd);
	}

	pthread_mutex_destroy(&log_lock);
}

