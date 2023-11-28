mandelbrot:
		push	es

		mov 	ax,#0x13
		int 	0x10

		mov 	ax,#0xa000
		mov 	es,ax
		xor 	di,di

		! display x=[-2.5,2.5], y=[-2.083 = -2.5*4/3*200/320,2.083]
		mov 	eax,#0xFFFDEAAB ! -2.083...
y_loop: mov 	ebx,#0xFFFD8000 ! -2.5
x_loop: call	calc
		add 	ebx,#1024		! 5/320
		cmp 	ebx,#0x28000	! 1024*320-0x28000
		jl		x_loop
		add 	eax,#1365		! 4.167/200
		cmp 	di,#64000		! 320*200
		jb		y_loop

		call	cycle

		xor 	ax,ax
		int 	0x16

		mov 	ax,#3
		int 	0x10

		pop	es

		ret

!	 // iterate x(t+1) = x(t) * x(t) + C
!	 y.r = x.r*x.r - x.i*x.i + C.r!
!	 y.i = 2*x.r*x.i + C.i!
!
!	 x.r = y.r! x.i = y.i!
!
!	 if (x.r*x.r + x.i*x.i > 100*100)
!	   break!

! C.r = ebx
! C.i = eax

!16.16 * 16.16 = 32.32

calc:	xor 	bp,bp	! reset iteration counter
		xor 	ecx,ecx
		xor 	edx,edx ! x.r = edx, x.i = ecx
iterate:push	eax
		push	ebx

		push	ecx
		push	edx

		xchg	eax,edx ! eax = x.r
		call	_imul_	! edx = x.r*x.r
		xchg	ebx,eax ! ebx = x.r*x.r
		xchg	eax,ecx ! eax = x.i
		call	_imul_	! edx = x.i*x.i
		neg 	eax
		add 	ebx,eax ! ebx = x.r*x.r - x.i*x.i
		mov 	ecx,ebx ! ecx = x.r*x.r - x.i*x.i

		pop 	eax 	! eax = x.r
		pop 	edx 	! edx = x.i
		imul	edx
		shr 	eax,#16
		shl 	edx,#16
		or		edx,eax
		shl 	edx,#1	 ! edx = 2*x.r*x.i

		xchg	ecx,edx

		pop 	ebx
		pop 	eax

		add 	edx,ebx ! edx = x.r*x.r - x.i*x.i + C.r
		add 	ecx,eax	! ecx = 2*x.r*x.i + C.i

		push	eax
		push	ebx
		push	ecx
		push	edx

		xchg	eax,edx
		imul	eax 	! edx:eax = x.r*x.r
		xchg	esi,edx ! edx = x.r*x.r
		xchg	eax,ecx
		imul	eax 	! edx:eax = x.i*x.i
		add 	esi,edx	! esi = x.r*x.r + x.i*x.i
		pop 	edx
		pop 	ecx
		pop 	ebx
		pop 	eax
		jc		calced
		cmp 	esi,#10000					! 100*100
		jg		calced

		inc 	bp
		cmp 	bp,#0xff
		jb		iterate
calced:	xchg	ax,bp
		stosb
		xchg	ax,bp

		ret

_imul_:	imul	eax 						! edx:eax = x.r*x.r
		shr 	eax,#16
		shl 	edx,#16
		or		eax,edx
		ret

cycle:	push	ds
		push	es
		pop 	ds

		mov 	dx,#0x3c7
		mov 	al,#0
		out 	dx,al
		inc 	dx
		inc 	dx
		mov 	cx,#768
		mov 	di,#64003					! 320*200+3
		rep
		insb

		mov 	bx,#0x100					! counter
cycle_palette:
		mov 	di,#64000					! 320*200
		lea 	si,(di+3)
		mov 	cx,#0x180					! 768/2
		rep
		movsw

		mov 	si,#64000					! 320*200
		movsw
		movsb

		call	wait_retrace

		mov 	dx,#0x3c8
		mov 	al,#0
		out 	dx,al
		inc 	dx
		mov 	cx,#768
!		mov		si,320*200+3
		rep
		outsb

		dec 	bx
		jnz 	cycle_palette

		pop 	ds
		ret

wait_retrace:
		mov 	dx,#0x3da
		in		al,dx
		test	al,#8
		jz		wait_retrace
wait_retrace2:
		in		al,dx
		test	al,#8
		jnz 	wait_retrace2
		ret
