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

# $Header: Makefile 2.3 89/07/31 $
#
CFLAGS	= -O $(DEBUG) $(INCLUDE)
OBJS	= cmds.o cmdtab.o ftp.o getpass.o glob.o main.o pclose.o \
			ruserpass.o domacro.o

all		:  ftp
ftp		:& $(OBJS)
		   $(CC) $(CFLAGS) -o ftp $(OBJS)
ftp.o		:  ftp_var.h
cmds.o		:  ftp_var.h
cmdtab.o	:  ftp_var.h
main.o		:  ftp_var.h
install		:; install -s ftp $(DESTDIR)/usr/ucb/ftp
clean		:; rm -f $(OBJS) ftp
