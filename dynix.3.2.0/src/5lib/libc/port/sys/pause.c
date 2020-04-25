/*
	pause -- system call emulation for 4.2BSD

	last edit:	14-Dec-1983	D A Gwyn
*/

extern int	_sigblock(), _sigpause();

int
pause()
	{
	return _sigpause( _sigblock( 0 ) );	/* errno == EINTR */
	}
