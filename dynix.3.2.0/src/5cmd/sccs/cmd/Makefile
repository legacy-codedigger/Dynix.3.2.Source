
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
CFLAGS	= $(ATT_INCLUDE) -O
HDR	= ../hdr
BIN	= $(DESTDIR)/usr/bin
HELPLOC = $(DESTDIR)/usr/lib/help
SLIBS	= ../lib/comobj.a ../lib/mpwlib.a ../lib/cassi.a ../lib/libPW.a
FILES	= admin comb delta get help prs rmchg unget val vc what help2
IFILES	=\
	$(BIN)/admin $(BIN)/comb $(BIN)/delta $(BIN)/get $(BIN)/help $(BIN)/prs\
	$(BIN)/rmdel $(BIN)/sccsdiff $(BIN)/unget $(BIN)/val $(BIN)/vc\
	$(BIN)/what $(HELPLOC)/lib/help2

I	= install -s $? $@
IC	= install -c $? $@

all:&	$(FILES)

$(FILES): $(SLIBS)
	$(CC) $(CFLAGS) $@.c $(SLIBS) -o $@

$(SLIBS):
	cd ../lib; make $(MFLAGS) $(MRULES)

install:& $(IFILES)
	rm -f $(BIN)/cdc
	ln $(BIN)/rmdel $(BIN)/cdc
	rm -f $(BIN)/sact
	ln $(BIN)/unget $(BIN)/sact

$(BIN)/sccsdiff: sccsdiff.sh; $(IC)
$(HELPLOC)/lib/help2: help2; $(I)
$(BIN)/rmdel: rmchg; $(I)
$(BIN)/admin: admin; $(I)
$(BIN)/comb: comb; $(I)
$(BIN)/delta: delta; $(I)
$(BIN)/get: get; $(I)
$(BIN)/help: help; $(I)
$(BIN)/prs: prs; $(I)
$(BIN)/unget: unget; $(I)
$(BIN)/val: val; $(I)
$(BIN)/vc: vc; $(I)
$(BIN)/what: what; $(I)

clean clobber:
	rm -f *.o rmchg $(FILES) help2 core
