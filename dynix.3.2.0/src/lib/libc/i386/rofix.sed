# $Header: rofix.sed 1.1 86/02/17 $
##
# Sed script to make RCS strings readonly and shared
#
#  This technique works on strings of the form:
#     static char rcsid[] = "$......$"
#  and in paticular does not work for
#     static char *rcsid = "$......$"
##
/^rcsid:/ {
	i\
	.text
	: loop
	n
	s/^.long/&/
	t loop
	i\
	.data
}
