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

# $Header: Makefile 1.3 87/08/25 $
TESTS	= alog asin atan exp pow sin sinh sqrt tan tanh
TESTR	= alog.r asin.r atan.r exp.r pow.r sin.r sinh.r sqrt.r tan.r tanh.r
OBJS	= sqrt.o exp.o alog.o atan.o pow.o asin.o sinh.o tanh.o sin.o tan.o \
	  ran.o randl.o values.o
MATHLIB	= -lm
CFLAGS	= -O -I$(MACHINE)

all	:  objects tests
objects	:& $(OBJS)
tests	:& $(TESTS)
clean	:; rm -f $(OBJS)
clobber	:  clean; rm -f $(TESTS) *.r core

$(TESTS):; $(CC) $(CFLAGS) -o $@ $@.o ran.o randl.o values.o $(MATHLIB)
$(TESTR):; -2>&1 >$*.r $*

test	:  all all.r
all.r	:& $(TESTR)
	   cat $(TESTR) > all.r

regression: clobber test
	  diff all.r $(MACHINE)/all.r
