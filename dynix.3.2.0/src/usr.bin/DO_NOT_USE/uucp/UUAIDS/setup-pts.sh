#!/bin/sh
# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: setup-pts.sh 2.6 91/01/29 $

PATH=/etc:/usr/etc:/usr/ucb:/bin:/usr/bin:.; export PATH
if [ `whoami` != "root" ]
then
	echo "$0: Must be run as root"
	exit 1
fi
if [ ! -d /usr/bin ]; then
	echo "$0: /usr filesystem does not appear to be mounted"
	echo "$0: mount it and try again"
	exit 1
fi
if [ `universe` != ucb ]; then
	echo "$0: must be run from the ucb universe"
	exit 1
fi
#
#	Must be run as root while with at least the /usr file system mounted
#
# ----------------------------------------------------------------------
HOST=`hostname`
if [ "$HOST" = "" ]; then
	echo "$0: hostname not set"
	exit 1
fi

echo -n "Is your connection to sequent via Internet? "; read ANS
if [ "$ANS" != "y" ]
then
echo -n "Terminal line that modem is attached to: (e.g. ttyh0) "; read TTY
echo "Supported modem types:
  dn11 hayes hayespulse hayestone hayes2400 hayes2400pulse hayes2400tone
  hayesq hayesqpulse hayesqtone cds224 novation DF02 DF112P DF112T ventel
  penril vadic va212 va811s va820 rvmacs vmacs att
  (run \"man L-devices\" for more info)
  "
echo -n "Modem type: "; read MODEM
echo -n "Baud rate: "; read BAUD
echo -n "Phone Number: "; read NUMBER
echo -n "Name of remote machine: "; read REMOTEHOST
echo -n "Login name on remote machine: "; read ACCOUNT
echo -n "Password on remote machine: "; read PASSWORD

echo "using: modem=$MODEM phone=$NUMBER remote=$REMOTEHOST login=$ACCOUNT passwd=$PASSWORD"
# ----------------------------------------------------------------------
#	Set up D.hostname and D.hostnameX in /usr/spool/uucp
#
cd /usr/spool/uucp
if [ -d D.hostname -a -d D.hostnameX ]; then
	echo "** Setting up D.hostname files in /usr/spool/uucp"
	mv D.hostname D.${HOST}
	mv D.hostnameX D.${HOST}X
fi
# ----------------------------------------------------------------------
#
cd /usr/lib/uucp
if [ ! -f L.sys.orig ]; then
	( umask 022
	  echo "** Saving example L.sys, L-devices, L.aliases, and L-dialcodes"
	  cp L.sys L.sys.orig
	  cp L-devices L-devices.orig
	  cp L.aliases L.aliases.orig
	  cp L-dialcodes L-dialcodes.orig
	  chown uucp L*.orig
	)
fi

if cmp -s L.aliases L.aliases.orig; then
	cp /dev/null L.aliases
fi

if cmp -s L.sys L.sys.orig; then
	cp /dev/null L.sys
else
	grep -v "^${REMOTEHOST}" L.sys > TMP.FILE
	cp TMP.FILE L.sys
	rm -f TMP.FILE
fi

echo "** Adding entry to L.sys"
cat >> L.sys << EOF
${REMOTEHOST} Any ACU ${BAUD} ${NUMBER} "" "" ogin:--ogin:--ogin: ${ACCOUNT} ssword: ${PASSWORD}
EOF
#

if cmp -s L-devices L-devices.orig; then
	cp /dev/null L-devices
else
	grep -v "^ACU ${TTY}" L-devices > TMP.FILE
	cp TMP.FILE L-devices
	rm -f TMP.FILE
fi

echo "** Adding entry to L-devices"
cat >> L-devices << EOF
ACU ${TTY} unused ${BAUD} ${MODEM}
EOF
# ----------------------------------------------------------------------
#	Make sure there's no getty on /dev/$TTY	(ed /etc/ttys), and
#	that we can read and write it.
#
if grep -s "^1.${TTY}" /etc/ttys; then
	echo "** Editing /etc/ttys to disable logins"
	ed /etc/ttys << EOF >/dev/null 2>&1
/$TTY/
s/^1/0/
w
q
EOF
	kill -HUP 1
fi
chmod 666 /dev/$TTY
# ----------------------------------------------------------------------
#	Get cron to do uucp administration -- poll $REMOTEHOST, clean
#	up log files daily and weekly.
#
if grep -s uupoll /usr/lib/crontab; then
	: 'already in crontab file'
else
	echo "** Adding lines to /usr/lib/crontab for polling and cleanup"
	cat >> /usr/lib/crontab << EOF
15 6,18 * * *	/usr/bin/uupoll ${REMOTEHOST}
15 3    * * * 	sh /usr/lib/uucp/uu.daily
25 3    * * 6	sh /usr/lib/uucp/uu.weekly
EOF
fi

# ----------------------------------------------------------------------
#	Alias pts so the mailbug command will work.  aliases.dir and
#	aliases.pag have to be initialized for 'newaliases' to work the
#	first time.
#
if grep -s '^pts:' /usr/lib/aliases; then
	: 'pts already in /usr/lib/aliases'
else
	echo "** Adding pts to /usr/lib/aliases"
	cat >> /usr/lib/aliases << EOF

# Alias for problem tracking system (mailbug)
pts:${REMOTEHOST}!pts
EOF
	cp /dev/null /usr/lib/aliases.dir
	cp /dev/null /usr/lib/aliases.pag
	chmod 644 /usr/lib/aliases.dir /usr/lib/aliases.pag
	newaliases
# ----------------------------------------------------------------------
#	Initialize the sendmail frozen configuration file.
#
	cp /dev/null /usr/lib/sendmail.fc
	/usr/lib/sendmail -bz
fi

cat << EOF

  Notify Sequent Service that they should add the line:

	`hostname` Never

  to their /usr/lib/uucp/L.sys file.

EOF
else # internet

REMOTEHOST=sequent.sequent.com

# ----------------------------------------------------------------------
#	Alias pts so the mailbug command will work.  aliases.dir and
#	aliases.pag have to be initialized for 'newaliases' to work the
#	first time.
#
if grep -s '^pts:' /usr/lib/aliases; then
	: 'pts already in /usr/lib/aliases'
else
	echo "** Adding pts to /usr/lib/aliases"
	cat >> /usr/lib/aliases << EOF

# Alias for problem tracking system (mailbug)
pts:pts@${REMOTEHOST}
EOF
	cp /dev/null /usr/lib/aliases.dir
	cp /dev/null /usr/lib/aliases.pag
	chmod 644 /usr/lib/aliases.dir /usr/lib/aliases.pag
	newaliases
# ----------------------------------------------------------------------
#	Initialize the sendmail frozen configuration file.
#
	cp /dev/null /usr/lib/sendmail.fc
	/usr/lib/sendmail -bz
fi
fi # Internet


#----------------------------------------------------------------------
#       Initialize the /usr/service/site-information and serial-number 
#	files.
SERVICE=/usr/service
echo ">> Setting up mailbug information files ..."

# check if /usr/service exists. If not create it.
if [ ! -s $SERVICE ]
then
   mkdir $SERVICE
fi

# check if the mailbug directory exists. If not create it and allow public 
# to write to it.

if [ ! -s $SERVICE/mailbug ]
then
    mkdir $SERVICE/mailbug
fi

chmod 777 $SERVICE/mailbug

# check if site-information file exists and is not blank. If not create it and
# prompt user for input.
if [ ! -s $SERVICE/site-information ]
then
    echo -n "Company Name: " ; read it
    echo "site_name: "$it > $SERVICE/site-information 
    echo -n "Address: " ; read it
    echo "address: "$it >> $SERVICE/site-information
    echo -n "City: " ; read it
    echo "city: "$it >> $SERVICE/site-information
    echo -n "State/Province Code: " ; read it
    echo "state_province_code: "$it >> $SERVICE/site-information
    echo -n "Postal Code: " ; read it
    echo "postal_code: "$it >> $SERVICE/site-information
    echo -n "Country: " ; read it
    echo "country: "$it >> $SERVICE/site-information
    echo -n "Phone Number: " ; read it
    echo "phone_number: "$it >> $SERVICE/site-information
    echo "Valid site types are: OEM, VAR, Distirbutor, and end user."
    echo -n "Site Type: " ; read it
    echo "site_type: "$it >> $SERVICE/site-information
    echo "Valid system types are: B8, B21, S27, S81, and S3"
    echo -n "System Type: " ; read it
    echo "system_type: "$it >> $SERVICE/site-information
    echo -n "Would you like to edit this information now? (y/n) " ; read it
	case "$it" in
	    [yY]* ) ${EDITOR-vi} $SERVICE/site-information ;;
	    *) ;;
	esac
fi

# check if serial-number file exists and is not blank. If not create it and 
# prompt user for input.
if [ ! -s $SERVICE/serial-number ]
then
    echo -n "System Serial Number: " ; read it
    echo "serial_number: "$it > $SERVICE/serial-number
    echo -n "Would you like to edit this information now? (y/n) " ; read it
	case "$it" in
   	    [yY]* ) ${EDITOR-vi} $SERVICE/serial-number ;;
   	    *) ;;
	esac
fi

echo "** All done"
