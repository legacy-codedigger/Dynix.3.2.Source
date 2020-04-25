
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
CFLAGS	= -O $(ATT_INCLUDE)

##
# Fix up to use shell script when converted to new tools
##
all:	ed ed_crypt

ed:	ed.c
	$(CC) -UCRYPT $(CFLAGS) ed.c -o ed

ed_crypt: ed.c
	-if [ "$(EXPORT)" != TRUE ]; then \
		$(ATT_CRYPT_CC) -DCRYPT $(CFLAGS) ed.c -o ed_crypt -lcrypt; \
	fi

install:
	rm -f $(DESTDIR)/bin/red
	install -s ed $(DESTDIR)/bin/ed
	ln $(DESTDIR)/bin/ed $(DESTDIR)/bin/red
	-if [ "$(EXPORT)" != TRUE ]; then \
		rm -f $(ATT_CRYPT_DESTDIR)/bin/red; \
		install -s ed_crypt $(ATT_CRYPT_DESTDIR)/bin/ed; \
		ln $(ATT_CRYPT_DESTDIR)/bin/ed $(ATT_CRYPT_DESTDIR)/bin/red; \
	fi

clean clobber:
	  rm -f *.o ed ed_crypt core
