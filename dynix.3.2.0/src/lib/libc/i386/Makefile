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

# $Header: Makefile 1.4 87/03/27 $
#
# Machine dependent routines for the Intel 80386
#
FPFLAGS	=
COMPAT	= compat-4.1
COMPATD	= compat-4.1.dir
ALL	= sys $(COMPAT) gen net stdio
ALLDIR	= sys.dir $(COMPATD) gen.dir net.dir stdio.dir
LIB	= $(MACHINE)lib $(MACHINE)lib_p

all		: $(LIB)
$(LIB)		:& $(ALLDIR)
		  rm -rf tmp; mkdir tmp
		  for i in $(ALL); do (cd tmp; $(AR) x ../$$i/$${i}lib); done
		  $(AR) cru $(MACHINE)lib `ls tmp/*.o | sort -t/ +1`
		  rm -f tmp/*
		  for i in $(ALL); do (cd tmp; $(AR) x ../$$i/$${i}lib_p); done
		  $(AR) cru $(MACHINE)lib_p `ls tmp/*.o | sort -t/ +1`
		  rm -rf tmp

$(ALLDIR)	:; cd `basename $@ .dir` && \
		   make $(MFLAGS) $(MRULES) FPFLAGS="$(FPFLAGS)"
clean clobber	:
		  for i in $(ALL); do \
		  	(cd $$i; make $(MFLAGS) $(MRULES) $@); done
		  rm -rf tmp $(LIB)
