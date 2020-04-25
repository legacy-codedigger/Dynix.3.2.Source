
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= -O $(ATT_INCLUDE)
OBJS	=\
	abspath.o alloca.o any.o anystr.o bal.o cat.o clean.o curdir.o dname.o\
	fatal.o fdfopen.o giveup.o imatch.o index.o lockit.o logname.o move.o\
	patoi.o patol.o regcmp.o regex.o rename.o repeat.o repl.o satoi.o\
	setsig.o sname.o strend.o substr.o trnslat.o userdir.o userexit.o\
	username.o verify.o xalloc.o xcreat.o xlink.o xmsg.o xopen.o xpipe.o\
	xunlink.o xwrite.o zero.o zeropad.o
LIB	= libPW.a libPW_p.a

.c.o:
		  $(CC) -p $(CFLAGS) -c $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC)    $(CFLAGS) -c $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled DEFS.h parallel
		  ls *.o profiled/*.o > t1
		  chmod    644 `cat t1`
		  -chgrp daemon `cat t1` > /dev/null 2>&1
		  -chown  root `cat t1` > /dev/null 2>&1
		  @echo "	building profiled $@" && \
		  cd profiled && $(AR) cru ../libPW_p.a $(OBJS) && \
		  echo "	building regular $@" && \
		  cd .. && $(AR) cru libPW.a $(OBJS) && \
		  rm -f t1

parallel	:& $(OBJS)
profiled	:; mkdir profiled
curdir.o	: curdir.c
		  $(CC) -p $(CFLAGS) -DPORTABLE=1 -c $*.c && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CC)    $(CFLAGS) -DPORTABLE=1 -c $*.c && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o
move.o		: $(MACHINE)/move.s DEFS.h
		  $(CPP) -DPROF -I. $(MACHINE)/$*.s | $(AS) -o $*.o; \
		  $(LD) -X -r $*.o -o profiled/$*.o; \
		  $(CPP) -UPROF -I. $(MACHINE)/$*.s | $(AS) -o $*.o; \
		  $(LD) -x -r $*.o -o $*.O; mv $*.O $*.o
$(MACHINE)/move.s: ../../lib/libc/$(MACHINE)/gen/bcopy.s
		  rm -f $@ && sed 's/bcopy/move/g' < $? > $@
DEFS.h		:; ln -s ../../lib/libc/$(MACHINE)/DEFS.h

# Sorry, don't profile this puppy
alloca.o	: $(MACHINE)/alloca.s DEFS.h
		  $(CPP) -UPROF -I. $(MACHINE)/$*.s | $(AS) -o $*.o && \
		  $(LD) -X -r $*.o -o profiled/$*.o && \
		  $(CPP) -UPROF -I. $(MACHINE)/$*.s | $(AS) -o $*.o && \
		  $(LD) -x -r $*.o -o $*.O && mv $*.O $*.o
$(MACHINE)/alloca.s: ../../lib/libc/$(MACHINE)/gen/alloca.s
		  rm -f $@ && cp $? $@
install		:
		  install -m 644 libPW_p.a $(DESTDIR)/usr/lib/libPW_p.a
		  $(RANLIB) $(DESTDIR)/usr/lib/libPW_p.a
		  install -m 644 libPW.a $(DESTDIR)/usr/lib/libPW.a
		  $(RANLIB) $(DESTDIR)/usr/lib/libPW.a
clean clobber	:; rm -rf profiled $(OBJS) $(LIB) \
		   $(MACHINE)/move.s $(MACHINE)/alloca.s DEFS.h
