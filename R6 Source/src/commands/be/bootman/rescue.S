		org		0x7c00

		mov		ax, 0x9000		; loader is VERY picky about initial
		mov		ss, ax			; stack
		mov		sp, ax

		jmp		0:_enter

print:
		mov		ah, 0x0e
		xor		bx, bx
print_loop:
		lodsb
		or		al, al
		jz		printed
		int		0x10
		jmp		short print_loop

printed:
		ret


intro:		db		"Be Boot Manager rescue disk", 13, 10
			db		"Would you like to restore the MBR of your boot hard drive", 13, 10
			db		"with this disk? "
zero:		db		0

badsigmsg:	db		"Rescue disk data is corrupted."
crlf:		db		13, 10, 0
no:			db		"MBR *NOT* restored.", 13, 10, 0
success:	db		"MBR successfully written to the boot hard drive.", 13, 10, 0
readerrmsg:	db		"Too many errors reading MBR data from floppy.", 0
writeerror:	db		'Too many errors writing MBR to hard drive.', 13, 10, 0
reboot:		db		"Press any key to reboot", 13, 10, 0

_enter:
		cld
		push	cs
		push	cs
		pop		ds
		pop		es

		mov		si, intro
		call	print

		xor		ax,ax
		int		0x16

		push	ax
		mov		ah, 0x0e
		xor		bx, bx
		int		0x10
		mov		si,  crlf
		call	print
		pop		ax

		cmp		al, 'y'
		jz		y
		cmp		al, 'Y'
		jz		y

		mov		si, zero

error:
		push	si
		mov		si, no
		call	print
		pop		si
completed:
		call	print

		mov		si, reboot
		call	print

		xor		ax, ax
		int		0x16

		int		0x19

y:
		mov		di, 5
read_loop:
		mov		ax, 0x204
		mov		bx, 0x7e00
		mov		cx, 2
		xor		dx, dx
		int		0x13
		jnc		read_completed

		xor		ax, ax
		int		0x13

		dec		di
		jnz		read_loop

		mov		si, readerrmsg
		jmp		short error

read_completed:
		mov		ax, [0x7ffe]
		mov		si, badsigmsg
		cmp		ax, 0xaa55
		jnz		error

		mov		si, success
		mov		di, 5
write_loop:
		mov		ax, 0x304
		mov		bx, 0x7e00
		mov		cx, 1
		mov		dx, 0x80
		int		0x13
		jnc		completed

		xor		ax, ax
		int		0x13

		dec		di
		jnz		write_loop

		mov		si, writeerror
		jmp		short error

		times	(0x7dfe - 0x7c00) - ($ - $$) db 0
		dw		0xaa55
