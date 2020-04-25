.\" $Copyright:	$
.\" Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
.\" Sequent Computer Systems, Inc.   All rights reserved.
.\"  
.\" This software is furnished under a license and may be used
.\" only in accordance with the terms of that license and with the
.\" inclusion of the above copyright notice.   This software may not
.\" be provided or otherwise made available to, or used by, any
.\" other person.  No title to or ownership of the software is
.\" hereby transferred.
...
. \" $Header: c.t 1.16 87/07/29 $
.Ct APPENDIX C "Sample Configuration File Listing"
.Pa
The following configuration file is developed in Section 5;
it is included here for completeness:
.ta 1.0i 1.5i 2.0i
.Ps
#
# B8: starter system
#
machine	balance
cpu	"NS32000"
ident	DYNIX
timezone	8 dst
maxusers	16
options	INET
options TIMESHARE
options	FPU_TRAP_BUG,MMU_MBUG,MMU_EIABUG,FPU_ABTBUG
.Pe
.Ps
## kernels to configure ##
config	dynix_sd	root on sd0a swap on sd0 and sd1
config	gendynix	swap generic
.Pe
.Ps
## MULTIBUS Adapter ##
controller	mbad0 at slot ?
.Pe
.Ps
## SCSI/Ether Controller ##
controller	sec0	at slot ?
.Pe
.Ps
## MULTIBUS terminal multiplexors ##
device	st0	at mbad? csr 0x200 maps  0 bin 4 intr 3
device	st1	at mbad? csr 0x210 maps  0 bin 4 intr 4
.Pe
.Ps
## SCSI disks on SCED ##
device	sd0	at sec? req 4 doneq 4 bin 5 unit 0 target 6
device	sd1	at sec? req 4 doneq 4 bin 5 unit 1 target 6
.Pe
.KS
.Ps
## SCSI tape on SCED
device	ts0	at sec? req 4 doneq 4 bin 5 unit 0 target 4
.Pe
.KE
.Ps
## Ether devices on SCED ##
device	se0	at sec? req 25 doneq 25 bin 6 unit 0	# input
device	se0	at sec? req 10 doneq 10 bin 6 unit 1	# output
.Pe
.Ps
## Console devices on SCED ##
device	co0	at sec? req 4 doneq 4 bin 4 unit 0	# input
device	co0	at sec? req 4 doneq 4 bin 4 unit 1	# output
device	co1	at sec? req 4 doneq 4 bin 4 unit 2	# output
device	co1	at sec? req 4 doneq 4 bin 4 unit 3	# output
.Pe
.Ps
## Battery-backed RAM on SCED ##
device	sm0	at sec? req  3 doneq  3 bin 4 unit 0
.Pe
.Ps
## Pseudo devices ##
pseudo-device	pty
pseudo-device	pci	8		# 8 PCI devices
pseudo-device	pmap			# phys-map driver
.Pe
.Tc
