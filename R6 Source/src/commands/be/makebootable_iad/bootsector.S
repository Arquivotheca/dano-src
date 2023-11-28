#if 0

If you make a change to the file, make sure you check in a new version of
bootsector.h (use make obj.i586/bootsector.h to build it).  Something is
wrong with the build and doesnt let us generate the file automatically the
way it ought to be done.

#endif

	.org	0

#define NUM_DIRECT_BLOCKS 12

! pointer to current block run
#define CURRENT_BRUN_ADDR	word ptr (bp+0)
! number of runs remaining at this level
#define BRUNS_REMAINING		word ptr (bp+2)

#define STRUCTURE_SIZE		4

! transfer buffer for lba disk access stored from 0 to 0x0f
! indirection level stored over boot sector code
! no need to fret since it is used but once
#define INDIRECTION_LEVEL	byte ptr (0x10)
#define NAME_END		word ptr (0x11)

.globl	_main
_main:
	mov		ax,#0x9000		! zbeos is very picky about initial
	mov		ss,ax			! stack
	mov		sp,ax

	cld
	push	#0x7c0
	pop		ds
	push	ds
	pop		es

	cmp		dl,#0x02
	jb		drive_okay
	cmp		dl,#0x80
	jb		drive_bad
	cmp		dl,#0x90
	jb		drive_okay
drive_bad:
	mov		dl,#0xff
drive_okay:

!	mov		bp,sp			! set by read_inode for now

	xor		eax,eax			! read superblock to 0x7c0:200
	inc		ax
	mov		bx,#0x200
	call	read_lba_sector

	cmp		dword ptr (bx+0x20),#0x42465331 ! check for boot block signature
!	cmp		word ptr (bx+0x20),#0x5331 ! check for boot block signature
	jnz		goto_error

	cmp		dword ptr (bx+0x1fc),#0x73696c6c ! check for boot block signature
	jnz		goto_error

	mov		si,#0x274
	call	inode_addr_to_block_num
	call	read_inode		! read root dir inode to 0x7c0:400
							! advances bx past inode block
	mov		di,#name
#ifdef ASCIIZ_NAME
	push	di
	mov		cx,di			! this is large enough
	mov		al,#0
	repnz
	scasb
	mov		NAME_END,di
	mov		byte ptr (di-1), #0x2f
	pop		di
#endif

find_zbeos:
	mov		bx,#0x600
	push	di
	call	read_next_block
	jc		goto_error		! quit if no more blocks
	pop		di
	push	bx

	mov		bx, #0x200
find_zbeos_again:
	pop		cx
	push	cx
	add		bx, #0x400
	cmp		bx, cx
	jae		find_zbeos

	inc		dword ptr (bx+0x10)	! check if it is a leaf node
	jnz		find_zbeos_again
	inc		dword ptr (bx+0x14)
	jnz		find_zbeos_again

	mov		al,#0x2f		! '/'
	mov		cx,bx			! should be large enough
	push	di
	repnz
	scasb
	jnz		goto_error
	sub		cx,bx			! calculate length of path component
	not		cx
	pop		di
	pusha
	call	find
	popa
	jc		find_zbeos_again

	mov		bp,ss

	add		di,cx
	inc		di
#ifdef ASCIIZ_NAME
	cmp		di,NAME_END
#else
	cmp	(di),al
!	scasb					! why doesnt this work?
#endif
	jnz		find_zbeos

	mov		ax, #0x1000
	mov		es, ax
	xor		bx, bx
read_low:
	shr		bx,#4
	mov		ax,es
	add		ax,bx
	mov		es,ax

	xor		bx,bx
	call	read_next_block
	jnc		read_low

	mov		eax, dword (partition_start)

	jmpi	0x200, 0x1000	! jump to zbeos

goto_error:
	jmp		error

! on entry:
!   es:bx -> buffer
!   eax = sector #
!   dl = drive

! trashes: nothing
read_lba_sector:
	pushad

	.byte	0x66, 0x05
partition_start:
	.long	0x01020304		! start of partition marker

	pusha
	xor		esi,esi			! build parameter structure
	mov		dword ptr (si),#0x10010	! for lba disk read
	mov		(si+4),bx
	mov		(si+6),es
	mov		(si+8),eax
	mov		(si+0xc),esi

	mov		ah,#0x42
	int		0x13
	popa
	jnc		read_lba_success

	push	es
	push	bx
	push	dx

	push	eax
	
	mov		ah,#8
	int		0x13			! get drive parameters in cx, dx
							! ch = low 8 bits of cylinders
							! cl = max sector + upper 2 bits of cyl
							! dh = max head number
							! trashes ax, cx, dx, bx, di, es
	mov		al,cl
	and		ax,#0x3f		! sectors
! vyt: check already performed below
!	jz		error			! preempt divide by 0
	mov		bx,ax			! squirrel it away
	mov		dl,#0			! fix head count (keeping in mind strange disks
	xchg	dh,dl			! with 256 heads)
	inc		dx
	mul		dx				! * heads

	! div: ax -> q: al r: ah
	! div: dx:ax -> q: ax, r: dx

	xchg	ax,cx			! sectors * heads
	pop		ax				! fetch LBA
	pop		dx
	jcxz	error			! guard against divide by 0
! vyt: test below
	! if on bios without ah=42 and requires > 8 gb, tell user
	cmp		dx,cx			! guard against overflow
	jae		error

	div		cx				! ax = cyl, dx = temp
	xchg	ax,cx			! cyl -> cx
	xchg	ax,dx			! temp -> ax
	div		bl				! al = head, ah = sector-1
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

	pop		dx				! restore the drive number
	mov		dh,al			! patch in the head

	pop		bx				! restore destination
	pop		es

	mov		di,#5
retry:
	mov		ax,#0x201	
	int		0x13
	jc		setup_retry

read_lba_success:
	popad

	ret

setup_retry:
	xor		ax,ax
	int		0x13

	dec		di
	jnz		retry

error:
	mov		si,#readerrmsg
	xor		bx,bx
	mov		ah,#0xe
print:
	lodsb
	int		0x10
	cmp		al,#0x2e
	jnz		print

	xchg	ax,bx			! bx = 0 from above
	int		0x16

	int		0x19

! find routine requires read_inode to clear CF
! the cmp performs this for free
read_inode:
	mov		bp,ss			! #0x9000
	mov		bx,#0x400
	call	read_block
	cmp		dword ptr (di),#0x3bbe0ad9 ! check for inode signature
!	cmp		word ptr (di),#0x0ad9 ! check for inode signature
	jnz		error
	mov		INDIRECTION_LEVEL, #2
	mov		CURRENT_BRUN_ADDR, #0x448
	mov		BRUNS_REMAINING, #NUM_DIRECT_BLOCKS
	ret

/*
On entry, bp should contain a pointer to a block run information structure.
This is set by read_inode, so the external caller should never have to worry
about it.

INDIRECTION_LEVEL measures the levels between the current one and a double
indirect block.  This means direct blocks have a level of 2, single indirect
blocks have a level of 1, and double indirect blocks have a level of 0.  This
value is updated as you travel through the indirect blocks, so the function
reads only when the level is 2.

es:bx -> base buffer to read into
bp -> block run information structure

alter di to point to where data buffer lies (stuff below it will be
indirect blocks)
bp may be altered if indirection level changes
bx is advanced by the number of bytes read (as returned by read_block)
*/

read_next_block0:
	add		CURRENT_BRUN_ADDR, ax	! move to next block run
read_next_block:
	mov		si,CURRENT_BRUN_ADDR
	call	inode_addr_to_block_num
	cmp		word ptr (si), #0
	jnz		read_this_block

	mov		ax,#8
	add		CURRENT_BRUN_ADDR, ax	! move to next block run
	dec		BRUNS_REMAINING		! as long as there are block runs
	jz		change_indirection	! remaining at this indirection level

	or		word ptr (si+8), #0	! any more to read?
	jnz		read_next_block

no_more:
	stc
	ret

change_indirection:
	inc		BRUNS_REMAINING		! if in inode, set BRUNS_REMAINING to 1
								! since there is just one direct or
								! indirect block run
	dec		INDIRECTION_LEVEL	
#ifdef TEST_DBL_INDIRECT
	jnz		__
	int		3
__:
#endif
	jl		no_more

	cmp		bp,#0x9000
	jz		read_next_block0

	sub		bp,#STRUCTURE_SIZE	! go back down a frame
	jmp		read_next_block

read_this_block:
	inc		word ptr (si-2)
	dec		word ptr (si)

	cmp		INDIRECTION_LEVEL, #2
	jz		read_block

	push	es					! save read buffer location
	push	bx

	push	ds					! read next buffer to current block
	pop		es
	! 0x0000 - 0x01ff: this code
	! 0x0200 - 0x03ff : superblock
	! 0x0400 - 0x05ff : inode
	! 0x0600 - 0x25ff : read buffer
	! 0x4000 - 0x5fff : single indirect block buffer
	! 0x6000 - 0x7fff : double indirect block buffer
	! (block size <= 8192)
	lea		bx,(bp+8)			! high bits shifted out
	shl		bx,#11			
	call	read_block			! read in current block

	xchg	ax,bx				! save pointer to end of block

	pop		bx
	pop		es

	add		bp,#STRUCTURE_SIZE	! allocate new frame
	mov		CURRENT_BRUN_ADDR,di! and populate it
	sub		ax,di
	shr		ax,#3
	mov		BRUNS_REMAINING,ax

	inc		INDIRECTION_LEVEL

	jmp		read_next_block

! on entry:
!   es:bx -> buffer
!   eax = block #
!   dl = drive

! on exit:
! trashes eax, cx, si
!   bx += # bytes read
!   di -> start of buffer
read_block:
	mov		si,#0x200
	mov		di,bx
	mov		cl,(si+0x2c)		! get block size shift count
	sub		cl,#9
	shl		eax,cl
	mov		cx,(si+0x28)		! what if block size > 65536?
	shr		cx,#9
read_block_loop:
	call	read_lba_sector
	add		bx,si
	inc		eax
	loop	read_block_loop
	ret

readerrmsg:	.ascii	"Error loading OS; press any key to reboot."

	.org	0x1fe
	.word	0xaa55

	.org	0x2a4

! trashes upper half of cx
inode_addr_to_block_num:
	push	cx
	lodsd					! get allocation group
	mov		cl,0x24c
	shl		eax,cl
	xchg	eax,ecx
	xor		eax,eax
	lodsw					! get starting block
	add		eax,ecx
	pop		cx
	ret

! trashes nearly all registers
find:
	push	dx
	lea		si,(bx+0x18)
	lodsw
	xchg	ax,dx				! dx = # entries
	push	dx
	lodsw						! length
	mov		bx,si				! bx -> name of current entry
	mov		bp,si				! bp -> start of name table
	add		si,ax
	add		si,#7				! offsets are 8-byte aligned
	and		si,#0xfff8
find_loop:
	lodsw						! ax = offset of next entry
	add		ax,bp
	sub		ax,bx				! ax = length of current entry
	cmp		ax,cx
	jnz		wrong_length
	pusha
	mov		si,bx
	rep
	cmpsb
	popa
	jz		found
wrong_length:
	add		bx,ax				! bx->new current entry
	dec		dx
	jnz		find_loop
	pop		ax
	pop		dx
	stc
	ret

found:
	pop		ax					! restore # entries
	sub		ax,dx
	shl		ax,#3
	add		si,ax
	dec		dx
	add		si,dx				! skip past remaining offset fields
	add		si,dx

	pop		dx					! restore drive number
	lodsd						! get block number
	.byte	0xe9				! load zbeos inode to 0x7e0:0x400
	.word	read_inode - _		! uh oh, bad habits rubbing off on me?
_:

name:		.ascii	"beos/system/zbeos//"

		.ascii	"Unida Kyuss Slo-Burn Acid King Orange Goblin "
		.ascii	"Fu Manchu Sleep Black Sabbath Metallica Carcass NIN "
		.ascii	"Floater Banco De Gaia Children of the Bong Cro-Mags "
		.ascii	"Minor Threat Danzig Queensryche Sundial DIO God Machine "
		.ascii	"Verve Drag Pack ProPain Wesley Willis"

