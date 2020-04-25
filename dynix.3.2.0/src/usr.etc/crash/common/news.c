/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred. */

#ifndef lint
static char rcsid[] = "$Header: news.c 2.3 1991/04/19 21:01:37 $";
#endif

#include "crash.h"

char *newspaper[] = {
"Version \"2.4\"",
"	Removed extraneous debugging from getsym().",
"	Added ability to print locked files",
"	usage:",
"		inode -l",
"",
"Version \"2.3\"",
"	Fixed file redirection so that \". foo | cat >bar\" works.",
"	(supports nesting as well)",
"	vnodes now print dev for block and char.",
"	u now prints open file table.",
"	Added Every",
"	usage:",
"		Every n",
"	Added \"<string>\" and '@'",
"	usage:",
"		eg \"tp? \" set TP=@",
"",
"Version \"2.0\"",
"	Added disassembly",
"	usage:",
"		dis addr Ninstrs",
"",
"Version \"1.11\"",
"	Enhanced the stack tracing a bit.",
"",
"Version \"1.10\"",
"	The \"set\" command has been added to allow dynamic creation",
"	of variables.  Such variables maintain any SDB style information",
"	available at the time of assignment.",
"	The engine display now prints out the proc slot of the panic'ed",
"	process (if available).",
"	The default format for dump commands is now hex.",
"	The stack traces are less verbose, always guessed, and more usable.",
"	A bug in the decoding of trap types has been fixed.",
"	A bug that disallowed digits in variable names has been fixed.",
"",
"Version \"1.09\"",
"	Crash now knows about -g sdb style symbolics.  This means",
"	expressions can now be complex like \"proc[2].p_uid\"",
"	(the expressions stuff is new and not totally debugged yet)",
"	Crash now reads symbols much faster even though there is twice",
"	as many symbols to process.  The dump command now supports the",
"	's' (string) format.  An expression can now be used almost anywhere",
"	a number would be before.  Expressions are expanded by atoi().  Also,",
"	the notion of '.' is maintained where appropriate so you can use it",
"	instead of repeating an expression.  Commands can now be put into a",
"	script file and sourced by using the '. file' command.",
"	The time of the crash is displayed if appropriate.",
"	The data structure panic_data is now used.  A double panic is noted.",
"",		
"Version \"1.08\"",
"	Added /dev/drum so live systems the U-area is gotten from",
"	the swap device.  Also, the proc display uses ps notation",
"	on displaying a swapped process ('W' flag).",
"	Crash now catches SIGPIPE and correctly handles it.",
"	Minor changes in several display formats.",
"	Stack tracing is more robust and displays register contents",
"	if system panic'ed.   Heuristic stack trace no longer looks",
"	for longest fp linkage but requires it start within 64 bytes",
"	of top of kernel stack.  This eliminates some longer but",
"	invalid linkages from being displayed.",
"",
"Version \"1.07\"",
"	Sanity check for size of Text in a.out",
"	Catches SIGSEGV now and prints 'Ouch!'",
"	'!!' echo's command (to stderr if piped output",
"	'version' is compared in /dynix and /vmcore for mismatch",
"	Reads from below &etext come from /dynix not /vmcore",
"	Trace command added to help debug crash itself",
"	Engine init routine notices if some engines are offline",
"	Stack tracing all changed to do a search for the longest fp",
"	linkage and display that if reasonable",
"	'stack -e #' now works",
"",
"Version \"1.06\"",
"	A live system is now detected so that tables like Sysmap, proc, and",
"	engine, are read per request not just one at startup time.",
"	Commands now only need to be specified far enough to uniquely",
"	identify them.",
"	Went crazy and added '!!' to repeat last command..:-)..",
"	Wait event of 'p' command (wchan) is symbolic if kernel symbol.",
"	If panicstr is not NULL, its value is displayed at startup.",
"	Engine display does symbolic decode of FLAG and STATE fields.",
"	Engine display tacks on '[paniceng]' if this engine is paniceng.",
"	Command line arguments (vmcore && dynix) can now be in any order.",
"	Beefed up the usage message a bit.",
"",
"Version \"1.04\"",
"	Fixed bug in parsing of commands.",
"",
"Version \"1.02\"",
"	Version of crash is now printed at startup time.",
"",
"	Command parser now allows '|' to appear anywhere on a line.",
"	The rest of command is piped to the shell.  Two really useful",
"	examples are:   p|egrep xxx   and   p | more (or news|more)",
"",
"	Added news command to help document changes as time goes on.",
"	Just type news when in crash.",
"",
"	Mail comments to \"phil\".",
	0, };

news()
{
	register char **n;

	for (n=newspaper; *n; n++)
		printf("%s\n", *n);
}
