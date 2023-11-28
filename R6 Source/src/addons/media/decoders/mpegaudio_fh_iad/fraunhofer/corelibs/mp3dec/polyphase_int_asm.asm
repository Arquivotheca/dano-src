; Fraunhofer MP3 decoder
;
; void CPolyphaseInt__cost32_s_asm(const int32 *in, int16 *out);
; void CPolyphaseInt__window_band_s_asm16( int bufOffset, int32 *syn_buf, int32 qual, int16 *out_samples );

%include "mx31x16.mac"

pmmtext:		db "mm%i = [ %8x, %8x ] ", 10, 0

EXTERN debugger
EXTERN printf
EXTERN dump_dword
EXTERN dump_2words

%macro DUMP_MMX_LO 1
	pusha
	sub esp, 40
	movq [esp], mm%1
	call dump_dword
	add esp, 40
	popa
%endmacro

%macro DUMP_MMX_2WORDS 1
	pusha
	sub esp, 40
	movd [esp], mm%1
	call dump_2words
	add esp, 40
	popa
%endmacro
	
%macro PRINT_MMX_REG 1
	pusha
	sub esp, 40
	mov eax, pmmtext
	mov [esp],eax
	mov [esp+4], dword %1
	movq [esp+8], mm%1
	call printf
	add esp, 40
	popa
%endmacro

;SECTION .data progbits alloc noexec write align=16
SECTION .data ;progbits alloc noexec write align=16

;;; WINDOW_BAND tables ;;;

syn_f_window_int:
dw 0x0000, 0x0000, 0x0001, 0x0001 
dw 0x0007, 0x0007, 0x001a, 0x001a 
dw 0x0035, 0x0035, 0xffffffdb, 0xffffffdb 
dw 0x0072, 0x0072, 0x0187, 0x0187 
dw 0x01fd, 0x01fd, 0x000b, 0x000b 
dw 0x0508, 0x0508, 0x097f, 0x097f 
dw 0x066b, 0x066b, 0x09bd, 0x09bd 
dw 0x249c, 0x249c, 0x3e84, 0x3e84 
dw 0x4947, 0x4947, 0xffffc17b, 0xffffc17b 
dw 0xffffdb64, 0xffffdb64, 0xfffff642, 0xfffff642 
dw 0x066b, 0x066b, 0xfffff680, 0xfffff680 
dw 0xfffffaf8, 0xfffffaf8, 0xfffffff5, 0xfffffff5 
dw 0x01fd, 0x01fd, 0xfffffe78, 0xfffffe78 
dw 0xffffff8d, 0xffffff8d, 0x0024, 0x0024 
dw 0x0035, 0x0035, 0xffffffe6, 0xffffffe6 
dw 0xfffffff9, 0xfffffff9, 0xffffffff, 0xffffffff 
dw 0x0000, 0x0000, 0x0006, 0x0006 
dw 0x0007, 0x0007, 0x0034, 0x0034 
dw 0x0036, 0x0036, 0x0064, 0x0064 
dw 0x0081, 0x0081, 0x0203, 0x0203 
dw 0x01f4, 0x01f4, 0x04ad, 0x04ad 
dw 0x0563, 0x0563, 0x06f7, 0x06f7 
dw 0x05d1, 0x05d1, 0x22ce, 0x22ce 
dw 0x266a, 0x266a, 0x493c, 0x493c 
dw 0x493c, 0x493c, 0xffffd996, 0xffffd996 
dw 0xffffdd32, 0xffffdd32, 0x05d1, 0x05d1 
dw 0x06f7, 0x06f7, 0xfffffa9d, 0xfffffa9d 
dw 0xfffffb53, 0xfffffb53, 0x01f4, 0x01f4 
dw 0x0203, 0x0203, 0xffffff7e, 0xffffff7e 
dw 0xffffff9c, 0xffffff9c, 0x0036, 0x0036 
dw 0x0034, 0x0034, 0xfffffff8, 0xfffffff8 
dw 0xfffffff9, 0xfffffff9, 0x0000, 0x0000 
dw 0x0000, 0x0000, 0x0006, 0x0006 
dw 0x0008, 0x0008, 0x0032, 0x0032 
dw 0x0037, 0x0037, 0x0056, 0x0056 
dw 0x0091, 0x0091, 0x0208, 0x0208 
dw 0x01e8, 0x01e8, 0x0452, 0x0452 
dw 0x05bd, 0x05bd, 0x0776, 0x0776 
dw 0x052a, 0x052a, 0x20ff, 0x20ff 
dw 0x2836, 0x2836, 0x491a, 0x491a 
dw 0x491a, 0x491a, 0xffffd7ca, 0xffffd7ca 
dw 0xffffdf00, 0xffffdf00, 0x052a, 0x052a 
dw 0x0776, 0x0776, 0xfffffa42, 0xfffffa42 
dw 0xfffffbae, 0xfffffbae, 0x01e8, 0x01e8 
dw 0x0208, 0x0208, 0xffffff6f, 0xffffff6f 
dw 0xffffffa9, 0xffffffa9, 0x0037, 0x0037 
dw 0x0032, 0x0032, 0xfffffff7, 0xfffffff7 
dw 0xfffffffa, 0xfffffffa, 0x0000, 0x0000 
dw 0x0000, 0x0000, 0x0005, 0x0005 
dw 0x0009, 0x0009, 0x0031, 0x0031 
dw 0x0038, 0x0038, 0x0049, 0x0049 
dw 0x00a1, 0x00a1, 0x0209, 0x0209 
dw 0x01d9, 0x01d9, 0x03f7, 0x03f7 
dw 0x0617, 0x0617, 0x07e7, 0x07e7 
dw 0x0474, 0x0474, 0x1f32, 0x1f32 
dw 0x29ff, 0x29ff, 0x48e1, 0x48e1 
dw 0x48e1, 0x48e1, 0xffffd600, 0xffffd600 
dw 0xffffe0cd, 0xffffe0cd, 0x0474, 0x0474 
dw 0x07e7, 0x07e7, 0xfffff9e9, 0xfffff9e9 
dw 0xfffffc08, 0xfffffc08, 0x01d9, 0x01d9 
dw 0x0209, 0x0209, 0xffffff5f, 0xffffff5f 
dw 0xffffffb6, 0xffffffb6, 0x0038, 0x0038 
dw 0x0031, 0x0031, 0xfffffff6, 0xfffffff6 
dw 0xfffffffb, 0xfffffffb, 0x0000, 0x0000 
dw 0x0000, 0x0000, 0x0004, 0x0004 
dw 0x000a, 0x000a, 0x002f, 0x002f 
dw 0x0038, 0x0038, 0x003d, 0x003d 
dw 0x00b1, 0x00b1, 0x0209, 0x0209 
dw 0x01c7, 0x01c7, 0x039e, 0x039e 
dw 0x066f, 0x066f, 0x084b, 0x084b 
dw 0x03b0, 0x03b0, 0x1d68, 0x1d68 
dw 0x2bc5, 0x2bc5, 0x4892, 0x4892 
dw 0x4892, 0x4892, 0xffffd43b, 0xffffd43b 
dw 0xffffe298, 0xffffe298, 0x03b0, 0x03b0 
dw 0x084b, 0x084b, 0xfffff991, 0xfffff991 
dw 0xfffffc62, 0xfffffc62, 0x01c7, 0x01c7 
dw 0x0209, 0x0209, 0xffffff4e, 0xffffff4e 
dw 0xffffffc3, 0xffffffc3, 0x0038, 0x0038 
dw 0x002f, 0x002f, 0xfffffff6, 0xfffffff6 
dw 0xfffffffb, 0xfffffffb, 0x0000, 0x0000 
dw 0x0000, 0x0000, 0x0004, 0x0004 
dw 0x000b, 0x000b, 0x002d, 0x002d 
dw 0x0039, 0x0039, 0x0031, 0x0031 
dw 0x00c2, 0x00c2, 0x0206, 0x0206 
dw 0x01b2, 0x01b2, 0x0345, 0x0345 
dw 0x06c5, 0x06c5, 0x08a2, 0x08a2 
dw 0x02dd, 0x02dd, 0x1ba0, 0x1ba0 
dw 0x2d86, 0x2d86, 0x482d, 0x482d 
dw 0x482d, 0x482d, 0xffffd27a, 0xffffd27a 
dw 0xffffe460, 0xffffe460, 0x02dd, 0x02dd 
dw 0x08a2, 0x08a2, 0xfffff93a, 0xfffff93a 
dw 0xfffffcba, 0xfffffcba, 0x01b2, 0x01b2 
dw 0x0206, 0x0206, 0xffffff3d, 0xffffff3d 
dw 0xffffffcf, 0xffffffcf, 0x0039, 0x0039 
dw 0x002d, 0x002d, 0xfffffff5, 0xfffffff5 
dw 0xfffffffc, 0xfffffffc, 0x0000, 0x0000 
dw 0x0000, 0x0000, 0x0004, 0x0004 
dw 0x000c, 0x000c, 0x002c, 0x002c 
dw 0x0039, 0x0039, 0x0026, 0x0026 
dw 0x00d4, 0x00d4, 0x0202, 0x0202 
dw 0x019b, 0x019b, 0x02ef, 0x02ef 
dw 0x0719, 0x0719, 0x08ec, 0x08ec 
dw 0x01fd, 0x01fd, 0x19dc, 0x19dc 
dw 0x2f41, 0x2f41, 0x47b1, 0x47b1 
dw 0x47b1, 0x47b1, 0xffffd0be, 0xffffd0be 
dw 0xffffe623, 0xffffe623, 0x01fd, 0x01fd 
dw 0x08ec, 0x08ec, 0xfffff8e6, 0xfffff8e6 
dw 0xfffffd11, 0xfffffd11, 0x019b, 0x019b 
dw 0x0202, 0x0202, 0xffffff2c, 0xffffff2c 
dw 0xffffffda, 0xffffffda, 0x0039, 0x0039 
dw 0x002c, 0x002c, 0xfffffff4, 0xfffffff4 
dw 0xfffffffc, 0xfffffffc, 0x0000, 0x0000 
dw 0xffffffff, 0xffffffff, 0x0003, 0x0003 
dw 0x000d, 0x000d, 0x002a, 0x002a 
dw 0x0038, 0x0038, 0x001b, 0x001b 
dw 0x00e5, 0x00e5, 0x01fc, 0x01fc 
dw 0x017f, 0x017f, 0x0299, 0x0299 
dw 0x076b, 0x076b, 0x092b, 0x092b 
dw 0x010e, 0x010e, 0x181d, 0x181d 
dw 0x30f6, 0x30f6, 0x4720, 0x4720 
dw 0x4720, 0x4720, 0xffffcf0a, 0xffffcf0a 
dw 0xffffe7e2, 0xffffe7e2, 0x010e, 0x010e 
dw 0x092b, 0x092b, 0xfffff895, 0xfffff895 
dw 0xfffffd66, 0xfffffd66, 0x017f, 0x017f 
dw 0x01fc, 0x01fc, 0xffffff1a, 0xffffff1a 
dw 0xffffffe4, 0xffffffe4, 0x0038, 0x0038 
dw 0x002a, 0x002a, 0xfffffff3, 0xfffffff3 
dw 0xfffffffc, 0xfffffffc, 0xffffffff, 0xffffffff 
dw 0xffffffff, 0xffffffff, 0x0003, 0x0003 
dw 0x000e, 0x000e, 0x0028, 0x0028 
dw 0x0038, 0x0038, 0x0012, 0x0012 
dw 0x00f7, 0x00f7, 0x01f4, 0x01f4 
dw 0x0161, 0x0161, 0x0246, 0x0246 
dw 0x07b9, 0x07b9, 0x095e, 0x095e 
dw 0x0011, 0x0011, 0x1664, 0x1664 
dw 0x32a3, 0x32a3, 0x467a, 0x467a 
dw 0x467a, 0x467a, 0xffffcd5d, 0xffffcd5d 
dw 0xffffe99c, 0xffffe99c, 0x0011, 0x0011 
dw 0x095e, 0x095e, 0xfffff846, 0xfffff846 
dw 0xfffffdb9, 0xfffffdb9, 0x0161, 0x0161 
dw 0x01f4, 0x01f4, 0xffffff08, 0xffffff08 
dw 0xffffffee, 0xffffffee, 0x0038, 0x0038 
dw 0x0028, 0x0028, 0xfffffff1, 0xfffffff1 
dw 0xfffffffd, 0xfffffffd, 0xffffffff, 0xffffffff 
dw 0xffffffff, 0xffffffff, 0x0002, 0x0002 
dw 0x000f, 0x000f, 0x0026, 0x0026 
dw 0x0037, 0x0037, 0x0009, 0x0009 
dw 0x010a, 0x010a, 0x01ea, 0x01ea 
dw 0x0140, 0x0140, 0x01f5, 0x01f5 
dw 0x0804, 0x0804, 0x0985, 0x0985 
dw 0xffffff06, 0xffffff06, 0x14b1, 0x14b1 
dw 0x3447, 0x3447, 0x45bf, 0x45bf 
dw 0x45bf, 0x45bf, 0xffffcbb8, 0xffffcbb8 
dw 0xffffeb4f, 0xffffeb4f, 0xffffff06, 0xffffff06 
dw 0x0985, 0x0985, 0xfffff7fc, 0xfffff7fc 
dw 0xfffffe0a, 0xfffffe0a, 0x0140, 0x0140 
dw 0x01ea, 0x01ea, 0xfffffef6, 0xfffffef6 
dw 0xfffffff7, 0xfffffff7, 0x0037, 0x0037 
dw 0x0026, 0x0026, 0xfffffff0, 0xfffffff0 
dw 0xfffffffd, 0xfffffffd, 0xffffffff, 0xffffffff 
dw 0xffffffff, 0xffffffff, 0x0002, 0x0002 
dw 0x0011, 0x0011, 0x0024, 0x0024 
dw 0x0035, 0x0035, 0x0000, 0x0000 
dw 0x011c, 0x011c, 0x01df, 0x01df 
dw 0x011a, 0x011a, 0x01a7, 0x01a7 
dw 0x084a, 0x084a, 0x09a1, 0x09a1 
dw 0xfffffded, 0xfffffded, 0x1305, 0x1305 
dw 0x35e2, 0x35e2, 0x44ef, 0x44ef 
dw 0x44ef, 0x44ef, 0xffffca1d, 0xffffca1d 
dw 0xffffecfa, 0xffffecfa, 0xfffffded, 0xfffffded 
dw 0x09a1, 0x09a1, 0xfffff7b5, 0xfffff7b5 
dw 0xfffffe59, 0xfffffe59, 0x011a, 0x011a 
dw 0x01df, 0x01df, 0xfffffee4, 0xfffffee4 
dw 0xffffffff, 0xffffffff, 0x0035, 0x0035 
dw 0x0024, 0x0024, 0xffffffef, 0xffffffef 
dw 0xfffffffd, 0xfffffffd, 0xffffffff, 0xffffffff 
dw 0xffffffff, 0xffffffff, 0x0002, 0x0002 
dw 0x0012, 0x0012, 0x0022, 0x0022 
dw 0x0034, 0x0034, 0xfffffff9, 0xfffffff9 
dw 0x012e, 0x012e, 0x01d3, 0x01d3 
dw 0x00f2, 0x00f2, 0x015b, 0x015b 
dw 0x088c, 0x088c, 0x09b3, 0x09b3 
dw 0xfffffcc7, 0xfffffcc7, 0x1161, 0x1161 
dw 0x3772, 0x3772, 0x440b, 0x440b 
dw 0x440b, 0x440b, 0xffffc88d, 0xffffc88d 
dw 0xffffee9e, 0xffffee9e, 0xfffffcc7, 0xfffffcc7 
dw 0x09b3, 0x09b3, 0xfffff773, 0xfffff773 
dw 0xfffffea5, 0xfffffea5, 0x00f2, 0x00f2 
dw 0x01d3, 0x01d3, 0xfffffed1, 0xfffffed1 
dw 0x0007, 0x0007, 0x0034, 0x0034 
dw 0x0022, 0x0022, 0xffffffee, 0xffffffee 
dw 0xfffffffe, 0xfffffffe, 0xffffffff, 0xffffffff 
dw 0xffffffff, 0xffffffff, 0x0002, 0x0002 
dw 0x0013, 0x0013, 0x0021, 0x0021 
dw 0x0032, 0x0032, 0xfffffff2, 0xfffffff2 
dw 0x0140, 0x0140, 0x01c6, 0x01c6 
dw 0x00c6, 0x00c6, 0x0111, 0x0111 
dw 0x08c9, 0x08c9, 0x09bb, 0x09bb 
dw 0xfffffb93, 0xfffffb93, 0x0fc6, 0x0fc6 
dw 0x38f7, 0x38f7, 0x4315, 0x4315 
dw 0x4315, 0x4315, 0xffffc709, 0xffffc709 
dw 0xfffff039, 0xfffff039, 0xfffffb93, 0xfffffb93 
dw 0x09bb, 0x09bb, 0xfffff736, 0xfffff736 
dw 0xfffffeee, 0xfffffeee, 0x00c6, 0x00c6 
dw 0x01c6, 0x01c6, 0xfffffebf, 0xfffffebf 
dw 0x000e, 0x000e, 0x0032, 0x0032 
dw 0x0021, 0x0021, 0xffffffec, 0xffffffec 
dw 0xfffffffe, 0xfffffffe, 0xffffffff, 0xffffffff 
dw 0xffffffff, 0xffffffff, 0x0001, 0x0001 
dw 0x0015, 0x0015, 0x001f, 0x001f 
dw 0x002f, 0x002f, 0xffffffeb, 0xffffffeb 
dw 0x0153, 0x0153, 0x01b7, 0x01b7 
dw 0x0097, 0x0097, 0x00cb, 0x00cb 
dw 0x0900, 0x0900, 0x09b9, 0x09b9 
dw 0xfffffa51, 0xfffffa51, 0x0e35, 0x0e35 
dw 0x3a6f, 0x3a6f, 0x420b, 0x420b 
dw 0x420b, 0x420b, 0xffffc590, 0xffffc590 
dw 0xfffff1cb, 0xfffff1cb, 0xfffffa51, 0xfffffa51 
dw 0x09b9, 0x09b9, 0xfffff6ff, 0xfffff6ff 
dw 0xffffff34, 0xffffff34, 0x0097, 0x0097 
dw 0x01b7, 0x01b7, 0xfffffead, 0xfffffead 
dw 0x0014, 0x0014, 0x002f, 0x002f 
dw 0x001f, 0x001f, 0xffffffeb, 0xffffffeb 
dw 0xfffffffe, 0xfffffffe, 0xffffffff, 0xffffffff 
dw 0xffffffff, 0xffffffff, 0x0001, 0x0001 
dw 0x0016, 0x0016, 0x001d, 0x001d 
dw 0x002c, 0x002c, 0xffffffe5, 0xffffffe5 
dw 0x0165, 0x0165, 0x01a8, 0x01a8 
dw 0x0064, 0x0064, 0x0088, 0x0088 
dw 0x0932, 0x0932, 0x09af, 0x09af 
dw 0xfffff903, 0xfffff903, 0x0cad, 0x0cad 
dw 0x3bda, 0x3bda, 0x40ef, 0x40ef 
dw 0x40ef, 0x40ef, 0xffffc426, 0xffffc426 
dw 0xfffff353, 0xfffff353, 0xfffff903, 0xfffff903 
dw 0x09af, 0x09af, 0xfffff6ce, 0xfffff6ce 
dw 0xffffff78, 0xffffff78, 0x0064, 0x0064 
dw 0x01a8, 0x01a8, 0xfffffe9b, 0xfffffe9b 
dw 0x001a, 0x001a, 0x002c, 0x002c 
dw 0x001d, 0x001d, 0xffffffe9, 0xffffffe9 
dw 0xfffffffe, 0xfffffffe, 0xffffffff, 0xffffffff 
dw 0xffffffff, 0xffffffff, 0x0001, 0x0001 
dw 0x0018, 0x0018, 0x001b, 0x001b 
dw 0x0028, 0x0028, 0xffffffe0, 0xffffffe0 
dw 0x0176, 0x0176, 0x0198, 0x0198 
dw 0x002e, 0x002e, 0x0048, 0x0048 
dw 0x095c, 0x095c, 0x099b, 0x099b 
dw 0xfffff7a9, 0xfffff7a9, 0x0b2f, 0x0b2f 
dw 0x3d37, 0x3d37, 0x3fc2, 0x3fc2 
dw 0x3fc2, 0x3fc2, 0xffffc2c9, 0xffffc2c9 
dw 0xfffff4d0, 0xfffff4d0, 0xfffff7a9, 0xfffff7a9 
dw 0x099b, 0x099b, 0xfffff6a4, 0xfffff6a4 
dw 0xffffffb8, 0xffffffb8, 0x002e, 0x002e 
dw 0x0198, 0x0198, 0xfffffe89, 0xfffffe89 
dw 0x001f, 0x001f, 0x0028, 0x0028 
dw 0x001b, 0x001b, 0xffffffe8, 0xffffffe8 
dw 0xfffffffe, 0xfffffffe, 0xffffffff, 0xffffffff 

;;; COST32 tables ;;;

cost32_c0:
; 0-10 unscaled
dw	0x4013, 0x4013, 0x40b3, 0x40b3, 0x41fa, 0x41fa, 0x43f9, 0x43f9,
dw	0x46cc, 0x46cc, 0x4a9d, 0x4a9d, 0x4fae, 0x4fae, 0x5660, 0x5660,
dw	0x5f4c, 0x5f4c, 0x6b6f, 0x6b6f, 0x7c7d, 0x7c7d,
; 11, 12: 				>> 1
dw	0x4ad8, 0x4ad8, 0x5efc, 0x5efc,
; 13, 14: 				>> 2
dw	0x41d9, 0x41d9, 0x6d0b, 0x6d0b,
; 15:     				>> 4
dw	0x5185, 0x5185,

cost32_c1:
; 0-4 unscaled
dw	0x404f, 0x404f, 0x42e1, 0x42e1, 0x4891, 0x4891, 0x52cb, 0x52cb, 0x64e2,
; 5, 6: 				>> 1
dw	0x43e2, 0x43e2, 0x6e3c, 0x6e3c,
; 7:    				>> 3	
dw	0x519e, 0x519e,

cost32_c2:
; 0-2 unscaled
dw	0x4140, 0x4140, 0x4cf8, 0x4cf8, 0x7332, 0x7332,
; 3:    				>> 2	
dw	0x5203, 0x5203,

cost32_c3:
; 0 unscaled
dw	0x4545, 0x4545,
; 1:    				>> 1
dw	0x539e, 0x539e,

cost32_c4:
dw	0x5a82, 0x5a82

;;; HELPERS ;;;

; used for 31x16 multiply
;
sign_mask:			db 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80

SECTION .text

;void CPolyphaseInt__window_band_s_asm16(    int bufOffset, int16 *syn_buf, int32 qual, int16 *out_samples );
;										    	4                8            12        	16

HAN_SIZE   equ 512

%define STK 16

GLOBAL CPolyphaseInt__window_band_s_asm16
ALIGN 16
CPolyphaseInt__window_band_s_asm16:

		push edi
		push esi
		push ebp
		push ebx

		mov ebx, [esp +4 +STK]		; bufPtr
		mov esi, [esp +8 +STK]		; syn_buf
		mov edx, [esp +16 +STK]		; out
		mov edi, syn_f_window_int

		pxor mm6, mm6				; sum 1
		pxor mm7, mm7				; sum 2
		pxor mm2, mm2
		
		mov ebp, 8
.loop
			movd mm1, [ edi ]						; winPtr[0]
			movd mm0, [ esi + ebx*2 + 32*2 ]		; syn_buf[bufPtr+32]
			pmulhw mm0, mm1
			paddsw mm6, mm0
	
			add ebx, 64
			and ebx, HAN_SIZE*2 -1
	
			movd mm1, [ edi + 8 ]					; winPtr[2]
			movd mm0, [ esi + ebx*2 + 32*2 ]		; syn_buf[bufPtr+32]
			pmulhw mm0, mm1
			paddsw mm6, mm0
	
			movd mm1, [ edi + 12 ]					; winPtr[3]
			movd mm0, [ esi + ebx*2 ]				; syn_buf[bufPtr]
			pmulhw mm0, mm1
			paddsw mm7, mm0
	
			add ebx, 64
			and ebx, HAN_SIZE*2 -1
	
			add edi, 16
			dec ebp
			jnz .loop
		
	;	psllw mm6, 2
	;	psllw mm7, 2
		paddsw mm6, mm6
		paddsw mm7, mm7
		paddsw mm6, mm6
		paddsw mm7, mm7
		paddsw mm6, mm6
		paddsw mm7, mm7

		movd [edx], mm6

		mov ecx, [esp +12 +STK]
		mov eax, 32
		shr eax, cl		
		movd [edx + eax*2], mm7
		

%if 0
pusha
push dword 0
call debugger
pop eax
popa
%endif
		
		mov ebp, 1								; j
.loop2:
			pxor mm6, mm6				; sum 1
			pxor mm7, mm7				; sum 2
	
			mov ebx, [esp +4 +STK]		; bufPtr
			mov eax, 2
			shl eax, cl
			imul eax, ebp
			add ebx, eax				; bufPtr
	
			mov eax, 32 *4
			shl eax, cl
			sub eax, 32 *4
			add edi, eax				; winPtr
			
			mov eax, 8
.loop3:
				movd mm1, [ edi ]						; winPtr[0]
				movd mm0, [ esi + ebx*2 + 32*2 ]		; syn_buf[bufPtr+32]
				pmulhw mm0, mm1
				paddsw mm6, mm0
		
				movd mm1, [ edi + 4 ]					; winPtr[1]
				movd mm0, [ esi + ebx*2 +32*2]			; syn_buf[bufPtr+32]
				pmulhw mm0, mm1
				paddsw mm7, mm0
		
				add ebx, 64
				and ebx, HAN_SIZE*2 -1
		
				movd mm1, [ edi + 8 ]					; winPtr[2]
				movd mm0, [ esi + ebx*2 ]				; syn_buf[bufPtr]
				pmulhw mm0, mm1
				paddsw mm6, mm0
		
				movd mm1, [ edi + 12 ]					; winPtr[3]
				movd mm0, [ esi + ebx*2 ]				; syn_buf[bufPtr]
				pmulhw mm0, mm1
				paddsw mm7, mm0
		
				add ebx, 64
				and ebx, HAN_SIZE*2 -1
		
				add edi, 16
				dec eax
				jnz near .loop3
	
	;		psllw mm6, 2
	;		psllw mm7, 2
			paddsw mm6, mm6
			paddsw mm7, mm7
			paddsw mm6, mm6
			paddsw mm7, mm7
			paddsw mm6, mm6
			paddsw mm7, mm7
	
			movd [edx + ebp*4], mm6
	
			mov eax, 32
			shr eax, cl
			sub eax, ebp
			movd [edx + eax*4], mm7
		
			inc ebp
			mov eax, 16
			shr eax, cl
			cmp eax, ebp
			jne near .loop2
		
		pop ebx
		pop ebp
		pop esi
		pop edi
		emms
		ret
		
		

;void CPolyphaseInt__cost32_s_asm(const int32 *in, int32 *out);
;                                            4          8

COST32_INPUT_DECIMATE EQU	14

; local stack offsets

struc cost32_stack
.begin

; packed t0
.t0_0	resd 1
.t0_1	resd 1
.t0_2	resd 1
.t0_3	resd 1
.t0_4	resd 1
.t0_5	resd 1
.t0_6	resd 1
.t0_7	resd 1
.t0_8	resd 1
.t0_9	resd 1
.t0_10	resd 1
.t0_11	resd 1
.t0_12	resd 1
.t0_13	resd 1
.t0_14	resd 1
.t0_15	resd 1
.t1_0	resd 1
.t1_1	resd 1
.t1_2	resd 1
.t1_3	resd 1
.t1_4	resd 1
.t1_5	resd 1
.t1_6	resd 1
.t1_7	resd 1
.t2_0	resd 1
.t2_1	resd 1
.t2_2	resd 1
.t2_3	resd 1
.t3_0	resd 1
.t3_1	resd 1
.r0_0	resd 1
.r0_1	resd 1
.r0_2	resd 1
.r0_3	resd 1
.r0_4	resd 1
.r0_5	resd 1
.r0_6	resd 1
.r0_7	resd 1
.r0_8	resd 1
.r0_9	resd 1
.r0_10	resd 1
.r0_11	resd 1
.r0_12	resd 1
.r0_13	resd 1
.r0_14	resd 1
.r0_15	resd 1
.r1_0	resd 1
.r1_1	resd 1
.r1_2	resd 1
.r1_3	resd 1
.r1_4	resd 1
.r1_5	resd 1
.r1_6	resd 1
.r1_7	resd 1
.r2_0	resd 1
.r2_1	resd 1
.r2_2	resd 1
.r2_3	resd 1
.r3_0	resd 1
.r3_1	resd 1
.end
endstruc

%macro cost32_load_even_s	3
; prereqs:
;    mm7 == 0
;    ebx -> input
; %1 reg1
; %2 reg2
; %3 index

	movq		mm%1, [ebx +(31-%3)*8]
	movq		mm%2, [ebx +%3 *8]
	paddd		mm%1, mm%2
	psrad		mm%1, COST32_INPUT_DECIMATE
	packssdw	mm%1, mm7
	movd		[esp +cost32_stack.t0_%3], mm%1

%endmacro

;; OPTIMIZE:
;; +++++
;; load coefs into registers

GLOBAL CPolyphaseInt__cost32_s_asm
ALIGN 16
CPolyphaseInt__cost32_s_asm:

	push	edi
	push	esi
	push	ebp
	push	ebx

	sub		esp, cost32_stack_size

	; *** line 1018
	mov		ebx, [esp +4 +16 +cost32_stack.end]	; in  (aka 'vec')
	mov		edi, [esp +8 +16 +cost32_stack.end]	; out (aka 'f_vec')

	;; ***** EVEN TERMS *****
	; decimate, sum, and store pairs of input samples

	pxor	mm7, mm7
	
	; t0
	
	cost32_load_even_s	0, 1, 0
	cost32_load_even_s	0, 1, 1
	cost32_load_even_s	0, 1, 2
	cost32_load_even_s	0, 1, 3
	cost32_load_even_s	0, 1, 4
	cost32_load_even_s	0, 1, 5
	cost32_load_even_s	0, 1, 6
	cost32_load_even_s	0, 1, 7
	cost32_load_even_s	0, 1, 8
	cost32_load_even_s	0, 1, 9
	cost32_load_even_s	0, 1, 10
	cost32_load_even_s	0, 1, 11
	cost32_load_even_s	0, 1, 12
	cost32_load_even_s	0, 1, 13
	cost32_load_even_s	0, 1, 14
	cost32_load_even_s	0, 1, 15

	; t1
	
	movd	mm1, [esp +cost32_stack.t0_0]
	paddsw	mm0, mm1						; t0_15
	movd	[esp +cost32_stack.t1_0], mm0
	movd	mm1, [esp +cost32_stack.t0_1]
	paddsw	mm1, [esp +cost32_stack.t0_14]
	movd	[esp +cost32_stack.t1_1], mm1
	movd	mm2, [esp +cost32_stack.t0_2]
	paddsw	mm2, [esp +cost32_stack.t0_13]
	movd	[esp +cost32_stack.t1_2], mm2
	movd	mm3, [esp +cost32_stack.t0_3]
	paddsw	mm3, [esp +cost32_stack.t0_12]
	movd	[esp +cost32_stack.t1_3], mm3
	movd	mm4, [esp +cost32_stack.t0_4]
	paddsw	mm1, [esp +cost32_stack.t0_11]
	movd	[esp +cost32_stack.t1_4], mm4
	movd	mm5, [esp +cost32_stack.t0_5]
	paddsw	mm5, [esp +cost32_stack.t0_10]
	movd	[esp +cost32_stack.t1_5], mm5
	movd	mm6, [esp +cost32_stack.t0_6]
	paddsw	mm6, [esp +cost32_stack.t0_9]
	movd	[esp +cost32_stack.t1_6], mm6
	movd	mm7, [esp +cost32_stack.t0_7]
	paddsw	mm7, [esp +cost32_stack.t0_8]
	movd	[esp +cost32_stack.t1_7], mm7
	
	; t2
	
	paddsw	mm0, mm7
;;	movd	[esp +cost32_stack.t2_0], mm0
	paddsw	mm1, mm6
;;	movd	[esp +cost32_stack.t2_1], mm1
	paddsw	mm2, mm5
;;	movd	[esp +cost32_stack.t2_2], mm2
	paddsw	mm3, mm4
;;	movd	[esp +cost32_stack.t2_3], mm3

	; t3
	
	movq	mm4, mm0
	paddsw	mm4, mm3
;;	movd	[esp +cost32_stack.t3_0], mm4
	movq	mm5, mm1
	paddsw	mm5, mm2
;;	movd	[esp +cost32_stack.t3_1], mm5
	
	; out(0, 8, 16, 24)
	
	movq	mm6, mm4
	paddsw	mm6, mm5
	movd	[edi +0 *4], mm6
	psubd	mm4, mm5
	pmulhw	mm4, [cost32_c4]
	psllw	mm4, 1
	movd	[edi +16*4], mm4			;f_vec[32]

	psubsw	mm0, mm3
	pmulhw	mm0, [cost32_c3 +0 *4]		;t3_0
	psllw	mm0, 1
	psubsw	mm1, mm2
	pmulhw	mm1, [cost32_c3 +1 *4]
	psllw	mm1, 2						;t3_1
	
	movq	mm2, mm1
	paddsw	mm2, mm0					;r3_0
	psubsw	mm0, mm1
	pmulhw	mm0, [cost32_c4]			;r3_1
	psllw	mm0, 1
	
	movd	[edi +24*4], mm0			;f_vec[48]
	paddsw	mm0, mm2
	movd	[edi +8 *4], mm0			;f_vec[16]
	
	; t2
	
	movd	mm0, [esp +cost32_stack.t1_0]
	psubsw	mm0, [esp +cost32_stack.t1_7]
	pmulhw	mm0, [cost32_c2 +0 *4]			; t2_0
	psllw	mm0, 1
	movd	mm1, [esp +cost32_stack.t1_1]
	psubsw	mm1, [esp +cost32_stack.t1_6]
	pmulhw	mm1, [cost32_c2 +1 *4]
	psllw	mm1, 1
	movd	mm2, [esp +cost32_stack.t1_2]
	psubsw	mm2, [esp +cost32_stack.t1_5]
	pmulhw	mm2, [cost32_c2 +2 *4]
	psllw	mm2, 1
	movd	mm3, [esp +cost32_stack.t1_3]
	psubsw	mm3, [esp +cost32_stack.t1_4]
	pmulhw	mm3, [cost32_c2 +3 *4]
	psllw	mm3, 3

	movq	mm4, mm0
	paddsw	mm4, mm3						; t3_0
	movq	mm5, mm1
	paddsw	mm5, mm2						; t3_1
	movq	mm6, mm5
	
	paddsw	mm6, mm4						; r2_0
	psubsw	mm4, mm5
	pmulhw	mm4, [cost32_c4]				; r2_2
	psllw	mm4, 1

	psubsw	mm0, mm3
	pmulhw	mm0, [cost32_c3 +0 *4]			; t3_0
	psllw	mm0, 1
	psubsw	mm1, mm2
	pmulhw	mm1, [cost32_c3 +1 *4]
	psllw	mm1, 2							; t3_1

	movq	mm2, mm1
	paddsw	mm2, mm0						; r3_0
	psubsw	mm0, mm1
	pmulhw	mm0, [cost32_c4]				; r3_1
	psllw	mm0, 1
	
	paddsw	mm2, mm0						; r2_1
	
	; out(4, 12, 20, 28)
	
	paddsw	mm6, mm2
	movd	[edi +12*4], mm6
	paddsw	mm2, mm4
	movd	[edi +4 *4], mm2
	paddsw	mm4, mm0
	movd	[edi +20*4], mm4
	movd	[edi +28*4], mm0

	; t1
	
	movd	mm0, [esp +cost32_stack.t0_0]
	psubsw	mm0, [esp +cost32_stack.t0_15]
	pmulhw	mm0, [cost32_c1 +0 *4]
	psllw	mm0, 1
	movd	[esp +cost32_stack.t1_0], mm0
	movd	mm1, [esp +cost32_stack.t0_1]
	psubsw	mm1, [esp +cost32_stack.t0_14]
	pmulhw	mm1, [cost32_c1 +1 *4]
	psllw	mm1, 1
	movd	[esp +cost32_stack.t1_1], mm1
	movd	mm2, [esp +cost32_stack.t0_2]
	psubsw	mm2, [esp +cost32_stack.t0_13]
	pmulhw	mm2, [cost32_c1 +2 *4]
	psllw	mm2, 1
	movd	[esp +cost32_stack.t1_2], mm2
	movd	mm3, [esp +cost32_stack.t0_3]
	psubsw	mm3, [esp +cost32_stack.t0_12]
	pmulhw	mm3, [cost32_c1 +3 *4]
	psllw	mm3, 1
	movd	[esp +cost32_stack.t1_3], mm3
	movd	mm4, [esp +cost32_stack.t0_4]
	psubsw	mm4, [esp +cost32_stack.t0_11]
	pmulhw	mm4, [cost32_c1 +4 *4]
	psllw	mm4, 1
	movd	[esp +cost32_stack.t1_4], mm4
	movd	mm5, [esp +cost32_stack.t0_5]
	psubsw	mm5, [esp +cost32_stack.t0_10]
	pmulhw	mm5, [cost32_c1 +5 *4]
	psllw	mm5, 2
	movd	[esp +cost32_stack.t1_5], mm5
	movd	mm6, [esp +cost32_stack.t0_6]
	psubsw	mm6, [esp +cost32_stack.t0_9]
	pmulhw	mm6, [cost32_c1 +6 *4]
	psllw	mm6, 2
	movd	[esp +cost32_stack.t1_6], mm6
	movd	mm7, [esp +cost32_stack.t0_7]
	psubsw	mm7, [esp +cost32_stack.t0_8]
	pmulhw	mm7, [cost32_c1 +7 *4]
	psllw	mm7, 4
	movd	[esp +cost32_stack.t1_7], mm7
	
	; r1
	
	paddsw	mm0, mm7						; t2...
	paddsw	mm1, mm6
	paddsw	mm2, mm5
	paddsw	mm3, mm4

	movq	mm4, mm3						; t3...
	paddsw	mm4, mm0
	movq	mm5, mm2
	paddsw	mm5, mm1

	movq	mm6, mm5
	paddsw	mm6, mm4
	movd	[esp +cost32_stack.r1_0], mm6
	psubsw	mm4, mm5
	pmulhw	mm4, [cost32_c4]
	psllw	mm4, 1
	movd	[esp +cost32_stack.r1_4], mm4
	
	psubsw	mm0, mm3
	pmulhw	mm0, [cost32_c3 +0 *4]			; t3_0
	psllw	mm0, 1
	psubsw	mm1, mm2
	pmulhw	mm1, [cost32_c3 +1 *4]
	psllw	mm1, 2							; t3_1
	
	movq	mm2, mm1
	paddsw	mm2, mm0						; r3_0
	psubsw	mm0, mm1
	pmulhw	mm0, [cost32_c4]				; r3_1
	psllw	mm0, 1

	movd	[esp +cost32_stack.r1_6], mm0
	paddsw	mm0, mm2
	movd	[esp +cost32_stack.r1_2], mm0
	
	; t2
	
	movd	mm0, [esp +cost32_stack.t1_0]
	psubsw	mm0, [esp +cost32_stack.t1_7]
	pmulhw	mm0, [cost32_c2 +0 *4]
	psllw	mm0, 1
	movd	mm1, [esp +cost32_stack.t1_1]
	psubsw	mm1, [esp +cost32_stack.t1_6]
	pmulhw	mm1, [cost32_c2 +1 *4]
	psllw	mm1, 1
	movd	mm2, [esp +cost32_stack.t1_2]
	psubsw	mm2, [esp +cost32_stack.t1_5]
	pmulhw	mm2, [cost32_c2 +2 *4]
	psllw	mm2, 1
	movd	mm3, [esp +cost32_stack.t1_3]
	psubsw	mm3, [esp +cost32_stack.t1_4]
	pmulhw	mm3, [cost32_c2 +3 *4]
	psllw	mm3, 2
	
	movq	mm4, mm0
	paddsw	mm4, mm3						; t3_0
	movq	mm5, mm1
	paddsw	mm5, mm2						; t3_1
	
	movq	mm6, mm4
	paddsw	mm6, mm5						; r2_0
	psubsw	mm4, mm5
	pmulhw	mm4, [cost32_c4]				; r2_2
	psllw	mm4, 1
	
	psubsw	mm0, mm3
	pmulhw	mm0, [cost32_c3 +0 *4]			; t3_0
	psllw	mm0, 1
	psubsw	mm1, mm2
	pmulhw	mm1, [cost32_c3 +1 *4]
	psllw	mm1, 2							; t3_1
	
	movq	mm2, mm0
	paddsw	mm2, mm1
	psubsw	mm0, mm1
	pmulhw	mm0, [cost32_c4]				; r2_3/r1_7	
	psllw	mm0, 1
	paddsw	mm2, mm0						; r2_1


	; r1
	
	paddsw	mm6, mm2						; r1_1
	paddsw	mm2, mm4						; r1_3
	paddsw	mm4, mm0						; r1_5


	; out(2, 6, 10, 14, 18, 22, 26, 30)
	
	movd	[edi +30*4], mm0
	movd	mm1, [esp +cost32_stack.r1_6]
	paddsw	mm0, mm1
	movd	[edi +26*4], mm0
	paddsw	mm1, mm4
	movd	[edi +22*4], mm1
	movd	mm3, [esp +cost32_stack.r1_4]
	paddsw	mm4, mm3
	movd	[edi +18*4], mm4
	paddsw	mm3, mm2
	movd	[edi +2 *4], mm3
	movd	mm1, [esp +cost32_stack.r1_2]
	paddsw	mm2, mm1
	movd	[edi +6 *4], mm2
	paddsw	mm1, mm6
	movd	[edi +10*4], mm1
	movd	mm3, [esp +cost32_stack.r1_0]
	paddsw	mm6, mm3
	movd	[edi +14*4], mm6
	
	;; ***** ODD TERMS *****
	; decimate, subtract, scale, and store pairs of input samples

	pxor	mm7, mm7

	movq		mm6, [cost32_c0 +0 *4]			; load 0, 1
	movq		mm0, [ebx +0 *8]
	movq		mm1, [ebx +31*8]
	psubd		mm0, mm1
	psrad		mm0, COST32_INPUT_DECIMATE
	packssdw	mm0, mm7
	pmulhw		mm0, mm6
	psllw		mm0, 1
	movd		[esp +cost32_stack.t0_0], mm0
	psrlq		mm6, 32							; [1]
	movq		mm1, [ebx +1 *8]
	movq		mm2, [ebx +30*8]
	psubd		mm1, mm2
	psrad		mm1, COST32_INPUT_DECIMATE
	packssdw	mm1, mm7
	pmulhw		mm1, mm6
	psllw		mm1, 1
	movd		[esp +cost32_stack.t0_1], mm1

	movq		mm6, [cost32_c0 +2 *4]			; load 2, 3
	movq		mm0, [ebx +2 *8]
	movq		mm1, [ebx +29*8]
	psubd		mm0, mm1
	psrad		mm0, COST32_INPUT_DECIMATE
	packssdw	mm0, mm7
	pmulhw		mm0, mm6
	psllw		mm0, 1
	movd		[esp +cost32_stack.t0_2], mm0
	psrlq		mm6, 32							; [3]
	packssdw	mm1, mm7
	movq		mm2, [ebx +28*8]
	psubd		mm1, mm2
	movq		mm1, [ebx +3 *8]
	psrad		mm1, COST32_INPUT_DECIMATE
	pmulhw		mm1, mm6
	psllw		mm1, 1
	movd		[esp +cost32_stack.t0_3], mm1

	movq		mm6, [cost32_c0 +4 *4]			; load 4, 5
	movq		mm0, [ebx +4 *8]
	movq		mm1, [ebx +27*8]
	psubd		mm0, mm1
	psrad		mm0, COST32_INPUT_DECIMATE
	packssdw	mm0, mm7
	pmulhw		mm0, mm6
	psllw		mm0, 1
	movd		[esp +cost32_stack.t0_4], mm0
	psrlq		mm6, 32							; [5]
	movq		mm1, [ebx +5 *8]
	movq		mm2, [ebx +26*8]
	psubd		mm1, mm2
	psrad		mm1, COST32_INPUT_DECIMATE
	packssdw	mm1, mm7
	pmulhw		mm1, mm6
	psllw		mm1, 1
	movd		[esp +cost32_stack.t0_5], mm1

	movq		mm6, [cost32_c0 +5 *4]			; load 6, 7
	movq		mm0, [ebx +6 *8]
	movq		mm1, [ebx +25*8]
	psubd		mm0, mm1
	psrad		mm0, COST32_INPUT_DECIMATE
	packssdw	mm0, mm7
	pmulhw		mm0, mm6
	psllw		mm0, 1
	movd		[esp +cost32_stack.t0_6], mm0
	psrlq		mm6, 32							; [7]
	movq		mm1, [ebx +7 *8]
	movq		mm2, [ebx +24*8]
	psubd		mm1, mm2
	psrad		mm1, COST32_INPUT_DECIMATE
	packssdw	mm1, mm7
	pmulhw		mm1, mm6
	psllw		mm1, 1
	movd		[esp +cost32_stack.t0_7], mm1

	movq		mm6, [cost32_c0 +8 *4]			; load 8, 9
	movq		mm0, [ebx +8 *8]
	movq		mm1, [ebx +23*8]
	psubd		mm0, mm1
	psrad		mm0, COST32_INPUT_DECIMATE
	packssdw	mm0, mm7
	pmulhw		mm0, mm6
	psllw		mm0, 1
	movd		[esp +cost32_stack.t0_8], mm0
	psrlq		mm6, 32							; [9]
	movq		mm1, [ebx +9 *8]
	movq		mm2, [ebx +22*8]
	psubd		mm1, mm2
	psrad		mm1, COST32_INPUT_DECIMATE
	packssdw	mm1, mm7
	pmulhw		mm1, mm6
	psllw		mm1, 1
	movd		[esp +cost32_stack.t0_9], mm1

	movq		mm6, [cost32_c0 +10*4]			; load 10, 11
	movq		mm0, [ebx +10*8]
	movq		mm1, [ebx +21*8]
	psubd		mm0, mm1
	psrad		mm0, COST32_INPUT_DECIMATE
	packssdw	mm0, mm7
	pmulhw		mm0, mm6
	psllw		mm0, 1
	movd		[esp +cost32_stack.t0_10], mm0
	psrlq		mm6, 32							; [11]
	movq		mm1, [ebx +11*8]
	movq		mm2, [ebx +20*8]
	psubd		mm1, mm2
	psrad		mm1, COST32_INPUT_DECIMATE
	packssdw	mm1, mm7
	pmulhw		mm1, mm6
	psllw		mm1, 2							; << 1
	movd		[esp +cost32_stack.t0_11], mm1

	movq		mm6, [cost32_c0 +12*4]			; load 12, 13
	movq		mm0, [ebx +12*8]
	movq		mm1, [ebx +19*8]
	psubd		mm0, mm1
	psrad		mm0, COST32_INPUT_DECIMATE
	packssdw	mm0, mm7
	pmulhw		mm0, mm6
	psllw		mm0, 2							; << 1
	movd		[esp +cost32_stack.t0_12], mm0
	psrlq		mm6, 32							; [13]
	movq		mm1, [ebx +13*8]
	movq		mm2, [ebx +18*8]
	psubd		mm1, mm2
	psrad		mm1, COST32_INPUT_DECIMATE
	packssdw	mm1, mm7
	pmulhw		mm1, mm6
	psllw		mm1, 3							; << 2
	movd		[esp +cost32_stack.t0_13], mm1

	movq		mm6, [cost32_c0 +14*4]			; load 14, 15
	movq		mm1, [ebx +14*8]
	movq		mm0, [ebx +17*8]
	psubd		mm1, mm0
	psrad		mm1, COST32_INPUT_DECIMATE
	packssdw	mm1, mm7
	pmulhw		mm1, mm6
	psllw		mm1, 3							; << 2
	movd		[esp +cost32_stack.t0_14], mm1
	psrlq		mm6, 32							; [15]
	movq		mm0, [ebx +15*8]
	movq		mm2, [ebx +16*8]
	psubd		mm0, mm2
	psrad		mm0, COST32_INPUT_DECIMATE
	packssdw	mm0, mm7
	pmulhw		mm0, mm6
	psllw		mm0, 5							; << 4
	movd		[esp +cost32_stack.t0_15], mm0

	; t1
	
	paddsw	mm0, [esp +cost32_stack.t0_0]
	movd	[esp +cost32_stack.t1_0], mm0
	paddsw	mm1, [esp +cost32_stack.t0_1]
	movd	[esp +cost32_stack.t1_1], mm1
	movd	mm2, [esp +cost32_stack.t0_2]
	paddsw	mm2, [esp +cost32_stack.t0_13]
	movd	[esp +cost32_stack.t1_2], mm2
	movd	mm3, [esp +cost32_stack.t0_3]
	paddsw	mm3, [esp +cost32_stack.t0_12]
	movd	[esp +cost32_stack.t1_3], mm3
	movd	mm4, [esp +cost32_stack.t0_4]
	paddsw	mm4, [esp +cost32_stack.t0_11]
	movd	[esp +cost32_stack.t1_4], mm4
	movd	mm5, [esp +cost32_stack.t0_5]
	paddsw	mm5, [esp +cost32_stack.t0_10]
	movd	[esp +cost32_stack.t1_5], mm5
	movd	mm6, [esp +cost32_stack.t0_6]
	paddsw	mm6, [esp +cost32_stack.t0_9]
	movd	[esp +cost32_stack.t1_6], mm6
	movd	mm7, [esp +cost32_stack.t0_7]
	paddsw	mm7, [esp +cost32_stack.t0_8]
	movd	[esp +cost32_stack.t1_7], mm7

	
	; t2
	
	paddsw	mm0, mm7
	paddsw	mm1, mm6
	paddsw	mm2, mm5
	paddsw	mm3, mm4

	; t3
	
	movq	mm4, mm0
	paddsw	mm4, mm3
	movq	mm5, mm1
	paddsw	mm5, mm2
	
	; r0
	
	movq	mm6, mm4
	paddsw	mm6, mm5
	movd	[esp +cost32_stack.r0_0], mm6
	psubsw	mm4, mm5
	pmulhw	mm4, [cost32_c4]
	psllw	mm4, 1
	movd	[esp +cost32_stack.r0_8], mm4

	psubsw	mm0, mm3
	pmulhw	mm0, [cost32_c3 +0 *4]			; t3_0
	psllw	mm0, 1
	psubsw	mm1, mm2
	pmulhw	mm1, [cost32_c3 +1 *4]
	psllw	mm1, 2							; t3_1
	
	movq	mm2, mm0
	paddsw	mm2, mm1						; r3_0
	psubsw	mm0, mm1
	pmulhw	mm0, [cost32_c4]				; r3_1
	psllw	mm0, 1
	movd	[esp +cost32_stack.r0_12], mm0
	paddsw	mm0, mm2
	movd	[esp +cost32_stack.r0_4], mm0
	
	; t2
	
	movd	mm0, [esp +cost32_stack.t1_0]
	psubsw	mm0, [esp +cost32_stack.t1_7]
	pmulhw	mm0, [cost32_c2 +0 *4]
	psllw	mm0, 1
	movd	mm1, [esp +cost32_stack.t1_1]
	psubsw	mm1, [esp +cost32_stack.t1_6]
	pmulhw	mm1, [cost32_c2 +1 *4]
	psllw	mm1, 1
	movd	mm2, [esp +cost32_stack.t1_2]
	psubsw	mm2, [esp +cost32_stack.t1_5]
	pmulhw	mm2, [cost32_c2 +2 *4]
	psllw	mm2, 1
	movd	mm3, [esp +cost32_stack.t1_3]
	psubsw	mm3, [esp +cost32_stack.t1_4]
	pmulhw	mm3, [cost32_c2 +3 *4]
	psllw	mm3, 3							; << 2

	; t3/r2 even
	
	movq	mm4, mm0
	paddsw	mm4, mm3						; t3_0
	movq	mm5, mm1
	paddsw	mm5, mm2						; t3_1
	movq	mm6, mm4
	paddsw	mm6, mm5						; r2_0
	psubsw	mm4, mm5
	pmulhw	mm4, [cost32_c4]				; r2_2
	psllw	mm4, 1
	
	; t3/r2 odd
	
	psubsw	mm0, mm3
	pmulhw	mm0, [cost32_c3 +0 *4]			; t3_0
	psllw	mm0, 1
	psubsw	mm1, mm2
	pmulhw	mm1, [cost32_c3 +1 *4]
	psllw	mm1, 2							; t3_1
	movq	mm2, mm0
	paddsw	mm2, mm1						; r3_0
	psubsw	mm0, mm1
	pmulhw	mm0, [cost32_c4]				; r3_1/r2_3
	psllw	mm0, 1
	movq	mm3, mm0
	paddsw	mm3, mm2						; r2_1
	
	; r0 (2, 6, 10, 14)
	movd	[esp +cost32_stack.r0_14], mm0
	paddsw	mm0, mm4
	movd	[esp +cost32_stack.r0_10], mm0
	paddsw	mm4, mm3
	movd	[esp +cost32_stack.r0_6], mm4
	paddsw	mm3, mm6
	movd	[esp +cost32_stack.r0_2], mm3
	
	
	; t1
	
	movd	mm0, [esp +cost32_stack.t0_0]
	psubsw	mm0, [esp +cost32_stack.t0_15]
	pmulhw	mm0, [cost32_c1 +0 *4]
	psllw	mm0, 1
	movd	[esp +cost32_stack.t1_0], mm0
	movd	mm1, [esp +cost32_stack.t0_1]
	psubsw	mm1, [esp +cost32_stack.t0_14]
	pmulhw	mm1, [cost32_c1 +1 *4]
	psllw	mm1, 1
	movd	[esp +cost32_stack.t1_1], mm1
	movd	mm2, [esp +cost32_stack.t0_2]
	psubsw	mm2, [esp +cost32_stack.t0_13]
	pmulhw	mm2, [cost32_c1 +2 *4]
	psllw	mm2, 1
	movd	[esp +cost32_stack.t1_2], mm2
	movd	mm3, [esp +cost32_stack.t0_3]
	psubsw	mm3, [esp +cost32_stack.t0_12]
	pmulhw	mm3, [cost32_c1 +3 *4]
	psllw	mm3, 1
	movd	[esp +cost32_stack.t1_3], mm3
	movd	mm4, [esp +cost32_stack.t0_4]
	psubsw	mm4, [esp +cost32_stack.t0_11]
	pmulhw	mm4, [cost32_c1 +4 *4]
	psllw	mm4, 1
	movd	[esp +cost32_stack.t1_4], mm4
	movd	mm5, [esp +cost32_stack.t0_5]
	psubsw	mm5, [esp +cost32_stack.t0_10]
	pmulhw	mm5, [cost32_c1 +5 *4]
	psllw	mm5, 2
	movd	[esp +cost32_stack.t1_5], mm5
	movd	mm6, [esp +cost32_stack.t0_6]
	psubsw	mm6, [esp +cost32_stack.t0_9]
	pmulhw	mm6, [cost32_c1 +6 *4]
	psllw	mm6, 2
	movd	[esp +cost32_stack.t1_6], mm6
	movd	mm7, [esp +cost32_stack.t0_7]
	psubsw	mm7, [esp +cost32_stack.t0_8]
	pmulhw	mm7, [cost32_c1 +7 *4]
	psllw	mm7, 4
	movd	[esp +cost32_stack.t1_7], mm7

	; t2
	paddsw	mm0, mm7
	paddsw	mm1, mm6
	paddsw	mm2, mm5
	paddsw	mm3, mm4
	
	; t3
	movq	mm4, mm0
	paddsw	mm4, mm3
	movq	mm5, mm1
	paddsw	mm5, mm2
	
	; r1(0, 4)
	movq	mm6, mm4
	paddsw	mm6, mm5
	movd	[esp +cost32_stack.r1_0], mm6
	psubsw	mm4, mm5
	pmulhw	mm4, [cost32_c4]
	psllw	mm4, 1
	movd	[esp +cost32_stack.r1_4], mm4
	
	; t3
	psubsw	mm0, mm3
	pmulhw	mm0, [cost32_c3 +0 *4]
	psllw	mm0, 1
	psubsw	mm1, mm2
	pmulhw	mm1, [cost32_c3 +1 *4]
	psllw	mm1, 2
	movq	mm2, mm0
	paddsw	mm2, mm1
	psubsw	mm0, mm1
	pmulhw	mm0, [cost32_c4]
	psllw	mm0, 1
	
	; r1(2, 6)
	movd	[esp +cost32_stack.r1_6], mm0
	paddsw	mm0, mm2
	movd	[esp +cost32_stack.r1_2], mm0
	
	; t2
	movd	mm0, [esp +cost32_stack.t1_0]
	psubsw	mm0, [esp +cost32_stack.t1_7]
	pmulhw	mm0, [cost32_c2 +0 *4]
	psllw	mm0, 1
	movd	mm1, [esp +cost32_stack.t1_1]
	psubsw	mm1, [esp +cost32_stack.t1_6]
	pmulhw	mm1, [cost32_c2 +1 *4]
	psllw	mm1, 1
	movd	mm2, [esp +cost32_stack.t1_2]
	psubsw	mm2, [esp +cost32_stack.t1_5]
	pmulhw	mm2, [cost32_c2 +2 *4]
	psllw	mm2, 1
	movd	mm3, [esp +cost32_stack.t1_3]
	psubsw	mm3, [esp +cost32_stack.t1_4]
	pmulhw	mm3, [cost32_c2 +3 *4]
	psllw	mm3, 2
	
	; t3
	movq	mm4, mm0
	paddsw	mm4, mm3
	movq	mm5, mm1
	paddsw	mm5, mm2
	
	; r2
	movq	mm6, mm4
	paddsw	mm6, mm5						; r2_0
	psubsw	mm4, mm5
	pmulhw	mm4, [cost32_c4]				; r2_2
	psllw	mm4, 1
	
	; t3/r3
	psubsw	mm0, mm3
	pmulhw	mm0, [cost32_c3 +0 *4]			; t3_0
	psllw	mm0, 1
	psubsw	mm1, mm2
	pmulhw	mm1, [cost32_c3 +1 *4]
	psllw	mm1, 2							; t3_1
	movq	mm2, mm0
	paddsw	mm2, mm1						; r3_0
	psubsw	mm0, mm1
	pmulhw	mm0, [cost32_c4]				; r3_1/r2_3/r1_7/r0_15
	psllw	mm0, 1
	
	; r2/r1
	paddsw	mm2, mm0						; r2_1
	paddsw	mm6, mm2						; r1_1
	paddsw	mm2, mm4						; r1_3
	paddsw	mm4, mm0						; r1_5
	
	; r0
	movd	mm1, [esp +cost32_stack.r1_0]
	paddsw	mm1, mm6						; r0_1
	movd	mm3, [esp +cost32_stack.r1_2]
	paddsw	mm6, mm3						; r0_3
	paddsw	mm3, mm2						; r0_5
	movd	mm5, [esp +cost32_stack.r1_4]
	paddsw	mm2, mm5						; r0_7
	paddsw	mm5, mm4						; r0_9
	movd	mm7, [esp +cost32_stack.r1_6]
	paddsw	mm4, mm7						; r0_11
	paddsw	mm7, mm0						; r0_13
	
	; out(1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31)
	movd	[edi +31*4], mm0
	paddsw	mm0, [esp +cost32_stack.r0_14]
	movd	[edi +29*4], mm0
	movd	mm0, [esp +cost32_stack.r0_14]
	paddsw	mm0, mm7
	movd	[edi +27*4], mm0
	movd	mm0, [esp +cost32_stack.r0_12]
	paddsw	mm7, mm0
	movd	[edi +25*4], mm7
	paddsw	mm0, mm4
	movd	[edi +23*4], mm0
	movd	mm7, [esp +cost32_stack.r0_10]
	paddsw	mm4, mm7
	movd	[edi +21*4], mm4
	paddsw	mm7, mm5
	movd	[edi +19*4], mm7
	movd	mm0, [esp +cost32_stack.r0_8]
	paddsw	mm5, mm0
	movd	[edi +17*4], mm5
	paddsw	mm0, mm2
	movd	[edi +1 *4], mm0
	movd	mm7, [esp +cost32_stack.r0_6]
	paddsw	mm2, mm7
	movd	[edi +3 *4], mm2
	paddsw	mm7, mm3
	movd	[edi +5 *4], mm7
	movd	mm0, [esp +cost32_stack.r0_4]
	paddsw	mm3, mm0
	movd	[edi +7 *4], mm3
	paddsw	mm0, mm6
	movd	[edi +9 *4], mm0
	movd	mm7, [esp +cost32_stack.r0_2]
	paddsw	mm6, mm7
	movd	[edi +11*4], mm6
	paddsw	mm7, mm1
	movd	[edi +13*4], mm7
	movd	mm0, [esp +cost32_stack.r0_0]
	paddsw	mm1, mm0
	movd	[edi +15*4], mm1

	add		esp, cost32_stack_size
	
	pop		ebx
	pop		ebp
	pop		esi
	pop		edi
	emms

	ret
