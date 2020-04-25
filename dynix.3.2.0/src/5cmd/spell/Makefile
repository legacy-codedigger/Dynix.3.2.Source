
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
OFILES	= spellin.o huff.o spellin.x.o huff.x.o spellprog.o hash.o\
	  hashlook.o malloc.o hashcheck.o hashmake.o hashmake.x.o hash.x.o
FILES	= spellprog spellin hashcheck hashmake hlista hlistb hstop
D	= $(DESTDIR)
T	= touch $@; chmod 777 $@
I	= install -s $? $@
ISH	= install -c -m 755 $? $@
ICP	= install -c -m 644 $? $@
IFILES	=\
	$(D)/usr/bin/spell\
	$(D)/usr/lib/spell/compress\
	$(D)/usr/lib/spell/spellprog\
	$(D)/usr/lib/spell/spellin\
	$(D)/usr/lib/spell/hashcheck\
	$(D)/usr/lib/spell/hashmake\
	$(D)/usr/lib/spell/hlista\
	$(D)/usr/lib/spell/hlistb\
	$(D)/usr/lib/spell/spellhist\
	$(D)/usr/lib/spell/hstop

all:	objects spellin.x hashmake.x binaries

objects:& $(OFILES)

binaries:& $(FILES)

spellprog: spellprog.o hash.o hashlook.o huff.o malloc.o
	$(CC) $(LDFLAGS) spellprog.o hash.o hashlook.o huff.o malloc.o -o spellprog

spellin.x: spellin.x.o huff.x.o
	cc -O spellin.x.o huff.x.o -o spellin.x

spellin: spellin.o huff.o
	$(CC) $(LDFLAGS) spellin.o huff.o -o spellin

hashcheck: hashcheck.o hash.o huff.o
	$(CC) $(LDFLAGS) hashcheck.o hash.o huff.o -o hashcheck

hashmake.x: hashmake.x.o hash.x.o
	cc -O hashmake.x.o hash.x.o -o hashmake.x

hashmake: hashmake.o hash.o
	$(CC) $(LDFLAGS) hashmake.o hash.o -o hashmake

spellin.x.o: spellin.c
	ln -s spellin.c spellin.x.c; \
	cc -O -c spellin.x.c; \
	rm -f spellin.x.c

huff.x.o: huff.c
	ln -s huff.c huff.x.c; \
	cc -O -c huff.x.c; \
	rm -f huff.x.c

hashmake.x.o: hashmake.c
	ln -s hashmake.c hashmake.x.c; \
	cc -O -c hashmake.x.c; \
	rm -f hashmake.x.c

hash.x.o: hash.c
	ln -s hash.c hash.x.c; \
	cc -O -c hash.x.c; \
	rm -f hash.x.c

htemp1:	list local extra hashmake.x
	cat list local extra | ./hashmake.x >htemp1

hlista: american hashmake.x spellin.x htemp1
	./hashmake.x <american |sort -u - htemp1 >hlista.htemp2; \
	./spellin.x `wc hlista.htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <hlista.htemp2 >hlista; \
	rm hlista.htemp2

hlistb: british hashmake.x spellin.x htemp1
	./hashmake.x <british |sort -u - htemp1 >hlistb.htemp2; \
	./spellin.x `wc hlistb.htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <hlistb.htemp2 >hlistb; \
	rm hlistb.htemp2

hstop:	stop spellin.x hashmake.x
	./hashmake.x <stop | sort -u >hstop.htemp2; \
	./spellin.x `wc hstop.htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <hstop.htemp2 >hstop; \
	rm hstop.htemp2

install:& $(IFILES)

$(D)/usr/bin/spell: spell.sh; $(ISH)
$(D)/usr/lib/spell/compress: compress.sh; $(ISH)
$(D)/usr/lib/spell/spellprog: spellprog; $(I)
$(D)/usr/lib/spell/spellin: spellin; $(I)
$(D)/usr/lib/spell/hashcheck: hashcheck; $(I)
$(D)/usr/lib/spell/hashmake: hashmake; $(I)
$(D)/usr/lib/spell/hlista: hlista; $(ICP)
$(D)/usr/lib/spell/hlistb: hlistb; $(ICP)
$(D)/usr/lib/spell/hstop: hstop; $(ICP)
$(D)/usr/lib/spell/spellhist: ; $(T) 

clean clobber:
	rm -f *.o spell spellprog spellin hashmake hlist*\
	hstop spellin.x hashmake.x htemp1 htemp2 hashcheck
