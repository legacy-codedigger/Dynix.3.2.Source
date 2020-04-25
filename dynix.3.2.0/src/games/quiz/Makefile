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

# $Header: Makefile 2.1 86/05/07 $
#
CFLAGS=	-O ${INCLUDE}
#

all:	quiz

quiz:	quiz.c
	${CC} ${CFLAGS} -o quiz quiz.c

install:
	install -s quiz $(DESTDIR)/usr/games/quiz
	rm -rf $(DESTDIR)/usr/games/lib/quiz.k
	umask 022 && mkdir $(DESTDIR)/usr/games/lib/quiz.k
	cd quiz.k &&  \
		install -c -m 644 * $(DESTDIR)/usr/games/lib/quiz.k

clean:
	rm -f a.out core *.s *.o quiz

depend:
	cat </dev/null >x.c
	for i in quiz; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
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

quiz: quiz.c

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
