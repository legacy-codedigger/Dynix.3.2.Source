/*
 *	c_usclk - Sample C program using usclk.
 */
#include <stdio.h>
#include <signal.h>
#include <usclkc.h>

void quit();

#define FALSE		0
#define RATE		1000000		/* ticks/second = 1 usec resolution*/
#define MAXSEC		4294.967296	/* secs to usclk rollover, */
					/*     = 1h:11m:34.967296secs */
main()
{
	int	i;
	usclk_t t32;
	double seq, b8 = 8000.0, b21 = 21000.0;

	signal(SIGBUS,quit);

	usclk_init();

	t32 = getusclk();

	for (i=0; i < 1000; i++)
		seq = b8 + b21;

	t32 = getusclk() - t32;

	/* Print out the delta time. */
	printf("C - Delta t32 = %lu microseconds.\n", t32);
}

void quit()
{
	fprintf(stderr, "c_usclk: access error reading /dev/usclk.\n");
	exit(1);
}
