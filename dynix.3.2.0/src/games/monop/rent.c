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

/* $Header: rent.c 2.0 86/01/28 $ */

# include	"monop.ext"

/*
 *	This routine has the player pay rent
 */
rent(sqp)
reg SQUARE	*sqp; {

	reg int		rnt;
	reg PROP	*pp;
	PLAY		*plp;

	plp = &play[sqp->owner];
	printf("Owned by %s\n", plp->name);
	if (sqp->desc->morg) {
		lucky("The thing is mortgaged.  ");
		return;
	}
	switch (sqp->type) {
	  case PRPTY:
		pp = sqp->desc;
		if (pp->monop)
			if (pp->houses == 0)
				printf("rent is %d\n", rnt=pp->rent[0] * 2);
			else if (pp->houses < 5)
				printf("with %d houses, rent is %d\n",
				    pp->houses, rnt=pp->rent[pp->houses]);
			else
				printf("with a hotel, rent is %d\n",
				    rnt=pp->rent[pp->houses]);
		else
			printf("rent is %d\n", rnt = pp->rent[0]);
		break;
	  case RR:
		rnt = 25;
		rnt <<= (plp->num_rr - 1);
		if (spec)
			rnt <<= 1;
		printf("rent is %d\n", rnt);
		break;
	  case UTIL:
		rnt = roll(2, 6);
		if (plp->num_util == 2 || spec) {
			printf("rent is 10 * roll (%d) = %d\n", rnt, rnt * 10);
			rnt *= 10;
		}
		else {
			printf("rent is 4 * roll (%d) = %d\n", rnt, rnt * 4);
			rnt *= 4;
		}
		break;
	}
	cur_p->money -= rnt;
	plp->money += rnt;
}
