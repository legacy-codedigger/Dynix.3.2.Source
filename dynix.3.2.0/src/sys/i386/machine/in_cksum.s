/ $Header: in_cksum.s 1.2 91/03/14 $

/
/ $Copyright:	$
/ Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
/ Sequent Computer Systems, Inc.   All rights reserved.
/  
/ This software is furnished under a license and may be used
/ only in accordance with the terms of that license and with the
/ inclusion of the above copyright notice.   This software may not
/ be provided or otherwise made available to, or used by, any
/ other person.  No title to or ownership of the software is
/ hereby transferred.
/

#include "assym.h"

		.globl	_in_cksum

		.text

/
/ We use registers %eax, %ebx, %ecx, %edx, %esi and %edi in this routine.
/ Registers %eax, %ecx and %edx are caller saves by convention.  Registers
/ %ebx, %esi and %edi are callee saves.  The 16-bit one's complement of the
/ one's complement sum of the data is returned in %eax.  The upper 16 bits
/ of %eax are guarenteed to be 0 on return, (it's done with mirrors though).
/
_in_cksum:	pushl	%ebx
		pushl	%esi
		pushl	%edi

/
/ Register %edx is the partial checksum.  %edi is a boolean-valued rotation
/ indication.  It is 0 when the rotation is correct and all 1's when it is
/ rotated 8-bits.  %edx's rotation changes each time we re-align the data
/ pointer (done at both the beginning and end of a message block).  We
/ initialize %edx and %edi here to 0.  Throughout the rest of the routine %esi
/ holds a pointer to the next message block to checksum.  We initialize it here
/ from argument 0 on the stack.
/
		xorl	%edx,%edx
		xorl	%edi,%edi
		movl	16(%esp),%esi

/
/ This is the top of the mbuf loop.  Argument 1 contains the current number of
/ bytes left to checksum.  When it drops to 0, we're done.
/
in_cksum_0:	movl	20(%esp),%eax
		orl	%eax,%eax
		jz	in_cksum_44

/
/ Set %ebx to %esi->m_off, %ecx to %esi->m_len, %ebx to %esi + %ebx, and %esi
/ to %esi->m_next.  %ebx will point to the first byte of data to checksum, %ecx
/ will be the length of the current mbuf and %esi will point to the next mbuf.
/

		movl	M_OFF(%esi),%ebx
		movzwl	M_LEN(%esi),%ecx
		addl	%esi,%ebx
		movl	M_NEXT(%esi),%esi

/
/ For each mbuf we take the minimum of argument 1 and %ecx, and store the
/ result back in %ecx.  We then decrement argument 1 by %ecx and save the
/ result back on the stack.  %ecx now contains the number of bytes to checksum
/ in this mbuf.  We will use it to drive the inner checksum loop, below.
/
		cmpl	%ecx,%eax
		ja	in_cksum_1
		movl	%eax,%ecx
in_cksum_1:	subl	%ecx,%eax
		movl	%eax,20(%esp)

/
/ Check to see that there are at least 4 bytes to checksum in this mbuf.  This
/ check has two purposes: (1) if there aren't at least 4 bytes, then falling
/ into the alignment (and eventually the unrolled loop code) is a complete
/ waste of time, and (2) the alignment code would become more expensive if it
/ had to deal with both misaligned and short mbufs.  If there aren't 4 bytes to
/ checksum, then we short-circuit down to the end of mbuf code.  Note, however,
/ the end of mbuf code can incur some additional overhead due to the fact that
/ %ebx could be misaligned.
/
		cmpl	$4,%ecx
		jb	in_cksum_40

/
/ We know that there are at least 4 bytes to checksum.  Knowing this makes the
/ alignment code much simpler.  No matter how %ebx is currently aligned, we
/ know that is it safe to checksum up to the next 32-bit boundary. Rather than
/ write one routine to handle all 4 alignment cases, it's faster to write 4
/ routines and switch on the alignment.  in_cksum_vec1 is the jump vector used
/ to switch to the 4 different alignment handlers.  Note, case 0 is the optimum
/ case, (i.e. %ebx is already 32-bit aligned) so it ends up at the top of the
/ unrolled loop code.  The other 3 cases actually have work to do, before
/ meeting case 0 there.  When we do get to the unrolled loop code, %ebx will be
/ 32-bit aligned.
/
		movl	%ebx,%eax
		andl	$0x3,%eax
		jmp	*in_cksum_vec1(,%eax,4)


/
/ Re-alignment case 1: We have a pointer misaligned by one byte.  To re-align
/ the data pointer we need to checksum a single byte, followed by a 16-bit
/ word.  This is only slightly tricky in that we have to add-in the first byte
/ at the current rotation, then rotate the partial checksum 8 bits, and add-in
/ the word, all the time being concerned about not trashing the carry flag.
/ After checksumming the 3 bytes, advance the data pointer by 3 bytes,
/ decrement the length by 3 bytes and head on down to the unrolled loop.
/
in_cksum_2:	addb	(%ebx),%dl
		lahf
		roll	$8,%edx
		notl	%edi
		sahf
		adcw	1(%ebx),%dx
		adcw	$0,%dx
		leal	3(%ebx),%ebx
		subl	$3,%ecx
		jmp	in_cksum_5

/
/ Re-alignment case 3: We have a pointer misaligned by three bytes.  To
/ re-align the data pointer we need to checksum a single byte.  This is pretty
/ simple, just checksum the byte, add-in the residual carry, rotate the
/ partial checksums, advance %ebx, decrement %ecx and meet everybody down
/ below.
/
in_cksum_3:	movzbl	(%ebx),%eax
		addl	%eax,%edx
		adcl	$0,%edx
		roll	$8,%edx
		notl	%edi
		leal	1(%ebx),%ebx
		subl	$1,%ecx
		jmp	in_cksum_5

/
/ Re-alignment case 2: We have a pointer misaligned by two bytes.  To re-align
/ the data pointer just add-in the 16-bit word, add-in the residual carry,
/ advance %ebx, decrement %ecx and fall into the unrolled loop code.
/
in_cksum_4:	addw	(%ebx),%dx
		adcw	$0,%dx
		leal	2(%ebx),%ebx
		subl	$2,%ecx

/
/ We now have a 32-bit aligned data pointer and there's reason to believe that
/ there is enough data for the unrolled loops to pay off.  It's possible that
/ the re-alignment process dropped the byte count below 4 bytes.  In this case
/ we miss both unrolled loops and eventually end up down at the end of mbuf
/ code.  Oh well...
/
/ Pre-compute (%ecx modulus 128) divided by 4, and save it in %eax (we'll use
/ it later).
/
/ Divide %ecx by 128 to determine how many interations of the 128-byte unrolled
/ loop to execute.  Note: we only use the bottom 16 bits of %ecx as a loop
/ control variable.  This allows us to hide the remainder of the division by
/ 128 in the top 16 bits.  We need to preserve them for later processing.  The
/ orw instruction is there to set ZF appropriately and clears CF so that the
/ first adcl instruction doesn't add-in a superfluous carry.
/
in_cksum_5:	rorl	$7,%ecx
		shldl	$5,%ecx,%eax
		andl	$0x1f,%eax
		orw	%cx,%cx
		jz	in_cksum_7

/
/ This is the top of the 128-bytes unrolled checksum loop.  Each iteration will
/ checksum 128 bytes.  Note: the carry bit doesn't get trashed at the bottom of
/ the loop so we don't have to worry about adding it in each time around.
/
in_cksum_6:	adcl	(%ebx),%edx
		adcl	4(%ebx),%edx
		adcl	8(%ebx),%edx
		adcl	12(%ebx),%edx
		adcl	16(%ebx),%edx
		adcl	20(%ebx),%edx
		adcl	24(%ebx),%edx
		adcl	28(%ebx),%edx
		adcl	32(%ebx),%edx
		adcl	36(%ebx),%edx
		adcl	40(%ebx),%edx
		adcl	44(%ebx),%edx
		adcl	48(%ebx),%edx
		adcl	52(%ebx),%edx
		adcl	56(%ebx),%edx
		adcl	60(%ebx),%edx
		adcl	64(%ebx),%edx
		adcl	68(%ebx),%edx
		adcl	72(%ebx),%edx
		adcl	76(%ebx),%edx
		adcl	80(%ebx),%edx
		adcl	84(%ebx),%edx
		adcl	88(%ebx),%edx
		adcl	92(%ebx),%edx
		adcl	96(%ebx),%edx
		adcl	100(%ebx),%edx
		adcl	104(%ebx),%edx
		adcl	108(%ebx),%edx
		adcl	112(%ebx),%edx
		adcl	116(%ebx),%edx
		adcl	120(%ebx),%edx
		adcl	124(%ebx),%edx
/
/ Recall from above that we only use the lower 16 bits of %ecx for loop
/ control.  Note: neither the decw or the leal affects the carry, so we we
/ have a residual carry floating back to the top of the loop, and carry *is*
/ significant when we fall through.
/
		decw	%cx
		leal	128(%ebx),%ebx
		jnz	in_cksum_6

/
/ At this point we know there are between 0 and 127 more bytes to checksum.  We
/ pre-computed the value (%ecx modulus 128) divided by 4 and stored it in %eax.
/ We will use %eax now as an index into in_cksum_vec2.  The trick is to
/ jump into the following unrolled loop at exactly the right place.  When we
/ fall through we'll have 0 to 3 bytes left to checksum.  Carry is significant
/ on entrance, but will be added in at the end of the unrolled loop.  Note: at
/ the end we will need the leal to advance the data pointer.  We only leal,
/ however, if there is a need to do so, (i.e. %eax > 0).
/
in_cksum_7:	jmp	*in_cksum_vec2(,%eax,4)
in_cksum_8:	adcl	120(%ebx),%edx
in_cksum_9:	adcl	116(%ebx),%edx
in_cksum_10:	adcl	112(%ebx),%edx
in_cksum_11:	adcl	108(%ebx),%edx
in_cksum_12:	adcl	104(%ebx),%edx
in_cksum_13:	adcl	100(%ebx),%edx
in_cksum_14:	adcl	96(%ebx),%edx
in_cksum_15:	adcl	92(%ebx),%edx
in_cksum_16:	adcl	88(%ebx),%edx
in_cksum_17:	adcl	84(%ebx),%edx
in_cksum_18:	adcl	80(%ebx),%edx
in_cksum_19:	adcl	76(%ebx),%edx
in_cksum_20:	adcl	72(%ebx),%edx
in_cksum_21:	adcl	68(%ebx),%edx
in_cksum_22:	adcl	64(%ebx),%edx
in_cksum_23:	adcl	60(%ebx),%edx
in_cksum_24:	adcl	56(%ebx),%edx
in_cksum_25:	adcl	52(%ebx),%edx
in_cksum_26:	adcl	48(%ebx),%edx
in_cksum_27:	adcl	44(%ebx),%edx
in_cksum_28:	adcl	40(%ebx),%edx
in_cksum_29:	adcl	36(%ebx),%edx
in_cksum_30:	adcl	32(%ebx),%edx
in_cksum_31:	adcl	28(%ebx),%edx
in_cksum_32:	adcl	24(%ebx),%edx
in_cksum_33:	adcl	20(%ebx),%edx
in_cksum_34:	adcl	16(%ebx),%edx
in_cksum_35:	adcl	12(%ebx),%edx
in_cksum_36:	adcl	8(%ebx),%edx
in_cksum_37:	adcl	4(%ebx),%edx
in_cksum_38:	adcl	(%ebx),%edx
		leal	0(%ebx,%eax,4),%ebx
in_cksum_39:	adcl	$0,%edx

/
/ At this point we have fallen through the unrolled loops.  It's now time to
/ wrap this mbuf up and move on to the next one.  Although end of mbuf
/ processing is simpler than the re-alignment processing, it is still faster to
/ switch on the residual byte count to a fast count-specific end of mbuf
/ handler.  in_cksum_vec3 is used to vector to the end of mbuf handler for the
/ appropriate residual count.  Case 0, of course, is the best case since it
/ means we're done.  All other cases require some extra straight-forward
/ processing.
/
		roll	$7,%ecx
		andl	$0x3,%ecx
		jmp	*in_cksum_vec3(,%ecx,4)

/
/ We got here because there were less than 4 bytes to checksum in this message
/ block, remember?  Well, we're just going to do the same thing as above: Use
/ %ecx as an index into in_cksum_vec3.  The reason we ended up here instead of
/ up there, is that %ecx is in a slightly different state.  Up there it expects
/ that %ecx has the index into in_cksum_vec3 in the upper 16 bits.  However,
/ when we bugged out above, %ecx hadn't been shifted right yet.  No biggie...
/
in_cksum_40:	jmp	*in_cksum_vec3(,%ecx,4)

/
/ We have one more byte to checksum in this mbuf.  This is easy to do: load the
/ last byte into %eax, add it to %edx, add-in any residual carry, rotate the
/ partial checksum and loop back to the top.
/
in_cksum_41:	movzbl	(%ebx),%eax
		addl	%eax,%edx
		adcl	$0,%edx
		roll	$8,%edx
		notl	%edi
		jmp	in_cksum_0

/
/ We have two bytes left to checksum.  Add in the final 16-bits, add-in any
/ residual carry and loop back around.
/
in_cksum_42:	addw	(%ebx),%dx
		adcw	$0,%dx
		jmp	in_cksum_0

/
/ We have three more bytes to checksum.  Add-in a 16-bit word and any residual
/ carry, rotate the partial checksum, add-in the final byte and any residual
/ carry and loop around.
/
in_cksum_43:	addw	(%ebx),%dx
		movzbl	2(%ebx),%eax
		adcl	%eax,%edx
		adcl	$0,%edx
		roll	$8,%edx
		notl	%edi
		jmp	in_cksum_0

/
/ We're done!  Add the upper and lower 16-bits of the partial checksum
/ together, add-in any residual carry, take the 1's complement of that and
/ do any final rotation.  (Actually, the maintanance of %edi which we have been
/ doing so diligently has been for this step alone!)  Finally fall into the
/ function epilogue.  Note: the upper 16-bits of %eax are 0s.  We don't
/ explicitly set them so here.  We are counting on knowledge from previous
/ events for this to be so.  (An exercise for the reader.)
/
in_cksum_44:	shldl	$16,%edx,%eax
		addw	%dx,%ax
		adcw	$0,%ax
		notw	%ax
		movb	$8,%cl
		andl	%edi,%ecx
		rolw	%cl,%ax

/
/ Restore %ebx, %esi and %edi.  The return value is in %eax.  We didn't
/ allocate any stack variables, so there's no need to restore %esp.  Then
/ return.
/
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

		.data

/
/ in_cksum_vec1 is used as a jump table to the appropriate re-alignment handler
/ before falling into the unrolled loop code.  We use the equation %ebx % 4 as
/ an index into in_cksum_vec1.  Case 0 indicates no misalignment and therefore
/ ends up at the top of the unrolled loop.  The other cases must deal with the
/ fact that the data pointer isn't 32-bit aligned, and goes ahead and
/ pre-checksums these odd bytes.  Once down, these misaligned cases meet case
/ 0 at the top of the unrolled loop.
/
in_cksum_vec1:	.long	in_cksum_5
		.long	in_cksum_2
		.long	in_cksum_4
		.long	in_cksum_3

/
/ in_cksum_vec2 is used as a jump table into the 0 to 128 byte checksummer.
/ After interating through the 128-byte unrolled checksum loop, we know that
/ there are less then 128 bytes left to checksum.  in_cksum_vec2 allows us
/ to checksum the remaining 32-bit long words.  When we fall through, all we
/ have left to do in jump through in_cksum_vec3 to handle the four possible
/ end of mbuf cases (i.e. 0 to 3 bytes left to checksum).
/
in_cksum_vec2:	.long	in_cksum_39
		.long	in_cksum_38
		.long	in_cksum_37
		.long	in_cksum_36
		.long	in_cksum_35
		.long	in_cksum_34
		.long	in_cksum_33
		.long	in_cksum_32
		.long	in_cksum_31
		.long	in_cksum_30
		.long	in_cksum_29
		.long	in_cksum_28
		.long	in_cksum_27
		.long	in_cksum_26
		.long	in_cksum_25
		.long	in_cksum_24
		.long	in_cksum_23
		.long	in_cksum_22
		.long	in_cksum_21
		.long	in_cksum_20
		.long	in_cksum_19
		.long	in_cksum_18
		.long	in_cksum_17
		.long	in_cksum_16
		.long	in_cksum_15
		.long	in_cksum_14
		.long	in_cksum_13
		.long	in_cksum_12
		.long	in_cksum_11
		.long	in_cksum_10
		.long	in_cksum_9
		.long	in_cksum_8

/
/ in_cksum_vec3 is the jump table used to find the appropriate end of message
/ block handler.  We use the equation %ecx % 4 as an index into in_cksum_vec3.
/ Each end of message block handler checksums the 3 less than 4 byte cases.
/ Case 0 is special since it means there's nothing more to do.  In which case,
/ we just head back up to in_cksum_0.  The other 3 cases will meet us there
/ later.
/
in_cksum_vec3:	.long	in_cksum_0
		.long	in_cksum_41
		.long	in_cksum_42
		.long	in_cksum_43
