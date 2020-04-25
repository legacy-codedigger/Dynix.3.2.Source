
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

I =	../../include
L =	../../lib
CPROGS = bel gd pd remcom cvrtopt
SPROGS = quit yoo
DIRS =	gtop.d ptog.d
LIB =	$(L)/glib.a
BIN =	$(DESTDIR)/usr/bin/graf

gutil all:	$(CPROGS) $(SPROGS) gp pg

install:
	install -c remcom $(BIN)/remcom
	install -c bel $(BIN)/bel
	install -c gd $(BIN)/gd
	install -c pd $(BIN)/pd
	install -c cvrtopt $(BIN)/cvrtopt
	install -c quit $(BIN)/quit
	install -c yoo $(BIN)/yoo
	cd gtop.d; make $(MFLAGS) $(MRULES) install
	cd ptog.d; make $(MFLAGS) $(MRULES) install

remcom: $(I)/errpr.h remcom.c
	$(CC) $(CFLAGS) -o $@ $@.c $(LIB)
bel:	bel.c
	$(CC) $(CFLAGS) -o $@ $@.c $(LIB)
gd:	gd.c $(I)/gsl.h $(I)/gpl.h $(I)/errpr.h $(I)/util.h $(LIB)
	$(CC) $(CFLAGS) -o $@ $@.c $(LIB)
pd:	pd.c $(I)/errpr.h $(I)/util.h
	$(CC) $(CFLAGS) -o $@ $@.c $(LIB)
cvrtopt: cvrtopt.c $(I)/errpr.h $(I)/util.h $(I)/setop.h
	$(CC) $(CFLAGS) -o $@ $@.c $(LIB)

quit:	quit.sh
	cp quit.sh quit;

yoo:	yoo.sh
	cp yoo.sh yoo;

gp:
	cd gtop.d; make $(MFLAGS) $(MRULES)

pg:
	cd ptog.d; make $(MFLAGS) $(MRULES)

clean clobber:
	-rm -f *.o remcom yoo quit bel gd pd cvrtopt pg gp
	-cd gtop.d; make $(MFLAGS) $(MRULES) clean
	-cd ptog.d; make $(MFLAGS) $(MRULES) clean
