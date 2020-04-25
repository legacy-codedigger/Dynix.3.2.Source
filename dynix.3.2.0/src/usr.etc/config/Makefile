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

# $Header: Makefile 2.2 87/01/06 $
#
CFLAGS	= -O $(INCLUDE) $(DEF) $(CCS) $(DEBUG)
#
OBJS	= y.tab.o main.o lex.yy.o mkioconf.o mkmakefile.o mkubglue.o \
	  mkheaders.o mkswapconf.o
CMD	= config
TARGET	= $P$N$(CMD)
M	= make $(MFLAGS) $(MRULES)
C	= CCS=-DCCS=1 N=ccs
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
all		:  y.tab.h $(TARGET)
$(TARGET)	:& $(OBJS); $(CC) $(CFLAGS) -o $@ $(OBJS) -ll; rm -f DEFS
y.tab.h		:; $(YACC) -d config.y
y.tab.c		:  config.y; $(YACC) config.y
lex.yy.c	:  config.l; lex config.l
clean		:; rm -f $(OBJS) lex.yy.c y.tab.c y.tab.h
clobber		:; rm -f $(OBJS) lex.yy.c y.tab.c y.tab.h DEFS *config
install		:; install -s config $(DESTDIR)/usr/etc/config
		   ln -s ../usr/etc/config $(DESTDIR)/etc/config
FRC		:

y.tab.o		: config.h
main.o		: y.tab.h config.h
lex.yy.o	: y.tab.h config.h
mkioconf.o	: y.tab.h config.h
mkmakefile.o	: y.tab.h config.h
mkubglue.o	: config.h y.tab.h
mkheaders.o	: config.h y.tab.h
mkswapconf.o	: config.h
