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

# $Header: Makefile 2.3 90/02/19 $
#
CFLAGS=	-O ${INCLUDE} $(STAND)
#
#####
## Moved ufs_* files to current directory to simplify build process
#####
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c setup.c ufs_subr.c ufs_tables.c utilities.c preen.c
OBJS=	dir.o inode.o main.o pass1.o pass1b.o pass2.o pass3.o pass4.o \
	pass5.o setup.o ufs_subr.o ufs_tables.o utilities.o preen.o
LIBS= -lseq

all:	fsck

standalone:
	rm -f ${OBJS}
	make $(MFLAGS) $(MRULES) STAND=-DSTANDALONE LDFLAGS=-r fsck.o
	rm -f ${OBJS}

fsck:&	${OBJS}
	${CC} ${CFLAGS} -o fsck ${OBJS} ${LIBS}

fsck.o:&	${OBJS}
	${LD} ${LDFLAGS} ${OBJS}  -o $@

install:
	install -s fsck ${DESTDIR}/etc

clean:
	rm -f fsck ${OBJS} errs a.out core

lint:
	lint ${CFLAGS} ${SRCS}

depend:
	echo >x.c
	for i in fsck; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e '/\.\.\/h/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
