#	@(#)libcrypt.mk	1.1
ROOT=
INS=cpset

all:	libcrypt.a

libcrypt.a: crypt.o
	mv crypt.o libcrypt.a

install: all
	$(INS) libcrypt.a $(ROOT)/usr/lib 664 

clean:
	-rm -f *.o
clobber: clean
	-rm -f libcrypt.a
