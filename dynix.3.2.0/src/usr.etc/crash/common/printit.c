/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef lint
static char rcsid[] = "$Header: printit.c 1.7 1991/08/02 15:12:36 $";
#endif

#include "crash.h"
#include <a.out.h>
#include <stab.h>
#include <strings.h>
#include "stabstring.h"
#include <assert.h>
#include <setjmp.h>
#include <sys/signal.h>
#include <pcc.h>

/*
 * Print out the declaration of a C variable.
 */

#define STACKSIZE 20000
#define BITSPERBYTE 8

typedef char Stack;
typedef int *Address;

#define push(type, value) { \
	((type *) (sp += sizeof(type)))[-1] = (value); \
}

#define pop(type) ((*((type *) (sp -= sizeof(type)))))

#define popn(n, dest) { \
	sp -= n; \
	bcopy(sp, (char *)dest, (unsigned int)n); \
}

#define alignstack() { \
	sp = (Stack *) (( ((int) sp) + sizeof(int) - 1)&~(sizeof(int) - 1)); \
}
#define ord(enumcon)    ((int) enumcon)
#define isdouble(range) ( \
	range->symvalue.rangev.upper == 0 && range->symvalue.rangev.lower > 0 \
)


static Stack pr_stack[STACKSIZE];
Stack *sp = &pr_stack[0];
int useInstLoc = 0;

#define chksp() \
{ \
	if (sp < &pr_stack[0]) { \
		sp = &pr_stack[0]; \
		panic("internal error: stack underflow"); \
	} \
}

#define poparg(n, r, fr) { \
	eval(p->value.arg[n]); \
	if (isreal(p->op)) { \
		if (size(p->value.arg[n]->nodetype) == sizeof(float)) { \
			fr = pop(float); \
		} else { \
			fr = pop(double); \
		} \
	} else if (isint(p->op)) { \
		r = popsmall(p->value.arg[n]->nodetype); \
	} \
}

struct symbol *rtype();
#define ident(n) ((n == NULL) ? "(noname)" : n)
#define symname(s) ident((s)->name)

int indent = 0;
#define	INDENT_BY 4

char *
c_classname(s)
	struct symbol *s;
{
	char * str;

	switch (s->class) {
	case RECORD:
		str = "struct";
		break;

	case VARNT:
		str = "union";
		break;

	case SCAL:
		str = "enum";
		break;

	default:
		str = "";
	}
	return str;
}


c_printdecl(s)
	struct symbol *s;
{
	printdecl(s, 0);
}

printdecl(s, indent)
	struct symbol *s;
	int indent;
{
	struct symbol *t;
	char semicolon, newline;

	semicolon = 1;
	newline = 1;
	if (indent > 0) {
		printf("%*c", indent, ' ');
	}
	if (s->class == TYPE) {
		printf("typedef ");
	}
	switch (s->class) {
	case CONST:
		if (s->type->class == SCAL) {
			printf("enumeration constant with value ");
			c_printval(s);
		} 
		else {
			printf("const %s = ", symname(s));
			c_printval(s);
		}
		break;

	case TYPE:
	case VAR:
		if (s->type->class == ARRAY) {
			printtype(s->type, s->type->type, indent);
			t = rtype(s->type->chain);
			assert(t->class == RANGE);
			printf(" %s[%d]", symname(s), t->symvalue.rangev.upper + 1);
		} 
		else {
			printtype(s, s->type, indent);
			if (s->type->class != PTR) {
				printf(" ");
			}
			printf("%s", symname(s));
		}
		break;

	case FIELD:
		if (s->type->class == ARRAY) {
			printtype(s->type, s->type->type, indent);
			t = rtype(s->type->chain);
			assert(t->class == RANGE);
			printf(" %s[%d]", symname(s), t->symvalue.rangev.upper + 1);
		} 
		else {
			printtype(s, s->type, indent);
			if (s->type->class != PTR) {
				printf(" ");
			}
			printf("%s", symname(s));
		}
		if (isbitfield(s)) {
			printf(" : %d", s->symvalue.field.length);
		}
		break;

	case TAG:
		if (s->type == NULL) {
			findtype(s);
			if (s->type == NULL) {
				panic("unexpected missing type information");
			}
		}
		printtype(s, s->type, indent);
		break;

	case RANGE:
	case ARRAY:
	case RECORD:
	case VARNT:
	case PTR:
	case FFUNC:
		semicolon = 0;
		printtype(s, s, indent);
		break;

	case SCAL:
		printf("(enumeration constant, value %d)", s->symvalue.iconval);
		break;


	default:
		printf("[%d]");
		break;
	}
	if (semicolon) {
		printf(" = ");
	}
#ifdef notdef
	if (newline) {
		putchar('\n');
	}
#endif
}

/*
 * Recursive whiz-bang procedure to print the type portion
 * of a declaration.
 *
 * The symbol associated with the type is passed to allow
 * searching for type names without getting "type blah = blah".
 */

printtype(s, t, indent)
	struct symbol *s;
	struct symbol *t;
	int indent;
{
	struct symbol *i;
	long r0, r1;
	char *p;

	switch (t->class) {
	case VAR:
	case CONST:
	case PROC:
		panic("printtype: class %s", c_classname(t));
		break;

	case ARRAY:
		printf("array[");
		i = t->chain;
		if (i != NULL) {
			for (;;) {
				printtype(i, i, indent);
				i = i->chain;
				if (i == NULL) {
					break;
				}
				printf(", ");
			}
		}
		printf("] of ");
		printtype(t, t->type, indent);
		break;

	case RECORD:
	case VARNT:
#ifdef notdef
		printf("%s ", c_classname(t));
		if (s->name != NULL && s->class == TAG) {
			p = symname(s);
			if (p[0] == '$' && p[1] == '$') {
				printf("%s ", &p[2]);
			} 
			else {
				printf("%s ", p);
			}
		}
		printf("{\n", t->class == RECORD ? "struct" : "union");
		for (i = t->chain; i != NULL; i = i->chain) {
			assert(i->class == FIELD);
			printdecl(i, indent+4);
		}
		if (indent > 0) {
			printf("%*c", indent, ' ');
		}
		printf("}");
#endif
		break;

	case RANGE:
		r0 = t->symvalue.rangev.lower;
		r1 = t->symvalue.rangev.upper;
		if (istypename(t->type, "char")) {
			if (r0 < 0x20 || r0 > 0x7e) {
				printf("%ld..", r0);
			} 
			else {
				printf("'%c'..", (char) r0);
			}
			if (r1 < 0x20 || r1 > 0x7e) {
				printf("\\%lo", r1);
			} 
			else {
				printf("'%c'", (char) r1);
			}
		} 
		else if (r0 > 0 && r1 == 0) {
			printf("%ld byte real", r0);
		} 
		else if (r0 >= 0) {
			printf("%lu..%lu", r0, r1);
		} 
		else {
			printf("%ld..%ld", r0, r1);
		}
		break;

	case PTR:
		printtype(t, t->type, indent);
		if (t->type->class != PTR) {
			printf(" ");
		}
		printf("*");
		break;

	case FUNC:
	case FFUNC:
		printtype(t, t->type, indent);
		printf("()");
		break;

	case TYPE:
		if (t->name != NULL) {
			printf("%s", symname(t));
		} 
		else {
			printtype(t, t->type, indent);
		}
		break;

	case TYPEREF:
		printf("@%s", symname(t));
		break;

	case SCAL:
		printf("enum ");
		if (s->name != NULL && s->class == TAG) {
			printf("%s ", symname(s));
		}
		printf("{ ");
		i = t->chain;
		if (i != NULL) {
			for (;;) {
				printf("%s", symname(i));
				i = i->chain;
				if (i == NULL) break;
				printf(", ");
			}
		}
		printf(" }");
		break;

	case TAG:
		if (t->type == NULL) {
			printf("unresolved tag %s", symname(t));
		} 
		else {
			i = rtype(t->type);
			printf("%s %s", c_classname(i), symname(t));
		}
		break;

	default:
		printf("(class %d)", t->class);
		break;
	}
}

istypename(type, name)
	struct symbol *type;
	char *name;
{
	struct symbol *t;
	int b;

	t = type;
	if (t == NULL) {
		b = 0;
	} 
	else {
		b = (t->class == TYPE && strcmp(ident(t->name), name) == 0);
	}
	return b;
}

/*
 * Reduce type to avoid worrying about type names.
 */

struct symbol *
rtype(type)
	struct symbol *type;
{
	struct symbol *t;

	t = type;
	if (t != NULL) {
		if (t->class == VAR || t->class == CONST ||
		    t->class == FIELD || t->class == REF) {
			t = t->type;
		}
		while (t->class == TYPE || t->class == TAG) {
			t = t->type;
			if (t->type == t)
				panic("internal error: rtype called for void type");
		}
	}
	return t;
}


/*
 * Print out the value on the top of the expression stack
 * in the format for the type of the given symbol.
 */

c_printval(s)
	struct symbol *s;
{
	struct symbol *t;
	Address a;
	int i, len;

	alignstack();

	switch (s->class) {
	case TYPE:
		if (is_void(s->type))
			printf("(void)");
		else
			c_printval(s->type);
		break;

	case CONST:
	case VAR:
	case REF:
	case FVAR:
	case TAG:
		c_printval(s->type);
		break;

	case FIELD:
		if (isbitfield(s)) {
			i = 0;
			popn(size(s), &i);
			i >>= (s->symvalue.field.offset % BITSPERBYTE);
			i &= ((1 << s->symvalue.field.length) - 1);
			t = rtype(s->type);
			if (t->class == SCAL) {
				printEnum(i, t);
			} 
			else {
				printRangeVal(i, t);
			}
		} 
		else {
			c_printval(s->type);
		}
		break;

	case ARRAY:
		t = rtype(s->type);
		if ((t->class == RANGE && istypename(t->type, "char")) ||
		    t == t_char->type) {
			len = size(s);
			sp -= len;
			printf("%.*s", len, sp);
		} 
		else {
			printarray(s);
		}
		break;

	case RECORD:
		c_printstruct(s);
		break;

	case VARNT:
		i = size(s);	/* size of whole union */
		printf("{ [union]");
		while (s = s->chain) {
			printf("\n");
			len = size(s);	/* size of this member */
			sp -= (i - len);
			indent += INDENT_BY;
			printdecl(s, indent);
			c_printval(s);
			if (s->type->class != RECORD && s->type->class != VARNT
			    && s->type->class != ARRAY && s->chain == NULL)
				printf("\n");
			indent -= INDENT_BY;
			sp += i;
		}
		printf("%*c}", indent, ' ');
		sp -= i;
		break;

	case RANGE:
		if (s == t_boolean->type || istypename(s->type, "boolean")) {
			printRangeVal(popsmall(s), s);
		} 
		else if (s == t_char->type || istypename(s->type, "char")) {
			printRangeVal(pop(char), s);
		} 
		else if (s == t_real->type || isdouble(s)) {
			switch (s->symvalue.rangev.lower) {
				case sizeof(float):
				prtreal(pop(float));
				break;

				case sizeof(double):
				prtreal(pop(double));
				break;

			default:
				panic("bad real size %d", t->symvalue.rangev.lower);
				break;
			}
		} 
		else {
			printRangeVal(popsmall(s), s);
		}
		break;

	case PTR:
		if (is_void(s->type)) {
			t = s;
		} else {
			t = rtype(s->type);
		}
		a = pop(Address);
		if (a == 0)
			printf("(nil)");
		else 
			printf("0x%x", a);
		break;

	case SCAL:
		i = pop(int);
		printEnum(i, s);
		break;

		/*
		 * Unresolved structure pointers?
		 */
	case BADUSE:
		a = pop(Address);
		printf("@%x", a);
		break;

	default:
		if (ord(s->class) > ord(TYPEREF)) {
			panic("printval: bad class %d", ord(s->class));
		}
		sp -= size(s);
		printf("[%s]", c_classname(s));
		break;
	}
}

/*
 * Print out a C structure.
 */

c_printstruct (s)
	struct symbol *s;
{
	struct symbol *f;
	Stack *savesp;
	int n, off, len;

	alignstack();

	sp -= size(s);
	savesp = sp;
	printf("{\n");
	indent += INDENT_BY;
	f = s->chain;
	for (;;) {
		off = f->symvalue.field.offset;
		len = f->symvalue.field.length;
		n = (off + len + BITSPERBYTE - 1) / BITSPERBYTE;
		sp += n;
		/* printf("%*c", indent, ' '); */
		printdecl(f, indent);
		/* printf("%s = ", symname(f)); */
		c_printval(f);
		printf("\n");
		sp = savesp;
		f = f->chain;
		if (f == NULL) break;
	}
	indent -= INDENT_BY;
	printf("%*c}", indent, ' ');
}

isbitfield(s)
struct symbol * s;
{
	int b;
	int off, len;
	struct symbol * t;

	off = s->symvalue.field.offset;
	len = s->symvalue.field.length;
	if ((off % BITSPERBYTE) != 0 || (len % BITSPERBYTE) != 0) {
		b = 1;
	} else {
		t = rtype(s->type);
		b = ((t->class == SCAL && len != (sizeof(int)*BITSPERBYTE)) ||
		    len != (size(t)*BITSPERBYTE));
	}
	return b;
}


is_void(type)
struct symbol *type;
{
	struct symbol * t;

	t = type;
	if (t != NULL) {
		if (t->class == VAR || t->class == CONST ||
		    t->class == FIELD || t->class == REF) {
			t = t->type;
		}
		while (t->class == TYPE || t->class == TAG) {
			t = t->type;
			if (t->type == t)
				return(1);
		}
	}
	return(0);
}

/*
 * Find the size in bytes of the given type.
 *
 * This is probably the WRONG thing to do.  The size should be kept
 * as an attribute in the symbol information as is done for structures
 * and fields.  I haven't gotten around to cleaning this up yet.
 */

#define MAXUCHAR 255
#define MAXUSHORT 65535L
#define MINCHAR -128
#define MAXCHAR 127
#define MINSHORT -32768
#define MAXSHORT 32767

findbounds (u, lower, upper)
	struct symbol * u;
	long *lower, *upper;
{
	Rangetype lbt, ubt;
	long lb, ub;

	if (u->class == RANGE) {
		lbt = u->symvalue.rangev.lowertype;
		ubt = u->symvalue.rangev.uppertype;
		lb = u->symvalue.rangev.lower;
		ub = u->symvalue.rangev.upper;
		*lower = lb;
		if (ubt == R_ASSUMED) {
			*upper = *lower;
		} else {
			*upper = ub;
		}
	} 
	else if (u->class == SCAL) {
		*lower = 0;
		*upper = u->symvalue.iconval - 1;
	} 
	else {
		panic("[internal error: unexpected array bound type]");
	}
}


size(sym)
	struct symbol * sym;
{
	struct symbol * t, *u;
	int nel, elsize;
	long lower, upper;
	int r, off, len;

	t = sym;
	if (t->type == t)		/* void type */
		return(sizeof(int));
	switch (t->class) {
	case STR:
		r = t->symvalue.offset;
		break;

	case RANGE:
		lower = t->symvalue.rangev.lower;
		upper = t->symvalue.rangev.upper;
		if (upper == 0 && lower > 0) {
			/* real */
			r = lower;
		} 
		else if (lower > upper) {
			/* unsigned long */
			r = sizeof(long);
		} 
		else if ((lower >= MINCHAR && upper <= MAXCHAR) ||
			(lower >= 0 && upper <= MAXUCHAR)) {
			r = sizeof(char);
		} 
		else if ((lower >= MINSHORT && upper <= MAXSHORT) ||
			(lower >= 0 && upper <= MAXUSHORT)) {
			r = sizeof(short);
		} 
		else {
			r = sizeof(long);
		}
		break;

	case ARRAY:
		elsize = size(t->type);
		nel = 1;
		for (t = t->chain; t != NULL; t = t->chain) {
			u = rtype(t);
			findbounds(u, &lower, &upper);
			nel *= (upper-lower+1);
		}
		r = nel*elsize;
		break;

	case DYNARRAY:
		r = (t->symvalue.ndims + 1) * sizeof(int);
		break;

	case SUBARRAY:
		r = (2 * t->symvalue.ndims + 1) * sizeof(int);
		break;

	case REF:
	case VAR:
		r = size(t->type);
		/*
			     *
	    if (r < sizeof(int) && isparam(t)) {
				r = sizeof(int);
			    }
			    */
		break;

	case FVAR:
	case CONST:
	case TAG:
		r = size(t->type);
		break;

	case TYPE:
		if (t->type->class == PTR && t->type->type->class == BADUSE) {
			findtype(t);
		}
		r = size(t->type);
		break;

	case FIELD:
		off = t->symvalue.field.offset;
		len = t->symvalue.field.length;
		r = (off + len + 7) / BITSPERBYTE - (off / BITSPERBYTE);
		break;

	case RECORD:
	case VARNT:
		r = t->symvalue.offset;
		if (r == 0 && t->chain != NULL) {
			panic("missing size information for record");
		}
		break;

	case PTR:
	case TYPEREF:
	case FILET:
		r = sizeof(int);
		break;

	case SCAL:
		r = sizeof(int);
		/*
			     *
	    if (t->symvalue.iconval > 255) {
				r = sizeof(short);
			    } else {
				r = sizeof(char);
			    }
			     *
	     */
		break;

	case FPROC:
	case FFUNC:
		r = sizeof(int);
		break;

	case PROC:
	case FUNC:
	case MODULE:
	case PROG:
		r = sizeof(struct symbol *);
		break;

	case SET:
		u = rtype(t->type);
		switch (u->class) {
		case RANGE:
			r = u->symvalue.rangev.upper + 1;
			break;

		case SCAL:
			r = u->symvalue.iconval;
			break;

		default:
			panic("expected range for set base type");
			break;
		}
		r = (r + BITSPERBYTE - 1) / BITSPERBYTE;
		if (r & 1)
			r++;
		break;

		/*
		 * These can happen in C (unfortunately) for unresolved type 
		 * references. Assume they are pointers.
		 */
	case BADUSE:
		r = sizeof(Address);
		break;

	default:
		if (ord(t->class) > ord(TYPEREF)) {
			panic("size: bad class (%d)", ord(t->class));
		} else {
			fprintf(stderr, "can't compute size of a %s\n", 
			    c_classname(t));
		}
		r = 0;
		break;
	}
	return r;
}


/*
 * Convert to old style STAG nlist n_desc.
 */
int
set_desc(sym)
	struct symbol * sym;
{
	struct symbol *t;
	int	desc;

	desc = 0;

	t = sym;
	for (; t; t = t->type) {
		switch (t->class) {
		case PROC:
		case FUNC:
		case MODULE:
		case PROG:
			desc = ((desc &~ PCCTM_BASETYPE)<<PCCTM_TYPESHIFT)|
					(PCCTM_FTN)|(desc&PCCTM_BASETYPE);
			break;
		case ARRAY:
			desc = ((desc &~ PCCTM_BASETYPE)<<PCCTM_TYPESHIFT)|
					(PCCTM_ARY)|(desc&PCCTM_BASETYPE);
			break;
		case PTR:
			desc = INCREF(desc);
			break;
		case TAG:
			desc |= PCCT_STRTY;
		case TYPE:
			return(desc);
		}
		if (t == t->type) {
			break;
		}
	}
	return (desc);
}

printarray(a)
struct symbol * a;
{
	int len, i, j, line_len;
	Stack *address;
	Stack *maddr;

	len = size(a);
	address = sp - len;
	line_len = len > 16 ? 16 : len;
	for (i = 0; i < len; address += 16, i += 16) {
		printf("\n+0x%04x", i);
		for (maddr = address, j = 0; j < line_len; maddr++, j++) {
			printf(" %02x", (unsigned char) *maddr);
		}

		printf(" ");

		for (maddr = address, j = 0; j < line_len; maddr++, j++) {
			if ((*maddr & 0x80) == 0 && isprint(*maddr))
				printf("%c", (unsigned char) *maddr);
			else
				printf(".");
		}
	}
	sp -= len;
}

/*
 * Print out the value of a real number in Pascal notation.
 * This is, unfortunately, different than what one gets
 * from "%g" in printf.
 */

jmp_buf	illegal_float;

catch_illegal_float()
{
	longjmp(illegal_float, 1);
}

prtreal(r)
double r;
{
	extern char *index();
	char buf[256];
	int catch_illegal_float();
	int (*old_fpe_handler)();
	int code;

	old_fpe_handler = signal(SIGFPE, catch_illegal_float);
	code = setjmp(illegal_float);

	if (code == 0) {
		sprintf(buf, "%g", r);
		if (buf[0] == '.') {
			printf("0%s", buf);
		} 
		else if (buf[0] == '-' && buf[1] == '.') {
			printf("-0%s", &buf[1]);
		} 
		else {
			printf("%s", buf);
		}
		if (index(buf, '.') == NULL) {
			printf(".0");
		}
	} 
	else {
		printf("Illegal floating point value");
	}

	signal(SIGFPE, old_fpe_handler);
}

/*
 * Print out a character using ^? notation for unprintables.
 */

printchar(c)
char c;
{
	if (c == 0) {
		putchar('\\');
		putchar('0');
	} 
	else if (c == '\n') {
		putchar('\\');
		putchar('n');
	} 
	else if (c > 0 && c < ' ') {
		putchar('^');
		putchar(c - 1 + 'A');
	} 
	else if (c >= ' ' && c <= '~') {
		putchar(c);
	} 
	else {
		printf("\\0%o",c);
	}
}

/*
 * Print out a value for a range type (int, char, or int).
 */

printRangeVal (val, t)
int val;
struct symbol * t;
{
	if (t == t_boolean->type || istypename(t->type, "boolean")) {
		if ((int) val) {
			printf("true");
		} 
		else {
			printf("false");
		}
	} 
	else if (t == t_char->type || istypename(t->type, "char")) {
		printf("0x%lx", val);
	} 
	else 
		printf("0x%lx", val);
}

/*
 * Print out an enumerated value by finding the corresponding
 * name in the enumeration list.
 */

printEnum (i, t)
int i;
struct symbol * t;
{
	struct symbol * e;

	e = t->chain;
	while (e != NULL && e->symvalue.constval != i) {
		e = e->chain;
	}
	if (e != NULL) {
		printf("%s", symname(e));
	} 
	else {
		printf("%d", i);
	}
}
/*
 * Pop an item of the given type which is assumed to be no larger
 * than a long and return it expanded into a long.
 */

popsmall(t)
struct symbol* t;
{
	int n;
	long r;

	n = size(t);
	if (n == sizeof(char)) {
		if (t->class == RANGE && t->symvalue.rangev.lower >= 0) {
			r = (long) pop(unsigned char);
		} 
		else {
			r = (long) pop(char);
		}
	} 
	else if (n == sizeof(short)) {
		if (t->class == RANGE && t->symvalue.rangev.lower >= 0) {
			r = (long) pop(unsigned short);
		} 
		else {
			r = (long) pop(short);
		}
	} 
	else if (n == sizeof(long)) {
		r = pop(long);
	} 
	else {
		panic("[internal error: size %d in popsmall]", n);
	}
	return r;
}

pr_symbol(addr, unit, s, RD)
	unsigned addr;
	int unit;
	struct symbol *s;
	int (*RD)();
{
	unsigned size;
	char buf[4096];		/* XXX */
	int n;

	if (s == NULL)
		return(0);

	switch (s->class) {
	case TAG:
		if (s->class == RECORD || s->class == TAG)
			size = s->type->symvalue.offset;
		else
			return(0);	/* Huh?? */
		break;

	case TYPE:
		if (s->type->class != TAG) {
			size = s->type->symvalue.offset;
			break;
		} 
		/* else, fall through... */

	default:
		return pr_symbol(addr, unit, s->type, RD);

	}

	printf("print 0x%x as %s (%d bytes) ", addr, s->name, size);
	if ((n = RD(addr, sp, size)) != size) {
		printf("  read error %d\n", n);
		perror("read");
		return(1);
	}
	sp += size;
	c_printval(s);
	printf("\n");
	return(1);
}
