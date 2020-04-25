
#include <stdio.h>
#include <signal.h>

char	buffer[80];

/*
 * Print the version of the currently running unix unless
 * the -r option is given, then print the version of the software that
 * this was released in. (used by software delta process.)
 */

main(argc, argv) 
	int	argc;
	char	**argv;
{
	signal(SIGSYS, SIG_IGN);
	if (((argc > 1) && (strcmp(argv[1], "-r") == 0 )) || 
				(get_vers(0, 80, buffer) == -1)) {
		printf("DYNIX(R) V3.2.0 \n");
	} else {
		printf("%s\n", buffer);
	} 
	exit(0);
}
