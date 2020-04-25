
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

I =	../../../include
CPROGS = erase hardcopy tekset 
DIRS =	td.d ged.d
LIB =	../../../lib/glib.a
BIN =	$(DESTDIR)/usr/bin/graf

tek4000 all:	$(CPROGS) td tek ged
		touch tek4000

install:
	install -c erase $(BIN)/erase
	install -c tekset $(BIN)/tekset
	install -c hardcopy $(BIN)/hardcopy
	cd td.d; make $(MFLAGS) $(MRULES) install
	cd ged.d; make $(MFLAGS) $(MRULES) install

erase:	erase.c
	$(CC) $(CFLAGS) -o erase erase.c
hardcopy: hardcopy.c
	$(CC) $(CFLAGS) -o hardcopy hardcopy.c
tekset: tekset.c
	$(CC) $(CFLAGS) -o tekset tekset.c

tek:	$(LIB) lib $(I) include
	cd lib; make $(MFLAGS) $(MRULES)
	touch tek
td:	$(LIB) td.d $(I)
	cd td.d; make $(MFLAGS) $(MRULES)
ged:	$(LIB) lib ged.d include $(I)
	cd ged.d; make $(MFLAGS) $(MRULES)

clean clobber:
	-rm -f *.o erase hardcopy tekset tek tek4000
	cd lib; make $(MFLAGS) $(MRULES) clean
	cd td.d; make $(MFLAGS) $(MRULES) clean
	cd ged.d; make $(MFLAGS) $(MRULES) clean
