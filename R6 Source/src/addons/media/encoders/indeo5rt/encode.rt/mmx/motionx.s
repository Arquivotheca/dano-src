
.text
.align 16

.text0:

.globl MotionEstimation

MotionEstimation:

.byte 0x56,0x57,0x55,0x53,0x83,0xec,0x40,0x8b,0xcc,0x81,0xec,0x0,0x10,0x0,0x0,0x8d,0x5
.long 0x1678 + METable

.byte 0x81,0xe4,0x0,0xf0,0xff,0xff,0x25,0xe0,0xf,0x0,0x0,0x83,0xc0,0x20,0x3,0xe0,0x89,0x4c,0x24,0x3c,0x8b,0x59
.byte 0x54,0x8b,0x41,0x58,0x89,0x5c,0x24,0x10,0x89,0x44,0x24,0x28,0x8b,0x59,0x5c,0x8b,0x41,0x60,0x8b,0x54,0x24,0x10
.byte 0x89,0x44,0x24,0x30,0x89,0x5c,0x24,0x2c,0x8b,0x59,0x64,0x89,0x5c,0x24,0x34,0xb8,0x1,0x0,0x0,0x0,0x83,0xea
.byte 0x20,0x33,0xdb,0x89,0x44,0x24,0xc,0x89,0x5c,0x24,0x38,0x89,0x44,0x24,0x18,0x8b,0x4a,0x24,0x83,0xc2,0x20,0x83
.byte 0xf9,0x0,0xf,0x88,0x47,0x4,0x0,0x0,0x88,0x4c,0x24,0xc,0x8b,0x74,0x24,0x2c,0x89,0x54,0x24,0x10,0x3,0x32
.byte 0xf,0x6f,0x5
.long 0x0 + ml0ff

.byte 0xf,0x6f,0xe,0xf,0x6f,0x96,0x80,0x2,0x0,0x0,0xf,0xdb,0xc8,0xf,0x6f,0x9e,0x0,0x5,0x0,0x0,0xf,0x6f
.byte 0xf9,0xf,0x6f,0xa6,0x80,0x7,0x0,0x0,0xf,0xdb,0xd0,0xf,0xfd,0xfa,0xf,0xdb,0xd8,0xf,0xfd,0xfb,0xf,0xdb
.byte 0xe0,0xf,0xfd,0xfc,0xf,0x6f,0xf7,0xf,0x73,0xd7,0x20,0xf,0xfd,0xf7,0xf,0x7e,0xf0,0x8b,0xd8,0x25,0xff,0xff
.byte 0x0,0x0,0xc1,0xeb,0x10,0x3,0xc3,0x81,0xf9,0x0,0x0,0x1,0x0,0x89,0x44,0x24,0x8,0x7c,0xe,0xb9,0xff,0xff
.byte 0xff,0xff,0x89,0x4c,0x24,0x1c,0xe9,0x8,0x3,0x0,0x0,0x8b,0x44,0x24,0x28,0x83,0xf8,0x0,0xf,0x89,0xc5,0x0
.byte 0x0,0x0,0x8d,0xbe,0x0,0x38,0x1,0x0,0x33,0xc9,0xf,0x6f,0x6,0xf,0x6f,0xf,0xf,0x6f,0x96,0x80,0x2,0x0
.byte 0x0,0xf,0xf9,0xc1,0xf,0x6f,0x9f,0x80,0x2,0x0,0x0,0xf,0xf5,0xc0,0xf,0x6f,0xa6,0x0,0x5,0x0,0x0,0xf
.byte 0xf9,0xd3,0xf,0x6f,0xaf,0x0,0x5,0x0,0x0,0xf,0xf5,0xd2,0xf,0x6f,0x8f,0x80,0x7,0x0,0x0,0xf,0xf9,0xe5
.byte 0xf,0x6f,0xf0,0xf,0xf5,0xe4,0xf,0x6f,0x86,0x80,0x7,0x0,0x0,0xf,0xdd,0xf2,0xf,0xf9,0xc1,0x8b,0x7c,0x24
.byte 0x10,0xf,0xdd,0xf4,0xf,0xf5,0xc0,0x8a,0x4f,0x8,0x8b,0x5c,0x24,0xc,0x8d,0x44,0xc9,0xff,0xf,0xdd,0xf0,0xf
.byte 0x6f,0xc6,0xf,0x73,0xd6,0x20,0xc1,0xe0,0x10,0xf,0xdd,0xf0,0x8a,0xc,0xdd
.long 0xc5d + METable

.byte 0x33,0xd2,0xf,0x7e,0xf5,0x8a,0x14,0xdd
.long 0xc5b + METable

.byte 0x33,0xdb,0x89,0x6c,0x24,0x1c,0x8d,0xbe,0x0,0x38,0x1,0x0,0x89,0x7c,0x24,0x14,0x3b,0xe8,0xf,0x86,0x17,0x2
.byte 0x0,0x0,0x8b,0x44,0x24,0x30,0x89,0x6c,0x24,0x38,0x83,0xf8,0x0,0xf,0x85,0x6,0x2,0x0,0x0,0x8b,0x99
.long 0xc04 + METable

.byte 0x8b,0x82
.long 0xc04 + METable

.byte 0xe9,0xc0,0x0,0x0,0x0,0x8d,0xbe,0x0,0xc8,0xfe,0xff,0x33,0xc9,0xf,0x6f,0x6,0xf,0x6f,0xf,0xf,0x6f,0x96
.byte 0x80,0x2,0x0,0x0,0xf,0xf9,0xc1,0xf,0x6f,0x9f,0x80,0x2,0x0,0x0,0xf,0xf5,0xc0,0xf,0x6f,0xa6,0x0,0x5
.byte 0x0,0x0,0xf,0xf9,0xd3,0xf,0x6f,0xaf,0x0,0x5,0x0,0x0,0xf,0xf5,0xd2,0xf,0x6f,0x8f,0x80,0x7,0x0,0x0
.byte 0xf,0xf9,0xe5,0xf,0x6f,0xf0,0xf,0xf5,0xe4,0xf,0x6f,0x86,0x80,0x7,0x0,0x0,0xf,0xdd,0xf2,0xf,0xf9,0xc1
.byte 0x8b,0x7c,0x24,0x10,0xf,0xdd,0xf4,0xf,0xf5,0xc0,0x8a,0x4f,0x8,0x8b,0x5c,0x24,0xc,0x8d,0x44,0xc9,0xff,0xf
.byte 0xdd,0xf0,0xf,0x6f,0xc6,0xf,0x73,0xd6,0x20,0xc1,0xe0,0x10,0xf,0xdd,0xf0,0x8a,0xc,0xdd
.long 0xc5d + METable

.byte 0x33,0xd2,0xf,0x7e,0xf5,0x8a,0x14,0xdd
.long 0xc5b + METable

.byte 0x33,0xdb,0x89,0x6c,0x24,0x1c,0x8d,0xbe,0x0,0xc8,0xfe,0xff,0x89,0x7c,0x24,0x14,0x3b,0xe8,0xf,0x86,0x52,0x1
.byte 0x0,0x0,0x8b,0x44,0x24,0x30,0x89,0x6c,0x24,0x38,0x83,0xf8,0x0,0xf,0x85,0x41,0x1,0x0,0x0,0x8b,0x99
.long 0xc04 + METable

.byte 0x8b,0x82
.long 0xc04 + METable

.byte 0x3,0xc7,0x3,0xdf,0xf,0x6f,0x6,0xf,0xef,0xed,0xf,0x6f,0x10,0xf,0x6f,0xc8,0xf,0x6f,0x1b,0xf,0xf9,0xc2
.byte 0xf,0xf9,0xcb,0xf,0xf5,0xc0,0xf,0x6f,0xa6,0x80,0x2,0x0,0x0,0xf,0xf5,0xc9,0xf,0x6f,0x90,0x80,0x2,0x0
.byte 0x0,0xf,0x6f,0xec,0xf,0x6f,0x9b,0x80,0x2,0x0,0x0,0xf,0xf9,0xe2,0xf,0xf9,0xeb,0xf,0xf5,0xe4,0xf,0x6f
.byte 0xf0,0xf,0xf5,0xed,0xf,0x6f,0x86,0x0,0x5,0x0,0x0,0xf,0x6f,0xf9,0xf,0x6f,0x90,0x0,0x5,0x0,0x0,0xf
.byte 0x6f,0xc8,0xf,0x6f,0x9b,0x0,0x5,0x0,0x0,0xf,0xf9,0xc2,0xf,0xf9,0xcb,0xf,0xf5,0xc0,0xf,0xdd,0xf4,0xf
.byte 0xf5,0xc9,0xf,0x6f,0xa6,0x80,0x7,0x0,0x0,0xf,0xdd,0xfd,0xf,0x6f,0x90,0x80,0x7,0x0,0x0,0xf,0x6f,0xec
.byte 0xf,0x6f,0x9b,0x80,0x7,0x0,0x0,0xf,0xf9,0xe2,0xf,0xf9,0xeb,0xf,0xf5,0xe4,0xf,0xdd,0xf0,0xf,0xf5,0xed
.byte 0xf,0xdd,0xf9,0x8b,0x6c,0x24,0x1c,0x8b,0x7c,0x24,0xc,0xf,0xdd,0xf4,0xf,0xdd,0xfd,0xf,0x6f,0xc6,0xf,0x6f
.byte 0xcf,0xf,0x73,0xd0,0x20,0xf,0x73,0xd1,0x20,0xf,0xdd,0xf0,0xf,0xdd,0xf9,0x33,0xdb,0xf,0x7e,0xf2,0xf,0x7e
.byte 0xf9,0x33,0xc0,0x3b,0xd1,0x13,0xc0,0x3b,0xd5,0x13,0xc0,0x3b,0xcd,0x13,0xc0,0x89,0x54,0x24,0x20,0x89,0x4c,0x24
.byte 0x24,0x33,0xc9,0x8a,0x98
.long 0xbfc + METable

.byte 0x33,0xd2,0x8b,0x6c,0x5c,0x1c,0x8a,0x8c,0xfb
.long 0xc58 + METable

.byte 0x8a,0x9c,0xfb
.long 0xc59 + METable

.byte 0x89,0x6c,0x24,0x1c,0x83,0xf9,0x0,0x74,0x33,0x8a,0x14,0xcd
.long 0xc5b + METable

.byte 0x8b,0xab
.long 0xc04 + METable

.byte 0x89,0x4c,0x24,0xc,0x8b,0x7c,0x24,0x14,0x8a,0xc,0xcd
.long 0xc5d + METable

.byte 0x3,0xfd,0x8b,0x82
.long 0xc04 + METable

.byte 0x8b,0x99
.long 0xc04 + METable

.byte 0x89,0x7c,0x24,0x14,0xe9,0xcb,0xfe,0xff,0xff,0x8b,0x74,0x24,0x14,0x8b,0x54,0x24,0x10,0x8b,0x83
.long 0xc04 + METable

.byte 0x8b,0x5c,0x24,0x28,0x8b,0x7c,0x24,0x2c,0x3,0xf0,0x3,0x3a,0x3,0xde,0x2b,0xdf,0x89,0x5a,0x18,0x8b,0xeb,0x8b
.byte 0xc3,0xc1,0xfd,0x8,0x8d,0x1c,0x9d,0x0,0x0,0x0,0x0,0xc0,0xfb,0x2,0x88,0x5a,0xb,0x8a,0x1c,0x2d
.long 0xc4e + METable

.byte 0x88,0x5a,0xa,0xeb,0x0,0x8b,0x5c,0x24,0x8,0xc1,0xeb,0x4,0xf,0x6f,0x4,0xdd
.long 0xe78 + METable

.byte 0xf,0xef,0xed,0xf,0x6f,0xe,0xf,0x6f,0xd0,0xf,0x6f,0x9e,0x80,0x2,0x0,0x0,0xf,0x6f,0xe0,0xf,0xf9,0xd1
.byte 0xf,0xf9,0xe3,0xf,0x6f,0x8e,0x0,0x5,0x0,0x0,0xf,0xf5,0xd2,0xf,0x6f,0x9e,0x80,0x7,0x0,0x0,0xf,0xf5
.byte 0xe4,0xf,0x6f,0xe8,0xf,0x6f,0xf0,0xf,0xf9,0xe9,0xf,0xf9,0xf3,0xf,0xf5,0xed,0xf,0x6f,0xfa,0xf,0xf5,0xf6
.byte 0x8b,0x54,0x24,0x10,0xf,0x6e,0x44,0x24,0x1c,0xf,0xdd,0xfc,0xf,0xdd,0xfd,0xf,0x72,0xd0,0x10,0xf,0x6e,0x4c
.byte 0x24,0x34,0xf,0xdd,0xfe,0xf,0x6f,0xf7,0xf,0x73,0xd7,0x20,0xf,0x7e,0xc5,0xf,0xf5,0xc8,0xf,0xdd,0xfe,0x33
.byte 0xdb,0xf,0x72,0xd7,0x10,0x8a,0x5a,0x8,0xb1,0x0,0x81,0xfd,0xff,0xff,0x0,0x0,0xf,0x7e,0xf8,0xf,0x72,0xd1
.byte 0x3,0x74,0x11,0x81,0xfd,0x0,0xf,0x0,0x0,0x73,0x9,0xf,0x7e,0xcd,0x3b,0xe8,0x73,0x2,0xb1,0x1,0x88,0x4a
.byte 0x9,0x8b,0x4c,0x24,0x18,0x89,0x42,0x10,0x3,0xc8,0xf,0xaf,0x4,0x9d
.long 0xe34 + METable

.byte 0x89,0x4c,0x24,0x18,0x89,0x42,0x14,0xe9,0xaa,0xfb,0xff,0xff,0x8b,0x44,0x24,0x18,0x8b,0x4c,0x24,0x38,0x8b,0x64
.byte 0x24,0x3c,0x8b,0x74,0x24,0x68,0x83,0xc4,0x40,0x89,0xe,0x5b,0x5d,0x5f,0x5e,0xc3

.data
.align 16

.data1:

ml0ff:

.byte 0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0
