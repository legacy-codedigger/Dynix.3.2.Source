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

/* $Header: externs.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)externs.c	4.2	(Berkeley)	5/9/83";
#endif not lint

# include	"trek.h"

/*
**	global variable definitions
*/

struct device	Device[NDEV] =
{
	"warp drive",		"Scotty",
	"S.R. scanners",	"Scotty",
	"L.R. scanners",	"Scotty",
	"phasers",		"Sulu",
	"photon tubes",		"Sulu",
	"impulse engines",	"Scotty",
	"shield control",	"Sulu",
	"computer",		"Spock",
	"subspace radio",	"Uhura",
	"life support",		"Scotty",
	"navigation system",	"Chekov",
	"cloaking device",	"Scotty",
	"transporter",		"Scotty",
	"shuttlecraft",		"Scotty",
	"*ERR 14*",		"Nobody",
	"*ERR 15*",		"Nobody"
};

char	*Systemname[NINHAB] =
{
	"ERROR",
	"Talos IV",
	"Rigel III",
	"Deneb VII",
	"Canopus V",
	"Icarus I",
	"Prometheus II",
	"Omega VII",
	"Elysium I",
	"Scalos IV",
	"Procyon IV",
	"Arachnid I",
	"Argo VIII",
	"Triad III",
	"Echo IV",
	"Nimrod III",
	"Nemisis IV",
	"Centarurus I",
	"Kronos III",
	"Spectros V",
	"Beta III",
	"Gamma Tranguli VI",
	"Pyris III",
	"Triachus",
	"Marcus XII",
	"Kaland",
	"Ardana",
	"Stratos",
	"Eden",
	"Arrikis",
	"Epsilon Eridani IV",
	"Exo III"
};
