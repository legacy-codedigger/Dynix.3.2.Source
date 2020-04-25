
DESTDIR	= $(ATT_DESTDIR)

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

# $Header: Makefile 1.2 87/08/12 $
#
FPA	=
FPFLAGS	=
ALL	=	libplot lib4014 lib300 lib300s lib450 libvt0
FPALL	=	libfpa4014 libfpa300 libfpa300s libfpa450 libfpavt0
SUBDIRS	=	plot t4014 t300 t300s t450 vt0
FP_OBJS	= t300/arc.o t300/erase.o t300/label.o t300/line.o t300/open.o \
	t300/space.o t300/subr.o t300s/arc.o t300s/erase.o t300s/label.o \
	t300s/line.o t300s/open.o t300s/space.o t300s/subr.o t4014/arc.o \
	t4014/space.o t4014/subr.o t450/arc.o t450/erase.o t450/label.o \
	t450/line.o t450/open.o t450/space.o t450/subr.o vt0/space.o \
	vt0/subr.o

all		: $(MACHINE)all

ns32000all	: libs
i386all		: rm_fp_objs libs
	make $(MFLAGS) $(MRULES) FPA="fpa" FPFLAGS="-f1167" rm_fp_objs libs

libs		:&	${SUBDIRS}

${SUBDIRS}	: FRC
	cd $@; make ${MFLAGS} ${MRULES} FPA="$(FPA)" FPFLAGS="$(FPFLAGS)"

rm_fp_objs	:; rm -f $(FP_OBJS); sleep 5
FRC:

install		: $(MACHINE)install

ns32000install	:
	make $(MFLAGS) $(MRULES) install_template

i386install	:
	make $(MFLAGS) $(MRULES) check_387 install_template
	make $(MFLAGS) $(MRULES) check_1167
	for i in ${FPALL}; do \
		install -m 644 $$i ${DESTDIR}/usr/lib/$${i}.a; \
		${RANLIB} ${DESTDIR}/usr/lib/$${i}.a; \
		install -m 644 $${i}_p ${DESTDIR}/usr/lib/$${i}_p.a; \
		${RANLIB} ${DESTDIR}/usr/lib/$${i}_p.a; \
	done

install_template:
	for i in ${ALL}; do \
		install -m 644 $$i ${DESTDIR}/usr/lib/$${i}.a; \
		${RANLIB} ${DESTDIR}/usr/lib/$${i}.a; \
		install -m 644 $${i}_p ${DESTDIR}/usr/lib/$${i}_p.a; \
		${RANLIB} ${DESTDIR}/usr/lib/$${i}_p.a; \
	done

clean:
	rm -f ${ALL} ${FPALL} *_p errs a.out core
	for i in ${SUBDIRS}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} clean); done

#
#   Make sure that libraries contain pure code, ie just 387 or just 1167
#
check_387	:
	$(NM) ${ALL} | awk '/_1167_flt/ { print; exit 1 } END { exit 0 }'
check_1167	:
	$(NM) ${FPALL} | awk '/_387_flt/ { print; exit 1 } END { exit 0 }'
