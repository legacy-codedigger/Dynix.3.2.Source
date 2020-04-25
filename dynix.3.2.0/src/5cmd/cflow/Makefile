
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
CFLAGS	= -O -DFLEXNAMES -DUNIX5 $(ATT_INCLUDE)
FILES	= Makefile README cflow.sh dag.c lpfx.c nmf.c flip.c
ALL	= dag lpfx nmf flip
LINT	= ../lint
MIP	= ../cc/pcc/mip
LDFLAGS =
IC	= install -c $? $@
I	= install -s $? $@
D	= $(DESTDIR)
IFILES	=\
	$(D)/usr/bin/cflow\
	$(D)/usr/lib/dag\
	$(D)/usr/lib/lpfx\
	$(D)/usr/lib/nmf\
	$(D)/usr/lib/flip

all:	$(ALL)

dag:	dag.c
	$(CC) $(CFLAGS) $(LDFLAGS) dag.c -o dag

lpfx:	lpfx.c $(LINT)/lerror.h $(LINT)/lmanifest $(LINT)/lpass2.h \
		$(MIP)/manifest
	$(CC) $(CFLAGS) -I$(LINT) -I$(MIP) $(LDFLAGS) lpfx.c -o lpfx -lmalloc

nmf:	nmf.c
	$(CC) $(CFLAGS) nmf.c $(LDFLAGS) -o nmf

flip:	flip.c
	$(CC) $(CFLAGS) flip.c $(LDFLAGS) -o flip

install:& $(IFILES)
	:

$(D)/usr/bin/cflow: cflow.sh; $(IC)
$(D)/usr/lib/dag: dag; $(I)
$(D)/usr/lib/lpfx: lpfx; $(I)
$(D)/usr/lib/nmf: nmf; $(I)
$(D)/usr/lib/flip: flip; $(I)

clean:
	-rm -f *.o a.out make.out core

clobber:	clean
	-rm -f $(ALL)
