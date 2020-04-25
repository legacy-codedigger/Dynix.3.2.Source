/bsr	_stack/{
	N
	/adjspb.*-[48]/d
}
/bsr	_unstack/s//movd	tos,r0/
/bsr	_sp[^a-z]/s//addr	0(sp),r0/
/movd	_xsp,/s//addr	0(sp),/
/movd	\(.*\),_xsp/s//lprd	sp,\1/

/bsr	_Pushframe/s/Pushframe/qpushframe/
/bsr	_Popframe/s/Popframe/qpopframe/
/bsr	_prunei/s/prunei/qprunei/
