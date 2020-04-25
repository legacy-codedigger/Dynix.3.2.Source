/*	@(#)nan.h	1.3	*/
/* Handling of Not_a_Number's (only in IEEE floating-point standard) */

#define KILLFPE()	(void) kill(getpid(), 8)
#if u3b || u3b5
# define NaN(X)	(((union { double d; struct { unsigned :1, e:11; } s; } \
			*)&X)->s.e == 0x7ff)
# define KILLNaN(X)	if (NaN(X)) KILLFPE()
#else
# if ns32000 || i386
#  define NaN(X)	(((struct _dbl_ { unsigned fracl:32, frach:20, e:11, s:1; } \
			*)&(X))->e == 0x7ff || (((struct _dbl_ *)&(X))->e == 0 && \
			(((struct _dbl_ *)&(X))->fracl || ((struct _dbl_ *)&(X))->frach)))
#  define KILLNaN(X)	if (NaN(X)) KILLFPE()
# else
#  define NaN(X)	0
#  define KILLNaN(X)
# endif
#endif
