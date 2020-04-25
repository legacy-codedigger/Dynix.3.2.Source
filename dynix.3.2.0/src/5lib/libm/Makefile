
DESTDIR	= $(ATT_DESTDIR)
FPA	=
FPFLAGS	=
CFLAGS	= $(ATT_INCLUDE) -O $(FPFLAGS)
# NOTE: the order is important here (at least under systemV)
OBJS	= asin.o atan.o erf.o fabs.o floor.o fmod.o gamma.o hypot.o\
	  jn.o j0.o j1.o pow.o log.o sin.o sinh.o sqrt.o tan.o tanh.o\
	  exp.o matherr.o

.c.o:
	$(CC) -p $(CFLAGS) -c $*.c; \
	$(LD) -X -r $*.o -o profiled/$*.o; \
	$(CC)    $(CFLAGS) -c $*.c; \
	$(LD) -x -r $*.o -o $*.O; mv $*.O $*.o

all:	$(MACHINE)all

ns32000all	: lib
i386all		: rm_fp_objs lib
	make $(MFLAGS) $(MRULES) FPA="fpa" FPFLAGS="-f1167" rm_fp_objs lib

lib		: profiled parallel
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 *.o profiled/*.o
	@-chgrp daemon *.o profiled/*.o > /dev/null 2>&1
	@-chown  root *.o profiled/*.o > /dev/null 2>&1
	@echo "	building profiled lib$(FPA)m.a"
	@cd profiled; $(AR) cru ../lib$(FPA)m_p.a $(OBJS)
	@echo "	building regular lib$(FPA)m.a"
	@$(AR) cru lib$(FPA)m.a $(OBJS)

rm_fp_objs	:; rm -f *.o profiled/*.o

parallel	:& $(OBJS)

profiled	:; mkdir profiled

install		: $(MACHINE)install

ns32000install	:
	make $(MFLAGS) $(MRULES) install_template

i386install	:
	make $(MFLAGS) $(MRULES) install_template
	make $(MFLAGS) $(MRULES) FPA="fpa" FPFLAGS="-f1167" install_template

install_template:
	install -m 644 lib$(FPA)m_p.a $(DESTDIR)/usr/lib/lib$(FPA)m_p.a
	$(RANLIB) $(DESTDIR)/usr/lib/lib$(FPA)m_p.a
	install -m 644 lib$(FPA)m.a $(DESTDIR)/lib/lib$(FPA)m.a
	$(RANLIB) $(DESTDIR)/lib/lib$(FPA)m.a

clean clobber: 
	rm -rf *.a profiled *.o core
