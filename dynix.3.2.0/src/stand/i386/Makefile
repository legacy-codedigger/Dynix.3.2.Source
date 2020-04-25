# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.28 1991/04/23 22:02:09 $
#
# Makefile for standalone programs
#
# Notes:
#    To make a bootable streamer tape, put "bootts"
#    followed by "boot" on a streamer tape.  This
#    change in format allows DYNIX 3.0 tapes to run
#    on both old and new SCED firmware.
#
C	= ../common
#
COPTS	= -DSTANDALONE -DBSD -I. -I$C  $(INCLUDE)
CFLAGS	= -O -i $(COPTS)
XCFLAGS	= -SO -DBOOTXX $(CFLAGS)
VCFLAGS	= -SO -DBOOTXX -DVTOC $(CFLAGS)
LDFLAGS	= -k -e start -T 4000
SRC=../../
# <-u symbol> causes undefined symbol to FORCE loading module from a library
XLDFLAGS= -u _putchar -k -e start
H	= $C/dump.h $C/mbad.h $C/rs.h $C/saio.h $C/scsi.h $C/sdreg.h $C/sec.h \
	  $C/sec_ctl.h $C/sec_diag.h $C/tsreg.h $C/xp.h $C/xt.h $C/zdc.h \
	  $C/zdc_ucode.h $C/wd.h $C/tm.h
LIBC	= libCMDS.a
LIBCO	= zdformat.o xpformat.o sdformat.o wdformat.o formatscsi.o boot.o \
	  bootxx.o cat.o copy.o copy2.o drtest.o dump.o ls.o ret.o ufs_subr.o \
	  ufs_tables.o zdaddbad.o zdlists.o zdverify.o zdwrdgn.o prtvtoc.o \
	  fsck.o rmvtoc.o cdump.o
MACHDEP	= gets.o putchar.o rtt.o slic_error.o slic_init.o slic_mIntr.o \
	  slic_setGM.o slic_wrAddr.o slic_wrData.o slic_wrslave.o \
	  slic_rdData.o slic_rdslave.o dkbad.o mbadinit.o bcopy.o bzero.o \
	  gsp.o bcmp.o rindex.o strcat.o getchar.o
DRIVERS	= rs.o packet.o sm.o sec.o sd.o wd.o ts.o xp.o xt.o zd.o ssm.o tm.o
PORTABLE= calloc.o conf.o conf_devsw.o conf_sd.o conf_wd.o\
	  conf_xt.o conf_xp.o conf_zd.o sys.o prf.o prompt.o stop.o exit.o\
	  clearbss.o atoi.o atol.o index.o qsort.o strcmp.o strcpy.o strlen.o\
	  strncmp.o R0.o buildcfg.o vtoc.o pad.o setargs.o compress.o
LIBA	= libsa.a
LIBAO	= $(MACHDEP) $(DRIVERS) $(PORTABLE) crt0.o
LIBX	= libsax.a
LIBXO	= sys_x.o mbad_x.o sec_x.o dkbad_x.o sd_x.o xp_x.o zd_x.o conf_x.o \
	  putchar_x.o conf_zd_x.o ts_x.o crt0_x.o vtoc_x.o  ssm_x.o
LIBV	= libsav.a
LIBVO	= sys_v.o mbad_v.o sec_v.o dkbad_v.o sd_v.o xp_v.o zd_v.o conf_v.o \
	  putchar_v.o conf_zd_v.o ts_v.o crt0_v.o vtoc_v.o
LIB	= $(LIBC) $(LIBA) $(LIBX) $(LIBV)
CMDS	= cat copy copy2 drtest dump sdformat wdformat formatscsi ls ret \
          xpformat prtvtoc rmvtoc cdump
SCMDS	= fsck
BOOT8K	= bootsd bootxp bootzd bootts
BOOT8KV	= bootsd_v bootxp_v bootzd_v bootts_v
CONF8K	= confsd.o confxp.o confzd.o confts.o

.s.o: 		; $(CPP) $(COPTS) $< | $(AS) -o $@

all		: objs $(LIB) cmds bootstraps
objs		: & $(LIBCO) $(LIBXO) $(LIBVO) $(CONF8K) $(LIBAO) installboot
cmds		: & zdformat $(CMDS) $(SCMDS) boot
bootstraps	: $(BOOT8K) $(BOOT8KV)

$(LIBA)		: & $(LIBAO)
		  @echo '	Fixing modes, groups, and owner'
		  @-chown root $? > /dev/null 2>&1
		  @-chgrp daemon $? > /dev/null 2>&1
		  @chmod 644 $?
		  $(AR) cru $(LIBA) $? && $(RANLIB) $(LIBA)
$(LIBC)		: & $(LIBCO)
		  @echo '	Fixing modes, groups, and owner'
		  @-chown root $? > /dev/null 2>&1
		  @-chgrp daemon $? > /dev/null 2>&1
		  @chmod 644 $?
		  $(AR) cru $(LIBC) $? && $(RANLIB) $(LIBC)
$(LIBX)		: & $(LIBXO)
		  @echo '	Fixing modes, groups, and owner'
		  @-chown root $(LIBXO) > /dev/null 2>&1
		  @-chgrp daemon $(LIBXO) > /dev/null 2>&1
		  @chmod 644 $(LIBXO)
		  $(AR) cru $(LIBX) $(LIBXO) && $(RANLIB) $(LIBX)
$(LIBV)		: & $(LIBVO)
		  @echo '	Fixing modes, groups, and owner'
		  @-chown root $(LIBVO) > /dev/null 2>&1
		  @-chgrp daemon $(LIBVO) > /dev/null 2>&1
		  @chmod 644 $(LIBVO)
		  $(AR) cru $(LIBV) $(LIBVO) && $(RANLIB) $(LIBV)

crt0.o		: crt0.s assym.h
bcopy.o		: bcopy.s assym.h
bzero.o		: bzero.s assym.h
gsp.o		: gsp.s assym.h
bcmp.o		: bcmp.s assym.h
rindex.o	: rindex.s assym.h
strcat.o	: strcat.s assym.h
assym.h		: genassym ; genassym.cross >assym.h
genassym	: genassym.c
		  cc -SO $(COPTS) genassym.c | as -o genassym.O & \
		  $(CC) $(COPTS) -o genassym genassym.c & wait
		  cc -o genassym.cross genassym.O ; rm -f genassym.O
installboot	: $C/installboot.c
		  $(CC) -O $(COPTS) -o installboot $C/installboot.c
$(CMDS)		: $(LIBA) ; $(LD) $(LDFLAGS) $@.o $(LIBA) -o $@ && chmod 644 $@

fsck.o:
		PWD=`pwd`;cd $(SRC)/etc/fsck; \
        	make $(MFLAGS) $(MRULES) standalone; \
		mv fsck.o $$PWD

fsck		: $(LIBA) fsck.o
		$(LD) $(LDFLAGS) fsck.o $(LIBA) -o $@ && chmod 644 $@

boot		: $(LIBA) crt0_x.o
		  $(LD) $(LDFLAGS) crt0_x.o $@.o $(LIBA) -o $@ && chmod 644 $@

boot		: boot.o
cat		: cat.o
copy		: copy.o
copy2		: copy2.o
drtest		: drtest.o
dump		: dump.o
cdump           : cdump.o
sdformat	: sdformat.o
wdformat	: wdformat.o
formatscsi	: formatscsi.o
ls		: ls.o
prtvtoc		: prtvtoc.o
rmvtoc		: rmvtoc.o
ret		: ret.o
xpformat	: xpformat.o
echo		: echo.o $(LIBA)
		$(LD) $(LDFLAGS) $@.o $(LIBA) -o $@ && chmod 644 $@

boot.o		: $C/boot.c ; $(CC) $(CFLAGS) -c $C/boot.c
cat.o		: $C/cat.c ;  $(CC) $(CFLAGS) -c $C/cat.c
copy.o		: $C/copy.c ; $(CC) $(CFLAGS) -c $C/copy.c
copy2.o		: $C/copy2.c ; $(CC) $(CFLAGS) -c $C/copy2.c
echo.o		: $C/echo.c ; $(CC) $(CFLAGS) -c $C/echo.c
drtest.o	: $C/drtest.c ; $(CC) $(CFLAGS) -c $C/drtest.c
dump.o		: $C/dump.c ; $(CC) $(CFLAGS) -c $C/dump.c
cdump.o		: $C/cdump.c ; $(CC) $(CFLAGS) -c $C/cdump.c
sdformat.o	: $C/sdformat.c ; $(CC) $(CFLAGS) -c $C/sdformat.c
wdformat.o	: $C/wdformat.c ; $(CC) $(CFLAGS) -c $C/wdformat.c
formatscsi.o	: $C/formatscsi.c ; $(CC) $(CFLAGS) -c $C/formatscsi.c
ls.o		: $C/ls.c ; $(CC) $(CFLAGS) -c $C/ls.c
prtvtoc.o	: $C/prtvtoc.c ; $(CC) $(CFLAGS) -c $C/prtvtoc.c
rmvtoc.o	: $C/rmvtoc.c ; $(CC) $(CFLAGS) -c $C/rmvtoc.c
ret.o		: $C/ret.c ; $(CC) $(CFLAGS) -c $C/ret.c
xpformat.o	: $C/xpformat.c ; $(CC) $(CFLAGS) -c $C/xpformat.c
buildcfg.o	: $C/buildcfg.c ; $(CC) $(CFLAGS) -c $C/buildcfg.c
pad.o		: $C/pad.c ; $(CC) $(CFLAGS) -c $C/pad.c

zdformat	: & zdformat.o zdaddbad.o zdlists.o zdverify.o zdwrdgn.o $(LIBA)
		  $(LD) $(LDFLAGS) -o zdformat \
		  zdformat.o zdaddbad.o zdlists.o zdverify.o zdwrdgn.o $(LIBA)
zdformat.o	: $C/zdformat/zdformat.c ; \
		  $(CC) -SO $(CFLAGS) $C/zdformat/zdformat.c | \
		  $(AS) -o zdformat.o
zdaddbad.o	: $C/zdformat/zdaddbad.c ; \
		  $(CC) $(CFLAGS) -c $C/zdformat/zdaddbad.c
zdlists.o	: $C/zdformat/zdlists.c ; \
		  $(CC) $(CFLAGS) -c $C/zdformat/zdlists.c
zdverify.o	: $C/zdformat/zdverify.c ; \
		  $(CC) $(CFLAGS) -c $C/zdformat/zdverify.c
zdwrdgn.o	: $C/zdformat/zdwrdgn.c ; \
		  $(CC) $(CFLAGS) -c $C/zdformat/zdwrdgn.c

zdcdbg		: & zdcdbg.o zdcdbg_u.o $(LIBA)
		  $(LD) $(LDFLAGS) -o zdcdbg \
		  zdcdbg.o zdcdbg_u.o $(LIBA)
zdcdbg.o	: $C/zdcdbg.c ; $(CC) $(CFLAGS) -c $C/zdcdbg.c
zdcdbg_u.o	: $C/zdcdbg_u.c ; $(CC) $(CFLAGS) -c $C/zdcdbg_u.c

zdceeld		: & zdceeld.o $(LIBA)
		  $(LD) $(LDFLAGS) -o zdceeld zdceeld.o $(LIBA) && \
		  chmod 644 zdceeld
zdceeld.o	: $C/zdceeld.c ; $(CC) $(CFLAGS) -c $C/zdceeld.c

ufs_subr.o	: $C/ufs_subr.c ; $(CC) $(CFLAGS) -c $C/ufs_subr.c
ufs_tables.o	: $C/ufs_tables.c ; $(CC) $(CFLAGS) -c $C/ufs_tables.c

bootsd		: confsd.o
confsd.o	: $C/conf_devsw.c
		  sed -e '/"sd",/p' -e '/"..",/d'< $C/conf_devsw.c >confsd.c ; \
		  $(CC) $(XCFLAGS) confsd.c | $(AS) -o confsd.o
bootxp		: confxp.o
confxp.o	: $C/conf_devsw.c
		  sed -e '/"xp",/p' -e '/"..",/d'< $C/conf_devsw.c >confxp.c; \
		  $(CC) $(XCFLAGS) -c confxp.c| $(AS) -o confxp.o
bootzd		: confzd.o
confzd.o	: $C/conf_devsw.c
		  sed -e '/"zd",/p' -e '/"..",/d'< $C/conf_devsw.c >confzd.c; \
		  $(CC) $(XCFLAGS) -c confzd.c| $(AS) -o confzd.o
bootts		: confts.o
confts.o	: $C/conf_devsw.c
		  sed -e '/"ts",/p' -e '/"..",/d'< $C/conf_devsw.c >confts.c ; \
		  $(CC) $(XCFLAGS) confts.c | $(AS) -o confts.o
$(BOOT8K)	: bootxx.o $(LIBX) $(LIBA) pad.o
		  @xx="`expr $@ : 'boot\(.*\)'`" && \
		  $(LD) -p $(XLDFLAGS) -o $@.symbols bootxx.o \
		    conf$${xx}.o $(LIBX) $(LIBA) pad.o && \
		  $(SIZE) $@.symbols | tail -1 | \
		  awk '{ sz = $$1 + $$2 - (16*1024); \
		    printf "\t%s: %d bytes (%d%%)\n", \
		    "$@", sz, ((sz * 100) + 8191)/8192; \
		    if (sz < 0 || sz > 8192) exit 1; exit 0; }' && \
		  cp $@.symbols tmp && \
		  $(STRIP) tmp && \
		  dd if=tmp of=$@ bs=8k count=1 2>/dev/null && \
		  rm -f tmp
$(BOOT8KV)	: bootxx.o $(LIBV) $(LIBA) pad.o
		  @xx="`expr $@ : 'boot\(..\)'`" && \
		  $(LD) -p $(XLDFLAGS) -o $@.symbols bootxx.o \
		    conf$${xx}.o $(LIBV) $(LIBA) pad.o && \
		  $(SIZE) $@.symbols | tail -1 | \
		  awk '{ sz = $$1 + $$2 - (16*1024); \
		    printf "\t%s: %d bytes (%d%%)\n", \
		    "$@", sz, ((sz * 100) + 8191)/8192; \
		    if (sz < 0 || sz > 8192) exit 1; exit 0; }' && \
		  cp $@.symbols tmp && \
		  $(STRIP) tmp && \
		  dd if=tmp of=$@ bs=8k count=1 2>/dev/null && \
		  rm -f tmp
bootxx.o	: $C/bootxx.c ; $(CC) $(CFLAGS) -c $C/bootxx.c

R0.o		: $C/R0.c ; $(CC) $(CFLAGS) -c $C/R0.c
atoi.o		: $C/atoi.c ; $(CC) $(CFLAGS) -c $C/atoi.c
atol.o		: $C/atol.c ; $(CC) $(CFLAGS) -c $C/atol.c
calloc.o	: $C/calloc.c ; $(CC) $(CFLAGS) -c $C/calloc.c
clearbss.o	: $C/clearbss.c ; $(CC) $(CFLAGS) -c $C/clearbss.c
compress.o      : $C/compress.c ; $(CC) $(CFLAGS) -c $C/compress.c
conf.o		: $C/conf.c ; $(CC) $(CFLAGS) -c $C/conf.c
conf_devsw.o	: $C/conf_devsw.c ; $(CC) $(CFLAGS) -c $C/conf_devsw.c
conf_sd.o	: $C/conf_sd.c ; $(CC) $(CFLAGS) -c $C/conf_sd.c
conf_wd.o	: $C/conf_wd.c ; $(CC) $(CFLAGS) -c $C/conf_wd.c
conf_xp.o	: $C/conf_xp.c ; $(CC) $(CFLAGS) -c $C/conf_xp.c
conf_xt.o	: $C/conf_xt.c ; $(CC) $(CFLAGS) -c $C/conf_xt.c
conf_zd.o	: $C/conf_zd.c ; $(CC) $(CFLAGS) -c $C/conf_zd.c
exit.o		: $C/exit.c ; $(CC) $(CFLAGS) -c $C/exit.c
index.o		: $C/index.c ; $(CC) $(CFLAGS) -c $C/index.c
prf.o		: $C/prf.c ; $(CC) $(CFLAGS) -c $C/prf.c
prompt.o	: $C/prompt.c ; $(CC) $(CFLAGS) -c $C/prompt.c
qsort.o		: $C/qsort.c ; $(CC) $(CFLAGS) -c $C/qsort.c
setargs.o	: $C/setargs.c ; $(CC) $(CFLAGS) -c $C/setargs.c
stop.o		: $C/stop.c ; $(CC) $(CFLAGS) -c $C/stop.c
strcmp.o	: $C/strcmp.c ; $(CC) $(CFLAGS) -c $C/strcmp.c
strcpy.o	: $C/strcpy.c ; $(CC) $(CFLAGS) -c $C/strcpy.c
strlen.o	: $C/strlen.c ; $(CC) $(CFLAGS) -c $C/strlen.c
strncmp.o	: $C/strncmp.c ; $(CC) $(CFLAGS) -c $C/strncmp.c
sys.o		: $C/sys.c ; $(CC) $(CFLAGS) -c $C/sys.c
vtoc.o		: $C/vtoc.c ; $(CC) $(CFLAGS) -c $C/vtoc.c

packet.o	: $C/packet.c ; $(CC) $(CFLAGS) -c $C/packet.c
rs.o		: $C/rs.c ; $(CC) $(CFLAGS) -c $C/rs.c
sd.o		: $C/sd.c ; $(CC) $(CFLAGS) -c $C/sd.c
sm.o		: $C/sm.c ; $(CC) $(CFLAGS) -c $C/sm.c
wd.o		: $C/wd.c ; $(CC) $(CFLAGS) -c $C/wd.c
ssm.o		: $C/ssm.c ; $(CC) $(CFLAGS) -c $C/ssm.c
sec.o		: $C/sec.c ; $(CC) $(CFLAGS) -c $C/sec.c
ts.o		: $C/ts.c ; $(CC) $(CFLAGS) -c $C/ts.c
tm.o		: $C/tm.c ; $(CC) $(CFLAGS)  -c $C/tm.c
xp.o		: $C/xp.c ; $(CC) $(CFLAGS) -c $C/xp.c
xt.o		: $C/xt.c ; $(CC) $(CFLAGS) -c $C/xt.c
zd.o		: $C/zd.c ; $(CC) $(CFLAGS) -c $C/zd.c

dkbad.o		: $C/dkbad.c ; $(CC) $(CFLAGS) -c $C/dkbad.c
getchar.o	: $C/getchar.c ; $(CC) $(CFLAGS) -c $C/getchar.c
gets.o		: $C/gets.c ; $(CC) $(CFLAGS) -c $C/gets.c
mbadinit.o	: $C/mbadinit.c ; $(CC) $(CFLAGS) -c $C/mbadinit.c
putchar.o	: $C/putchar.c ; $(CC) $(CFLAGS) -c $C/putchar.c
rtt.o		: $C/rtt.c ; $(CC) $(CFLAGS) -c $C/rtt.c
slic_error.o	: $C/slic_error.c ; $(CC) $(CFLAGS) -c $C/slic_error.c
slic_init.o	: $C/slic_init.c ; $(CC) $(CFLAGS) -c $C/slic_init.c
slic_mIntr.o	: $C/slic_mIntr.c ; $(CC) $(CFLAGS) -c $C/slic_mIntr.c
slic_rdData.o	: $C/slic_rdData.c ; $(CC) $(CFLAGS) -c $C/slic_rdData.c
slic_rdslave.o	: $C/slic_rdslave.c ; $(CC) $(CFLAGS) -c $C/slic_rdslave.c
slic_setGM.o	: $C/slic_setGM.c ; $(CC) $(CFLAGS) -c $C/slic_setGM.c
slic_wrAddr.o	: $C/slic_wrAddr.c ; $(CC) $(CFLAGS) -c $C/slic_wrAddr.c
slic_wrData.o	: $C/slic_wrData.c ; $(CC) $(CFLAGS) -c $C/slic_wrData.c
slic_wrslave.o	: $C/slic_wrslave.c ; $(CC) $(CFLAGS) -c $C/slic_wrslave.c

conf_x.o	: $C/conf.c ; $(CC) $(XCFLAGS) $C/conf.c | $(AS) -o $@
conf_zd_x.o	: $C/conf_zd.c ; $(CC) $(XCFLAGS) $C/conf_zd.c | $(AS) -o $@
dkbad_x.o	: $C/dkbad.c ; $(CC) $(XCFLAGS) $C/dkbad.c | $(AS) -o $@
mbad_x.o	: $C/mbadinit.c ; $(CC) $(XCFLAGS) $C/mbadinit.c | $(AS) -o $@
putchar_x.o	: $C/putchar.c ; $(CC) $(XCFLAGS) $C/putchar.c | $(AS) -o $@
sd_x.o		: $C/sd.c ; $(CC) $(XCFLAGS) $C/sd.c | $(AS) -o $@
ssm_x.o		: $C/ssm.c ; $(CC) $(XCFLAGS) $C/ssm.c | $(AS) -o $@
sec_x.o		: $C/sec.c ; $(CC) $(XCFLAGS) $C/sec.c | $(AS) -o $@
sys_x.o		: $C/sys.c ; $(CC) $(XCFLAGS) $C/sys.c | $(AS) -o $@
xp_x.o		: $C/xp.c ; $(CC) $(XCFLAGS) $C/xp.c | $(AS) -o $@
zd_x.o		: $C/zd.c ; $(CC) $(XCFLAGS) $C/zd.c | $(AS) -o $@
ts_x.o		: $C/ts.c ; $(CC) $(XCFLAGS) $C/ts.c | $(AS) -o $@
vtoc_x.o	: $C/vtoc.c ; $(CC) $(XCFLAGS) $C/vtoc.c | $(AS) -o $@
crt0_x.o	: crt0.s assym.h
		  $(CPP) -I. -DBOOTXX $(COPTS) crt0.s | $(AS) -o $@

conf_v.o	: $C/conf.c ; $(CC) $(VCFLAGS) $C/conf.c | $(AS) -o $@
conf_zd_v.o	: $C/conf_zd.c ; $(CC) $(VCFLAGS) $C/conf_zd.c | $(AS) -o $@
dkbad_v.o	: $C/dkbad.c ; $(CC) $(VCFLAGS) $C/dkbad.c | $(AS) -o $@
mbad_v.o	: $C/mbadinit.c ; $(CC) $(VCFLAGS) $C/mbadinit.c | $(AS) -o $@
putchar_v.o	: $C/putchar.c ; $(CC) $(VCFLAGS) $C/putchar.c | $(AS) -o $@
sd_v.o		: $C/sd.c ; $(CC) $(VCFLAGS) $C/sd.c | $(AS) -o $@
wd_v.o		: $C/wd.c ; $(CC) $(VCFLAGS) $C/wd.c | $(AS) -o $@
sec_v.o		: $C/sec.c ; $(CC) $(VCFLAGS) $C/sec.c | $(AS) -o $@
sys_v.o		: $C/sys.c ; $(CC) $(VCFLAGS) $C/sys.c | $(AS) -o $@
xp_v.o		: $C/xp.c ; $(CC) $(VCFLAGS) $C/xp.c | $(AS) -o $@
zd_v.o		: $C/zd.c ; $(CC) $(VCFLAGS) $C/zd.c | $(AS) -o $@
ts_v.o		: $C/ts.c ; $(CC) $(VCFLAGS) $C/ts.c | $(AS) -o $@
vtoc_v.o	: $C/vtoc.c ; $(CC) $(VCFLAGS) $C/vtoc.c | $(AS) -o $@
crt0_v.o	: crt0.s assym.h
		  $(CPP) -I. -DBOOTXX $(COPTS) crt0.s | $(AS) -o $@

clean		:; rm -f zdformat $(CMDS) $(SCMDS) boot installboot genassym \
		   $(LIBA) $(LIBAO) $(LIBC) $(LIBCO) $(BOOT8K) $(LIBX) $(LIBXO)\
		   $(BOOT8KV) $(LIBV) $(LIBVO) \
		   genassym.cross conf??.c conf??.o assym.h pad.o \
		   zdcdbg zdcdbg.o zdcdbg_u.o zdceeld zdceeld.o depend \
		   *.symbols

install.headers	:; cd $C && install -m 644 -c *.h $(DESTDIR)/sys/stand
install		:; cd $C && install -c conf_*.c $(DESTDIR)/sys/stand
		   install -c -s genassym $(DESTDIR)/sys/stand & \
		   install -c -s installboot $(DESTDIR)/etc & \
		   install -c -s -m 644 boot $(DESTDIR)/boot & \
		   install -c -s -m 644 boot $(DESTDIR)/usr/boot & \
		   install -c -m 644 $(BOOT8K) $(BOOT8KV) $(DESTDIR)/stand & \
		   install -c -s -m 644 zdformat $(CMDS) $(SCMDS) boot \
			$(DESTDIR)/stand & \
		   install -c -m 644 $(LIBC) $(LIBX) $(LIBV) $(LIBA) \
			$(DESTDIR)/sys/stand & \
		   install -c -m 644 Makefile.obj \
			$(DESTDIR)/sys/stand/Makefile & \
		   install -c -m 644 $(LIBA) $(DESTDIR)/usr/lib/$(LIBA) & \
		   wait
		   rm -f $(DESTDIR)/stand/boot
		   $(RANLIB) $(DESTDIR)/usr/lib/$(LIBA)
		   $(RANLIB) $(DESTDIR)/sys/stand/$(LIBX)
		   $(RANLIB) $(DESTDIR)/sys/stand/$(LIBV)
		   $(RANLIB) $(DESTDIR)/sys/stand/$(LIBA)
depend		:; $(CC) -M $(COPTS) *.c $C/*.c $C/zdformat/*.c >depend

#
# Macro for vi to generate local header dependencies
#  :r!grep -v tmpos depend | grep -v 'c$' | sed 's;\.\./common;$C;' | sort
#

boot.o: $C/aouthdr.h
boot.o: $C/filehdr.h
boot.o: $C/saio.h
boot.o: $C/scnhdr.h
bootxx.o: $C/saio.h
buildcfg.o: $C/sec.h
cdump.o: $C/dump.h
cdump.o: $C/saio.h
cdump.o: $C/sec.h
compress.o: $C/dump.h
compress.o: $C/saio.h
compress.o: $C/sec.h
conf.o: $C/saio.h
conf_devsw.o: $C/saio.h
conf_sd.o: $C/ccs.h
conf_sd.o: $C/saio.h
conf_sd.o: $C/scsi.h
conf_sd.o: $C/scsidisk.h
conf_sd.o: $C/sdreg.h
conf_wd.o: $C/ccs.h
conf_wd.o: $C/saio.h
conf_wd.o: $C/scsi.h
conf_wd.o: $C/scsidisk.h
conf_wd.o: $C/wd.h
conf_xp.o: $C/mbad.h
conf_xp.o: $C/saio.h
conf_xp.o: $C/xp.h
conf_xt.o: $C/mbad.h
conf_zd.o: $C/zdc.h
drtest.o: $C/saio.h
dump.o: $C/dump.h
dump.o: $C/saio.h
dump.o: $C/sec.h
exit.o: $C/saio.h
formatscsi.o: $C/saio.h
formatscsi.o: $C/scsi.h
formatscsi.o: $C/scsidisk.h
formatscsi.o: $C/sdreg.h
formatscsi.o: $C/sec_diag.h
ls.o: $C/saio.h
mbadinit.o: $C/mbad.h
packet.o: $C/rs.h
prtvtoc.o: $C/saio.h
ret.o: $C/saio.h
ret.o: $C/scsi.h
rmvtoc.o: $C/saio.h
rs.o: $C/rs.h
rs.o: $C/saio.h
rtt.o: $C/sec.h
sd.o: $C/ccs.h
sd.o: $C/saio.h
sd.o: $C/scsi.h
sd.o: $C/scsidisk.h
sd.o: $C/sdreg.h
sd.o: $C/sec.h
sd.o: $C/sec_ctl.h
sdformat.o: $C/ccs.h
sdformat.o: $C/saio.h
sdformat.o: $C/scsi.h
sdformat.o: $C/scsidisk.h
sdformat.o: $C/scsiioctl.h
sdformat.o: $C/sdformat.h
sdformat.o: $C/sdreg.h
sec.o: $C/saio.h
sec.o: $C/scsi.h
sec.o: $C/sec.h
sec.o: $C/sec_ctl.h
slic_error.o: $C/saio.h
slic_mIntr.o: $C/saio.h
slic_rdData.o: $C/saio.h
slic_rdslave.o: $C/saio.h
slic_setGM.o: $C/saio.h
slic_wrAddr.o: $C/saio.h
slic_wrData.o: $C/saio.h
slic_wrslave.o: $C/saio.h
sm.o: $C/sec.h
ssm.o: $C/scsi.h
ssm.o: $C/ssm.h
ssm.o: $C/ssm_cons.h
ssm.o: $C/ssm_misc.h
ssm.o: $C/ssm_scsi.h
stop.o: $C/saio.h
sys.o: $C/saio.h
tm.o: $C/saio.h
tm.o: $C/scsi.h
tm.o: $C/ssm.h
tm.o: $C/ssm_scsi.h
tm.o: $C/tm.h
ts.o: $C/saio.h
ts.o: $C/scsi.h
ts.o: $C/sec.h
ts.o: $C/sec_ctl.h
ts.o: $C/tsreg.h
vtoc.o: $C/saio.h
wd.o: $C/ccs.h
wd.o: $C/saio.h
wd.o: $C/scsi.h
wd.o: $C/scsidisk.h
wd.o: $C/scsiioctl.h
wd.o: $C/ssm.h
wd.o: $C/ssm_scsi.h
wd.o: $C/wd.h
wdformat.o: $C/ccs.h
wdformat.o: $C/saio.h
wdformat.o: $C/scsi.h
wdformat.o: $C/scsidisk.h
wdformat.o: $C/scsiioctl.h
wdformat.o: $C/sdformat.h
xp.o: $C/mbad.h
xp.o: $C/saio.h
xp.o: $C/xp.h
xpformat.o: $C/saio.h
xpformat.o: $C/xp.h
xt.o: $C/mbad.h
xt.o: $C/saio.h
xt.o: $C/xt.h
zd.o: $C/saio.h
zd.o: $C/zdc.h
zdaddbad.o: $C/zdformat/../saio.h
zdaddbad.o: $C/zdformat/zdformat.h
zdcdbg.o: $C/zdc_ucode.h
zdformat.o: $C/zdformat/../saio.h
zdformat.o: $C/zdformat/../zdc.h
zdformat.o: $C/zdformat/zdformat.h
zdlists.o: $C/zdformat/../saio.h
zdlists.o: $C/zdformat/zdformat.h
zdverify.o: $C/zdformat/../saio.h
zdverify.o: $C/zdformat/zdformat.h
zdwrdgn.o: $C/zdformat/../saio.h
zdwrdgn.o: $C/zdformat/zdformat.h
