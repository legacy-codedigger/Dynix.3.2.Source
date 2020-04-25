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

# $Header: Makefile 2.0 86/01/28 $ 
#
# Note: merge uses /usr/lib/rdiff3

SRCS=	diff3.c

CFLAGS	      = -O ${INCLUDE}

DEST	      = /usr/lib

EXTHDRS	      =

HEADRS	      =

LDFLAGS	      =

LINKER	      = ${CC}

MAKEFILE      = Makefile

OBJS	      = diff3.o

PRINT	      = pr

PROGRAM	      = rdiff3

all:		$(PROGRAM)

$(PROGRAM):     $(OBJS)
		@echo -n "Loading $(PROGRAM) ... "
		@$(LINKER) $(LDFLAGS) $(OBJS) -o $(PROGRAM)
		@echo "done"

clean:;		@rm -f $(OBJS)

depend:;	@mkmf -f $(MAKEFILE) PROGRAM=$(PROGRAM) ${ENVIRON} DEST=$(DEST)

index:;		@ctags -wx $(HEADRS) $(SRCS)

install:
		@echo Installing $(PROGRAM) in ${DESTDIR}/$(DEST)
		@install -s $(PROGRAM) ${DESTDIR}/$(DEST)

print:;		@$(PRINT) $(HEADRS) $(SRCS)

program:        $(PROGRAM)

tags:           $(HEADRS) $(SRCS); @ctags $(HEADRS) $(SRCS)

update:		${DESTDIR}/$(DEST)/$(PROGRAM)
