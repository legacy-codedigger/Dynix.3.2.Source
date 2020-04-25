
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

I =	../../include
L =	$(DESTDIR)/usr/lib/graf
CMDS =	dtoc ttoc vtoc
INS = install

toc all : $(CMDS)
	touch toc

install:
	install -c ttoc $(DESTDIR)/usr/bin/graf/ttoc
	install -c dtoc $(DESTDIR)/usr/bin/graf/dtoc
	for i in ttoc.d/*; do install -c -m 644 $$i $(L)/$$i; done
	cd vtoc.d; make $(MFLAGS) $(MRULES) install

ttoc :	ttoc.sh ttoc.d
	cp ttoc.sh ttoc;

dtoc :	dtoc.sh
	cp dtoc.sh dtoc;

vtoc :	vtoc.d $(I)
	cd vtoc.d; make $(MFLAGS) $(MRULES)

clean clobber:
	-rm -f dtoc vtoc ttoc toc
	cd vtoc.d; make $(MFLAGS) $(MRULES) clean
