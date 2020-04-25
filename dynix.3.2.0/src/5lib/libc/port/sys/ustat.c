/*
	ustat -- system call emulation for 4.2BSD or BRL PDP-11 UNIX

	last edit:	17-Apr-1984	D A Gwyn
*/

#include	<errno.h>
#include	<sys/types.h>
#include	<ustat.h>

extern char	*strcpy();
extern int	errno;
extern int	_ustat();

int
ustat( dev, buf )			/* returns 0 if ok, else -1 */
	int			dev;	/* device number */
	register struct ustat	*buf;	/* where to put information */
{
	if (_ustat(dev, buf) != -1) {
		(void)strcpy(buf->f_fname, "files");
		(void)strcpy(buf->f_fpack, "pack?");
		return(0);
	}
	return(-1);
}
