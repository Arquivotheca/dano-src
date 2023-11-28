	org		0x7c00

	mov		ax, 0x9000		; loader is VERY picky about initial
	mov		ss, ax			; stack
	mov		sp, ax

	jmp		0:_enter

readerrmsg0:db		13, "Too many read errors.", 0
badsigmsg:	db		13, ""
signature:	db		"Boot Manager"
			db		" signature not found.", 0

#include "mand.S"

_enter:
	cld
	push	cs
	push	cs
	pop		ds
	pop		es

	mov		ax, 0xe00 + 'B'
	int		0x10

	mov		di, 5
read_loop:
	mov		ax, 0x203
	mov		bx, 0x7e00
	mov		cx, 2
	mov		dx, 0x80
	int		0x13
	jnc		read_completed

	xor		ax, ax
	int		0x13

	dec		di
	jnz		read_loop

	mov		si, readerrmsg0
error1:
	xor		bx, bx
	mov		ah, 0xe
print0:
	lodsb
	cmp		al, 0
spin:
	jz		spin
	int		0x10
	jmp		short print0

read_completed:
	mov		si, signature
	mov		di, signature2
	mov		cx, 12
	rep
	cmpsb
	mov		si, badsigmsg
	jnz		error1

	mov		ax, 0xe00 + 'e'
	int		0x10

	jmp		short second

	/*
	don't forget to leave room for the partition table
	this must not be at or past offset 1be
	*/
	times	(0x7dbe - 0x7c00) - ($ - $$) db 0
	db		 "This space is reserved for the partition table."

	times	(0x7dfe - 0x7c00) - ($ - $$) db 0
	dw		0xaa55

#define DELAY_TIME			map
#define NUMBER_MAP_ENTRIES	map+2
#define CURRENT_MAP_ENTRY	map+4
#define MAP_ENTRIES			map+6

BOOTMAN:	db		" "
signature2:	db		"Boot Manager"
crlf:		db		13, 10, 0

second:
	mov		si, BOOTMAN
	call	print

	push	dx						; preserve boot device
	call	menu					; on return, si->name

	mov		ax, 3
	int		0x10

	mov		ax, 0x200
	xor		bx, bx
	xor		dx, dx
	int		0x10

	pop		dx

	push	si
	mov		si, loadingmsg
	call	print
	pop		si

	lodsb
	cbw
	xchg	ax, cx
printname:
	lodsb
	cmp		al, 0xf
	jb		color_data
	mov		ah, 0xe
;	xor		bx, bx
	int		0x10
color_data:
	loop	printname

	lodsb							; get device
	xchg	ax, dx

	xor		ax, ax
	mov		es, ax
	mov		bx, 0x7c00
	push	bx
	lodsd
	call	read_lba_sector

	call	print_crlf

	mov		si, nobootsignature
	mov		ax, [0x7dfe]
	cmp		ax, 0xaa55
	jnz		error

	ret

error:
	call	print
	dw		0xfeeb

print_crlf:
	mov		si, crlf
print:
	lodsb
	or		al, 0
	jz		printed
	mov		ah, 0xe
	xor		bx, bx
	int		0x10
	jmp		short print
printed:
	ret

; not general; use_lba_mode will always load to 0:7c00 (easily fixed)
; on entry:
;   es:bx -> buffer
;   eax = sector  
;   dl = drive

; trashes: nothing
read_lba_sector:
	pushad

	mov		[blocknum], eax

	push	es
	push	bx
	push	dx

	push	eax
	
	mov		ah, 8
	int		0x13					; get drive parameters in cx, dx
									; ch = low 8 bits of cylinders
									; cl = max sector + upper 2 bits of cyl
									; dh = max head number
									; trashes ax, cx, dx, bx, di, es
	mov		al, cl
	xchg	ch, cl
	rol		ch, 2
	and		ch, 3
	mov		bp, cx
	and		ax, 0x3f
	mov		bx, ax					; squirrel it away
	mov		dl, 0
	xchg	dl, dh					; deal properly with dh=0xff
	inc		dx
	mul		dx						; * heads

	; div: ax -> q: al r: ah
	; div: dx:ax -> q: ax, r: dx

	xchg	ax, cx					; sectors * heads
	pop		ax						; fetch LBA
	pop		dx
	jcxz	read_error				; guard against divide by 0
	cmp		dx, cx					; cylinders too large?
	jae		use_lba_mode
	div		cx						; ax = cyl, dx = temp
	cmp		ax, bp
	jae		use_lba_mode
	xchg	ax, cx					; cyl -> cx
	xchg	ax, dx					; temp -> ax
	div		bl						; al = head, ah = sector-1
	xchg	cl, ch
	ror		cl, 2
	or		cl, ah
	inc		cx

	; cylinder = LBA / (heads * sectors_per_track)
	; temp = LBA % (heads * sectors_per_track)
	; head = temp / sectors_per_track
	; sector = temp % sectors_per_track + 1

	; ch = low 8 bits of cylinder
	; cl = sector (0-5) + high 2 bits of cylinder (bits 6-7)
	; dh = head 

	pop		dx						; restore the drive number
	mov		dh, al					; patch in the head

	pop		bx						; restore destination
	pop		es

	mov		di, 5
retry:
	mov		ax, 0x201	
	int		0x13
	jnc		bi

	xor		ax, ax
	int		0x13

	dec		di
	jnz		retry

read_error:
	mov		si, readerrmsg
	call	error				; does not return

use_lba_mode:
	pop		dx
	pop		ax
	pop		ax
	mov		ah, 0x42
	mov		si, parameters
	int		0x13
	jc		read_error

bi:
	popad
	ret


parameters:
    dd	   0x10010
    dw	   0x7c00
    dw	   0
blocknum:
    dd	   0
    dd	   0

readerrmsg:			db	"Too many read errors"
period:				db	".", 0
loadingmsg:			db	"Loading ", 0
nobootsignature:	db	"Boot sector signature not found.", 13, 10, 0

#include "menu.S"

#if 0
map_blocks:
; +0    device
; +1    number sectors
; +3    sectors
#endif

map:
