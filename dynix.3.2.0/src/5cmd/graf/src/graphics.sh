# @(#)graphics.sh	1.3
: Graphics environment change
: GRAF directory optionally used for graflog file and announcements.
: r option creates restricted environment.
:
GRAF=/usr/adm

if test "$TERM" = 4014
then /usr/bin/graf/tekset
fi

if test -w $GRAF/graflog
then
  if  tty>/dev/null
  then
    tty=`tty | sed "s/.*\///"`
    who | grep ${tty} >>${GRAF}/graflog
  else
    who am i >>${GRAF}/graflog
  fi
fi

if test -r ${GRAF}/announce
then  cat ${GRAF}/announce
fi

if test "$1" != "-r"
then
  PATH=/usr/bin/graf:$PATH PS1="^ " sh $@
else
  PATH=:/usr/bin/graf:/rbin:/usr/rbin:/bin:/usr/bin PS1="^ " rsh $@
fi
