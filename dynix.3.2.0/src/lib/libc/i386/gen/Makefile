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

# $Header: Makefile 1.11 87/06/02 $
#
# _ppinit.o is a Sequent addition.
#
FPFLAGS	=
CFLAGS	= -O -I. -I.. $(INCLUDE) $(FPFLAGS)

# Reorder suffix list so .s's are found before .c's
.SUFFIXES:
.SUFFIXES: .out .o .s .c .e .r .f .y .l .p

OBJS	=\
	alloca.o abort.o abs.o bcmp.o bcopy.o bzero.o\
	disktab.o ecvt.o fabs.o ffs.o fpvalues.o frexp.o index.o insque.o\
	nargs.o remque.o rindex.o strcat.o strcmp.o\
	strcpy.o strlen.o strncat.o strncmp.o strncpy.o udiv.o urem.o\
	_ppinit.o _profbrk.o atof.o ldexp.o modf.o _longjmp.o _setjmp.o\
	longjmp.o setjmp.o
# due to various problems
SED	= sed -f ../rofix.sed
LIB	= genlib

.s.o:
	$(CPP) -DPROF -I. -I.. $(INCLUDE) $(FPFLAGS) $*.s | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CPP) -UPROF -I. -I.. $(INCLUDE) $(FPFLAGS) $*.s | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o
.c.o:
	$(CC) -SO -p -DPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../$(LIB)_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru $(LIB) $(OBJS)

parallel	:& $(OBJS)
$(OBJS)		: ../DEFS.h
setjmp.o 	: Setjmp.h
_setjmp.o 	: Setjmp.h
longjmp.o 	: Setjmp.h SYS.h
_longjmp.o	: Setjmp.h
atof.o 		: values.h
ecvt.o 		: values.h
frexp.o 	: values.h
ldexp.o 	: values.h
modf.o		: values.h

SYS.h		:; ln -s ../sys/SYS.h

# Handled special to make data shared and readonly
fpvalues.o	: fpvalues.c
		  $(CC) -R $(CFLAGS) -c $*.c && cp $*.o profiled/$*.o
profiled	:; mkdir profiled
clean		:; rm -rf $(OBJS) profiled $(LIB) $(LIB)_p SYS.h
