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

# $Header: /usr/src/dynix.3.2.0/src/lib/libc/compat-sys5/RCS/Makefile,v 1.3 1993/01/19 12:13:06 bruce Exp $
#
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(FPFLAGS)
#
OBJS	= memccpy.o memchr.o memcmp.o memcpy.o memset.o strcatn.o \
	strchr.o strcmpn.o strcpyn.o strcspn.o strpbrk.o strrchr.o \
	strspn.o strtok.o tmpnam.o \
	$(ADDED)
ADDED	= getopt.o \
	strstr.o strtod.o strtol.o

SED	= sed -f ../$(MACHINE)/rofix.sed
LIB	= compat-sys5lib compat-sys5lib_p

.c.o:
	$(CC) -SO -p -DPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../compat-sys5lib_p $(OBJS) && \
		  echo "	buiding normal $@" && \
		  cd .. && $(AR) cru compat-sys5lib $(OBJS)

parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean		:; rm -rf compat-sys5lib compat-sys5lib_p profiled $(OBJS)
