#if __POWERPC__
void *Xmemcpy(void * dst, const void * src, long len);
void *Xmemset(void * dst, int val, long len);

__asm void *Xmemcpy() {
	cmpli	cr2, 0, r5, 63
	rlwinm	r6, r5, 29, 29, 31
	rlwinm	r7, r5, 0, 29, 29
	cmpli	cr3, 0, r6, 0
	cmpli	cr4, 0, r7, 0
	rlwinm	r8, r5, 0, 30, 30
	rlwinm	r9, r5, 0, 31, 31
	cmpli	cr5, 0, r8, 0
	cmpli	cr6, 0, r9, 0
	subi	r3, r3, 4
	subi	r4, r4, 4
	blt		cr2, noloop
	li		r7, 35
loop:
	dcbt	r7, r4
	dcbz	r7, r3
	subi	r5, r5, 32
	cmpli	cr2, 0, r5, 63
	li		r6, 4
loop2:
	subic.	r6, r6, 1
	lwz		r8, +4(r4)
	lwzu	r9, +8(r4)
	stw 	r8, +4(r3)
	stwu	r9, +8(r3)
	bne		cr0, loop2
	bge		cr2, loop
	rlwinm	r6, r5, 29, 29, 31
	b		loop3
noloop:
	beq		cr3, queue
loop3:
	subic.	r6, r6, 1
	lwz		r9, +4(r4)
	lwzu	r8, +8(r4)
	stw	 	r9, +4(r3)
	stwu	r8, +8(r3)
	bne		cr0, loop3
queue:
	beq		cr4, no4
	lwzu	r9, +4(r4)
	stwu	r9, +4(r3)
no4:
	beq		cr5, no2
	lhz		r9, +4(r4)
	addi	r3, r3, 2
	addi	r4, r4, 2
	sth		r9, +2(r3)
no2:
	beq		cr6, no1
	lbz		r9, +4(r4)
	addi	r3, r3, 1
	stb		r9, +3(r3)
no1:
	addi	r3, r3, 4
	blr
}

__asm void *Xmemset() {
	cmpli	cr2, 0, r5, 31
	subi	r3, r3, 4
	rlwimi	r4, r4, 8, 16, 23
	rlwimi	r4, r4, 16, 0, 15
	blt		cr2, queue
	
	subfic	r9, r3, 28
	rlwinm	r6, r9, 29, 30, 31
	rlwinm	r7, r9, 0, 31, 31
	cmpli	cr7, 0, r6, 0
	cmpli	cr6, 0, r7, 0
	rlwinm	r10, r9, 0, 27, 31
	rlwinm	r8, r9, 0, 30, 30
	rlwinm	r7, r9, 0, 29, 29
	sub		r10, r5, r10
	cmpli	cr5, 0, r8, 0
	cmpli	cr4, 0, r7, 0
	cmpli	cr2, 0, r10, 32
	
	beq		cr6, al1
	subi	r5, r5, 1
	addi	r3, r3, 1
	stb		r4, +3(r3)
al1:
	beq		cr5, al2
	subi	r5, r5, 2
	addi	r3, r3, 2
	sth		r4, +2(r3)
al2:
	beq		cr4, alt4
	subi	r5, r5, 4
	stwu	r4, +4(r3)
alt4:
	beq		cr7, alt8
loop4:
	subic.	r6, r6, 1
	subi	r5, r5, 8
	stw	 	r4, +4(r3)
	stwu	r4, +8(r3)
	bne		cr0, loop4
alt8:
	blt		cr2, queue
	li		r7, 4
loop:
	dcbz	r7, r3
	subi	r5, r5, 32
	cmpli	cr2, 0, r5, 32
	stw 	r4, +4(r3)
	stw 	r4, +8(r3)
	stw 	r4, +12(r3)
	stw 	r4, +16(r3)
	stw 	r4, +20(r3)
	stw 	r4, +24(r3)
	stw 	r4, +28(r3)
	stwu	r4, +32(r3)
	bge		cr2, loop
	
queue:
	rlwinm	r6, r5, 29, 30, 31
	rlwinm	r7, r5, 0, 29, 29
	cmpli	cr3, 0, r6, 0
	cmpli	cr4, 0, r7, 0
	rlwinm	r8, r5, 0, 30, 30
	rlwinm	r9, r5, 0, 31, 31
	cmpli	cr5, 0, r8, 0
	cmpli	cr6, 0, r9, 0
	beq		cr3, no8
loop3:
	subic.	r6, r6, 1
	stw	 	r4, +4(r3)
	stwu	r4, +8(r3)
	bne		cr0, loop3
no8:
	beq		cr4, no4
	stwu	r4, +4(r3)
no4:
	beq		cr5, no2
	addi	r3, r3, 2
	sth		r4, +2(r3)
no2:
	beq		cr6, no1
	addi	r3, r3, 1
	stb		r4, +3(r3)
no1:
	addi	r3, r3, 4
	blr
}

#endif


