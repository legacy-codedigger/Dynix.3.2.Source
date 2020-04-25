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

# $Header: Makefile 2.5 90/11/27 $
#
# doprnt is not here because it is machine dependent
#
FPFLAGS	=
CFLAGS	= -O $(INCLUDE) $(FPFLAGS)
#
OBJS	=\
	clrerr.o data.o doscan.o exit.o fdopen.o fgetc.o fgets.o\
	filbuf.o flsbuf.o fopen.o fprintf.o fputc.o fputs.o fread.o freopen.o\
	fseek.o ftell.o fwrite.o getchar.o gets.o getw.o printf.o putchar.o\
	puts.o putw.o rew.o scanf.o setbuf.o setbuffer.o sibuf.o sobuf.o\
	sprintf.o strout.o ungetc.o vfprintf.o vprintf.o vsprintf.o
SED	= sed -f ../$(MACHINE)/rofix.sed
LIB	= stdiolib stdiolib_p

.c.o:
	$(CC) -SO -p -DPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC) -SO    -UPROF $(CFLAGS) -c $*.c | $(SED) | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../stdiolib_p $(OBJS) && \
		  echo "	building normal $@" && \
		  cd .. && $(AR) cru stdiolib $(OBJS)

parallel	:& $(OBJS)
profiled	:; mkdir profiled
clean		:; rm -rf stdiolib stdiolib_p $(OBJS) profiled
