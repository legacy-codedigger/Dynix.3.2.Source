	PROGRAM	f_usclk
C	f_usclk:  Sample Fortran program using usclk.
C		No Fortran support for unsigned, hence the fuss.

$INCLUDE /usr/include/usclkf.h

	integer*4	t32, i
	real*8		t32x, seq, b8, b21

	b8 = 8000.0
	b21 = 21000.0
	call	usclk_init()

	t32 = getusclk()

	do 10 i = 1,1000
		seq = b8 + b21
10	continue

	t32 = getusclk() - t32

	if (t32 .LT. 0) then
		t32x = t32 + 4.294967296e09
	else
		t32x = t32
	endif

	print 110, t32x
110	format ('Fortran - Delta t32 =', F16.6, ' microseconds.')
	end
