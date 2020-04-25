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

#ident	"$Header: weitek.c 1.6 1992/02/13 00:17:27 $"

/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)kern-io:weitek.c	1.3.1.1"

/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *     This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */

/*
 * This file deals with the i386 portion of the emulation and is
 * responsible for moving any operands into/out of the user mode
 * address space.  It calls routines within "wemulate.c" to actually
 * emulate the 1167.
 */


#include "../h/types.h"
#include "../h/signal.h"
#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../balance/engine.h"
#include "../h/vm.h"
#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/psl.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"
#include "../machine/trap.h"
#include "../machine/reg.h"

extern caddr_t kmem_alloc();

/*
 * Define some needed WEITEK instructions.
 */
#define	WINS_STOR	0x03
#define	WINS_STORD	0x23
#define	WINS_STCTX	0x31

/*
 * Define 386 op codes.
 */
#define	REP_REPE		0xF3
#define	REPNE			0xF2
#define	LOCK			0xF0
#define	ADDR_SIZE		0x67
#define	OP_SIZE			0x66
#define	CS_OVERRIDE		0x2E
#define	SS_OVERRIDE		0x36
#define	DS_OVERRIDE		0x3E
#define	ES_OVERRIDE		0x26
#define	FS_OVERRIDE		0x64
#define	GS_OVERRIDE		0x65

#define	MOV_TO_R8		0x8A
#define	MOV_TO_R16_32		0x8B
#define	MOV_FROM_R8		0x88
#define	MOV_FROM_R16_32		0x89
#define	POP			0x8f
#define	MOV_TO_AL		0xA0
#define	MOV_TO_AX_EAX		0xA1
#define	MOV_FROM_AL		0xA2
#define	MOV_FROM_AX_EAX		0xA3
#define	MOVS8			0xA4
#define	MOVS16_32		0xA5
#define	LODS8			0xAC
#define	LODS16_32		0xAD
#define	STOS8			0xAA
#define	STOS16_32		0xAB
#define	MOV_IMM8		0xC6
#define	MOV_IMM16_32		0xC7
#define	GRP5			0xFF
#define	MODRM_OPCODE		0x38
#define	MODRM_OPCODE_PUSH	0x30
#define	PUSH			0x1ff	/* pseudo op for push */

/*
 * Decoded 386 instruction information.
 */
typedef	struct {
	long	ins_loc;		/* Beginning of instruction */
	long	op_code_loc;		/* Location of op code */
	int	op_code;		/* Value of op code */
	int	op_size;		/* 1, 2, or 4 byte operand */
	int	addr_size;		/* 1, 2, or 4 byte address */
	int	len;			/* Number of bytes in instruction, <= 15 */
	int	op_error;		/* 1 if an instruction error */
	int	copy_error;		/* copyout/copyin error */
	int	repeat;			/* 1 if a REP prefix seen */
} ins386;

/*
 * Define the mappings of the register bits in the ModR/M byte to the offsets
 * of the corresponding registers in the u_ar0 element of the user
 * information.
 */
static	Reg_to_user[8] = {EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI};

/*
 * Define a mask for data which will be 1, 2, or 4 bytes long.  The
 * undefined sizes are zero to insure that nothing changes, but a consistency
 * check could look for the zeros since they are illegal sizes anyway.
 */
static	size_mask[5] = {0, 0xFF, 0xFFFF, 0, 0xFFFFFFFF};

extern	void wtl1167_mr();
extern	void wtl1167_rr();
extern	int wtl1167_read();
static	void store_value();
static	void decode_ins();
static	void inc_string();
static	int reg_op();
static	long calc_esi(), calc_edi();
static	long get_value();

#ifndef	lint
/*
 * This returns the value of the D bit in the executable segment-descriptor
 * for the current instruction.  This bit determines the default address and
 * operand size.
 * 
 * NOTE: This always returns 1 because this port of UNIX always uses 32 bit
 * values.
 */
#define	segdesc_D()	(1)
#endif	/* lint */

#ifndef lint
asm void wfpusave(fpup)
{
%reg fpup;
/PEEPOFF
	fsave	(fpup)
	wait
/PEEPON
}

asm void wfpurestor(fpup)
{
%reg fpup;
/PEEPOFF
	frstor	(fpup)
/PEEPON
}
#else	/* lint */
void wfpusave();
void wfpurestor();
#endif	/* lint */

#define	pcr	u.u_fpaesave->fpae_pcr
#define	regs	u.u_fpaesave->fpae_regs

/*
 * On first use of emulation area, allocate it
 */
static void
emula_setup(uptr)
	struct user *uptr;
{
	/*
	 * On first use of emulation area, allocate and zero it
	 */
	if (uptr->u_fpaesave == NULL) {
		uptr->u_fpaesave = (struct fpaesave *)
			kmem_alloc(sizeof(struct fpaesave));
		bzero((caddr_t)uptr->u_fpaesave, sizeof(struct fpaesave));
	}
}

/*
 * This called to emulate the instruction.  It is passed the address in user
 * space which caused the fault.  It is assumed that this is within the
 * memory area occupied by the WEITEK board.
 * 
 * The offset from the beginning of the WEITEK board determines the operation
 * to be performed.  This may be considered an instruction whose format is
 * given by the WEITEK documentation.
 */
emula_fpa(vaddr)
	unsigned vaddr;
{
	struct	fpusave	fpusave;
	register struct fpusave *fpup = &fpusave;
	ins386	ins;
	int	src1;
	long	value;
	int	wasusingfpu ;
	unsigned oldcr0;

	/* Arrange for register save area setup if necessary */
	emula_setup(&u);

	/*
	 * Record previous state of FPU
	 */
	oldcr0 = READ_MSW();
	wasusingfpu = l.usingfpu;

	/*
	 * Enable the FPU for our immediate usage only.
	 */
	WRITE_MSW((unsigned)l.fpuon);
	l.usingfpu = 1;

	/*
         *If our user was using it himself, need to save state
	 */
	if (wasusingfpu) {
		/*
		 * This is probably overly conservative, as it appears
		 * that the kernel does not use floating point (except
		 * for this emulation).  In any case, saving the fpu state
		 * in the manner will allow interrupt level routines to
		 * use the fpu, so long as they take the proper precautions
		 * (i.e. save the fpu state on the stack as here).
		 * It's important to note that the user mode fpu is saved
		 * on our stack and our fpu is saved in the uarea (assuming
		 * a page fault on an smovl).
		 */
		wfpusave(fpup);
	} 

	src1 = ((vaddr >> 5) & 0x1C) | (vaddr & 3);
	decode_ins(&ins);

	if (!ins.op_error) {
		if (src1 != 0) {
			wtl1167_rr(vaddr);
		} else {
			switch ((vaddr >> 10) & 0x3F) {
			case WINS_STOR:		/* Register to memory */
			case WINS_STORD:
			case WINS_STCTX:
				value = wtl1167_read(vaddr);
				store_value(&ins, value, vaddr);
				break;

			default:		/* Memory to register */
				value = get_value(&ins, vaddr);
				if (!ins.copy_error && !ins.op_error) {
					wtl1167_mr(vaddr, value);
				}
				break;
			}
		}
	}

	u.u_ar0[EIP] += ins.len;	/* Skip the instruction */
        /*
	 * Restore accessibility and flags for FPU
	 */


	if (wasusingfpu) {
		wfpurestor(fpup);
	} 

	/*
	 * Restore accessibility and flags for FPU
	 */
	WRITE_MSW(oldcr0);
	l.usingfpu = wasusingfpu;

	/*
	 * Coalesce the error codes into something understood by the
	 * caller.
	 */
	if (pcr & FPA_PCR_AE_EE) {
		return(FPA_EM_EXCEPTION);
	} else if (ins.copy_error) {
		return(FPA_EM_SEG);
	} else if (ins.op_error) {
		return(FPA_EM_ILLINSTR);
	} else {
		return(FPA_EM_OK);
	}
}

/*
 * This is given a value to store in the 386's registers or memory.  The
 * 386 instruction has already undergone some decoding the result of which
 * is pointed to by the instruction information.
 */
static void
store_value(ins, value, vaddr)
	ins386	*ins;
	long	value;
	unsigned vaddr;
{
	int	Reg, user_reg;
	long	mask, esi, edi;
	register unsigned sp;
	mask = size_mask[ins->op_size];

	switch (ins->op_code) {
	case MOV_TO_R8:
	case MOV_TO_R16_32:
		Reg = fuibyte((caddr_t)ins->op_code_loc + 1) >> 3 & 0x07;
		user_reg = Reg_to_user[Reg];
		u.u_ar0[user_reg] = (u.u_ar0[user_reg] & ~mask) |
			(value & mask);
		break;

	case MOV_TO_AL:
	case MOV_TO_AX_EAX:
		u.u_ar0[EAX] = (u.u_ar0[EAX] & ~mask) | (value & mask);
		break;

	case LODS8:
	case LODS16_32:
		u.u_ar0[EAX] = (u.u_ar0[EAX] & ~mask) | (value & mask);
		inc_string(ins, 1, 0);
		break;

	case MOVS8:
	case MOVS16_32:
		esi = calc_esi(ins);

		if (esi+VA_USER != vaddr) {
			ins->op_error = 1;
		} else {
			edi = calc_edi(ins);
			if (copyout((caddr_t)&value, (caddr_t)edi,
				(u_int)ins->op_size)) {
				ins->copy_error = 1;
			} else {
				inc_string(ins, 1, 1);
			}
		}
		break;

	case PUSH:
		sp = u.u_ar0[ESP];
		sp -= ins->op_size;
		u.u_ar0[ESP] = sp;
		if (copyout((caddr_t)&value, (caddr_t)sp, (u_int)ins->op_size)) {
			ins->copy_error = 1;
		}
		break;

	default:
		ins->op_error = 1;
		break;
	}
}

/*
 * This gets a value from the 386's registers or memory to move to the Weitek
 * board.  The 386 instruction has already undergone some decoding the result
 * of which is pointed to by the instruction information.
 */
static long
get_value(ins, vaddr)
	ins386	*ins;
	unsigned vaddr;
{
	int	Reg, user_reg, imm_start;
	long	value, value2, mask, esi, edi;
	register unsigned int sp;

	mask = size_mask[ins->op_size];

	switch (ins->op_code) {
	case POP:
		sp = u.u_ar0[ESP];
		if (copyin((caddr_t)sp, (caddr_t)&value, (u_int)ins->op_size)) {
			ins->copy_error = 1;
			break;
		}
		sp += ins->op_size;
		u.u_ar0[ESP] = sp;
		break;

	case MOV_FROM_R8:
	case MOV_FROM_R16_32:
		Reg = fuibyte((caddr_t)ins->op_code_loc + 1) >> 3 & 0x07;
		user_reg = Reg_to_user[Reg];
		value = (u.u_ar0[user_reg] & mask);
		break;

	case MOV_FROM_AL:
	case MOV_FROM_AX_EAX:
		value = (u.u_ar0[EAX] & mask);
		break;

	case STOS8:
	case STOS16_32:
		value = (u.u_ar0[EAX] & mask);
		inc_string(ins, 0, 1);
		break;

	case MOV_IMM8:
	case MOV_IMM16_32:
		/*
		 * Here we have to fetch the immediate value, which we know
		 * must be the last thing in the instruction according to the
		 * manual.  So, we point to the start of the immediate data,
		 * point to the right place in the value and copy over op_size
		 * characters.  Then we mask just to be sure extra bits are
		 * cleared.
		 */
		imm_start = ins->ins_loc + ins->len - ins->op_size;

		switch (ins->op_size) {
		case 1:
			value = fuibyte((caddr_t)imm_start);
			if (value == -1) {
				ins->copy_error = 1;
			}
			break;

		case 2:
			value = fuibyte((caddr_t)imm_start);
			value2 = fuibyte((caddr_t)imm_start + 1);
			if (value == -1 || value2 == -1) {
				ins->copy_error = 1;
			}
			else {
				value += (value2 << 8);
			}
			break;

		case 4:
			if (copyin((caddr_t)imm_start, (caddr_t)&value, sizeof(value)))
				ins->copy_error = 1;
			break;

		default:
			ins->op_error = 1;
			break;
		}
		break;

	case MOVS8:
	case MOVS16_32:
		edi = calc_edi(ins);

		if (edi+VA_USER != vaddr) {
			ins->op_error = 1;
		} else {
			esi = calc_esi(ins);

			if (copyin((caddr_t)esi, (caddr_t)&value, (u_int)ins->op_size)) {
				ins->copy_error = 1;
			} else {
				inc_string(ins, 1, 1);
			}
		}

		break;

	default:
		ins->op_error = 1;
		break;
	}

	return(value);
}

/*
 * This calculates the address in use space (i.e. the linear address) of the
 * esi used in the move string instruction.
 * 
 * NOTE: This version does not take segments into consideration because this
 * version of UNIX always sets them so that the offset due to the segment is
 * zero.
 */
static long
calc_esi(ins)
	ins386 *ins;
{
	return(u.u_ar0[ESI] & size_mask[ins->addr_size]);
}

/*
 * This calculates the address in use space (i.e. the linear address) of the
 * edi used in the move string instruction.
 * 
 * NOTE: This version does not take segments into consideration because this
 * version of UNIX always sets them so that the offset due to the segment is
 * zero.
*/
static long
calc_edi(ins)
	ins386	*ins;
{
	return(u.u_ar0[EDI] & size_mask[ins->addr_size]);
}

/*
 * This increments the SI and DI registers, as necessary, after a string
 * instruction.  It will also decrement the CX register if this is a repeat
 * operation.
 * 
 * The one really odd thing this does is that, if we are doing a repeat
 * operation and we have not reached zero, it sets the instruction length to
 * zero so that we re-run the instruction.
 */
static void
inc_string(ins, inc_si, inc_di)
	ins386	*ins;
	int inc_si, inc_di;
{
	long	addr_mask;
	int	inc_dec;

	if ((u.u_ar0[FLAGS] & FLAGS_DF) == 0) {
		inc_dec = ins->op_size;
	} else {
		inc_dec = -ins->op_size;
	}
	addr_mask = size_mask[ins->addr_size];

	if (inc_si) {
		u.u_ar0[ESI] = u.u_ar0[ESI] & ~addr_mask |
			       u.u_ar0[ESI] + inc_dec & addr_mask;
	}

	if (inc_di) {
		u.u_ar0[EDI] = u.u_ar0[EDI] & ~addr_mask |
			       u.u_ar0[EDI] + inc_dec & addr_mask;
	}

	if (ins->repeat) {
		u.u_ar0[ECX] = u.u_ar0[ECX] & ~addr_mask |
			       u.u_ar0[ECX] - 1 & addr_mask;
		if ((u.u_ar0[ECX] & addr_mask) != 0)
			ins->len = 0;
	}
}

/*
 * This does simple decoding of instructions in order to find out their size,
 * the size of the their operands and addresses, the op codes, etc.  It is
 * given a pointer to a structure for holding all of this information.  The
 * length given will be at least one so that if an error occurs and the program
 * tries to handle it, the program won't get in a loop because the instruction
 * pointer never advanced.  This will probably cause the program to die with
 * an illegal op code, but at least it will die.
 */

static void
decode_ins(ins)
	ins386	*ins;
{
	int	inst_length, data_size, address_size, switch_op_size,
		switch_addr_size, done, inst_byte;
	ins->ins_loc = u.u_ar0[EIP];

	ins->copy_error = 0;
	ins->op_error = 0;
	ins->repeat = 0;

	if (segdesc_D())
		data_size = address_size = 4;
	else
		data_size = address_size = 2;

	switch_op_size = switch_addr_size = 0;
	inst_length = 0;
	done = 0;

	do {
		inst_byte = fuibyte((char *)u.u_ar0[EIP] + inst_length);

		switch (inst_byte) {
		case REP_REPE:		/* Instruction prefix codes */
			inst_length++;
			ins->repeat = 1;
			break;

		case REPNE:
		case LOCK:
			inst_length++;
			break;

		case ADDR_SIZE:		/* Address size prefix */
			switch_addr_size = 1;
			inst_length++;
			break;

		case OP_SIZE:		/* Operand size prefix */
			switch_op_size = 1;
			inst_length++;
			break;

		case CS_OVERRIDE:	/* Segment override */
		case SS_OVERRIDE:
		case DS_OVERRIDE:
		case ES_OVERRIDE:
		case FS_OVERRIDE:
		case GS_OVERRIDE:
			inst_length++;
			break;

		case -1:
			ins->copy_error = 1;
			ins->op_error = 1;
			return;

		default:
			done = 1;
			break;
		}
	} while (!done);

	/*
	 * If we were told to override the segment default for the operand or
	 * address size, switch from 2 to 4 bytes or 4 to 2 bytes.
	 */
	if (switch_op_size) {

		data_size = 6 - data_size;
	}
	if (switch_addr_size) {
		address_size = 6 - address_size;
	}

	ins->op_size = data_size;
	ins->addr_size = address_size;
	ins->op_code = inst_byte;
	ins->op_code_loc = u.u_ar0[EIP] + inst_length;

	/*
	 * The following loop handles the lengths for specific instructions.
	 * Note that the op size has been set to either 2 or 4, which is the
	 * way the 386 handles distinguishing between the two.  This doesn't
	 * work for one byte operands, however, because this information is
	 * specified by use of the appropriate op code.  Thus, each instruction
	 * with a one byte operand updates the op_size element.
	 */

	switch (inst_byte) {
	case MOV_TO_R8:
	case MOV_FROM_R8:
		ins->op_size = 1;
		inst_length += 1;
		inst_length += reg_op(ins, inst_length, address_size);
		break;

	case POP:
	case MOV_TO_R16_32:
	case MOV_FROM_R16_32:
		inst_length += 1;
		inst_length += reg_op(ins, inst_length, address_size);
		break;

	case MOV_TO_AL:
	case MOV_FROM_AL:
		ins->op_size = 1;
		inst_length += 1 + address_size;
		break;

	case MOV_TO_AX_EAX:
	case MOV_FROM_AX_EAX:
		inst_length += 1 + address_size;
		break;

	case MOV_IMM8:
		ins->op_size = 1;
		inst_length + = 1;
		inst_length += reg_op(ins, inst_length, address_size) + 1;
		break;

	case MOV_IMM16_32:
		inst_length + = 1;
		inst_length += reg_op(ins, inst_length, address_size) +
			data_size;
		break;

	case MOVS8:
		ins->op_size = 1;
		inst_length += 1;
		break;

	case MOVS16_32:
		inst_length += 1;
		break;

	case LODS8:
	case STOS8:
		ins->op_size = 1;
		inst_length += 1;
		break;

	case LODS16_32: 
	case STOS16_32:
		inst_length += 1;
		break;

	case GRP5:
		inst_length += 1;
		inst_length += reg_grp5(ins, inst_length, address_size);
		break;

	default:
		inst_length += 1;
		ins->op_error = 1;
		break;
	}

	ins->len = inst_length;
	return;
}

/*
 * This handles a ModR/M byte with the register field specifying the opcode.
 */
static int
reg_grp5(ins, inst_length, address_size)
	ins386 *ins;
	int inst_length;
	int address_size;
{
	register int reg_op_byte;

	if ((reg_op_byte = fuibyte((char *)u.u_ar0[EIP] + inst_length)) < 0) {
		ins->copy_error = -1;
		ins->op_error = 1;
		return(0);
	}

	switch (reg_op_byte & MODRM_OPCODE) {
	case MODRM_OPCODE_PUSH:
		/*
		 * The instruction is a push instruction.
		 */
		ins->op_code = PUSH;
		return(modRM(reg_op_byte, address_size));
	default:
		/*
		 * Could also have inc, dec, call, jmp.
		 */
		ins->op_error = 1;
		return(2);
	}
}

/*
 * This handles a ModR/M byte with a register operand and an r/m operand.  It
 * is given the offset of the ModR/M byte from the current value of EIP and
 * the address size.  The address_size is 2 for 16 bit addressing and 4 for
 * 32 bit addressing.
 */
static int
reg_op(ins, inst_length, address_size)
	ins386 *ins;
	int inst_length;
	int address_size;
{
	int	reg_op_byte;

	reg_op_byte = fuibyte((char *)u.u_ar0[EIP] + inst_length);

	if (reg_op_byte == -1) {
		ins->op_error = 1;
		ins->copy_error = 1;
		return(0);
	}

	return(modRM(reg_op_byte, address_size));
}

/*
 * Determine the total length of the instruction based on the size
 * of the modR/M byte.
 */
static int
modRM(reg_op_byte, address_size)
	int reg_op_byte;
	register int address_size;
{
	register int len, Mod, R_M;

	Mod = ((reg_op_byte >> 6) & 0x03);
	R_M = (reg_op_byte & 0x07);

	switch (address_size) {
	case 2:
		switch (Mod) {
		case 0:
			if (R_M == 6) {
				len = 3;
			} else {
				len = 1;
			}
			break;

		case 1:
			len = 2;
			break;

		case 2:
			len = 3;
			break;

		case 3:
			len = 1;
			break;
		}
		break;
	case 4:
		switch (Mod) {
		case 0:
			switch (R_M) {
			case 0x4:
				len = 2;
				break;
			case 0x5:
				len = 5;
				break;
			default:
				len = 1;
				break;
			}
			break;
		case 1:
			if (R_M == 0x04) {
				len = 3;
			} else {
				len = 2;
			}
			break;
		case 2:
			if (R_M == 0x04) {
				len = 6;
			} else {
				len = 5;
			}
			break;
		case 3:
			len = 1;
			break;
		}
		break;
	}

	return(len);
}

/*
 * Save and fiddle the pcr the same way the actual fpa code
 * does (fpa_trap).
 */
emula_fpa_trap()
{
	register int ebits;
	register int tpcr = pcr;

	/*
	 * Set u_code to the un-masked exception bits, marked as FPA floating
	 * exception.  Restore PCR with relevent AE bits cleared.
	 */
	ASSERT_DEBUG(u.u_fpaesave, "emula_fpa_trap: not initialized");
	ebits = (tpcr & FPA_PCR_AE) & ~(tpcr >> FPA_PCR_EM_SHIFT);
	u.u_segvcode = ebits;
	u.u_code = (T_FPA <<16) | FPA_FPE;
	pcr = tpcr & ~(ebits | FPA_PCR_AE_EE);
}

#ifdef	FPU_SIGNAL_BUG
/*
 * Move registers between the real FPA and emulation registers.
 */
emula_fpa_hw2sw(fpap)
	register struct fpasave *fpap;
{
	register int i, j;

	/* Arrange for register save area setup if necessary */
	emula_setup(&u);

	pcr = fpap->fpa_pcr;
	for (i=0, j=1; i < FPA_NREGS; i++, j++)
		regs[j^1] = fpap->fpa_regs[i];
}

emula_fpa_sw2hw(fpap)
	register struct fpasave *fpap;
{
	register int i, j;

	ASSERT_DEBUG(u.u_fpaesave, "emula_fpa_sw2hw: not initialized");
	fpap->fpa_pcr = pcr;
	for (i=0, j=1; i < FPA_NREGS; i++, j++)
		fpap->fpa_regs[i] = regs[j^1];
}

#else	/* FPU_SIGNAL_BUG */
/*
 * Move registers between the real FPA and emulation registers.
 */
emula_fpa_hw2sw()
{
	register int i, j;

	/* Arrange for register save area setup if necessary */
	emula_setup(&u);

	pcr = u.u_fpasave.fpa_pcr;
	for (i=0, j=1; i < FPA_NREGS; i++, j++)
		regs[j^1] = u.u_fpasave.fpa_regs[i];
}

emula_fpa_sw2hw()
{
	register int i, j;

	ASSERT_DEBUG(u.u_fpaesave, "emula_fpa_sw2hw: not initialized");
	u.u_fpasave.fpa_pcr = pcr;
	for (i=0, j=1; i < FPA_NREGS; i++, j++)
		u.u_fpasave.fpa_regs[i] = regs[j^1];
}

#endif	/* FPU_SIGNAL_BUG */
