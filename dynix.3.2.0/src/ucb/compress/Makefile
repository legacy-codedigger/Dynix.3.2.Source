
COMFLAGS= -O -DBSD4_2 $(INCLUDE)
BIN	= $(DESTDIR)/usr/ucb
MAN	= /usr/man/man1
#define to "l" if manuals go on /usr/man/manl
L	= 1

all :&	compress atob btoa

compress : compress.c USERMEM
	$(CC) $(COMFLAGS) -DUSERMEM=`cat USERMEM` -o compress compress.c

# USERMEM may have to be set by hand.  It should contain the amount of
# available user memory in bytes.  See the README file for more info.
USERMEM:
#	sh usermem > USERMEM

atob:	atob.c
	$(CC) $(COMFLAGS) -o atob atob.c

btoa:	btoa.c
	$(CC) $(COMFLAGS) -o btoa btoa.c

# Manual pages are not installed but a copy of them
# live here as a convenience to doing src comparisons
install:
	install -s compress $(BIN)/compress
	rm -f $(BIN)/uncompress $(BIN)/zcat
	ln $(BIN)/compress $(BIN)/uncompress
	ln $(BIN)/compress $(BIN)/zcat
	install -s atob $(BIN)/atob
	install -s btoa $(BIN)/btoa
	install -c compressdir $(BIN)/compressdir
	install -c uncompressdir $(BIN)/uncompressdir
	install -c tarmail $(BIN)/tarmail
	install -c untarmail $(BIN)/untarmail
	install -c zmore $(BIN)/zmore
	install -c zdiff $(BIN)/zdiff
	install -c zcmp $(BIN)/zcmp

clean:
	rm -f compress atob btoa a.out core *.o
