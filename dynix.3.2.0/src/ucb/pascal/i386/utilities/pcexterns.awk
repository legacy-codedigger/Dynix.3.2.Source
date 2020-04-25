# $Copyright:	$
# Copyright (c) 1984, 1985, 1986 Sequent Computer Systems, Inc.
# All rights reserved
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: pcexterns.awk 1.1 89/03/12 $
#
# This generates .stabs for all the global routines and variables
# in a library. The format of a stab can be found in man5/stab.5.
#
# This value must be coordinated with the one in ../src/pstab.h.
#
BEGIN {
	N_FLAGCHECKSUM = 1;
}
#
# Generate "source file" stab for the library name.
#
NR == 1	{
	name = substr($1, 1, index($1, ":") - 1);
	printf "	.stabs	\"%s\",0x30,0,0x1,%d\n", name, N_FLAGCHECKSUM;
}
#
# Generate "library routine" stab.
#
NF == 3 && $2 == "T" {
	printf "	.stabs	\"%s\",0x30,0,0xc,0x%d\n", substr($3, 2), NR;
}
#
# Generate "library variable" stab.
#
NF == 3 && $2 ~ /[ABD]/ {
	printf "	.stabs	\"%s\",0x30,0,0xb,0x%d\n", substr($3, 2), NR;
}
