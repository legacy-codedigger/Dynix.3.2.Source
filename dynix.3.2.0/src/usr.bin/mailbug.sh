#!/bin/ksh
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
#
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: mailbug.sh 2.8 91/03/06 $
PATH=/usr/ucb:/bin:/usr/bin:$PATH
trap 'rm -f $DES ; exit 1' 1 2 3 15

# turn off noclobber so we can do redirects into files that may exist,
# such as /dev/null.
set +o noclobber

# data defined by Customer Service
SERVICE=/usr/service
SITE=$SERVICE/site-information
SERIAL_NUM=$SERVICE/serial-number

# to be consistent with old system
COMPANY_NAME=/usr/lib/mailbug_co_name

# prompts
NOT_VALID="(... please enter a valid unique choice ...)"
PS3="(number/first letter)? "
ALPHA="a b c"

# create temporary file for problem report description
DES="/tmp/mailbug_"$USER"_"$$
while [ -f $DES ]
do
    for letter in $ALPHA
    do
        DES="/tmp/mailbug_"$USER"_"$$"_"$letter
        if [ ! -f $DES ] ; then break 2 ; fi
    done
    echo "Error creating temporary file." ; exit 1
done

touch $DES 2> /dev/null
if [ ! -w $DES ]
    then echo "(Permission to create temporary file $DES denied)"; exit 1
fi

# misc
AWK=/bin/awk
# alias to send mail to
PTS=pts
login_name=$USER
mailbug_type="external"

# Function section

# Save personal data in users home directory
save_personal() {
    until
        read answer?"Save personal data in $HOME/.mailbug? (y/n) "
        case "$answer" in
            [yY]* )
                touch $HOME/.mailbug 2> /dev/null
                if [ -w $HOME/.mailbug ]
                then
                    echo "Full name: "$full_name >> $HOME/.mailbug
                    echo "Phone number: "$phone_number >> $HOME/.mailbug
                    echo "Return email path: "$mail_path >> $HOME/.mailbug
                else
                    echo "(Permissions to update $HOME/.mailbug denied)"
                fi
                true ;;
            [nN]* ) true ;;
            * ) echo $NOT_VALID ; false ;;
        esac
    do
        :
    done
}

# get_desc - read a description from the user, interpreting `~' escapes.
get_desc() {
    echo "Please enter your description of the problem."
    echo "   At the beginning of any line:"
    echo "      Type ~v to invoke ${EDITOR-vi}."
    echo "      Type ~r and the name of a file to read its contents."
    echo "      Type CTRL-D when description is complete"
    if [ "$localconfig" = "no" ]
    then
        echo "(please include appropriate configuration data for the affected system)"
    fi
    echo " "

    cat >> $1 </dev/null
    while read desc
    do
        case "$desc" in
	    ~v )  trap ':' 1 2 3 15
		  ${EDITOR-vi} $1
		  trap 'rm -f $DES ; exit 1' 1 2 3 15
	          echo "(continue) Type CTRL-D when description is complete"
	          continue ;;
	    ~r* ) filename=`echo $desc | sed 's/~r//'`
		  list=`eval ls -d $filename`
		  for name in $list
		  do
		      if [ -f $name ]
		      then
    		          if [ -r $name ]
        		     then echo $name" ..."
				  cat $name >> $1
        		     else echo "$name: Permission denied"
    		          fi
		      else
    		          echo "$name: Not found or is not a regular file"
		      fi
		  done
	          echo "(continue) Type CTRL-D when description is complete"
	          continue ;;
	    * )   echo "$desc" >> $1
	          continue ;;
        esac
    done
}

get_severity() {
until
    echo "Severity:
        1) Critical             (prevents total use of system)
        2) Serious              (prevents use of affected portion)
        3) Normal               (workarounds difficult or unknown)
        4) Low                  (workarounds known, but a nuisance)
        5) Enhancement          (a request for a new capability or feature)
        6) For Your Information (a suggestion or comment)"
	read it?"$PS3"
        case "$it" in
	    1|[Cc]* ) severity="Critical"    ; break ;;
	    2|[Ss]* ) severity="Serious"     ; break ;;
	    3|[Nn]* ) severity="Normal"      ; break ;;
	    4|[Ll]* ) severity="Low"         ; break ;;
	    5|[Ee]* ) severity="Enhancement" ; break ;;
	    6|[Ff]* ) severity="FYI"         ; break ;;
	    * )    false ;;
        esac
do
	echo $NOT_VALID
done
}

get_os_type() {
until
    echo "Operating System Type:
       1) DYNIX			(Dynix 3)
       2) PTX			(DYNIX/ptx)
       3) All			(All of the above)
       4) Other			(Other)"
	read it?"$PS3"
	case "$it" in
	    1|[Dd]* ) os_type=Dynix		; break ;;
	    2|[Pp]* ) os_type=DYNIX/ptx	; break ;;
	    3|[Aa]* ) os_type=All		; break ;;
	    4|[Oo]* ) read os_type?"Please describe: " ; break ;;
	    * ) 	false;;
	esac
do
	echo $NOT_VALID
done
}

get_system_type() {
until
    echo "System Type:
	1) Balance
	2) Symmetry
	3) All of the above"
	read it?"$PS3"
	case "$it" in
	    1|[Bb]* )
		until
	    	echo "Model Type:
	1) B8
	2) B21
	3) All of the above
	4) Other"
		    read it?"$PS3"
		    case "$it" in
		        1|[Bb]8*  ) system_type="B8" ;		 break ;;
		        2|[Bb]21* ) system_type="B21" ;		 break ;;
		        3|[Aa]*   ) system_type="All Balance"; break ;;
		        4|[Oo]*   ) read system_type?"Please describe: "
				      break ;;
		        * ) false ;;
		    esac
		do
		    echo $NOT_VALID
	        done
		break ;;
	    2|[Ss]* )
		until
		echo "Model Type:
	1) S81
	2) S27
	3) S16
	4) S3
	5) S2000/40
	6) S2000/200
	7) S2000/400
	8) S2000/700
	9) All of the above
	0) Other"
		read it?"$PS3"
		    case "$it" in
			1|[Ss]81* ) system_type="S81" ;		break ;;
			2|[Ss]27* ) system_type="S27" ;		break ;;
			3|[Ss]16* ) system_type="S16" ;		break ;;
			4|[Ss]3*  ) system_type="S3" ;		break ;;
			5|[Ss]2000/40 ) system_type="S2000/40";	break ;;
			6|[Ss]2000/2* ) system_type="S2000/200";	break ;;
			7|[Ss]2000/400 ) system_type="S2000/400";	break ;;
			8|[Ss]2000/7* ) system_type="S2000/700";	break ;;
			9|[Aa]*   ) system_type="All Symmetry"; break ;;
			0|[Oo]*   ) read system_type?"Please describe: "
				      break ;;
			* ) false ;;
		    esac
	        do
		    echo $NOT_VALID
		done
		break ;;
	    3|[Aa]* )
		system_type="All" ; break ;;
	    * ) false ;;
	esac
    do
	echo $NOT_VALID
    done
}

get_documentation() {
    read it?"Document Title [$doc_title]: "
    if [ "$it" != "" ] ; then doc_title=$it  ; fi
    read it?"Part Number [$doc_part]: "
    if [ "$it" != "" ] ; then doc_part=$it   ; fi
    read it?"Page Number [$doc_num]: "
    if [ "$it" != "" ] ; then doc_num=$it    ; fi
    read it?"Rev Number [$doc_rev]: "
    if [ "$it" != "" ] ; then doc_rev=$it    ; fi
}

get_hardware() {
    read it?"Type of hardware [$hdw_type]: "
    if [ "$it" != "" ] ; then hdw_type=$it   ; fi
    read it?"Part Number [$hdw_part]: "
    if [ "$it" != "" ] ; then hdw_part=$it   ; fi
    read it?"Revision [$hdw_rev]: "
    if [ "$it" != "" ] ; then hdw_rev=$it    ; fi
    read it?"Serial Number [$hdw_sn]: "
    if [ "$it" != "" ] ; then hdw_sn=$it     ; fi
}

get_mechanical() {
    read it?"Type [$mech_type]: "
    if [ "$it" != "" ] ; then mech_type=$it  ; fi
    read it?"Part Number [$mech_part]: "
    if [ "$it" != "" ] ; then mech_part=$it  ; fi
    read it?"Revision [$mech_rev]: "
    if [ "$it" != "" ] ; then mech_rev=$it   ; fi
    read it?"Serial Number [$mech_sn]: "
    if [ "$it" != "" ] ; then mech_sn=$it    ; fi
}

get_software() {
    echo "Please enter the suspected software component"
    read it?"(press RETURN if unknown): "
    if [ "x$it" != "x" ];then
	    softwr=`type $it`
    fi
    if [ "$softwr" != "$it not found" ] ; then
            sw_comp=`echo $softwr | $AWK '{print $3}'`
	    if [ "$localconfig" = "yes" ]; then
		if [ -f "$sw_comp" ]
	  	    then
		        sw_comp="`id $sw_comp`" 
		    else
			sw_comp="unknown ($it)"
	        fi
	    fi
    else
            sw_comp="unknown ($it)"
    fi
}

get_diag_ver() {
    read it?"Diagnostic version [$dig_ver]: "
    if [ "$it" != "" ] ; then dig_ver=$it     ; fi
}

get_diag_S3() {
    until
    echo "Diagnostic Subsystem:
	1) Controller and Drive Test
	2) Processor and RAM Tests
	3) System Configuration
	4) DYNIX boot
	5) Other"
	read it?"$PS3"
	case "$it" in
	    1|[Cc]* ) dig_sub="Controller and Drive Test"
		   until
		   echo "Diagnostic Test:
	1) Video Controller
	2) Hard Disk Drive(s)
	3) Tape Drive
	4) Ethernet Controller(s)
	5) Serial Communication Controller(s)
	6) Internal Modem
	7) Mouse
	8) Utilities
	9) Other"
			read it?"$PS3"
			case "$it" in
			   1|[Vv]* ) dig_test="Video Controller" ; break ;;
			   2|[Hh]* ) dig_test="Hard Disk Drive(s)" ; break ;;
			   3|[Tt]* ) dig_test="Tape Drive" ; break ;;
			   4|[Ee]* ) dig_test="Ethernet Controller(s)" ; break ;;
			   5|[Ss]* ) dig_test="Serial Communication Controller(s)" ; break ;;
			   6|[Ii]* ) dig_test="Internal Modem" ; break ;;
			   7|[Mm]* ) dig_test="Mouse" ; break ;;
			   8|[Uu]* ) dig_test="Utilities" ; break ;;
			   9|[Oo]* ) read dig_test?"Please describe: "
				  break ;;
			   * )    false ;;
		       esac
		   do
			echo $NOT_VALID
		   done
		   break ;;
	    2|[Pp]* ) dig_sub="Processor and RAM Tests" ; break ;;
	    3|[Ss]* ) dig_sub="System Configuration" ; break ;;
	    4|[Dd]* ) dig_sub="DYNIX boot" ; break ;;
	    5|[Oo]* ) read dig_sub?"Please describe: "
		      break ;;
	    * )    false ;;
	esac
    do
	echo $NOT_VALID
    done
    get_diag_ver
}

get_diag_Sym() {
until
    echo "Product Type:
	1) Executive based
	2) Supervisor based
	3) Power up tests
	4) Other"
	read it?"$PS3"
        case "$it" in
            1|[Ee]* ) dig_prod="Executive based" ; break ;;
            2|[Ss]* ) dig_prod="Supervisor based" ; break ;;
	    3|[Pp]* ) dig_prod="Power up tests"
		   if [ "$localconfig" = "no" ]
		   then
			until
        		echo "Console Processor Type:
	1) SCED
	2) SSM
	3) Other"
			read it?"$PS3"
            		case "$it" in
                		1|[Ss][Cc]* ) dig_type="SCED" ; break ;;
                		1|[Ss][Ss]* ) dig_type="SCED" ; break ;;
				3|[Oo]* ) read dig_type?"Please describe: "
				  break ;;
                		* )     false ;;
            		    esac
			do
				echo $NOT_VALID
        		done
		   fi
		   break ;;
            4|[Oo]*  ) read dig_prod?"Please describe: " ; break ;;
            * )    false ;;
        esac
    do
	echo $NOT_VALID
    done
    if [ "$localconfig" = "no" ]
        then
	until
        echo "System Processor Type:
	1) Model A
	2) Model B
	3) Model C
	4) Model D
	5) Mixed
	6) Other"
	    read it?"$PS3"
            case "$it" in
                1|*[aA] ) dig_proc="Model A" ; break ;;
                2|*[bB] ) dig_proc="Model B" ; break ;;
                3|*[cC] ) dig_proc="Model C" ; break ;;
                5|*[xX]* ) dig_proc="Mixed" ; break ;;
                4|*[dD] ) dig_proc="Model D" ; break ;;
                6|[Oo]* ) read dig_proc?"Please describe: "
			    break ;;
                * )         false ;;
            esac
	do
		echo $NOT_VALID
        done
	get_diag_ver
        read it?"Firmware version [$dig_firm]: "
        if [ "$it" != "" ] ; then dig_firm=$it    ; fi
    fi
}

get_diagnostics() {
    case "$system_type" in
	S3 ) get_diag_S3   ;;
	S* ) get_diag_Sym  ;;
	*)   get_diag_ver ;;
    esac
}

get_category() {
until
    echo "Category:
	1) Documentation
	2) Hardware
	3) Mechanical
	4) Software
	5) Diagnostics
	6) Other (or unidentified)"
	read it?"$PS3"
        case "$it" in
    	    1|[Dd]o* ) category="Documentation" ; get_documentation ; break ;;
	    2|[Hh]* ) category="Hardware" ; get_hardware      ; break ;;
	    3|[Mm]* ) category="Mechanical" ; get_mechanical    ; break ;;
	    4|[Ss]* ) category="Software" ; get_software      ; break ;;
	    5|[Dd]i* ) category="Diagnostics" ; get_diagnostics   ; break ;;
	    6|[Oo]* ) read category?"Please describe: " ; break ;;
	    * )    false ;;
        esac
    do
	echo $NOT_VALID
    done
}

create_mailbug() {
    echo "Mailbug Type: "$mailbug_type
    echo "Company: "$site_name
    echo "System Serial Number: "$system_sn
    if [ "$localconfig" = "yes" ]
    then
    $AWK '
        /^address:/             { print "Address: "substr($0,10) }
        /^city:/                { print "City: "substr($0,7) }
        /^state_province_code:/ { print "State/Province: "substr($0,22) }
        /^postal_code:/         { print "Postal Code: "substr($0,14) }
        /^country:/             { print "Country: "substr($0,10) }
        /^phone_number:/        { print "Customer Phone Number: "substr($0,15) }
	/^site_type:/		{ print "Customer Type: "substr($0,12) }
        ' $SITE 2> /dev/null
    fi
    echo "Login Name: "$login_name
    echo "Full name: "$full_name
    echo "Phone number: "$phone_number
    echo "Return email path: "$mail_path
    echo "Summary: "$summary
    echo "Severity: "$severity
    echo "System Type: "$system_type
    echo "OS type: "$os_type
    echo "OS version: "$os_vers
    echo "Category: "$category
    case "$category" in
        Documentation )
            echo "Document Title: "$doc_title
            echo "Part Number: "$doc_part
            echo "Page Number: "$doc_num
            echo "Rev Number: "$doc_rev
            ;;
        Hardware )
            echo "Type: "$hdw_type
            echo "Part Number: "$hdw_part
            echo "Revision: "$hdw_rev
            echo "Serial Number: "$hdw_sn
            ;;
        Mechanical )
            echo "Type: "$mech_type
            echo "Part Number: "$mech_part
            echo "Revision: "$mech_rev
            echo "Serial Number: "$mech_sn
            ;;
        Software )
            echo "Id: "$sw_comp
            ;;
        Diagnostics )
	    case "$system_type" in
		S3 ) echo "Diag Subsystem: "$dig_sub
		     if [ "$dig_sub" = "Controller and Drive Test" ]
		         then echo "Diag Test: "$dig_test
		     fi
		     echo "Diag Version: "$dig_ver
		     ;;
	        S* ) echo "Diag Product: "$dig_prod
                     if [ "$localconfig" = "no" ]
                     then
                         echo "System Proc Type: "$dig_proc
                         echo "Diag Version: "$dig_ver
                         echo "Firmware Version: "$dig_firm
			 if [ "$dig_prod" = "Power up tests" ]
			     then echo "Console Proc type: "$dig_type
			 fi
                     fi ;;
		* )  ;;
	    esac
            ;;
        * ) ;;
    esac
    echo "Description:"
    sed 's/^/ /' < $DES
    echo "[End Description]"
    echo "Version:"
    if [ "$localconfig" = "yes" ]
	then sed 's/^/ /'  < /etc/versionlog
    fi
    echo "[End Version]"
    echo "Configuration:"
    if [ "$localconfig" = "yes" ]
	then /etc/showcfg
    fi
    echo "[End Configuration]"
}

get_login() {
# if the user is logged in as root then we want to get his real login name
# and home directory and make sure they are valid.
    while [ true ]
    do
# use $login_name as the default
        read it?"Your real login name[$login_name]: "
        if [ "$it" = "" ] ; then it=$login_name ; fi
# check to see if this is a valid user account
        grep -s "^$it:" /etc/passwd
        if [ $? != 0 ]
            then echo "$it is not a valid login name for this system."
        else
	    login_name=$it
# get the users real home directory from /etc/passwd
	    real_home=`$AWK -F: ' /^'$login_name':/ { print $6 } ' /etc/passwd`
            if [ -d "$real_home" ]
	        then HOME=$real_home
	    fi
            break
        fi
    done
}

get_personal() {
    eval `$AWK '
    /^Full name:/         { print "full_name='\''"    substr($0,12) "'\''" }
    /^Phone number:/      { print "phone_number='\''" substr($0,15) "'\''" }
    /^Return email path:/ { print "mail_path='\''"    substr($0,20) "'\''" }
    ' $HOME/.mailbug 2> /dev/null`

    change_personal="no"
    if [ "$full_name" = "" ]
    then
# get default data from /etc/passwd
	full_name=`$AWK -F: ' /^'$login_name':/ { print $5 } ' /etc/passwd |
	    $AWK -F, '{ print $1 }'`
        read it?"Your full name[$full_name]: "
	if [ "$it" != "" ] ; then full_name=$it ; fi
	change_personal="yes"
    fi

    if [ "$phone_number" = "" ]
    then
	eval `$AWK '
# get default data from $SITE
	/^phone_number:/ { print "phone_number='\''" substr($0,15) "'\''" }
	' $SITE 2> /dev/null`
        read it?"Your phone number[$phone_number]: "
	if [ "$it" != "" ] ; then phone_number=$it ; fi
	change_personal="yes"
    fi

    if [ "$mail_path" = "" ]
    then
# get default data from hostname and login
	mail_path="$login_name@`ucb hostname`"
        read it?"Your return email path[$mail_path]: "
	if [ "$it" != "" ] ; then mail_path=$it ; fi
	change_personal="yes"
    fi

    if [ "$change_personal" = "yes" ]
        then save_personal
    fi
}

get_summary() {
    read summary?"One line summary of bug: "
}

get_os_vers() {
    read it?"Operating System Version[$os_vers]: "
    if [ "$it" != "" ] ; then os_vers=$it ; fi
}

get_site() {
# Retrieve necessary data from the $SITE file
    eval `$AWK '
        /^site_name:/   { print "site_name='\''"   substr($0,12) "'\''" }
        /^system_type:/ { print "system_type='\''" substr($0,14) "'\''" }
        ' $SITE 2> /dev/null`

# If site_name is blank look for site_name in $COMPANY_NAME.
# If you cant find site_name prompt the user for it.
    if [ "$site_name" = "" ]
    then
        if [ -f $COMPANY_NAME ]
        then
            site_name=`$AWK ' /^Company:/ { print substr($0,10) }
		' $COMPANY_NAME 2> /dev/null`
        else
            read site_name?"Your Company/Organization name: "
        fi
    fi
}

get_serial() {
    if [ "$localconfig" = "no" ]
    then
        read system_sn?"System serial number: "
    else
        system_sn=`$AWK ' /^serial_number:/ { print substr($0,16) }
	    ' $SERIAL_NUM 2> /dev/null`
    fi
}

update_site() {
    read it?"Company name [$site_name]: "
    if [ "$it" != "" ]
	then site_name=$it
    fi
}

update_serial() {
    read it?"System Serial number [$system_sn]: "
    if [ "$it" != "" ]
        then system_sn=$it
    fi
}

update_full_name() {
    read it?"Full name [$full_name]: "
    if [ "$it" != "" ]
        then full_name=$it ; save_personal
    fi
}

update_phone() {
    read it?"Phone number [$phone_number]: "
    if [ "$it" != "" ]
        then phone_number=$it ; save_personal
    fi
}

update_mail_path() {
    read it?"Return email path [$mail_path]: "
    if [ "$it" != "" ]
        then mail_path=$it ; save_personal
    fi
}


# start user prompts

echo "=== Problem Tracking System bug report ===
Follow the instructions:
    Press RETURN after entering each response.

    If the requested information is unknown or is not applicable, some fields
    can be skipped by simply pressing RETURN in response to the prompt.

    The default editor is vi.  To use a different editor, set your
    environment variable EDITOR to be the editor of your choice.

    Default values appear in brackets and can be used by simply pressing
    RETURN in response to the prompt.
"

# If the user is logged in as root get the users real login name.
if [ "$login_name" = "root" ]
    then get_login
fi

get_personal

get_site

until
    read it?"Does the problem occur on this system? (y/n) "
    case "$it" in
	[yY]* ) localconfig="yes" ; true ;;
	[nN]* ) localconfig="no"  ; true ;;
	*)      echo $NOT_VALID   ; false ;;
    esac
do
    :
done

get_serial

if [ "$localconfig" = "no" ]
then
    get_os_type
    get_os_vers
else
    os_type="Dynix"
    eval `/etc/version -v | $AWK '
    { print "os_vers='\''" $2 "'\''" }
    ' 2> /dev/null`
fi

if [ "$localconfig" = "no" ]
then
    get_system_type
fi

get_summary

get_severity

get_category

get_desc $DES

while [ true ]
do
until
    echo "Main menu:
	1) Display problem report to screen
	2) Edit description using ${EDITOR-vi}
	3) Change problem report header information
        4) Send problem report
	5) Exit and save problem report in $HOME
	6) Abort and don't save problem report"
	read it?"$PS3"
        case "$it" in
	    1|[Dd]* ) # Display problem report
	        create_mailbug | ${PAGER-more}
	        read it?"Hit return to continue "
	        break ;;
	    2|[Ee]* ) # Edit description
	        trap ':' 1 2 3 15
	        ${EDITOR-vi} $DES
	        trap 'rm -f $DES ; exit 1' 1 2 3 15
	        break ;;
            3|[Cc]* ) # Edit Header
		while [ true ]
		do
		    until
	            echo "Problem Report Header:
	1) Company name: $site_name
	2) Serial number: $system_sn
	3) Full name: $full_name
	4) Phone number: $phone_number
	5) Return email path: $mail_path
	6) Summary: $summary
	7) Severity: $severity
	8) System type: $system_type
	9) Operating system type: $os_type
	10) Version: $os_vers
	11) Category: $category
	12) Main menu"
			read it?"$PS3"
                        case "$it" in
		            1|[Cc]o* ) update_site ; break ;;
		            2|[Ss]er* ) update_serial  ; break ;;
        	            3|[Ff]* ) update_full_name ; break ;;
        	            4|[Pp]* ) update_phone ; break ;;
        	            5|[Rr]et* ) update_mail_path ; break ;;
        	            6|[Ss]u* ) get_summary ; break ;;
        	            7|[Ss]ev* ) get_severity ; break ;;
        	            8|[Ss]y* ) get_system_type ; break ;;
		            9|[Oo]* ) get_os_type ; break ;;
        	            10|[Vv]* ) get_os_vers ; break ;;
        	            11|[Cc]a* ) get_category ; break ;;
			    12|[Mm]* ) break 2;;
        	            * ) false ;;
	                esac
		    do
			echo $NOT_VALID
	   	    done
		done ; break ;;
	    4|[Ss]* ) # Send mailbug report to Sequent
	    	echo "Sending ..."
		create_mailbug | /usr/ucb/mail -s "mailbug: ${summary} $login_name" $login_name $PTS
		echo " Done. "
		# check to see if $MAILBUG file already exists.
		# If it does add a character to the name of the file and try
		# again.
		MAILBUG=$SERVICE"/mailbug/mailbug_"$login_name"_"$$
		while [ -f $MAILBUG ]
		do
    		    for letter in $ALPHA
    		    do
		        MAILBUG=$SERVICE"/mailbug/mailbug_"$login_name"_"$$"_"$letter
        	        if [ ! -f $MAILBUG ] ; then break 2 ; fi
    		    done
    		    echo "Error saving problem report" ; break 3
		done
		touch $MAILBUG 2> /dev/null
		if [ -w $MAILBUG ]
		then
		    create_mailbug >> $MAILBUG
		    echo "(Report saved in $MAILBUG)"
    		else
		    echo "(Permission to create $MAILBUG denied)"
		    echo "(Your System Admin needs to rerun /usr/lib/uucp/setup-pts.sh)"
		fi
	        break 2 ;;
	    5|[Ee]* ) # Exit and save mailbug in $HOME/mailbug$$
		# check to see if $HOMEBUG file already exists.
		# If it does add 1 to the name of the file and try again.
		HOMEBUG=$HOME"/mailbug_"$login_name"_"$$
 		while [ -f $HOMEBUG ]
                do
		    for letter in $ALPHA
		    do
		       HOMEBUG=$HOME"/mailbug_"$login_name"_"$$"_"$letter
		       if [ ! -f $HOMEBUG ] ; then break 2 ; fi
		    done
    		    echo "(Can not save problem report)" ; break 3
                done
                touch $HOMEBUG 2> /dev/null
                if [ -w $HOMEBUG ]
                then
		    create_mailbug >> $HOMEBUG
                    echo "(Report saved in $HOMEBUG)"
                else
                    echo "(Permission to create $HOMEBUG denied)"
                fi
		break 2 ;;
	    6|[Aa]* ) # Abort
	        break 2 ;;
	    * ) false ;;
        esac
    do
	echo $NOT_VALID
    done ; # end of select
done ; # end of while loop
rm -f $DES
exit 0
