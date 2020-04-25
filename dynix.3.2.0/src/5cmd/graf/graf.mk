# @(#)graf.mk	1.3
ROOT =
ARGS =
ARGH =	$(ROOT)/usr/src/arglist
BIN1 =	$(ROOT)/usr/bin
BIN2 =	$(ROOT)/usr/bin/graf
LIB =	$(ROOT)/usr/lib/graf
SRC =	$(ROOT)/usr/src/cmd/graf
_SH_ =	sh

install:
	-mkdir $(BIN2); chmod 755 $(BIN2)
	-mkdir $(LIB); chmod 755 $(LIB)
	-mkdir $(SRC)/lib; chmod 755 $(SRC)/lib
	if test -x $(ARGH); \
	then cd src; eval make _SH_=$(_SH_) BIN1=$(BIN1) BIN2=$(BIN2) `$(ARGH) + $(ARGS)`;\
	else cd src; make _SH_=$(_SH_) BIN1=$(BIN1) BIN2=$(BIN2); fi
	chown bin $(BIN1)/graphics; chgrp bin $(BIN1)/graphics
	chown bin `find $(BIN2) -print`; chgrp bin `find $(BIN2) -print`
	chown bin `find $(LIB) -print`; chgrp bin `find $(LIB) -print`

clobber:
	-rm -f $(SRC)/src/dev.d/hp7220.d/hp7220 $(SRC)/src/dev.d/lolib/dev $(SRC)/src/dev.d/lolib/lolib.a $(SRC)/src/dev.d/uplib/uplib $(SRC)/src/dev.d/uplib/uplib.a
	-rm -f $(SRC)/src/dev.d/hp7220.d/hpd.d/hp $(SRC)/src/dev.d/hp7220.d/hpd.d/dev.a
	-rm -f $(SRC)/src/dev.d/tek4000.d/ged.d/ged $(SRC)/src/dev.d/tek4000.d/ged.d/ged.a $(SRC)/src/dev.d/tek4000.d/lib/tek $(SRC)/src/dev.d/tek4000.d/lib/tek.a $(SRC)/src/dev.d/tek4000.d/td.d/td $(SRC)/src/dev.d/tek4000.d/td.d/dev.a $(SRC)/src/dev.d/tek4000.d/tek $(SRC)/src/dev.d/tek4000.d/erase $(SRC)/src/dev.d/tek4000.d/hardcopy $(SRC)/src/dev.d/tek4000.d/tekset $(SRC)/src/dev.d/tek4000.d/tek4000
	-rm -f $(SRC)/src/glib.d/gpl.d/gpl $(SRC)/src/glib.d/gsl.d/gsl $(SRC)/src/glib.d/glib lib/glib.a
	-rm -f $(SRC)/src/gutil.d/gtop.d/gtop $(SRC)/src/gutil.d/gtop.d/gtop.a $(SRC)/src/gutil.d/ptog.d/ptog $(SRC)/src/gutil.d/ptog.d/ptog.a $(SRC)/src/gutil.d/bel $(SRC)/src/gutil.d/gd $(SRC)/src/gutil.d/pd $(SRC)/src/gutil.d/remcom $(SRC)/src/gutil.d/cvrtopt $(SRC)/src/gutil.d/quit $(SRC)/src/gutil.d/yoo
	-rm -f $(SRC)/src/stat.d/abs $(SRC)/src/stat.d/s.a $(SRC)/src/stat.d/af $(SRC)/src/stat.d/bar $(SRC)/src/stat.d/bucket $(SRC)/src/stat.d/ceil $(SRC)/src/stat.d/cor $(SRC)/src/stat.d/cusum $(SRC)/src/stat.d/exp $(SRC)/src/stat.d/floor $(SRC)/src/stat.d/gamma $(SRC)/src/stat.d/gas $(SRC)/src/stat.d/hilo $(SRC)/src/stat.d/hist $(SRC)/src/stat.d/label $(SRC)/src/stat.d/list $(SRC)/src/stat.d/log $(SRC)/src/stat.d/lreg $(SRC)/src/stat.d/mean $(SRC)/src/stat.d/mod
	-rm -f $(SRC)/src/stat.d/pair $(SRC)/src/stat.d/pie $(SRC)/src/stat.d/plot $(SRC)/src/stat.d/point $(SRC)/src/stat.d/power $(SRC)/src/stat.d/prime $(SRC)/src/stat.d/prod $(SRC)/src/stat.d/qsort $(SRC)/src/stat.d/rand $(SRC)/src/stat.d/rank $(SRC)/src/stat.d/root $(SRC)/src/stat.d/round $(SRC)/src/stat.d/siline $(SRC)/src/stat.d/subset $(SRC)/src/stat.d/sin $(SRC)/src/stat.d/title $(SRC)/src/stat.d/total $(SRC)/src/stat.d/var $(SRC)/src/stat.d/stat
	-rm -f $(SRC)/src/toc.d/vtoc.d/vtoc $(SRC)/src/toc.d/vtoc.d/vtoc.a $(SRC)/src/toc.d/dtoc $(SRC)/src/toc.d/ttoc $(SRC)/src/toc.d/toc
