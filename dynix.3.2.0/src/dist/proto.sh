#! /bin/sh
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

# $Header: proto.sh 1.156 1992/03/13 20:32:50 $
#
# $Log: proto.sh,v $

# Shell script to build a prototype filesystem for 4.2 BSD.

if [ -f /dynix ]
then
	ident=`whoami`
else
	ident=`logname`
fi
chown=/etc/chown
chgrp=/bin/chgrp
chmod=/bin/chmod

if [ "$ident" != "root" ]; then
	echo "proto.sh: You are not \"root\""
	echo "proto.sh: Correct owners and groups will be lost"
	chown=: chgrp=:
fi

# Loop through directories building them
# If run as root, restore the owner and group
# being careful to do ownership first as this may clear permission bits
#
# IMPORTANT!  The "service" numeric userid is hardcoded here due to
# inconsistencies with the shipped vs. internal "service" uid.  If
# the "service" numeric uid changes in the passwd file, these must
# be changed to match.  See tmpos/usr/service/* below...

while read mode owner group dir
do
	mkdir $dir
	( $chown $owner $dir && $chgrp $group $dir && $chmod $mode $dir ) &
done << '_P_'
755	root	daemon	tmpos
755	root	daemon	tmpos/bin
755	root	daemon	tmpos/dev
755	root	daemon	tmpos/etc
755	root	daemon	tmpos/etc/vtoc
755	root	daemon	tmpos/etc/diskinfo
755	root	daemon	tmpos/lib
755	root	daemon	tmpos/stand
1777	root	daemon	tmpos/tmp
755	root	daemon	tmpos/usr
755	root	daemon	tmpos/usr/adm
755	root	daemon	tmpos/usr/bin
755	root	daemon	tmpos/usr/crash
755	root	daemon	tmpos/usr/diag
755	root	daemon	tmpos/usr/dict
755	root	daemon	tmpos/usr/dict/papers
755	root	daemon	tmpos/usr/doc
755	root	daemon	tmpos/usr/doc/Mail
755	root	daemon	tmpos/usr/doc/adv.ed
755	root	daemon	tmpos/usr/doc/as
755	root	daemon	tmpos/usr/doc/beginners
755	root	daemon	tmpos/usr/doc/cacm
755	root	daemon	tmpos/usr/doc/config
755	root	daemon	tmpos/usr/doc/csh
755	root	daemon	tmpos/usr/doc/ctour
755	root	daemon	tmpos/usr/doc/curses
755	root	daemon	tmpos/usr/doc/diction
755	root	daemon	tmpos/usr/doc/diskperf
755	root	daemon	tmpos/usr/doc/edtut
755	root	daemon	tmpos/usr/doc/eqn
755	root	daemon	tmpos/usr/doc/ex
755	root	daemon	tmpos/usr/doc/fs
755	root	daemon	tmpos/usr/doc/fsck
755	root	daemon	tmpos/usr/doc/gprof
755	root	daemon	tmpos/usr/doc/hints
755	root	daemon	tmpos/usr/doc/ipc
755	root	daemon	tmpos/usr/doc/kchanges
755	root	daemon	tmpos/usr/doc/lisp
755	root	daemon	tmpos/usr/doc/lpd
755	root	daemon	tmpos/usr/doc/memacros
755	root	daemon	tmpos/usr/doc/misc
755	root	daemon	tmpos/usr/doc/msmacros
755	root	daemon	tmpos/usr/doc/net
755	root	daemon	tmpos/usr/doc/pascal
755	root	daemon	tmpos/usr/doc/porttour
755	root	daemon	tmpos/usr/doc/refer
755	root	daemon	tmpos/usr/doc/sendmail
755	root	daemon	tmpos/usr/doc/setup
755	root	daemon	tmpos/usr/doc/shell
755	root	daemon	tmpos/usr/doc/summary
755	root	daemon	tmpos/usr/doc/sysman
755	root	daemon	tmpos/usr/doc/troff
755	root	daemon	tmpos/usr/doc/trofftut
755	root	daemon	tmpos/usr/doc/uchanges
755	root	daemon	tmpos/usr/doc/uprog
755	root	daemon	tmpos/usr/doc/uucp
755	root	daemon	tmpos/usr/doc/yacc
755	root	daemon	tmpos/usr/doc/xdr
755	root	daemon	tmpos/usr/etc
755	root	daemon	tmpos/usr/games
755	root	daemon	tmpos/usr/games/lib
755	root	daemon	tmpos/usr/games/lib/quiz.k
755	root	daemon	tmpos/usr/hosts
755	root	daemon	tmpos/usr/include
755	root	daemon	tmpos/usr/lib
755	root	daemon	tmpos/usr/lib/cf
755	root	daemon	tmpos/usr/lib/cf/cf
755	root	daemon	tmpos/usr/lib/cf/m4
755	root	daemon	tmpos/usr/lib/cf/sitedep
755	root	daemon	tmpos/usr/lib/font
755	root	daemon	tmpos/usr/lib/fontinfo
755	root	daemon	tmpos/usr/lib/learn
755	root	daemon	tmpos/usr/lib/learn/C
755	root	daemon	tmpos/usr/lib/learn/bin
755	root	daemon	tmpos/usr/lib/learn/editor
755	root	daemon	tmpos/usr/lib/learn/eqn
755	root	daemon	tmpos/usr/lib/learn/files
755	root	daemon	tmpos/usr/lib/learn/log
755	root	daemon	tmpos/usr/lib/learn/macros
755	root	daemon	tmpos/usr/lib/learn/morefiles
755	root	daemon	tmpos/usr/lib/lex
755	root	daemon	tmpos/usr/lib/lint
755	root	daemon	tmpos/usr/lib/lisp
755	root	daemon	tmpos/usr/lib/lisp/manual
755	root	daemon	tmpos/usr/lib/me
755	root	daemon	tmpos/usr/lib/me/src
755	root	daemon	tmpos/usr/lib/mm
755	root	daemon	tmpos/usr/lib/ms
755	root	daemon	tmpos/usr/lib/refer
755	root	daemon	tmpos/usr/lib/struct
755	root	daemon	tmpos/usr/lib/tabset
755	root	daemon	tmpos/usr/lib/term
755	root	daemon	tmpos/usr/lib/tmac
755	uucp	daemon	tmpos/usr/lib/uucp
755	root	daemon	tmpos/usr/lib/vfont
755	root	daemon	tmpos/usr/local
755	root	daemon	tmpos/usr/local/bin
755	root	daemon	tmpos/usr/local/lib
755	root	daemon	tmpos/usr/man
755	root	daemon	tmpos/usr/man/man0
755	root	daemon	tmpos/usr/man/man1
755	root	daemon	tmpos/usr/man/man2
755	root	daemon	tmpos/usr/man/man3
755	root	daemon	tmpos/usr/man/man4
755	root	daemon	tmpos/usr/man/man5
755	root	daemon	tmpos/usr/man/man6
755	root	daemon	tmpos/usr/man/man7
755	root	daemon	tmpos/usr/man/man8
755	root	daemon	tmpos/usr/man/manl
755	root	daemon	tmpos/usr/man/mann
755	root	daemon	tmpos/usr/man/mano
777	root	daemon	tmpos/usr/man/cat0
777	root	daemon	tmpos/usr/man/cat1
777	root	daemon	tmpos/usr/man/cat2
777	root	daemon	tmpos/usr/man/cat3
777	root	daemon	tmpos/usr/man/cat4
777	root	daemon	tmpos/usr/man/cat5
777	root	daemon	tmpos/usr/man/cat6
777	root	daemon	tmpos/usr/man/cat7
777	root	daemon	tmpos/usr/man/cat8
777	root	daemon	tmpos/usr/man/catl
777	root	daemon	tmpos/usr/man/catn
777	root	daemon	tmpos/usr/man/cato
777	root	daemon	tmpos/usr/msgs
777	root	daemon	tmpos/usr/preserve
755	root	daemon	tmpos/usr/pub
755	root	daemon	tmpos/usr/sequent
755	root	daemon	tmpos/usr/sequent/libpp
755	root	daemon	tmpos/usr/sequent/pdbx.demo
755	root	daemon	tmpos/usr/sequent/gtpp.demo
755	75	daemon	tmpos/usr/service
777	75	daemon	tmpos/usr/service/mailbug
755	75	daemon	tmpos/usr/service/stand
755	75	daemon	tmpos/usr/service/stress_tools
755	root	daemon	tmpos/usr/skel
755	root	daemon	tmpos/usr/spool
755	root	daemon	tmpos/usr/spool/adm
755	root	daemon	tmpos/usr/spool/at
755	root	daemon	tmpos/usr/spool/at/past
777	root	daemon	tmpos/usr/spool/locks
775	daemon	daemon	tmpos/usr/spool/lpd
755	root	daemon	tmpos/usr/spool/mail
750	root	daemon	tmpos/usr/spool/mqueue
755	root	daemon	tmpos/usr/spool/rwho
777	root	daemon	tmpos/usr/spool/secretmail
775	uucp	daemon	tmpos/usr/spool/uucp
755	uucp	daemon	tmpos/usr/spool/uucp/AUDIT
755	uucp	daemon	tmpos/usr/spool/uucp/C.
755	uucp	daemon	tmpos/usr/spool/uucp/CORRUPT
755	uucp	daemon	tmpos/usr/spool/uucp/D.
755	uucp	daemon	tmpos/usr/spool/uucp/D.hostname
755	uucp	daemon	tmpos/usr/spool/uucp/D.hostnameX
755	uucp	daemon	tmpos/usr/spool/uucp/LOG
755	uucp	daemon	tmpos/usr/spool/uucp/LOG/uucico
755	uucp	daemon	tmpos/usr/spool/uucp/LOG/uucp
755	uucp	daemon	tmpos/usr/spool/uucp/LOG/uux
755	uucp	daemon	tmpos/usr/spool/uucp/LOG/uuxqt
755	uucp	daemon	tmpos/usr/spool/uucp/LOG/xferstats
755	uucp	daemon	tmpos/usr/spool/uucp/STST
755	uucp	daemon	tmpos/usr/spool/uucp/TM.
755	uucp	daemon	tmpos/usr/spool/uucp/X.
755	uucp	daemon	tmpos/usr/spool/uucp/XTMP
777	uucp	daemon	tmpos/usr/spool/uucp/LCK
777	uucp	daemon	tmpos/usr/spool/uucppublic
755	root	daemon	tmpos/usr/sys
755	root	daemon	tmpos/usr/sys/stand
1777	root	daemon	tmpos/usr/tmp
755	root	daemon	tmpos/usr/ucb
755	root	daemon	tmpos_att
755	root	daemon	tmpos_att/bin
755	root	daemon	tmpos_att/dev
755	root	daemon	tmpos_att/etc
755	root	daemon	tmpos_att/etc/log
755	root	daemon	tmpos_att/lib
1777	root	daemon	tmpos_att/tmp
755	root	daemon	tmpos_att/usr
755	root	daemon	tmpos_att/usr/adm
755	root	daemon	tmpos_att/usr/adm/acct
755	root	daemon	tmpos_att/usr/adm/acct/nite
755	root	daemon	tmpos_att/usr/bin
755	root	daemon	tmpos_att/usr/bin/graf
755	root	daemon	tmpos_att/usr/include
755	root	daemon	tmpos_att/usr/include/sys
755	root	daemon	tmpos_att/usr/include/machine
755	root	daemon	tmpos_att/usr/lib
755	root	daemon	tmpos_att/usr/lib/acct
755	root	daemon	tmpos_att/usr/lib/cron
755	root	daemon	tmpos_att/usr/lib/ctrace
755	root	daemon	tmpos_att/usr/lib/dwb
755	root	daemon	tmpos_att/usr/lib/font
755	root	daemon	tmpos_att/usr/lib/font/devi10
755	root	daemon	tmpos_att/usr/lib/font/devi10/rasti10
755	root	daemon	tmpos_att/usr/lib/font/devi10/rasti10/devaps
755	root	daemon	tmpos_att/usr/lib/dwb/samples
755	root	daemon	tmpos_att/usr/lib/graf
755	root	daemon	tmpos_att/usr/lib/graf/whatis
755	root	daemon	tmpos_att/usr/lib/graf/ttoc.d
755	root	daemon	tmpos_att/usr/lib/help
755	root	daemon	tmpos_att/usr/lib/help/lib
755	root	daemon	tmpos_att/usr/lib/lex
755	root	daemon	tmpos_att/usr/lib/macros
755	root	daemon	tmpos_att/usr/lib/mailx
755	root	daemon	tmpos_att/usr/lib/nterm
755	root	daemon	tmpos_att/usr/lib/sa
755	root	daemon	tmpos_att/usr/lib/spell
755	root	daemon	tmpos_att/usr/lib/terminfo
755	root	daemon	tmpos_att/usr/lib/terminfo/1
755	root	daemon	tmpos_att/usr/lib/terminfo/2
755	root	daemon	tmpos_att/usr/lib/terminfo/3
755	root	daemon	tmpos_att/usr/lib/terminfo/4
755	root	daemon	tmpos_att/usr/lib/terminfo/5
755	root	daemon	tmpos_att/usr/lib/terminfo/6
755	root	daemon	tmpos_att/usr/lib/terminfo/7
755	root	daemon	tmpos_att/usr/lib/terminfo/8
755	root	daemon	tmpos_att/usr/lib/terminfo/9
755	root	daemon	tmpos_att/usr/lib/terminfo/a
755	root	daemon	tmpos_att/usr/lib/terminfo/b
755	root	daemon	tmpos_att/usr/lib/terminfo/c
755	root	daemon	tmpos_att/usr/lib/terminfo/d
755	root	daemon	tmpos_att/usr/lib/terminfo/e
755	root	daemon	tmpos_att/usr/lib/terminfo/f
755	root	daemon	tmpos_att/usr/lib/terminfo/g
755	root	daemon	tmpos_att/usr/lib/terminfo/h
755	root	daemon	tmpos_att/usr/lib/terminfo/i
755	root	daemon	tmpos_att/usr/lib/terminfo/j
755	root	daemon	tmpos_att/usr/lib/terminfo/k
755	root	daemon	tmpos_att/usr/lib/terminfo/l
755	root	daemon	tmpos_att/usr/lib/terminfo/m
755	root	daemon	tmpos_att/usr/lib/terminfo/n
755	root	daemon	tmpos_att/usr/lib/terminfo/o
755	root	daemon	tmpos_att/usr/lib/terminfo/p
755	root	daemon	tmpos_att/usr/lib/terminfo/q
755	root	daemon	tmpos_att/usr/lib/terminfo/r
755	root	daemon	tmpos_att/usr/lib/terminfo/s
755	root	daemon	tmpos_att/usr/lib/terminfo/t
755	root	daemon	tmpos_att/usr/lib/terminfo/u
755	root	daemon	tmpos_att/usr/lib/terminfo/v
755	root	daemon	tmpos_att/usr/lib/terminfo/w
755	root	daemon	tmpos_att/usr/lib/terminfo/x
755	root	daemon	tmpos_att/usr/lib/terminfo/y
755	root	daemon	tmpos_att/usr/lib/terminfo/z
755	root	daemon	tmpos_att/usr/lib/tmac
755	uucp	daemon	tmpos_att/usr/lib/uucp
777	uucp	daemon	tmpos_att/usr/lib/uucp/.XQTDIR
777	uucp	daemon	tmpos_att/usr/lib/uucp/.OLD
777	root	daemon	tmpos_att/usr/news
755	root	daemon	tmpos_att/usr/pub
755	root	daemon	tmpos_att/usr/spool
755	root	daemon	tmpos_att/usr/spool/cron
755	daemon	daemon	tmpos_att/usr/spool/cron/atjobs
755	daemon	daemon	tmpos_att/usr/spool/cron/crontabs
755	lp	daemon	tmpos_att/usr/spool/lp
755	lp	daemon	tmpos_att/usr/spool/lp/model
755	lp	daemon	tmpos_att/usr/spool/lp/class
755	lp	daemon	tmpos_att/usr/spool/lp/interface
755	lp	daemon	tmpos_att/usr/spool/lp/member
755	lp	daemon	tmpos_att/usr/spool/lp/request
777	uucp	daemon	tmpos_att/usr/spool/uucp
777	uucp	daemon	tmpos_att/usr/spool/uucppublic
1777	root	daemon	tmpos_att/usr/tmp
755	root	daemon	tmpos_crypt
755	root	daemon	tmpos_crypt/bin
755	root	daemon	tmpos_crypt/etc
755	root	daemon	tmpos_crypt/lib
755	root	daemon	tmpos_crypt/usr
755	root	daemon	tmpos_crypt/usr/lib
755	root	daemon	tmpos_crypt/usr/bin
755	root	daemon	tmpos_crypt/usr/man
755	root	daemon	tmpos_crypt/usr/man/man1
777	root	daemon	tmpos_crypt/usr/man/cat1
755	root	daemon	tmpos_crypt/usr/man/man3
777	root	daemon	tmpos_crypt/usr/man/cat3
755	root	daemon	tmpos_crypt/usr/ucb
755	root	daemon	tmpos_att_crypt
755	root	daemon	tmpos_att_crypt/bin
755	root	daemon	tmpos_att_crypt/lib
755	root	daemon	tmpos_att_crypt/usr
755	root	daemon	tmpos_att_crypt/usr/lib
755	root	daemon	tmpos_att_crypt/usr/bin
755	root	daemon	tmpos_stripe
755	root	daemon	tmpos_stripe/etc
755	root	daemon	tmpos_stripe/usr
755	root	daemon	tmpos_stripe/usr/lib
755	root	daemon	tmpos_stripe/usr/include
755	root	daemon	tmpos_stripe/usr/man
755	root	daemon	tmpos_stripe/usr/man/man4
755	root	daemon	tmpos_stripe/usr/man/man5
755	root	daemon	tmpos_stripe/usr/man/man8
755	root	daemon	tmpos_stripe/usr/man/cat4
755	root	daemon	tmpos_stripe/usr/man/cat5
755	root	daemon	tmpos_stripe/usr/man/cat8
755	root	daemon	tmpos_stripe/usr/sys
755	root	daemon	tmpos_mirror
755	root	daemon	tmpos_mirror/etc
755	root	daemon	tmpos_mirror/usr
755	root	daemon	tmpos_mirror/usr/lib
755	root	daemon	tmpos_mirror/usr/include
755	root	daemon	tmpos_mirror/usr/include/sys
755	root	daemon	tmpos_mirror/usr/man
755	root	daemon	tmpos_mirror/usr/man/man1
755	root	daemon	tmpos_mirror/usr/man/man2
755	root	daemon	tmpos_mirror/usr/man/man3
755	root	daemon	tmpos_mirror/usr/man/man4
755	root	daemon	tmpos_mirror/usr/man/man5
755	root	daemon	tmpos_mirror/usr/man/man6
755	root	daemon	tmpos_mirror/usr/man/man7
755	root	daemon	tmpos_mirror/usr/man/man8
755	root	daemon	tmpos_mirror/usr/man/cat4
755	root	daemon	tmpos_mirror/usr/man/cat5
755	root	daemon	tmpos_mirror/usr/man/cat8
755	root	daemon	tmpos_mirror/usr/sys
_P_

(cd tmpos; ln -s usr/sys sys)
(cd tmpos_stripe; ln -s usr/sys sys)
(cd tmpos_mirror; ln -s usr/sys sys)


/bin/cat > tmpos/usr/skel/.profile << '_P_'
PATH=.:/usr/local:/usr/ucb:/usr/bin:/bin
export PATH 
umask 027
stty new crt erase \ kill \ intr \
_P_
$chown     uucp tmpos/usr/skel/.profile &
$chgrp   daemon tmpos/usr/skel/.profile &
$chmod      755 tmpos/usr/skel/.profile &
wait
/bin/cat > tmpos/usr/skel/.login << '_P_'
stty new crt erase \ kill \ intr \
set noglob; eval `tset -sQ -m 'dialup:tvi925' -m 'network:tvi925'`
set path=(/usr/local /usr/ucb /usr/bin /bin /usr/games .)
biff n
_P_
$chown     uucp tmpos/usr/skel/.login &
$chgrp   daemon tmpos/usr/skel/.login &
$chmod      755 tmpos/usr/skel/.login &
wait
/bin/cat > tmpos/usr/skel/.cshrc << '_P_'
setenv EXINIT 'set ai redraw nomagic'
setenv MORE -d
set history=20 noclobber ignoreeof
alias ls ls -F
alias cp cp -i
alias mv mv -i
alias rm rm -i
alias msgs msgs -p
_P_
$chown     uucp tmpos/usr/skel/.cshrc &
$chgrp   daemon tmpos/usr/skel/.cshrc &
$chmod      755 tmpos/usr/skel/.cshrc &
wait
/bin/cat > tmpos/usr/skel/.mailrc << '_P_'
set ask askcc dot metoo hold
_P_
$chown     uucp tmpos/usr/skel/.mailrc &
$chgrp   daemon tmpos/usr/skel/.mailrc &
$chmod      755 tmpos/usr/skel/.mailrc &
wait
/bin/cat > tmpos/usr/spool/uucppublic/.hushlogin << '_P_'
_P_
$chown     uucp tmpos/usr/spool/uucppublic/.hushlogin &
$chgrp   daemon tmpos/usr/spool/uucppublic/.hushlogin &
$chmod      644 tmpos/usr/spool/uucppublic/.hushlogin &
wait
/bin/cat > tmpos/dev/MAKEDEV << '_P_'
#! /bin/sh
#
# Device "make" file.  Valid arguments:
#	std		standard devices
#	local		configuration specific devices
#	generic 	generic devices
# Disks:
#	sd*	SCSI disks
#	xp*	Fujitsu Eagle on Xylogics 450 on multibus
#	zd*	ZDC disks
#	ds*	DCC disk striping
#	mr*	DCC disk mirroring
#       wd*     SCSI disks on SSM
# Tapes:
#	xt*	Cipher 1600 bpi *or* Fujitsu GCR on Xylogics 472 on multibus
#	ts*	Streamer tape on SCSI
#       tm*     SCSI Streamer tape on SSM
#       tg*     SCSI 1/2" tape on SSM
# Printers:
#	lp*	Systech MLP-2000 parallel line printer interface
#       sp*     SSM parallel line printer interface
# Terminals:
#	st*	Systech 8-16 line mux on Multibus
#	co*	SCED console terminal lines
#       sc*     SSM terminal lines
# Pseudo terminals:
#	pty*	set of master and slave pseudo terminals
# PC Interconnect pseudo devices:
#	pci	Set of PC interconnect network devices
# Atomic Lock Memory:
#	alm	set of minor devices in the alm directory
# System Controller dependent pseudo drivers:
#       console console tty line
#       smemco  system controller memory and special function driver
#       sm      SCED controller memory driver
#       ss      SSM controller memory driver
# Permissions:
#	Std is on a per device basis, with memory devices group deamon mode 640
#	Disks are owned by root, group daemon, and mode 640
#	Tapes are mode 666
#	Permission on terminals will be set by login
#
umask 77
for i
do
case $i in

# These should be 'sort of' in sync with /sys/conf/GENERIC
generic)
	sh MAKEDEV std 
# note: co0, sc0 and sm0 are included in 'std' above
        sh MAKEDEV     sc1
	sh MAKEDEV     co1 co2 co3
	sh MAKEDEV     sm1 sm2 sm3
	sh MAKEDEV     ss1 ss2 ss3
	sh MAKEDEV sd0 sd1 sd2 sd3 sd4 sd5 sd6 sd7
	sh MAKEDEV wd0 wd1 wd2 wd3 wd4 wd5 wd6 wd7
	sh MAKEDEV xp0 xp1 xp2 xp3 xp4 xp5 xp6 xp7
	sh MAKEDEV zd0 zd1 zd2 zd3 zd4 zd5 zd6 zd7
	sh MAKEDEV mr0 mr1 mr2 mr3 mr4 mr5 mr6 mr7
	sh MAKEDEV st0 st1 st2 st3
	sh MAKEDEV ts0 ts1 ts2 ts3
	sh MAKEDEV tm0 tm1 tm2 tm3
	sh MAKEDEV tg0 tg1 
	sh MAKEDEV xt0 xt1 xt2 xt3
	sh MAKEDEV pty0
	;;

std)
	/etc/mknod console	c 1  64	; chmod 622 console
	/etc/mknod smemco	c 28 64	; chmod 660 smemco;	chgrp daemon smemco
	sh MAKEDEV co0 sm0 ss0 sc0
	/etc/mknod tty		c 2  0	; chmod 666 tty
	/etc/mknod mem		c 3  0	; chmod 640 mem;	chgrp daemon mem
	/etc/mknod kmem		c 3  1	; chmod 640 kmem;	chgrp daemon kmem
	/etc/mknod null		c 3  2	; chmod 666 null
	/etc/mknod kMWmem	c 3  3	; chmod 600 kMWmem;	chgrp daemon kMWmem
	/etc/mknod kMBmem	c 3  4	; chmod 600 kMBmem;	chgrp daemon kMBmem
	/etc/mknod drum		c 4  0	; chmod 660 drum;	chgrp daemon drum
	/etc/mknod vlsi		c 21  0	; chmod 640 vlsi;	chgrp daemon vlsi
	sh MAKEDEV ctape
	;;

sm*)
	unit=`expr $i : "sm\(.*\)"`
	case $unit in
	[0-3])	/etc/mknod smem$unit c 10 $unit
		chmod 660 smem$unit
		chgrp daemon smem$unit ;;
	*) echo bad unit for sm in: $i ;;
	esac
	;;

ss*)
	unit=`expr $i : "ss\(.*\)"`
	case $unit in
	[0-3])  /etc/mknod ssmem$unit c 26 $unit
		chmod 660 ssmem$unit
		chgrp daemon ssmem$unit ;;
	*) echo bad unit for ss in: $i ;;
	esac
	;;

xt*)
	umask 0 ; unit=`expr $i : '..\(.*\)'`
	case $i in
	xt*) blk=6; chr=13;;
	esac
	case $unit in
	0|1|2|3)
		four=`expr $unit + 4` ; eight=`expr $unit + 8`
		twelve=`expr $unit + 12`;
		# /etc/mknod mt$unit	b $blk $unit
		# /etc/mknod mt$four	b $blk $four
		/etc/mknod mt$eight	b $blk $eight
		/etc/mknod mt$twelve	b $blk $twelve
		# /etc/mknod nmt$unit	b $blk $four ;: sanity w/pdp11 v7
		/etc/mknod nmt$eight	b $blk $twelve ;: ditto
		# /etc/mknod nrmt$unit	c $chr $four ;: sanity w/pdp11 v7
		/etc/mknod nrmt$eight	c $chr $twelve ;: ditto
		# /etc/mknod rmt$unit	c $chr $unit
		# /etc/mknod rmt$four	c $chr $four
		/etc/mknod rmt$eight	c $chr $eight
		/etc/mknod rmt$twelve	c $chr $twelve
		umask 77
		;;
	*)
		echo bad unit for tape in: $1
		;;
	esac
	;;

ts*|tm*)
	umask 0; unit=`expr $i : '..\(.*\)'`
	case $i in
	ts*) name=ts; blk=3; chr=9 ;;
	tm*) name=tm; blk=8; chr=23 ;;
	esac
	case $unit in
	[0123])
		zero=`expr $unit + 0`; eight=`expr $unit + 8`
#		/etc/mknod  $name$zero  b $blk $zero
#		/etc/mknod  $name$eight b $blk $eight
		/etc/mknod r$name$zero  c $chr $zero
		/etc/mknod r$name$eight c $chr $eight
		umask 77
		;;
	*)
		echo bad unit for tape in: $1
		;;
	esac
	;;

tg*)
	umask 0; unit=`expr $i : '..\(.*\)'`
	case $i in
	tg*) name=tg; blk=10; chr=29 ;;
	esac
	case $unit in
	[01234567])

		rewa=`expr $unit + 0` ; rewb=`expr $unit + 16`
		rewc=`expr $unit + 32`
		norewa=`expr $unit + 8` ; norewb=`expr $unit + 24`
		norewc=`expr $unit + 40`
		rew1=`expr $unit \* 256 + 0` ; rew2=`expr $unit \* 256 + 2`
		rew3=`expr $unit \* 256 + 3`
		norew1=`expr $unit \* 256 + 128` ; 
		norew2=`expr $unit \* 256 + 130`
		norew3=`expr $unit \* 256 + 131`;

		/etc/mknod tg$rewa	b $blk $rew1
		/etc/mknod tg$rewb	b $blk $rew2
		/etc/mknod tg$rewc	b $blk $rew3
		/etc/mknod tg$norewa	b $blk $norew1
		/etc/mknod tg$norewb	b $blk $norew2
		/etc/mknod tg$norewc	b $blk $norew3

		/etc/mknod rtg$rewa	c $chr $rew1
		/etc/mknod rtg$rewb	c $chr $rew2
		/etc/mknod rtg$rewc	c $chr $rew3
		/etc/mknod rtg$norewa	c $chr $norew1
		/etc/mknod rtg$norewb	c $chr $norew2
		/etc/mknod rtg$norewc	c $chr $norew3
		umask 77
		;;
	*)
		echo bad unit for tape in: $1
		;;
	esac
	;;
ds*)
	unit=`expr $i : '..\(.*\)'`
	/etc/mknod ds$unit	b	11	$unit
	/etc/mknod rds$unit	c	30	$unit
	chgrp daemon ds$unit
	chmod 640 ds$unit
	chgrp daemon rds$unit
	chmod 640 rds$unit
	;;

mr*)
	unit=`expr $i : '..\(.*\)'`
	/etc/mknod mr$unit	b	8	$unit
	/etc/mknod rmr$unit	c	20	$unit
	chgrp daemon mr$unit
	chmod 640 mr$unit
	chgrp daemon rmr$unit
	chmod 640 rmr$unit
	;;

sd*|xp*|zd*|wd*)
	unit=`expr $i : '..\(.*\)'`
	# (see VUNIT() and VPART() in vtoc.h)
	# unit number is bits 3-11
	# partition number is bit 0-2,12-16 ie (P&~0x3 << 8) + (P&0x3)
	#				    or (P/8 << 11) + (P%8)
	# all  is partition 255               = 248 << 8 + 7 = 63495
	# default layouts:
	# (see /etc/vtoc/* for actual partitions)
	# fw   is partition 12(m) (SCSI only) =   8 << 8 + 4 =  2052
	# boot is partition 14(o)             =   8 << 8 + 6 =  2054
	# vtoc is partition 15(p)             =   8 << 8 + 7 =  2055
	case $i in
	sd*) name=sd; blk=1; chr=8;;
	xp*) name=xp; blk=5; chr=12;;
	zd*) name=zd; blk=7; chr=17;;
	wd*) name=wd; blk=2; chr=22;;
	esac
	case $unit in
	[0-9]|1[0-9]|2[0-9]|3[0-9]|4[0-9]|5[0-9]|6[0-3])
		/etc/mknod ${name}${unit}a  b $blk `expr $unit '*' 8 + 0`
		/etc/mknod ${name}${unit}b  b $blk `expr $unit '*' 8 + 1`
		/etc/mknod ${name}${unit}c  b $blk `expr $unit '*' 8 + 2`
		/etc/mknod ${name}${unit}g  b $blk `expr $unit '*' 8 + 6`
		/etc/mknod ${name}${unit}d  b $blk `expr $unit '*' 8 + 3`
		/etc/mknod ${name}${unit}e  b $blk `expr $unit '*' 8 + 4`
		/etc/mknod ${name}${unit}f  b $blk `expr $unit '*' 8 + 5`
		/etc/mknod ${name}${unit}h  b $blk `expr $unit '*' 8 + 7`
		/etc/mknod ${name}${unit}i  b $blk `expr $unit '*' 8 + 2048 + 0`
		/etc/mknod ${name}${unit}j  b $blk `expr $unit '*' 8 + 2048 + 1`
		/etc/mknod ${name}${unit}k  b $blk `expr $unit '*' 8 + 2048 + 2`
		/etc/mknod ${name}${unit}l  b $blk `expr $unit '*' 8 + 2048 + 3`
		/etc/mknod ${name}${unit}m  b $blk `expr $unit '*' 8 + 2048 + 4`
		/etc/mknod ${name}${unit}n  b $blk `expr $unit '*' 8 + 2048 + 5`
		/etc/mknod ${name}${unit}o  b $blk `expr $unit '*' 8 + 2048 + 6`
		/etc/mknod ${name}${unit}p  b $blk `expr $unit '*' 8 + 2048 + 7`
		/etc/mknod r${name}${unit}a c $chr `expr $unit '*' 8 + 0`
		/etc/mknod r${name}${unit}b c $chr `expr $unit '*' 8 + 1`
		/etc/mknod r${name}${unit}c c $chr `expr $unit '*' 8 + 2`
		/etc/mknod r${name}${unit}g c $chr `expr $unit '*' 8 + 6`
		/etc/mknod r${name}${unit}d c $chr `expr $unit '*' 8 + 3`
		/etc/mknod r${name}${unit}e c $chr `expr $unit '*' 8 + 4`
		/etc/mknod r${name}${unit}f c $chr `expr $unit '*' 8 + 5`
		/etc/mknod r${name}${unit}h c $chr `expr $unit '*' 8 + 7`
		/etc/mknod r${name}${unit}i c $chr `expr $unit '*' 8 + 2048 + 0`
		/etc/mknod r${name}${unit}j c $chr `expr $unit '*' 8 + 2048 + 1`
		/etc/mknod r${name}${unit}k c $chr `expr $unit '*' 8 + 2048 + 2`
		/etc/mknod r${name}${unit}l c $chr `expr $unit '*' 8 + 2048 + 3`
		/etc/mknod r${name}${unit}m c $chr `expr $unit '*' 8 + 2048 + 4`
		/etc/mknod r${name}${unit}n c $chr `expr $unit '*' 8 + 2048 + 5`
		/etc/mknod r${name}${unit}o c $chr `expr $unit '*' 8 + 2048 + 6`
		/etc/mknod r${name}${unit}p c $chr `expr $unit '*' 8 + 2048 + 7`
		/etc/mknod r${name}${unit}  c $chr `expr $unit '*' 8 + 63495`
		for j in a b c d e f g h i j k l m n o p; do
			chgrp daemon ${name}${unit}$j
			chmod 640 ${name}${unit}$j
		done
		for j in a b c d e f g h i j k l m n o p ""; do
			chgrp daemon r${name}${unit}$j
			chmod 640 r${name}${unit}$j
		done
		;;
	*)
		echo bad unit for disk in: $i
		;;
	esac
	;;

st*)
	case $i in
	st*) name=st; major=7;;
	esac
	unit=`expr $i : "$name\(.*\)"`
	case $unit in
	0) ch=h ;; 1) ch=i ;; 2) ch=j ;; 3) ch=k ;;
	4) ch=l ;; 5) ch=m ;; 6) ch=n ;; 7) ch=o ;;
	 8) ch=H ;;  9) ch=I ;; 10) ch=J ;; 11) ch=K ;;
	12) ch=L ;; 13) ch=M ;; 14) ch=N ;; 15) ch=O ;;
	*) echo bad unit for $name in: $i ;;
	esac
	case $ch in
	h|i|j|k|l|m|n|o|H|I|J|K|L|M|N|O)
		eval `echo $ch $unit $major |
		  awk ' { ch = $1; u = 16 * $2; m = $3 } END {
		    for (i = 0; i < 16; i++)
			printf("/etc/mknod tty%s%x c %d %d; ",ch,i,m,u+i); }'`
		;;
	esac
	;;

lp*)
	case $i in
	lp*) name=lp; major=16;;
	esac
	unit=`expr $i : "$name\(.*\)"`
	case $unit in
	0|1|2|3)
		umask 7
		eval `echo $unit $major |
	  		awk '{ u = 2 * $1; m = $2; } END {
	    		printf("/etc/mknod lp%d c %d %d; \
			      chgrp daemon lp%d; ", u,m,u,u);
	    		printf("/etc/mknod lp%d c %d %d; \
			      chgrp daemon lp%d; ", u+1,m,u+1,u+1); }'`
		umask 77
		;;
	*) echo bad unit for $name in: $i;;
	esac
	;;

sp*)
	unit=`expr $i : "..\(.*\)"`
	case $unit in
	0|1|2|3)
		umask 7
		/etc/mknod sp$unit c 25 $unit; \
		chgrp daemon sp$unit
		umask 77
		;;
	*) echo bad unit for sp in: $i;;
	esac
	;;

co*)
	unit=`expr $i : "co\(.*\)"`
	case $unit in
	[0-3])	offset=`expr $unit '*' 2`;;
	*) echo bad unit for co in: $i ;;
	esac
	case $unit in
	[0-3])	for port in 0 1; do
			realunit=`expr $offset + $port`
			/etc/mknod ttyc$realunit c 27 $realunit
			chmod 622 ttyc$realunit
		done
		;;
	esac
	;;

sc*)
	unit=`expr $i : "sc\(.*\)"`
	case $unit in
	0|1)
		/etc/mknod ttyb$unit c 24 $unit
		chmod 644 ttyb$unit
		;;
	*)
		echo bad unit for sc in: $i
		;;
	esac
	;;

pty*)
	class=`expr $i : 'pty\(.*\)'`
	case $class in
	0) offset=0  name=p;; 1) offset=62 name=q;;
	2) offset=124 name=r;; 3) offset=186 name=s;;
	4) offset=248 name=t;; 5) offset=310 name=u;;
	6) offset=372 name=v;; 7) offset=434 name=w;;
	8) offset=496 name=P;; 9) offset=558 name=Q;;
	10) offset=620 name=R;; 11) offset=682 name=S;;
	12) offset=744 name=T;; 13) offset=806 name=U;;
	14) offset=868 name=V;; 15) offset=930 name=W;;
	*) echo bad unit for pty in: $i;;
	esac
	case $class in
	0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15)
		umask 0
		eval `echo $offset $name | awk ' { b=$1; n=$2 } END {
			if (b == 0)
				printf("/etc/mknod getpty c 6 -1; ");
	m = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
			for (i = 0; i < 62; i++)
				printf("/etc/mknod tty%s%s c 5 %d; \
					/etc/mknod pty%s%s c 6 %d; ", \
					n, substr(m, i+1, 1), b+i, \
					n, substr(m, i+1, 1), b+i); }'`
		umask 77
		;;
	esac
	;;

pci)
	/etc/mknod eshare	c 14 1
	/etc/mknod email	c 14 2
	/etc/mknod eprint	c 14 3
	/etc/mknod eadmin	c 14 5
	/etc/mknod eremote	c 14 7
	chmod 600 eshare email eprint eadmin eremote
	;;
alm)
	if [ "`/etc/showcfg | grep 'MBAD  *0 .*\.01\.'`" != "" ]
	then
		echo "OLD REV MBAD, NO ALM SUPPORT -- CAN'T INSTALL ALM DEVICES"
		exit 1
	fi
	if /etc/showcfg | grep -s MBAD
	then :
	else
		echo "NO MBAD'S PRESENT -- CAN'T INSTALL ALM DEVICES"
		exit 1
	fi
	mkdir alm
	chmod 755 alm
	if [ "`pagesize`" -lt 4096 ]
	then
		alms="00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 \
			 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31"
	else
		alms="00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15"
	fi
	for i in $alms
	do
		/etc/mknod alm/alm$i c 11 $i
	done
	chmod 666 alm/alm*
	/etc/chown root alm alm/*
	chgrp daemon alm alm/*
	;;
usclk)
	/etc/mknod usclk c 19 0
	chmod 444 usclk
	/etc/chown root usclk
	chgrp daemon usclk
	;;
local)
	sh MAKEDEV.local
	;;
ctape|mtape)
	if /etc/showcfg | grep -i -s ssm; then
		sh MAKEDEV SSM_SA
	else
		sh MAKEDEV SCED_SA
	fi
	;;
#---
#--- The following section will make the appropriate device links
#--- for installation scripts.
#---
SCED_SA)
	rm -f ctape0 ctape8 mtape0 mtape8
	if [ -f "rts8" ]; then
		ln rts8 ctape8
	fi
	if [ -f "rts0" ]; then
		ln rts0 ctape0
	fi
	if [ -f "rmt8" ]; then
		ln rmt8 mtape0
	fi
	if [ -f "rmt12" ]; then
		ln rmt12 mtape8
	fi
	;;

SSM_SA)
	rm -f ctape0 ctape8 rts0 rts8 mtape0 mtape8
	if [ -f "rtm8" ]; then
		ln rtm8 ctape8
		ln rtm8 rts8
	fi
	if [ -f "rtm0" ]; then
		ln rtm0 ctape0
		ln rtm0 rts0
	fi
	if [ -f "rtg0" ]; then
		ln rtg0 mtape0
	fi
	if [ -f "rtg8" ]; then
		ln rtg8 mtape8
	fi
	;;
esac
done
_P_
/bin/cp tmpos/dev/MAKEDEV tmpos_att/dev/MAKEDEV
$chown     root tmpos/dev/MAKEDEV tmpos_att/dev/MAKEDEV &
$chgrp   daemon tmpos/dev/MAKEDEV tmpos_att/dev/MAKEDEV &
$chmod      555 tmpos/dev/MAKEDEV tmpos_att/dev/MAKEDEV &
wait
/bin/cat > tmpos/dev/MAKEDEV.local << '_P_'
_P_
/bin/cp tmpos/dev/MAKEDEV.local tmpos_att/dev/MAKEDEV.local
$chown     root tmpos/dev/MAKEDEV.local tmpos_att/dev/MAKEDEV.local &
$chgrp   daemon tmpos/dev/MAKEDEV.local tmpos_att/dev/MAKEDEV.local &
$chmod      555 tmpos/dev/MAKEDEV.local tmpos_att/dev/MAKEDEV.local &
wait
/bin/cat > tmpos/etc/.login << '_P_'
#################################################################
## This file is sourced at login time before .cshrc and .login
#################################################################
umask 022
_P_
$chown     root tmpos/etc/.login &
$chgrp   daemon tmpos/etc/.login &
$chmod      755 tmpos/etc/.login &
wait
/bin/cat > tmpos/etc/.profile << '_P_'
#################################################################
## This file is sourced at login time before $USER/.profile
#################################################################
umask 022
_P_
$chown     root tmpos/etc/.profile &
$chgrp   daemon tmpos/etc/.profile &
$chmod      755 tmpos/etc/.profile &
wait
/bin/cat > tmpos/etc/chk << '_P_'
for i in $*
do
	echo icheck $i
	icheck $i
	echo dcheck $i
	dcheck $i
done
_P_
$chown     root tmpos/etc/chk &
$chgrp   daemon tmpos/etc/chk &
$chmod      755 tmpos/etc/chk &
wait
/bin/cat > tmpos/etc/disktab << '_P_'
#
# Disk geometry and partition layout tables. 
# Key:
#	ty	type of disk
#	ns	#sectors/track
#	nt	#tracks/cylinder
#	nc	#cylinders/disk
#	rm	#revolutions per minute
#	ft	disk format type
#	p[a-h]	partition sizes in sectors
#	b[a-h]	partition block sizes in bytes
#	f[a-h]	partition fragment sizes in bytes
#

#
# Drives on SCSI (with Adaptec target adaptor)
#

# 3 physical cylinders are not available 
# because of bad blocks and diagnostic tracks.  The controller
# actually hides direct access to the bad blocks but
# the result is that the disk is logically shorter.
# Thus partitions are calculated as if the disk had
# "nc-3" cylinders.
#
vertex170|vertex|Vertex model 170 with interleave 1-1:\
	:ty=winchester:ns#17:nt#7:nc#987:rm#3600:\
	:ft=scsi:\
	:pa#15884:ba#8192:fa#1024:\
	:pb#33440:\
	:pc#117096:bc#8192:fc#1024:\
	:pd#15884:bd#8192:fd#1024:\
	:pe#51888:be#8192:fe#1024:\
	:pg#67772:bg#8192:fg#1024:
fujitsu2243|fujitsu|Fujitsu model 2243AS with interleave 1-1:\
	:ty=winchester:ns#17:nt#11:nc#754:rm#3600:\
	:ft=scsi:\
	:pa#15884:ba#8192:fa#1024:\
	:pb#33440:\
	:pc#140436:bc#8192:fc#1024:\
	:pd#15884:bd#8192:fd#1024:\
	:pe#75228:be#8192:fe#1024:\
	:pg#91112:bg#8192:fg#1024:
maxtor1140|maxtor140|maxtor|Maxtor model XT1140 with interleave 1-1:\
	:ty=winchester:ns#17:nt#15:nc#918:rm#3600:\
	:ft=scsi:\
	:pa#15884:ba#8192:fa#1024:\
	:pb#33440:\
	:pc#233324:bc#8192:fc#1024:\
	:pd#15884:bd#8192:fd#1024:\
	:pe#168116:be#8192:fe#1024:\
	:pg#184000:bg#8192:fg#1024:

#
# CCS drives on SCSI
#

# The last two physical cylinders on the disk are reserved for
# diagnostics.  Thus partitions are calculated as if the disk had
# "nc-2" cylinders.
#
wren3|Wren3|CDC Wren III 94161:\
	:ty=winchester:ns#35:nt#9:nc#965:rm#3600:\
	:ft=scsi:\
	:pa#15884:ba#8192:fa#1024:\
	:pb#33440:\
	:pc#303975:bc#8192:fc#1024:\
	:pd#15884:bd#8192:fd#1024:\
	:pe#238767:be#8192:fe#1024:\
	:pg#254651:bg#8192:fg#1024:
wren4|Wren4|CDC Wren IV 94171:\
	:ty=winchester:ns#54:nt#9:nc#1324:rm#3600:\
	:ft=scsi:\
	:pa#15884:ba#8192:fa#1024:\
	:pb#33440:\
	:pc#639332:bc#8192:fc#1024:\
	:pd#15884:bd#8192:fd#1024:\
	:pe#574124:be#8192:fe#1024:\
	:pg#590008:bg#8192:fg#1024:
microp1375|Microp1375|Micropolis 1375:\
	:ty=winchester:ns#35:nt#8:nc#1016:rm#3600:\
	:ft=scsi:\
	:pa#15884:ba#8192:fa#1024:\
	:pb#33440:\
	:pc#284480:bc#8192:fc#1024:\
	:pd#15884:bd#8192:fd#1024:\
	:pe#219272:be#8192:fe#1024:\
	:pg#235156:bg#8192:fg#1024:
m2246sa|M2246SA|Fujitsu 2246sa:\
	:ty=winchester:ns#33:nt#10:nc#819:rm#3600:\
	:ft=scsi:\
	:pa#15884:ba#8192:fa#1024:\
	:pb#33440:\
	:pc#270270:bc#8192:fc#1024:\
	:pd#15884:bd#8192:fd#1024:\
	:pe#205062:be#8192:fe#1024:\
	:pg#220946:bg#8192:fg#1024:
m2249sa|M2249SA|Fujitsu 2249sa:\
 	:ty=winchester:ns#35:nt#15:nc#1239:rm#3600:\
 	:ft=scsi:\
 	:pa#15884:ba#8192:fa#1024:\
 	:pb#33440:bb#8192:fb#1024:\
 	:pc#650475:bc#8192:fc#1024:\
 	:pd#15884:bd#8192:fd#1024:\
 	:pe#585267:be#8192:fe#1024:\
 	:pf#300575:bf#8192:ff#1024:\
 	:pg#601151:bg#8192:fg#1024:\
	:ph#300576:bh#8192:fh#1024:

#
# Drives on Multibus
#

# Partition 'c' really has 774640(46*20*842) blocks but
# we have to subtract 3 physical cylinders for bad blocks
# and diagnostics tracks, thus we use 771880(46*20*839).
#
eagle|Eagle|eagle46|Fujitsu Eagle with 46 sectors:\
	:ty=winchester:ns#46:nt#20:nc#842:rm#3961:\
	:ft=dec144:\
	:pa#15884:ba#8192:fa#1024:\
	:pb#66880:bb#8192:fb#1024:\
	:pc#771880:bc#8192:fc#1024:\
	:pd#15884:bd#8192:fd#1024:\
	:pe#307200:be#8192:fe#1024:\
	:pf#72680:bf#8192:ff#1024:\
	:pg#396520:bg#8192:fg#1024:\
	:ph#291346:bh#8192:fh#1024:

#
# Drives on ZDC
#
# 3 physical cylinders are not available.
# These are the first (cylinder 0) and the last 2 cylinders.
# Cylinder 0 is reserved and contains the disk description data
# and bad block lists. The last 2 cylinders are reserved for diagnostics.
# None of the partitions below include these cylinders.
# Thus partition sizes are calculated as if the disk had
# "nc-3" cylinders.
#

m2333k|M2333K|zdswallow|Fujitsu M2333K (Swallow) with 66 sectors:\
	:ty=winchester:ns#66:nt#10:nc#823:rm#3600:\
	:ft=zdc:\
	:pa#16500:ba#8192:fa#1024:\
	:pb#67320:bb#8192:fb#1024:\
	:pc#541200:bc#8192:fc#1024:\
	:pd#270600:bd#8192:fd#1024:\
	:pe#270600:be#8192:fe#1024:\
	:pf#236940:bf#8192:ff#1024:\
	:pg#236940:bg#8192:fg#1024:\
	:ph#220440:bh#8192:fh#1024:
m2351a|M2351A|zdeagle|Fujitsu M2351A (Eagle) with 46 sectors:\
	:ty=winchester:ns#46:nt#20:nc#842:rm#3961:\
	:ft=zdc:\
	:pa#16560:ba#8192:fa#1024:\
	:pb#67160:bb#8192:fb#1024:\
	:pc#771880:bc#8192:fc#1024:\
	:pd#385480:bd#8192:fd#1024:\
	:pe#385480:be#8192:fe#1024:\
	:pf#352360:bf#8192:ff#1024:\
	:pg#352360:bg#8192:fg#1024:\
	:ph#335800:bh#8192:fh#1024:
m2344k|M2344K|zdswallow4|Fujitsu M2344K (Swallow 4) with 66 sectors:\
	:ty=winchester:ns#66:nt#27:nc#624:rm#3600:\
	:ft=zdc:\
	:pa#17820:ba#8192:fa#1024:\
	:pb#67716:bb#8192:fb#1024:\
	:pc#1106622:bc#8192:fc#1024:\
	:pd#552420:bd#8192:fd#1024:\
	:pe#554202:be#8192:fe#1024:\
	:pf#518562:bf#8192:ff#1024:\
	:pg#520344:bg#8192:fg#1024:\
	:ph#500742:bh#8192:fh#1024:
m2382k|M2382K|zdswallow5|Fujitsu M2382K (Swallow 5) with 81 sectors:\
	:ty=winchester:ns#81:nt#27:nc#745:rm#3600:\
	:ft=zdc:\
	:pa#17496:ba#8192:fa#1024:\
	:pb#69984:bb#8192:fb#1024:\
	:pc#1622754:bc#8192:fc#1024:\
	:pd#811377:bd#8192:fd#1024:\
	:pe#811377:be#8192:fe#1024:\
	:pf#776385:bf#8192:ff#1024:\
	:pg#776385:bg#8192:fg#1024:\
	:ph#758889:bh#8192:fh#1024:
m2392k|M2392K|zdswallow6|Fujitsu M2392K (Swallow 6) with 81 sectors:\
	:ty=winchester:ns#81:nt#21:nc#1916:rm#3600:\
	:ft=zdc:\
	:pa#20412:ba#8192:fa#1024:\
	:pb#69741:bb#8192:fb#1024:\
	:pc#3254013:bc#8192:fc#1024:\
	:pd#1626156:bd#8192:fd#1024:\
	:pe#1627857:be#8192:fe#1024:\
	:pf#1602342:bf#8192:ff#1024:\
	:pg#1581930:bg#8192:fg#1024:\
	:ph#1581930:bh#8192:fh#1024:
cdc9715-340|CDC9715-340|CDC 9715-340 MB (FSD) with 34 sectors:\
	:ty=winchester:ns#34:nt#24:nc#711:rm#3600:\
	:ft=zdc:\
	:pa#17136:ba#8192:fa#1024:\
	:pb#67728:bb#8192:fb#1024:\
	:pc#577728:bc#8192:fc#1024:\
	:pd#288864:bd#8192:fd#1024:\
	:pe#288864:be#8192:fe#1024:\
	:pf#254592:bf#8192:ff#1024:\
	:pg#255408:bg#8192:fg#1024:\
	:ph#237456:bh#8192:fh#1024:
cdc9771-800|CDC9771-800|CDC 9771-800 MB (XMD) with 85 sectors:\
	:ty=winchester:ns#85:nt#16:nc#1024:rm#2160:\
	:ft=zdc:\
	:pa#16320:ba#8192:fa#1024:\
	:pb#68000:bb#8192:fb#1024:\
	:pc#1388560:bc#8192:fc#1024:\
	:pd#694960:bd#8192:fd#1024:\
	:pe#693600:be#8192:fe#1024:\
	:pf#659600:bf#8192:ff#1024:\
	:pg#660960:bg#8192:fg#1024:\
	:ph#643280:bh#8192:fh#1024:
cdc9720-850|CDC9720-850|CDC 9720-850 MB (EMD) with 68 sectors:\
	:ty=winchester:ns#68:nt#15:nc#1378:rm#3600:\
	:ft=zdc:\
	:pa#17340:ba#8192:fa#1024:\
	:pb#67320:bb#8192:fb#1024:\
	:pc#1405560:bc#8192:fc#1024:\
	:pd#702780:bd#8192:fd#1024:\
	:pe#702780:be#8192:fe#1024:\
	:pf#669120:bf#8192:ff#1024:\
	:pg#669120:bg#8192:fg#1024:\
	:ph#651780:bh#8192:fh#1024:
cdc9720-1230|CDC9720-1230|CDC 9720-1230 MB (EMD) with 83 sectors:\
	:ty=winchester:ns#83:nt#15:nc#1635:rm#3600:\
	:ft=zdc:\
	:pa#19920:ba#8192:fa#1024:\
	:pb#68475:bb#8192:fb#1024:\
	:pc#2031840:bc#8192:fc#1024:\
	:pd#1015920:bd#8192:fd#1024:\
	:pe#1015920:be#8192:fe#1024:\
	:pf#976080:bf#8192:ff#1024:\
	:pg#987285:bg#8192:fg#1024:\
	:ph#956160:bh#8192:fh#1024:
sabre5-1230|SABRE5-1230|UPDATED CDC 9720-1230 MB (EMD) with 83 sectors:\
	:ty=winchester:ns#83:nt#15:nc#1635:rm#3600:\
	:ft=zdc:\
	:pa#19920:ba#8192:fa#1024:\
	:pb#68475:bb#8192:fb#1024:\
	:pc#2031840:bc#8192:fc#1024:\
	:pd#1015920:bd#8192:fd#1024:\
	:pe#1015920:be#8192:fe#1024:\
	:pf#976080:bf#8192:ff#1024:\
	:pg#987285:bg#8192:fg#1024:\
	:ph#956160:bh#8192:fh#1024:
_P_
$chown     root tmpos/etc/disktab &
$chgrp   daemon tmpos/etc/disktab &
$chmod      644 tmpos/etc/disktab &
wait
/bin/cat > tmpos/etc/fstab << '_P_'
_P_
$chown     root tmpos/etc/fstab &
$chgrp   daemon tmpos/etc/fstab &
$chmod      644 tmpos/etc/fstab &
wait
/bin/cat > tmpos/etc/dumpdates << '_P_'
_P_
$chown     root tmpos/etc/dumpdates &
$chgrp   daemon tmpos/etc/dumpdates &
$chmod      644 tmpos/etc/dumpdates &
wait
/bin/cat > tmpos/etc/securetty << '_P_'
console
ttyc0
ttyc1
ttyb0
ttyb1
_P_
$chown     root tmpos/etc/securetty &
$chgrp   daemon tmpos/etc/securetty &
$chmod      640 tmpos/etc/securetty &
wait
/bin/cat > tmpos/etc/ftpusers << '_P_'
uucp
root
_P_
$chown     root tmpos/etc/ftpusers &
$chgrp   daemon tmpos/etc/ftpusers &
$chmod      644 tmpos/etc/ftpusers &
wait
/bin/cat > tmpos/etc/group << '_P_'
root:*:0:root
daemon:*:10:daemon
news:*:11:root
_P_
$chown     root tmpos/etc/group &
$chgrp   daemon tmpos/etc/group &
$chmod      644 tmpos/etc/group &
wait
/bin/cat > tmpos/etc/hosts << '_P_'
#
# Sample Host Database
#
127.1		localhost myhost
#
# Local Network stuff
#
97.0.0.01	host1 
97.0.0.02	host2
97.0.0.03	host3 
97.0.0.04	host4	gwhost
_P_
$chown     root tmpos/etc/hosts &
$chgrp   daemon tmpos/etc/hosts &
$chmod      644 tmpos/etc/hosts &
wait
/bin/cat > tmpos/etc/hosts.equiv << '_P_'
myfriendlyhost
yourfriendlyhost
_P_
$chown     root tmpos/etc/hosts.equiv &
$chgrp   daemon tmpos/etc/hosts.equiv &
$chmod      644 tmpos/etc/hosts.equiv &
wait
/bin/cat > tmpos/etc/motd << '_P_'
DYNIX(R)
Copyright 1984 Sequent Computer Systems, Inc.

_P_
$chown     root tmpos/etc/motd &
$chgrp   daemon tmpos/etc/motd &
$chmod      644 tmpos/etc/motd &
wait
/bin/cat > tmpos/etc/mtab << '_P_'
_P_
$chown     root tmpos/etc/mtab &
$chgrp   daemon tmpos/etc/mtab &
$chmod      644 tmpos/etc/mtab &
wait
/bin/cat > tmpos/etc/passwd << '_P_'
root::0:0:Admin:/:/bin/csh
nobody:*:-2:-2:::
daemon:*:1:10:Admin:/:
usrlimit:*:2:2:This is a XX user system, DO NOT REMOVE THIS LINE:/:/dev/null
uucp::66:10:UNIX-to-UNIX Copy:/usr/spool/uucppublic:/usr/lib/uucp/uucico
lp::70:10:System V Lp Admin,,,,universe(att):/usr/spool/lp:
service:*:75:0:Sequent Service,,,,:/usr/service:
_P_
#
# IMPORTANT!  Do not change "service" numeric uid below without also
# changing the hardcoded uids used to correctly chown the contents of
# /usr/service.  You will find these in the first script in this file.
# It is used for the creation of tmpos.
#
$chown     root tmpos/etc/passwd &
$chgrp   daemon tmpos/etc/passwd &
$chmod      644 tmpos/etc/passwd &
wait
/bin/cat > tmpos/etc/protocols << '_P_'
#
# Internet (IP) protocols
#
ip	0	IP	# internet protocol, pseudo protocol number
icmp	1	ICMP	# internet control message protocol
ggp	3	GGP	# gateway-gateway protocol
tcp	6	TCP	# transmission control protocol
pup	12	PUP	# PARC universal packet protocol
udp	17	UDP	# user datagram protocol
_P_
$chown     root tmpos/etc/protocols &
$chgrp   daemon tmpos/etc/protocols &
$chmod      644 tmpos/etc/protocols &
wait
/bin/cat > tmpos/etc/rc << '_P_'
echo "
*** Not set-up for Multi-User operation ***

You first need to check and change these files:
	/etc/rc
	/etc/rc.local
	/etc/rc.dumpstrings
	/etc/ttys
	/dev/MAKEDEV
	/etc/ttytype

Then delete the lines through the line \"exit 1\"
from \"/etc/rc\". ">/dev/console
exit 1
HOME=/; export HOME
PATH=/bin:/usr/bin
/etc/online -a > /dev/console 2>&1
if [ -f /etc/mirror ]
then	if /etc/mirror -a >/dev/console 2>&1
	then
		echo Mirroring started >/dev/console
	else
		echo Mirroring failure--get help! >/dev/console
		exit 1
	fi
fi
if [ -r /fastboot ]
then
	/bin/rm -f /fastboot
	echo Fast boot ... skipping disk checks >/dev/console
elif [ $1x = autobootx ]
then
	echo Automatic reboot in progress... >/dev/console
	date >/dev/console
	/etc/fsck -p >/dev/console
	case $? in
	0)
		date >/dev/console
		;;
	4)
		echo "Root fixed - rebooting" >/dev/console
		if [ -f /etc/mirror ]; then
			/etc/unmirror -a >/dev/console 2>&1
		fi
		/etc/reboot -q -n
		;;
	8)
		echo "Automatic reboot failed... help!" >/dev/console
		exit 1
		;;
	12)
		echo "Reboot interrupted" >/dev/console
		exit 1
		;;
	*)
		echo "Unknown error in reboot" > /dev/console
		exit 1
		;;
	esac
else
	date >/dev/console
fi
/bin/rm -f /etc/nologin

# attempt to rationally recover the passwd file if needed
if [ -s /etc/ptmp ]
then
	if [ -s /etc/passwd ]
	then
		ls -l /etc/passwd /etc/ptmp >/dev/console
		/bin/rm -f /etc/ptmp		# should really remove the shorter
	else
		echo 'passwd file recovered from ptmp' >/dev/console
		mv /etc/ptmp /etc/passwd
	fi
elif [ -r /etc/ptmp ]
then
	echo 'removing passwd lock file' >/dev/console
	/bin/rm -f /etc/ptmp
fi

/bin/sh /etc/rc.local

/usr/etc/swapon -a						>/dev/console

# syslogd needs to be started before the other deamons.
if [ -f /etc/syslogd ]; then
	/etc/syslogd & echo 'starting system logger.'	>/dev/console
fi
				echo preserving editor files 	>/dev/console
(cd /tmp; /usr/lib/ex3.7preserve -a)
if [ -r /etc/rc.sys5 ]
then
	att /bin/sh /etc/rc.sys5
fi
				echo clearing /tmp 		>/dev/console
(cd /tmp; /bin/rm -f - *)
				echo -n standard daemons:	>/dev/console
/etc/update &			echo -n ' update'		>/dev/console
/etc/cron &			echo -n ' cron'			>/dev/console
cd /usr/spool
/bin/rm -f uucp/LCK/LCK.* rwho/*
if [ -f /usr/lib/lpd ]; then
	/bin/rm -f /dev/printer /usr/spool/lpd.lock
	/usr/lib/lpd &		echo -n ' printer'		>/dev/console
fi
if [ -f /usr/adm/acct ]; then
	/usr/etc/accton /usr/adm/acct &	echo -n ' accounting'	>/dev/console
fi
				echo '.'			>/dev/console

cd /
echo -n starting network:					>/dev/console
echo -n ' pseudos'						>/dev/console
( 
  cd /dev
  files="`/bin/echo pty??`"
  /etc/chown root $files
  /bin/chmod 666 $files
  files="`/bin/echo tty[p-wP-W]?`"
  /etc/chown root $files
  /bin/chmod 666 $files
)
if [ -f /etc/inetd ]; then
	/etc/inetd & echo -n ' inet'				>/dev/console
fi
if [ -f /usr/etc/rwhod ]; then
	/usr/etc/rwhod & echo -n ' rwhod'			>/dev/console
fi
				echo '.'			>/dev/console

if [ -d /usr/pci ]; then
	(TERM=ansi export TERM;
	 echo -n starting PCI:					>/dev/console
	 cd /usr/pci
	 rm -f .m .mlock; echo -n ' locks'			>/dev/console
	 /usr/pci/eshare & echo -n ' disk'			>/dev/console
	 sleep 5
	 /usr/pci/email & echo -n ' mail'			>/dev/console
	 /usr/pci/eprint & echo -n ' printer'			>/dev/console
	 echo '.'						>/dev/console
	)
fi
				date				>/dev/console
exit 0
_P_
$chown     root tmpos/etc/rc &
$chgrp   daemon tmpos/etc/rc &
$chmod     0644 tmpos/etc/rc &
wait
/bin/cat > tmpos/etc/rc.local << '_P_'
/etc/umount -a 
> /etc/mtab
/etc/mount -f /
#
/bin/domainname mydomainname
/bin/hostname myhostname
# 
/etc/ifconfig lo0 localhost up
/etc/ifconfig se0 `hostname` up arp -trailers
/etc/mount -at 4.2						>/dev/console
##if [ -f /usr/etc/named ]; then
##	/usr/etc/named & echo  'starting domain nameserver.'		>/dev/console
##fi
/etc/umount -at nfs
echo -n 'starting rpc and net services:'			>/dev/console
if [ -f /etc/portmap ]; then
	/etc/portmap & echo -n ' portmap'			>/dev/console
fi
##if [ -f /usr/etc/ypserv -a -d /usr/etc/yp/`domainname` ]; then
##	/usr/etc/ypserv & echo -n ' ypserv'			>/dev/console
##fi
##if [ -f /etc/ypbind ]; then
##	/etc/ypbind & echo -n ' ypbind'				>/dev/console
##fi
if [ -f /etc/biod ]; then
	/etc/biod 8 & echo -n ' biod'				>/dev/console
fi
echo '.'							>/dev/console
/etc/mount -vat nfs						>/dev/console
#echo -n 'check quotas: '					>/dev/console
#	/usr/etc/quotacheck -a
#echo 'done.'							>/dev/console
#/usr/etc/quotaon -a

/bin/rm -f /tmp/t1
/etc/dmesg | /usr/bin/egrep DYNIX | /usr/ucb/tail -1 >/tmp/t1
/bin/grep -v "^DYNIX(R)" /etc/motd >>/tmp/t1
cat /tmp/t1 > /etc/motd
/usr/etc/savecore /usr/crash					>/dev/console 2>&1
/bin/sh /etc/rc.dumpstrings					>/dev/console

echo -n 'local daemons:'					>/dev/console

# routed does not work with pre-3.0 versions of routed on the same
# network.  In general, it is not needed with flat networks (ie, no gateways)

##if [ -f /usr/etc/routed ]; then
##	/usr/etc/routed & echo -n ' routed'			>/dev/console
##fi
if [ -f /usr/etc/uucpd ]; then
	/usr/etc/uucpd & echo -n ' uucpd'			>/dev/console
fi
if [ -f /usr/etc/sweepd ]; then
	/usr/etc/sweepd & echo -n ' sweepd'			>/dev/console
fi
if [ -f /usr/etc/42talkd ]; then
	/usr/etc/42talkd & echo -n ' 42talkd'				>/dev/console
fi
if [ -f /usr/lib/sendmail ]; then
	(cd /usr/spool/mqueue; /bin/rm -f lf* nf*)
	/usr/lib/sendmail -bd -q1h & echo -n ' sendmail'	>/dev/console
fi
##
## if nfs daemon exists and /etc/exports file exists become nfs server
##
## The number of nfsd daemons should be tuned based on server load
## and the number of processors.
##
if [ -f /etc/nfsd -a -f /etc/exports ]; then
	/etc/nfsd 8 & echo -n ' nfsd'				>/dev/console
fi
echo '.'							>/dev/console
_P_
$chown     root tmpos/etc/rc.local &
$chgrp   daemon tmpos/etc/rc.local &
$chmod      644 tmpos/etc/rc.local &
wait

/bin/cat > tmpos/etc/rc.dumpstrings << '_P_'
KERNEL="sd(0,0)dynix"
#KERNEL="wd(0,0)dynix"
#KERNEL="xp(0,0)dynix"
#KERNEL="zd(0,0)dynix"
DUMPER="sd(0,0)stand/dump sd(0,1) 4000 /dev/rsd0b"
#DUMPER="wd(0,0)stand/dump wd(0,1) 4000 /dev/rwd0b"
#DUMPER="xp(0,0)stand/dump xp(0,1) 8000 /dev/rxp0b"
#DUMPER="zd(0,0)stand/dump zd(0,1) 8000 /dev/rzd0b"
TEXTSIZE=`/bin/size /dynix | /bin/awk 'NR==2{print $1}'`
/etc/bootflags -p -c n0="$KERNEL" n1="$DUMPER" ra1="$TEXTSIZE"	
_P_
$chown     root tmpos/etc/rc.dumpstrings &
$chgrp   daemon tmpos/etc/rc.dumpstrings &
$chmod      644 tmpos/etc/rc.dumpstrings &

/bin/cat > tmpos/etc/rc.single << '_P_'
eval `/bin/grep "/bin/domainname" /etc/rc.local`
eval `/bin/grep "/bin/hostname" /etc/rc.local`
_P_
$chown     root tmpos/etc/rc.single &
$chgrp   daemon tmpos/etc/rc.single &
$chmod      644 tmpos/etc/rc.single &
wait
/bin/cat > tmpos/etc/rc.shutdown << '_P_'
# /etc/rc.shutdown.  Called from /etc/shutdown, /etc/halt and
# /etc/reboot, each time with a single parameter.  From /etc/shutdown,
# it is called twice, the first time with the parameter "warn" if
# -h or -r or -k are not specified , then SIGTERM is sent to all processes,
# and /etc/rc.shutdown is called a second time ,with the parameter "shutdown".
# It is called from /etc/halt with the
# parameter "halt", and from /etc/reboot with the parameter "reboot",
# in both cases after all processes have been killed (or killed as
# much as they can be).

if [ $# -lt 1 ]
then	echo $0 called with wrong number of parameters
	exit 1
fi

nosync=0			# don't sync the disks
type=$1
shift
while [ $# -gt 0 ]; do
	case $1 in
	-*n*)	nosync=1
		;;
	*)
		;;
	esac
	shift
done

case $type in
warn)
	;;
shutdown)
	if [ -f /usr/adm/acct ]; then
		/usr/etc/accton >/dev/console 2>&1	# turn off accounting
	fi
	if [ $nosync -eq 0 ]; then
		/etc/umount -t 4.2
	fi
	if	[ -f /etc/mirror ]; then	
		if [ $nosync -eq 1 ]; then
			/etc/umount -t 4.2	# umount needed for unmirror
		fi
		/etc/unmirror -a
	fi
	;;
halt)
	if [ -f /usr/adm/acct ]; then
		/usr/etc/accton >/dev/console 2>&1	# turn off accounting
	fi
	if [ $nosync -eq 0 ]; then
		/etc/umount -t 4.2
	fi
	if	[ -f /etc/mirror ]; then	
		if [ $nosync -eq 1 ]; then
			/etc/umount -t 4.2	# umount needed for unmirror
		fi
		/etc/unmirror -a
	fi
	;;
reboot)
	if [ -f /usr/adm/acct ]; then
		/usr/etc/accton >/dev/console 2>&1	# turn off accounting
	fi
	if [ $nosync -eq 0 ]; then
		/etc/umount -t 4.2
	fi
	if	[ -f /etc/mirror ]; then
		if [ $nosync -eq 1 ]; then
			/etc/umount -t 4.2	# umount needed for unmirror
		fi
		/etc/unmirror -a
	fi
	;;
*)	echo $0 called with non-standard parameter '"'$1'"'
	exit 1
	;;
esac
_P_
$chown     root tmpos/etc/rc.shutdown &
$chgrp   daemon tmpos/etc/rc.shutdown &
$chmod      644 tmpos/etc/rc.shutdown &
wait
/bin/cat > tmpos/etc/remote << '_P_'
#
# General dialer definitions used below
#
dial1200|1200 Baud Ventel attributes:\
	:dv=/dev/cul0:br#1200:cu=/dev/cul0:at=ventel:du:
dial300|300 Ventel attributes:\
	:dv=/dev/cul0:br#300:cu=/dev/cul0:at=ventel:du:
#
# UNIX system definitions
#
UNIX-1200|1200 Baud dial-out to another UNIX system:\
	:el=^U^C^R^O^D^S^Q@:ie=#%$:oe=^D:tc=dial1200:
UNIX-300|300 Baud dial-out to another UNIX system:\
	:el=^U^C^R^O^D^S^Q@:ie=#%$:oe=^D:tc=dial300:
tip0|tip1200:tc=UNIX-1200:
tip300:tc=UNIX-300:
cu0|cu300:tc=UNIX-300:
cu1200:tc=UNIX-1200:
dialer:dv=/dev/cul0:br#1200:

arpa:pn=2-7750:tc=UNIX-1200:
#--------------------------------------------------------------------
#The attributes are:
#
#dv	device to use for the tty
#el	EOL marks (default is NULL)
#du	make a call flag (dial up)
#pn	phone numbers (@ =>'s search phones file; possibly taken from
#			      PHONES environment variable)
#at	ACU type
#ie	input EOF marks	(default is NULL)
#oe	output EOF string (default is NULL)
#cu	call unit (default is dv)
#br	baud rate (defaults to 300)
#fs	frame size (default is BUFSIZ) -- used in buffering writes
#	  on receive operations
#tc	to continue a capability
_P_
$chown     root tmpos/etc/remote &
$chgrp   daemon tmpos/etc/remote &
$chmod      644 tmpos/etc/remote &
wait
/bin/cat > tmpos/etc/services << '_P_'
#
# Network services, Internet style
# This file is never consulted when the yellow pages are running
#
echo		7/udp
echo		7/tcp
discard		9/udp		sink null
discard		9/tcp		sink null
systat		11/tcp
daytime		13/udp
daytime		13/tcp
netstat		15/tcp
chargen		19/udp
chargen		19/tcp
ftp		21/tcp
telnet		23/tcp
smtp		25/tcp		mail
time		37/udp		timserver
time		37/tcp		timserver
name		42/tcp		nameserver
whois		43/tcp		nicname
domain		53/udp
domain		53/tcp
mtp		57/tcp				# deprecated
hostnames	101/tcp		hostname
sunrpc		111/udp
sunrpc		111/tcp
#
# Host specific functions
#
bootps		67/udp				# bootp server
bootpc		68/udp				# bootp client
tftp		69/udp
rje		77/tcp
finger		79/tcp
link		87/tcp		ttylink
supdup		95/tcp
uucp		540/tcp		uucpd		# uucp daemon
ingreslock	1524/tcp
#
# UNIX specific services
#
exec		512/tcp
login		513/tcp
shell		514/tcp		cmd		# no passwords used
printer		515/tcp		spooler		# experimental
efs		520/tcp
courier		530/tcp		rpc		# experimental
biff		512/udp		comsat
who		513/udp		whod
syslog		514/udp
talk		517/udp
ntalk		518/udp
route		520/udp		router routed	# 521 also
timed           525/udp		timeserver
new-rwho	550/udp		new-who		# experimental
rmonitor	560/udp		rmonitord	# experimental
monitor		561/udp				# experimental
_P_
$chown     root tmpos/etc/services &
$chgrp   daemon tmpos/etc/services &
$chmod      644 tmpos/etc/services &
wait
/bin/cat > tmpos/etc/syslog.conf << '_P_'
#kern,mark.debug		/dev/console
*.notice;mail.info	/usr/spool/adm/syslog
*.crit			/usr/adm/critical
user.info		/usr/spool/adm/syslog
*.info			/usr/spool/adm/syslog
*.err			/usr/spool/adm/syslog
_P_
$chown   daemon tmpos/etc/syslog.conf &
$chgrp   daemon tmpos/etc/syslog.conf &
$chmod      644 tmpos/etc/syslog.conf &
wait
/bin/cat > tmpos/etc/syslog.pid << '_P_'
_P_
$chown   daemon tmpos/etc/syslog.pid &
$chgrp   daemon tmpos/etc/syslog.pid &
$chmod      644 tmpos/etc/syslog.pid &
wait
/bin/cat > tmpos/usr/spool/adm/syslog<< '_P_'
_P_
$chown   daemon tmpos/usr/spool/adm/syslog &
$chgrp   daemon tmpos/usr/spool/adm/syslog &
$chmod      644 tmpos/usr/spool/adm/syslog &
wait
/bin/cat > tmpos/usr/adm/critical<< '_P_'
_P_
$chown   daemon tmpos/usr/adm/critical &
$chgrp   daemon tmpos/usr/adm/critical &
$chmod      644 tmpos/usr/adm/critical &
wait
/bin/cat > tmpos/etc/ttys << '_P_'
1?console
02ttyp0
02ttyp1
02ttyp2
02ttyp3
02ttyp4
02ttyp5
02ttyp6
02ttyp7
02ttyp8
02ttyp9
02ttypa
02ttypb
02ttypc
02ttypd
02ttype
02ttypf
02ttypg
02ttyph
02ttypi
02ttypj
02ttypk
02ttypl
02ttypm
02ttypn
02ttypo
02ttypp
02ttypq
02ttypr
02ttyps
02ttypt
02ttypu
02ttypv
02ttypw
02ttypx
02ttypy
02ttypz
02ttypA
02ttypB
02ttypC
02ttypD
02ttypE
02ttypF
02ttypG
02ttypH
02ttypI
02ttypJ
02ttypK
02ttypL
02ttypM
02ttypN
02ttypO
02ttypP
02ttypQ
02ttypR
02ttypS
02ttypT
02ttypU
02ttypV
02ttypW
02ttypX
02ttypY
02ttypZ
_P_
$chown     root tmpos/etc/ttys &
$chgrp   daemon tmpos/etc/ttys &
$chmod      644 tmpos/etc/ttys &
wait
/bin/cat > tmpos/etc/ttytype << '_P_'
unknown console
network ttyp0
network ttyp1
network ttyp2
network ttyp3
network ttyp4
network ttyp5
network ttyp6
network ttyp7
network ttyp8
network ttyp9
network ttypa
network ttypb
network ttypc
network ttypd
network ttype
network ttypf
network ttypg
network ttyph
network ttypi
network ttypj
network ttypk
network ttypl
network ttypm
network ttypn
network ttypo
network ttypp
network ttypq
network ttypr
network ttyps
network ttypt
network ttypu
network ttypv
network ttypw
network ttypx
network ttypy
network ttypz
network ttypA
network ttypB
network ttypC
network ttypD
network ttypE
network ttypF
network ttypG
network ttypH
network ttypI
network ttypJ
network ttypK
network ttypL
network ttypM
network ttypN
network ttypO
network ttypP
network ttypQ
network ttypR
network ttypS
network ttypT
network ttypU
network ttypV
network ttypW
network ttypX
network ttypY
network ttypZ
_P_
$chown     root tmpos/etc/ttytype &
$chgrp   daemon tmpos/etc/ttytype &
$chmod      644 tmpos/etc/ttytype &
wait
/bin/cat > tmpos/etc/utmp << '_P_'
_P_
$chown     root tmpos/etc/utmp &
$chgrp   daemon tmpos/etc/utmp &
$chmod      644 tmpos/etc/utmp &
wait
/bin/cat > tmpos/etc/networks << '_P_'
#
# Internet networks (reordered for local efficiency)
#
loopback-net	127		software-loopback-net
sample-net	 97		sample-in-house-network
other-net	 98		other-in-house-network
_P_
$chown     root tmpos/etc/networks &
$chgrp   daemon tmpos/etc/networks &
$chmod      644 tmpos/etc/networks &
wait
/bin/cat > tmpos/etc/gateways << '_P_'
net other-net gateway gwhost metric 3 passive
_P_
$chown     root tmpos/etc/gateways &
$chgrp   daemon tmpos/etc/gateways &
$chmod      644 tmpos/etc/gateways &
wait
/bin/cat > tmpos/etc/buildmini << '_P_'
#! /bin/sh
#
# /etc/buildmini miniroot (e.g. zd1b) distroot 
#
# Used to build a distribution tape.
# Ties in with /etc/maketape.
#
if [ "$#" -eq 2 ] ;then
	MINIROOT=$1
	DISTROOT=$2
else
	echo 'usage: buildmini miniroot (e.g. zd1b) distroot'
	exit 1
fi
#
date
miniroot=/miniroot$$
/etc/umount /dev/${MINIROOT} 2>/dev/null
#
# v3.1.1 currently uses 3022 blocks and 350 inodes
# each cylinder is 32*16=512 blocks
# 14 cylinders=7168 blocks
# 15 cylinders=7680 blocks etc.
# 25 cylinders=12800
#  9216 bytes per inode is 416 inodes for 15 cylinders
#  9216 bytes per inode is 690 inodes for 25 cylinders
# Note mkfs on 3.0.17 dosn't get the number of bytes per inode right.
#
			# WARNING: don't change the following sector
			# count (12800) without also changing the size
			# that is dd'ed to tape in "tmpos/etc/maketape".
/etc/mkfs /dev/${MINIROOT} 12800 32 16 4096 512 16 0 60 9216
/etc/fsck /dev/r${MINIROOT}
mkdir $miniroot
/etc/mount /dev/${MINIROOT} $miniroot
if [ $? != 0 ]
then
	echo 'mount failed. Exiting'
	exit 1
fi
cd $miniroot
#
if [ `pwd` = '/' ]
then
	echo You just '(almost)' destroyed the root
	exit 1
fi
rm -rf bin; mkdir bin
rm -rf dev; mkdir dev
rm -rf etc; mkdir etc; mkdir etc/vtoc; mkdir etc/diskinfo
rm -rf tmp; mkdir tmp
rm -rf stand; mkdir stand 
rm -rf usr; mkdir usr; mkdir usr/ucb; mkdir usr/etc
rm -rf ssm; mkdir ssm
cp $DISTROOT/bin/awk bin
cp $DISTROOT/bin/cat bin
cp $DISTROOT/bin/chgrp bin
cp $DISTROOT/bin/chmod bin
cp $DISTROOT/bin/cmp bin
cp $DISTROOT/bin/cp bin
cp $DISTROOT/bin/csh bin
cp $DISTROOT/bin/date bin
cp $DISTROOT/bin/dd bin
cp $DISTROOT/bin/df bin
cp $DISTROOT/bin/diff bin
cp $DISTROOT/bin/du bin
cp $DISTROOT/bin/echo bin
cp $DISTROOT/bin/ed bin
cp $DISTROOT/bin/expr bin
cp $DISTROOT/bin/false bin
cp $DISTROOT/bin/grep bin
cp $DISTROOT/bin/ln bin
cp $DISTROOT/bin/ls bin
cp $DISTROOT/bin/make bin
cp $DISTROOT/bin/mkdir bin
cp $DISTROOT/bin/mt bin
cp $DISTROOT/bin/mv bin
cp $DISTROOT/bin/od bin
cp $DISTROOT/bin/pagesize bin
cp $DISTROOT/bin/ps bin
cp $DISTROOT/bin/pwd bin
cp $DISTROOT/bin/rm bin
cp $DISTROOT/bin/rmdir bin
cp $DISTROOT/bin/sed bin
cp $DISTROOT/bin/sh bin
cp $DISTROOT/bin/stty bin; ln bin/stty bin/STTY
cp $DISTROOT/bin/sync bin
cp $DISTROOT/bin/tar bin
cp $DISTROOT/bin/test bin; ln bin/test bin/\[
cp $DISTROOT/bin/true bin
# cp $DISTROOT/ssm/* ssm !!! needs to be fixed
cp $DISTROOT/dev/MAKEDEV dev
> dev/MAKEDEV.local
( cd dev
./MAKEDEV std \
	  sd0 sd1 sd2 \
	  wd0 wd1 wd2 \
	  st0 st1 \
	  ts0 \
	  tm0 \
	  xt0 xp0 zd0) &
cp $DISTROOT/etc/format etc/format
cp $DISTROOT/etc/chown etc
cp $DISTROOT/etc/disktab etc
cp $DISTROOT/etc/dump etc
cp $DISTROOT/etc/fsck etc
cp $DISTROOT/etc/halt etc
cp $DISTROOT/etc/init etc
cp $DISTROOT/etc/mknod etc
cp $DISTROOT/etc/mkvtoc etc
cp $DISTROOT/etc/prtvtoc etc
cp $DISTROOT/etc/mount etc
cp $DISTROOT/etc/newfs etc
ln etc/newfs etc/mkfs
cp $DISTROOT/etc/fsirand etc
cp $DISTROOT/etc/online etc
ln etc/online etc/offline
cp $DISTROOT/etc/reboot etc
cp $DISTROOT/etc/restore etc
cp $DISTROOT/etc/restore.root etc
cp $DISTROOT/etc/restore.oldroot etc
cp $DISTROOT/etc/restore.oldmore etc
cp $DISTROOT/etc/showcfg etc
cp $DISTROOT/etc/rc.shutdown etc
cp $DISTROOT/etc/termcap etc
cp $DISTROOT/etc/umount etc
cp $DISTROOT/etc/installboot etc
cp $DISTROOT/etc/format etc/format
cp $DISTROOT/stand/boot?? stand
cp $DISTROOT/stand/boot??_v stand
cp $DISTROOT/boot .
cp $DISTROOT/usr/etc/savecore usr/etc
cp $DISTROOT/usr/ucb/vi usr/ucb
cp $DISTROOT/etc/vtoc/* etc/vtoc
cp $DISTROOT/etc/diskinfo/* etc/diskinfo
cp $DISTROOT/.profile .
cp $DISTROOT/gendynix .
cat >etc/passwd <<EOF
root::0:0::/:/bin/sh
daemon::1:10::/:
EOF
cat >etc/group <<EOF
root:*:0:root
daemon:*:10:root
EOF
cat >etc/mtab <<EOF
EOF
wait
sync
cd /; sync
/etc/umount $miniroot
/etc/fsck /dev/r${MINIROOT}
rm -rf $miniroot
date
_P_
$chown     root tmpos/etc/buildmini &
$chgrp   daemon tmpos/etc/buildmini &
$chmod      744 tmpos/etc/buildmini &
wait
/bin/cat > tmpos/etc/maketape << '_P_'
#! /bin/sh
#
# maketape miniroot tmpos_root tmpos_usr distroot
#
# For example:
#   maketape zd1b zd1a zd1h distroot
#
# Used to make a distribution tape. Ties in with /etc/buildmini.
#
if [ "$#" -ne "4" ] ;then
	echo 'usage: /etc/maketape miniroot tmpos_root tmpos_usr distroot'
	echo '   example: /etc/maketape zd1b zd1a zd1h distroot'
	exit 1
else
	MINIROOT=$1
	TMPOS_ROOT=$2
	TMPOS_USR=$3
	DISTROOT=$4
fi
CK1=/tmp/checksum.base1		# checksum file for first tape of base
CK2=/tmp/checksum.base2		# checksum file for second tape of base
ID=/tmp/sqnt_tapeid2		# null file used to ID 2nd tape of base
partition1=/dev/${TMPOS_ROOT}	# /
partition2=/dev/${TMPOS_USR}	# /usr
tape=/dev/ctape8		# do not rewind
rewindtape="mt -f $tape rewind"
erasetape="mt -f $tape erase"
if [ ! -f /dev/ctape8 ]; then
	ln /dev/rts8 /dev/ctape8
fi
#
$erasetape
$rewindtape
date
/etc/umount $DISTROOT/usr
/etc/umount $DISTROOT
/etc/mount $partition1 $DISTROOT
rm -rf $DISTROOT/lost+found
/etc/mount $partition2 $DISTROOT/usr
cat > /tmp/tape.info$$ << 'BLAH'
TAPE 1
tapefile	what			format
--------	----			------
0		/stand/bootts		dd
1		/boot			dd
2		tape.info		ascii
3		/stand/cat		dd
4		/stand/copy		dd
5		/stand/copy2		dd
6		/stand/dump		dd
7		/stand/formatscsi	dd
8		/stand/fsck		dd
9 		/stand/ls		dd
10		/stand/wdformat		dd
11		/stand/xpformat		dd
12		/stand/zdformat		dd
13		/stand/sdformat		dd
14		mini-root file sys	dd
15		real root file sys	dump

TAPE 2
tapefile	what			format
--------	----			------
16	        /tmp/sqnt_tapeid2       tar (zero length)	
17		most of /usr file sys	tar
18		unformatted man pages	tar
19		/usr/doc		tar
20		/usr/games		tar
21		/usr/lib/vfont		tar
22		/usr/local/gnu		tar
BLAH
cd $DISTROOT/stand
echo "Checksum for tape 1 of base " >$CK1
#
# Make sure tape boot loader is first tape file.
#
dd if=bootts		of=$tape bs=512 conv=sync	# file 0
dd if=bootts		bs=512 conv=sync | sum >>$CK1
dd if=../boot		of=$tape bs=512 conv=sync	# file 1
dd if=../boot		bs=512 conv=sync | sum >>$CK1
dd if=/tmp/tape.info$$	of=$tape			# file 2
dd if=/tmp/tape.info$$  bs=512 conv=sync | sum >>$CK1
dd if=cat		of=$tape			# file 3
sum cat >>$CK1
dd if=copy		of=$tape			# file 4
sum copy >>$CK1
dd if=copy2		of=$tape			# file 5
sum copy2 >>$CK1
dd if=dump		of=$tape			# file 6
sum dump >>$CK1
dd if=formatscsi	of=$tape			# file 7
sum formatscsi >>$CK1
dd if=fsck		of=$tape			# file 8
sum fsck >>$CK1
dd if=ls		of=$tape			# file 9 
sum ls >>$CK1
dd if=wdformat		of=$tape			# file 10
sum wdformat >>$CK1
dd if=xpformat		of=$tape			# file 11
sum xpformat >>$CK1
dd if=zdformat		of=$tape			# file 12
sum zdformat >>$CK1
dd if=sdformat		of=$tape			# file 13
sum sdformat >>$CK1
rm /tmp/tape.info$$
sync
echo "** Add dump of mini-root file system"
size=6400	# WARNING: size calculated from the sector count when
		# running mkfs on the miniroot.  Do not change one
		# without the other.  Currently 12800 sectors, at
		# 512 bytes/sec this make 6400 1k blocks.  See comment
		# in tmpos/etc/buildmini.
dd if=/dev/r${MINIROOT} bs=1k count=$size conv=sync | tee $tape |
	sum >>$CK1 # file 14
echo "** Add full dump of real file system"
/etc/dump 0uf - ${partition1} | tee $tape | sum >>$CK1   # file 15

list=` /bin/ls $DISTROOT/usr | egrep -v 'doc|games|lib|local|man' `
list_local=`/bin/ls $DISTROOT/usr/local | egrep -v 'gnu' `
list_local=`for i in $list_local ;do echo local/$i ;done `
list_lib=`/bin/ls $DISTROOT/usr/lib | egrep -v 'vfont$' `
list_lib=`for i in $list_lib ;do echo lib/$i ;done `
list="$list $list_local $list_lib"
catlist=` /bin/ls $DISTROOT/usr/man | egrep -v 'man*' `
manlist=` /bin/ls $DISTROOT/usr/man | egrep -v 'cat*' `
mlist=`for file in $manlist ;do echo man/$file ;done`
clist=`for file in $catlist ;do echo man/$file ;done`

echo "** Done with tape 1. Rewinding."
$rewindtape
echo -n "** Ready for second tape.  Hit return when second tape in drive:"
read JUNK
echo ""
echo "** Erasing and rewinding second tape "
$erasetape
$rewindtape

echo "Checksum for tape 2 of base " >$CK2

echo "** Add tar file for second tape ID"
rm -f $ID
cp /dev/null $ID
tar cf - $ID | tee $tape | sum >>$CK2		   # file 16

echo "** Add tar image of most of /usr"
cd $DISTROOT/usr && tar cf - $list $clist | tee $tape | sum >>$CK2 # file 17
echo "** Add tar image of unformatted man pages"
cd $DISTROOT/usr && tar cf - $mlist       | tee $tape | sum >>$CK2 # file 18
echo "** Add tar image of /usr/doc"
cd $DISTROOT/usr && tar cf - doc          | tee $tape | sum >>$CK2 # file 19
echo "** Add tar image of /usr/games"
cd $DISTROOT/usr && tar cf - games        | tee $tape | sum >>$CK2 # file 20
echo "** Add tar image of varian fonts"
cd $DISTROOT/usr/lib && tar cf - vfont    | tee $tape | sum >>$CK2 # file 21
echo "** Add tar image of /usr/local/gnu"
cd $DISTROOT/usr/local && tar cf - gnu	  | tee $tape | sum >>$CK2 # file 22
echo "Done with second tape. Rewinding."
$rewindtape
_P_
$chown     root tmpos/etc/maketape &
$chgrp   daemon tmpos/etc/maketape &
$chmod      744 tmpos/etc/maketape &
wait
/bin/cat > tmpos/etc/restore.root << '_P_'
#! /bin/sh
#
# restore.root
#
# This shell script is used during initial system load from
# tape. It prompts the user for affirmative replies prior to
# loading files from tape.
#
/etc/online -a
/bin/echo 'Build the root file system'
(cd /dev;./MAKEDEV ctape) >/dev/null 2>&1
TAPE=/dev/ctape8
TFILE=16
if [ "$TFILE" != 16 ]
then
	/bin/echo -n "tape drive with root dump on (eg $TAPE) "
	read TAPE
fi
	 
/bin/mt -f $TAPE noret >/dev/null 2>&1
/bin/mt -f $TAPE rewind &
#
# get the type of disk
#
if [ "$1" = "" ]
then
	/bin/echo -n 'disk name for root? (e.g. sd0, wd0, xp0 or zd0) '
	read DISK
else
	DISK=$1
	echo "disk name for root is $DISK"
	shift
fi

(cd /dev;./MAKEDEV $DISK) 2>/dev/null

TYPE=`format -i r$DISK 2>/dev/null`
if [ $? != 0 -o "$TYPE" = "" ]
then
	TYPES=`(cd /etc/diskinfo;echo *.geom | sed -e 's/\.geom/, /g')`
	until	/bin/echo -n "disk type? (e.g. $TYPES) "
		read TYPE
		/bin/grep -w -s "$TYPE" /etc/diskinfo/*
		case "$?" in 
		0)	;;	*)	false;;
		esac
	do
	/bin/echo 'unknown disk type
'
	done
else
	echo "$DISK is a $TYPE"
fi

case $DISK in 
zd*)
	DISC=zd
	if [ "$DISK" != "zd0" ] ;then
		echo -n "Drive number on controller? (0,1,...15) "
		read drivenum
		echo -n "Controller number for drive? (0,1,2,3,4) "
		read controller
		#
		# zd unit composed of following bits:
		#
		# 0-3 drive number on controller
		# 4-6 controller number
		controller=`expr $controller \* 16`
		diskunit=`expr $controller + $drivenum`
	else
		diskunit=0
	fi
	;;
xp*)
	DISC=xp
	if [ "$DISK" != "xp0" ] ;then
		echo -n "Drive number on controller? (0,1,2,3) "
		read drivenum
		echo -n "Controller number for drive? (0,1,2,3) "
		read controller
		echo -n "Drive type - index into xpst table? (normally 0 for eagle) "
		read drivetype
		echo -n "Multibus adapter number for controller? "
		read mbad
		#
		# xp unit composed of following bits:
		#	
		#  0-2	drive number on controller
		#  3-5	controller number
		#  6-8	drive type - index into xpst table
		#  9-11	multibus adaptor number
		controller=`expr $controller \* 8`
		drivetype=`expr $drivetype \* 64`
		mbad=`expr $mbad \* 512`
		diskunit=`expr $mbad + $drivetype + $controller + $drivenum`
	else
		diskunit=0
	fi
	;;
sd*|wd*)	
	DISC=`expr $DISK : '\(..\)'`
	UNIT=`expr $DISK : '..\(.*\)'`
	if [ "$UNIT" != "0" ] ;then
		case "$TYPE" in
		wren*|microp1375|m2246sa|m2249sa|hp97548|hp97544) # embedded scsi
			unitnum=0	# always zero for embedded SCSI disk
			echo -n "Target number select? (0,1,...,7) "
			read ta
			;;
		*)					# adaptec scsi
			echo -n "Unit number of the disk? (0,1,...,7) "
			read unitnum
			echo -n "Controller select (target adapter)? (0,1,...,7) "
			read ta
			;;
		esac
		echo -n "Drive type? (normally 0) (0,1,...,7) "
		read drivetype
		echo -n "SCED board number? (0,1,...,7) "
		read sced
		#
		# sd and wd unit number composed of following bits:
		#
 		# 0-2: unit number on the hardware (up to 8)
 		# 3-5: controller select (target adapter) (up to 8)
 		# 6-8: drive  type (up to 8), index into configuration table 
 		# 9-11: scsi board number
		ta=`expr $ta \* 8`
		drivetype=`expr $drivetype \* 64`
		sced=`expr $sced \* 512`
		diskunit=`expr $sced + $drivetype + $ta + $unitnum`
	else
		case "$TYPE" in
		wren*|microp1375|m2246sa|m2249sa|hp97548|hp97544) # embedded scsi
			diskunit=0
			;;
		*)	
			diskunit=48
			;;
		esac
	fi
	;;
esac

NTYPE=$TYPE
/bin/echo -n "Do you want to write a new VTOC?"
read v
if [ "$v" = "y" -o "$v" = "yes" ]
then
	if mkvtoc /dev/r$DISK $TYPE; then
		:
	else
		echo "mkvtoc failed.  Be sure /dev/r$DISK is valid."
		exit 1
	fi
	NTYPE=
fi
if [ "$TFILE" = 16 ]
then
/bin/echo '
	Press the AUTO button so an automatic reboot can happen.
'
fi

#
# If we don't have a VTOC, do a pre-3.1 newfs so that it (hopefully) isn't
# too small for root.
#
if /etc/prtvtoc /dev/r${DISK}a > /dev/null 2>&1; then
	/etc/newfs /dev/r${DISK}a $NTYPE
else
	/etc/newfs -C /dev/r${DISK}a $NTYPE
fi
if /etc/showcfg | grep -i -s ssm; then
	:
else
	/etc/installboot r${DISK}
fi
case "$?" in
0)	;;	*)	exit;;
esac
/bin/echo 'Check the file system'
/etc/fsck /dev/r${DISK}a
/bin/mkdir /a
/etc/mount /dev/${DISK}a /a
/bin/chgrp daemon /a
cd /a
/bin/echo 'Restore the dump image of the root file system'
wait
/etc/restore rsf $TFILE $TAPE		# 's 16' = skip to dump of root on tape
case "$?" in
0)	;;	*)	exit;;
esac
/bin/rm -f restoresymtable
# make a log file for subsequent installations of software
echo "`/bin/date`  - DYNIX V3.2.0 PN: 1003-xxxxx" > /a/etc/versionlog
chmod 644 /a/etc/versionlog
/bin/echo ' '
/bin/echo 'Making standard devices std pty0 ...'
cd /a/dev
(
./MAKEDEV std pty0
./MAKEDEV alm
./MAKEDEV usclk
) 2>&1 | grep -v "File exists"
# Look at the hardware configuration information.
# Edit out all but the last boot configuration.
/a/etc/dmesg > /tmp/dmesg
/bin/ed /tmp/dmesg << EOE > /dev/null
$
?avail mem?
.+1,$ d
?Mem?
1,.-1 d
w
q
EOE
# make devices that match the hardware configuration
/bin/echo -n 'Making configuration specific devices...'

for i in ts0 tm0
do
	/bin/grep -s "$i found" /tmp/dmesg
	case "$?" in 
	0)	/bin/echo -n " $i"
		./MAKEDEV "$i"& ;;
	esac
done
/bin/echo -n " ctape"
./MAKEDEV ctape >/dev/null 2>&1

if [ "$TFILE" = "16" ]
then
	for i in sd0 sd1 sd2 sd3 wd0 wd1 wd2 wd3 
	do
		/bin/grep -s "$i found" /tmp/dmesg
		case "$?" in 
		0)	/bin/echo -n " $i"
			./MAKEDEV "$i"& ;;
		esac
	done

	for i in xp0 xp1 xp2 xp3 xp4 xp5 xp6 xp7
	do
		/bin/grep -s "$i at" /tmp/dmesg
		case "$?" in 
		0)	/bin/echo -n " $i"
			./MAKEDEV "$i"& ;;
		esac
	done

	# gendynix anchors drives, dynix does not, so just start with 0 and work up.
	#
	j=0
	for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 \
		16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 \
		32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 \
		48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63
	do
		if /bin/grep -s "zd$i bound" /tmp/dmesg; then
			/bin/echo -n " zd$j"
			./MAKEDEV zd$j &
			j=`/bin/expr $j + 1`
		fi
	done

	# create devices we always want to exist
	./MAKEDEV xt0 lp0 &

	for i in xt1 xt2 xt3
	do
		/bin/grep -s "$i at" /tmp/dmesg
		case "$?" in 
		0)	/bin/echo -n " $i"
			./MAKEDEV "$i"& ;;
		esac
	done

	for i in lp1 lp2 lp3
	do
		/bin/grep -s "$i found" /tmp/dmesg
		case "$?" in
		0)	/bin/echo -n " $i"
			./MAKEDEV "$i"& ;;
		esac
	done

	for i in st0 st1 st2 st3 st4 st5 
	do
		/bin/grep -s "$i found" /tmp/dmesg
		case "$?" in 
		0)	/bin/echo -n " $i"
			./MAKEDEV "$i"&
			name=st
			unit=`expr $i : "$name\(.*\)"`
			case $unit in
			0) ch=h ;; 1) ch=i ;; 2) ch=j ;; 3) ch=k ;;
			4) ch=l ;; 5) ch=m ;; 6) ch=n ;; 7) ch=o ;;
			esac
			case $ch in
			h|i|j|k|l|m|n|o)
				eval `echo $ch $unit $major |
				  /bin/awk ' { ch = $1 } END {
				    for (i = 0; i < 16; i++) {
					printf("unknown tty%s%x\n",ch,i)\
						>> "/a/etc/ttytype"
					printf("12tty%s%x\n",ch,i) >> "/a/etc/ttys" }}'`
				;;
			esac
		esac
	done
fi
/bin/echo ""
/bin/rm -f /tmp/dmesg
cd /
/bin/sync
cd /a
if [ -f dynix ]
then
	mv dynix dynix.old
fi
case "$DISK" in
zd*) /bin/rm -f dynix_sd dynix_xp dynix_wd
	if [ "$DISK" != "zd0" ] ;then
		/a/etc/bootflags -p -c "n0=zd($diskunit,0)gendynix $DISK" "f=0"
		/bin/ln gendynix dynix
		/bin/echo 'Remember to rebuild your kernel and reboot.'
	else
		/a/etc/bootflags -p -c "n0=zd($diskunit,0)dynix" "f=0"
		/bin/ln dynix_zd dynix
	fi
	# edit dump string in /a/etc/rc.dumpstrings
	/bin/ed /a/etc/rc.dumpstrings > /dev/null 2>&1 << EOE
	g/^DUMPER/s/^/#/
	/DUMPER="zd(/
	s/^#//
	s/zd(0,/zd($diskunit,/g
	s/zd0/$DISK/
	g/^KERNEL/s/^/#/
	/KERNEL="zd(/
	s/^#//
	s/zd(0,/zd($diskunit,/g
	w
	q
EOE
;;
xp*)	/bin/rm -f dynix_sd dynix_zd dynix_wd
	if [ "$DISK" != "xp0" ] ;then
		/a/etc/bootflags -p -c "n0=xp($diskunit,0)gendynix $DISK" "f=0"
		/bin/ln gendynix dynix
		/bin/echo 'Remember to rebuild your kernel and reboot.'
	else
		/a/etc/bootflags -p -c "n0=xp($diskunit,0)dynix" "f=0"
		/bin/ln dynix_xp dynix
	fi
	# edit dump string in /a/etc/rc.dumpstrings
	/bin/ed /a/etc/rc.dumpstrings > /dev/null 2>&1 << EOE
	g/^DUMPER/s/^/#/
	/DUMPER="xp(/
	s/^#//
	s/xp(0,/xp($diskunit,/g
	s/xp0/$DISK/
	g/^KERNEL/s/^/#/
	/KERNEL="xp(/
	s/^#//
	s/xp(0,/xp($diskunit,/g
	w
	q
EOE
;;
sd*)
	/bin/rm -f dynix_xp dynix_zd dynix_wd
	if [ "$DISK" != "sd0" ] ;then
		/a/etc/bootflags -p -c "n0=sd($diskunit,0)gendynix $DISK" "f=0"
		/bin/ln gendynix dynix
		/bin/echo 'Remember to rebuild your kernel and reboot.'
	else
		/a/etc/bootflags -p -c "n0=sd($diskunit,0)dynix" "f=0"
		/bin/ln dynix_sd dynix
	fi
	# edit dump string in /a/etc/rc.dumpstrings
	/bin/ed /a/etc/rc.dumpstrings > /dev/null 2>&1 << EOE
	g/^DUMPER/s/^/#/
	/DUMPER="sd(/
	s/^#//
	s/sd(0,/sd($diskunit,/g
	s/sd0/$DISK/
	g/^KERNEL/s/^/#/
	/KERNEL="sd(/
	s/^#//
	s/sd(0,/sd($diskunit,/g
	w
	q
EOE
	;;
wd*)
	/bin/rm -f dynix_xp dynix_zd dynix_sd
	if [ "$DISK" != "wd0" ] ;then
		/a/etc/bootflags -p -c "n0=wd($diskunit,0)gendynix $DISK" "f=0"
		/bin/ln gendynix dynix
		/bin/echo 'Remember to rebuild your kernel and reboot.'
	else
		/a/etc/bootflags -p -c "n0=wd($diskunit,0)dynix" "f=0"
		/bin/ln dynix_wd dynix
	fi
	# edit dump string in /a/etc/rc.dumpstrings
	/bin/ed /a/etc/rc.dumpstrings > /dev/null 2>&1 << EOE
	g/^DUMPER/s/^/#/
	/DUMPER="wd(/
	s/^#//
	s/wd(0,/wd($diskunit,/g
	s/wd0/$DISK/
	g/^KERNEL/s/^/#/
	/KERNEL="wd(/
	s/^#//
	s/wd(0,/wd($diskunit,/g
	w
	q
EOE
	;;
esac
cd /a/etc
if [ -f fstab ]
then
	mv fstab fstab.old
fi
/bin/echo "/dev/${DISK}a /    4.2 rw          1 1" > /a/etc/fstab
cd /
wait
/bin/sync
/etc/umount /a
/bin/sync
/etc/fsck /dev/r${DISK}a
/etc/mount /dev/${DISK}a /a
/bin/echo 'Root file system extracted'
if [ "$TFILE" = "16" ]
then
	/bin/echo 'Now booting off real root file system...'
	/a/etc/shutdown -r now > /dev/null 2>&1
fi
_P_
sed -e '/TFILE=16/s//TFILE=1/' tmpos/etc/restore.root >tmpos/etc/restore.oldroot
$chown     root tmpos/etc/restore.root &
$chgrp   daemon tmpos/etc/restore.root &
$chmod      744 tmpos/etc/restore.root &
$chown     root tmpos/etc/restore.oldroot &
$chgrp   daemon tmpos/etc/restore.oldroot &
$chmod      744 tmpos/etc/restore.oldroot &
wait
/bin/cat > tmpos/etc/restore.more << '_P_'
#! /bin/sh
#
# restore.more
#
# This shell script is used during initial system load from
# tape. It prompts the user for affirmative replies prior to
# loading files from tape.

/etc/online -a
/bin/echo ' '
TAPE=/dev/ctape8
TAPE2ID=/tmp/sqnt_tapeid2
TFILE=16
if [ "$TFILE" != 16 ]
then
	/bin/echo -n "tape drive with /usr dump on (eg $TAPE) "
	read TAPE
fi

rm -f /tmp/done$$
/bin/mt -f $TAPE noret 
(/bin/mt -f $TAPE rewind; echo $? > /tmp/done$$)&
while [ ! -f /tmp/done$$ ]
do
	:
done
stat=`cat /tmp/done$$`
rm /tmp/done$$
case $stat in
0)	;;
*)	echo "Problems in rewinding tape"
	exit 1 ;;
esac

# don't need to skip past dump of root on tape since /usr is on the 2nd
# base tape now.  Make sure the correct tape is in the drive before
# continuing.
#
rm -f /tmp/done$$
tar tf $TAPE >/tmp/done$$ 2>&1; stat=`echo $?`
idfile=`cat /tmp/done$$`
rm /tmp/done$$
if [ "$stat" != "0" -o "$idfile" != "$TAPE2ID" ]
then
	echo "WARNING: Tape in drive is not second tape of base s/w."
	exit 1
fi

echo "`/bin/date`  - DYNIX V3.2.0 PN: 1003-xxxxx" >> /etc/versionlog

/bin/echo -n 'restore /usr ?(n/y) '
read USR
case "$USR" in
y)	until
		/bin/echo -n 'disk name for /usr? (e.g. sd0, wd0, xp0 or zd0) '
		read DISK
		[ -f /dev/${DISK}g ] 
		case "$?" in
		0)	;;	*)	prtvtoc /dev/r${DISK} >/dev/null;
		esac
	do
		/bin/echo 'unknown disk name
	'
	done

	NTYPE=
	TYPE=`format -i ${DISK} 2>/dev/null`
	if [ $? != 0 -o "$TYPE" = "" ]
	then
		TYPES=`(cd /etc/diskinfo;echo *.geom | sed -e 's/\.geom/, /g')`
		until	/bin/echo -n "disk type? (e.g. $TYPES) "
			read TYPE
			/bin/grep -w -s "$TYPE" /etc/disktab
			case "$?" in 
			0)	;;	*)	false;;
			esac
		do
			/bin/echo 'unknown disk type
		'
		done
		NTYPE=$TYPE
	fi
	/bin/echo -n 'which partition for /usr? '
	read PART
	;;
esac

if [ "$TFILE" != 16 ]
then
	NEWROOT=/a
	/etc/newfs /dev/$DISK$PART $NTYPE
	case "$?" in 
	0)	;;	*)	exit;;
	esac
	rm /tmp/done$$
	/bin/sync
	/etc/fsck /dev/r$DISK$PART
	cd $NEWROOT
	/bin/chmod 755 usr	# since this was a mounted filesys before
	/bin/chgrp 10  usr	# ditto
	/etc/mount /dev/$DISK$PART /a/usr
	/bin/chmod 755 usr
	/bin/chgrp 10  usr
	cd usr
	echo "restoring /usr (`/bin/pwd`)"
	/etc/restore rf $TAPE
	case $stat in
	0)	;;
	*)	echo "Problems in restoring tape files, cleaning up."
		/etc/umount /a/usr
		exit 1 ;;
	esac
	/bin/echo "/dev/$DISK$PART /usr 4.2 rw          1 2" >> /a/etc/fstab
	exit
fi
/bin/echo 'NOTE: The following tape file does not need to be restored from'
/bin/echo 'tape unless changes to man pages are planned or the user wants to'
/bin/echo 'generate hardcopies of man pages. Formatted compressed man pages'
/bin/echo 'are installed with /usr file system in the previous tape file.'
/bin/echo
/bin/echo -n 'restore unformatted man pages to /usr/man ?(n/y) '
read MAN
/bin/echo -n 'restore /usr/doc ?(n/y) '
read DOC
/bin/echo -n 'restore /usr/games ?(n/y) '
read GAMES
/bin/echo -n 'restore /usr/lib/vfont ?(n/y) '
read VFONT
/bin/echo -n 'restore /usr/local/gnu (EMACS)?(n/y) '
read GNU

case "$USR" in
y)	/etc/newfs /dev/$DISK$PART $NTYPE
	case "$?" in 
	0)	;;	*)	exit;;
	esac
	/bin/sync
	/etc/fsck /dev/r$DISK$PART
	/bin/chmod 755 /usr	# since this was a mounted filesys before
	/bin/chgrp 10  /usr	# ditto
	/etc/mount /dev/$DISK$PART /usr
	/bin/chmod 755 /usr
	/bin/chgrp 10  /usr
	cd /usr
	echo "restoring /usr"
	(/bin/tar xpf $TAPE; echo $? > /tmp/done$$)&
	cnt=0
	while [ ! -f /tmp/done$$ ]
	do
		echo -n .
		cnt=`expr $cnt + 1`
		if [ $cnt -gt 79 ] ;then 
			echo ""
			cnt=0
		fi
	done
	stat=`cat /tmp/done$$`
	rm /tmp/done$$
	echo ""
	case $stat in
	0)	;;
	*)	echo "Problems in un-taring tape file, cleaning up."
		/etc/umount /usr
		exit 1 ;;
	esac
					# creat empty man directories
	for i in man0 man1 man2 man3 man4 man5 man6 man7 man8 manl mann mano
	do
		mkdir /usr/man/$i		2>/dev/null
		chmod 755 /usr/man/$i		2>/dev/null
		chgrp daemon /usr/man/$i	2>/dev/null
	done
					# create /etc/fstab entry
	/bin/echo "/dev/$DISK$PART /usr 4.2 rw          1 2" >> /etc/fstab
	/bin/echo "/usr filesystem restored" ;;

*)	# skip past /usr tar image on tape
	(/bin/mt -f $TAPE fsf 1; echo $? > /tmp/done$$)&
	while [ ! -f /tmp/done$$ ]
	do
		:
	done
	stat=`cat /tmp/done$$`
	rm /tmp/done$$
	case $stat in
	0)	;;
	*)	echo "Problems skipping forward tape files"
		exit 1 ;;
	esac ;;
esac

/bin/grep '/usr' /etc/fstab > /dev/null
USRTHERE="$?"

if [ "${MAN}${DOC}${GAMES}${VFONT}${GNU}" = "nnnnn" ]
then
	cd /
	/bin/sync
	/bin/mt -f $TAPE rewind
	date
	/bin/echo "All done."
	exit 0
fi

case "$MAN" in
y)	case $USRTHERE in
	0)	cd /usr
		/bin/echo "restoring unformatted man pages"
		(/bin/tar xpf $TAPE; echo $? > /tmp/done$$)&
		cnt=0
		while [ ! -f /tmp/done$$ ]
		do
			echo -n .
			cnt=`expr $cnt + 1`
			if [ $cnt -gt 79 ] ;then 
				echo ""
				cnt=0
			fi
		done
		stat=`cat /tmp/done$$`
		rm /tmp/done$$
		echo ""
		case $stat in
		0)	;;
		*)	echo "Problems in un-taring tape file"
			exit 1 ;;
		esac
		/bin/echo unformatted man pages restored ;;
	
	*)	/bin/echo "You must restore /usr first." ;;
	esac ;;

*)	# skip past unformatted man pages 
	(/bin/mt -f $TAPE fsf 1; echo $? > /tmp/done$$)&
	while [ ! -f /tmp/done$$ ]
	do
		:
	done
	stat=`cat /tmp/done$$`
	rm /tmp/done$$
	case $stat in
	0)	;;
	*)	echo "Problems skipping forward tape files"
		exit 1 ;;
	esac ;;
esac

if [ "${DOC}${GAMES}${VFONT}${GNU}" = "nnnn" ]
then
	cd /
	/bin/sync
	/bin/mt -f $TAPE rewind
	date
	/bin/echo "All done."
	exit 0
fi

case "$DOC" in
y)	case $USRTHERE in
	0)	cd /usr
		/bin/echo "restoring /usr/doc"
		(/bin/tar xpf $TAPE; echo $? > /tmp/done$$)&
		cnt=0
		while [ ! -f /tmp/done$$ ]
		do
			echo -n .
			cnt=`expr $cnt + 1`
			if [ $cnt -gt 79 ] ;then 
				echo ""
				cnt=0
			fi
		done
		stat=`cat /tmp/done$$`
		rm /tmp/done$$
		echo ""
		case $stat in
		0)	;;
		*)	echo "Problems in un-taring tape file"
			exit 1 ;;
		esac
		/bin/echo /usr/doc restored ;;
	
	*)	/bin/echo "You must restore /usr first." ;;
	esac ;;

*)	# skip past /usr/doc tar image on tape
	(/bin/mt -f $TAPE fsf 1; echo $? > /tmp/done$$)&
	while [ ! -f /tmp/done$$ ]
	do
		:
	done
	stat=`cat /tmp/done$$`
	rm /tmp/done$$
	case $stat in
	0)	;;
	*)	echo "Problems skipping forward tape files"
		exit 1 ;;
	esac ;;
esac

if [ "${GAMES}${VFONT}${GNU}" = "nnn" ]
then
	cd /
	/bin/sync
	/bin/mt -f $TAPE rewind
	date
	/bin/echo "All done."
	exit 0
fi

case "$GAMES" in
y)	case $USRTHERE in
	0)	cd /usr
		/bin/echo "restoring /usr/games"
		(/bin/tar xpf $TAPE; echo $? > /tmp/done$$)&
		cnt=0
		while [ ! -f /tmp/done$$ ]
		do
			echo -n .
			cnt=`expr $cnt + 1`
			if [ $cnt -gt 79 ] ;then 
				echo ""
				cnt=0
			fi
		done
		stat=`cat /tmp/done$$`
		rm /tmp/done$$
		echo ""
		case $stat in
		0)	;;
		*)	echo "Problems in un-taring tape file"
			exit 1 ;;
		esac
		/bin/echo "/usr/games restored" ;;
	
	*)	/bin/echo "You must restore /usr first." ;;
	esac ;;

*)	# skip past /usr/games tar image on tape
	(/bin/mt -f $TAPE fsf 1; echo $? > /tmp/done$$)&
	while [ ! -f /tmp/done$$ ]
	do
		:
	done
	stat=`cat /tmp/done$$`
	rm /tmp/done$$
	case $stat in
	0)	;;
	*)	echo "Problems skipping forward tape files"
		exit 1 ;;
	esac ;;
esac

if [ "${VFONT}${GNU}" = "nn" ]
then
	cd /
	/bin/sync
	/bin/mt -f $TAPE rewind
	date
	/bin/echo "All done."
	exit 0
fi

case "$VFONT" in
y)	case $USRTHERE in
	0)	cd /usr/lib
		/bin/echo "restoring /usr/lib/vfont" 
		(/bin/tar xpf $TAPE; echo $? > /tmp/done$$)&
		cnt=0
		while [ ! -f /tmp/done$$ ]
		do
			echo -n .
			cnt=`expr $cnt + 1`
			if [ $cnt -gt 79 ] ;then 
				echo ""
				cnt=0
			fi
		done
		stat=`cat /tmp/done$$`
		rm /tmp/done$$
		echo ""
		case $stat in
		0)	;;
		*)	echo "Problems in un-taring tape file"
			exit 1 ;;
		esac
		/bin/echo "/usr/lib/vfont restored" ;;
	
	*)	/bin/echo "You must restore /usr first." ;;
	esac ;;

*)	# skip past /usr/lib/vfont
	(/bin/mt -f $TAPE fsf 1; echo $? > /tmp/done$$)&
	while [ ! -f /tmp/done$$ ]
	do
		:
	done
	stat=`cat /tmp/done$$`
	rm /tmp/done$$
	case $stat in
	0)	;;
	*)	echo "Problems skipping forward tape files"
		exit 1 ;;
	esac ;;
esac

if [ "${GNU}" = "n" ]
then
	cd /
	/bin/sync
	/bin/mt -f $TAPE rewind
	date
	/bin/echo "All done."
	exit 0
fi

case "$GNU" in
y)	case $USRTHERE in
	0)	cd /usr/local
		/bin/echo "restoring  /usr/local/gnu"
		(/bin/tar xpf $TAPE; echo $? > /tmp/done$$)&
		cnt=0
		while [ ! -f /tmp/done$$ ]
		do
			echo -n .
			cnt=`expr $cnt + 1`
			if [ $cnt -gt 79 ] ;then 
				echo ""
				cnt=0
			fi
		done
		stat=`cat /tmp/done$$`
		rm /tmp/done$$
		echo ""
		case $stat in
		0)	;;
		*)	echo "Problems in un-taring tape file"
			exit 1 ;;
		esac
		/bin/echo "/usr/local/gnu restored" ;;

	*)	/bin/echo "You must restore /usr first. " ;;
	esac ;;

*)	# skip past /usr/local/gnu
	(/bin/mt -f $TAPE fsf 1; echo $? > /tmp/done$$)&
	while [ ! -f /tmp/done$$ ]
	do
		:
	done
	stat=`cat /tmp/done$$`
	rm /tmp/done$$
	case $stat in
	0)	;;
	*)	echo "Problems skipping forward tape files"
		exit 1 ;;
	esac ;;
esac

cd /
/etc/mount -f /         # so that df will show everything
/bin/sync
/bin/mt -f $TAPE rewind
date
/bin/echo "All done."
_P_
sed -e '/TFILE=16/s//TFILE=0/' tmpos/etc/restore.more >tmpos/etc/restore.oldmore
$chown     root tmpos/etc/restore.more &
$chgrp   daemon tmpos/etc/restore.more &
$chmod      744 tmpos/etc/restore.more &
$chown     root tmpos/etc/restore.oldmore &
$chgrp   daemon tmpos/etc/restore.oldmore &
$chmod      744 tmpos/etc/restore.oldmore &
wait
/bin/cat > tmpos/etc/restore.ssm << '_P_'
#!/bin/sh
#
# restore.ssm
#
# This shell script is used during initial system load from
# tape. It is for loading the SSM software into /usr/ssw from
# the SSM software tape.

PATH=/bin:/usr/etc:/etc:/usr/bin:/usr/ucb
TAPE=/dev/ctape8
D3_SSM=5		# file number of d3 ssm install script
FWDIR=/usr/ssw

if showcfg | grep -i -s ssm; then
	:
else
	echo "This is not an SSM based system."
	exit 1
fi

if [ ! -d $FWDIR ]; then
	echo "$FWDIR does not exist."
	exit 1
fi
FWDF=`df $FWDIR | grep -v "Filesystem"`

set `echo $FWDF | awk '{tom=substr($1,8);print $1, $6," ",substr(tom, 0, length(tom) -1) * 8," ", index("abcdefghijklmno", substr(tom, length(tom)))-1;}'`
fwdiskdev=$1
fwmntpt=$2
fwdisk=$3
fwpart=$4

if echo $fwdiskdev | grep -s "/dev/wd"; then
	:
else
	echo "$FWDIR is not mounted on a wd disk."
	exit 1
fi

fwdirectory=`cd $FWDIR; /bin/pwd`
fwpath=`expr $fwdirectory : "^$fwmntpt\(.*\)"`
fwpath=$fwpath/boot

#
# create fw, fw/ssm, and diag under $FWDIR if they aren't already there
#
if [ ! -d $FWDIR/fw ]; then
	if mkdir $FWDIR/fw; then
		:
	else
		echo "Failed to create $FWDIR/fw."
		exit 1
	fi
fi
if [ ! -d $FWDIR/fw/ssm ]; then
	if mkdir $FWDIR/fw/ssm; then
		:
	else
		echo "Failed to create $FWDIR/fw/ssm."
		exit 1
	fi
fi
if [ ! -d $FWDIR/diag ]; then
	if mkdir $FWDIR/diag; then
		:
	else
		echo "Failed to create $FWDIR/diag."
		exit 1
	fi
fi
echo "Extracting SSM installation script..."
mt -f $TAPE rew
mt -f $TAPE fsf $D3_SSM
dd if=$TAPE of=install.sh
mt -f $TAPE rew
echo "Running SSM installation..."
sh install.sh
rm -f install.sh
cp /boot $FWDIR
echo "/boot copied to $FWDIR"
echo "Firmware bh loaderPath should be \"wd($fwdisk,$fwpart)$fwpath\""
echo "All done."
exit 0
_P_
$chown     root tmpos/etc/restore.ssm &
$chgrp   daemon tmpos/etc/restore.ssm &
$chmod      744 tmpos/etc/restore.ssm &
wait
/bin/cat > tmpos_crypt/etc/instl.encrypt << '_P_'
#! /bin/sh
#
# /etc/instl.encrypt (aka instl on the distribution tape)
#
# Shell script to install the encryption software distribution.
# It updates /etc/versionlog to log installation of software. It also
# checks to make sure that System V has been installed on DYNIX prior
# to installing encryption software for System V. Must be run as root.
#
# This file should be the first tape file on distribution tape.
# The second tape file should be a tar image of DYNIX encryption software.
# The third tape file should be a tar image of System V crypt encryption.
#
TAPE=/dev/ctape8
PATH=:/bin:/usr/bin:/usr/ucb:/etc; export PATH

##
# Default ROOT for entire System V file structure.
# This can be changed although I do not recommend it.
##
R="/usr/att"

if /etc/mount | /bin/awk '$3=="/usr" {exit 1}'; then
	echo "Usr filesystem appears not to be mounted."
	echo "Please mount /usr and try again."
	exit 1
fi

if [ `whoami` != root ]; then
	echo "Sorry this script must be run as super-user to work properly."
	exit 1
fi

if [ "`universe`" != "ucb" ] ; then
	exec ucb $0 $@
fi

cd /
echo "rewinding tape..."
mt -f $TAPE rew
echo "extracting DYNIX encryption software..."
mt -f $TAPE fsf 1		# move to second tape file
tar xpf $TAPE			# extract DYNIX encryption software
echo "DYNIX encryption software installed"

if [ ! -d "$R" ]; then
	echo "System V software appears not to be installed."
	echo "System V encryption software will not be installed."
else
	cd $R
	echo "extracting System V encryption software..."
	tar xpf $TAPE		# extract System V encryption software
	echo "DYNIX System V encryption software installed"
fi

mt -f $TAPE rew
echo "`/bin/date`  - DYNIX V3.2.0 Encryption SW PN: 1003-xxxxx" >> /etc/versionlog
echo "All done."
_P_
$chown     root tmpos_crypt/etc/instl.encrypt &
$chgrp   daemon tmpos_crypt/etc/instl.encrypt &
$chmod      744 tmpos_crypt/etc/instl.encrypt &
wait
/bin/cat > tmpos/usr/adm/acct << '_P_'
_P_
$chown     root tmpos/usr/adm/acct &
$chgrp   daemon tmpos/usr/adm/acct &
$chmod      644 tmpos/usr/adm/acct &
wait
/bin/cat > tmpos/usr/adm/aculog << '_P_'
_P_
$chown     uucp tmpos/usr/adm/aculog &
$chgrp   daemon tmpos/usr/adm/aculog &
$chmod      600 tmpos/usr/adm/aculog &
wait
/bin/cat > tmpos/usr/adm/lastlog << '_P_'
_P_
$chown     root tmpos/usr/adm/lastlog &
$chgrp   daemon tmpos/usr/adm/lastlog &
$chmod      644 tmpos/usr/adm/lastlog &
wait
/bin/cat > tmpos/usr/adm/lpd-errs << '_P_'
_P_
$chown     root tmpos/usr/adm/lpd-errs &
$chgrp   daemon tmpos/usr/adm/lpd-errs &
$chmod      644 tmpos/usr/adm/lpd-errs &
wait
/bin/cat > tmpos/usr/adm/messages << '_P_'
_P_
$chown     root tmpos/usr/adm/messages &
$chgrp   daemon tmpos/usr/adm/messages &
$chmod      644 tmpos/usr/adm/messages &
wait
/bin/cat > tmpos/usr/adm/msgbuf << '_P_'
_P_
$chown     root tmpos/usr/adm/msgbuf &
$chgrp   daemon tmpos/usr/adm/msgbuf &
$chmod      644 tmpos/usr/adm/msgbuf &
wait
/bin/cat > tmpos/usr/adm/wtmp << '_P_'
_P_
$chown     root tmpos/usr/adm/wtmp &
$chgrp   daemon tmpos/usr/adm/wtmp &
$chmod      644 tmpos/usr/adm/wtmp &
wait
/bin/cat > tmpos/usr/adm/sus << '_P_'
_P_
$chown     root tmpos/usr/adm/sus &
$chgrp   daemon tmpos/usr/adm/sus &
$chmod      600 tmpos/usr/adm/sus &
wait
/bin/cat > tmpos/usr/adm/badlogins << '_P_'
_P_
$chown     root tmpos/usr/adm/badlogins &
$chgrp   daemon tmpos/usr/adm/badlogins &
$chmod      600 tmpos/usr/adm/badlogins &
wait
/bin/cat > tmpos/usr/adm/newsyslog << '_P_'
cd /usr/spool/mqueue
rm -f syslog.7
if [ -f syslog.6 ]; then mv -f syslog.6  syslog.7; fi
if [ -f syslog.5 ]; then mv -f syslog.5  syslog.6; fi
if [ -f syslog.4 ]; then mv -f syslog.4  syslog.5; fi
if [ -f syslog.3 ]; then mv -f syslog.3  syslog.4; fi
if [ -f syslog.2 ]; then mv -f syslog.2  syslog.3; fi
if [ -f syslog.1 ]; then mv -f syslog.1  syslog.2; fi
if [ -f syslog.0 ]; then mv -f syslog.0  syslog.1; fi
mv -f /usr/spool/adm/syslog    syslog.0
> /usr/spool/adm/syslog
chmod 644    /usr/spool/adm/syslog
rm -f critical.7
if [ -f critical.6 ]; then mv -f critical.6  critical.7; fi
if [ -f critical.5 ]; then mv -f critical.5  critical.6; fi
if [ -f critical.4 ]; then mv -f critical.4  critical.5; fi
if [ -f critical.3 ]; then mv -f critical.3  critical.4; fi
if [ -f critical.2 ]; then mv -f critical.2  critical.3; fi
if [ -f critical.1 ]; then mv -f critical.1  critical.2; fi
if [ -f critical.0 ]; then mv -f critical.0  critical.1; fi
mv -f /usr/adm/critical    critical.0
> /usr/adm/critical
chmod 644    /usr/adm/critical
kill -1 `cat /etc/syslog.pid`
_P_
$chown     root tmpos/usr/adm/newsyslog &
$chgrp     root tmpos/usr/adm/newsyslog &
$chmod      755 tmpos/usr/adm/newsyslog &
wait
/bin/cat > tmpos/usr/hosts/MAKEHOSTS << '_P_'
#!/bin/sh
cd ${DESTDIR}/usr/hosts
rm -f [a-z]*
for i in `cat ${DESTDIR}/etc/hosts.equiv`
do
	ln -s /usr/ucb/rsh $i
done
_P_
$chown     root tmpos/usr/hosts/MAKEHOSTS &
$chgrp   daemon tmpos/usr/hosts/MAKEHOSTS &
$chmod      755 tmpos/usr/hosts/MAKEHOSTS &
wait
/bin/cat > tmpos/usr/lib/crontab << '_P_'
30 4 * * * /usr/etc/sa -s > /dev/null
0 4 * * * calendar -
15 4 * * * find /usr/preserve -mtime +7 -a -exec rm -f {} \;
20 4 * * * find /usr/msgs -mtime +21 -a ! -perm 444 -a ! -name bounds -a -exec rm -f {} \;
40 4 * * * find / '(' -fstype nfs -prune ')' -o '(' -name '#*' -o -name 'core' ')' -a -atime +3 -a -exec rm -f {} ';'
0,15,30,45 * * * * /usr/lib/atrun
0,10,20,30,40,50 * * * * /etc/dmesg - >>/usr/adm/messages
5 4 * * * sh /usr/adm/newsyslog
_P_
$chown     root tmpos/usr/lib/crontab &
$chgrp   daemon tmpos/usr/lib/crontab &
$chmod      644 tmpos/usr/lib/crontab &
wait
/bin/cat > tmpos/usr/lib/aliases << '_P_'
##
#  Aliases in this file will NOT be expanded in the header from
#  Mail, but WILL be visible over networks or from /bin/mail.
#
#	>>>>>>>>>>	The program "newaliases" must be run after
#	>> NOTE >>	this file is updated for any changes to
#	>>>>>>>>>>	show through to sendmail.
##

# Alias for mailer daemon
MAILER-DAEMON:root

# Following alias is required by the new mail protocol, RFC 822
postmaster:root

# Aliases to handle mail to msgs and news
msgs: "|/usr/ucb/msgs -s"
nobody: "|cat>/dev/null"

# Alias for uucp maintenance
uucplist:root
_P_
$chown     root tmpos/usr/lib/aliases &
$chgrp   daemon tmpos/usr/lib/aliases &
$chmod      644 tmpos/usr/lib/aliases &
wait
/bin/cat > tmpos/usr/etc/nslookup.help << '_P_'
Commands: 	(identifiers are shown in uppercase, [] means optional)
NAME		- print info about the host/domain NAME using default server
NAME1 NAME2	- as above, but use NAME2 as server
help or ?	- print info on common commands; see nslookup(1) for details
set OPTION	- set an option
    all		- print options, current server and host
    [no]debug	- print debugging information
    [no]d2	- print exhaustive debugging information
    [no]defname	- append domain name to each query 
    [no]recurse	- ask for recursive answer to query
    [no]vc	- always use a virtual circuit
    domain=NAME	- set default domain name to NAME
    srchlist=N1[/N2/.../N6] - set domain to N1 and search list to N1,N2, etc.
    root=NAME	- set root server to NAME
    retry=X	- set number of retries to X
    timeout=X	- set initial time-out interval to X seconds
    querytype=X	- set query type, e.g., A,ANY,CNAME,HINFO,MX,NS,PTR,SOA,WKS
    type=X	- synonym for querytype
    class=X	- set query class to one of IN (Internet), CHAOS, HESIOD or ANY
server NAME	- set default server to NAME, using current default server
lserver NAME	- set default server to NAME, using initial server
finger [USER]	- finger the optional NAME at the current default host
root		- set current default server to the root
ls [opt] DOMAIN [> FILE] - list addresses in DOMAIN (optional: output to FILE)
    -a 		-  list canonical names and aliases
    -h 		-  list HINFO (CPU type and operating system)
    -s 		-  list well-known services
    -d 		-  list all records
    -t TYPE 	-  list records of the given type (e.g., A,CNAME,MX, etc.)
view FILE	- sort an 'ls' output file and view it with more
exit		- exit the program, ^D also exits
_P_
$chown     root tmpos/usr/etc/nslookup.help &
$chgrp     root tmpos/usr/etc/nslookup.help &
$chmod      444 tmpos/usr/etc/nslookup.help &
wait
/bin/cat > tmpos/usr/lib/sendmail.hf << '_P_'
@(#)	sendmail.hf	4.1	7/25/83
smtp	Commands:
smtp		HELO	MAIL	RCPT	DATA	RSET
smtp		NOOP	QUIT	HELP	VRFY	EXPN
smtp	For more info use "HELP <topic>".
smtp	To report bugs in the implementation contact eric@Berkeley.ARPA
smtp	or eric@UCB-ARPA.ARPA.
smtp	For local information contact postmaster at this site.
help	HELP [ <topic> ]
help		The HELP command gives help info.
helo	HELO <hostname>
helo		Introduce yourself.  I am a boor, so I really don't
helo		care if you do.
mail	MAIL FROM: <sender>
mail		Specifies the sender.
rcpt	RCPT TO: <recipient>
rcpt		Specifies the recipient.  Can be used any number of times.
data	DATA
data		Following text is collected as the message.
data		End with a single dot.
rset	RSET
rset		Resets the system.
quit	QUIT
quit		Exit sendmail (SMTP).
vrfy	VRFY <recipient>
vrfy		Not implemented to protocol.  Gives some sexy
vrfy		information.
expn	EXPN <recipient>
expn		Same as VRFY in this implementation.
noop	NOOP
noop		Do nothing.
send	SEND FROM: <sender>
send		replaces the MAIL command, and can be used to send
send		directly to a users terminal.  Not supported in this
send		implementation.
soml	SOML FROM: <sender>
soml		Send or mail.  If the user is logged in, send directly,
soml		otherwise mail.  Not supported in this implementation.
saml	SAML FROM: <sender>
saml		Send and mail.  Send directly to the user's terminal,
saml		and also mail a letter.  Not supported in this
saml		implementation.
turn	TURN
turn		Reverses the direction of the connection.  Not currently
turn		implemented.
_P_
$chown     root tmpos/usr/lib/sendmail.hf &
$chgrp     root tmpos/usr/lib/sendmail.hf &
$chmod      755 tmpos/usr/lib/sendmail.hf &
wait
/bin/cat > tmpos/usr/lib/eign << '_P_'
the
of
and
to
a
in
that
is
was
he
for
it
with
as
his
on
be
at
by
i
this
had
not
are
but
from
or
have
an
they
which
one
you
were
her
all
she
there
would
their
we
him
been
has
when
who
will
more
no
if
out
so
said
what
up
its
about
into
than
them
can
only
other
new
some
could
time
these
two
may
then
do
first
any
my
now
such
like
our
over
man
me
even
most
made
after
also
did
many
before
must
through
back
years
where
much
your
way
well
down
should
because
each
just
those
people
mr
how
too
little
state
good
very
make
world
still
own
see
men
work
long
get
here
between
both
life
being
under
never
day
same
another
know
while
last
might
us
great
old
year
off
come
since
against
go
came
right
used
take
three
_P_
$chown     root tmpos/usr/lib/eign &
$chgrp   daemon tmpos/usr/lib/eign &
$chmod      644 tmpos/usr/lib/eign &
wait
/bin/cat > tmpos/usr/pub/ascii << '_P_'
|000 nul|001 soh|002 stx|003 etx|004 eot|005 enq|006 ack|007 bel|
|010 bs |011 ht |012 nl |013 vt |014 np |015 cr |016 so |017 si |
|020 dle|021 dc1|022 dc2|023 dc3|024 dc4|025 nak|026 syn|027 etb|
|030 can|031 em |032 sub|033 esc|034 fs |035 gs |036 rs |037 us |
|040 sp |041  ! |042  " |043  # |044  $ |045  % |046  & |047  ' |
|050  ( |051  ) |052  * |053  + |054  , |055  - |056  . |057  / |
|060  0 |061  1 |062  2 |063  3 |064  4 |065  5 |066  6 |067  7 |
|070  8 |071  9 |072  : |073  ; |074  < |075  = |076  > |077  ? |
|100  @ |101  A |102  B |103  C |104  D |105  E |106  F |107  G |
|110  H |111  I |112  J |113  K |114  L |115  M |116  N |117  O |
|120  P |121  Q |122  R |123  S |124  T |125  U |126  V |127  W |
|130  X |131  Y |132  Z |133  [ |134  \ |135  ] |136  ^ |137  _ |
|140  ` |141  a |142  b |143  c |144  d |145  e |146  f |147  g |
|150  h |151  i |152  j |153  k |154  l |155  m |156  n |157  o |
|160  p |161  q |162  r |163  s |164  t |165  u |166  v |167  w |
|170  x |171  y |172  z |173  { |174  | |175  } |176  ~ |177 del|


| 00 nul| 01 soh| 02 stx| 03 etx| 04 eot| 05 enq| 06 ack| 07 bel|
| 08 bs | 09 ht | 0a nl | 0b vt | 0c np | 0d cr | 0e so | 0f si |
| 10 dle| 11 dc1| 12 dc2| 13 dc3| 14 dc4| 15 nak| 16 syn| 17 etb|
| 18 can| 19 em | 1a sub| 1b esc| 1c fs | 1d gs | 1e rs | 1f us |
| 20 sp | 21  ! | 22  " | 23  # | 24  $ | 25  % | 26  & | 27  ' |
| 28  ( | 29  ) | 2a  * | 2b  + | 2c  , | 2d  - | 2e  . | 2f  / |
| 30  0 | 31  1 | 32  2 | 33  3 | 34  4 | 35  5 | 36  6 | 37  7 |
| 38  8 | 39  9 | 3a  : | 3b  ; | 3c  < | 3d  = | 3e  > | 3f  ? |
| 40  @ | 41  A | 42  B | 43  C | 44  D | 45  E | 46  F | 47  G |
| 48  H | 49  I | 4a  J | 4b  K | 4c  L | 4d  M | 4e  N | 4f  O |
| 50  P | 51  Q | 52  R | 53  S | 54  T | 55  U | 56  V | 57  W |
| 58  X | 59  Y | 5a  Z | 5b  [ | 5c  \ | 5d  ] | 5e  ^ | 5f  _ |
| 60  ` | 61  a | 62  b | 63  c | 64  d | 65  e | 66  f | 67  g |
| 68  h | 69  i | 6a  j | 6b  k | 6c  l | 6d  m | 6e  n | 6f  o |
| 70  p | 71  q | 72  r | 73  s | 74  t | 75  u | 76  v | 77  w |
| 78  x | 79  y | 7a  z | 7b  { | 7c  | | 7d  } | 7e  ~ | 7f del|
_P_
$chown     root tmpos/usr/pub/ascii &
$chgrp   daemon tmpos/usr/pub/ascii &
$chmod      444 tmpos/usr/pub/ascii &
wait
/bin/cat > tmpos/usr/pub/eqnchar << '_P_'
.EQ
tdefine ciplus % "\o'\(pl\(ci'" %
ndefine ciplus % O+ %
tdefine citimes % "\o'\(mu\(ci'" %
ndefine citimes % Ox %
tdefine =wig % "\(eq\h'-\w'\(eq'u-\w'\s-2\(ap'u/2u'\v'-.4m'\s-2\z\(ap\(ap\s+2\v'.4m'\h'\w'\(eq'u-\w'\s-2\(ap'u/2u'" %
ndefine =wig % ="~" %
tdefine bigstar % "\o'\(pl\(mu'" %
ndefine bigstar % X|- %
tdefine =dot % "\z\(eq\v'-.6m'\h'.2m'\s+2.\s-2\v'.6m'\h'.1m'" %
ndefine =dot % = dot %
tdefine orsign % "\s-2\v'-.15m'\z\e\e\h'-.05m'\z\(sl\(sl\v'.15m'\s+2" %
ndefine orsign % \e/ %
tdefine andsign % "\s-2\v'-.15m'\z\(sl\(sl\h'-.05m'\z\e\e\v'.15m'\s+2" %
ndefine andsign % /\e %
tdefine =del % "\v'.3m'\z=\v'-.6m'\h'.3m'\s-1\(*D\s+1\v'.3m'" %
ndefine =del % = to DELTA %
tdefine oppA % "\s-2\v'-.15m'\z\e\e\h'-.05m'\z\(sl\(sl\v'-.15m'\h'-.75m'\z-\z-\h'.2m'\z-\z-\v'.3m'\h'.4m'\s+2" %
ndefine oppA % V- %
tdefine oppE %"\s-3\v'.2m'\z\(em\v'-.5m'\z\(em\v'-.5m'\z\(em\v'.55m'\h'.9m'\z\(br\z\(br\v'.25m'\s+3" %
ndefine oppE % E/ %
tdefine incl % "\s-1\z\(or\h'-.1m'\v'-.45m'\z\(em\v'.7m'\z\(em\v'.2m'\(em\v'-.45m'\s+1" %
ndefine incl % C_ %
tdefine nomem % "\o'\(mo\(sl'" %
ndefine nomem % C-/ %
tdefine angstrom % "\fR\zA\v'-.3m'\h'.2m'\(de\v'.3m'\fP\h'.2m'" %
ndefine angstrom % A to o %
tdefine star %{ roman "\v'.5m'\s+3*\s-3\v'-.5m'"}%
ndefine star % * %
tdefine || % \(or\(or %
tdefine <wig % "\z<\v'.4m'\(ap\v'-.4m'" %
ndefine <wig %{ < from "~" }%
tdefine >wig % "\z>\v'.4m'\(ap\v'-.4m'" %
ndefine >wig %{ > from "~" }%
tdefine langle % "\s-3\b'\(sl\e'\s0" %
ndefine langle %<%
tdefine rangle % "\s-3\b'\e\(sl'\s0" %
ndefine rangle %>%
tdefine hbar % "\zh\v'-.6m'\h'.05m'\(ru\v'.6m'" %
ndefine hbar % h\u-\d %
ndefine ppd % _| %
tdefine ppd % "\o'\(ru\s-2\(or\s+2'" %
tdefine <-> % "\o'\(<-\(->'" %
ndefine <-> % "<-->" %
tdefine <=> % "\s-2\z<\v'.05m'\h'.2m'\z=\h'.55m'=\h'-.6m'\v'-.05m'>\s+2" %
ndefine <=> % "<=>" %
tdefine |< % "\o'<\(or'" %
ndefine |< % <| %
tdefine |> % "\o'>\(or'" %
ndefine |> % |> %
tdefine ang % "\v'-.15m'\z\s-2\(sl\s+2\v'.15m'\(ru" %
ndefine ang % /_ %
tdefine rang % "\z\(or\h'.15m'\(ru" %
ndefine rang % L %
tdefine 3dot % "\v'-.8m'\z.\v'.5m'\z.\v'.5m'.\v'-.2m'" %
ndefine 3dot % .\u.\u.\d\d %
tdefine thf % ".\v'-.5m'.\v'.5m'." %
ndefine thf % ..\u.\d %
tdefine quarter % roman \(14 %
ndefine quarter % 1/4 %
tdefine 3quarter % roman \(34 %
ndefine 3quarter % 3/4 %
tdefine degree % \(de %
ndefine degree % nothing sup o %
tdefine square % \(sq %
ndefine square % [] %
tdefine circle % \(ci %
ndefine circle % O %
tdefine blot % "\fB\(sq\fP" %
ndefine blot % HIX %
tdefine bullet % \(bu %
ndefine bullet % oxe %
tdefine -wig % "\(~=" %
ndefine -wig % - to "~" %
tdefine wig % \(ap %
ndefine wig % "~" %
tdefine prop % \(pt %
ndefine prop % oc %
tdefine empty % \(es %
ndefine empty % O/ %
tdefine member % \(mo %
ndefine member % C- %
tdefine cup % \(cu %
ndefine cup % U %
define cap % \(ca %
define subset % \(sb %
define supset % \(sp %
define !subset % \(ib %
define !supset % \(ip %
.EN
_P_
$chown     root tmpos/usr/pub/eqnchar &
$chgrp   daemon tmpos/usr/pub/eqnchar &
$chmod      444 tmpos/usr/pub/eqnchar &
wait
/bin/cat > tmpos/usr/pub/greek << '_P_'
alpha	A  A  |  beta	B  B  |  gamma	\  \
GAMMA	G  G  |  delta	D  D  |  DELTA	W  W
epsilon	S  S  |  zeta	Q  Q  |  eta	N  N
THETA	T  T  |  theta	O  O  |  lambda	L  L
LAMBDA	E  E  |  mu	M  M  |  nu	@  @
xi	X  X  |  pi	J  J  |  PI	P  P
rho	K  K  |  sigma	Y  Y  |  SIGMA	R  R
tau	I  I  |  phi	U  U  |  PHI	F  F
psi	V  V  |  PSI	H  H  |  omega	C  C
OMEGA	Z  Z  |  nabla	[  [  |  not	_  _
partial	]  ]  |  integral ^ ^
_P_
$chown     root tmpos/usr/pub/greek &
$chgrp   daemon tmpos/usr/pub/greek &
$chmod      444 tmpos/usr/pub/greek &
wait
/bin/cat >  tmpos/usr/spool/secretmail/notice << '_P_'
Secret mail has arrived.
_P_
$chown     root tmpos/usr/spool/secretmail/notice &
$chgrp   daemon tmpos/usr/spool/secretmail/notice &
$chmod      444 tmpos/usr/spool/secretmail/notice &
wait
/bin/cat > tmpos/usr/crash/minfree << '_P_'
1000
_P_
$chown     root tmpos/usr/crash/minfree &
$chgrp   daemon tmpos/usr/crash/minfree &
$chmod      644 tmpos/usr/crash/minfree &
wait
/bin/cat > tmpos/usr/crash/bounds << '_P_'
0
_P_
$chown     root tmpos/usr/crash/bounds &
$chgrp   daemon tmpos/usr/crash/bounds &
$chmod      644 tmpos/usr/crash/bounds &
wait
/bin/cat > tmpos/.profile << '_P_'
stty -raw -cbreak echo; stty
PATH=/etc:/usr/etc:/usr/ucb:/bin:/usr/bin:.; export PATH
HOME=/; export HOME
export TERM
_P_
$chown     root tmpos/.profile &
$chgrp   daemon tmpos/.profile &
$chmod     0644 tmpos/.profile &
wait
/bin/cat > tmpos/.cshrc << '_P_'
set path=(/etc /usr/etc /usr/ucb /bin /usr/bin .)
if ($?prompt) then
	set history=64
	set prompt="`hostname`(\!)# "
endif
_P_
$chown     root tmpos/.cshrc &
$chgrp   daemon tmpos/.cshrc &
$chmod     0644 tmpos/.cshrc &
wait
/bin/cat > tmpos/.login << '_P_'
stty dec crt
_P_
$chown     root tmpos/.login &
$chgrp   daemon tmpos/.login &
$chmod     0644 tmpos/.login &
wait
/bin/cat > tmpos/.rhosts << '_P_'
host1
host2
host3
host4
_P_
$chown     root tmpos/.rhosts &
$chgrp   daemon tmpos/.rhosts &
$chmod     0644 tmpos/.rhosts &
wait
/bin/cat > tmpos_att/etc/profile << '_P_'

#	@(#)profile.sh	1.6

trap "" 1 2 3
case "$0" in
-sh | -rsh)
	trap : 1 2 3
	echo "UNIX System V Release `uname -r|cut -dv -f1` `uname -m` Version `uname -r|cut -dv -f2`"
	uname -n
	echo "Copyright (c) 1984 AT&T Technologies, Inc.\nAll Rights Reserved\n"
	cat /etc/motd
	trap "" 1 2 3
	if mail -e
	then echo "you have mail"
	fi
	if [ "$LOGNAME" != root ]
	then
		news -n
	fi
	;;
-su)
	:
	;;
esac
trap 1 2 3
_P_
$chown     root tmpos_att/etc/profile &
$chgrp   daemon tmpos_att/etc/profile &
$chmod      755 tmpos_att/etc/profile &
wait
/bin/cat > tmpos_att/etc/rc.sys5 << '_P_'
			echo 'System V: shmem\c'>/dev/console
/bin/rm -rf /usr/tmp/SysVshmem
/bin/mkdir /usr/tmp/SysVshmem
/bin/chmod 777 /usr/tmp/SysVshmem
			echo ' editor files\c'	>/dev/console
(cd /tmp; /usr/lib/expreserve -)
/bin/ps -ef >/dev/null
# /bin/rm -f /usr/spool/lp/SCHEDLOCK
# /usr/lib/lpsched
#			echo ' lpsched\c' >/dev/console
			echo '.'		>/dev/console
_P_
$chown     root tmpos_att/etc/rc.sys5 &
$chgrp   daemon tmpos_att/etc/rc.sys5 &
$chmod      644 tmpos_att/etc/rc.sys5 &
wait
/bin/cat > tmpos_att/usr/bin/mailbug << '_P_'
/bin/ucb /usr/bin/mailbug
_P_
$chown     root tmpos_att/usr/bin/mailbug &
$chgrp   daemon tmpos_att/usr/bin/mailbug &
$chmod      755 tmpos_att/usr/bin/mailbug &
wait
/bin/cat > tmpos_att/etc/instl.svae << '_P_'
#! /bin/sh
#
# /etc/instl.svae (aka instl on the distribution tape )
#
# Install script for System V Release 2 Version 2 for DYNIX.  Should be run
# from root, single user with all file systems mounted. This script is the
# first tape file on the SVAE distribution tape.
#

TAPE=/dev/ctape8
PATH=:/bin:/usr/bin:/usr/ucb:/etc; export PATH

##
# Default ROOT for entire System V file structure.
# This can be changed although I do not recommend it.
##
R="/usr/att"

if /etc/mount | /bin/awk '$3=="/usr" {exit 1}'; then
	echo "Usr filesystem appears not to be mounted."
	echo "Please mount /usr and try again."
	exit 1
fi

if [ `whoami` != root ]; then
	echo "Sorry this script must be run as super-user to work properly."
	exit 1
fi

if [ "`universe`" != "ucb" ] ; then
	exec ucb $0 $@
fi

# in case the above gets munged
if [ -z "$R" ]; then
	R="/usr/att"
	echo "*** WARNING!: Assuming ROOT = ($R)"
fi

##
# Create ROOT directory for System V (if necessary)
##
if [ ! -d "$R" ]; then
	echo "*** Making root directory for System V ($R)"
	mkdir "$R"
	chmod 755 "$R"
	chown root "$R"
	chgrp daemon "$R"
else
	echo "*** ERROR!!: ($R) already exists, you should 'rm -rf' it first and try again"
	exit 1
fi
echo ""

##
# Extract from tape
##
echo "rewinding tape..."
mt -f $TAPE rew
mt -f $TAPE fsf 1		# move to second tape file
echo "*** Extracting System V from tape (into $R)"
cd "$R"
rm -f /tmp/done$$
(/bin/tar xpf $TAPE; echo $? > /tmp/done$$)&
cnt=0
while [ ! -f /tmp/done$$ ]
do
	echo -n .
	sleep 6
	cnt=`expr $cnt + 1`
	if [ $cnt -gt 79 ] ;then 
		echo ""
		cnt=0
	fi
done
stat=`cat /tmp/done$$`
rm /tmp/done$$
echo ""
case $stat in
0)	;;
*)	echo "Problems in un-taring tape file"
	exit 1 ;;
esac
/bin/mt -f $TAPE rew
echo ""

##
# Create conditional symbolic links (if necessary).  DYNIX directory 
# gets renamed by adding dot ('.') prefix.
# BTW: We need a private copy of mv and ln since /bin gets moved in the process.
##
cd /
cp /bin/mv /mv; MV=/mv
cp /bin/ln /ln; LN=/ln
D="bin lib CD-TO-USR bin include lib pub"
P=
/bin/cat << 'EOF'

*** Creating directory symbolic links
EOF

for I in $D; do
	if [ "$I" = "CD-TO-USR" ]; then
		cd /usr
		P=usr/
		continue
	fi
	if [ -d .$I -a -d $I -a -d $R/$P$I ]; then
		echo "	/$P$I: skipped"
	else
		echo	"	/$P$I"
		${MV} $I .$I
		${LN} -c ucb=.$I att=$R/$P$I /$P$I
	fi
done
cd /
rm -f ${MV} ${LN}

##
# Create symbolic links in System V for shared programs that
# really live in ucb directory tree.
##
/bin/cat << 'EOF'

*** Creating file symbolic links
EOF
P=
while read F; do
	if [ "$F" = "CD-TO-USR" ]; then
		cd /usr
		P=usr/
		continue
	fi
	rm -f $R/$P$F
	ln -s /$P.$F $R/$P$F
	echo "	/$P$F"
done << 'EOF'
bin/ar
bin/as
bin/cc
bin/csh
bin/ksh
bin/rksh
bin/df
bin/ld
bin/login
bin/nm
bin/passwd
bin/size
bin/strip
bin/wall
bin/who
bin/write
bin/universe
bin/att
bin/ucb
lib/ccom
lib/c2
lib/wccom
CD-TO-USR
bin/ranlib
bin/lorder
bin/prof
EOF

##
# Create symbolic links in DYNIX for shared programs that
# really live in att directory tree.
##
/bin/cat << 'EOF'

*** Creating DYNIX symbolic links
EOF
while read F; do
	rm -f /$F
	ln -s $R/$F /$F
	echo "	/$F"
done << 'EOF'
etc/chroot
etc/magic
etc/profile
etc/rc.sys5
usr/catman
EOF

##
# Misc
##
rm -rf /usr/mail; ln -s /usr/spool/mail /usr/mail
rm -f $R/usr/lib/libg.a; ln -s /usr/.lib/libg.a $R/usr/lib/libg.a
rm -f /usr/news; ln -s $R/usr/news /usr/news
rm -rf /usr/spool/lp; ln -s $R/usr/spool/lp /usr/spool/lp
if [ ! -f /etc/.mknod ] ;then
	mv /etc/mknod /etc/.mknod
	ln -c att=$R/etc/mknod ucb=/etc/.mknod /etc/mknod
fi
rm -f $R/usr/lib/tabset; ln -s /usr/.lib/tabset $R/usr/lib/tabset
if [ -f /.bin/ddt ]; then
	ln -s /.bin/ddt $R/bin/ddt
fi
ln -s $R/usr/lib/terminfo /usr/.lib/terminfo

# This solves name collision problems
rm -f /etc/install; ln -c ucb=/usr/bin/install att=$R/etc/install /etc/install
echo "`/bin/date`  - DYNIX V3.2.0 SVAE PN: 1003-xxxxx" >> /etc/versionlog
if [ -f /usr/.bin/crypt ]; then
	echo "Encryption software must be re-installed to affect SVAE."
fi
echo "All done."
exit 0
_P_
$chown     root tmpos_att/etc/instl.svae &
$chgrp   daemon tmpos_att/etc/instl.svae &
$chmod      744 tmpos_att/etc/instl.svae &
wait
/bin/mkdir	tmpos/tftpboot
$chown root	tmpos/tftpboot &
$chgrp daemon	tmpos/tftpboot &
$chmod 755	tmpos/tftpboot &
wait
/bin/cat > tmpos/etc/inetd.conf << '_P_'
daytime	stream	tcp	nowait	root	internal
time	stream	tcp	nowait	root	internal
echo	stream	tcp	nowait	root	internal
discard	stream	tcp	nowait	root	internal
chargen	stream	tcp	nowait	root	internal
ftp	stream	tcp	nowait	root	/usr/etc/ftpd		ftp
telnet	stream	tcp	nowait	root	/usr/etc/telnetd	telnetd
shell	stream	tcp	nowait	root	/usr/etc/rshd		rshd
login	stream	tcp	nowait	root	/usr/etc/rlogind	rlogind
exec	stream	tcp	nowait	root	/usr/etc/rexecd		rexecd
comsat	dgram	udp	wait	root	/usr/etc/comsat		comsat
tftp	dgram	udp	wait	root	/usr/etc/tftpd 		tftpd -s /tftpboot
ntalk	dgram	udp	wait	root	/usr/etc/talkd		talkd
finger	stream	tcp	nowait	nobody	/usr/etc/fingerd	fingerd
echo	dgram	udp	wait	root	internal
time	dgram	udp	wait	root	internal
rstatd/1-3	dgram	rpc/udp	wait	root	/usr/etc/rpc.rstatd	rpc.rstatd
rusersd/1-2	dgram	rpc/udp	wait	root	/usr/etc/rpc.rusersd	rpc.rusersd
walld/1		dgram	rpc/udp	wait	root	/usr/etc/rpc.rwalld  	rpc.rwalld
mountd/1	dgram	rpc/udp	wait	root	/usr/etc/rpc.mountd  	rpc.mountd
sprayd/1	dgram	rpc/udp	wait	root	/usr/etc/rpc.sprayd  	rpc.rsprayd
rquotad/1	dgram	rpc/udp wait	root	/usr/etc/rpc.rquotad 	rpc.rquotad
_P_
$chown     root tmpos/etc/inetd.conf &
$chgrp   daemon tmpos/etc/inetd.conf &
$chmod      644 tmpos/etc/inetd.conf &
wait
/bin/cat > tmpos/etc/rpc << '_P_'
#
#	rpc   1.2   86/01/07
#
rstatd		100001	rstat rup perfmeter
rusersd		100002	rusers
nfs		100003	nfsprog
ypserv		100004	ypprog
mountd		100005	mount showmount
ypbind		100007
walld		100008	rwall shutdown
yppasswdd	100009	yppasswd
etherstatd	100010	etherstat
rquotad		100011	rquotaprog quota rquota
sprayd		100012	spray
selection_svc	100015	selnsvc
dbsessionmgr	100016	unify netdbms dbms
rexd		100017	rex remote_exec
office_auto	100018	alice
_P_
$chown     root tmpos/etc/rpc &
$chgrp   daemon tmpos/etc/rpc &
$chmod      644 tmpos/etc/rpc &
wait
/bin/cat > tmpos/etc/netgroup << '_P_'
_P_
$chown     root tmpos/etc/netgroup &
$chgrp   daemon tmpos/etc/netgroup &
$chmod      644 tmpos/etc/netgroup &
wait
/bin/cat > tmpos/etc/shells << '_P_'
/bin/sh
/bin/csh
/bin/ksh
_P_
$chown     root tmpos/etc/shells &
$chgrp   daemon tmpos/etc/shells &
$chmod      644 tmpos/etc/shells &
wait
/bin/cat > tmpos/etc/resolv.conf << '_P_'
#domain		sequent.com
#nameserver	127.1
_P_
$chown     root tmpos/etc/resolv.conf &
$chgrp   daemon tmpos/etc/resolv.conf &
$chmod      644 tmpos/etc/resolv.conf &
wait
/bin/cat > tmpos/etc/bootptab << '_P_'
# /etc/bootptab: database for bootp server (/etc/bootpd)
# Blank lines and lines beginning with '#' are ignored.
#
# The entries provided below are here as examples
#
# Legend:
#
#	first field -- hostname
#			(may be full domain name and probably should be)
#
#	hd -- home directory
#	bf -- bootfile
#	cs -- cookie servers
#	ds -- domain name servers
#	gw -- gateways
#	ha -- hardware address
#	ht -- hardware type
#	im -- impress servers
#	ip -- host IP address
#	lg -- log servers
#	lp -- LPR servers
#	ns -- IEN-116 name servers
#	rl -- resource location protocol servers
#	sm -- subnet mask
#	tc -- template host (points to similar host entry)
#	to -- time offset (seconds)
#	ts -- time servers

# Be careful about including backslashes where they're needed.  Weird (bad)
# things can happen when a backslash is omitted where one is intended.
#
# First, we define a global entry which specifies the stuff every host uses.

global.dummy:\
	:sm=255.255.255.0:\
	:hd=/usr/boot:bf=null:\
	:ds=128.2.35.50 128.2.13.21:\
	:ns=0x80020b4d 0x80020ffd:\
	:ts=0x80020b4d 0x80020ffd:\
	:to=18000:

# Next, we can define different master entries for each subnet. . .

subnet13.dummy:\
	:tc=global.dummy:gw=128.2.13.1:

# We should be able to use as many levels of indirection as desired.  Use
# your imagination. . .

# Individual entries (could also have different servers for some/all of these
# hosts, but we don't really use this feature at CMU):

carnegie:tc=subnet13.dummy:ht=ieee802:ha=7FF8100000AF:ip=128.2.11.1:
_P_
$chown     root tmpos/etc/bootptab &
$chgrp   daemon tmpos/etc/bootptab &
$chmod      644 tmpos/etc/bootptab &
wait
/bin/cat > tmpos/etc/named.boot << '_P_'
;
;  boot file for authoritive master name server for COMPANYNAME.COM
;  Note that there should be one primary entry for each SOA record.
;
;

; type    domain				source host/file		backup file

domain		COMPANYNAME.com
primary   	COMPANYNAME.COM			/etc/named.hosts
primary   	0.103.IN-ADDR.ARPA		/etc/named.hosts.rev
primary   	0.0.127.IN-ADDR.ARPA	localhost.rev
_P_
$chown     root tmpos/etc/named.boot &
$chgrp   daemon tmpos/etc/named.boot &
$chmod      644 tmpos/etc/named.boot &
wait
/bin/cat > tmpos/etc/named.local << '_P_'
;
;	named.local
;

@	IN	SOA	MACHINE.COMPANYNAME.com  root.MACHINE.COMPANYNAME.com (
								1.1     ; Serial
								3600    ; Refresh
								300     ; Retry
								3600000 ; Expire
								14400 )  ; Minimum
	IN	NS	MACHINE.COMPANYNAME.com.
1	IN	PTR	localhost.
_P_
$chown     root tmpos/etc/named.local &
$chgrp   daemon tmpos/etc/named.local &
$chmod      644 tmpos/etc/named.local &
wait
/bin/cat > tmpos/etc/named.hosts << '_P_'
; Authoritative data for SOMECOMPANY.COM (ORIGIN assumed SOMECOMPAY.COM)
;
@	IN	SOA	MACHINE.COMPANYNAME.com root.MACHINE.COMPANYNAME.com	(
				1.1		; Serial
				10800	; Refresh 3 hours
				3600	; Retry   1 hour
				3600000	; Expire  1000 hours
				86400 )	; Minimum 24 hours
				IN	NS		MACHINE.COMPANYNAME.COM
localhost		IN	A		127.1
HOST_A	        IN	A	    103.0.1.03
				IN	HINFO	    s27
HOST_B	       	IN	A	   	103.0.1.06
			 	IN	HINFO	    s27
HOST_C			IN	A		103.0.1.07
				IN 	HINFO		s81
_P_
$chown     root tmpos/etc/named.hosts &
$chgrp   daemon tmpos/etc/named.hosts &
$chmod      644 tmpos/etc/named.hosts &
wait
/bin/cat > tmpos/etc/named.hosts.rev << '_P_'
;
;	sample reverse address example
;

@	IN	SOA	MACHINE.COMPANYNAME.COM root.MACHINE.COMPANYNAME.com (
				1.1		; Serial
				10800	; Refresh 3 hours
				3600	; Retry   1 hour
				3600000 ; Expire  1000 hours
				86400 )	; Minimum 24 hours
		IN	NS	HOST_A.sequent.COM
3.1		IN	PTR	HOST_B.sequent.COM.
6.1		IN	PTR	HOST_C.sequent.COM.
7.1		IN	PTR	HOST_A.sequent.COM.
_P_
$chown     root tmpos/etc/named.hosts.rev &
$chgrp   daemon tmpos/etc/named.hosts.rev &
$chmod      644 tmpos/etc/named.hosts.rev &
wait
/bin/cat > tmpos_att/etc/svaedirs << '_P_'
bin
etc
usr
usr/bin
usr/lib
_P_
/bin/cat > tmpos_att/etc/svaefiles << '_P_'
bin/bs
bin/diff
bin/ed
bin/red
bin/file
bin/ipcs
bin/ipcrm
bin/make
bin/sed
bin/sh
bin/rsh
bin/cat
bin/chgrp
bin/chmod
bin/chown
bin/cmp
bin/cpio
bin/date
bin/dd
bin/du
bin/echo
bin/env
bin/expr
bin/find
bin/grep
bin/kill
bin/line
bin/ls
bin/mesg
bin/mv
bin/nice
bin/od
bin/nohup
bin/pr
bin/ps
bin/pwd
bin/rm
bin/sleep
bin/sort
bin/stty
bin/sum
bin/sync
bin/tail
bin/tee
bin/time
bin/touch
bin/tty
bin/uname
bin/wc
bin/mail
bin/mkdir
bin/rmdir
bin/su
bin/basename
bin/dirname
bin/false
bin/true
bin/pdp11
bin/u370
bin/u3b10
bin/u3b2
bin/u3b5
bin/u3b
bin/vax
bin/ns32000
bin/i386
bin/rmail
bin/cp
bin/ln
etc/profile
etc/rc.sys5
etc/magic
etc/chroot
etc/mknod
etc/install
lib
usr/bin/graf
usr/bin/mailbug
usr/bin/tic
usr/bin/awk
usr/bin/bc
usr/bin/bdiff
usr/bin/bfs
usr/bin/calendar
usr/bin/cflow
usr/bin/ctrace
usr/bin/ctcr
usr/bin/ctc
usr/bin/cxref
usr/bin/dc
usr/bin/diff3
usr/bin/efl
usr/bin/graphics
usr/bin/hpio
usr/bin/lex
usr/bin/lint
usr/bin/m4
usr/bin/mailx
usr/bin/man
usr/bin/ratfor
usr/bin/admin
usr/bin/comb
usr/bin/delta
usr/bin/get
usr/bin/help
usr/bin/prs
usr/bin/rmdel
usr/bin/sccsdiff
usr/bin/unget
usr/bin/val
usr/bin/vc
usr/bin/what
usr/bin/cdc
usr/bin/sact
usr/bin/sno
usr/bin/spell
usr/bin/tplot
usr/bin/ex
usr/bin/vi
usr/bin/view
usr/bin/edit
usr/bin/vedit
usr/bin/yacc
usr/bin/300
usr/bin/300s
usr/bin/4014
usr/bin/450
usr/bin/asa
usr/bin/cal
usr/bin/banner
usr/bin/cb
usr/bin/checkeq
usr/bin/col
usr/bin/comm
usr/bin/cpset
usr/bin/csplit
usr/bin/cut
usr/bin/deroff
usr/bin/egrep
usr/bin/factor
usr/bin/fgrep
usr/bin/fsplit
usr/bin/getopt
usr/bin/graph
usr/bin/hp
usr/bin/id
usr/bin/join
usr/bin/logname
usr/bin/newform
usr/bin/news
usr/bin/nl
usr/bin/pack
usr/bin/paste
usr/bin/pg
usr/bin/regcmp
usr/bin/sdiff
usr/bin/split
usr/bin/spline
usr/bin/tabs
usr/bin/tar
usr/bin/tput
usr/bin/tr
usr/bin/tsort
usr/bin/uniq
usr/bin/units
usr/bin/unpack
usr/bin/xargs
usr/bin/dircmp
usr/bin/greek
usr/bin/pcat
usr/bin/cancel
usr/bin/disable
usr/bin/enable
usr/bin/lp
usr/bin/lpstat
usr/bin/checkmm
usr/bin/checkmm1
usr/bin/di10
usr/bin/diffmk
usr/bin/eqn
usr/bin/grap
usr/bin/hc
usr/bin/hyphen
usr/bin/macref
usr/bin/mm
usr/bin/mmt
usr/bin/mvt
usr/bin/ndx
usr/bin/neqn
usr/bin/nroff
usr/bin/pic
usr/bin/ptx
usr/bin/subj
usr/bin/ta
usr/bin/tbl
usr/bin/tc
usr/bin/troff
usr/bin/whatis
usr/include
usr/lib/ctrace
usr/lib/dwb
usr/lib/graf
usr/lib/help
usr/lib/lex
usr/lib/macros
usr/lib/mailx
usr/lib/nterm
usr/lib/font
usr/lib/spell
usr/lib/terminfo
usr/lib/tmac
usr/lib/gcrt0.o
usr/lib/libc_p.a
usr/lib/libl.a
usr/lib/libmalloc_p.a
usr/lib/libm_p.a
usr/lib/liby.a
usr/lib/libPW_p.a
usr/lib/libcrypt.a
usr/lib/libplot.a
usr/lib/libmalloc.a
usr/lib/libcurses.a
usr/lib/libplot_p.a
usr/lib/lib4014.a
usr/lib/libPW.a
usr/lib/lib4014_p.a
usr/lib/lib300.a
usr/lib/lib300_p.a
usr/lib/lib300s.a
usr/lib/lib300s_p.a
usr/lib/lib450.a
usr/lib/lib450_p.a
usr/lib/libvt0.a
usr/lib/libvt0_p.a
usr/lib/lib.b
usr/lib/calprog
usr/lib/dag
usr/lib/lpfx
usr/lib/nmf
usr/lib/flip
usr/lib/xpass
usr/lib/xcpp
usr/lib/diffh
usr/lib/diff3prog
usr/lib/lint1
usr/lib/lint2
usr/lib/llib-lc
usr/lib/llib-lc.ln
usr/lib/llib-port
usr/lib/llib-port.ln
usr/lib/llib-lm
usr/lib/llib-lm.ln
usr/lib/llib-lmalloc
usr/lib/llib-lmalloc.ln
usr/lib/eign
usr/lib/t4014
usr/lib/t300
usr/lib/t300s
usr/lib/t450
usr/lib/vplot
usr/lib/exrecover
usr/lib/expreserve
usr/lib/yaccpar
usr/lib/mv_dir
usr/lib/makekey
usr/lib/unittab
usr/lib/accept
usr/lib/lpadmin
usr/lib/lpmove
usr/lib/lpsched
usr/lib/lpshut
usr/lib/reject
usr/lib/hp2631a
usr/lib/prx
usr/lib/pprx
usr/news
usr/pub
usr/catman
usr/spool/lp
_P_
#
# For i386, which has libfpac, add it on the SVAE side also
#
if [ "${MACHINE}" = "i386" ]
then
	echo usr/lib/libfpac_p.a >> tmpos_att/etc/svaefiles
fi
$chown     root tmpos_att/etc/svaefiles &
$chgrp   daemon tmpos_att/etc/svaefiles &
$chmod      644 tmpos_att/etc/svaefiles &
wait
/bin/cat > tmpos_stripe/etc/instl.stripe << '_P_'
#! /bin/sh
#
# /etc/instl.stripe
#
# Shell script to install the disk striping software distribution.
# It updates /etc/versionlog to log installation of software. It
# checks to be sure that the unformatted man pages have been installed
# in /usr/man prior to installing those for striping. Must be run as
# root.  
#
# This file should be the first tape file on distribution tape.
# The second tape file should be a tar image of DYNIX striping software.
# The third tape file should be a tar image of DYNIX striping man pages.
#
TAPE=/dev/ctape8
PATH=:/bin:/usr/bin:/usr/ucb:/etc; export PATH
TMP=/tmp/instl$$

# The next variable is the location of the unformatted man pages.
MAN="/usr/man/man1"

trap "rm -f $TMP;exit" 0 1 2 3

(cd /usr; umount /usr >$TMP 2>&1)

if grep -s "not mounted" $TMP; then
	echo "Usr filesystem not mounted--Please mount /usr and try again."
	exit 1
fi
rm $TMP

if [ `whoami` != root ]; then
	echo "Sorry this script must be run as super-user to work properly."
	exit 1
fi

if [ "`universe`" != "ucb" ] ; then
	exec ucb $0 $@
fi

cd /
echo "rewinding tape..."
mt -f $TAPE rew
echo "extracting DYNIX striping software..."
mt -f $TAPE fsf 1		# move to second tape file
tar xpf $TAPE			# extract DYNIX striping software
echo "DYNIX striping software installed"

if [ ! -d $MAN ]; then
	echo "Unformatted man pages appear not to be installed."
	echo "Unformatted man pages for striping will not be installed."
else
	cd $MAN/..
	echo "extracting unformatted man pages for DYNIX striping..."
	tar xpf $TAPE		# extract unformatted stripe man pages
	echo "Unformatted man pages for DYNIX striping installed"
fi

echo "Rewinding tape..."
mt -f $TAPE rew

if [ -s /usr/lib/whatis ] ; then
	echo -n "Updating /usr/lib/whatis..."
	cat /usr/lib/whatis.stripe >>/usr/lib/whatis
	sort /usr/lib/whatis -o /usr/lib/whatis
	echo "done"
fi
rm /usr/lib/whatis.stripe

echo "`/bin/date`  - DYNIX V3.2.0 Disk Striping SW PN: 1003-XXXXX" >>/etc/versionlog

echo "All done."
_P_
$chown     root tmpos_stripe/etc/instl.stripe &
$chgrp   daemon tmpos_stripe/etc/instl.stripe &
$chmod      744 tmpos_stripe/etc/instl.stripe &
wait
/bin/cat > tmpos_mirror/etc/instl.mirror << '_P_'
#! /bin/sh
#
# /etc/instl.mirror
#
# Shell script to install the disk mirroring software distribution.
# It updates /etc/versionlog to log installation of software. It
# checks to be sure that the unformatted man pages have been installed
# in /usr/man prior to installing those for mirroring. Must be run as
# root.  
#
# This file should be the first tape file on distribution tape.
# The second tape file should be a tar image of DYNIX mirroring software.
# The third tape file should be a tar image of DYNIX mirroring man pages.
#
TAPE=/dev/ctape8
PATH=:/bin:/usr/bin:/usr/ucb:/etc; export PATH
TMP=/tmp/instl$$

# The next variable is the location of the unformatted man pages.
MAN="/usr/man/man?"

trap "rm -f $TMP;exit" 0 1 2 3

(cd /usr; umount /usr >$TMP 2>&1)

if grep -s "not mounted" $TMP; then
	echo "Usr filesystem not mounted--Please mount /usr and try again."
	exit 1
fi
rm $TMP

if [ `whoami` != root ]; then
	echo "Sorry this script must be run as super-user to work properly."
	exit 1
fi

if [ "`universe`" != "ucb" ] ; then
	exec ucb $0 $@
fi

cd /
echo "rewinding tape..."
mt -f $TAPE rew
echo "extracting DYNIX mirroring software..."
mt -f $TAPE fsf 1		# move to second tape file
tar xpf $TAPE			# extract DYNIX mirroring software
echo "DYNIX mirroring software installed"

if [ ! -d $MAN ]; then
	echo "Unformatted man pages appear not to be installed."
	echo "Unformatted man pages for mirroring will not be installed."
else
	cd $MAN/..
	echo "extracting unformatted man pages for DYNIX mirroring..."
	tar xpf $TAPE		# extract unformatted mirror man pages
	echo "Unformatted man pages for DYNIX mirroring installed"
fi

echo "Rewinding tape..."
mt -f $TAPE rew

if [ -s /usr/lib/whatis ] ; then
	echo -n "Updating /usr/lib/whatis..."
	cat /usr/lib/whatis.mirror >>/usr/lib/whatis
	sort /usr/lib/whatis -o /usr/lib/whatis
	echo "done"
fi
rm /usr/lib/whatis.mirror

echo "`/bin/date`  - DYNIX V3.2.0 Disk Mirroring SW PN: 1003-XXXXX" >> /etc/versionlog

echo "All done."
_P_
$chown     root tmpos_mirror/etc/instl.mirror &
$chgrp   daemon tmpos_mirror/etc/instl.mirror &
$chmod      744 tmpos_mirror/etc/instl.mirror &
wait
/bin/cat > tmpos_mirror/etc/mrtab << '_P_'
# This file is a list of the permanent mirrors in the system.  The format is:
#	<mirror_name>	<unit0_name>	<unit1_name>
# Comments run from a '#' to the end of line.  All names are fully
# qualified path names.

_P_
$chown     root tmpos_mirror/etc/mrtab &
$chgrp   daemon tmpos_mirror/etc/mrtab &
$chmod      644 tmpos_mirror/etc/mrtab &
wait
