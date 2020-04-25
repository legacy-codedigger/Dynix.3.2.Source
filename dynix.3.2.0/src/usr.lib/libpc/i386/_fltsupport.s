#
# The 8087 doesn't convert unsigned int to float.  We load it as unsigned
# and if negative bias it by adding the following constant, max unsigned
# int + 1.
#
	.text
	.globl	_.bias.ui2lf
_.bias.ui2lf:
	.double	0Dx41f0000000000000

#
# this is a scratch varible for floating point operations
#
	.data
	.globl	_.cvt
_.cvt:
	.double	0Dx0000000000000000

