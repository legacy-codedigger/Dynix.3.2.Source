SRC	= $C/main.c $C/sym.c $C/misc.c $C/proc.c $C/engine.c $C/od.c \
	  $C/uarea.c $C/news.c $C/stdio.c stack.c $C/expr.c \
	  $C/bsearch.c $C/buffer.c $C/file.c $C/pte.c $C/nodes.c $C/maps.c \
	  $C/printit.c $C/stabstring.c $C/util.c
HDR 	= $C/crash.h $C/define.h $C/compact.h
OBJ 	= main.o sym.o misc.o proc.o engine.o od.o uarea.o news.o stdio.o \
	  stack.o expr.o bsearch.o buffer.o file.o pte.o nodes.o maps.o \
	  uncompress.o printit.o stabstring.o util.o

VPATH	=../common
C	=../common
CFLAGS	= -O -DBSD -DREADLINE -DCRASH_DEBUG -I$C $(INCLUDE) $(DEBUG)
LIB	= -lreadline -lseq -lcurses -ltermcap

all:	crash

crash:	parallel version.c
	$(CC) $(CFLAGS) -o crash $(OBJ) version.c $(LIB)

parallel:& $(OBJ)

version.c: mkdate $C/Version
	if [ "$(CCSVIEW)" != "" ] ;\
	then \
		Date="";\
		echo "char Version[] = \"`cat $C/Version`\";"  > version.c ;\
		echo "char *Date = &\"$$Date: 1991/05/31 20:42:16 $ (sequent)\"[1];" >> version.c ;\
	else \
		./mkdate `cat $C/Version` > version.c; \
	fi

version:
	chmod +w $C/Version
	awk '{ printf "%.2f\n", $$1 + 0.01 > "$C/Version" }' $C/Version

clean:	; rm -f *.o core date.c mkdate tags diffs version.c crash Makefile.bak

install:; install -s crash $(DESTDIR)/usr/etc/crash

mkdate:	mkdate.c
	cc $(CFLAGS) -o mkdate mkdate.c

tags:	$(HDR) $(SRC)
	ctags -tw $(HDR) $(SRC)

diffs: $(HDR) $(SRC) mkdate.c
	rdiff $(HDR) $(SRC) mkdate.c Makefile > diffs

listing:; @pr $(HDR) $(SRC)

main.o:	define.h

depend:
	echo '# DO NOT DELETE THIS LINE  --  make depend uses it' > makedep
	$(CC) -M $(CFLAGS) $(SRC) mkdate.c | \
	sed -e ':loop' \
	    -e 's/\.\.\/[^ /]*\/\.\./../' \
	    -e 't loop'  -e 's/\.\///g' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' >> makedep
	echo '/^# DO NOT DELETE THIS LINE/,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ex - Makefile < eddep
	rm eddep makedep

# DO NOT DELETE THIS LINE  --  make depend uses it

