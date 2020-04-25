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
static char rcsid[] = "$Header: vars.c 2.0 86/01/28 $";
#endif

#include "tip.h"

/*
 * Definition of variables
 */
value_t vtable[] = {
	{ "beautify",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "be",		(char *)TRUE },
	{ "baudrate",	NUMBER|IREMOTE|INIT,	(READ<<PUBLIC)|(WRITE<<ROOT),
	  "ba",		(char *)&BR },
	{ "dialtimeout",NUMBER,			(READ<<PUBLIC)|(WRITE<<ROOT),
	  "dial",	(char *)60 },
	{ "eofread",	STRING|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "eofr",	(char *)&IE },
	{ "eofwrite",	STRING|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "eofw",	(char *)&OE },
	{ "eol",	STRING|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  NOSTR,	(char *)&EL },
	{ "escape",	CHAR,			(READ|WRITE)<<PUBLIC,
	  "es",		(char *)'~' },
	{ "exceptions",	STRING|INIT|IREMOTE,	(READ|WRITE)<<PUBLIC,
	  "ex",		(char *)&EX },
	{ "force",	CHAR,			(READ|WRITE)<<PUBLIC,
	  "fo",		(char *)CTRL(p) },
	{ "framesize",	NUMBER|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "fr",		(char *)&FS },
	{ "host",	STRING|IREMOTE|INIT,	READ<<PUBLIC,
	  "ho",		(char *)&HO },
	{ "log",	STRING|INIT,		(READ|WRITE)<<ROOT,
	  NOSTR,	"/usr/adm/aculog" },
	{ "phones",	STRING|INIT|IREMOTE,	READ<<PUBLIC,
	  NOSTR,	(char *)&PH },
	{ "prompt",	CHAR,			(READ|WRITE)<<PUBLIC,
	  "pr",		(char *)'\n' },
	{ "raise",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "ra",		(char *)FALSE },
	{ "raisechar",	CHAR,			(READ|WRITE)<<PUBLIC,
	  "rc",		(char *)CTRL(a) },
	{ "record",	STRING|INIT|IREMOTE,	(READ|WRITE)<<PUBLIC,
	  "rec",	(char *)&RE },
	{ "remote",	STRING|INIT|IREMOTE,	READ<<PUBLIC,
	  NOSTR,	(char *)&RM },
	{ "script",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "sc",		(char *)FALSE },
	{ "tabexpand",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "tab",	(char *)FALSE },
	{ "verbose",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "verb",	(char *)TRUE },
	{ "SHELL",	STRING|ENVIRON|INIT,	(READ|WRITE)<<PUBLIC,
	  NULL,		"/bin/sh" },
	{ "HOME",	STRING|ENVIRON,		(READ|WRITE)<<PUBLIC,
	  NOSTR,	NOSTR },
	{ "echocheck",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "ec",		(char *)FALSE },
	{ "disconnect",	STRING|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "di",		(char *)&DI },
	{ "tandem",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "ta",		(char *)TRUE },
	{ "linedelay",	NUMBER|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "ldelay",	(char *)&DL },
	{ "chardelay",	NUMBER|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "cdelay",	(char *)&CL },
	{ "etimeout",	NUMBER|IREMOTE|INIT,	(READ|WRITE)<<PUBLIC,
	  "et",		(char *)&ET },
	{ "rawftp",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "raw",	(char *)FALSE },
	{ "halfduplex",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "hdx",	(char *)FALSE },
	{ "localecho",	BOOL,			(READ|WRITE)<<PUBLIC,
	  "le",		(char *)FALSE },
	{ "parity",	STRING|INIT|IREMOTE,	(READ|WRITE)<<PUBLIC,
	  "par",	(char *)&PA },
	{ NOSTR, NULL, NULL, NOSTR, NOSTR }
};
