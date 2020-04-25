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
static char rcsid[] = "$Header: bootflags.c 2.1 87/01/09 $";
#endif

/*
 * bootflags -- show/change reboot structure
 *
 * Usage:
 *	bootflags [-p] [-c] [-v] [flag=value] ...
 *
 * Flags:
 *	-p	write permanent flags instead of temps
 *	-v	show flags after they're changed
 *	-c	copy permanent->temp if -p, otherwise temp->permanent
 *
 * the assignments are:
 *
 *	f=	set re_boot_flags
 *	ra=	set re_cfg_addr[0]
 *	ra0=	set re_cfg_addr[0]
 *	ra1=	set re_cfg_addr[1]
 *	n=	set re_boot_name[0]
 *	n0=	set re_boot_name[0]
 *	n1=	set re_boot_name[1]
 *
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <machine/cfg.h>

#define	TRUE	1
#define FALSE	0
#define eq(x,y)	(!strcmp(x,y))

/*
 * things you can do
 */
#define ILLEGAL		-1		/* illegal command */
#define BOOTNAME0	0		/* change re_boot_name[0] */
#define BOOTNAME1	1		/* change re_boot_name[1] */
#define FLAG		2		/* change re_flag */
#define CFGADDR0	3		/* change re_cfg_addr[0] */
#define CFGADDR1	4		/* change re_cfg_addr[1] */

/*
 * globals
 */
int verbose_flag = FALSE;		/* set if -v flag */
int permanent_flag = FALSE;		/* set if -p flag */
int copy_flag = FALSE;			/* set if -c flag */
int something = FALSE;			/* if something to set */
int sec;				/* file descriptor for /dev/smem */
int command;				/* current command to do */
char *value;				/* current value for command */

char *console_sec();
char *index();
extern errno;

main(argc, argv)
	int argc;
	char **argv;
{

	argc--; argv++;

	parse_command_line(argc, argv);

	sec = open(console_sec(), 0, 0);
	if (sec < 0) {
		perror(console_sec());
		exit(errno);
	}

	execute_command_line(argc, argv);

	if (copy_flag) {
		copy();
		something = TRUE;
	}

	if (!something || verbose_flag)
		show();

	close(sec);
	exit(0);
}

parse_command_line(argc, argv)
	int argc;
	char **argv;
{
	register char *cp;

	while (argc--) {
		cp = *argv;
		if (*cp == '-') 
			while (*(++cp) != '\0')
				switch(*cp) {
				case 'p':
					permanent_flag = TRUE;
					break;
				case 'v':
					verbose_flag = TRUE;
					break;
				case 'c':
					copy_flag = TRUE;
					break;
				default:
					fprintf(stderr, 
 			"Usage: bootflags [-p] [-c] [-v] [flag=value] ...\n");
					exit(1);
				}
		argv++;
	}
}


execute_command_line(argc, argv)
	int argc;
	char **argv;
{
	register char *cp;
	register char *rp;
	struct ioctl_reboot rb;
	int ioctl_r_cmd, ioctl_w_cmd;

	while (argc) {
		if (**argv == '-')
			goto skip;

		/*
		 * figure out what the command is
		 */
		parse(*argv);
		if (command == ILLEGAL) {
			printf("illegal command: '%s'\n", *argv);
			goto skip;
		}
		something = TRUE;

		/*
		 * get the correct half of the structure
		 */
		if (command == BOOTNAME1 || command == CFGADDR1) {
			ioctl_r_cmd = SMIOGETREBOOT1;
			ioctl_w_cmd = SMIOSETREBOOT1;
		} else {
			ioctl_r_cmd = SMIOGETREBOOT0;
			ioctl_w_cmd = SMIOSETREBOOT0;
		}
		
		if (permanent_flag)
			rb.re_powerup = 1;
		else
			rb.re_powerup = 0;

		/*
		 * get the current values
		 */
		Ioctl(sec, ioctl_r_cmd, &rb);

		/*
		 * change what's specified to be changed
		 */
		switch(command) {
		case BOOTNAME0:
		case BOOTNAME1:
			/*
			 * copy value to bootname, translating spaces
			 * to nulls.  pad with nulls.
			 */
			for (cp=value, rp = rb.re_boot_name; *cp; cp++, rp++)
				if (*cp == ' ')
					*rp = '\0';
				else
					*rp = *cp;
			while(rp < &rb.re_boot_name[BNAMESIZE])
				*rp++ = '\0';
			break;

		case FLAG:
			if (strncmp(value, "0x", 2) == 0)
				rb.re_boot_flag = gethex(value);
			else
				rb.re_boot_flag = atoi(value);
			break;

		case CFGADDR0:
		case CFGADDR1:
			if (strncmp(value, "0x", 2) == 0)
				rb.re_cfg_addr = (unsigned char *)gethex(value);
			else
				rb.re_cfg_addr = (unsigned char *)atoi(value);
			break;
		}

		/*
		 * stuff it back
		 */
		Ioctl(sec, ioctl_w_cmd, &rb);
		
		/*
		 * go to next command
		 */
skip:		argv++;
		argc--;
	}
}

/*
 * decide what command is in the string s.
 */
parse(s)
	char *s;		/* command to parse */
{
	register char *commandname;

	value = "";

	/*
	 * find start of command name
	 */
	while (*s == ' ')
		s++;
	commandname = s;
	if (*s == '\0') {
		command = ILLEGAL;
		return;
	}

	/*
	 * find end of command name, and stuff a null there to 
	 * terminate it.
	 */
	while ( *s != ' '  &&  *s != '='  &&  *s != '\0' )
		s++;
	if (*s == '\0') {
		command = ILLEGAL;
		return;
	}
	*s++ = '\0';

	/*
	 * find beginning of command text
	 */
	while ( *s == ' '  ||  *s == '=' )
		s++;
	value = s;

	/*
	 * figure out what command
	 */
	if (*commandname == 'w')
		commandname++;
	if ( eq(commandname, "n") || eq(commandname, "n0") )
		command = BOOTNAME0;
	else if ( eq(commandname, "n1") )
		command = BOOTNAME1;
	else if ( eq(commandname, "f") )
		command = FLAG;
	else if ( eq(commandname, "ra") || eq(commandname, "ra0") )
		command = CFGADDR0;
	else if ( eq(commandname, "ra1") )
		command = CFGADDR1;
	else
		command = ILLEGAL;
}

show()
{
	struct ioctl_reboot rb0;	/* reboot structure from sec */
	struct ioctl_reboot rb1;	/* reboot structure from sec */

	if (permanent_flag)
		rb0.re_powerup = rb1.re_powerup = 1;
	else
		rb0.re_powerup = rb1.re_powerup = 0;

	Ioctl(sec, SMIOGETREBOOT0, &rb0);
	Ioctl(sec, SMIOGETREBOOT1, &rb1);
	nulls_to_spaces(rb0.re_boot_name, BNAMESIZE);
	printf("n0  = %s\n", rb0.re_boot_name);
	nulls_to_spaces(rb1.re_boot_name, BNAMESIZE);
	printf("n1  = %s\n", rb1.re_boot_name);
	printf("f   = 0x%x\n", rb0.re_boot_flag);
	printf("ra0 = 0x%x\n", rb0.re_cfg_addr);
	printf("ra1 = 0x%x\n", rb1.re_cfg_addr);
}

/*
 * if -p flag, copy from permanent flags into temp flags.
 * if no -p flag, copy from temps into permanent.
 */
copy()
{
	struct ioctl_reboot rb0;	/* reboot structure from sec */
	struct ioctl_reboot rb1;	/* reboot structure from sec */

	/*
	 * read permanents if -p; temps if not
	 */

	if (permanent_flag)
		rb0.re_powerup = rb1.re_powerup = 1;
	else
		rb0.re_powerup = rb1.re_powerup = 0;

	Ioctl(sec, SMIOGETREBOOT0, &rb0);
	Ioctl(sec, SMIOGETREBOOT1, &rb1);

	/* 
	 * write the other one
	 */

	rb0.re_powerup = rb1.re_powerup = 1 - rb0.re_powerup;

	Ioctl(sec, SMIOSETREBOOT0, &rb0);
	Ioctl(sec, SMIOSETREBOOT1, &rb1);
}

char *
console_sec()
{
	return("/dev/smemco");
}

nulls_to_spaces(s, len)
	char *s;
{
	register char *cp;
	register int i;

	for (i=len-1, cp = &s[len-1]; i>0 && *cp=='\0'; i--, cp--)
		continue;

	for (; i > 0; i--, cp--)
		if (*cp == '\0')
			*cp = ' ';
}

Ioctl(d, request, argp)
	int d;
	int request;
	char *argp;
{
	if (ioctl(d, request, argp) == -1) {
		perror(console_sec());
		exit(1);
	}
}

gethex(p)
	register char *p;
{
	register n = 0;

	if (strncmp(p, "0x", 2) == 0)
		p += 2;
	while ( (*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ) {
		if (*p >= '0' && *p <= '9')
			n = n*16 + *p++ - '0';
		else
			n = n*16 + *p++ - 'a' + 10;
	}
	return (n);
}
