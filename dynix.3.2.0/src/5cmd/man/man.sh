#	Catable manual entries printer
#
# SCCS @(#)man.sh	1.1

BASE=/usr

notes="$BASE/catman/notes"
sec=\?
dir=
col=
cmd=
entries=
TTERM=


#  parse options

for i in $*
do case $i in

	[1-8])	sec=$i ;;
	-c)	col=col ;;
	-d)	dir=. ;;
	-w)	cmd=w ;;
	-T*)	TERM="`echo $i | sed 's/-T//'`" ;;
	-12)	TTERM=-12 ;;
	*)	entries="$entries $i" ;;
   esac
done


#  now find the named entries

for i in $entries
do
    if [ "$dir" = "." ]

	then if [ ! -r "$i" ]

		then  echo $0: $i not found >&2
		      exit 1

		else  all="$all $i"

	     fi

	else
	     cd $BASE/catman
	     fil="`find */man$sec/$i.* -print 2>/dev/null`"
	     if [ -n "$fil" ]

		then  all="$all $fil"
		else  echo $0: $i not found >&2
		      exit 1

	     fi
    fi
done


# fix up the terminal type option (make sure to use col, etc.)

case "$TERM$TTERM" in

	300|300s|450|37|300-12|300s-12|450-12| \
		4000a|4000A|382|1620|1620-12)	post="greek -T$TERM$TTERM" ;;
	hp|2621|2645|2640)	col=col; post="greek -T$TERM$TTERM" ;;
	4014|tek)	post="greek -T$TERM$TTERM" ;;
	X97)		col=col; post=xerox ;;
	*)		col=col; post=cat ;;
esac

if [ "$col" != "" ]
   then post="$col | $post"
fi


# now print the pages

if [ "$cmd" = "w" ]

   then	echo $all
	exit 0

   else	for i in $all

	do if [ -f "$notes" -a -r "$notes" ]; then
		egrep "^$i" "$notes" | sed -e 's/^[^ 	]*[ 	]*//' > /tmp/man.$$
		if [ -s /tmp/man.$$ ]; then
			cat /tmp/man.$$
			rm -f /tmp/man.$$
			echo 'view the AT&T manual page? [(y)n] \c'
			read ans
			case "$ans" in
				y*|Y*|"") ;;
				*) continue ;;
			esac
		fi
		rm -f /tmp/man.$$
	   fi
	
	   case $i in

		*.z)	eval "pcat $i | $post" ;;
		*)	eval "cat $i | $post" ;;

	  esac
	done

fi
