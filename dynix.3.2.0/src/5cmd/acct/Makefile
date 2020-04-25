
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
INSDIR	= $(DESTDIR)/usr/lib/acct
LIB	= lib/a.a
TESTDIR	= .
LIBACCT	= acctcms acctcon1 acctcon2 acctdisk acctdusg acctmerg\
	  acctprc1 acctprc2 acctwtmp fwtmp wtmpfix chargefee\
	  ckpacct dodisk monacct lastlogin nulladm prctmp prdaily\
	  prtacct remove runacct shutacct startup turnacct holidays\
	  ptecms.awk ptelus.awk

NOTYET	= diskusg

all:	library acctcms acctcom acctcon1\
	acctcon2 acctdisk acctdusg acctmerg accton\
	acctprc1 acctprc2 acctwtmp\
	fwtmp wtmpfix\
	chargefee ckpacct dodisk lastlogin\
	monacct nulladm prctmp prdaily\
	prtacct remove runacct\
	shutacct startup turnacct holtable \
	awkecms awkelus

install:
	for i in $(LIBACCT); do\
		install -c $$i $(INSDIR)/$$i;\
	done
	install -c acctcom $(DESTDIR)/bin
	install -c -m 4755 -o root accton $(INSDIR)/accton

library:
		cd lib; make $(MFLAGS) $(MRULES)

acctcms:	library acctcms.c
		$(CC) $(CFLAGS) acctcms.c $(LIB) -o $(TESTDIR)/acctcms

acctcom:	library acctcom.c
		$(CC) $(CFLAGS) acctcom.c $(LIB) -o $(TESTDIR)/acctcom

acctcon1:	library acctcon1.c
		$(CC) $(CFLAGS) acctcon1.c $(LIB) -o $(TESTDIR)/acctcon1

acctcon2:	acctcon2.c
		$(CC) $(CFLAGS) acctcon2.c -o $(TESTDIR)/acctcon2

acctdisk:	acctdisk.c
		$(CC) $(CFLAGS) acctdisk.c -o $(TESTDIR)/acctdisk

acctdusg:	acctdusg.c
		$(CC) $(CFLAGS) acctdusg.c -o $(TESTDIR)/acctdusg

acctmerg:	library acctmerg.c
		$(CC) $(CFLAGS) acctmerg.c $(LIB) -o $(TESTDIR)/acctmerg

accton:		accton.c
		$(CC) $(CFLAGS) accton.c -o $(TESTDIR)/accton

acctprc1:	library acctprc1.c
		$(CC) $(CFLAGS) acctprc1.c $(LIB) -o $(TESTDIR)/acctprc1

acctprc2:	acctprc2.c
		$(CC) $(CFLAGS) acctprc2.c -o $(TESTDIR)/acctprc2

acctwtmp:	acctwtmp.c
		$(CC) $(CFLAGS) acctwtmp.c -o $(TESTDIR)/acctwtmp

diskusg:	diskusg.c
		$(CC) $(CFLAGS) diskusg.c -o $(TESTDIR)/diskusg

fwtmp:		fwtmp.c
		$(CC) $(CFLAGS) fwtmp.c -o $(TESTDIR)/fwtmp

wtmpfix:	wtmpfix.c
		$(CC) $(CFLAGS) wtmpfix.c -o $(TESTDIR)/wtmpfix

chargefee:	chargefee.sh
		cp chargefee.sh $(TESTDIR)/chargefee;\
		chmod +x $(TESTDIR)/chargefee;

ckpacct:	ckpacct.sh
		cp ckpacct.sh $(TESTDIR)/ckpacct;\
		chmod +x $(TESTDIR)/ckpacct

dodisk:		dodisk.sh
		cp dodisk.sh $(TESTDIR)/dodisk;\
		chmod +x $(TESTDIR)/dodisk

monacct:	monacct.sh
		cp monacct.sh $(TESTDIR)/monacct;\
		chmod +x $(TESTDIR)/monacct

lastlogin:	lastlogin.sh
		cp lastlogin.sh $(TESTDIR)/lastlogin;\
		chmod +x $(TESTDIR)/lastlogin

nulladm:	nulladm.sh
		cp nulladm.sh $(TESTDIR)/nulladm;\
		chmod +x $(TESTDIR)/nulladm

prctmp:		prctmp.sh
		cp prctmp.sh $(TESTDIR)/prctmp;\
		chmod +x $(TESTDIR)/prctmp

prdaily:	prdaily.sh
		cp prdaily.sh $(TESTDIR)/prdaily;\
		chmod +x $(TESTDIR)/prdaily

prtacct:	prtacct.sh
		cp prtacct.sh $(TESTDIR)/prtacct;\
		chmod +x $(TESTDIR)/prtacct

remove:		remove.sh
		cp remove.sh $(TESTDIR)/remove;\
		chmod +x $(TESTDIR)/remove

runacct:	runacct.sh
		cp runacct.sh $(TESTDIR)/runacct;\
		chmod +x $(TESTDIR)/runacct

shutacct:	shutacct.sh
		cp shutacct.sh $(TESTDIR)/shutacct;\
		chmod +x $(TESTDIR)/shutacct

startup:	startup.sh
		cp startup.sh $(TESTDIR)/startup;\
		chmod +x $(TESTDIR)/startup

turnacct:	turnacct.sh
		cp turnacct.sh $(TESTDIR)/turnacct;\
		chmod +x $(TESTDIR)/turnacct

holtable:	holidays

awkecms:	ptecms.awk

awkelus:	ptelus.awk

clean clobber:
		-rm -f *.o
		cd lib; make $(MFLAGS) $(MRULES) clean
		-rm -f acctcms acctcom acctcon1 acctcon2 acctdisk diskusg\
			acctdusg acctmerg accton acctprc1 acctprc2 acctwtmp\
			fwtmp wtmpfix
		-rm -f chargefee ckpacct dodisk lastlogin nulladm\
			monacct prctmp prdaily prtacct remove runacct\
			shutacct startup turnacct
