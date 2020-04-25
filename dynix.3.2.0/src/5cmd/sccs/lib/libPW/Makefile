
CFLAGS	= $(ATT_INCLUDE) -O
LIB	= ../libPW.a

##
# This is a entire copy of src/lib/libPW but why fight AT&T ...
##
FILES	= abspath.o alloca.o anystr.o bal.o curdir.o fdfopen.o giveup.o \
	imatch.o index.o lockit.o logname.o move.o patoi.o \
	patol.o regcmp.o regex.o rename.o repeat.o repl.o satoi.o \
	setsig.o sname.o strend.o substr.o trnslat.o userdir.o \
	username.o verify.o any.o xalloc.o xcreat.o xlink.o \
	xopen.o xpipe.o xunlink.o xwrite.o xmsg.o \
	cat.o dname.o fatal.o clean.o userexit.o zero.o zeropad.o

all:	$(LIB)

$(LIB):& $(FILES)
	@$(AR) cru $(LIB) $(FILES)
	@$(RANLIB) $(LIB)

move.o: $(MACHINE)/move.s
	$(CPP) -UPROF -I. $(MACHINE)/$*.s | $(AS) -o $*.o

move.o: DEFS.h
alloca.o: DEFS.h

$(MACHINE)/move.s: ../../../../lib/libc/$(MACHINE)/gen/bcopy.s
	rm -f $@; sed 's/bcopy/move/g' < $? > $@

DEFS.h:
	ln -s ../../../../lib/libc/$(MACHINE)/DEFS.h

alloca.o: $(MACHINE)/alloca.s
	$(CPP) -UPROF -I. $(MACHINE)/$*.s | $(AS) -o $*.o

$(MACHINE)/alloca.s: ../../../../5lib/libPW/$(MACHINE)/alloca.s
	cd $(MACHINE); ln -s ../$?

curdir.o: curdir.c
	$(CC) $(CFLAGS) -DPORTABLE=1 -c curdir.c

install:

clean:
	rm -f *.o core $(MACHINE)/move.s $(MACHINE)/alloca.s DEFS.h

clobber: clean
	rm -f $(LIB)
