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

#ifdef RCS
static char rcsid[]= "$Header: zdcdbg.c 1.9 87/02/13 $";
#endif RCS

/*
 * zdcdbg - simple zdc debug assist program.
 *
 * zdcdbg will prompt for controller number then allows the
 * user to do a variety of debug commands with the ZDC.
 * Its main functions are to load the WCS of the ZDC and display/set
 * various registers and memory within the ZDC. All commands are interactive.
 * The commands supported are:
 *
 *	. ld filename	- load file name into ZDC WCS.
 *	. display reg [loc] 	- display ZDC reg/ram/stack value.
 *		where reg is one of:
 *			acc	- ALU accumulator.
 *			stat	- ALU status reg.
 *			ctr	- sequencer counter.
 *			pc	- sequence micro-PC.
 *			ram loc	- ALU internal ram location.
 *				  where 0 <= loc < ZRAMSIZE
 *			stk loc	- sequencer stack.
 *				  where 0 <= loc < ZSTKSIZE
 *	. set reg [loc] value	- set ZDC reg/ram/stack value.
 *	. wcr val	- set slic slave register (SL_Z_CNTRL).
 *	. rsr		- read slic status register.
 *	. go		- start ZDC microcode and wait for ZDC microcode pause.
 *	. gb		- start ZDC microcode and overlay "/boot".
 *	. boot		- overlay "/boot" on top of this program.
 *	. stop		- stop HSC.
 *	. quit		- simple exit.
 *	. wait		- wait for ZDC microcode pause.
 *	. reset		- reset the ZDC board.
 *	. cmp		- Compare WCS contents w/ specified file
 *	. si slic	- Set slic address for sr/sw commands.
 *	. sr slvaddr	- read slic slave address
 *	. sw slvaddr value	- slic write value at slave address slvaddr
 */

/* $Log:	zdcdbg.c,v $
 */

#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/param.h>
#include <machine/cfg.h>
#include <machine/slicreg.h>
#include <machine/slic.h>
#include <a.out.h>
#include "zdc_ucode.h"

#define	TOPWCSADDR	(MAX_UCODESIZE-1)
#define	ZRAMSIZE	32
#define	ZSTKSIZE	9
#define	UBUFSIZE	(UWORDSZ * MAX_UCODESIZE + 1)
#define	TRUE	1
#define	FALSE	0

char	*prompt();
caddr_t	calloc();

/*
 * commands
 */
char	*cmds[] = {
#define	LD	0
	"ld",
#define	GO	1
	"go",
#define	GB	2
	"gb",
#define	QUIT	3
	"quit",
#define	DISPLAY	4
	"display",
#define	SET	5
	"set",
#define	WCR	6
	"wcr",
#define	RSR	7
	"rsr",
#define	STOP	8
	"stop",
#define	WAIT	9
	"wait",
#define	RESET	10
	"reset",
#define	BOOT	11
	"boot",
#define	CMP	12
	"cmp",
#define	SI	13
	"si",
#define	SR	14
	"sr",
#define	SW	15
	"sw",
	0
};

/*
 * For display and set commands
 */
char	*regs[] = {
#define	ACC	0
	"acc",
#define	STAT	1
	"stat",
#define	CTR	2
	"ctr",
#define	PC	3
	"pc",
#define	RAM	4
	"ram",
#define	STK	5
	"stk",
	0
};
/* Basic ALU register dump structure. */
struct dump {
	u_short	val;		/* Register value. */
	u_short chg_flag;	/* True if the register's value changed. */
};

/* Register dump areas and read routines. */
struct dump	dumpram[ZRAMSIZE];	/* ALU internal RAM. */
struct dump	dumpacc;		/* ALU accumulator. */
struct dump	dumpstat;		/* ALU status register. */
struct dump	dumppc;			/* Sequencer micro-PC. */
struct dump 	dumpctr;		/* Sequencer counter. */
u_short		dumpstk[ZSTKSIZE];	/* Sequencer stack. */

u_short		readram(), readacc(), readstat();
u_short 	readpc(), readctr();

#define BOGSTK	(1 << 11)	/* Identifies bogus stack values. */

/* 
 * The microcode instructions for these routines are all in one microassembler
 * source file.  They're packed together like real microcode, but each
 * instruction is used independently.  The microcode transporter program allows
 * us refer to these microcode bytes as a ucode-format structure called
 * u_instructions.  (We don't need to bother with the "loaded" flag or "size"
 * field of struct ucode in this application.)  The uwords structure is a
 * template laid over the bottom part of the u_instructions to pick out the
 * required piece of microcode.
 */
struct uwords {
	struct uword	uw_readram;	/* MOVE RAM->Y, Y->KBUS */
	struct uword	uw_readacc;	/* MOVE ACC->Y, Y->KBUS */
	struct uword	uw_readstat;	/* SVSTNR NRY->Y, Y->KBUS */
	struct uword	uw_readctr;	/* JRP & fail: COMM_TO_HSC0 == 0 */
	struct uword	uw_readstk1;	/* LOOP & fail */
	struct uword	uw_readstk2;	/* CRTN & pass */
	struct uword	uw_restore_ram;	/* MOVE D->RAM, KBUS->YBUS */
	struct uword	uw_restore_acc;	/* MOVE D->ACC, KBUS->YBUS */
	struct uword	uw_restore_stat;/* MOVEW D->STATUS, KBUS->YBUS */
	struct uword	uw_restore_ctr;	/* LDCT, val->KBUS */
	struct uword	uw_restore_stk1;/* JMAP, addr->KBUS */
	struct uword	uw_restore_stk2;/* PUSH & fail: COMM_TO_HSC0 == 0 */
	struct uword	uw_restore_pc;	/* JMAP, addr->KBUS */
};

extern struct ucode u_instructions;
struct uwords	*uwords = (struct uwords *)u_instructions.u_word;

/* 
 * Defaults for ZDC SLIC slave control register.
 */
int	zgbcreg = SLB_RUN_RP | SLB_UNRESET;		/* GB */
int	zgocreg = SLB_RUN_SP | SLB_UNRESET;		/* GO */
bool_t	gotregs = FALSE;

u_char	ucodebuf[UBUFSIZE];
char	*input;			/* pointer to input string */

main() 
{
	register int i;
	int cc;
	int data;			/* sw command data */
	register struct config_desc *cd = CD_LOC;
	char bootprog[BNAMESIZE];
	u_char	zdcslic;		/* SLIC address of ZDC */
	u_char	srwslic;		/* initially SLIC address of ZDC */

	/*
	 * Find out which controller to use.
	 * Keep trying until the user gets a valid one.
	 */
	for (;;) {
		input = prompt("ZDC? ");
		i = atoi(input);
		if (i < 0 || i >= cd->c_toc[SLB_ZDCBOARD].ct_count) {
			printf("Invalid ZDC\n");
			continue;
		}
		zdcslic = 
		  cd->c_ctlrs[cd->c_toc[SLB_ZDCBOARD].ct_start+i].cd_slic;
		break;
	}

	srwslic = zdcslic;

	/*
	 * Main loop - exit via quit command.
	 */
	for (;;) {
		input = prompt("Cmd? ");

		switch (gettok(cmds)) {

		case CMP:
			cc = getfile();		/* read microcode file */
			if (cc < 0)
				break;
			/* 
			 * Reset the ZDC, and compare the file's contents
			 * with the WCS.
			 */
			wrslave(zdcslic, SL_Z_CNTRL, 0);
			wrslave(zdcslic, SL_Z_CNTRL, SLB_UNRESET);

			(void) cmp_wcs(zdcslic, ucodebuf, cc);
			break;

		case LD:
			cc = getfile();		/* read microcode file */
			if (cc < 0)
				break;
			/* 
			 * Reset the ZDC, and load the file's contents
			 * into the WCS.
			 */
			wrslave(zdcslic, SL_Z_CNTRL, 0);
			wrslave(zdcslic, SL_Z_CNTRL, SLB_UNRESET);

			if (!load_wcs(zdcslic, ucodebuf, cc))
				break;
			/*
			 * init upc
			 */
			dumppc.val = 0;
			dumppc.chg_flag = TRUE;
			break;

		case DISPLAY:			/* display saved ZDC values */
			if (gotregs == FALSE) {
				printf("Cannot display must stop HSC\n");
				break;
			}
			display();
			break;

		case SET:			/* Set ZDC reg values */
			if (gotregs == FALSE && 
			    rdslave(zdcslic, SL_Z_STATUS) & SLB_HSCRUNNING) {
				printf("Warning: HSC not stopped.\n");
			}
			set();
			break;

		case WCR:			/* Write slic reg SL_Z_CNTRL */
			i = getnum();
			if (i < 0)
				break;		/* invalid number */
			/*
			 * Always include the "unreset" bit.  This prevents
			 * accidental ZDC resets if the user forgets to
			 * include the bit.  Use the reset command to reset
			 * the board.
			 */
			i |= SLB_UNRESET;
			wrslave(zdcslic, SL_Z_CNTRL, i);
			/*
			 * Override defaults.
			 */
			zgocreg = i;
			zgbcreg = i;
			break;

		case RSR:			/* Read slic reg SL_Z_STATUS */
			printf("SLIC status = 0x%x\n",
				rdslave(zdcslic, SL_Z_STATUS));
			break;

		case GO:			/* Start ZDC */
			go(zdcslic, GO);
		case WAIT:			/* Wait for ZDC to Pause */
			/*
			 * wait for ZDC to Pause.
			 * Break out if character typed on terminal.
			 */
			i = -1;
			while ((rdslave(zdcslic, SL_Z_STATUS) & SLB_HSCRUNNING)
			       && ((i = igetchar()) == -1))
				continue;
			if (i != -1)
				break;		/* prompt cmd? */
			/* 
			 * Set stop mode, preserving the comm bits and
			 * avoiding reset. 
			 */
			wrslave(zdcslic, SL_Z_CNTRL, zgocreg & SLB_RUN_SP); 
			wrslave(zdcslic, SL_Z_CNTRL, zgocreg & ~SLB_MODEMASK);
			if (rdslave(zdcslic, SL_Z_STATUS) & SLB_HSCRUNNING) {
				printf("Cannot stop HSC\n");
				break;
			}
			dumpregs(zdcslic);
			break;

		case STOP:			/* STOP ZDC */
			/*
			 * STOP ZDC and gather registers.
			 *
			 * to stop:
			 *	wrslave SL_Z_CNTRL mode 10
			 *	wrslave SL_Z_CNTRL mode 00
			 */
			if ((rdslave(zdcslic, SL_Z_STATUS) & SLB_HSCRUNNING) !=
			    SLB_HSCRUNNING) {
				printf("HSC not running.\n");
				break;
			}
			wrslave(zdcslic, SL_Z_CNTRL, zgocreg & SLB_RUN_SP); 
			wrslave(zdcslic, SL_Z_CNTRL, zgocreg & ~SLB_MODEMASK);
			if (rdslave(zdcslic, SL_Z_STATUS) & SLB_HSCRUNNING) {
				printf("Cannot stop HSC\n");
				break;
			}
			dumpregs(zdcslic);
			break;

		case RESET:
			wrslave(zdcslic, SL_Z_CNTRL, 0); 
			break;
			
		case GB:			/* Start ZDC and overlay boot */
			go(zdcslic, GB);	/* Go Green Bay! */
		case BOOT:
			cd->c_boot_flag |= RB_ASKNAME;
			strcpy(bootprog, cd->c_boot_name);
			strcpy(index(bootprog, ')')+1, "boot");

			printf("loading %s\n", bootprog);
			i = open(bootprog, 0);
			if (i >= 0)
				copyboot(i);
			_stop("boot failed");

		case QUIT:			/* quit */
			exit(0);

		case SI:
			i = getnum();
			if (i < 0)
				break;		/* invalid number */
			srwslic = i;
			break;

		case SR:
			i = getnum();
			if (i < 0)
				break;		/* invalid number */
			printf("0x%x\n", rdslave(srwslic, i));
			break;

		case SW:
			i = getnum();
			if (i < 0)
				break;		/* invalid number */
			data = getnum();
			if (data < 0)
				break;		/* invalid number */
			wrslave(srwslic, i, data);
			printf("sw: address %d, data 0x%x\n", i, data);
			break;

		default:
			/* Ask again */
			printf("Unknown command\n");
			break;
		}
	}
}

/*
 * Start ZDC microcode execution.
 */
go(slic, gflag)
	u_char slic;		/* ZDC SLIC ID */
	int gflag;
{
	register int i;

	/* 
	 * Set stop mode and avoid board reset.  This allows the single-steps
	 * in the restore routines to work correctly.
	 */
	wrslave(slic, SL_Z_CNTRL, SLB_UNRESET);

	/* 
	 * Restore any changed registers.
	 * Sequencer stack and PC registers always have to be restored.
	 */

	for (i = 0; i < ZRAMSIZE; i++) {
		if (dumpram[i].chg_flag) {
			restore_ram(slic, i, dumpram[i].val);
			dumpram[i].chg_flag = FALSE;
		}
	}
	if (dumpacc.chg_flag) {
		restore_acc(slic, dumpacc.val);
		dumpacc.chg_flag = FALSE;
	}
	if (dumpstat.chg_flag) {
		restore_stat(slic, dumpstat.val);
		dumpstat.chg_flag = FALSE;
	}
	if (dumpctr.chg_flag) {
		restore_ctr(slic, dumpctr.val);
		dumpctr.chg_flag = FALSE;
	}
	restore_stk(slic, dumpstk);
	restore_pc(slic, dumppc.val, dumppc.chg_flag);

	/*
	 * Set ZDC's group mask so that interrupts will be accepted.
	 */
	(void)setGM(slic, SL_GM_ALLON);

	/*
	 * Set up the WREG for microcode interrupts. The current code
	 * here is only a temporary solution.
	 */
	WRITE_WREG(slic, 0);

	/*
	 * Start it.
	 */
	wrslave(slic, SL_Z_CNTRL, (gflag == GO) ? zgocreg : zgbcreg);
	wrslave(slic, SL_Z_STARTHSC, 0);
	gotregs = FALSE;
}

/*
 * display an HSC register
 */
display()
{
	register int i;

	switch (gettok(regs)) {
	case ACC:
		printf("acc = 0x%x\n", dumpacc.val);
		break;
	case STAT:
		printf("stat = 0x%x\n", dumpstat.val);
		break;
	case CTR:
		printf("ctr = 0x%x\n", dumpctr.val);
		break;
	case PC:
		printf("pc = 0x%x\n", dumppc.val);
		break;
	case RAM:
		i = getnum();
		if (i < 0 || i >= ZRAMSIZE) {
			printf("Invalid RAM address\n");
			break;
		}
		printf("RAM[%d] = 0x%x\n", i, dumpram[i].val);
		break;
	case STK:
		i = getnum();
		if (i < 0 || i >= ZSTKSIZE) {
			printf("Invalid Stack address\n");
			break;
		}
		printf("Stk[%d] = 0x%x\n", i, dumpstk[i]);
		break;
	default:
		printf("Unknown register\n");
		break;
	}
}

/*
 * Set a new value for an HSC register.
 */
set()
{
	register int i, newval;

	switch (gettok(regs)) {
	case ACC:
		newval = getnum();
		if (newval < 0) {
			printf("value missing\n");
			break;
		}
		printf("old acc = 0x%x, new acc = 0x%x\n", dumpacc.val, newval);
		dumpacc.val = newval;
		dumpacc.chg_flag = TRUE;
		break;
	case STAT:
		newval = getnum();
		if (newval < 0) {
			printf("value missing\n");
			break;
		}
		printf("old stat = 0x%x, new stat = 0x%x\n", dumpstat.val,
			newval);
		dumpstat.val = newval;
		dumpstat.chg_flag = TRUE;
		break;
	case CTR:
		newval = getnum();
		if (newval < 0) {
			printf("value missing\n");
			break;
		}
		printf("old ctr = 0x%x, new ctr = 0x%x\n", dumpctr.val, newval);
		dumpctr.val = newval;
		dumpctr.chg_flag = TRUE;
		break;
	case PC:
		newval = getnum();
		if (newval < 0) {
			printf("value missing\n");
			break;
		}
		printf("old pc = 0x%x, new pc = 0x%x\n", dumppc.val, newval);
		dumppc.val = newval;
		dumppc.chg_flag = TRUE;
		break;
	case RAM:
		i = getnum();
		if (i < 0 || i >= ZRAMSIZE) {
			printf("Invalid RAM address\n");
			break;
		}
		newval = getnum();
		if (newval < 0) {
			printf("value missing\n");
			break;
		}
		printf("old RAM[%d] = 0x%x, new RAM[%d] = 0x%x\n",
			i, dumpram[i].val, i, newval);
		dumpram[i].val = newval;
		dumpram[i].chg_flag = TRUE;
		break;
	case STK:
		i = getnum();
		if (i < 0 || i >= ZSTKSIZE) {
			printf("Invalid Stack address\n");
			break;
		}
		newval = getnum();
		if (newval < 0) {
			printf("value missing\n");
			break;
		}
		printf("old Stk[%d] = 0x%x, new Stk[%d] = 0x%x\n",
			i, dumpstk[i], i, newval);
		dumpstk[i] = newval;
		break;
	default:
		printf("Unknown register\n");
		break;
	}
}

/*
 * Read and print ZDC internal registers and RAM.
 * We've set stop mode and made sure the board isn't
 * reset before entry to this routine.
 */
dumpregs(slic)
	u_char	slic;		/* ZDC slic ID */
{
	register int i, j;

	dumppc.val = readpc(slic);
	for (i = 0; i < ZRAMSIZE; i++)
		dumpram[i].val = readram(slic, i);
	dumpacc.val = readacc(slic);
	dumpstat.val = readstat(slic);
	dumpctr.val = readctr(slic);
	readstk(slic, dumpstk);
	gotregs = TRUE;

	/*
	 * Print regs
	 */
	printf("pc = %x, ctr = %x, acc = %x, stat = %x\n", dumppc.val,
		dumpctr.val, dumpacc.val, dumpstat.val);

	printf("stk: ");
	for (i = 0; i < ZSTKSIZE; i++)
		printf("%x ", dumpstk[i]);
	printf("\n");

	printf("RAM:\n");
	/*
	 * dump out 8 per line
	 */
	for (i = 0; i < ZRAMSIZE; i += 8) {
		for (j = 0; j < 8; j++)
			printf("%x ", dumpram[i + j].val);
		printf("\n");
	}
}

/*
 * recognize a token.
 */
gettok(cp)
	register char *cp[];		/* string table */
{
	register int i;			/* string table index */
	register char *sp;		/* string ptr */

	sp = input;
	/*
	 * Skip any blank space
	 */
	for (; *sp && *sp == ' '; sp++)
		continue;
	/* 
	 * find match
	 */
	for (i = 0; cp[i] != 0; i++) {
		if (strncmp(cp[i], sp, strlen(cp[i])) == 0) {
			/*
			 * Skip to end of token.
			 */
			for (; *sp && *sp != ' '; sp++)
				continue;
			input = sp;
			return(i);
		}
	}
	return(-1);		/* unknown string */
}

/*
 * Read a file into the ucodebuf.
 * input string is passed via global "input" pointer.
 */
int
getfile()
{
	register char	*sp;
	register int cc;
	int fd;

	/*
	 * Scan past white space.
	 */
	sp = input;
	for (; *sp && *sp == ' '; sp++)
		continue;
	fd = open(sp, 0);

	if (fd < 0) {
		printf("Open of %s failed\n", sp);
		return(-1);
	}
	/*
	 * read entire file into buffer.
	 */
	cc = read(fd, ucodebuf, UBUFSIZE);
	close(fd);

	if ((cc % UWORDSZ) != 1) {
		printf("File bad size = %d\n", cc);
		return(-1);
	}
	return(cc);
}

/*
 * Get a number from the input string
 * input string is passed via global "input" pointer.
 */
getnum()
{
	register char	*sp;
	register int	retval;

	/*
	 * Scan past white space.
	 */
	sp = input;
	for (; *sp && *sp == ' '; sp++)
		continue;

	retval = gethex(sp);
	if (retval == 0 && *sp != '0') {
		printf("Illegal number\n");
		return(-1);
	}

	/*
	 * Skip to end of number.
	 */
	for (; *sp && *sp != ' '; sp++)
		continue;
	input = sp;
	return(retval);
}

/*
 * Read hex input
 */
gethex(p)
	register char *p;
{
	register n = 0;

	if (strncmp(p, "0x", 2) == 0)
		p += 2;
	while ( (*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ) {
		if (*p >= '0' && *p <= '9')
			n = n*16 + *p++ - '0';
		else
			n = n*16 + *p++ - 'a' + 10;
	}
	return (n);
}

/*
 * Copy bootstrap and overlay.
 * This should never return.
 */
copyboot(io)
	register io;
{
	register int n, offset;
	register struct exec *e;

	/*
	 * start reading file at 1K boundary past allocated memory.
	 */
	callocrnd(1024);
	offset = (int)calloc(0);
	e = (struct exec *)offset;

	/* read header and check magic number */
	n = roundup(sizeof(struct exec), DEV_BSIZE);
	if (read(io, offset, n) != n)
		goto sread;
	if (e->a_magic != SMAGIC) 
		_stop("Bad a.out magic number");

	/* read text */
	if (read(io, offset + n, e->a_text - n) != e->a_text - n)
		goto sread;

	/* read data */
	n = e->a_data;
	if (read(io, offset + e->a_text, n) != n)
		goto sread;

	/* copy configuration tables */
	bcopy(CD_LOC, offset + (int)CD_LOC, CD_STAND_ADDR - (int)CD_LOC);

	(void) close(io);
	/* exec new program over ourselves */
	gsp(offset, 0, e->a_text + e->a_data, e->a_entry);
sread: 
	(void) close(io);
	_stop("short read");
}

/*
 * Transfer the KBUS contents to the shadow register. Then read the
 * shadow register contents back over the SLIC bus. Notice that the first
 * byte read from the shadow register is garbage (actually the old shift
 * register contents).
 */
u_short
read_kreg(slic)
	u_char	slic;	/* SLIC ID number */
{
	u_short	data;
	wrslave(slic, SL_Z_KBUSTOSHAD, 0);	/* No data required. */
	data = (u_short)rdslave(slic, SL_Z_SCANSR);	/* Discard. */
	data = (u_short)rdslave(slic, SL_Z_SCANSR) & 0xff;	/* LSB */
	data |= ((u_short)rdslave(slic, SL_Z_SCANSR) & 0xff) << 8; /* MSB */
	return (data);
}

/*
 * Read ZDC ALU internal RAM. 
 * MOVE RAM->Y, Y->KBUS
 */
u_short
readram(slic, num)
	u_char	slic;	/* SLIC ID number. */
	int	num;	/* RAM register number. */
{
	struct uword	instr;
	
	instr = uwords->uw_readram;
	instr.uw_bytes[2] |= num & 0x1f;
	write_pl(slic, &instr);
	single_step(slic);
	return (read_kreg(slic));
}

/*
 * Read ZDC ALU accumulator register.
 * MOVE ACC->Y, Y->KBUS
 */
u_short
readacc(slic)
	u_char	slic;	/* SLIC ID number. */
{
	write_pl(slic, &uwords->uw_readacc);
	single_step(slic);
	return (read_kreg(slic));
}

/*
 * Read ZDC ALU status register.
 * SVSTNR NRY->Y, Y->KBUS
 */
u_short
readstat(slic)
	u_char	slic;	/* SLIC ID number. */
{
	write_pl(slic, &uwords->uw_readstat);
	single_step(slic);
	return (read_kreg(slic));
}

/*
 * Read ZDC sequencer counter.
 * JRP & fail
 */
u_short
readctr(slic)
	u_char	slic;	/* SLIC ID number. */
{
	wrslave(slic, SL_Z_CNTRL, SLB_UNRESET);	/* Force failing CC. */
	write_pl(slic, &uwords->uw_readctr);
	/* DON'T single step. */
	return(READ_UPC(slic));
}

/*
 * Read ZDC sequencer stack.
 *
 * The 2910A stack has nine locations.  When the stack is reset, the TOS is
 * at location 0, which contains garbage.  The first push operation puts a
 * value in location 0, which is still the TOS.  Subsequent push operations
 * increment the location counter and put values in the new location.  If you
 * try to push a 10th (and any more) value, the ninth location gets overwritten
 * but remains the TOS.  Other stack locations aren't affected in this case.
 * Pop operations return a value, then decrement the location counter.
 *
 * The readstk() routine uses these characteristics to figure out how many
 * valid entries are in the stack.  First, readstk() saves the current TOS
 * value in a temporary variable, using the LOOP instruction and a failing
 * condition code - this prevents the TOS value from being popped. Next, 
 * readstk() pushes BOGSTK 9 times onto the stack.  BOGSTK is a guaranteed
 * bogus value, because real stack values will never have bit 11 set. (The
 * ZDC only uses 2K words of microcode.)  Now readstk() pops (with the CRTN
 * instruction and a passing condition) the stack 9 times into the dump area.
 * Finally, readstk() examines the dump area to determine if the value saved
 * earlier needs to be restored.  If location 0 is BOGSTK, the stack was empty
 * and the saved value is garbage.  If only location 8 is BOGSTK and the 
 * saved value doesn't occur in location 7, the stack was full and the saved 
 * value needs to be restored to location 8.  Otherwise, the saved value is in 
 * an intermediate stack location and therefore already recorded.
 *
 * NOTICE! There's a minor bug in this algorithm: if a value gets pushed
 * on the stack twice in succession AND these were the last two pushes before
 * zdcdbg dumped the registers, readstk will misinterpret the situation and
 * assume an almost-full stack, rather than a full stack.  This should be at
 * most a rare occurrance, and the user should be able to determine this
 * situation from the register dump and tweak the values appropriately.
 */
readstk(slic, dump_area)
	u_char	slic;		/* SLIC ID number. */
	u_short	*dump_area;	/* Where the dump area starts. */
{
	register int	i; 
	u_short		temp; 
	struct uword	instr;
	
	instr = uwords->uw_restore_stk1;

	/* Read (not pop) TOS into temp. */
	wrslave(slic, SL_Z_CNTRL, SLB_UNRESET);	/* Force failing CC. */
	write_pl(slic, &uwords->uw_readstk1);	/*LOOP,fail*/
	/* DON'T single step. */
	temp = READ_UPC(slic);

	/* 
	 * Push 9 BOGSTK values onto the stack.  The push code is
	 * grabbed from restore_stk().  The relevant comments are
	 * there.
	 */
	for (i = 0; i < ZSTKSIZE; i++) {
		*(u_short *)instr.uw_bytes = BOGSTK - 1;
		write_pl(slic, &instr);
		single_step(slic);
		write_pl(slic, &uwords->uw_restore_stk2);
		single_step(slic);
	}

	/*
	 * Pop the stack into the dump area.  The loop goes backwards
	 * because the stack grows up. (Location 9 is the current TOS.)
	 */
	for (i = ZSTKSIZE - 1; i >= 0; i--) {
		write_pl(slic, &uwords->uw_readstk2);	/*CRTN,pass*/
		dump_area[i] = READ_UPC(slic);
		single_step(slic);
	}

	/*
	 * Restore location 8 if necessary.
	 * If location 7 isn't BOGSTK, then only location 8 is.
	 */
	if (dump_area[7] != BOGSTK && dump_area[7] != temp)
		dump_area[8] = temp;
}

/*
 * Restore ZDC ALU internal RAM.
 * MOVE D->RAM, KBUS->YBUS
 */
restore_ram(slic, num, data)
	u_char	slic;	/* SLIC ID number. */
	int	num;	/* RAM register number. */
	u_short	data;
{
	struct uword	instr;
	
	instr = uwords->uw_restore_ram;

	instr.uw_bytes[2] |= num & 0x1f;  /* Plug in RAM number. */
	*(u_short *)instr.uw_bytes = data;	/* Plug in the data. */
	write_pl(slic, &instr);
	single_step(slic);
}

/*
 * Restore ZDC ALU accumulator register.
 * MOVE D->ACC, KBUS->YBUS
 */
restore_acc(slic, data)
	u_char	slic;	/* SLIC ID number. */
	u_short	data;
{
	struct uword	instr;
	
	instr = uwords->uw_restore_acc;

	*(u_short *)instr.uw_bytes = data;	/* Plug in the data. */
	write_pl(slic, &instr);
	single_step(slic);
}

/*
 * Restore ZDC ALU status register.
 * MOVEW D->STATUS, KBUS->YBUS
 */
restore_stat(slic, data)
	u_char	slic;	/* SLIC ID number. */
	u_short	data;
{
	struct uword	instr;
	
	instr = uwords->uw_restore_stat;

	*(u_short *)instr.uw_bytes = data;	/* Plug in the data. */
	write_pl(slic, &instr);
	single_step(slic);
}

/*
 * Restore ZDC sequencer counter.
 * LDCT, val->KBUS
 */
restore_ctr(slic, data)
	u_char	slic;	/* SLIC ID number. */
	u_short	data;
{
	struct uword	instr;
	
	instr = uwords->uw_restore_ctr;

	*(u_short *)instr.uw_bytes = data;	/* Plug in the data. */
	write_pl(slic, &instr);
	single_step(slic);
}

/*
 * Restore ZDC sequencer stack.
 */
restore_stk(slic, dump_area)
	u_char	slic;		/* SLIC ID number. */
	u_short	*dump_area;	/* Where the dump area starts. */
{
	register int	i;
	struct uword	instr;
	
	instr = uwords->uw_restore_stk1;


	/* 
	 * Reload the stack registers. 
	 * Location 0 is the oldest value, so push it first.
	 */
	for (i = 0; i < ZSTKSIZE; i++) {
		/* 
		 * Bail out on the first bogus stack value.
		 */
		if (dump_area[i] == BOGSTK)
			break;
		/*
		 * Load uPC with one less than desired stack contents.
		 * Use JMAP, addr -> KBUS.  We must subtract one from the
		 * value, because the value + one gets pushed onto the stack.
		 */
		*(u_short *)instr.uw_bytes = dump_area[i] - 1;
		write_pl(slic, &instr); /* JMAP, addr->KBUS */
		single_step(slic);
		/* 
		 * Now push to the uPC to the stack. Use PUSH & fail.
		 * This step adds one to the value, counteracting the
		 * subtraction done in the previous step.  Notice that the
		 * single-step() function will automatically set COMM_TO_HSC0
		 * to zero, so the condition check for this bit will fail.
		 * This prevents the counter register from being overwritten.
		 */
		write_pl(slic, &uwords->uw_restore_stk2); /* PUSH & fail */
		single_step(slic);
	}
}

static u_short	pc_next; /* Not printed, but needed to resume execution. */

/*
 * Read ZDC sequencer micro-PC
 */
u_short
readpc(slic)
	u_char	slic;	/* SLIC ID number. */
{
	pc_next = READ_UPC(slic);
	return(READ_WREG(slic));
}

/*
 * Restore ZDC sequencer micro-PC
 */
restore_pc(slic, data, new)
	u_char	slic;	/* SLIC ID number. */
	u_short	data;
	u_short	new;	/* Use new PC */
{
	struct uword	instr;
	
	instr = uwords->uw_restore_pc;

	/* 
	 * Put the sequencer back to it's original state by JMAPping
	 * back to pc_next. The pc_next address goes into the KBUS field. 
	 */
	if (new)
		*(u_short *)instr.uw_bytes = data & TOPWCSADDR;
	else
		*(u_short *)instr.uw_bytes = pc_next;
	write_pl(slic, &instr);
	single_step(slic);
}

/*
 * Low-level library routines for the ZDC.
 */
#define FAILURE	0
#define PASSED	1

/*
 * Load the contents of buf into the ZDC's WCS.  The load always starts at
 * WCS address 0.
 *
 * Note: the checksum byte will be a residual byte following the last 
 * 8-byte microinstruction word.
 */
load_wcs(slic, bp, nbytes)
	u_char		slic;		/* SLIC address of target ZDC. */
	register u_char	*bp;		/* Pointer to the microcode buffer. */
	int		 nbytes;	/* Number of microcode bytes. */
{
	register int	i, j;
	register u_char	*save_bp = bp;
	u_char		fu;
	u_char		chksum = 0;

	/* 
	 * Make sure that the WREG (not the sequencer) drives the 
	 * WCS address bus.
	 */
	wrslave(slic, SL_Z_CNTRL, SLB_STOP_EW | SLB_UNRESET);
	if (rdslave(slic, SL_Z_STATUS) & SLB_HSCRUNNING) {
		printf("Cannot stop HSC\n");
		exit(1);
	}
	/*
	 * Load the WCS.
	 * Accumulate the checksum as we go.
	 */
	for (i = 0; i < nbytes / UWORDSZ; i++) {
		WRITE_WREG(slic, i);
		wrAddr(slic, SL_Z_SCANSR);
		for (j = 0; j < UWORDSZ; j++, bp++) {
			chksum ^= *bp;
			wrData(slic, *bp);
		}
		wrAddr(slic, SL_Z_SHADTOWCS);
	}

	/*
	 * Fill rest of WCS with last instruction
	 */
	for (i = nbytes / UWORDSZ; i < MAX_UCODESIZE; i++) {
		WRITE_WREG(slic, i);
		wrAddr(slic, SL_Z_SHADTOWCS);
	}

	/*
	 * Confirm checksum.
	 *
	 * Final byte is checksum byte.
	 * Result MUST be zero to pass.
	 */
	chksum ^= *bp;
	if (chksum) {
		printf("file checksum failed 0x%x\n", chksum);
		return (FAILURE);
	}

	/*
	 * Now verify the checksum again, reading back the contents of
	 * WCS.  This makes sure that the transfer occured correctly.
	 * *bp still contains the checksum byte, and chksum is zero.
	 */
	for (i = 0; i < nbytes / UWORDSZ; i++) {
		WRITE_WREG(slic, i);
		wrslave(slic, SL_Z_WCSTOPREG, 0);
		wrslave(slic, SL_Z_PREGTOSHAD, 0);
		wrAddr(slic, SL_Z_SCANSR);
		(void)rdData(slic);
		for (j = 0; j < UWORDSZ; j++, save_bp++) {
			fu = rdData(slic);
			chksum ^= fu;
			if (fu != *save_bp) {
				printf("word %d, byte %d MISMATCH ... expecting 0x%x and got 0x%x\n", i, j, *save_bp, fu);
			}
		}
	}
	chksum ^= *bp;
	if (chksum) {
		printf("WCS checksum failed 0x%x\n", chksum);
		return (FAILURE);
	}
	return(PASSED);
}

/*
 * Compare contents of WCS with buffer
 */
cmp_wcs(slic, bp, nbytes)
	u_char		slic;		/* SLIC address of target ZDC. */
	register u_char	*bp;		/* Pointer to the microcode buffer. */
	int		 nbytes;	/* Number of microcode bytes. */
{
	register int	i, j;
	u_char		fu;
	u_char		chksum = 0;

	/* 
	 * Make sure that the WREG (not the sequencer) drives the 
	 * WCS address bus.
	 */
	wrslave(slic, SL_Z_CNTRL, SLB_STOP_EW | SLB_UNRESET);
	if (rdslave(slic, SL_Z_STATUS) & SLB_HSCRUNNING) {
		printf("Cannot stop HSC\n");
		exit(1);
	}

	/*
	 * Now verify the checksum again, reading back the contents of
	 * WCS.  This makes sure that the transfer occured correctly.
	 * *bp still contains the checksum byte, and chksum is zero.
	 */
	for (i = 0; i < nbytes / UWORDSZ; i++) {
		WRITE_WREG(slic, i);
		wrslave(slic, SL_Z_WCSTOPREG, 0);
		wrslave(slic, SL_Z_PREGTOSHAD, 0);
		wrAddr(slic, SL_Z_SCANSR);
		(void)rdData(slic);
		for (j = 0; j < UWORDSZ; j++, bp++) {
			fu = rdData(slic);
			chksum ^= fu;
			if (fu != *bp) {
				printf("word %d, byte %d MISMATCH ... expecting 0x%x and got 0x%x\n", i, j, *bp, fu);
			}
		 }
	}
	chksum ^= *bp;
	if (chksum) {
		printf("WCS checksum failed 0x%x\n", chksum);
		return (FAILURE);
	}
	return(PASSED);
}

/*
 * Single-step the ZDC's HSC.
 */
single_step(slic)
	u_char	slic;		/* SLIC address of target ZDC. */
{

	/* Set HSC stop mode. The 2910 drives the WBUS. */
	wrslave(slic, SL_Z_CNTRL, SLB_UNRESET);
	
	/* Start the HSC. */
	wrslave(slic, SL_Z_STARTHSC, 0);
}

/*
 * Write a microinstruction word to the ZDC's pipeline register.
 */
write_pl(slic, uword)
	u_char	slic;	/* SLIC address of target ZDC. */
	struct uword	*uword;	/* Pointer to one microinstruction word. */
{
	register int i;
	register u_char *p = (u_char *)uword;

	for (i = 0; i < UWORDSZ; i++, p++)
		wrslave(slic, SL_Z_SCANSR, *p);
	wrslave(slic, SL_Z_SHADTOPREG, 0);
}
