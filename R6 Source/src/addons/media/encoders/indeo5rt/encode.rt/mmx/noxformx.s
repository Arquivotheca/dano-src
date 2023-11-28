
.text
.align 16

.text0:

.globl EncYBandNoXform

EncYBandNoXform:

.byte 0x56,0x57,0x55,0x53,0x81,0xec,0xbc,0x2,0x0,0x0,0x8b,0xcc,0x83,0xe4,0xe0,0x89,0x8c,0x24,0x8c,0x2,0x0,0x0
.byte 0x90,0x8b,0x81,0xd0,0x2,0x0,0x0,0x8b,0x99,0xd4,0x2,0x0,0x0,0x89,0x84,0x24,0xa0,0x2,0x0,0x0,0x89,0x9c
.byte 0x24,0xa4,0x2,0x0,0x0,0x8b,0x81,0xd8,0x2,0x0,0x0,0x8b,0x99,0xdc,0x2,0x0,0x0,0x89,0x84,0x24,0xa8,0x2
.byte 0x0,0x0,0x89,0x9c,0x24,0xac,0x2,0x0,0x0,0x8b,0x81,0xe0,0x2,0x0,0x0,0x8b,0x99,0xe4,0x2,0x0,0x0,0x89
.byte 0x84,0x24,0xb0,0x2,0x0,0x0,0x89,0x9c,0x24,0xb4,0x2,0x0,0x0,0x8b,0x81,0xe8,0x2,0x0,0x0,0x89,0x84,0x24
.byte 0xb8,0x2,0x0,0x0,0x8b,0xac,0x24,0xa0,0x2,0x0,0x0,0x33,0xd2,0x8b,0x5d,0x10,0x8b,0x45,0x4,0x89,0x9c,0x24
.byte 0x94,0x2,0x0,0x0,0x89,0x94,0x24,0x88,0x2,0x0,0x0,0x8d,0x1d
.long 0x0 + NoXform88qtablex

.byte 0x8d,0x35
.long 0x0 + NoXform88ZZPos

.byte 0x89,0x9c,0x24,0x90,0x2,0x0,0x0,0x33,0xc9,0x8b,0x4,0x8e,0x8b,0x5c,0x8e,0x4,0x89,0x84,0x8c,0x84,0x1,0x0
.byte 0x0,0x89,0x9c,0x8c,0x88,0x1,0x0,0x0,0x83,0xc1,0x2,0x83,0xf9,0x40,0x7c,0xe3,0xb8,0x40,0x0,0x0,0x0,0x89
.byte 0x84,0x24,0x84,0x2,0x0,0x0,0x8b,0xac,0x24,0xa4,0x2,0x0,0x0,0x8b,0xb4,0x24,0xa8,0x2,0x0,0x0,0x8b,0xbc
.byte 0x24,0xac,0x2,0x0,0x0,0x8b,0x45,0x18,0x8b,0x5d,0x0,0x3,0xc0,0x3,0xdb,0x3,0xf8,0x3,0xf3,0x3,0xfb,0x89
.byte 0xb4,0x24,0x98,0x2,0x0,0x0,0x89,0xbc,0x24,0x9c,0x2,0x0,0x0,0x8a,0x45,0x9,0x3c,0x0,0xf,0x84,0x3b,0x1
.byte 0x0,0x0,0xf,0x6f,0x6,0xf,0xf9,0x7,0xf,0x6f,0x4e,0x8,0xf,0xf9,0x4f,0x8,0xf,0x7f,0x6,0xf,0x7f,0x4e
.byte 0x8,0xf,0x6f,0x86,0x80,0x2,0x0,0x0,0xf,0xf9,0x87,0x80,0x2,0x0,0x0,0xf,0x6f,0x8e,0x88,0x2,0x0,0x0
.byte 0xf,0xf9,0x8f,0x88,0x2,0x0,0x0,0xf,0x7f,0x86,0x80,0x2,0x0,0x0,0xf,0x7f,0x8e,0x88,0x2,0x0,0x0,0xf
.byte 0x6f,0x86,0x0,0x5,0x0,0x0,0xf,0xf9,0x87,0x0,0x5,0x0,0x0,0xf,0x6f,0x8e,0x8,0x5,0x0,0x0,0xf,0xf9
.byte 0x8f,0x8,0x5,0x0,0x0,0xf,0x7f,0x86,0x0,0x5,0x0,0x0,0xf,0x7f,0x8e,0x8,0x5,0x0,0x0,0xf,0x6f,0x86
.byte 0x80,0x7,0x0,0x0,0xf,0xf9,0x87,0x80,0x7,0x0,0x0,0xf,0x6f,0x8e,0x88,0x7,0x0,0x0,0xf,0xf9,0x8f,0x88
.byte 0x7,0x0,0x0,0xf,0x7f,0x86,0x80,0x7,0x0,0x0,0xf,0x7f,0x8e,0x88,0x7,0x0,0x0,0xf,0x6f,0x86,0x0,0xa
.byte 0x0,0x0,0xf,0xf9,0x87,0x0,0xa,0x0,0x0,0xf,0x6f,0x8e,0x8,0xa,0x0,0x0,0xf,0xf9,0x8f,0x8,0xa,0x0
.byte 0x0,0xf,0x7f,0x86,0x0,0xa,0x0,0x0,0xf,0x7f,0x8e,0x8,0xa,0x0,0x0,0xf,0x6f,0x86,0x80,0xc,0x0,0x0
.byte 0xf,0xf9,0x87,0x80,0xc,0x0,0x0,0xf,0x6f,0x8e,0x88,0xc,0x0,0x0,0xf,0xf9,0x8f,0x88,0xc,0x0,0x0,0xf
.byte 0x7f,0x86,0x80,0xc,0x0,0x0,0xf,0x7f,0x8e,0x88,0xc,0x0,0x0,0xf,0x6f,0x86,0x0,0xf,0x0,0x0,0xf,0xf9
.byte 0x87,0x0,0xf,0x0,0x0,0xf,0x6f,0x8e,0x8,0xf,0x0,0x0,0xf,0xf9,0x8f,0x8,0xf,0x0,0x0,0xf,0x7f,0x86
.byte 0x0,0xf,0x0,0x0,0xf,0x7f,0x8e,0x8,0xf,0x0,0x0,0xf,0x6f,0x86,0x80,0x11,0x0,0x0,0xf,0xf9,0x87,0x80
.byte 0x11,0x0,0x0,0xf,0x6f,0x8e,0x88,0x11,0x0,0x0,0xf,0xf9,0x8f,0x88,0x11,0x0,0x0,0xf,0x7f,0x86,0x80,0x11
.byte 0x0,0x0,0xf,0x7f,0x8e,0x88,0x11,0x0,0x0,0x8b,0xbc,0x24,0xa4,0x2,0x0,0x0,0x8b,0xac,0x24,0x90,0x2,0x0
.byte 0x0,0x8b,0x84,0x24,0x94,0x2,0x0,0x0,0x8b,0xb4,0x24,0x98,0x2,0x0,0x0,0x8a,0x57,0x9,0x3,0x47,0xc,0x69
.byte 0xc0,0x80,0x1,0x0,0x0,0xf,0x6f,0x6,0x3,0xe8,0x80,0xfa,0x0,0x75,0x6,0x81,0xc5,0x0,0x24,0x0,0x0,0x8b
.byte 0x9c,0x24,0xb8,0x2,0x0,0x0,0x8b,0xfc,0x83,0xfb,0x1,0xf,0x84,0xa1,0x5,0x0,0x0,0xf,0x7f,0x6,0xf,0xef
.byte 0xff,0xf,0x6f,0xc8,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x8,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf
.byte 0xe5,0x8d,0x80,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0xef,0xec,0x81,0xc6,0x80,0x2,0x0,0x0,0xf,0xf9,0xec
.byte 0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x0,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x0,0x1,0x0
.byte 0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0x88,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef
.byte 0xd0,0xf,0x61,0xdf,0xf,0xf9,0xd0,0xf,0x69,0xcf,0xf,0x7f,0x9f,0x80,0x0,0x0,0x0,0xf,0x6f,0xf5,0xf,0x7f
.byte 0x8f,0x88,0x0,0x0,0x0,0xf,0x6f,0xdd,0xf,0xd5,0x75,0x8,0xf,0x7f,0x17,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x8
.byte 0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x6,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf
.byte 0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9f,0x90,0x0,0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f,0xaf,0x98
.byte 0x0,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x8,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5
.byte 0x8d,0x90,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f,0x77,0x8,0xf,0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff
.byte 0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x10,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x10,0x1,0x0,0x0,0xf,0xfd
.byte 0xc9,0xf,0xe5,0xad,0x98,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61
.byte 0xdf,0x81,0xc6,0x80,0x2,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x9f,0xa0,0x0,0x0,0x0,0xf,0xf9,0xd0,0xf,0x7f
.byte 0x8f,0xa8,0x0,0x0,0x0,0xf,0x6f,0xf5,0xf,0xd5,0x75,0x18,0xf,0x6f,0xdd,0xf,0x7f,0x57,0x10,0xf,0x75,0xdf
.byte 0xf,0xdf,0x9d,0x18,0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x6,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd
.byte 0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9f,0xb0,0x0,0x0,0x0,0xf,0x6f,0xc8
.byte 0xf,0x7f,0xaf,0xb8,0x0,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x8,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf
.byte 0xf9,0xc8,0xf,0xe5,0x8d,0xa0,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f,0x77,0x18,0xf,0xef,0xec,0xf,0xf9
.byte 0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x20,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x20,0x1
.byte 0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0xa8,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf
.byte 0xef,0xd0,0xf,0x61,0xdf,0x81,0xc6,0x80,0x2,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x9f,0xc0,0x0,0x0,0x0,0xf
.byte 0xf9,0xd0,0xf,0x7f,0x8f,0xc8,0x0,0x0,0x0,0xf,0x6f,0xf5,0xf,0xd5,0x75,0x28,0xf,0x6f,0xdd,0xf,0x7f,0x57
.byte 0x20,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x28,0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x6,0xf,0xf9,0xec,0xf,0xfd
.byte 0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9f,0xd0,0x0,0x0
.byte 0x0,0xf,0x6f,0xc8,0xf,0x7f,0xaf,0xd8,0x0,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x8,0xf,0xef,0xc8
.byte 0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0xb0,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f,0x77,0x28,0xf
.byte 0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x30,0xf,0x75,0xdf,0xf
.byte 0xdf,0x9d,0x30,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0xb8,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3
.byte 0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0x81,0xc6,0x80,0x2,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x9f,0xe0
.byte 0x0,0x0,0x0,0xf,0xf9,0xd0,0xf,0x7f,0x8f,0xe8,0x0,0x0,0x0,0xf,0x6f,0xf5,0xf,0xd5,0x75,0x38,0xf,0x6f
.byte 0xdd,0xf,0x7f,0x57,0x30,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x38,0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x6,0xf
.byte 0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f
.byte 0x9f,0xf0,0x0,0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f,0xaf,0xf8,0x0,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66
.byte 0x8,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0xc0,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf
.byte 0x7f,0x77,0x38,0xf,0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x40
.byte 0xf,0x75,0xdf,0xf,0xdf,0x9d,0x40,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0xc8,0x0,0x0,0x0,0xf,0xf9
.byte 0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0x81,0xc6,0x80,0x2,0x0,0x0,0xf,0x69,0xcf
.byte 0xf,0x7f,0x9f,0x0,0x1,0x0,0x0,0xf,0xf9,0xd0,0xf,0x7f,0x8f,0x8,0x1,0x0,0x0,0xf,0x6f,0xf5,0xf,0xd5
.byte 0x75,0x48,0xf,0x6f,0xdd,0xf,0x7f,0x57,0x40,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x48,0x1,0x0,0x0,0xf,0xfd,0xed
.byte 0xf,0x6f,0x6,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf
.byte 0x69,0xef,0xf,0x7f,0x9f,0x10,0x1,0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f,0xaf,0x18,0x1,0x0,0x0,0xf,0x71,0xe0
.byte 0xf,0xf,0x6f,0x66,0x8,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0xd0,0x0,0x0,0x0,0xf
.byte 0x71,0xe4,0xf,0xf,0x7f,0x77,0x48,0xf,0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9
.byte 0xf,0xd5,0x55,0x50,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x50,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0xd8,0x0
.byte 0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0x81,0xc6,0x80,0x2,0x0
.byte 0x0,0xf,0x69,0xcf,0xf,0x7f,0x9f,0x20,0x1,0x0,0x0,0xf,0xf9,0xd0,0xf,0x7f,0x8f,0x28,0x1,0x0,0x0,0xf
.byte 0x6f,0xf5,0xf,0xd5,0x75,0x58,0xf,0x6f,0xdd,0xf,0x7f,0x57,0x50,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x58,0x1,0x0
.byte 0x0,0xf,0xfd,0xed,0xf,0x6f,0x6,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf
.byte 0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9f,0x30,0x1,0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f,0xaf,0x38,0x1,0x0
.byte 0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x8,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0xe0
.byte 0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f,0x77,0x58,0xf,0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f
.byte 0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x60,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x60,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf
.byte 0xe5,0xad,0xe8,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0x81
.byte 0xc6,0x80,0x2,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x9f,0x40,0x1,0x0,0x0,0xf,0xf9,0xd0,0xf,0x7f,0x8f,0x48
.byte 0x1,0x0,0x0,0xf,0x6f,0xf5,0xf,0xd5,0x75,0x68,0xf,0x6f,0xdd,0xf,0x7f,0x57,0x60,0xf,0x75,0xdf,0xf,0xdf
.byte 0x9d,0x68,0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x6,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef
.byte 0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9f,0x50,0x1,0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f
.byte 0xaf,0x58,0x1,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x8,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8
.byte 0xf,0xe5,0x8d,0xf0,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f,0x77,0x68,0xf,0xef,0xec,0xf,0xf9,0xec,0xf
.byte 0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x70,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x70,0x1,0x0,0x0
.byte 0xf,0xfd,0xc9,0xf,0xe5,0xad,0xf8,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0
.byte 0xf,0x61,0xdf,0xf,0xf9,0xd0,0xf,0x69,0xcf,0xf,0x7f,0x9f,0x60,0x1,0x0,0x0,0xf,0x6f,0xf5,0xf,0x7f,0x8f
.byte 0x68,0x1,0x0,0x0,0xf,0x6f,0xdd,0xf,0xd5,0x75,0x78,0xf,0x7f,0x57,0x70,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x78
.byte 0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf
.byte 0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9f,0x70,0x1,0x0,0x0,0xf,0x7f,0xaf,0x78,0x1,0x0,0x0,0xf,0x7f,0x77
.byte 0x78,0xe9,0x9f,0x3,0x0,0x0,0xf,0x7f,0x6,0xf,0xef,0xff,0xf,0x6f,0xc8,0xf,0x71,0xe0,0xf,0xf,0x6f,0x56
.byte 0x8,0xf,0xef,0xc8,0xf,0x6f,0xa6,0x80,0x2,0x0,0x0,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0x80,0x0,0x0,0x0,0xf
.byte 0x6f,0xda,0xf,0x71,0xe2,0xf,0xf,0xef,0xda,0xf,0xeb,0xff,0xf,0x6f,0xe9,0xf,0x6f,0xf1,0xf,0xd5,0x6d,0x0
.byte 0xf,0x75,0xf7,0xf,0xdf,0xb5,0x0,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xf9,0xda,0xf,0xf9,0xc8,0xf,0xfd,0xf5
.byte 0xf,0x6f,0xec,0xf,0xe5,0x9d,0x88,0x0,0x0,0x0,0xf,0xef,0xf0,0xf,0x71,0xe4,0xf,0xf,0xf9,0xf0,0xf,0x6f
.byte 0xc1,0xf,0xef,0xec,0xf,0x61,0xc7,0xf,0xf9,0xec,0xf,0xe5,0xad,0x90,0x0,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f
.byte 0x87,0x80,0x0,0x0,0x0,0xf,0xfd,0xdb,0xf,0x7f,0x8f,0x88,0x0,0x0,0x0,0xf,0xf9,0xda,0xf,0x6f,0x86,0x88
.byte 0x2,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0xd3,0xf,0x6f,0xc8,0xf,0x61,0xd7,0xf,0xf9,0xec,0xf,0x69,0xdf,0xf
.byte 0x6f,0xe5,0xf,0x7f,0x97,0x90,0x0,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x7f,0x9f,0x98,0x0,0x0,0x0,0xf,0xef
.byte 0xc8,0xf,0x61,0xe7,0xf,0xf9,0xc8,0xf,0x6f,0x96,0x0,0x5,0x0,0x0,0xf,0x69,0xef,0xf,0xe5,0x8d,0x98,0x0
.byte 0x0,0x0,0xf,0x6f,0xda,0xf,0x7f,0xa7,0xa0,0x0,0x0,0x0,0xf,0x71,0xe2,0xf,0xf,0x7f,0xaf,0xa8,0x0,0x0
.byte 0x0,0xf,0xef,0xda,0xf,0x6f,0xa6,0x8,0x5,0x0,0x0,0xf,0xfd,0xc9,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0x71
.byte 0xe4,0xf,0xf,0xf9,0xda,0xf,0x7f,0x37,0xf,0xef,0xec,0xf,0xe5,0x9d,0xa0,0x0,0x0,0x0,0xf,0x6f,0xc1,0xf
.byte 0x61,0xc7,0xf,0xf9,0xec,0xf,0xe5,0xad,0xa8,0x0,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x87,0xb0,0x0,0x0,0x0
.byte 0xf,0xfd,0xdb,0xf,0x7f,0x8f,0xb8,0x0,0x0,0x0,0xf,0xf9,0xda,0xf,0x6f,0x86,0x80,0x7,0x0,0x0,0xf,0xfd
.byte 0xed,0xf,0x6f,0xd3,0xf,0x6f,0xc8,0xf,0x61,0xd7,0xf,0xf9,0xec,0xf,0x69,0xdf,0xf,0x6f,0xe5,0xf,0x7f,0x97
.byte 0xc0,0x0,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x7f,0x9f,0xc8,0x0,0x0,0x0,0xf,0xef,0xc8,0xf,0x61,0xe7,0xf
.byte 0xf9,0xc8,0xf,0x6f,0x96,0x88,0x7,0x0,0x0,0xf,0x69,0xef,0xf,0xe5,0x8d,0xb0,0x0,0x0,0x0,0xf,0x6f,0xda
.byte 0xf,0x7f,0xa7,0xd0,0x0,0x0,0x0,0xf,0x71,0xe2,0xf,0xf,0x7f,0xaf,0xd8,0x0,0x0,0x0,0xf,0xef,0xda,0xf
.byte 0x6f,0xa6,0x0,0xa,0x0,0x0,0xf,0xfd,0xc9,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0x71,0xe4,0xf,0xf,0xf9,0xda
.byte 0xf,0xef,0xec,0xf,0xe5,0x9d,0xb8,0x0,0x0,0x0,0xf,0x6f,0xc1,0xf,0x61,0xc7,0xf,0xf9,0xec,0xf,0xe5,0xad
.byte 0xc0,0x0,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x87,0xe0,0x0,0x0,0x0,0xf,0xfd,0xdb,0xf,0x7f,0x8f,0xe8,0x0
.byte 0x0,0x0,0xf,0xf9,0xda,0xf,0x6f,0x86,0x8,0xa,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0xd3,0xf,0x6f,0xc8,0xf
.byte 0x61,0xd7,0xf,0xf9,0xec,0xf,0x69,0xdf,0xf,0x6f,0xe5,0xf,0x7f,0x97,0xf0,0x0,0x0,0x0,0xf,0x71,0xe0,0xf
.byte 0xf,0x7f,0x9f,0xf8,0x0,0x0,0x0,0xf,0xef,0xc8,0xf,0x61,0xe7,0xf,0xf9,0xc8,0xf,0x6f,0x96,0x80,0xc,0x0
.byte 0x0,0xf,0x69,0xef,0xf,0xe5,0x8d,0xc8,0x0,0x0,0x0,0xf,0x6f,0xda,0xf,0x7f,0xa7,0x0,0x1,0x0,0x0,0xf
.byte 0x71,0xe2,0xf,0xf,0x7f,0xaf,0x8,0x1,0x0,0x0,0xf,0xef,0xda,0xf,0x6f,0xa6,0x88,0xc,0x0,0x0,0xf,0xfd
.byte 0xc9,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0x71,0xe4,0xf,0xf,0xf9,0xda,0xf,0xef,0xec,0xf,0xe5,0x9d,0xd0,0x0
.byte 0x0,0x0,0xf,0x6f,0xc1,0xf,0x61,0xc7,0xf,0xf9,0xec,0xf,0xe5,0xad,0xd8,0x0,0x0,0x0,0xf,0x69,0xcf,0xf
.byte 0x7f,0x87,0x10,0x1,0x0,0x0,0xf,0xfd,0xdb,0xf,0x7f,0x8f,0x18,0x1,0x0,0x0,0xf,0xf9,0xda,0xf,0x6f,0x86
.byte 0x0,0xf,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0xd3,0xf,0x6f,0xc8,0xf,0x61,0xd7,0xf,0xf9,0xec,0xf,0x69,0xdf
.byte 0xf,0x6f,0xe5,0xf,0x7f,0x97,0x20,0x1,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x7f,0x9f,0x28,0x1,0x0,0x0,0xf
.byte 0xef,0xc8,0xf,0x61,0xe7,0xf,0xf9,0xc8,0xf,0x6f,0x96,0x8,0xf,0x0,0x0,0xf,0x69,0xef,0xf,0xe5,0x8d,0xe0
.byte 0x0,0x0,0x0,0xf,0x6f,0xda,0xf,0x7f,0xa7,0x30,0x1,0x0,0x0,0xf,0x71,0xe2,0xf,0xf,0x7f,0xaf,0x38,0x1
.byte 0x0,0x0,0xf,0xef,0xda,0xf,0x6f,0xa6,0x80,0x11,0x0,0x0,0xf,0xfd,0xc9,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf
.byte 0x71,0xe4,0xf,0xf,0xf9,0xda,0xf,0xef,0xec,0xf,0xe5,0x9d,0xe8,0x0,0x0,0x0,0xf,0x6f,0xc1,0xf,0x61,0xc7
.byte 0xf,0xf9,0xec,0xf,0xe5,0xad,0xf0,0x0,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x87,0x40,0x1,0x0,0x0,0xf,0xfd
.byte 0xdb,0xf,0x7f,0x8f,0x48,0x1,0x0,0x0,0xf,0xf9,0xda,0xf,0x6f,0x86,0x88,0x11,0x0,0x0,0xf,0xfd,0xed,0xf
.byte 0x6f,0xd3,0xf,0x6f,0xc8,0xf,0x71,0xe0,0xf,0xf,0xf9,0xec,0xf,0xef,0xc8,0xf,0x61,0xd7,0xf,0xf9,0xc8,0xf
.byte 0x69,0xdf,0xf,0xe5,0x8d,0xf8,0x0,0x0,0x0,0xf,0x6f,0xe5,0xf,0x7f,0x97,0x50,0x1,0x0,0x0,0xf,0x61,0xe7
.byte 0xf,0x7f,0x9f,0x58,0x1,0x0,0x0,0xf,0x69,0xef,0xf,0x7f,0xa7,0x60,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0x7f
.byte 0xaf,0x68,0x1,0x0,0x0,0xf,0xf9,0xc8,0xf,0x6f,0xc1,0xf,0x69,0xcf,0xf,0x61,0xc7,0xf,0x7f,0x8f,0x78,0x1
.byte 0x0,0x0,0xf,0x7f,0x87,0x70,0x1,0x0,0x0,0x8b,0x84,0x24,0xb0,0x2,0x0,0x0,0x33,0xff,0xba,0xff,0xff,0x0
.byte 0x0,0xb9,0xf8,0xff,0xff,0xff,0x8b,0x9c,0xbc,0x84,0x1,0x0,0x0,0x89,0x94,0x24,0x80,0x1,0x0,0x0,0x8b,0x30
.byte 0x2e,0x8b,0xc0,0x2e,0x8b,0xc0,0x2e,0x8b,0xc0,0x2e,0x8b,0xc0,0x90,0x8b,0x84,0x9c,0x80,0x0,0x0,0x0,0x83,0xc1
.byte 0x8,0x8b,0x9c,0xbc,0x88,0x1,0x0,0x0,0x47,0x83,0xf8,0x1,0x7e,0xe9,0x3d,0x0,0x80,0x0,0x0,0x7f,0x3e,0x8b
.byte 0x91
.long 0x0 + NonEscCoeff

.byte 0x48,0x3b,0xd0,0x7c,0x11,0x8a,0x94,0xc8
.long 0xffffffff + CodeBookIdx

.byte 0xb9,0xf8,0xff,0xff,0xff,0x88,0x16,0x46,0xeb,0xc6,0xc1,0xe9,0x3,0xc6,0x6,0xb,0x8d,0x14,0x85,0x0,0x0,0x0
.byte 0x0,0x88,0x4e,0x1,0x88,0x76,0x3,0x24,0x3f,0xb9,0xf8,0xff,0xff,0xff,0x88,0x46,0x2,0x83,0xc6,0x4,0xeb,0xa4
.byte 0x83,0xff,0x41,0x74,0x77,0x8b,0x9c,0xbc,0x80,0x1,0x0,0x0,0x8b,0x84,0x24,0x98,0x2,0x0,0x0,0x83,0xfb,0x8
.byte 0x7d,0x4a,0x8b,0x4,0x58,0x66,0x89,0x4,0x5c,0x8b,0x9c,0xbc,0x84,0x1,0x0,0x0,0x25,0xff,0xff,0x0,0x0,0x3d
.byte 0x0,0x80,0x0,0x0,0x7d,0x15,0x3,0xc0,0xf,0x84,0x6a,0xff,0xff,0xff,0x8b,0x91
.long 0x0 + NonEscCoeff

.byte 0x48,0x3b,0xd0,0x7d,0x88,0xeb,0x97,0x35,0xff,0xff,0x0,0x0,0x40,0x8b,0x91
.long 0x0 + NonEscCoeff

.byte 0x3,0xc0,0x3b,0xd0,0xf,0x8d,0x70,0xff,0xff,0xff,0xe9,0x7c,0xff,0xff,0xff,0xc1,0xeb,0x3,0x8b,0x94,0xbc,0x80
.byte 0x1,0x0,0x0,0x69,0xdb,0x80,0x2,0x0,0x0,0x3,0xc3,0x83,0xe2,0x7,0x8b,0x4,0x50,0xeb,0x9f,0x8b,0xbc,0x24
.byte 0xa4,0x2,0x0,0x0,0x8b,0x84,0x24,0xb0,0x2,0x0,0x0,0x8b,0x9c,0x24,0xa0,0x2,0x0,0x0,0x8a,0x57,0x9,0x8b
.byte 0x5b,0xc,0x8d,0x6f,0x1c,0x80,0xfa,0x0,0x75,0x1e,0x83,0xfb,0x0,0x75,0x19,0x8b,0x14,0x24,0x8b,0x9c,0x24,0x88
.byte 0x2,0x0,0x0,0x66,0x3,0xd3,0x66,0x89,0x14,0x24,0x66,0x89,0x94,0x24,0x88,0x2,0x0,0x0,0x8b,0x18,0xc6,0x6
.byte 0x4,0x3b,0xde,0x75,0x6,0xc6,0x45,0x0,0x1,0xeb,0x7,0xc6,0x45,0x0,0x0,0x46,0x89,0x30,0x8b,0x9c,0x24,0xb8
.byte 0x2,0x0,0x0,0x8b,0x94,0x24,0xa4,0x2,0x0,0x0,0x83,0xfb,0x1,0xf,0x84,0xdc,0x1,0x0,0x0,0x8a,0x42,0x9
.byte 0x8b,0xb4,0x24,0x98,0x2,0x0,0x0,0x8d,0x2c,0x24,0x3c,0x0,0xf,0x84,0x1e,0x1,0x0,0x0,0x8b,0xbc,0x24,0x9c
.byte 0x2,0x0,0x0,0xf,0x6f,0x7,0xf,0xfd,0x45,0x0,0xf,0x6f,0x4f,0x8,0xf,0xfd,0x4d,0x8,0xf,0x7f,0x6,0xf
.byte 0x7f,0x4e,0x8,0xf,0x6f,0x87,0x80,0x2,0x0,0x0,0xf,0xfd,0x45,0x10,0xf,0x6f,0x8f,0x88,0x2,0x0,0x0,0xf
.byte 0xfd,0x4d,0x18,0xf,0x7f,0x86,0x80,0x2,0x0,0x0,0xf,0x7f,0x8e,0x88,0x2,0x0,0x0,0xf,0x6f,0x87,0x0,0x5
.byte 0x0,0x0,0xf,0xfd,0x45,0x20,0xf,0x6f,0x8f,0x8,0x5,0x0,0x0,0xf,0xfd,0x4d,0x28,0xf,0x7f,0x86,0x0,0x5
.byte 0x0,0x0,0xf,0x7f,0x8e,0x8,0x5,0x0,0x0,0xf,0x6f,0x87,0x80,0x7,0x0,0x0,0xf,0xfd,0x45,0x30,0xf,0x6f
.byte 0x8f,0x88,0x7,0x0,0x0,0xf,0xfd,0x4d,0x38,0xf,0x7f,0x86,0x80,0x7,0x0,0x0,0xf,0x7f,0x8e,0x88,0x7,0x0
.byte 0x0,0xf,0x6f,0x87,0x0,0xa,0x0,0x0,0xf,0xfd,0x45,0x40,0xf,0x6f,0x8f,0x8,0xa,0x0,0x0,0xf,0xfd,0x4d
.byte 0x48,0xf,0x7f,0x86,0x0,0xa,0x0,0x0,0xf,0x7f,0x8e,0x8,0xa,0x0,0x0,0xf,0x6f,0x87,0x80,0xc,0x0,0x0
.byte 0xf,0xfd,0x45,0x50,0xf,0x6f,0x8f,0x88,0xc,0x0,0x0,0xf,0xfd,0x4d,0x58,0xf,0x7f,0x86,0x80,0xc,0x0,0x0
.byte 0xf,0x7f,0x8e,0x88,0xc,0x0,0x0,0xf,0x6f,0x87,0x0,0xf,0x0,0x0,0xf,0xfd,0x45,0x60,0xf,0x6f,0x8f,0x8
.byte 0xf,0x0,0x0,0xf,0xfd,0x4d,0x68,0xf,0x7f,0x86,0x0,0xf,0x0,0x0,0xf,0x7f,0x8e,0x8,0xf,0x0,0x0,0xf
.byte 0x6f,0x87,0x80,0x11,0x0,0x0,0xf,0xfd,0x45,0x70,0xf,0x6f,0x8f,0x88,0x11,0x0,0x0,0xf,0xfd,0x4d,0x78,0xf
.byte 0x7f,0x86,0x80,0x11,0x0,0x0,0xf,0x7f,0x8e,0x88,0x11,0x0,0x0,0xe9,0xa9,0x0,0x0,0x0,0xf,0x6f,0x45,0x0
.byte 0xf,0x6f,0x4d,0x8,0xf,0x7f,0x6,0xf,0x7f,0x4e,0x8,0xf,0x6f,0x45,0x10,0xf,0x6f,0x4d,0x18,0xf,0x7f,0x86
.byte 0x80,0x2,0x0,0x0,0xf,0x7f,0x8e,0x88,0x2,0x0,0x0,0xf,0x6f,0x45,0x20,0xf,0x6f,0x4d,0x28,0xf,0x7f,0x86
.byte 0x0,0x5,0x0,0x0,0xf,0x7f,0x8e,0x8,0x5,0x0,0x0,0xf,0x6f,0x45,0x30,0xf,0x6f,0x4d,0x38,0xf,0x7f,0x86
.byte 0x80,0x7,0x0,0x0,0xf,0x7f,0x8e,0x88,0x7,0x0,0x0,0xf,0x6f,0x45,0x40,0xf,0x6f,0x4d,0x48,0xf,0x7f,0x86
.byte 0x0,0xa,0x0,0x0,0xf,0x7f,0x8e,0x8,0xa,0x0,0x0,0xf,0x6f,0x45,0x50,0xf,0x6f,0x4d,0x58,0xf,0x7f,0x86
.byte 0x80,0xc,0x0,0x0,0xf,0x7f,0x8e,0x88,0xc,0x0,0x0,0xf,0x6f,0x45,0x60,0xf,0x6f,0x4d,0x68,0xf,0x7f,0x86
.byte 0x0,0xf,0x0,0x0,0xf,0x7f,0x8e,0x8,0xf,0x0,0x0,0xf,0x6f,0x45,0x70,0xf,0x6f,0x4d,0x78,0xf,0x7f,0x86
.byte 0x80,0x11,0x0,0x0,0xf,0x7f,0x8e,0x88,0x11,0x0,0x0,0x8b,0xac,0x24,0xa4,0x2,0x0,0x0,0x8b,0x45,0x24,0x83
.byte 0xc5,0x20,0x3c,0xff,0x74,0xc,0x89,0xac,0x24,0xa4,0x2,0x0,0x0,0xe9,0x8e,0xf1,0xff,0xff,0x8b,0xa4,0x24,0x8c
.byte 0x2,0x0,0x0,0xf,0x77,0x81,0xc4,0xbc,0x2,0x0,0x0,0x5b,0x5d,0x5f,0x5e,0xc3

.data
.align 16

.data1:

