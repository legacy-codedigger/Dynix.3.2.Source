/*
	setuid -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

extern int	_att_setuid();

int
setuid( uid )
	int	uid;
	{
	return (_att_setuid(uid));
	}
