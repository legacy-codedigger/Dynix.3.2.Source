/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef lint
static char rcsid[] = "$Header: chsh.c 2.6 90/04/10 $";
#endif

/*
 * chsh
 */
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>

char	*_PASSWD = "/etc/passwd";
char	temp[]	 = "/etc/ptmp";
char	def_path[] = "/bin/";
struct	passwd *pwd;
struct	passwd *_getpwent();
int	_endpwent();
char	*strcat();
char *index();
char	buffer[2*BUFSIZ];

main(argc, argv)
register char *argv[];
{
	register int u, fd;
	register FILE *tf;
	int	len;

	if (argc < 2 || argc > 3) {
		printf("Usage: chsh user [ shell ]\n");
		exit(1);
	}
	if (argc == 2)
		argv[2] = "";
	else {
		if (argv[2][0] != '/') {
			char *xarg = (char *)malloc(strlen(argv[2])+strlen(def_path)+3);
			strcpy(xarg, def_path);
			strcat(xarg, argv[2]);
			argv[2] = xarg;
		}
		if (index(argv[2], '\n') != NULL) {
			printf("%s is not a valid shell\n", argv[2]);
			exit(1);
		}
		if (access(argv[2], 1) < 0) {
			printf("%s is not available\n", argv[2]);
			exit(1);
		}
		if (illegal_input(argv[2]))
			exit(1);
	}
	if (strlen(argv[2]) > BUFSIZ) {
		printf("Path too long\n");
		exit(1);
	}
	unlimit(RLIMIT_CPU);
	unlimit(RLIMIT_FSIZE);
	while ((pwd=_getpwent()) != NULL) {
		if (strcmp(pwd->pw_name, argv[1]) == 0) {
			u = getuid();
			if (u!=0 && u != pwd->pw_uid) {
				printf("Permission denied.\n");
				exit(1);
			}
			break;
		}
	}
	_endpwent();

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	(void) umask(0);
	if ((fd = open(temp, O_CREAT|O_EXCL|O_RDWR, 0644)) < 0) {
		printf("Temporary file busy -- try again\n");
		exit(1);
	}
	if ((tf=fdopen(fd, "w")) == NULL) {
		printf("Absurd fdopen failure - seek help\n");
		goto out;
	}
	/*
	 * Copy passwd to temp, replacing matching lines
	 * with new shell.
	 */
	while ((pwd=_getpwent()) != NULL) {
		if (strcmp(pwd->pw_name, argv[1]) == 0) {
			u = getuid();
			if (u != 0 && u != pwd->pw_uid) {
				printf("Permission denied.\n");
				goto out;
			}
			pwd->pw_shell = argv[2];
		}
		if (strcmp(pwd->pw_shell, "/bin/sh") == 0)
			pwd->pw_shell = "";
		sprintf(buffer, "%s:%s:%d:%d:%s:%s:%s\n"
			, pwd->pw_name
			, pwd->pw_passwd
			, pwd->pw_uid
			, pwd->pw_gid
			, pwd->pw_gecos
			, pwd->pw_dir
			, pwd->pw_shell
		);
		if ((len = strlen(buffer)) > BUFSIZ) {
			printf("entry too long\n");
			goto out;
		}
		if (fwrite(buffer, sizeof(char), len, tf) == EOF) {
			perror("write to passwd file");
			goto out;
		}
	}
	_endpwent();
	if (rename(temp, _PASSWD) < 0) {
		fprintf(stderr, "chsh: "); perror("rename");
  out:
		unlink(temp);
		exit(1);
	}
	fclose(tf);
	exit(0);
}

unlimit(lim)
{
	struct rlimit rlim;

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	setrlimit(lim, &rlim);
}

/*
 * Prints an error message if a ':' or a newline is found in the string.
 * A message is also printed if the input string is too long.
 * The password file uses :'s as seperators, and are not allowed in the "gcos"
 * field.  Newlines serve as delimiters between users in the password file,
 * and so, those too, are checked for.  (I don't think that it is possible to
 * type them in, but better safe than sorry)
 *
 * Returns '1' if a colon or newline is found or the input line is too long.
 */
illegal_input(input_str)
	char *input_str;
{
	char *index();
	char *ptr;
	int error_flag = 0;
	int length = strlen(input_str);

	if (index(input_str, ':')) {
		printf("':' is not allowed.\n");
		error_flag = 1;
	}

	/*
	 * Don't allow control characters, etc in input string.
	 */
	for (ptr=input_str; *ptr != '\0'; ptr++) {
		if ((int) *ptr < 040) {
			printf("Control characters are not allowed.\n");
			error_flag = 1;
			break;
		}
		if (*ptr == '\n') {
			printf("new lines are not allowed.\n");
			error_flag = 1;
			*ptr = '\0';
			break;
		}
	}
	return(error_flag);
}
