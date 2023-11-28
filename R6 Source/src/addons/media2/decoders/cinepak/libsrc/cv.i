
kCodeBookType		equ	020h
kPartialBookBit		equ	001h
kSmoothBookBit		equ	002h
kGreyBookBit		equ 004h
kFullDBookType		equ	kCodeBookType
kPartialDBookType	equ	kCodeBookType + kPartialBookBit
kFullSBookType		equ	kCodeBookType + kSmoothBookBit
kPartialSBookType	equ	kCodeBookType + kPartialBookBit + kSmoothBookBit
