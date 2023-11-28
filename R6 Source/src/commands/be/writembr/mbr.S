		org		0

		xor		ax, ax
		mov		ss, ax
		mov		sp, 0x7c00

		cld
		push	ax
		pop		ds
		push	ds
		pop		es

		mov		si, sp
		mov		di, 0x7e00
		mov		cx, 0x100
		cld
		rep
		movsw
		db		0xe9
		dw		0x200

		mov		si, 0x7fbe
		mov		cx, 4
find_active:
		mov		dl, [si]
		test	dl, 0x80
		jnz		found_active

		add		si, byte 0x10
		loop	find_active

		mov		si, 0x7e00 + noactive
		jmp		short error

found_active:
		mov		eax, [si+8]
		mov		bx, 0x7c00

/*
on entry:
   es:bx -> buffer
   eax = sector #
   dl = drive
*/
read_lba_sector:
		push	es
		push	bx

#if 0
		.byte	0x66, 0x05
		.long	0x01020304		/* start of partition marker */
#endif

		mov		dword [bx+0x200+blocknum], eax

		push	es
		push	bx
		push	dx

		push	eax
	
		mov		ah, 8
		int		0x13		/* get drive parameters in cx, dx
							   ch = low 8 bits of cylinders
							   cl = max sector + upper 2 bits of cyl
							   dh = max head number
							   trashes ax, cx, dx, bx, di, es */
		mov		al, cl
		xchg	ch, cl
		rol		ch, 2
		and		ch, 3
		mov		bp, cx
		and		ax, 0x3f	; sectors
		mov		bx, ax		; squirrel it away
		mov		dl, 0		; fix head count (keeping in mind strange disks
		xchg	dl, dh		; with 256 heads)
		inc		dx
		mul		dx			; * heads

	; div: ax -> q: al r: ah
	; div: dx:ax -> q: ax, r: dx

		xchg	ax, cx		; sectors * heads
		pop		ax			; fetch LBA
		pop		dx
		jcxz	error		; guard against divide by 0
		; if on bios without ah=42 and requires > 8 gb, tell user
		cmp		dx, cx		; guard against overflow
		jae		use_lba_mode
		div		cx			; ax = cyl, dx = temp
		cmp		ax, bp
		jae		use_lba_mode; ja or jae?
		xchg	ax, cx		; cyl -> cx
		xchg	ax, dx		; temp -> ax
		div		bl			; al = head, ah = sector-1
		xchg	cl, ch
		ror		cl, 2
		or		cl, ah
		inc		cx
/*
	cylinder = LBA / (heads * sectors_per_track)
	temp = LBA % (heads * sectors_per_track)
	head = temp / sectors_per_track
	sector = temp % sectors_per_track + 1

	ch = low 8 bits of cylinder
	cl = sector (0-5) + high 2 bits of cylinder (bits 6-7)
	dh = head 
*/
		pop		dx			; restore the drive number
		mov		dh, al		; patch in the head

		pop		bx			; restore destination
		pop		es

		mov		di, 5
retry:
		mov		ax, 0x201	
		int		0x13
		jnc		read_success

setup_retry:
		xor		ax, ax
		int		0x13

		dec		di
		jnz		retry

error:
		mov		si, 0x7e00 + readerrmsg
print:
		xor		bx, bx
		mov		ah, 0xe
print_loop:
		lodsb
		int		0x10
		cmp		al, 0x2e
		jnz		print_loop

		xchg	ax, bx		; bx = 0 from above
		int		0x16

		int		0x19

use_lba_mode:
		pop		dx			; restore drive number
		pop		ax
		pop		ax
		mov		ah, 0x42
		mov		si, 0x7e00 + parameters
		int		0x13
		jc		error

read_success:
		mov		bx, [0x7dfe]
		mov		si, 0x7e00 + no_bootloader
		cmp		bx, 0xaa55
		jnz		error
	
		retf

parameters:
		dd		0x10010
		dw		0x7c00
		dw		0
blocknum:
		dd		0
		dd		0

noactive:		db	"No active partitions found on the disk."
readerrmsg:		db	"Disk error reading active partition."
no_bootloader:	db	"Boot loader not found on active partition."

		; make sure we do not extend into the partition table
		times	0x1be - ($ - $$) db 0
		db		0

		times	0x1fe - ($ - $$) db 0
		dw		0xaa55
