/*	@(#)expand.c	1.3 of 3/3/83	*/
#include <sys/types.h>
#include <sys/param.h>
#include <sys/acct.h>

time_t
expand(ct)
register comp_t ct;
{
	register e;
	register time_t f;

	e = (ct >> 13) & 07;
	f = ct & 017777;
	while (e-- > 0)
		f <<= 3;
	return f;
}
