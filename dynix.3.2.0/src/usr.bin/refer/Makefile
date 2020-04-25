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

# $Header: Makefile 2.2 86/06/25 $
#
CFLAGS	= -w -O $(INCLUDE)
OBJS	=\
	addbib.o deliv1.o deliv2.o glue1.o glue2.o glue3.o glue4.o glue5.o\
	hunt1.o hunt2.o hunt3.o hunt5.o hunt6.o hunt7.o hunt8.o hunt9.o inv1.o\
	inv2.o inv3.o inv5.o inv6.o lookbib.o mkey1.o mkey2.o mkey3.o refer0.o\
	refer1.o refer2.o refer3.o refer4.o refer5.o refer6.o refer7.o refer8.o\
	shell.o sortbib.o tick.o what1.o what2.o what3.o what4.o

all		:  objects cmds
objects		:& $(OBJS)
cmds		:& mkey inv hunt refer addbib lookbib sortbib papers
mkey		:  mkey1.o mkey2.o mkey3.o deliv2.o
		   $(CC) -o mkey mkey?.o deliv2.o
inv		:  inv1.o inv2.o inv3.o inv5.o inv6.o deliv2.o
		   $(CC) -o inv inv?.o deliv2.o
hunt		:  hunt1.o hunt2.o hunt3.o hunt5.o hunt6.o hunt7.o glue5.o \
		   refer3.o hunt9.o shell.o deliv2.o hunt8.o glue4.o tick.o
		   $(CC) -o hunt hunt?.o refer3.o glue5.o glue4.o shell.o deliv2.o tick.o
refer		:  glue1.o refer1.o refer2.o refer4.o refer5.o refer6.o mkey3.o \
		   refer7.o refer8.o hunt2.o hunt3.o deliv2.o hunt5.o hunt6.o hunt8.o \
		   glue3.o hunt7.o hunt9.o glue2.o glue4.o glue5.o refer0.o shell.o
		   $(CC) -o refer glue?.o refer[01245678].o hunt[2356789].o \
		   mkey3.o shell.o deliv2.o
addbib		:  addbib.o; $(CC) -o addbib addbib.o
lookbib		:  lookbib.o; $(CC) -o lookbib lookbib.o
sortbib		:  sortbib.o; $(CC) -o sortbib sortbib.o
papers		:; /usr/lib/refer/mkey Rbstjissue Rv7man | /usr/lib/refer/inv -v -h997 -n Xind
# NOTE: tmac.bib not installed because already done from troff/tmac/tmac.bib
install		:; install -s mkey inv hunt $(DESTDIR)/usr/lib/refer
		   install -s refer addbib sortbib lookbib $(DESTDIR)/usr/bin
		   install -c roffbib.sh $(DESTDIR)/usr/bin/roffbib
		   install -c indxbib.sh $(DESTDIR)/usr/bin/indxbib
		   install -c -m 644 Rbstjissue Rv7man $(DESTDIR)/usr/dict/papers
		   install -c runinv $(DESTDIR)/usr/dict/papers
		   install -m 644 Xind.ia $(DESTDIR)/usr/dict/papers/Ind.ia
		   install -m 644 Xind.ib $(DESTDIR)/usr/dict/papers/Ind.ib
		   install -m 644 Xind.ic $(DESTDIR)/usr/dict/papers/Ind.ic
clean		:; rm -f refer inv hunt mkey addbib lookbib sortbib \
		   $(OBJS) Xind.ia Xind.ib Xind.ic
whatabout	:  what1.o what2.o what3.o what4.o shell.o mkey3.o
		   $(CC) -o whatabout what?.o shell.o mkey3.o
deliv		: deliv1.o deliv2.o; $(CC) -o deliv deliv?.o
refpart		: refer0.o refer1.o refer2.o refer3.o refer4.o refer5.o \
		  refer6.o refer7.o refer8.o deliv2.o glue4.o
		  $(CC) -o refpart refer?.o deliv2.o glue4.o
glue3.o		:  refer..c
hunt2.o		:  refer..c
hunt3.o		:  refer..c
refer0.o	:  refer..c
refer1.o	:  refer..c
refer2.o	:  refer..c
refer3.o	:  refer..c
refer4.o	:  refer..c
refer6.o	:  refer..c
refer5.o	:  refer..c
