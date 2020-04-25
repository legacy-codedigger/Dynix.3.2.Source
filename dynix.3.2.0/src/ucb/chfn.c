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
static char rcsid[] = "$Header: chfn.c 2.5 90/11/13 $";
#endif

/*
 *	 chfn - change finger entries
 */
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <ctype.h>

struct default_values {
	char *name;
	char *office_num;
	char *office_phone;
	char *home_phone;
	char *universe;
};

char	passwd[] = "/etc/passwd";
char	temp[]	 = "/etc/ptmp";
struct	passwd *pwd;
struct	passwd *_getpwent(), *getpwnam(), *getpwuid();
int	_endpwent();
char	*crypt();
char	*getpass();

main(argc, argv)
	int argc;
	char *argv[];
{
	int user_uid;
	char replacement[6*BUFSIZ];
	char outbuffer[7*BUFSIZ];
	int fd;
	FILE *tf;
	int	len;

	if (argc > 2) {
		printf("Usage: chfn [user]\n");
		exit(1);
	}
	/*
	 * Error check to make sure the user (foolishly) typed their own name.
	 */
	user_uid = getuid();
	if ((argc == 2) && (user_uid != 0)) {
		pwd = getpwnam(argv[1]);
		if (pwd == NULL) {
			printf("%s%s%s%s%s%s%s%s",
				"There is no account for ", argv[1],
				" on this machine.\n", 
				"You probably mispelled your login name;\n",
				"only root is allowed to change another",
				" person's finger entry.\n",
				"Note:  you do not need to type your login",
				" name as an argument.\n");
			exit(1);
		}
		if (pwd->pw_uid != user_uid) {
			printf("%s%s",
				"You are not allowed to change another",
				" person's finger entry.\n");
			exit(1);
		}
	}
	/*
	 * If root is changing a finger entry, then find the uid that
	 * corresponds to the user's login name.
	 */
	if ((argc == 2) && (user_uid == 0)) {
		pwd = getpwnam(argv[1]);
		if (pwd == NULL) {
			printf("There is no account for %s on this machine\n", 
				pwd->pw_name);
			exit(1);
		}
		user_uid = pwd->pw_uid;
	}
	if (argc == 1) {
		pwd = getpwuid(user_uid);
		if (pwd == NULL) {
			fprintf(stderr, "No passwd file entry!?\n");
			exit(1);
		}
	}
	/*
	 * Collect name, room number, school phone, and home phone.
	 */
	get_info(pwd->pw_gecos, replacement);

	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTSTP, SIG_IGN);
	(void) umask(0);
	if ((fd = open(temp, O_CREAT|O_EXCL|O_RDWR, 0644)) < 0) {
		printf("Temporary file busy -- try again\n");
		exit(1);
	}
	if ((tf = fdopen(fd, "w")) == NULL) {
		printf("fdopen failure - seek help\n");
		goto out;
	}
	unlimit(RLIMIT_CPU);
	unlimit(RLIMIT_FSIZE);
	/*
	 * Copy passwd to temp, replacing matching lines
	 * with new gecos field.
	 */
	while ((pwd = _getpwent()) != NULL) {
		if (pwd->pw_uid == user_uid)
			pwd->pw_gecos = replacement;
		(void)sprintf(outbuffer,"%s:%s:%d:%d:%s:%s:%s\n",
			pwd->pw_name,
			pwd->pw_passwd,
			pwd->pw_uid,
			pwd->pw_gid,
			pwd->pw_gecos,
			pwd->pw_dir,
			pwd->pw_shell);
		if ((len=strlen(outbuffer)) >= BUFSIZ) {
			fprintf(stderr, "chfn: total size of entry is too large\n");
			(void) unlink(temp);
			exit(1);
		}
		if (fwrite(outbuffer,sizeof(char),len,tf) == EOF) {
			fprintf(stderr, "chfn: "); perror("write");
			(void) unlink(temp);
			exit(1);
		}
	}
	if (fflush(tf) == EOF) {
		fprintf(stderr, "chfn: "); perror("write");
		(void) unlink(temp);
		exit(1);
	}
	(void) _endpwent();
	if (rename(temp, passwd) < 0) {
		fprintf(stderr, "chfn: "); perror("rename");
  out:
		(void) unlink(temp);
		exit(1);
	}
	(void) fclose(tf);
	exit(0);
}

unlimit(lim)
{
	struct rlimit rlim;

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	(void) setrlimit(lim, &rlim);
}

/*
 * Get name, room number, school phone, and home phone.
 */
get_info(gecos_field, answer)
	char *gecos_field;
	char *answer;
{
	char *strcpy(), *strcat();
	char in_str[BUFSIZ];
	struct default_values *defaults, *get_defaults();

	answer[0] = '\0';
	defaults = get_defaults(gecos_field);
	printf("\nDefault values are printed inside of '[]'.\n");
	printf("To accept the default, type <return>.\n");
	printf("To have a blank entry, type the word 'none'.\n");
	/*
	 * Get name.
	 */
	do {
		printf("\nName [%s]: ", defaults->name);
		*in_str = '\0';
		(void) fgets(in_str, BUFSIZ, stdin);
		if (special_case(in_str, defaults->name)) 
			break;
	} while (illegal_input(in_str));
	(void) strcpy(answer, in_str);
	/*
	 * Get room number.
	 */
	do {
		printf("Room number [%s]: ", defaults->office_num);
		*in_str = '\0';
		(void) fgets(in_str, BUFSIZ, stdin);
		if (special_case(in_str, defaults->office_num))
			break;
	} while (illegal_input(in_str));
	(void) strcat(strcat(answer, ","), in_str);
	/*
	 * Get office phone number - remove hyphens if present.
	 */
	do {
		printf("Office Phone [%s]: ", defaults->office_phone);
		*in_str = '\0';
		(void) fgets(in_str, BUFSIZ, stdin);
		if (special_case(in_str, defaults->office_phone))
			break;
		remove_hyphens(in_str);
	} while (illegal_input(in_str) || not_all_digits(in_str));
	(void) strcat(strcat(answer, ","), in_str);
	/*
	 * Get home phone number - remove hyphens if present.
	 */
	do {
		printf("Home Phone [%s]: ", defaults->home_phone);
		*in_str = '\0';
		(void) fgets(in_str, BUFSIZ, stdin);
		if (special_case(in_str, defaults->home_phone))
			break;
		remove_hyphens(in_str);
	} while (illegal_input(in_str) || not_all_digits(in_str));
	(void) strcat(strcat(answer, ","), in_str);
	/*
	 * Get universe - att or ucb/bsd or NULL
	 */
	do {
		printf("Universe [%s]: ", defaults->universe);
		*in_str = '\0';
		(void) fgets(in_str, BUFSIZ, stdin);
		if (special_case(in_str, defaults->universe))
			break;
		if( strncmp(in_str, "bsd", 3) == 0 ) {
			strcpy(in_str, "ucb");
			break;
		}
	} while ( strncmp(in_str, "att", 3) && strncmp(in_str, "ucb", 3) );
	in_str[3] = '\0';
	strcat(answer, ",");
	if( in_str[0] == '\0' )
		return;
	(void) strcat(strcat(strcat(answer, "universe("), in_str), ")");
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
	if (input_str[length-1] != '\n') {
		/* the newline and the '\0' eat up two characters */
		printf("Maximum number of characters allowed is %d\n",
			BUFSIZ-2);
		/* flush the rest of the input line */
		while (getchar() != '\n')
			/* void */;
		error_flag = 1;
	}
	/*
	 * Delete newline by shortening string by 1.
	 */
	input_str[length-1] = '\0';
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

/*
 * Removes '-'s from the input string.
 */
remove_hyphens(str)
	char *str;
{
	char *hyphen, *index(), *strcpy();

	while ((hyphen=index(str, '-')) != NULL) {
		(void) strcpy(hyphen, hyphen+1);
	}
}

/*
 *  Checks to see if 'str' contains only digits (0-9).  If not, then
 *  an error message is printed and '1' is returned.
 */
not_all_digits(str)
	char *str;
{
	char *ptr;

	for (ptr=str; *ptr != '\0'; ++ptr) {
		if (!isdigit(*ptr)) {
			printf("Phone numbers can only contain digits.\n");
			return(1);
		}
	}
	return(0);
}

/* get_defaults picks apart "str" and returns a structure points.
 * "str" contains up to 5 fields separated by commas.
 * Any field that is missing is set to blank.
 */
struct default_values 
*get_defaults(str)
	char *str;
{
	struct default_values *answer;
	char *malloc(), *index();

	answer = (struct default_values *)
		malloc((unsigned)sizeof(struct default_values));
	if (answer == (struct default_values *) NULL) {
		fprintf(stderr,
			"\nUnable to allocate storage in get_defaults!\n");
		exit(1);
	}
	/*
	 * Values if no corresponding string in "str".
	 */
	answer->name = str;
	answer->office_num = "";
	answer->office_phone = "";
	answer->home_phone = "";
	answer->universe = "";
	str = index(answer->name, ',');
	if (str == 0) 
		return(answer);
	*str = '\0';
	answer->office_num = str + 1;
	str = index(answer->office_num, ',');
	if (str == 0) 
		return(answer);
	*str = '\0';
	answer->office_phone = str + 1;
	str = index(answer->office_phone, ',');
	if (str == 0) 
		return(answer);
	*str = '\0';
	answer->home_phone = str + 1;
	str = index(answer->home_phone, ',');
	if (str == 0) 
		return(answer);
	*str = '\0';
	if( strncmp("universe(", str+1, 9) || (str[13] != ')') )
		return(answer);
	*(str+13) = '\0';
	answer->universe = str + 10;
	return(answer);
}

/*
 *  special_case returns true when either the default is accepted
 *  (str = '\n'), or when 'none' is typed.  'none' is accepted in
 *  either upper or lower case (or any combination).  'str' is modified
 *  in these two cases.
 */
int 
special_case(str,default_str)
	char *str;
	char *default_str;
{
	static char word[] = "none\n";
	char *ptr, *wordptr;

	/*
	 *  If the default is accepted, then change the old string do the 
	 *  default string.
	 */
	if ((*str == '\n') || (*str == '\0')) {
		(void) strcpy(str, default_str);
		return(1);
	}
	/*
	 *  Check to see if str is 'none'.  (It is questionable if case
	 *  insensitivity is worth the hair).
	 */
	wordptr = word-1;
	for (ptr=str; *ptr != '\0'; ++ptr) {
		++wordptr;
		if (*wordptr == '\0')	/* then words are different sizes */
			return(0);
		if (*ptr == *wordptr)
			continue;
		if (isupper(*ptr) && (tolower(*ptr) == *wordptr))
			continue;
		/*
		 * At this point we have a mismatch, so we return
		 */
		return(0);
	}
	/*
	 * Make sure that words are the same length.
	 */
	if (*(wordptr+1) != '\0')
		return(0);
	/*
	 * Change 'str' to be the null string
	 */
	*str = '\0';
	return(1);
}
