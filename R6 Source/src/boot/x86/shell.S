#define TESTING 0
#define PRINTING 0

/*
 * protected mode shell for zbeos
 *
 * It can run under both raw (no memory manager) and VCPI-controlled
 * environments, but not under DPMI.
 *
 * things to investigate:
 *     o move more code into the first sector
 *     o XMS and DPMI support?
 *
 * scratch segment format:
 *     0x0000 - 0x???? : free for application use
 *     0x???? - 0x8000 : initial protected mode stack
 *     0x???? - 0xe000 : real mode stack
 *     0xe000 - 0xefff : VCPI page table 0
 *     0xf000 - 0xf7ff : idt
 *     0xf800 - 0xffff : isr stubs
 *
 * If VCPI is enabled, all real-mode code will be running in V86 mode instead.
 * We'll run with paging off in protected mode, but turn it on right before
 * switching back to V86 mode since VCPI assumes paging is enabled on entry.
 * The number of entries in the page table will be stored in a long at 0xe004.
 *
 * 32-bit stack lies just below scratch segment
 *
 * Non-local assumptions:
 *     o Code below offset 0x200 may be clobbered by pmtorm.
 *
 * as86 quirks to watch out for:
 *     o push/pop segment registers work as expected
 *       only in 16 bit mode; in 32 bit mode, as86 adds
 *       a 0x66 prefix.
 */

/* number of sectors to read; grow as needed */
#define SECTORS 200

		org		0

		jmp		short avoid_bad_bios
		nop

vcpi_init:

vcpi_patch1:
		clc
		jnc		vcpi_done

		/* initialize VCPI protected mode interface */
		push	ebx
		mov		ax, 0xde01
		mov		si, vcpi_cs	/* ds:si -> vcpi descriptors */
		mov		di, 0xe000		/* es:di -> page table 0 */
		int		0x67
		mov		[vcpi_entry], ebx
		pop		ebx
		or		ah, ah
		jnz		vcpi_error
		
		/* di points to the first invalid pte. */
		sub		di, 0xe000
		shr		di, 2			/* convert to index */
		movzx	ecx, di

		/* pde 0 -> self */
		lea		eax, [ebp+0xe003]
		mov		di, 0xe000
		stosd
		xchg	eax, ecx
		stosd

		/* enable other VCPI code */
		inc		byte [vcpi_patch2]
		inc		byte [vcpi_patch3]

		/* since we're going to allocate all the available memory pages
		 * later, reserve 32k so that any other memory managers will have
		 * memory to play with if they need it */
		mov		cx, 8
allocate_pages:
		mov		ax, 0xde04
		int		0x67
		or		ah, ah
		jnz		vcpi_error
		push	edx
		loop	allocate_pages

		call	vcpi_find_lowest

		/* tell C code that we are running under VCPI */
		mov		dword [dos_patch], ecx

		mov		cx, 8
free_pages:
		pop		edx
		mov		ax, 0xde05
		int		0x67
		or		ah, ah
		jnz		vcpi_error
		loop	free_pages

vcpi_done:
		ret

vcpi_error:
		mov		si, vcpierrmsg
		call	print
		mov		ah, 0x4c
		int		0x21

avoid_bad_bios:
		call	avoid_bad_bios + 3
		pop		ax

		/* check ip to see if this was loaded as a COM file */
		cmp		ax, avoid_bad_bios + 0x103
		jnz		boot

		/* DOS entry point */

		mov		bx, 0xb800
		mov		es, bx
		mov		ah, 0xfe		/* check for RSIS */
		xor		di, di
		int		0x10

		mov		ax, es
		cmp		ax, bx
		jz		no_rsis

		/* We are running under a RSIS environment such as Japanese Windows. We
		 * call the option ROM initialization routine to reset the interrupt
		 * vector back to its unadulterated state and use it to set the video
		 * mode. We can't just use the ISR address cached by Windows because
		 * it appears to be in a different part of the ROM.
		 */
		pushf
		cli
		push	cs
		call	0xc000:3
		popf

		mov		ax, 0x03
		int		0x10

no_rsis:
		inc		dword [dos_patch+0x100]

		/* look for EMM */
		push	ds
		xor		ax, ax
		mov		ds, ax
		mov		ax, [0x19e]					/* 0x67*4 + 2 */
		mov		ds, ax
		cmp		dword [0x0e], 0x30585858	/* '0XXX' */
		jnz		no_emm
		cmp		dword [0x0a], 0x584d4d45	/* 'XMME' */
		jz		no_emm						/* actually, yes_emm */
		cmp		dword [0x0a], 0x514d4d45	/* 'QMME' */
no_emm:
		pop		ds
		jnz		no_vcpi

		/* allocate a page via VCPI */
		mov		ah, 0x43
		mov		bx, 0x01
		int		0x67
		or		ah, ah
		jnz		no_vcpi

		/* look for VCPI */
		mov		ax, 0xde00
		int		0x67
		or		ah, ah
		jnz		no_vcpi

		/* VCPI is in the house */
		inc		byte [vcpi_patch1+0x100]

no_vcpi:
		mov		ax, [2]
		xor		dx, dx
		xor		ecx, ecx
		jmp		main

		/* floppy boot sector init code */
boot:	xor		cx, cx
		mov		ss, cx		/* cx = 0 */
		mov		sp, 0x7c00
		mov		ds, cx

		mov		si, 0x7c00 + loading
		call	print

		/* There are two versions of the floppy loader here. The first reads
		 * entire tracks at a time; the second reads one sector at a time.
		 * Surprisingly, the first version is only marginally faster than the
		 * second. The second is smaller than the first by about 6 bytes.
		 * The first is more desirable because of bugs in extended reads on
		 * emulated floppy images and bootable CD-ROMs.
		 */
#if 1
		/* get drive parameters. trashes ax, cx, dx, bx, di, es */
		push	dx
		mov		ah, 8
		int		0x13

		/* Check for bad BIOS geometry from Acer laptop (see bios.c) */
		cmp		cx, 0x5012
		jnz		not_acer
		cmp		dh, 2
		jnz		not_acer
/*		mov		ch, 0x4f	*/	/* we don't use this value here anyway */
		mov		dh, 1
not_acer:
		mov		[maxhead+0x7bff], dh
		pop		dx

		push	ds
		pop		es

		/* setup initial parameters for the loop */
		and		cx, byte 0x3f	/* get number of sectors */
		mov		si, cx
		mov		di, SECTORS
		mov		bx, 0x7e00
		mov		cx, 2
		mov		dh, 0

		/* read entire tracks at a time */
load:	mov		ax, di		/* calculate sectors to read */
		cmp		ax, si
		jge		above
		add		al, cl
		dec		ax
		cmp		ax, si
		jbe		below
above:	mov		ax, si
below:	inc		ax
		sub		al, cl

		/* check that we don't span a 64k boundary */
		mov		bp, ax
		shl		bp, 9
		add		bp, bx
		jnc		nospan
		xor		ax, ax
		sub		ax, bx
		shr		ax, 9
nospan:

		mov		ah, 2
		mov		bp, 6		/* retries */
retry:	pushad
		int		0x13
		jnc		readok
		
		xor		ax, ax
		int		0x13

		popad
		dec		bp
		jnz		retry

error:
		mov		si, 0x7c00 + readerrmsg
		call	print
		xchg	ax, bx			/* ASSUMPTION: bx = 0 from above */
		int		0x16

		int		0x19
		
readok:
		mov		si, 0x7c00 + dot
		call	print

		popad
		cbw

		add		cx, ax
		sub		di, ax
		
		shl		ax, 9
		add		bx, ax
		jnz		check_sectors
		
		mov		ax, es
		add		ax, 0x1000
		mov		es, ax

check_sectors:
		mov		ax, si
		cmp		cl, al
		jbe		no_more

		sub		cx, si
		inc		dh
check_heads:
		cmp		dh, 1
maxhead:
		jbe		more_heads
		mov		dh, 0
		inc		ch
more_heads:
		jmp		short check_sectors

no_more:
		or		di, di
		jnz		load

		jmp		init

#else
		xor		eax, eax
		mov		cx,  SECTORS
		mov		bx, 0x7c0
load:	add		bx, 0x20
		mov		es, bx
		inc		ax
		pushad
		xor		bx, bx
		call	read_lba_sector
		mov		ax, es
		cmp		al, 0xa0
		jnz		nodot
		mov		si, 0x7c00 + dot
		call	print
nodot:
		popad
		loop	load

		jmp		near init

; on entry:
;   es:bx -> buffer
;   eax = sector #
;   dl = drive
read_lba_sector:
		push	es
		push	bx
		push	dx
	
		push	eax
		
		mov		ah, 8
		int		0x13			; get drive parameters in cx, dx
								; ch = low 8 bits of cylinders
								; cl = max sector + upper 2 bits of cyl
								; dh = max head number
								; trashes ax, cx, dx, bx, di, es

		/* Check for bad BIOS geometry from Acer laptop (see bios.c) */
		cmp		cx, 0x5012
		jnz		not_acer
		cmp		dh, 2
		jnz		not_acer
		mov		ch, 0x4f
		mov		dh, 1
not_acer:
		xchg	ch, cl
		mov		bp, cx
		and		bp, 0x3ff
		xchg	ch, cl
		mov		al, cl
		and		ax, 0x3f		; sectors
		mov		bx, ax			; squirrel it away
		mov		dl, 0			; fix head count (keeping in mind strange disks
		xchg	dh, dl			; with 256 heads)
		inc		dx
		mul		dx				; * heads
	
		; div: ax -> q: al r: ah
		; div: dx:ax -> q: ax, r: dx
	
		xchg	ax, cx			; sectors * heads
		pop		ax				; fetch LBA
		pop		dx
		jcxz	error			; guard against divide by 0
		; if on bios without ah=42 and requires > 8 gb, tell user
		cmp		dx, cx			; guard against overflow
		jae		error
		div		cx				; ax = cyl, dx = temp
		cmp		ax, bp
		jae		error			; ja or jae?
		xchg	ax, cx			; cyl -> cx
		xchg	ax, dx			; temp -> ax
		div		bl				; al = head, ah = sector-1
		xchg	ch, cl
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
	
		pop		dx				; restore the drive number
		mov		dh, al			; patch in the head
	
		pop		bx				; restore destination
		pop		es
	
		mov		di, 5
retry:
		mov		ax, 0x201	
		int		0x13
		jc		setup_retry
		ret

setup_retry:
		xor		ax, ax
		int		0x13
	
		dec		di
		jnz		retry

error:
		mov		si, 0x7c00 + readerrmsg
		call	print
		xchg	ax, bx			/* ASSUMPTION: bx = 0 from above */
		int		0x16

		int		0x19

#endif

/* DEPENDENCY: bx = 0 on exit */
print:
		mov		ah, 0x0e
		xor		bx, bx
print_loop:
		lodsb
		int		0x10
		cmp		al, 0x2e
		jnz		print_loop
		ret
		
loading:		db		"Be Boot Loader", 13, 10
				db		"Loading"
dot:			db		"."
readerrmsg:		db		"Too many disk errors."

/* Allocate all the remaining pages of memory through the VCPI interface.
 * Keep track of the lowest returned memory location. This is where the
 * kernel will be loaded. This is kind of flawed, since there could be
 * memory holes, but it will do for now.
 */
vcpi_find_lowest:
		or		ecx, byte -1
		mov		edx, ecx
allocated_page:
		cmp		edx, 0x110000	/* don't allow kernel to load too low */
		jb		not_lowest
		cmp		edx, ecx
		ja		not_lowest
		mov		ecx, edx
not_lowest:
		mov		ax, 0xde04
		int		0x67
		or		ah, ah
		jz		allocated_page
		ret

		times	0x1fe - ($ - $$) db 0
		dw		0xaa55

		bits	16

		/* boot loader entry point. eax = partition start unless cs = 9020 */
		xchg	eax, ecx

		mov		bx, cs
		cmp		bx, 0x9020
		jnz		init

		/* compatibility code for old boot sector: move boot stub down */

		mov		bx, 0xf60		/* 1000 - a0 */
		mov		es, bx
		mov		cx, 0xa00
		xor		si, si
		xor		di, di
		rep
		ss		movsb

		/* mark invalid starting sector */
		or		ecx, byte -1

		jmp		0xf60:init

init:	int		0x12		/* get top of memory, in K */
		shl		ax, 6		/* convert to segment */

/* on entry, ax should be the segment of the top of available conventional 
 * memory, ecx should be the starting sector of the boot partition, and dx
 * should be the boot drive id */
main:
		/* save partition starting sector */
		push	ecx
		/* save drive parameters */
		push	dx

		/* We can't use memory at 9000:0 because it's used to store values
		 * to pass on to the kernel */
		/* vyt: this should be fixed; pass pointers in BIOS data area instead */
		sub		ax, 0x2000	/* - 128k vyt: move back to 64k*/
		and		ax, 0xf000 /* align at 64k boundary for floppy buffer */
		movzx	edx, ax

		xor		eax, eax
		call	pic
pic:	pop		ax
		sub		ax, pic
		shr		ax, 4
		mov		bx, cs
		add		ax, bx
		push	ax
		mov		bx, normalized_eip
		push	bx
		retf

/* relocate offsets used by protected mode routines
 * this loop assumes the relocation amount is less than 0x100000 
 * since it patches gdt entries */
relocate:
		lodsw
		xchg	ax, cx
relocate_l:
		lodsw
		xchg	ax, di
		add		dword [di], ebx
		loop	relocate_l
		ret

normalized_eip:
		cld

		/* initialize FPU */
		finit

		/* load segment */
		mov		ds, ax
		mov		[seg1], ax
		shl		eax, 4		/* ASSUMPTION: high 16 bits clear */
		xchg	ebx, eax

		pop		ax
		mov		[drive_patch], al
		pop		eax
		mov		[sector_patch], eax
		mov		[load_addr], ebx

		mov		si, reloc_load
		call	relocate

		/* scratch segment */

		/* vyt: check for overlap with load segment */

		mov		ss, dx			/* set new real mode stack */
		mov		sp, 0xe000

		mov		es, dx
		mov		[seg2 + 1], dx
		shl		edx, 4
		xchg	ebx, edx
		/* ASSUMPTION: si is already set to #reloc_scratch from above */
		call	relocate
		xchg	ebx, edx

		/* setup idt entries and build isr stubs in scratch segment */
		mov		di, 0xf000			/* idt starts here */
push edx
		add		edx, 0xf800			/* edx->isr stub #0 */
		mov		cx, 0x100
		mov		ebp, ebx			/* call target = ebx + #isr */
		add		ebp, isr - 5		/* call source = edx + 5 */
		sub		ebp, edx
setup_idt:
		mov		ax, dx
		stosw
		mov		eax, 0x8e000008
		stosd
		mov		eax, edx
		shr		eax, 0x10
		stosw
		
		mov		byte [es:di+0x7f8], 0xe8
		mov		dword [es:di+0x7f9], ebp

		mov		ax, 0x8	/* upper bits clear from above */
		add		edx, eax
		sub		ebp, eax

		loop	setup_idt
pop ebp

		call	vcpi_init

		/* reset eflags */
		xor		eax, eax
		push	eax
		popfd

		cli		/* probably unnecessary given above */
		lea		edx, [ebx+pentry]
		jmp		near rmtopm

		bits	32
pentry:
		sti

#if !TESTING
		db		0xb8
dos_patch:
		dd		0		/* 0 = boot, 1 = real mode, 2+ = EMM */
		push	eax
		xor		eax, eax
		db		0xb0	/* ASSUMPTION: upper bits already cleared from above */
drive_patch:
		db		0x80
		push	eax
		db		0xb8
sector_patch:
		dd		0
		push	eax
		push	ebp
		db		0xb8
load_addr:
		dd		0
		push	eax

		call	main1 

		/* don't fixup stack because main1 may have altered it */
#else
		mov		edi, 0xb8008

		/* indicate that we are in protected mode */
		mov		eax, 0x05050404
		stosd

		xor		ax, ax
		int		0x16

#if 0
		/* test preserving registers across int calls */
		xor		ax, ax
		call	print_registers32
		int		0x16
		call	print_registers32
#else
		mov		ax, 0x03
		int		0x10

		mov		ax, 0x201
		mov		bx, 0x1000
		mov		cx, 1
		mov		dx, 0x80
		int		0x13
		jc		bad

		call	print_word32
		inc		edi
		inc		edi
		mov		eax, [gs:0x1000]
		call	print_dword32
bad:

		mov		ax, 0x666
		call	print_word32
#endif
		xor		ax, ax
		int		0x16

		jmp		past

dos_patch:
drive_patch:
sector_patch:
		dd		0
past:

#endif
		/* now fall back to real mode */
		cli
		mov		dx, rentry
		jmp		near pmtorm

		bits	16
rentry:
		sti
		mov		ax, cs
		cmp		ax, 0x7c0
		jz		reboot

		mov		ax, 0x4c00
		int		0x21

reboot:
#if TESTING
		mov		ax, 0xb800
		mov		ds, ax
		mov		ax, 0x0202
		mov		0, ax

		xor		ax, ax
		int		0x16
#endif
		jmp		0xf000:0xfff0

/* saves real mode stack at ss_sp
 * ss:esp restored from ss_esp
 * sets cs = CS32, ds = es = DS32, fs = DS16, gs = SCRATCH16
 * eax and esi destroyed, all others preserved
 * transfers control to CS32:edx
 */
		bits	16
rmtopm:

		push	cs
		pop		ds

		mov		[ss_sp], sp
		mov		[ss_sp+2], ss

vcpi_patch2:
		clc
		jc		v86topm

		lidt	[pidt]
		lgdt	[pgdt]

		/* enter protected mode */
		mov		eax, cr0
		or		al, 1
		mov		cr0, eax

		/* load cs */
		db		0x66, 0xea
patch0:	dd		xtopm
		dw		0x08

v86topm:
		cli
		mov		ax, 0xde0c
		db		0x66, 0xbe			/* esi */
patch2:
		dd		vcpi_frame
		int		0x67

		bits	32
vcpi_pm:
		/* turn off paging */
		mov		eax, cr0
		and		eax, 0x7fffffff
		mov		cr0, eax

		dw		0x00eb

		xor		eax, eax
		mov		cr3, eax

xtopm:
		mov		ax, 0x10
		mov		ds, eax		; YUK
		mov		es, eax		; YUK
		shl		eax, 1		/* ax = DS16 (0x20) */
		mov		fs, eax		; YUK
		add		eax, byte 0x08	/* ax = SCRATCH16 (0x28) */
		mov		gs, eax		; YUK

		lss		esp, [fs:ss_esp]

		jmp		edx

/* saves protected mode mode stack at ss_esp
 * ss:sp restored from ss_sp
 * sets cs = load segment, ds = es = fs = gs = scratch segment
 * eax destroyed, all others preserved
 * transfers control to CS32:dx
 */
		bits	32
pmtorm:
		mov		eax, ss_esp
		mov		[eax], esp
		mov		dword [eax+4], ss

		/* both VCPI and raw code depend on the stack being below 1 MB, so
		 * use the memory at 0x200 as a temporary stack; this is safe since
		 * the code there is not used after this is called */
		mov		ax, 0x20
		mov		ss, eax			; YUK
		mov		esp, 0x200

vcpi_patch3:
		clc
		jc		pmtov86

		/* load all segment registers with 64k limit */
		mov		ax, 0x20
		mov		ds, eax			; YUK
		mov		es, eax			; YUK
		mov		fs, eax			; YUK
		mov		gs, eax			; YUK

		jmp		0x18:$+7

		bits	16

		lidt	[ridt]

		mov		eax, cr0
		dec		ax
		mov		cr0, eax

		db		0xea
		dw		$ + 4
seg1:	dw		0			/* load segment */

seg2:	mov		ax, 0		/* scratch segment */
		mov		ds, ax
		mov		es, ax
		mov		fs, ax
		mov		gs, ax
		lss		sp, [cs:ss_sp]

		jmp		dx
		
		bits	32
pmtov86:
		push	fs
		pop		ds

		/* enable paging */
		mov		eax, [_cr3]
		mov		cr3, eax

		mov		eax, cr0
		or		eax, 0x80000000
		mov		cr0, eax

		dw		0x00eb

		movzx	eax, word [seg2+1]
		push	eax			/* GS */
		push	eax			/* FS */
		push	eax			/* DS */
		push	eax			/* ES */
		movzx	eax, word [ss_sp+2]
		push	eax			/* SS */
		movzx	eax, word [ss_sp]
		push	eax			/* ESP */
		push	eax			/* EFLAGS */
		movzx	eax, word [seg1]
		push	eax			/* CS */
		movzx	eax, dx
		push	eax			/* EIP */

		mov		ax, 0x10
		mov		ds, eax		; YUK

		mov		ax, 0xde0c
		db		0x9a
vcpi_entry:
		dd		0
		dw		0x40

/* used for preserving flags in isr */
#define	MASK1	0x41
#define	MASK2	0xBe

/* this isn't called directly; instead, control is transferred here via a call
 * from the stub routines in the scratch segment.
 */
		bits	32
isr:
		/* bug in as86: segment register push'es encoded with 0x66 prefix */
		push	ds
		push	es
		push	fs
		push	gs

		push	eax
		mov		ax, 0x20
		mov		ds, eax			; YUK
		mov		eax, [esp]
		mov		[_eax], eax
		mov		[_edx], edx
		mov		[_esi], esi

		mov		eax, [esp+0x20]
		and		eax, byte MASK1
		mov		[_eflags], eax

		pop		eax

		mov		eax, [esp+0x10]
		db		0x2d				/* sub eax */
rel0c:	dd		5
		shr		eax, 3
		mov		[intnum], al

		mov		dx, isr_rm
		jmp		near pmtorm

		bits	16
isr_rm:
		pushf
		pop		ax
		and		al, MASK2
		or		al, byte [cs:_eflags]
		push	ax
		popf

		mov		eax, [cs:_eax]
		mov		edx, [cs:_edx]
		mov		esi, [cs:_esi]

		db		0xcd
intnum: db		0

		cli

		push	cs
		pop		ds

		pushf
		pop		word [_eflags]

		mov		[_eax], eax
		mov		[_edx], edx
		mov		[_esi], esi

		mov		edx, isr_pm
patch1:
		jmp		near rmtopm

		bits	32
isr_pm:
		push	fs
		pop		ds

		/* propagate ZF and CF */
		mov		al, [_eflags]
		and		al, MASK1
		and		byte [esp+0x1c], MASK2
		or		byte [esp+0x1c], al

		mov		eax, [_eax]
		mov		edx, [_edx]
		mov		esi, [_esi]

		pop		gs
		pop		fs
		pop		es
		pop		ds

		add		esp, byte 4
		iretd

#if TESTING || PRINTING
		bits	32
print_dword32:
		ror		eax, 0x10
		call	print_word32
		ror		eax, 0x10
print_word32:
		xchg	ah, al
		call	print_byte32
		xchg	ah, al
print_byte32:
		ror		al, 4
		call	print_nibble32
		rol		al, 4
print_nibble32:
		push	eax
		push	ebx
		call	hex32
		db		"0123456789abcdef"
hex32:	pop		ebx
		and		eax, 0x0f
		mov		al, [cs:ebx+eax]
		mov		ah, 2
		stosw
		pop		ebx
		pop		eax
		ret

print_registers32:
		pushad
		pushfd
		push	cs
		push	ds
		push	es
		push	fs
		push	gs
		push	ss
		lea		esi, [esp+0x38]
		mov		ax, 0x10
		mov		fs, ax
		mov		edi, 0xb8000
		mov		ecx, 15
print_reg32:
		std
		seg		ss
		lodsd
		cld
		call	print_dword32
		add		edi, 64	/* (40 - 8) * 2 */
		loop	print_reg32
		pop		ax
		pop		gs
		pop		fs
		pop		es
		pop		ds
		pop		ax
		popfd
		popad
		ret

		bits	16
print_dword16:
		ror		eax, 0x10
		call	print_word16
		ror		eax, 0x10
print_word16:
		xchg	ah, al
		call	print_byte16
		xchg	ah, al
print_byte16:
		ror		al, 4
		call	print_nibble16
		rol		al, 4
print_nibble16:
		push	ax
		push	bx
		call	hex16
		db		"0123456789abcdef"
hex16:	pop		bx
		and		ax, 0x0f
		add		bx, ax
		mov		al, [cs:bx]
		mov		ah, 2
		stosw
		pop		bx
		pop		ax
		ret

print_registers16:
		pushad
		pushfd
		push	cs
		push	ds
		push	es
		push	fs
		push	gs
		push	ss
		mov		si, sp
		add		si, 0x38
		mov		ax, 0xb800
		mov		es, ax
		xor		di, di
		mov		cx, 15
print_reg16:
		std
		seg		ss
		lodsd
		cld
		call	print_dword16
		add		di, 64		/* (40 - 8) * 2 */
		loop	print_reg16
		pop		ax
		pop		gs
		pop		fs
		pop		es
		pop		ds
		pop		ax
		popfd
		popad
		ret
#endif

pgdt:	dw		0x57	/* limit */
		dd		gdt		/* base */

gdt:	dd		0x00000000, 0x00000000	/*  0 NULL descriptor */
		dd		0x0000ffff, 0x00cf9a00	/*  8 CS32 */
		dd		0x0000ffff, 0x00cf9200	/* 10 DS32 */
		dd		0x0000ffff, 0x00009a00	/* 18 CS16 */
		dd		0x0000ffff, 0x00009200	/* 20 DS16 */
		dd		0x0000ffff, 0x00009200	/* 28 SCRATCH16 */
		dd		0x0000ffff, 0x00008200	/* 30 LDT */
		dd		0x0000ffff, 0x00008900	/* 38 TSS */
vcpi_cs:
		dd		0, 0
		dd		0, 0
		dd		0, 0

pidt:	dw		0x7ff
		dd		0xf000

ridt:	dw		0x3ff
		dd		0

ss_esp:	dd		0x8000
		dw		0x10

ss_sp:	dw		0, 0

vcpierrmsg:		db		"Fatal VCPI error."
vcpi_frame:
_cr3:	dd		0xe000	/* CR3 */
v1:		dd		pgdt	/* GDT */
v2:		dd		pidt	/* IDT */
		dw		0x30	/* LDT */
		dw		0x38	/* TR */
v3:		dd		vcpi_pm	/* EIP */
		dw		0x08	/* CS32 */

_eax:	dd		0
_edx:	dd		0
_esi:	dd		0
_eflags:dd		0

reloc_load:
		dw		9			/* number of relocations */
		dw		pgdt + 2	/* and the offsets to patch */
		dw		gdt + 0x1a
		dw		gdt + 0x22
		dw		patch0
		dw		patch1 - 4
		dw		patch2
		dw		v1
		dw		v2
		dw		v3

reloc_scratch:
		dw		5
		dw		gdt + 0x2a
		dw		pidt + 2
		dw		rel0c
		dw		ss_esp
		dw		_cr3

#if !TESTING

		align	4, db 0
main1:

#endif

#if 0
		/* miscellaneous testing bits */
		
		cmp		byte intnum, [cs:0x13]
		jnz		not_132
		call	print_registers16
		pushad
		push	es
		mov		ax, 0xb800
		mov		es, ax
		mov		eax, 0x1000
		mov		edi, 1600
		call	print_dword16
		pop		es
		xor		ax, ax
		int		0x16
		popad
not_132:

		cmp		byte [fs:intnum], 0x13
		jnz		not_133
		call	print_registers32
		pushad
		push	es
		mov		ax, 0x10
		mov		es, ax
		mov		eax, [gs:0x1000]
		mov		edi, 0xb8000
		call	print_dword32
		pop		es
		xor		ax, ax
		int		0x16
		popad
not_133:

#endif
