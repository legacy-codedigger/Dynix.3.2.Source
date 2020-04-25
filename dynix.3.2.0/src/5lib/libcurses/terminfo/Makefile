#	"@(#)makefile	1.3"	(1.9	12/28/82)
#
# There is currently no provision for local mods (reorder) or
# user mods ($TERMCAP).  These will be coming eventually.
#
# PARTS=	header \
# 	adds annarbor beehive cdc concept datamedia dec diablo general \
# 	hardcopy hazeltine heath homebrew hp ibm lsi microterm misc \
# 	pc perkinelmer print special \
# 	tektronix teleray teletype televideo ti visual \
# 	trailer (wyse cobar)

DESTDIR	= $(ATT_DESTDIR)
PARTS	= header *.ti trailer
COMPILE	= ../screen/tic.x

install: terminfo.src
	TERMINFO=$(DESTDIR)/usr/lib/terminfo; export TERMINFO; \
	$(COMPILE) terminfo.src

terminfo.src:	$(PARTS)
	cat $(PARTS) > terminfo.src

clean clobber:
	rm -f terminfo.src core
