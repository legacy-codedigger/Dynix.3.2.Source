/* @(#)types.h	6.1 */
typedef	struct { int r[1]; } *	physadr;
typedef	long		daddr_t;
typedef	char *		caddr_t;
typedef	unsigned int	uint;
typedef	unsigned short	ushort;
typedef	unsigned long	ino_t;
typedef short		cnt_t;
typedef	long		time_t;
typedef	int		label_t[9];
typedef	long		dev_t;
typedef	long		off_t;
typedef	long		paddr_t;
typedef	long		key_t;
typedef	int		size_t;
#ifdef	KERNEL
/*
 * For mutex data-structures.
 *
 * For ease of use for user-level code, we declare lock and sema struct's
 * here.  More detail is in h/mutex.h and machine/mutex.h.
 */

typedef	unsigned char	gate_t;

#ifdef	ns32000
typedef	struct	{
	gate_t		l_gate;
	char		l_state;
} lock_t;
#endif	ns32000

#ifdef	i386
typedef	unsigned char	lock_t;
#endif	i386


typedef	struct	{
	int		*s_head;
	int		*s_tail;
	short		s_count;
	unsigned char	s_gate;
	char		s_flags;
#ifdef SEMLOG
	short		s_pid;
	short		s_maxqlen;
#endif SEMLOG
} sema_t;
#endif KERNEL
