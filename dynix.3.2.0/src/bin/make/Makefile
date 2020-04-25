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

# $Header: Makefile 2.3 86/04/28 $
#
# Description file for the Make command
#
CFLAGS	= -O -DASCARCH $(INCLUDE) $(DEF) $(CCS) $(DEBUG)
#
OBJS	= gram.o ident.o main.o doname.o misc.o files.o dosys.o
CMD	= make
TARGET	= $P$N$(CMD)
M	= make $(MFLAGS) $(MRULES)
C	= CCS=-DCCS=1 N=ccs YACC=yacc
X	= P=x CC=cc "`cat DEFS`"

native		:; $M clean all
nativeccs	:; $M $C clean all
cross		:; $M $X clean all
crossccs	:; $M $X $C clean all
cross crossccs	:  DEFS
everything	:  native nativeccs cross crossccs clean
#################################################################
#  we have to be real careful to not undef something to		#
#  cpp that we may want to define again since it is "sticky"	#
#################################################################
DEFS		:FRC
		  -if [ "$(NATIVE)" != "$(MACHINE)" ]; then \
			echo "DEF=-U$$(NATIVE) -D$$(MACHINE) -Dsequent" >DEFS; \
		  else \
			echo "DEF=" >DEFS ; \
		  fi
all		:  $(TARGET)
$(TARGET)	:& $(OBJS); $(CC) $(CFLAGS) -o $@ $(OBJS); rm -f DEFS
$(OBJS)		:  defs
clean		:; rm -f *.o *.xo gram.c
clobber		:; rm -f *.o *.xo gram.c DEFS *make
install		:; install -s make $(DESTDIR)/bin
FRC		:
