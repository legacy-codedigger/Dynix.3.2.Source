/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Id: pass2.h,v 2.9 88/12/01 10:46:06 ksb Exp $ */
/*
 *	Berkeley Pascal Compiler	(pass2.h)
 */

#if !defined(_PASS2_)
#define	_PASS2_

#include "macdefs.h"
#include "mac2defs.h"
#include "manifest.h"
extern int cerror();

#define SETCON	0xa5a5		/* an assembler .set constant (rval) LC4*/
#define NORMCON	0x0000		/* a NORMAL constant $5			*/

/* match returns -- for generator to figure out */
/* NOTE  M_MRIGHT|M_MLEFT must be unique -- ksb */
#define M_DONE		0x0000	/* done evalution			*/
#define M_MLEFT		0x0001	/* match left first			*/
#define M_MRIGHT	0x0002	/* match right first			*/
/* M_LEFT|M_RIGHT	0x0003	** do either, we don't care		*/
#define M_FAIL		0x0004	/* no match generated			*/
#define M_SLEFT		0x0005	/* reshape left first			*/
#define M_SRIGHT	0x0006	/* reshape right first			*/
#define M_TLEFT		0x0007	/* match left first			*/
#define M_TRIGHT	0x0008	/* match right first			*/
typedef int MATCH;		/* rteturns one of the above		*/

#define RA_NONE		0x0000	/* no allocation yet			*/
#define RA_LFIRST	0x1000	/* left side was done first		*/	
#define RA_RFIRST	0x2000	/* right side was done first		*/	
#define RA_SUGGEST	0x0100	/* suggest a register for allocation	*/
#define RA_REJECT	0x0200	/* suggest we not take register X	*/
#define RA_USE(Mr)	(Mr)	/* register allocation encoded reg name */
#define RA_REG(Me)	((Me)&0xff) /* recover register from ra_use	*/

/* shapes -- like a partially typed storage class */
#define SH_NONE		0x0000	/* no shape or data (void)		*/
#define SH_FOREFF	0x0001	/* no storage, just effects		*/
#define SH_AREG		0x0002	/* an a perm A register			*/
#define SH_TAREG	0x0004	/* in a temp A register			*/
#define SH_BREG		0x0008	/* in a perm B register			*/
#define SH_TBREG	0x0010	/* in a temp B register			*/
#define SH_NAME		0x0020	/* name					*/
#define SH_FLD		0x0040	/* field				*/
#define SH_COND		0x0080	/* condition codes (dest of set)	*/
#define SH_QCON		0x0100	/* -8 <= constant < 8			*/
#define SH_BCON		0x0200	/* -128 <= constant =< 127u		*/
#define SH_WCON		0x0400	/* -32768 <= constant =< 32767u		*/
#define SH_LCON		0x0800	/* any other constant (could be float)	*/
#define SH_OAREG	0x1000	/* offset from an A register		*/
#define SH_OFP		0x2000	/* offset form frame pointer		*/
#define SH_ZZZ		0x4000	/* for an argument			*/
#define SH_EAX		0x8000	/* Geeze! eax has to have its own	*/

/* macros for classes for shapes */
#define SH_ANY_CON	(SH_QCON|SH_BCON|SH_WCON|SH_LCON)
#define SH_ANY_TEMP	(SH_TAREG|SH_TBREG)
#define SH_ANY_MEM	(SH_NAME|SH_OFP|SH_OAREG)
#define SH_ANY_AREG	(SH_AREG|SH_TAREG|SH_EAX|SH_COND)
#define SH_ANY_BREG	(SH_BREG|SH_TBREG)
#define SH_ANY_REG	(SH_ANY_AREG|SH_ANY_BREG)
#define SH_ANY_A	(SH_ANY_AREG|SH_ANY_MEM)
#define SH_ANY_B	(SH_ANY_BREG|SH_ANY_MEM)
#define SH_ANY_ADST	(SH_ANY_AREG|SH_ANY_MEM)
#define SH_ANY_BSRC	(SH_ANY_BREG|SH_ANY_MEM|SH_ANY_CON)
#define SH_ANY_BDST	(SH_ANY_BREG|SH_ANY_MEM)

#define SH_ANY		(SH_ANY_A|SH_ANY_CON)
#define SH_ORDER	(SH_ANY_AREG|SH_ANY_BREG|SH_ANY_MEM|SH_ANY_CON)

typedef unsigned int SHAPE;	/* the type for an int shape spec	*/


/* types */
#define TY_UNDEF	0x0000	/* undefine, nil, type		*/
#define TY_CHAR		0x0001	/* char				*/
#define TY_SHORT	0x0002	/* short			*/
#define TY_INT		0x0004	/* int				*/
#define TY_LONG		0x0008	/* long				*/
#define TY_DOUBLE	0x0010	/* double			*/
#define TY_UCHAR	0x0020	/* unsigned char		*/
#define TY_USHORT	0x0040	/* unsigned short		*/
#define TY_UNSIGNED	0x0080	/* unsigned int			*/
#define TY_ULONG	0x0100	/* unsigned long		*/
#define TY_POINT	0x0400	/* pointer to something		*/
#define TY_PTRTO	0x0800	/* pointer to one of the above	*/
#define TY_ANY		0x1000	/* matches anything reasonable	*/
#define TY_STRUCT	0x2000	/* structure or union		*/

#define TY_ANY_BYTE	(TY_CHAR|TY_UCHAR)
#define TY_ANY_WORD	(TY_SHORT|TY_USHORT)
#define TY_ANY_LONG	(TY_INT|TY_UNSIGNED|TY_LONG|TY_ULONG|TY_POINT)
#define TY_ANY_SMALL	(TY_ANY_BYTE|TY_ANY_WORD)

#define TY_ANY_SIGNED	(TY_POINT|TY_INT|TY_LONG|TY_SHORT|TY_CHAR)
#define TY_ANY_UNSIGNED	(TY_UNSIGNED|TY_ULONG|TY_USHORT|TY_UCHAR)
#define TY_ANY_FIXED	(TY_ANY_SIGNED|TY_ANY_UNSIGNED)

typedef unsigned int ITYPE;	/* the type of an int type spec	*/

/* reclamation cookies */
#define RC_NULL		0x0000	/* clobber result		*/
#define RC_LEFT		0x0001	/* reclaim left resource	*/
#define RC_RIGHT	0x0002	/* reclaim right resource	*/
#define RC_KEEP1	0x0004	/* reclaim resource allocated #1 */
#define RC_KEEP2	0x0008	/* reclaim resource allocated #2 */
#define RC_KEEP3	0x0010	/* reclaim resource allocated #3 */
#define RC_KEEPCC	0x0020	/* reclaim condition codes */
#define RC_POP		0x0040	/* take the lower of the two bregs */
#define RC_NOP		0x0080	/* DANGER: can cause loops.. */

typedef long LABEL;
extern LABEL getlab();

/* needs -- we can get up to 3 things from a need specification */
/* for instance NEED2(N_AREG(1)|N_SHAREL, N_TEMP(4))
 * requestes an A register shared with the left subtree and
 * 4 temparary bytes, these will be in res[0] and res[1]!
 * Easy.  (NEED0 is for people who don't need anything -- use it.)
 */
#define N_SHIFT			10		/* next resource	*/
#define N_CSHIFT		 6		/* count shift		*/
#define NEED0			NEED3(N_FREE,N_FREE,N_FREE)
#define NEED1(Mn1)		NEED3(Mn1,N_FREE,N_FREE)
#define NEED2(Mn1,Mn2)		NEED3(Mn1,Mn2,N_FREE)
#define NEED3(Mn1,Mn2,Mn3)	(((Mn3)<<(2*N_SHIFT))|((Mn2)<<N_SHIFT)|(Mn1))

#define N__A		0x01
#define N__B		0x02
#define N__R		0x03
#define N__S		0x04
#define N__T		0x05
#define N__L		0x06
#define N__F		0x07
#define N__C		0x08
#define N__MASK		0x0f
#define N_AREG		(N__A)			/* an A register	*/
#define N_BREG		(N__B)			/* a B register		*/
#define N_COND		(N__C)			/* a cond shape reg	*/
#define N_TEMP(Mx)	(N__T|((Mx)<<N_CSHIFT))	/* x temporary bytes	*/
#define N_REG(Mr)	(N__S|((Mr)<<N_CSHIFT))	/* register number r	*/
#define N_REWRITE	(N__R)		 	/* need rewrite		*/
#define N_LABEL		(N__L)			/* need a new label	*/
#define N_FREE		(N__F)			/* leave slot free	*/
#define N_SHAREL	 0x10			/* shared with left	*/
#define N_SHARER	 0x20			/* shared with right	*/
#define N_MASK		((1<<N_SHIFT)-1)	/* only this resource	*/

typedef long int NEED;		/* 30 bits al least */

/* register allocation */
typedef struct respref {
	int	cform;
	int	mform;
} RESPREF;

typedef struct SPnode {
	struct SPnode *pSPnext;	/* next spill of this register		*/
	int islevel;		/* spill level				*/
	int isbusy;		/* old busy bit				*/
	NODE *pNOsown;		/* old owner				*/
} SPILL;
extern SPILL *newSP();
#define nilSP	((SPILL *)0)

typedef struct RSnode {
	char acname[8]; 	/* rnames				*/
	int ibusy;		/* busy					*/
	int istatus;		/* rstatus				*/
	NODE *pNOown;		/* who owns us				*/
	SPILL *pSPfirst;	/* are we spilled			*/
} RSTATS;
extern RSTATS amregs[REGSZ];
extern RESPREF respref[];	/* resource preference rules */

#define ABUSY		04000	/* register busy for a with statement	*/
#define PBUSY		02000	/* register busy until read		*/
#define TBUSY		01000	/* register busy (during this expand)	*/
#define ISABUSY(Mr)	(0 != (amregs[Mr].ibusy & (ABUSY)))
#define ISPBUSY(Mr)	(0 != (amregs[Mr].ibusy & (PBUSY)))
#define ISTBUSY(Mr)	(0 != (amregs[Mr].ibusy & (TBUSY)))
#define ISBUSY(Mr)	(0 != (amregs[Mr].ibusy & (ABUSY|PBUSY|TBUSY)))
#define isbreg(Mr)	(0 != (amregs[Mr].istatus & (SH_TBREG|SH_BREG)))
#define istreg(Mr)	(0 != (amregs[Mr].istatus & (SH_TBREG|SH_TAREG)))
#define istnode(Mp)	(Mp->in.op==REG && istreg(Mp->tn.rval))

extern	NODE *deltrees[DELAYS];	/* trees held for delayed evaluation */
extern	int deli;		/* mmmmm */

extern	int stocook;
extern	NODE *stotree;
extern	int callflag;

extern	int fregs;

#include "ndu.h"

extern	NODE node[];

/* code tables */
typedef struct optab {
	int	op;			/* operator to match		*/
	int	nshape;			/* goal to match		*/
	int	lshape;			/* left shape to match		*/
	int	ltype;			/* left type to match		*/
	int	rshape;			/* right shape to match		*/
	int	rtype;			/* right type to match		*/
	int	needs;			/* resource required		*/
	int	rewrite;		/* how to rewrite afterwards	*/
	char	*cstring;		/* code generation template	*/
} OPCODE;

extern	OPCODE *table[];		/* map ops -> opcodes		*/
extern void opinit();

extern	NODE resc[];

extern	OFFSZ tmpoff;
extern	OFFSZ maxoff;
extern	OFFSZ baseoff;
extern	OFFSZ maxtemp;
extern	int maxtreg;
extern	int ftnno;
extern	int rtyflg;
extern	int nrecur;		/* flag to keep track of recursions */

extern	NODE
	*talloc(),
	*eread(),
	*tcopy(),
	*getlr();

extern	CONSZ rdin();
extern	int eprint();

extern	int lineno;
extern	char filename[];
extern	int fldshf, fldsz;
extern	int lflag, xdebug, udebug, edebug, odebug;
extern	int rdebug, radebug, tdebug, sdebug;
extern	int Oflag;

#if !defined(callchk)
#define callchk(x) allchk()
#endif

#if !defined(PUTCHAR)
#define PUTCHAR(x) putchar(x)
#endif

/* macros for doing double indexing */
#define R2PACK(x,y,z)	(0200*((x)+1)+y+040000*z)	/* pack 3 regs */
#define R2UPK1(x)	((((x)>>7)-1)&0177)		/* unpack reg 1 */
#define R2UPK2(x)	((x)&0177)			/* unpack reg 2 */
#define R2UPK3(x)	(x>>14)				/* unpack reg 3 */
#define R2TEST(x)	((x)>=0200)			/* test if packed */

#if defined(MULTILEVEL)
typedef union mltemplate {
	struct ml_head {
		int	tag;			/* tree class */
		int	subtag;			/* subclass of tree */
		union	mltemplate *nexthead;	/* linked by mlinit() */
	} mlhead;
	struct ml_node {
		int	op;			/* operator or op description */
		int	nshape;			/* node shape */
		/*
		 * Both op and nshape must match the node.
		 * where the work is to be done entirely by
		 * op, nshape can be SANY, visa versa, op can
		 * be OPANY.
		 */
		int	ntype;			/* type descriptor */
	} mlnode;
} MLCODE;
extern	union mltemplate mltree[];
#endif	/* multi-level stuff */

#define FORADDR	(SH_AREG|SH_BREG|SH_NAME|SH_CON|SH_OREG|SH_TAREG)
#define ANYCOOK	(SH_FOREFF|FORADDR|SH_AREG|SH_BREG|SH_ARG|SH_CC)

extern long freetemp();
extern NODE *tfree1();
extern int fsparea;		/* floating spill area (ksb)		*/
#if defined(KBUG)
extern FILE *kdebug;
#endif	/* debug output to private channel */

#endif	/* no multi includes of file file */
