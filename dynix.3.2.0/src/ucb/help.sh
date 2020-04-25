#! /bin/sh
# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: help.sh 2.1 86/04/06 $
echo 'Look in a printed manual if you can for general help.  You should
have someone show you some things and then read one of the tutorial papers
(e.g An Introduction to the C Shell) to get started.

The commands:
	man -k keyword		lists commands relevant to keyword
	man command		prints out the manual for a command
are helpful; other basic commands are:
	cat		- concatenates files (and just prints them out)
	ex		- text editor
	finger		- user information lookup program
	ls		- list contents of directory
	mail		- send and receive mail
	msgs            - system messages and junk mail program
	passwd		- change login password
	tset		- set terminal modes
	who		- who is on the system
	write		- write to another user
You could find programs about mail by the command:	man -k mail
And print out the mail command documentation via:	man mail

You can logout by typing a control-d (if your prompt is $)
or by typing ``logout'\'\'' if your prompt is %.'
