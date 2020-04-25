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

# $Header: Makefile 1.2 87/05/11 $
#
CFLAGS=	-O $(INCLUDE)
#

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	biod nfsd ypbind

all:&	$(STD)

$(STD) :
	$(CC) $(CFLAGS) -o $@ $@.c

install:
	install -s $(STD) $(NFS_DESTDIR)/etc

clean:
	rm -f a.out core *.s *.o
	rm -f $(STD)

depend:
	cat </dev/null >x.c
	for i in $(STD) ; do \
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

biod		: biod.c
nfsd		: nfsd.c
ypbind		: ypbind.c

# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
