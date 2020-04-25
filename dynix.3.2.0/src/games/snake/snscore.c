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

/* $Header: snscore.c 2.1 87/01/08 $ */

#ifndef lint
static char sccsid[] = "@(#)snscore.c	4.1 (Berkeley) 7/4/83";
#endif

#include <stdio.h>
#include <pwd.h>
char *recfile = "/usr/games/lib/snakerawscores";
#define MAXPLAYERS 256

char	*malloc();

struct	player	{
	short	uids;
	short	scores;
	char	*name;
} players[MAXPLAYERS], temp;

main()
{
	char	buf[80], cp;
	short	uid, score;
	FILE	*fd;
	int	noplayers;
	int	i, j, notsorted;
	short	whoallbest, allbest;
	char	*q;
	struct	passwd	*p;

	fd = fopen(recfile, "r");
	if (fd == NULL) {
		perror(recfile);
		exit(1);
	}
	printf("Snake players scores to date\n");
	fread(&whoallbest, sizeof(short), 1, fd);
	fread(&allbest, sizeof(short), 1, fd);
	for (uid=2;;uid++) {
		if(fread(&score, sizeof(short), 1, fd) == 0)
			break;
		if (score > 0) {
			if (noplayers > MAXPLAYERS) {
				printf("too many players\n");
				exit(2);
			}
			players[noplayers].uids = uid;
			players[noplayers].scores = score;
			/* This is faster if passwd is sorted by uid. */
			p = getpwuid(uid);
			if (p == NULL)
				continue;
			q = p -> pw_name;
			players[noplayers].name = malloc(strlen(q)+1);
			strcpy(players[noplayers].name, q);
			noplayers++;
		}
	}

	/* bubble sort scores */
	for (notsorted=1; notsorted; ) {
		notsorted = 0;
		for (i=0; i<noplayers-1; i++)
			if (players[i].scores < players[i+1].scores) {
				temp = players[i];
				players[i] = players[i+1];
				players[i+1] = temp;
				notsorted++;
			}
	}

	j = 1;
	for (i=0; i<noplayers; i++) {
		printf("%d:\t$%d\t%s\n", j, players[i].scores, players[i].name);
		if (players[i].scores > players[i+1].scores)
			j = i+2;
	}
	exit(0);
}

#ifdef	notdef
struct passwd *
getpwuid(uid)
register uid;
{
	register struct passwd *p;
	struct passwd *getpwent();

	while( (p = getpwent()) && p->pw_uid != uid );
	if (p->pw_uid == uid)
		return(p);
	setpwent();
	while( (p = getpwent()) && p->pw_uid != uid );
	endpwent();
	return(p);
}
#endif
