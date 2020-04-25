
##
# The loader may incorrectly resolves names where one is an external
# function and an external data structure.  In our case it misses the
# coin toss so we resolve the issue be renaming the data structure
#  -- AT&T really should fix this type of bug!!! --
##
LD_BUG	= -Dtmpnam=Tmpnam
DESTDIR	= $(ATT_DESTDIR)
# Removed -DNICE -DNICEVAL=4
CFLAGS	= $(ATT_INCLUDE) $(LD_BUG) -O -DACCT
CC	= $(ATT_CC)

OFILES = setbrk.o blok.o stak.o cmd.o fault.o main.o word.o string.o\
	 name.o args.o xec.o service.o error.o io.o print.o macro.o expand.o\
	 ctype.o msg.o test.o defs.o echo.o hash.o hashserv.o pwd.o func.o

all:	sh

sh:&	$(OFILES)
	$(CC) $(OFILES) -o sh

$(OFILES):	defs.h

ctype.o:	ctype.h

# Make strings readonly and shared
ctype.o msg.o:
	$(CC) -R $(CFLAGS) -c $*.c

service.o:	service.c
	$(CC) $(CFLAGS) -I../acct -c service.c

blok.o fault.o main.o stak.o:	brkincr.h

io.o main.o:	dup.h

xec.o hash.o hashserv.o:	hash.h

cwd.o:	mac.h

cmd.o macro.o main.o msg.o word.o xec.o:	sym.h

main.o:	timeout.h

install:
	rm -f $(DESTDIR)/bin/rsh
	install -s sh $(DESTDIR)/bin/sh
	ln $(DESTDIR)/bin/sh  $(DESTDIR)/bin/rsh

clean clobber:
	  rm -f *.o sh core 
