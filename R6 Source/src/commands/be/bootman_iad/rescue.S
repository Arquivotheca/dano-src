	.org	0x7c00

.globl	_main
_main:
	mov		ax,#0x9000		! loader is VERY picky about initial
	mov		ss,ax			! stack
	mov		sp,ax

	.byte	0xea
	.word	_enter, 0

print:
	mov		ah,#0x0e
	xor		bx,bx
print_loop:
	lodsb
	or		al, al
	jz		printed
	int		0x10
	jmp		print_loop

printed:
	ret


intro:		.ascii	"Be Boot Manager rescue disk\r\n"
			.ascii	"Would you like to restore the MBR of your boot hard drive\r\n"
			.ascii	"with this disk? "
zero:		.byte	0

badsigmsg:	.ascii	"Rescue disk data is corrupted."
crlf:		.ascii	"\r\n\0"
no:			.ascii	"MBR *NOT* restored.\r\n\0"
success:	.ascii	"MBR successfully written to the boot hard drive.\r\n\0"
readerrmsg:	.ascii	"Too many errors reading MBR data from floppy.\0"
writeerror:	.ascii	"Too many errors writing MBR to hard drive.\r\n\0"
reboot:		.ascii	"Press any key to reboot\r\n\0"

_enter:
	cld
	push	cs
	push	cs
	pop		ds
	pop		es

	mov		si, #intro
	call	print

	xor		ax,ax
	int		0x16

	push	ax
	mov		ah,#0x0e
	xor		bx,bx
	int		0x10
	mov		si, #crlf
	call	print
	pop		ax

	cmp		al,#0x79
	jz		y
	cmp		al,#0x59
	jz		y

	mov		si,#zero

error:
	push	si
	mov		si, #no
	call	print
	pop		si
completed:
	call	print

	mov		si,#reboot
	call	print

	xor		ax,ax
	int		0x16

	int		0x19

y:
	mov		di,#5
read_loop:
	mov		ax,#0x204
	mov		bx,#0x7e00
	mov		cx,#2
	xor		dx,dx
	int		0x13
	jnc		read_completed

	xor		ax,ax
	int		0x13

	dec		di
	jnz		read_loop

	mov		si,#readerrmsg
	jmp		error

read_completed:
	mov		ax,0x7ffe
	mov		si,#badsigmsg
	cmp		ax,#0xaa55
	jnz		error

	mov		si,#success
	mov		di,#5
write_loop:
	mov		ax,#0x304
	mov		bx,#0x7e00
	mov		cx,#1
	mov		dx,#0x80
	int		0x13
	jnc		completed

	xor		ax,ax
	int		0x13

	dec		di
	jnz		write_loop

	mov		si,#writeerror
	jmp		error

	.org	0x7dfe
	.word	0xaa55
