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

#ifndef	lint
static	char	rcsid[] = "$Header: emon_cmds.c 2.3 87/04/11 $";
#endif

/*
 * $Log:	emon_cmds.c,v $
 */

#include "emon.h"

/*
 *  emon command table - to add an emon command:
 *	1. place command into commtab (emon_cmds.c)
 *	2. reference entry point in emon.h (note make depend)
 *	3. add do_do_<newcommand> wherever
 * note the "test" command can be used to develop new commands
 *	without having to affect emon.h - when the test is developed
 *	a make clean can be done.
 */

struct commtab {		/*** entry consists of name and handle **/
	char * name;		/*** pointer to command name ***/
	int (*do_do)();		/*** handle to commmand function ***/
	};

struct commtab doit[] = {
	{"autoprint", do_do_autoprint}, /** don't ask to print data **/
	{"bkeep", do_do_bkeep},		/** report bkeepet counts **/
	{"buck", do_do_buck},		/** report bucket counts **/
	{"count", do_do_count},		/** new counter **/
	{"clear", do_do_clear},		/** clear counters **/
	{"debug", do_do_debug},		/** flip flop debug flag **/
	{"dumpit", do_do_dumpit},	/** flip flop dumpit flag **/
	{"dump", do_do_dumpit},		/** flip flop dumpit flag **/
	{"h", do_do_help},		/** print command table **/
	{"help", do_do_help},		/** print command table **/
	{"intson", do_do_intson},	/** ensure interrupts enabled **/
	{"intsoff", do_do_intsoff},	/** ensure interrupts disabled **/
	{"k", do_do_k},			/** report keeps **/
	{"keepall", do_do_keepall},	/** change keepall policy */
	{"keeparp", do_do_keeparp},	/** change keeparp policy */
	{"keepat", do_do_keepat},	/** change keepat policy */
	{"keepbogus", do_do_keepbogus},	/** change keepbogus policy */
	{"keeptrail", do_do_keeptrail},	/** change keeptrail policy */
	{"keeptrap", do_do_keeptrap},	/** change keeptrap policy */
	{"keepwho", do_do_keepwho},	/** report keeper policies */
	{"keepxns", do_do_keepxns},	/** change keepxns policy */
	{"l", do_do_lookit},		/** short form for... */
	{"lookit", do_do_lookit},	/** lookit received buffers **/
	{"quit", do_do_quit},		/** quit **/
	{"q", do_do_quit},		/** quit **/
	{"reverse", do_do_reverse},	/** flip flop reverse flag **/
	{"stats", do_do_stats},		/** print statistics **/
	{"s", do_do_stats},		/** print statistics (short form)**/
	{"summ", do_do_summ},		/** print summary lines **/
	{"test", do_do_test},		/****for adding new cmds w/o emon.h **/
	{"trap", do_do_trap},		/** assign trap packet */
	{"trapwho", do_do_trapwho},	/** report trapees */
	{0, do_do_huh}			/***** END commtab ****/
};

char * ttab[] = {	/* translation table */
	"\\0",	/* - 00 - */
	"SOH",	/* - 01 - */
	"STX",	/* - 02 - */
	"ETX",	/* - 03 - */
	"EOT",	/* - 04 - */
	"ENQ",	/* - 05 - */
	"ACK",	/* - 06 - */
	"BEL",	/* - 07 - */
	"BS",	/* - 08 - */
	"HT",	/* - 09 - */
	"LF",	/* - 0a - */
	"VT",	/* - 0b - */
	"FF",	/* - 0c - */
	"CR",	/* - 0d - */
	"SO",	/* - 0e - */
	"SI",	/* - 0f - */
	"DLE",	/* - 10 - */
	"DC1",	/* - 11 - */
	"DC2",	/* - 12 - */
	"DC3",	/* - 13 - */
	"DC4",	/* - 14 - */
	"NAK",	/* - 15 - */
	"SYN",	/* - 16 - */
	"ETB",	/* - 17 - */
	"CAN",	/* - 18 - */
	"EM",	/* - 19 - */
	"SUB",	/* - 1a - */
	"ESC",	/* - 1b - */
	"FS",	/* - 1c - */
	"GS",	/* - 1d - */
	"RS",	/* - 1e - */
	"US",	/* - 1f - */
	"SP",	/* - 20 - */
	"!",	/* - 21 - */
	"\"",	/* - 22 - special case */
	"#",	/* - 23 - */
	"$",	/* - 24 - */
	"%",	/* - 25 - */
	"&",	/* - 26 - */
	"'",	/* - 27 - */
	"(",	/* - 28 - */
	")",	/* - 29 - */
	"*",	/* - 2a - */
	"+",	/* - 2b - */
	",",	/* - 2c - */
	"-",	/* - 2d - */
	".",	/* - 2e - */
	"/",	/* - 2f - */
	"0",	/* - 30 - */
	"1",	/* - 31 - */
	"2",	/* - 32 - */
	"3",	/* - 33 - */
	"4",	/* - 34 - */
	"5",	/* - 35 - */
	"6",	/* - 36 - */
	"7",	/* - 37 - */
	"8",	/* - 38 - */
	"9",	/* - 39 - */
	":",	/* - 3a - */
	";",	/* - 3b - */
	"<",	/* - 3c - */
	"=",	/* - 3d - */
	">",	/* - 3e - */
	"?",	/* - 3f - */
	"@",	/* - 40 - */
	"A",	/* - 41 - */
	"B",	/* - 42 - */
	"C",	/* - 43 - */
	"D",	/* - 44 - */
	"E",	/* - 45 - */
	"F",	/* - 46 - */
	"G",	/* - 47 - */
	"H",	/* - 48 - */
	"I",	/* - 49 - */
	"J",	/* - 4a - */
	"K",	/* - 4b - */
	"L",	/* - 4c - */
	"M",	/* - 4d - */
	"N",	/* - 4e - */
	"O",	/* - 4f - */
	"P",	/* - 50 - */
	"Q",	/* - 51 - */
	"R",	/* - 52 - */
	"S",	/* - 53 - */
	"T",	/* - 54 - */
	"U",	/* - 55 - */
	"V",	/* - 56 - */
	"W",	/* - 57 - */
	"X",	/* - 58 - */
	"Y",	/* - 59 - */
	"Z",	/* - 5a - */
	"[",	/* - 5b - */
	"\\",	/* - 5c - special case */
	"]",	/* - 5d - */
	"^",	/* - 5e - */
	"_",	/* - 5f - */
	"`",	/* - 60 - */
	"a",	/* - 61 - */
	"b",	/* - 62 - */
	"c",	/* - 63 - */
	"d",	/* - 64 - */
	"e",	/* - 65 - */
	"f",	/* - 66 - */
	"g",	/* - 67 - */
	"h",	/* - 68 - */
	"i",	/* - 69 - */
	"j",	/* - 6a - */
	"k",	/* - 6b - */
	"l",	/* - 6c - */
	"m",	/* - 6d - */
	"n",	/* - 6e - */
	"o",	/* - 6f - */
	"p",	/* - 70 - */
	"q",	/* - 71 - */
	"r",	/* - 72 - */
	"s",	/* - 73 - */
	"t",	/* - 74 - */
	"u",	/* - 75 - */
	"v",	/* - 76 - */
	"w",	/* - 77 - */
	"x",	/* - 78 - */
	"y",	/* - 79 - */
	"z",	/* - 7a - */
	"{",	/* - 7b - */
	"|",	/* - 7c - */
	"}",	/* - 7d - */
	"~",	/* - 7e - */
	"DEL",	/* - 7f - */
	"80",	/* - 80 - */
	"81",	/* - 81 - */
	"82",	/* - 82 - */
	"83",	/* - 83 - */
	"84",	/* - 84 - */
	"85",	/* - 85 - */
	"86",	/* - 86 - */
	"87",	/* - 87 - */
	"88",	/* - 88 - */
	"89",	/* - 89 - */
	"8a",	/* - 8a - */
	"8b",	/* - 8b - */
	"8c",	/* - 8c - */
	"8d",	/* - 8d - */
	"8e",	/* - 8e - */
	"8f",	/* - 8f - */
	"90",	/* - 90 - */
	"91",	/* - 91 - */
	"92",	/* - 92 - */
	"93",	/* - 93 - */
	"94",	/* - 94 - */
	"95",	/* - 95 - */
	"96",	/* - 96 - */
	"97",	/* - 97 - */
	"98",	/* - 98 - */
	"99",	/* - 99 - */
	"9a",	/* - 9a - */
	"9b",	/* - 9b - */
	"9c",	/* - 9c - */
	"9d",	/* - 9d - */
	"9e",	/* - 9e - */
	"9f",	/* - 9f - */
	"a0",	/* - a0 - */
	"a1",	/* - a1 - */
	"a2",	/* - a2 - */
	"a3",	/* - a3 - */
	"a4",	/* - a4 - */
	"a5",	/* - a5 - */
	"a6",	/* - a6 - */
	"a7",	/* - a7 - */
	"a8",	/* - a8 - */
	"a9",	/* - a9 - */
	"aa",	/* - aa - */
	"ab",	/* - ab - */
	"ac",	/* - ac - */
	"ad",	/* - ad - */
	"ae",	/* - ae - */
	"af",	/* - af - */
	"b0",	/* - b0 - */
	"b1",	/* - b1 - */
	"b2",	/* - b2 - */
	"b3",	/* - b3 - */
	"b4",	/* - b4 - */
	"b5",	/* - b5 - */
	"b6",	/* - b6 - */
	"b7",	/* - b7 - */
	"b8",	/* - b8 - */
	"b9",	/* - b9 - */
	"ba",	/* - ba - */
	"bb",	/* - bb - */
	"bc",	/* - bc - */
	"bd",	/* - bd - */
	"be",	/* - be - */
	"bf",	/* - bf - */
	"c0",	/* - c0 - */
	"c1",	/* - c1 - */
	"c2",	/* - c2 - */
	"c3",	/* - c3 - */
	"c4",	/* - c4 - */
	"c5",	/* - c5 - */
	"c6",	/* - c6 - */
	"c7",	/* - c7 - */
	"c8",	/* - c8 - */
	"c9",	/* - c9 - */
	"ca",	/* - ca - */
	"cb",	/* - cb - */
	"cc",	/* - cc - */
	"cd",	/* - cd - */
	"ce",	/* - ce - */
	"cf",	/* - cf - */
	"d0",	/* - d0 - */
	"d1",	/* - d1 - */
	"d2",	/* - d2 - */
	"d3",	/* - d3 - */
	"d4",	/* - d4 - */
	"d5",	/* - d5 - */
	"d6",	/* - d6 - */
	"d7",	/* - d7 - */
	"d8",	/* - d8 - */
	"d9",	/* - d9 - */
	"da",	/* - da - */
	"db",	/* - db - */
	"dc",	/* - dc - */
	"dd",	/* - dd - */
	"de",	/* - de - */
	"df",	/* - df - */
	"e0",	/* - e0 - */
	"e1",	/* - e1 - */
	"e2",	/* - e2 - */
	"e3",	/* - e3 - */
	"e4",	/* - e4 - */
	"e5",	/* - e5 - */
	"e6",	/* - e6 - */
	"e7",	/* - e7 - */
	"e8",	/* - e8 - */
	"e9",	/* - e9 - */
	"ea",	/* - ea - */
	"eb",	/* - eb - */
	"ec",	/* - ec - */
	"ed",	/* - ed - */
	"ee",	/* - ee - */
	"ef",	/* - ef - */
	"f0",	/* - f0 - */
	"f1",	/* - f1 - */
	"f2",	/* - f2 - */
	"f3",	/* - f3 - */
	"f4",	/* - f4 - */
	"f5",	/* - f5 - */
	"f6",	/* - f6 - */
	"f7",	/* - f7 - */
	"f8",	/* - f8 - */
	"f9",	/* - f9 - */
	"fa",	/* - fa - */
	"fb",	/* - fb - */
	"fc",	/* - fc - */
	"fd",	/* - fd - */
	"fe",	/* - fe - */
	"ff",	/* - ff - */
};

/*--------------------------------------------------*/

char 
getandocommands()
{
	int commi;

	printf("Wadaya want? ");
	while ((commi = getline(tinput, 80)) != 0) {

		/*
		 * eat test commands
		 */

		tindex = 0;
		tinput[commi-1] = '\0';	/* blast the \n for strcmp */
		commi = lookee();	/* check command name */
		doit[commi].do_do();	/* do the command */

		printf("\nnow what? ");	/* get next input */
	}
	return;
}

/*----------------------------------------------------------------*/

/*
 * lookee()
 *	this routine looks for keywords and their matching functions
 *	in the doit struct 
 */

lookee()
{
	int i;
	char arg[16];

	getarg(arg, sizeof(arg));

	for (i = 0; doit[i].name != 0; i++)
		if ( strcmp8(arg, doit[i].name) == 0 )
			 break;
	return(i);
}

getarg(arg, len)
	char arg[];
	int len;
{
	int i;

	for(i=0; i < len; i++) {
		*arg = tinput[tindex++];
		if(*arg == '\0'){
			tindex--;
			break;
		}
		if(*arg == '\n' || *arg == ' ') {
			*arg = '\0';
			break;
		}
		arg++;
	}
}

/*--------------------------------------------------*/

/*
 * Compare 8 char strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

strcmp8(s1, s2)
	register char *s1, *s2;
{
	short i;
	i = 0;

	while (*s1 == *s2++) {
		i++;
		if (*s1++=='\0' || i == 8)
			return(0);
	}
	return(*s1 - *--s2);
}

/*--------------------------------------------------*/

getline(s, lim)	
	char s[];
	int lim;
{
	int c, i = 0;

	--lim;
	while(--lim > 0 && (c = getchar()) != EOF && c != '\n')
		s[i++] = c;
	s[i++] = '\n';
	s[i] = '\0';
	return(i);
}

char
get1char()
{
	char	cbuf[32];

	(void) getline(cbuf, sizeof(cbuf));
	return(cbuf[0]);
}

/*--------------------------------------------------*/

do_do_help()		/* help - list the commtab */
{
	struct commtab *commname;
	short i;

	printf("--------- command table so far ----------\n");
	commname = doit;
	while(commname->name != 0) {
		for (i = 0; i < 8; i++) {
			if(commname->name == 0) break;
			printf("%-10s", commname->name);
			commname++;
		}
		printf("\n");
	}
	return(0);
}

/*--------------------------------------------------*/

/*
 * do_do_count()
 *	quick local packet counter that clears a counter
 *	so packet traffic can be determined
 */

do_do_count()
{
	printf("counter value %d - now cleared\n", counter);
	counter = 0;
	bytes_kept = 0;
	return(0);
}

/*----------------------------------------------------*/

do_do_bkeep()
{
	if(!bkeep) {
		bkeep = ON;
		printf("bkeep now ON - keep broadcasts packets\n");
	}else{
		bkeep = OFF;
		printf("bkeep now OFF - do not keep based on bcast\n");
	}
	return(0);
}

/*----------------------------------------------------*/

do_do_reverse ()
{
	if(reverse == 1) {
		reverse = -1;
		printf("reverse now LIFO\n");
	}else{
		reverse = 1;
		printf("reverse now FIFO\n");
	}
}

/*----------------------------------------------------*/

do_do_dumpit()
{
	int dumpfrom, dumpto;
	int c, turnedoff;
	char arg[16];

	turnedoff = 0;
	if(intson) {
		spl7();
		intson = OFF;
		turnedoff = ON;
		printf("INTSOFF:\n");
	}
	getarg(arg, 16);

	/*
	 * setup to automagically avoid printit (y/n) question at end of
	 * printing out a packet - can specify -y or -n to print or not print
	 * raw packet info
	 */

	autoprint = ON;
	savechar = 'y';
	if(arg[0] == '-')
	{
		savechar = arg[1];
		getarg(arg, 16);	/* get the dumpfrom argument */
	}
	dumpfrom = atoi(arg);
	if(dumpfrom > WNUMBUFS) {
		printf("WRONGO\n");
		dumpfrom = -1;
	}
	getarg(arg, 16);
	dumpto = atoi(arg);
	if(dumpto > WNUMBUFS) {
		printf("WRONGO\n");
		dumpto = -1;
	}
	
	if(dumpfrom >= 0 && dumpto >= 0) {

		dumpit = ON;
		printf("dump from %d to %d\n", dumpfrom, dumpto);

		for(c = dumpfrom; c != dumpto; c++, c %= WNUMBUFS)
			(void) printpacket(c);

		(void) printpacket(c);
	}
	if(turnedoff){
		spl0();
		intson = ON;
		printf("INTSON:\n");
	}

	do_do_autoprint();

	if(!dumpit) {
		dumpit = ON;
		printf("dumpit now ON\n");
	}else {
		dumpit = OFF;
		printf("dumpit now OFF\n");
	}
	return(0);
}

/*----------------------------------------------------*/

do_do_autoprint()
{
	if(!autoprint) {
		autoprint = ON;
		printf("autoprint now ON\n");
	}else {
		autoprint = OFF;
		printf("autoprint now OFF\n");
	}
	return;
}

/*----------------------------------------------------*/

do_do_huh()	/*** command unknown **/
{
	printf("huh?\n");
	return(0);
}

/*----------------------------------------------------*/

translate(s, n)
	u_char	*s;
	int	n;
{
	printf("\n\t");
	for(; n>0; n--)
		printf("%s ", ttab[*s++]);
	printf("\n");
	return;
}

do_do_k()
{
	printf("\t\t\tpackets kept = %d,\t%d bytes\n", counter, bytes_kept);
	return(0);
}
