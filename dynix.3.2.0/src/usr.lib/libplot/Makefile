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

# $Header: Makefile 2.1 87/03/30 $
#
FPA	=
LIB	= lib$(FPA)4014 lib$(FPA)300 lib$(FPA)300s lib$(FPA)450 lib$(FPA)vt0
DIRS	= plot.dir t4014.dir t300.dir t300s.dir t450.dir vt0.dir

all			:& $(DIRS)
$(DIRS)			:; cd `basename $@ .dir` && make $(MFLAGS) $(MRULES)
clean			:
	rm -f libplot $(LIB) *fpa* *_p
	for i in $(DIRS); do \
		(cd `basename $$i .dir` && make $(MFLAGS) $(MRULES) clean); \
	done

install			: $(MACHINE)install
	for i in libplot; do \
		install -m 644 $$i $(DESTDIR)/usr/lib/$${i}.a; \
		$(RANLIB) $(DESTDIR)/usr/lib/$${i}.a; \
		install -m 644 $${i}_p $(DESTDIR)/usr/lib/$${i}_p.a; \
		$(RANLIB) $(DESTDIR)/usr/lib/$${i}_p.a; \
	done
install_template	:
	for i in $(LIB); do \
		install -m 644 $$i ${DESTDIR}/usr/lib/$${i}.a; \
		${RANLIB} ${DESTDIR}/usr/lib/$${i}.a; \
		install -m 644 $${i}_p ${DESTDIR}/usr/lib/$${i}_p.a; \
		${RANLIB} ${DESTDIR}/usr/lib/$${i}_p.a; \
	done

ns32000install		:
	make $(MFLAGS) $(MRULES) install_template

i386install		:
	make $(MFLAGS) $(MRULES) check_387 install_template
	make $(MFLAGS) $(MRULES) FPA="fpa" check_1167 install_template

#
#  Check the see libraries are pure (only 387 or only 1167 code)
#
check_387		:
	$(NM) $(LIB) | awk '/_1167_flt/ { print; exit 1 } END { exit 0 }'
check_1167		:
	$(NM) $(LIB) | awk '/_387_flt/  { print; exit 1 } END { exit 0 }'
