/* $Header: define.h 2.11 1991/07/29 23:26:31 $ */

/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * $Log: define.h,v $
 *
 *
 */

extern int stack(), Upte(), Uarea(), File(), trace(), Inode(), 
	mount(), tty(), cmdsum(), Text(), Proc(), stat(), Kfp(), 
	Bufhdr(), buffer(), tout(), map(), var(), exit(), nop(),
	Eng(), Every(), Dv(), Dp(), news(), expr(), set(), Dis(), Vnode(),
	Vtop(), What(), Debug();
#ifdef _SEQUENT_
extern int getdballoc(), getdbfree(), getdblk(),
	getlinkblk(), getmbfree(), getmess(), getqrun(), getqueue(),
	getstream(), getstrstat(), Snode(), Fnode(), Dev(), Sess();
#endif

struct	tsw	t[] = {
	"D",    "Debug",        Debug,  "change debug settings",
	"dis",	"disassemble",	Dis,	"Disassemble addr count",
	"hdr",	"bufhdr",	Bufhdr, "buffer headers",
#ifndef BSD
	"dba",	"dballoc",	getdballoc,"display dballoc table",
	" ",	"   value(s)",	getdballoc,"dballoc modifier (give class)",
	"dbf",	"dbfree", 	getdbfree,"display free data blocks",
	" ",	"   value(s)",	getdbfree,"dbfree modifier (give class)",
	"dbl",	"dblock", 	getdblk,"display allocated stream dblocks",
	" ",	"   -e",	getdblk,"dblock modifier (all dblocks)",
	" ",	"   -c value(s)",	getdblk,"dblock modifier (give class)",
	" ",	"   value(s)",	getdblk,"dblock identifier (give slot # or addr)",
	"dev",	"dev",		Dev,	"display devsw",
#endif
	"dp",	"dp",		Dp,	"dump physical",
	" ",	" ", 		0,	" i.e. dp addr [count] [dhoxDHOXcs]",
	"dv",	"dv",		Dv,	"dump virtual",
	" ",	" ",		0,	" i.e. dv addr [count] [dhoxDHOXcs]",
#ifdef _SEQUENT_
	"fno",	"fnode",	Fnode,	"fnode table",
#endif
	"f",	"file",		File,	"file table",
	"ino",  "inode",	Inode,	"inode table",
	" ",	"     -l",	Inode,	"list only inodes with locked vnodes",
	"i",	"i",		Inode,	NULL,
	"kfp",	"kfp",		Kfp,	"frame pointer for kernel stack trace start",
	"fp",	"fp",		Kfp,	NULL,
	"every","every",        Every,  "repeat periodically",
	"ex",	"expr",		expr,	"evaluate expressions",
	"e",	"engine",	Eng, 	"display engine tables",
#ifndef BSD
	"link",	"linkblk", 	getlinkblk,"display linkblk table",
	" ",	"   -e",	getlinkblk,"linkblk modifier (all entries)",
	" ",	"   value(s)",	getlinkblk,"linkblk identifier (give slot # or addr)",
#endif
	"map",  "map",		map,	"display resources maps",
#ifndef BSD
	"mbf",	"mbfree", 	getmbfree,"display free stream msgs",
	"mbl",	"mblock", 	getmess,"display allocated stream mblocks",
	" ",	"   -e",	getmess,"mblock modifier (all mblocks)",
	" ",	"   value(s)",	getmess,"mblock identifier (give slot # or addr)",
#endif
	"N",	"News",		news,	"news and features",
	"p",	"proc",		Proc,	"process table (one per line)",
	" ",	"     -l",	Proc,	"process table (long listing)",
	" ",	"     -r",	Proc,	"RUN & ONPROC procs only",
	" ",	"     -o",	Proc,	"ONPROC procs only",
#ifndef BSD
	"qrun",	"qrun", 	getqrun,"display serviceable stream queues",
	"Q",	"Queue", 	getqueue,"display allocated stream queues",
	" ",	"   -e",	getqueue,"Queue modifier (all queues)",
	" ",	"   value(s)",	getqueue,"Queue identifier (give slot # or addr)",
#endif
	"q",	"quit",		exit,	"quit or exit",
#ifndef BSD
	"snode","snode",	Snode,	"snodes (all from inode table)",
	" ",	"   -b",	Snode,	"snode modifier (common VBLK snodes)",
	" ",	"   value",	Snode,	"snode modifier (give inode slot # or addr)",
	"strm",	"stream", 	getstream,"display allocated streams",
	" ",	"   -e",	getstream,"stream modifier (all queues)",
	" ",	"   -f",	getstream,"stream modifier (full display)",
	" ",	"   value(s)",	getstream,"stream identifier (give slot # or addr)",
	"strst","strstat", getstrstat,"display streams statistics",
#endif
	"set",	"set var=expr", set,	"set variable equal to expression value",
	"s",	"stack",	stack,	"kernel stack trace (all procs)",
	" ",	"   -r",	stack,	"kernel stack modifier (display registers)",
	" ",	"   -f",	stack,	"kernel stack modifier (use kfp value)",
	" ",	"   -e value",	stack,	"kernel stack trace (give engine #)",
	" ",	"   value",	stack,	"kernel stack trace (give proc #)",
#ifndef BSD
	"S",	"Session",	Sess,	"session information",
	" ",	"   value(s)",	Sess,	"session information (give proc #)",
#endif
	"t",	"trace",	trace,	"turn on debug tracing",
	"upte",	"upte",		Upte,	"dump user pte's (give proc #)",
	"u",	"user",		Uarea,	"display user area (give proc #)",
	"vnode", "vnode",	Vnode,	"dump a vnode table (sortof)",
	"vtop",	"vtop",		Vtop,	"virtual to physical",
	" ",	"!",		nop,	"escape to shell",
	" ",    "!!",		nop,	"repeat last command",
	" ",	". <file>",	nop,	"source a file",
	"?",	"?",		cmdsum,	"print this list of available commands",
	"h",	"help",		cmdsum, "print this list of available commands",
	"w",	"what",		What,	"interpret addresses, symbols or expressions",
	NULL,		NULL,	NULL
};

struct	prmode	prm[] = {
	"d",		DEC2,
	"D",		DEC4,
	"o",		OCT2,
	"O",		OCT4,
	"x",		HEX2,
	"X",		HEX4,
	"c",		CHAR,
	"b",		BYTE,
	"inode",	INODE,
	"ino",		INODE,
	"directory",	DIRECT,
	"direct",	DIRECT,
	"dir",		DIRECT,
	"write",	WRITE,
	"w",		WRITE,
	"s",		STRING,
	"struct",	STRUCT,
	"sdb",		SDB,
	NULL,		NULL
} ;

extern getsymboltable(), getsysmap(), eng_init(), proc_init(), heuristics();
extern inode_init(), vnode_init(), buffer_init(), file_init(), pre_read();
#ifndef BSD
extern sess_init(), dev_init(), getv(), stream_init();
#endif

struct initfuns initfuns[] = {
	getsymboltable,
	getsysmap,
#ifndef BSD
	getv,
#endif
	eng_init,
	proc_init,
	pre_read,
#ifndef BSD
	sess_init,
#endif
	heuristics,
	inode_init,
	vnode_init,
	buffer_init,
	file_init,
#ifndef BSD
	stream_init,
	dev_init,
#endif
	NULL
};
