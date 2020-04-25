set path=(/etc /usr/etc /usr/ucb /bin /usr/bin .)
if ($?prompt) then
	set history=64
	set prompt="`hostname`(\!)# "
endif
