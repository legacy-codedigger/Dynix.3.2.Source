program p_usclk(output);
{	p_usclk:  Sample Pascal program using usclk.
		No Pascal support for unsigned, hence the fuss.
}

var	t32 : longint;
var	i : integer;
var	seq, b8, b21, t32x : real;

{$I /usr/include/usclkp.h}

begin
	b8 := 8000.0;
	b21 := 21000.0;
	usclk_init;

	t32 := getusclk;

	for i := 1 to 1000 do
		seq := b8 + b21;

	t32 := getusclk - t32;

	if t32 < 0 then
		t32x := 4.29496729e09 + t32
	else
		t32x := t32;

	writeln('Pascal - Delta t32 = ', t32x, ' microseconds.');
end.
