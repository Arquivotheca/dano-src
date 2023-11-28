; screen layout:
; line 1: blank
; line 2: header
; line 3: blank
; lines 4-22: menu
; line 23: blank
; line 24: footer
; line 25: blank

#define KEY_UNKNOWN	0
#define KEY_UP		0x48
#define KEY_DOWN	0x50
#define KEY_ENTER	0x1c

menu:
		mov 	ax, 0xb800
		mov		es, ax				; keep it here

		mov 	ah, 1
		mov 	cx, 0x2000
		int 	0x10				; disable cursor

		call	clear_screen

		xor		bx, bx				; clear drawing mask

		mov		fs, bx				; init delay vars
		mov		bp, [fs:0x46c]

		mov 	di, 160
		mov 	si, header
		call	draw_centered

		mov 	di, 3680			; 160*23
		mov 	si, footer
		call	draw_centered

draw_loop:
		push	bp
		call	display_menu
		pop		bp
key_loop:
		mov		cx, [DELAY_TIME]
		jcxz	no_delay

		mov		ax, bp
		sub		ax, [fs:0x46c]
		jg		noneg
		neg		ax
noneg:	cmp		ax, cx
		ja		item_selected

no_delay:
		call	get_key
		cmp 	al, KEY_ENTER
		jz		item_selected
; XXX: can shorter instruction
;		or		bx, byte -1
		or		bx, 0xffff
		cmp 	al, KEY_UP
		jz		change_active
		neg 	bx
		cmp 	al, KEY_DOWN
		jnz 	key_loop
change_active:
		and		word [DELAY_TIME], byte 0
		add 	[CURRENT_MAP_ENTRY], bx
		jmp 	short draw_loop

item_selected:
		mov 	ah, 1				; restore cursor
		mov 	cx, 0x0607
		int 	0x10

		mov		si, CURRENT_MAP_ENTRY
		lodsw
		add		si, ax
		add		si, ax
		lodsw
		xchg	si, ax
#if 0
		lodsb
		cbw
		add		si, ax				; si -> map area
#endif
		push	cs
		pop		es

		ret

display_menu:
		mov 	si, NUMBER_MAP_ENTRIES
		lodsw						; get number of entries
		mov 	cx, ax
		neg 	ax
		add 	ax, 25				; 19+6 = line number * 2
		and 	al, 0xfe
		mov 	bl, 80
		mul 	bl
		xchg	ax, di

		lodsw						; get current entry
		or		ax, ax
		jge 	positive
		mov 	ax, cx
		dec 	ax
positive:
		cmp 	ax, cx
		jl		current_ok
		xor 	ax, ax
current_ok:
		mov 	[si-2], ax
		inc 	ax
		mov 	bp, ax

display_menu_items:
		lodsw						; ptr to map
		push	si

		xor 	bx, bx
		dec 	bp
		jnz 	niet
		mov 	bx, 0x0800
niet:
		xchg	ax, si
		call	draw_centered
		pop 	si
		add 	di, 160
		loop	display_menu_items

		ret

; returns KEY_UP, KEY_DOWN, KEY_ENTER, or KEY_UNKNOWN in al
get_key:
		mov 	ah, 1					; check for key
		int 	0x16
		jz		no_key
		xor 	ax, ax					; grab key
		int 	0x16
		cmp		al, 'm'
		jnz		not_mand
		call	mandelbrot
		pop		ax
		db		0xe9
		dw		menu - not_mand

not_mand:
		mov 	al, ah
		cmp 	ah, KEY_UP
		jz		got_key
		cmp 	ah, KEY_DOWN
		jz		got_key
		cmp 	ah, KEY_ENTER
		jz		got_key
		mov 	al, KEY_ENTER
		cmp 	ah, 0x39				; space
		jz		got_key

no_key: mov 	al, KEY_UNKNOWN
got_key:ret

; bx holds options bitmask
;	 bh = or color mask
;	 if bh nonzero, will use > <
; di->start of line
; si->beginning of string
; preserves cx si bp
draw_centered:
		push	cx
		push	si
		push	di

		push	di						; clear existing line
;		xor		ax, ax
		mov		ax, 0x700
		mov 	cx, 80
		rep
		stosw
		pop 	di

		lodsb							; get physical string length
		cbw
		xor 	dx, dx
		xchg	ax, cx
		push	si
calc_len:
		lodsb
		cmp 	al, 16
		jb		skip_ctrl
		inc 	dx
skip_ctrl:
		loop	calc_len
		pop 	si

		mov 	ax, dx
		neg 	ax
		add 	ax, 80
		and 	al, 0xfe
		add 	di,ax

		mov 	cx, dx					; length -> cx

		or		bh, bh
		jz		no_opener
		push	di
		sub 	di, byte 4
		mov 	ax, 0xf10
		stosw
		scasw
		add 	di, cx
		add 	di, cx
		scasw
		inc 	ax
		stosw
		pop 	di
no_opener:

		mov 	ah, 7				   ; default color
		or		ax, bx
draw:	lodsb
		cmp 	al, 15
		ja		not_control
		mov 	ah, al
		or		ax, bx
		jmp 	short draw
not_control:
		stosw
		loop	draw

		pop 	di
		pop 	si
		pop 	cx
		ret

clear_screen:
		mov		ax, 0x700
;		xor		ax, ax
		xor 	di, di
		mov 	cx, 2000				; 80*25
		rep
		stosw
		ret

header:	db		30
		db		9, "B", 12, "e", 15, "OS Partition Manager Menu"

footer:	db		27
		db		7, "Select an OS from the menu"

#if 0

sample map data:

		dw		19		; timeout, in ticks
		dw		3		; number of entries
		dw		0		; current highlight
		dw		offset entry1		; ptr to entry 1...
		dw		offset entry2
		dw		offset entry3

entry1: db		10		; physical string length
		db		1,'B',4,'e',7,'OS R3'
		; map business here....

entry2: db		11
		db		5,'Windows 98'
		; map stuff

entry3: db		12
		db		1,'B',4,'e',7,'OS R3.1'
		; map
#endif
