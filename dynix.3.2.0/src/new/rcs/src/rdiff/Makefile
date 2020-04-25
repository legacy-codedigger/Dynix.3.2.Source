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
# Note: RCS uses /usr/new/lib/rdiff and /usr/lib/diffh
#
CFLAGS	= -O -DDIFF='"${DIFF}"' -DDIFFH='"${DIFFH}"' -DPR='"${PR}"' ${INCLUDE}

DEST	= /usr/lib

DIFF	= /bin/diff

DIFFH	= /usr/lib/diffh

EXTHDRS	=

LDFLAGS	=

LINKER	= ${CC}

MAKEFILE= Makefile

OBJS	= diff.o \
	  diffdir.o \
	  diffreg.o

PRINT	= pr

PROGRAM	= rdiff

all:		$(PROGRAM)

$(PROGRAM):     $(OBJS)
		@echo -n "Loading $(PROGRAM) ... "
		@$(LINKER) $(LDFLAGS) $(OBJS) -o $(PROGRAM)
		@echo "done"

clean:;		@rm -f $(OBJS)

depend:;	@mkmf -f $(MAKEFILE) PROGRAM=$(PROGRAM) DEST=$(DEST)

index:;		@ctags -wx $(SRCS)

install:
		@echo Installing $(PROGRAM) in ${DESTDIR}/$(DEST)
		@install -s $(PROGRAM) ${DESTDIR}/$(DEST)

print:;		@$(PRINT) $(SRCS)

program:        $(PROGRAM)

tags:           $(SRCS); @ctags $(SRCS)

update:		$(DEST)/$(PROGRAM)
