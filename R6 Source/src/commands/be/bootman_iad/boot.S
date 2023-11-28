	.org	0x7c00

.globl	_main
_main:
	mov		ax,#0x9000		! loader is VERY picky about initial
	mov		ss,ax			! stack
	mov		sp,ax

	.byte	0xea
	.word	_enter, 0

readerrmsg:	.ascii	"\rToo many read errors.\0"
badsigmsg:	.ascii	"\r"
signature:	.ascii	"Boot Manager"
			.ascii	" signature not found.\0"

#include "mand.S"

_enter:
	cld
	push	cs
	push	cs
	pop		ds
	pop		es

	mov		ax,#0xe42		! 'B'
	int		0x10

	mov		di,#5
read_loop:
	mov		ax,#0x203
	mov		bx,#0x7e00
	mov		cx,#2
	mov		dx,#0x80
	int		0x13
	jnc		read_completed

	xor		ax,ax
	int		0x13

	dec		di
	jnz		read_loop

	mov		si,#readerrmsg
error1:
	xor		bx,bx
	mov		ah,#0xe
print0:
	lodsb
	cmp		al,#0
spin:
	jz		spin
	int		0x10
	jmp		print0

read_completed:
	mov		si,#signature
	mov		di,#signature2
	mov		cx,#12
	rep
	cmpsb
	mov		si,#badsigmsg
	jnz		error1

	mov		ax,#0xe65		! 'e'
	int		0x10

	jmp		second

	/*
	don't forget to leave room for the partition table
	this must not be at or past offset 1be
	*/
	.org	0x7dbe
	.ascii "This space is reserved for the partition table."

	.org	0x7dfe
	.word	0xaa55

DELAY_TIME			=	map
NUMBER_MAP_ENTRIES	=	map+2
CURRENT_MAP_ENTRY	=	map+4
MAP_ENTRIES			=	map+6

BOOTMAN:	.ascii	" "
signature2:	.ascii	"Boot Manager"
crlf:		.ascii	"\r\n\0"

second:
	mov		si,#BOOTMAN
	call	print

	push	dx						! preserve boot device
	call	menu					! on return, si->name

	mov		ax,#3
	int		0x10

	mov		ax,#0x200
	xor		bx,bx
	xor		dx,dx
	int		0x10

	pop		dx

	push	si
	mov		si,#loadingmsg
	call	print
	pop		si

	lodsb
	cbw
	xchg	ax,cx
printname:
	lodsb
	cmp		al,#0xf
	jb		color_data
	mov		ah,#0xe
!	xor		bx,bx
	int		0x10
color_data:
	loop	printname

	lodsb							! get device
	xchg	ax,dx

	xor		ax,ax
	mov		es,ax
	mov		bx,#0x7c00
	push	bx
	lodsd
	call	read_lba_sector

	call	print_crlf

	mov		si,#nobootsignature
	mov		ax,0x7dfe
	cmp		ax,#0xaa55
	jnz		error

	ret

error:
	call	print
	.word	0xfeeb

print_crlf:
	mov		si,#crlf
print:
	lodsb
	or		al,#0
	jz		printed
	mov		ah,#0xe
	xor		bx,bx
	int		0x10
	jmp		print
printed:
	ret

! not general; use_lba_mode will always load to 0:7c00 (easily fixed)
! on entry:
!   es:bx -> buffer
!   eax = sector #
!   dl = drive

! trashes: nothing
read_lba_sector:
	pushad

	mov		blocknum, eax

	push	es
	push	bx
	push	dx

	push	eax
	
	mov		ah,#8
	int		0x13					! get drive parameters in cx, dx
									! ch = low 8 bits of cylinders
									! cl = max sector + upper 2 bits of cyl
									! dh = max head number
									! trashes ax, cx, dx, bx, di, es
	xchg	ch,cl
	mov		bp,cx
	and		bp,#0x3ff
	xchg	ch,cl
	mov		al,cl
	and		ax,#0x3f
	mov		bx,ax					! squirrel it away
	mov		dl,#0
	xchg	dh,dl					! deal properly with dh=0xff
	inc		dx
	mul		dx						! * heads

	! div: ax -> q: al r: ah
	! div: dx:ax -> q: ax, r: dx

	xchg	ax,cx					! sectors * heads
	pop		ax						! fetch LBA
	pop		dx
	jcxz	read_error				! guard against divide by 0
	cmp		dx,cx					! cylinders too large?
	jae		use_lba_mode
	div		cx						! ax = cyl, dx = temp
	cmp		ax,bp
	jae		use_lba_mode
	xchg	ax,cx					! cyl -> cx
	xchg	ax,dx					! temp -> ax
	div		bl						! al = head, ah = sector-1
	xchg	ch,cl
	ror		cl,2
	or		cl,ah
	inc		cx

	! cylinder = LBA / (heads * sectors_per_track)
	! temp = LBA % (heads * sectors_per_track)
	! head = temp / sectors_per_track
	! sector = temp % sectors_per_track + 1

	! ch = low 8 bits of cylinder
	! cl = sector (0-5) + high 2 bits of cylinder (bits 6-7)
	! dh = head 

	pop		dx						! restore the drive number
	mov		dh,al					! patch in the head

	pop		bx						! restore destination
	pop		es

	mov		di,#5
retry:
	mov		ax,#0x201	
	int		0x13
	jnc		bi

	xor		ax,ax
	int		0x13

	dec		di
	jnz		retry

read_error:
	mov		si,#readerrmsg
	call	error				! does not return

use_lba_mode:
	pop		dx
	pop		ax
	pop		ax
	mov		ah,#0x42
	mov		si,#parameters
	int		0x13
	jc		read_error

bi:
	popad
	ret


parameters:
    .long   0x10010
    .word   0x7c00
    .word   0
blocknum:
    .long   0
    .long   0

readerrmsg:			.ascii	"Too many read errors"
period:				.ascii	".\0"
loadingmsg:			.ascii	"Loading \0"
nobootsignature:	.ascii	"Boot sector signature not found.\r\n\0"

#include "menu.S"

#if 0
map_blocks:
! +0    device
! +1    number sectors
! +3    sectors
#endif

map:
