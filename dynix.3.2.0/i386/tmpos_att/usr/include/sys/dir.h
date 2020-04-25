/* @(#)dir.h	6.1 */
#ifndef	DIRSIZ
#define	DIRSIZ	255
#endif
struct	direct
{
	ino_t	d_ino;
	char	d_name[DIRSIZ + 1];	/* for assured null termination */
};
