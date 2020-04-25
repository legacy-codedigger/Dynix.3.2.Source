
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
YACC	= $(ATT_YACC)
UBIN	= $(DESTDIR)/usr/bin
ULIB	= $(DESTDIR)/usr/lib
MIP	= ../cc/pcc/mip
LINT	= lint
LINT1	= lintpass1
TMPDIR	= /usr/tmp
I	= install -s $? $@
IC	= install -c -m 644 $? $@
ICX	= install -c -m 755 $? $@
IFILES	=\
	$(ULIB)/lint1\
	$(ULIB)/lint2\
	$(ULIB)/llib-lc\
	$(ULIB)/llib-lc.ln\
	$(ULIB)/llib-port\
	$(ULIB)/llib-port.ln\
	$(ULIB)/llib-lm\
	$(ULIB)/llib-lm.ln\
	$(ULIB)/llib-lmalloc\
	$(ULIB)/llib-lmalloc.ln\
	$(UBIN)/lint
#  -DBUG4 is necessary to turn off pcc debugging tools (these tools cannot
#   be used with the lint shell script since their output conflicts)
CFLAGS	= -O -DLINT -I$(MIP) -I. -DBUG4 -DFLEXNAMES $(ATT_INCLUDE)
MFILES	= macdefs $(MIP)/manifest $(MIP)/mfile1

#object files for the first and second passes
OFILES1 = trees.o pftn.o scan.o comm1.o messages.o optim.o xdefs.o\
	  lerror.o msgbuf.o lint.o cgram.o
OFILES2 = lpass2.o lerror2.o messages2 msgbuf2.o

all:	$(LINT1) lint1 lint2 llib-lc.ln llib-port.ln llib-lm.ln llib-lmalloc.ln

#makes for the first pass of lint
lint1:&	$(OFILES1)
	$(CC) -o lint1 $(OFILES1)

# Make a lint1 that will run on the host processor when compiling
# lint in the cross-compilation environment.  This is needed so that
# the lint libraries may be built in the cross-compilation environment.

xobjects:&	$(OFILES1)
	
$(LINT1):	ctype.c getopt.c strchr.c tolower.c
		rm -f $(OFILES1) cgram.c
		make $(MFLAGS) $(MRULES) CC='cc' ATT_INCLUDE= xobjects
		cc -o $(LINT1) $(OFILES1) $(ATT_INCLUDE) ctype.c getopt.c strchr.c tolower.c
		rm $(OFILES1) cgram.c ctype.o getopt.o strchr.o tolower.o

trees.o:	$(MIP)/messages.h $(MFILES) $(MIP)/trees.c
			$(CC) $(CFLAGS) -c  $(MIP)/trees.c
pftn.o:		$(MIP)/messages.h $(MFILES) $(MIP)/pftn.c
			$(CC) $(CFLAGS) -c  $(MIP)/pftn.c
scan.o:		$(MIP)/messages.h $(MFILES) $(MIP)/scan.c
			$(CC) $(CFLAGS) -c  $(MIP)/scan.c
comm1.o:	$(MIP)/common $(MFILES)     $(MIP)/comm1.c
			$(CC) $(CFLAGS) -c  $(MIP)/comm1.c
messages.o:	$(MIP)/messages.h           $(MIP)/messages.c
			$(CC) $(CFLAGS) -c  $(MIP)/messages.c
optim.o:	$(MFILES)                   $(MIP)/optim.c
			$(CC) $(CFLAGS) -c  $(MIP)/optim.c
xdefs.o:	$(MFILES)                   $(MIP)/xdefs.c
			$(CC) $(CFLAGS) -c  $(MIP)/xdefs.c
cgram.o:	$(MIP)/messages.h $(MFILES) 
cgram.c:	$(MIP)/cgram.y
		$(YACC) $(MIP)/cgram.y ; mv y.tab.c cgram.c
lerror.o:	lerror.h $(MIP)/messages.h 
msgbuf.o:	lerror.h $(MIP)/messages.h
lint.o:		lerror.h lmanifest $(MIP)/messages.h $(MFILES)

#makes for the lint libraries
llib-lc.ln:  llib-lc $(LINT1)
	$(CC) $(CFLAGS) -E -C -Dlint llib-lc | $(LINT1) -vx -H$(TMPDIR)/hlint.lc >llib-lc.ln; \
	rm $(TMPDIR)/hlint.lc
llib-port.ln:  llib-port $(LINT1)
	$(CC) $(CFLAGS) -E -C -Dlint llib-port | $(LINT1) -vxp -H$(TMPDIR)/hlint.port >llib-port.ln; \
	rm $(TMPDIR)/hlint.port 
llib-lm.ln:  llib-lm $(LINT1)
	$(CC) $(CFLAGS) -E -C -Dlint llib-lm | $(LINT1) -vx -H$(TMPDIR)/hlint.lm >llib-lm.ln; \
	rm $(TMPDIR)/hlint.lm 
llib-lmalloc.ln:  llib-lmalloc $(LINT1)
	$(CC) $(CFLAGS) -E -C -Dlint llib-lmalloc | $(LINT1) -vx -H$(TMPDIR)/hlint.lmalloc >llib-lmalloc.ln; \
	rm $(TMPDIR)/hlint.lmalloc 
llib-lmalloc:  ../../5lib/libmalloc/llib-lmall.c
	rm -f $@; ln -s $? $@

#makes for the second pass of lint
lint2:&	$(OFILES2)
	$(CC) $(LFLAGS) $(FFLAG) -o lint2 $(OFILES2)
messages2:	messages.o; ln -s $? $@
msgbuf2.o:	lerror.h 
lpass2.o:	lerror.h lmanifest lpass2.h $(MIP)/manifest
lerror2.o:	lerror.h lmanifest lpass2.h $(MIP)/manifest $(MIP)/messages.h

install:& $(IFILES)

$(ULIB)/llib-lc: llib-lc; $(IC)
$(ULIB)/llib-lc.ln: llib-lc.ln; $(IC)
$(ULIB)/llib-port: llib-port; $(IC)
$(ULIB)/llib-port.ln: llib-port.ln; $(IC)
$(ULIB)/llib-lm: llib-lm; $(IC)
$(ULIB)/llib-lm.ln: llib-lm.ln; $(IC)
$(ULIB)/llib-lmalloc: llib-lmalloc; $(IC)
$(ULIB)/llib-lmalloc.ln: llib-lmalloc.ln; $(IC)
$(ULIB)/lint1: lint1; $(I)
$(ULIB)/lint2: lint2; $(I)
$(UBIN)/lint: lint.sh; $(ICX)

lintall:
	$(LINT) -DLINT  -I. -I$(MIP)  $(MIP)/xdefs.c $(MIP)/scan.c \
	$(MIP)/pftn.c $(MIP)/trees.c $(MIP)/optim.c lint.c $(MIP)/messages.c \
	$(MIP)/comm1.c msgbuf.c lerror.c -b cgram.c 
	$(LINT) -DLINT  -I$(MIP) -I. lpass2.c lerror2.c msgbuf2.c \
	$(MIP)/messages.c
clean:
	rm -f *.o messages2 cgram.c $(LINT1) llib-lc.ln llib-lm.ln llib-lmalloc* llib-port.ln
clobber: clean
	rm -f lint1 lint2 lint
