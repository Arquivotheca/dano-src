
.data
.align 8

c422_vcsid:

.byte 0x40,0x28,0x23,0x29,0x20,0x24,0x57,0x6f,0x72,0x6b,0x66,0x69,0x6c,0x65,0x3a,0x20,0x20,0x20,0x63,0x34,0x32,0x32
.byte 0x2e,0x63,0x20,0x20,0x24,0x20,0x24,0x52,0x65,0x76,0x69,0x73,0x69,0x6f,0x6e,0x3a,0x20,0x20,0x20,0x31,0x2e,0x32
.byte 0x20,0x20,0x24,0x0
.globl B_ONE

B_ONE:

.byte 0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1
.globl B_XFE

B_XFE:

.byte 0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe
.globl B_X7F

B_X7F:

.byte 0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f

.text
.align 16

.globl C_YVU9toYUY2

C_YVU9toYUY2:

.byte 0x55,0x8b,0xec,0x57,0x56,0x53,0x8b,0xcc,0x83,0xe1,0x1f,0x83,0xe4,0xe0,0x83,0xec,0x40,0x89,0xc,0x24,0xb8,0x1
.byte 0x0,0x0,0x0,0x8b,0x55,0xc,0xc1,0xea,0x2,0xf,0x84,0x71,0x7,0x0,0x0,0x89,0x54,0x24,0x18,0x8b,0x4d,0x8
.byte 0x83,0xe1,0xfc,0xf,0x84,0x61,0x7,0x0,0x0,0x89,0x4c,0x24,0x1c,0x8b,0x5d,0x1c,0x8b,0x55,0xc,0x89,0x5c,0x24
.byte 0x30,0x83,0xe2,0xfc,0x2b,0xda,0x89,0x5c,0x24,0x10,0x8b,0x45,0x14,0x89,0x44,0x24,0x4,0x8b,0x45,0x18,0x89,0x44
.byte 0x24,0x8,0x8b,0x45,0x20,0x89,0x44,0x24,0x14,0x8b,0x4c,0x24,0x18,0x2b,0xc1,0x89,0x44,0x24,0x38,0x8b,0x4d,0x28
.byte 0x8b,0x5d,0xc,0x83,0xe3,0xfc,0x3,0xdb,0x89,0x4c,0x24,0x24,0x2b,0xcb,0x89,0x4c,0x24,0xc,0x8b,0x44,0x24,0x30
.byte 0x8d,0x4,0x40,0x83,0xe8,0x10,0x89,0x44,0x24,0x28,0x8b,0x4c,0x24,0x24,0x8d,0xc,0x49,0x83,0xe9,0x20,0x89,0x4c
.byte 0x24,0x3c,0x8b,0x75,0x10,0x8b,0x7d,0x24,0xb8,0x1,0x0,0x0,0x0,0xf7,0xc6,0x3,0x0,0x0,0x0,0xf,0x85,0xe3
.byte 0x6,0x0,0x0,0xf7,0xc7,0x3,0x0,0x0,0x0,0xf,0x85,0xd7,0x6,0x0,0x0,0x83,0x3d
.long 0x0 + use_mmx

.byte 0x2,0x74,0x19,0xf7,0xc7,0x7,0x0,0x0,0x0,0xf,0x85,0xcf,0x6,0x0,0x0,0x83,0x3d
.long 0x0 + use_mmx

.byte 0x1,0xf,0x85,0xc2,0x6,0x0,0x0,0x8b,0x4d,0x28,0x8b,0x5d,0xc,0xc1,0xe1,0x2,0x83,0xe3,0xfc,0x3,0xdb,0x2b
.byte 0xcb,0x89,0x4c,0x24,0xc,0x8b,0x5d,0x1c,0x8b,0x55,0xc,0xc1,0xe3,0x2,0x83,0xe2,0xfc,0x2b,0xda,0x89,0x5c,0x24
.byte 0x10,0x8b,0x55,0xc,0x83,0xfa,0x20,0xf,0x8c,0x83,0x6,0x0,0x0,0x8b,0x55,0x8,0x83,0xfa,0x8,0xf,0x8c,0x77
.byte 0x6,0x0,0x0,0x8b,0x4c,0x24,0x4,0x8b,0x54,0x24,0x8,0x2b,0xd1,0x8b,0x6c,0x24,0x18,0xc1,0xed,0x2,0xf,0x84
.byte 0x60,0x6,0x0,0x0,0x8b,0x44,0x24,0x1c,0x8b,0x5c,0x24,0x18,0x80,0xe3,0x3,0x75,0x7,0x4d,0xf,0x86,0x4c,0x6
.byte 0x0,0x0,0xc1,0xe8,0x2,0x90,0x48,0xf,0x86,0x41,0x6,0x0,0x0,0x89,0x44,0x24,0x1c,0x89,0x6c,0x24,0x34,0x89
.byte 0x54,0x24,0x20,0x33,0xc0,0x8a,0x44,0x11,0x4,0x8b,0x14,0x11,0x8b,0xda,0x81,0xe2,0x0,0xff,0xff,0xff,0xb,0xc2
.byte 0x90,0xf,0x6e,0xcb,0xc1,0xc8,0x8,0x8b,0xd3,0xd1,0xeb,0x23,0xd0,0xd1,0xe8,0x81,0xe3,0x7f,0x7f,0x7f,0x7f,0x81
.byte 0xe2,0x1,0x1,0x1,0x1,0x25,0x7f,0x7f,0x7f,0x7f,0x3,0xd8,0x33,0xc0,0x3,0xd3,0x8a,0x41,0x4,0xf,0x6e,0xd2
.byte 0x8b,0x11,0x90,0x8b,0xda,0x81,0xe2,0x0,0xff,0xff,0xff,0xb,0xc2,0x8b,0xd3,0xf,0x6f,0x6,0xf,0x60,0xca,0xf
.byte 0x6e,0xd3,0xf,0x7f,0xc4,0x90,0x81,0xe3,0xfe,0xfe,0xfe,0xfe,0xc1,0xc8,0x8,0xf,0x7f,0xcd,0xd1,0xeb,0x23,0xd0
.byte 0xd1,0xe8,0x81,0xe2,0x1,0x1,0x1,0x1,0x25,0x7f,0x7f,0x7f,0x7f,0x3,0xda,0x3,0xd8,0x33,0xc0,0xf,0x6e,0xdb
.byte 0xf,0x7f,0xce,0xf,0x6f,0x7e,0x8,0xf,0x60,0xd3,0x8b,0x54,0x24,0x14,0xf,0x60,0xea,0x3,0xca,0xf,0x60,0xe5
.byte 0x8b,0x54,0x24,0x20,0xf,0x68,0xf2,0xf,0x7f,0x27,0xf,0x68,0xc5,0x8a,0x44,0x11,0x4,0x8b,0x14,0x11,0x8b,0xda
.byte 0x81,0xe2,0x0,0xff,0xff,0xff,0xf,0x7f,0xfc,0xb,0xc2,0xf,0x6e,0xdb,0xf,0x60,0xe6,0xc1,0xc8,0x8,0x8b,0xd3
.byte 0xd1,0xeb,0x23,0xd0,0xd1,0xe8,0x81,0xe3,0x7f,0x7f,0x7f,0x7f,0x81,0xe2,0x1,0x1,0x1,0x1,0x25,0x7f,0x7f,0x7f
.byte 0x7f,0x3,0xd8,0x33,0xc0,0x3,0xd3,0x8b,0x19,0xf,0x6e,0xea,0xf,0x68,0xfe,0xf,0x7f,0x47,0x8,0xf,0x60,0xdd
.byte 0xf,0x6e,0xeb,0xf,0x7f,0xd8,0x8b,0xd3,0x8a,0x41,0x4,0x81,0xe2,0x0,0xff,0xff,0xff,0x90,0xb,0xc2,0x8b,0x54
.byte 0x24,0x30,0xc1,0xc8,0x8,0x3,0xf2,0x90,0x8b,0xd3,0xd1,0xeb,0x23,0xd0,0xd1,0xe8,0x81,0xe3,0x7f,0x7f,0x7f,0x7f
.byte 0x81,0xe2,0x1,0x1,0x1,0x1,0x25,0x7f,0x7f,0x7f,0x7f,0x3,0xd8,0x33,0xc0,0x3,0xda,0x8b,0x54,0x24,0x24,0xf
.byte 0x6e,0xf3,0xf,0xdb,0xc1,0xf,0x7f,0x67,0x10,0xf,0x60,0xee,0xf,0x6f,0x35
.long 0x0 + B_ONE

.byte 0xf,0x7f,0xec,0xf,0x7f,0x7f,0x18,0xf,0xdb,0xc6,0xf,0xdb,0x1d
.long 0x0 + B_XFE

.byte 0xf,0xdb,0xe2,0xf,0xdb,0x2d
.long 0x0 + B_XFE

.byte 0xf,0x73,0xd3,0x1,0xf,0xfc,0xd8,0xf,0x73,0xd5,0x1,0xf,0xdb,0xe6,0x3,0xfa,0xf,0x6f,0x6,0xf,0xfc,0xe5
.byte 0xf,0x7f,0xc6,0xf,0x7f,0xcd,0xf,0x60,0xea,0xf,0x7f,0xcf,0xf,0xdb,0xd
.long 0x0 + B_XFE

.byte 0xf,0x60,0xc5,0xf,0x68,0xf5,0x8b,0x54,0x24,0x14,0xf,0x7f,0x7,0xf,0x73,0xd1,0x1,0xf,0x7f,0x77,0x8,0xf
.byte 0x68,0xfa,0xf,0x6f,0x46,0x8,0xf,0xfc,0xcb,0xf,0x7f,0xc6,0x2b,0xca,0xf,0xdb,0x15
.long 0x0 + B_XFE

.byte 0xf,0x60,0xf7,0xf,0x73,0xd2,0x1,0x83,0xc1,0x4,0xf,0x7f,0x77,0x10,0xf,0x68,0xc7,0x8b,0x54,0x24,0x30,0xf
.byte 0xfc,0xd4,0x3,0xf2,0x8b,0x54,0x24,0x24,0xf,0x7f,0x47,0x18,0xf,0x7f,0xcb,0xf,0x6f,0x6,0xf,0x60,0xda,0x3
.byte 0xfa,0x8b,0x54,0x24,0x30,0xf,0x6f,0x7e,0x8,0xf,0x68,0xca,0xf,0x7f,0xc5,0xf,0x60,0xc3,0x3,0xf2,0x8b,0x54
.byte 0x24,0x24,0xf,0x7f,0x7,0xf,0x68,0xeb,0xf,0x6f,0x6,0xf,0x7f,0xfe,0xf,0x7f,0x6f,0x8,0xf,0x60,0xf9,0xf
.byte 0x6f,0x56,0x8,0xf,0x68,0xf1,0xf,0x7f,0x7f,0x10,0xf,0x7f,0xc7,0xf,0x7f,0x77,0x18,0xf,0x60,0xc3,0x3,0xfa
.byte 0x8b,0x54,0x24,0x28,0xf,0x68,0xfb,0x2b,0xf2,0xf,0x7f,0x7,0xf,0x7f,0xd6,0xf,0x7f,0x7f,0x8,0xf,0x60,0xd1
.byte 0x8b,0x54,0x24,0x3c,0xf,0x68,0xf1,0xf,0x7f,0x57,0x10,0xf,0x7f,0x77,0x18,0x2b,0xfa,0x8b,0x54,0x24,0x20,0x4d
.byte 0xf,0x85,0xaf,0xfd,0xff,0xff,0x8b,0x6c,0x24,0x18,0x90,0x83,0xe5,0x3,0x75,0x6,0xbd,0x4,0x0,0x0,0x0,0x90
.byte 0x4d,0xf,0x86,0xe2,0x0,0x0,0x0,0x33,0xc0,0x33,0xdb,0x8a,0x41,0x1,0x8a,0x19,0x2,0xc3,0xd0,0xd8,0xc1,0xe0
.byte 0x8,0xb,0xc3,0xf,0x6e,0xc8,0x33,0xc0,0x8a,0x44,0x11,0x1,0x8a,0x1c,0x11,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8
.byte 0xb,0xc3,0xf,0x6e,0xd0,0xf,0x60,0xd1,0xf,0x6e,0x6,0xf,0x60,0xc2,0xf,0x7f,0x7,0x8b,0x54,0x24,0x30,0x3
.byte 0xf2,0xf,0x6e,0x26,0xf,0x6e,0x2c,0x16,0xf,0x6e,0x34,0x56,0x2b,0xf2,0xf,0x60,0xe2,0x8b,0x54,0x24,0x24,0x3
.byte 0xfa,0xf,0x7f,0x27,0x3,0xfa,0x8b,0x54,0x24,0x14,0x3,0xca,0x8b,0x54,0x24,0x20,0x33,0xc0,0x8a,0x41,0x1,0x8a
.byte 0x19,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb,0xc3,0xf,0x6e,0xd8,0x33,0xc0,0x8a,0x44,0x11,0x1,0x8a,0x1c,0x11
.byte 0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb,0xc3,0xf,0x6e,0xc8,0xf,0x60,0xcb,0xf,0x7f,0xd7,0xf,0xdb,0xf9,0xf
.byte 0xdb,0x3d
.long 0x0 + B_ONE

.byte 0xf,0xdb,0x15
.long 0x0 + B_XFE

.byte 0xf,0xdb,0xd
.long 0x0 + B_XFE

.byte 0xf,0x73,0xd2,0x1,0xf,0x73,0xd1,0x1,0xf,0xfc,0xca,0xf,0xfc,0xcf,0xf,0x60,0xe9,0xf,0x60,0xf1,0x83,0xc6
.byte 0x4,0x8b,0x54,0x24,0x24,0xf,0x7f,0x2f,0xf,0x7f,0x34,0x17,0x3,0xd2,0x2b,0xfa,0x83,0xc7,0x8,0x8b,0x54,0x24
.byte 0x14,0x2b,0xca,0x41,0x8b,0x54,0x24,0x20,0x4d,0xf,0x85,0x1e,0xff,0xff,0xff,0x33,0xdb,0x8b,0x54,0x24,0x14,0x33
.byte 0xc0,0x8a,0x4,0x11,0x8a,0x19,0x2,0xc3,0xd0,0xd8,0xf,0x6e,0xcb,0xf,0x6e,0xd0,0x8b,0x6c,0x24,0x20,0x3,0xe9
.byte 0x41,0x8a,0x44,0x15,0x0,0x8a,0x5d,0x0,0x2,0xc3,0xd0,0xd8,0xf,0x6e,0xdb,0xf,0x6e,0xe0,0x8b,0x54,0x24,0x30
.byte 0xf,0x6e,0x6,0x3,0xf2,0xf,0x60,0xdb,0xf,0x60,0xe4,0xf,0x60,0xc9,0xf,0x60,0xd2,0xf,0x60,0xd9,0xf,0x60
.byte 0xe2,0xf,0x60,0xc3,0xf,0x7f,0x7,0xf,0x6e,0x2e,0xf,0x6e,0x34,0x16,0xf,0x6e,0x3c,0x56,0x2b,0xf2,0x83,0xc6
.byte 0x4,0x8b,0x54,0x24,0x24,0x3,0xfa,0xf,0x60,0xeb,0xf,0x60,0xf4,0xf,0x60,0xfc,0xf,0x7f,0x2f,0xf,0x7f,0x34
.byte 0x17,0xf,0x7f,0x3c,0x57,0x2b,0xfa,0x83,0xc7,0x8,0x3,0x74,0x24,0x10,0x3,0x7c,0x24,0xc,0x8b,0x54,0x24,0x38
.byte 0x8b,0x6c,0x24,0x1c,0x3,0xca,0x33,0xc0,0x8b,0x54,0x24,0x20,0x4d,0x89,0x6c,0x24,0x1c,0x8b,0x6c,0x24,0x34,0xf
.byte 0x85,0x10,0xfc,0xff,0xff,0x33,0xc0,0x8b,0x1c,0x11,0x8a,0x44,0x11,0x4,0x8b,0xd3,0x81,0xe2,0x0,0xff,0xff,0xff
.byte 0x90,0xb,0xc2,0x90,0xf,0x6e,0xcb,0xc1,0xc8,0x8,0x8b,0xd3,0xd1,0xeb,0x23,0xd0,0xd1,0xe8,0x81,0xe3,0x7f,0x7f
.byte 0x7f,0x7f,0x81,0xe2,0x1,0x1,0x1,0x1,0x25,0x7f,0x7f,0x7f,0x7f,0x3,0xd8,0x33,0xc0,0x3,0xd3,0x8a,0x41,0x4
.byte 0xf,0x6e,0xd2,0xf,0x6f,0x6,0xf,0x60,0xca,0x8b,0x11,0x90,0x8b,0xda,0x81,0xe2,0x0,0xff,0xff,0xff,0xb,0xc2
.byte 0x8b,0xd3,0xf,0x6e,0xd3,0xf,0x7f,0xc4,0x90,0x81,0xe3,0xfe,0xfe,0xfe,0xfe,0xc1,0xc8,0x8,0xf,0x7f,0xcd,0xd1
.byte 0xeb,0x23,0xd0,0xd1,0xe8,0x81,0xe2,0x1,0x1,0x1,0x1,0x25,0x7f,0x7f,0x7f,0x7f,0x3,0xda,0x3,0xd8,0x33,0xc0
.byte 0xf,0x6e,0xdb,0xf,0x7f,0xce,0xf,0x6f,0x7e,0x8,0xf,0x60,0xd3,0xf,0x60,0xea,0x8b,0x54,0x24,0x30,0xf,0x60
.byte 0xe5,0x3,0xf2,0xf,0x68,0xf2,0x8b,0x54,0x24,0x24,0xf,0x7f,0x27,0xf,0x68,0xc5,0xf,0x7f,0xfc,0xf,0x60,0xfe
.byte 0xf,0x7f,0x47,0x8,0xf,0x68,0xe6,0xf,0x7f,0x7f,0x10,0xf,0x7f,0x67,0x18,0x3,0xfa,0x8b,0x54,0x24,0x30,0xf
.byte 0x6f,0x6,0xf,0x6f,0x4e,0x8,0xf,0x7f,0xc2,0xf,0x60,0xc5,0x3,0xf2,0xf,0x68,0xd5,0xf,0x7f,0xcb,0xf,0x7f
.byte 0x7,0xf,0x60,0xce,0xf,0x6f,0x6,0xf,0x68,0xde,0xf,0x7f,0x57,0x8,0xf,0x7f,0xc2,0xf,0x7f,0x4f,0x10,0xf
.byte 0x60,0xc5,0xf,0x6f,0x4e,0x8,0xf,0x68,0xd5,0xf,0x7f,0x5f,0x18,0xf,0x7f,0xcb,0x3,0xf2,0x8b,0x54,0x24,0x24
.byte 0x3,0xfa,0xf,0x60,0xce,0xf,0x6f,0x26,0xf,0x68,0xde,0xf,0x7f,0x7,0xf,0x7f,0xe0,0xf,0x7f,0x57,0x8,0xf
.byte 0x60,0xc5,0xf,0x6f,0x7e,0x8,0xf,0x68,0xe5,0xf,0x7f,0x4f,0x10,0xf,0x7f,0xf9,0xf,0x7f,0x5f,0x18,0xf,0x60
.byte 0xce,0x3,0xfa,0x8b,0x54,0x24,0x28,0x2b,0xf2,0xf,0x68,0xfe,0xf,0x7f,0x7,0xf,0x7f,0x67,0x8,0xf,0x7f,0x4f
.byte 0x10,0xf,0x7f,0x7f,0x18,0x8b,0x54,0x24,0x3c,0x83,0xc1,0x4,0x2b,0xfa,0x8b,0x54,0x24,0x20,0x4d,0xf,0x85,0xa2
.byte 0xfe,0xff,0xff,0x8b,0x6c,0x24,0x18,0x90,0x83,0xe5,0x3,0x75,0x6,0xbd,0x4,0x0,0x0,0x0,0x90,0x4d,0x76,0x7a
.byte 0x33,0xc0,0x33,0xdb,0x8a,0x41,0x1,0x8a,0x19,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb,0xc3,0xf,0x6e,0xc8,0x33
.byte 0xc0,0x8a,0x44,0x11,0x1,0x8a,0x1c,0x11,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb,0xc3,0xf,0x6e,0xd0,0xf,0x60
.byte 0xd1,0xf,0x6e,0x6,0xf,0x60,0xc2,0xf,0x7f,0x7,0x8b,0x54,0x24,0x30,0x3,0xf2,0xf,0x6e,0x26,0xf,0x6e,0x2c
.byte 0x16,0xf,0x6e,0x34,0x56,0x2b,0xf2,0xf,0x60,0xe2,0x8b,0x54,0x24,0x24,0x3,0xfa,0xf,0x7f,0x27,0x3,0xfa,0xf
.byte 0x60,0xea,0xf,0x60,0xf2,0x83,0xc6,0x4,0x8b,0x54,0x24,0x24,0xf,0x7f,0x2f,0xf,0x7f,0x34,0x17,0x3,0xd2,0x2b
.byte 0xfa,0x83,0xc7,0x8,0x8b,0x54,0x24,0x20,0x41,0x4d,0x75,0x86,0x33,0xc0,0x33,0xdb,0x8a,0x1c,0x11,0x8b,0x54,0x24
.byte 0x30,0x8a,0x1,0x8b,0x4c,0x24,0x24,0xc1,0xe0,0x8,0x90,0xb,0xc3,0x90,0x8b,0xd8,0x90,0xc1,0xe3,0x10,0x90,0xb
.byte 0xd8,0x90,0xf,0x6e,0xc3,0xf,0x6e,0xe,0xf,0x6e,0x14,0x16,0xf,0x60,0xc8,0xf,0x6e,0x1c,0x56,0xf,0x60,0xd0
.byte 0x3,0xf2,0xf,0x60,0xd8,0xf,0x7f,0xf,0xf,0x6e,0x24,0x56,0x3,0xf9,0xf,0x60,0xe0,0x90,0x90,0xf,0x7f,0x17
.byte 0xf,0x7f,0x1c,0xf,0xf,0x7f,0x24,0x4f,0x33,0xc0,0xeb,0x0,0x8b,0x1c,0x24,0x83,0xc4,0x40,0x3,0xe3,0x5b,0x5e
.byte 0x5f,0x5d,0xc3,0x8b,0x1d
.long 0x0 + family

.byte 0x83,0xfb,0x6,0x74,0x7d,0x8b,0x4c,0x24,0x4,0x8b,0x54,0x24,0x8,0x8b,0x6c,0x24,0x18,0x90,0x33,0xc0,0x33,0xdb
.byte 0x8a,0x46,0x1,0x90,0x8a,0x21,0x8a,0x5e,0x3,0xc1,0xe0,0x10,0x8a,0x39,0xc1,0xe3,0x10,0x8a,0x6,0x8a,0x22,0x8a
.byte 0x5e,0x2,0x8a,0x3a,0x83,0xc6,0x4,0x41,0x42,0x89,0x7,0x4d,0x89,0x5f,0x4,0x8d,0x7f,0x8,0x75,0xd6,0x8b,0x6c
.byte 0x24,0x18,0x8b,0x54,0x24,0x10,0x8b,0x4c,0x24,0xc,0x8b,0x44,0x24,0x1c,0x3,0xf2,0x48,0x89,0x44,0x24,0x1c,0x3
.byte 0xf9,0x8b,0x4c,0x24,0x4,0x8b,0x54,0x24,0x8,0xa9,0x3,0x0,0x0,0x0,0x75,0xaa,0x8b,0x5c,0x24,0x14,0x90,0x3
.byte 0xcb,0x3,0xd3,0x89,0x4c,0x24,0x4,0x89,0x54,0x24,0x8,0xb,0xc0,0x75,0x95,0xe9,0x6b,0xff,0xff,0xff,0x8b,0x4c
.byte 0x24,0x4,0x8b,0x54,0x24,0x8,0x8b,0x6c,0x24,0x18,0x90,0xf,0xb6,0x1e,0x41,0xf,0xb6,0x46,0x1,0x83,0xc7,0x8
.byte 0xc1,0xe0,0x10,0x83,0xc6,0x4,0xb,0xd8,0xf,0xb6,0x41,0xff,0xc1,0xe0,0x18,0x42,0xb,0xd8,0xf,0xb6,0x42,0xff
.byte 0xc1,0xe0,0x8,0xb,0xd8,0xf,0xb6,0x46,0xff,0x89,0x5f,0xf8,0xf,0xb6,0x5e,0xfe,0xc1,0xe0,0x10,0xb,0xd8,0xf
.byte 0xb6,0x41,0xff,0xc1,0xe0,0x18,0xb,0xd8,0xf,0xb6,0x42,0xff,0xc1,0xe0,0x8,0xb,0xd8,0x4d,0x89,0x5f,0xfc,0x75
.byte 0xb2,0x8b,0x6c,0x24,0x18,0x8b,0x54,0x24,0x10,0x8b,0x4c,0x24,0xc,0x8b,0x44,0x24,0x1c,0x3,0xf2,0x48,0x89,0x44
.byte 0x24,0x1c,0x3,0xf9,0x8b,0x4c,0x24,0x4,0x8b,0x54,0x24,0x8,0xa9,0x3,0x0,0x0,0x0,0x75,0x8a,0x8b,0x5c,0x24
.byte 0x14,0x90,0x3,0xcb,0x3,0xd3,0x89,0x4c,0x24,0x4,0x89,0x54,0x24,0x8,0xb,0xc0,0xf,0x85,0x71,0xff,0xff,0xff
.byte 0xe9,0xca,0xfe,0xff,0xff,0x90,0x90,0x90,0x90

.text
.align 16

.globl C_YVU9toUYVY

C_YVU9toUYVY:

.byte 0x55,0x8b,0xec,0x57,0x56,0x53,0x8b,0xcc,0x83,0xe1,0x1f,0x83,0xe4,0xe0,0x83,0xec,0x40,0x89,0xc,0x24,0xb8,0x1
.byte 0x0,0x0,0x0,0x8b,0x55,0xc,0xc1,0xea,0x2,0xf,0x84,0x90,0x7,0x0,0x0,0x89,0x54,0x24,0x18,0x8b,0x4d,0x8
.byte 0x83,0xe1,0xfc,0xf,0x84,0x80,0x7,0x0,0x0,0x89,0x4c,0x24,0x1c,0x8b,0x5d,0x1c,0x8b,0x55,0xc,0x89,0x5c,0x24
.byte 0x30,0x83,0xe2,0xfc,0x2b,0xda,0x89,0x5c,0x24,0x10,0x8b,0x45,0x14,0x89,0x44,0x24,0x4,0x8b,0x45,0x18,0x89,0x44
.byte 0x24,0x8,0x8b,0x45,0x20,0x89,0x44,0x24,0x14,0x8b,0x4c,0x24,0x18,0x2b,0xc1,0x89,0x44,0x24,0x38,0x8b,0x4d,0x28
.byte 0x8b,0x5d,0xc,0x83,0xe3,0xfc,0x3,0xdb,0x89,0x4c,0x24,0x24,0x2b,0xcb,0x89,0x4c,0x24,0xc,0x8b,0x44,0x24,0x30
.byte 0x8d,0x4,0x40,0x83,0xe8,0x10,0x89,0x44,0x24,0x28,0x8b,0x4c,0x24,0x24,0x8d,0xc,0x49,0x83,0xe9,0x20,0x89,0x4c
.byte 0x24,0x3c,0x8b,0x75,0x10,0x8b,0x7d,0x24,0xb8,0x1,0x0,0x0,0x0,0xf7,0xc6,0x3,0x0,0x0,0x0,0xf,0x85,0x2
.byte 0x7,0x0,0x0,0xf7,0xc7,0x3,0x0,0x0,0x0,0xf,0x85,0xf6,0x6,0x0,0x0,0x83,0x3d
.long 0x0 + use_mmx

.byte 0x2,0x74,0x19,0xf7,0xc7,0x7,0x0,0x0,0x0,0xf,0x85,0xee,0x6,0x0,0x0,0x83,0x3d
.long 0x0 + use_mmx

.byte 0x1,0xf,0x85,0xe1,0x6,0x0,0x0,0x8b,0x4d,0x28,0x8b,0x5d,0xc,0xc1,0xe1,0x2,0x83,0xe3,0xfc,0x3,0xdb,0x2b
.byte 0xcb,0x89,0x4c,0x24,0xc,0x8b,0x5d,0x1c,0x8b,0x55,0xc,0xc1,0xe3,0x2,0x83,0xe2,0xfc,0x2b,0xda,0x89,0x5c,0x24
.byte 0x10,0x8b,0x55,0xc,0x83,0xfa,0x20,0xf,0x8c,0xa2,0x6,0x0,0x0,0x8b,0x55,0x8,0x83,0xfa,0x8,0xf,0x8c,0x96
.byte 0x6,0x0,0x0,0x8b,0x4c,0x24,0x4,0x8b,0x54,0x24,0x8,0x2b,0xd1,0x8b,0x6c,0x24,0x18,0xc1,0xed,0x2,0xf,0x84
.byte 0x7f,0x6,0x0,0x0,0x8b,0x44,0x24,0x1c,0x8b,0x5c,0x24,0x18,0x80,0xe3,0x3,0x75,0x7,0x4d,0xf,0x86,0x6b,0x6
.byte 0x0,0x0,0xc1,0xe8,0x2,0x90,0x48,0xf,0x86,0x60,0x6,0x0,0x0,0x89,0x44,0x24,0x1c,0x89,0x6c,0x24,0x34,0x89
.byte 0x54,0x24,0x20,0x33,0xc0,0x8b,0x54,0x24,0x20,0x8b,0x44,0x24,0x14,0x8b,0x5c,0x24,0x30,0xf,0x6e,0x4c,0x11,0x4
.byte 0xf,0x6e,0x2c,0x11,0xf,0x73,0xf1,0x20,0xf,0x6e,0x59,0x4,0xf,0x7f,0xec,0xf,0x6e,0x31,0xf,0xeb,0xcd,0xf
.byte 0xdb,0x2d
.long 0x0 + B_XFE

.byte 0xf,0x73,0xd1,0x8,0x3,0xc8,0xf,0x7f,0xe0,0xf,0xdb,0x25
.long 0x0 + B_ONE

.byte 0xf,0x73,0xd5,0x1,0xf,0xdb,0xe1,0xf,0x73,0xd1,0x1,0xf,0xdb,0xd
.long 0x0 + B_X7F

.byte 0xf,0xfc,0xec,0xf,0xfc,0xcd,0xf,0x7f,0xf4,0xf,0x6e,0x7c,0x11,0x4,0xf,0x73,0xf3,0x20,0xf,0x6e,0x2c,0x11
.byte 0xf,0xeb,0xde,0xf,0xdb,0x25
.long 0x0 + B_ONE

.byte 0xf,0x7f,0xf2,0xf,0xdb,0x35
.long 0x0 + B_XFE

.byte 0xf,0x73,0xd3,0x8,0xf,0xdb,0xe3,0xf,0x73,0xd3,0x1,0xf,0xdb,0x1d
.long 0x0 + B_X7F

.byte 0xf,0x73,0xd6,0x1,0xf,0xfc,0xde,0xf,0x60,0xc1,0xf,0xfc,0xdc,0xf,0x7f,0xee,0xf,0xdb,0x2d
.long 0x0 + B_XFE

.byte 0xf,0x73,0xf7,0x20,0xf,0xeb,0xfe,0xf,0x7f,0xf1,0xf,0xdb,0x35
.long 0x0 + B_ONE

.byte 0xf,0x73,0xd7,0x8,0xf,0x73,0xd5,0x1,0xf,0xdb,0xf7,0xf,0xdb,0x3d
.long 0x0 + B_XFE

.byte 0xf,0xfc,0xee,0xf,0x73,0xd7,0x1,0xf,0x60,0xd3,0xf,0xfc,0xef,0xf,0x7f,0xc6,0xf,0xdb,0x35
.long 0x0 + B_ONE

.byte 0xf,0x60,0xcd,0xf,0x6e,0x79,0x4,0xf,0x7f,0xc5,0xf,0xdb,0x2d
.long 0x0 + B_XFE

.byte 0xf,0xdb,0xf1,0xf,0xdb,0xd
.long 0x0 + B_XFE

.byte 0xf,0x73,0xd5,0x1,0xf,0x6e,0x21,0xf,0x73,0xd1,0x1,0x2b,0xc8,0xf,0xfc,0xcd,0x83,0xc1,0x4,0xf,0xfc,0xce
.byte 0xf,0x73,0xf7,0x20,0xf,0x7f,0xe6,0xf,0xdb,0x35
.long 0x0 + B_ONE

.byte 0xf,0xeb,0xfc,0xf,0x7f,0xe3,0xf,0x73,0xd7,0x8,0xf,0xdb,0x25
.long 0x0 + B_XFE

.byte 0xf,0xdb,0xf7,0xf,0xdb,0x3d
.long 0x0 + B_XFE

.byte 0xf,0x7f,0xd5,0xf,0xdb,0x2d
.long 0x0 + B_ONE

.byte 0xf,0x73,0xd4,0x1,0xf,0xfc,0xe6,0xf,0x7f,0xd6,0xf,0xdb,0x35
.long 0x0 + B_XFE

.byte 0xf,0x73,0xd7,0x1,0xf,0x73,0xd6,0x1,0xf,0xfc,0xe7,0xf,0x6f,0x3e,0xf,0x60,0xdc,0xf,0xdb,0xeb,0xf,0x73
.byte 0xd3,0x1,0xf,0xdb,0x1d
.long 0x0 + B_X7F

.byte 0xf,0xfc,0xf5,0xf,0xfc,0xde,0xf,0x7f,0xc4,0xf,0x60,0xc2,0xf,0x7f,0xcd,0xf,0x68,0xe2,0xf,0x7f,0xc2,0xf
.byte 0x6f,0x76,0x8,0xf,0x60,0xd7,0x3,0xf3,0xf,0x60,0xcb,0xf,0x68,0xeb,0xf,0x7f,0xc3,0xf,0x68,0xdf,0xf,0x7f
.byte 0xe7,0xf,0x7f,0x17,0xf,0x60,0xfe,0xf,0x7f,0x5f,0x8,0xf,0x7f,0xe2,0xf,0x6f,0x1e,0xf,0x68,0xd6,0xf,0x7f
.byte 0x7f,0x10,0xf,0x7f,0xc6,0xf,0x7f,0x57,0x18,0xf,0x60,0xc3,0x8b,0x54,0x24,0x24,0xf,0x68,0xf3,0x3,0xfa,0xf
.byte 0x7f,0xe2,0xf,0x6f,0x5e,0x8,0xf,0x7f,0xcf,0xf,0x7f,0x7,0xf,0x7f,0xc8,0xf,0x7f,0x77,0x8,0xf,0x60,0xd3
.byte 0xf,0x6f,0x34,0x1e,0xf,0x68,0xe3,0x3,0xf3,0xf,0x60,0xc6,0xf,0x7f,0x57,0x10,0xf,0x68,0xfe,0xf,0x6f,0x76
.byte 0x8,0xf,0x7f,0xeb,0xf,0x7f,0x67,0x18,0xf,0x60,0xde,0x3,0xfa,0xf,0x7f,0xea,0xf,0x6f,0x24,0x1e,0xf,0x68
.byte 0xd6,0xf,0x7f,0x7,0xf,0x7f,0xce,0xf,0x7f,0x7f,0x8,0xf,0x60,0xcc,0xf,0x7f,0x5f,0x10,0xf,0x68,0xf4,0xf
.byte 0x6f,0x7c,0x1e,0x8,0xf,0x7f,0xec,0xf,0x7f,0x57,0x18,0xf,0x60,0xef,0xf,0x7f,0xc,0x17,0xf,0x68,0xe7,0xf
.byte 0x7f,0x74,0x17,0x8,0xf,0x7f,0x6c,0x17,0x10,0xf,0x7f,0x64,0x17,0x18,0x3,0xd2,0x3,0xdb,0x2b,0xfa,0x2b,0xf3
.byte 0x8b,0x54,0x24,0x20,0x83,0xc6,0x10,0xd1,0xfb,0x83,0xc7,0x20,0x4d,0xf,0x85,0xad,0xfd,0xff,0xff,0x8b,0x6c,0x24
.byte 0x18,0x90,0x83,0xe5,0x3,0x75,0x6,0xbd,0x4,0x0,0x0,0x0,0x90,0x4d,0xf,0x86,0xe5,0x0,0x0,0x0,0x33,0xc0
.byte 0x33,0xdb,0x8a,0x41,0x1,0x8a,0x19,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb,0xc3,0xf,0x6e,0xc8,0x33,0xc0,0x8a
.byte 0x44,0x11,0x1,0x8a,0x1c,0x11,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb,0xc3,0xf,0x6e,0xd0,0xf,0x60,0xd1,0xf
.byte 0x7f,0xd0,0xf,0x60,0x6,0xf,0x7f,0x7,0x8b,0x54,0x24,0x30,0x3,0xf2,0xf,0x7f,0xd4,0xf,0x60,0x26,0xf,0x6f
.byte 0x2c,0x16,0xf,0x6f,0x34,0x56,0x2b,0xf2,0x8b,0x54,0x24,0x24,0x3,0xfa,0xf,0x7f,0x27,0x3,0xfa,0x8b,0x54,0x24
.byte 0x14,0x3,0xca,0x8b,0x54,0x24,0x20,0x33,0xc0,0x8a,0x41,0x1,0x8a,0x19,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb
.byte 0xc3,0xf,0x6e,0xd8,0x33,0xc0,0x8a,0x44,0x11,0x1,0x8a,0x1c,0x11,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb,0xc3
.byte 0xf,0x6e,0xc8,0xf,0x60,0xcb,0xf,0x7f,0xd7,0xf,0xdb,0xf9,0xf,0xdb,0x3d
.long 0x0 + B_ONE

.byte 0xf,0xdb,0x15
.long 0x0 + B_XFE

.byte 0xf,0xdb,0xd
.long 0x0 + B_XFE

.byte 0xf,0x73,0xd2,0x1,0xf,0x73,0xd1,0x1,0xf,0xfc,0xca,0xf,0xfc,0xcf,0xf,0x7f,0xcc,0xf,0x60,0xcd,0xf,0x60
.byte 0xe6,0x83,0xc6,0x4,0x8b,0x54,0x24,0x24,0xf,0x7f,0xf,0xf,0x7f,0x24,0x17,0x3,0xd2,0x2b,0xfa,0x83,0xc7,0x8
.byte 0x8b,0x54,0x24,0x14,0x2b,0xca,0x41,0x8b,0x54,0x24,0x20,0x4d,0xf,0x85,0x1b,0xff,0xff,0xff,0x33,0xdb,0x8b,0x54
.byte 0x24,0x14,0x33,0xc0,0x8a,0x4,0x11,0x8a,0x19,0x2,0xc3,0xd0,0xd8,0xf,0x6e,0xcb,0xf,0x6e,0xd0,0x8b,0x6c,0x24
.byte 0x20,0x3,0xe9,0x41,0x8a,0x44,0x15,0x0,0x8a,0x5d,0x0,0x2,0xc3,0xd0,0xd8,0xf,0x6e,0xdb,0xf,0x6e,0xe0,0x8b
.byte 0x54,0x24,0x30,0xf,0x60,0xdb,0xf,0x60,0xe4,0xf,0x60,0xc9,0xf,0x60,0xd2,0xf,0x60,0xd9,0xf,0x60,0xe2,0xf
.byte 0x7f,0xd8,0xf,0x60,0x6,0x3,0xf2,0xf,0x7f,0x7,0xf,0x60,0x1e,0xf,0x7f,0xe6,0xf,0x60,0x34,0x16,0xf,0x60
.byte 0x24,0x56,0x2b,0xf2,0x83,0xc6,0x4,0x8b,0x54,0x24,0x24,0x3,0xfa,0xf,0x7f,0x1f,0xf,0x7f,0x34,0x17,0xf,0x7f
.byte 0x24,0x57,0x2b,0xfa,0x83,0xc7,0x8,0x3,0x74,0x24,0x10,0x3,0x7c,0x24,0xc,0x8b,0x54,0x24,0x38,0x8b,0x6c,0x24
.byte 0x1c,0x3,0xca,0x33,0xc0,0x8b,0x54,0x24,0x20,0x4d,0x89,0x6c,0x24,0x1c,0x8b,0x6c,0x24,0x34,0xf,0x85,0x5,0xfc
.byte 0xff,0xff,0x8b,0x54,0x24,0x20,0x8b,0x44,0x24,0x24,0x8b,0x5c,0x24,0x30,0xf,0x6e,0x4c,0x11,0x4,0xf,0x6e,0x2c
.byte 0x11,0xf,0x73,0xf1,0x20,0xf,0x6e,0x59,0x4,0xf,0x7f,0xec,0xf,0x6e,0x31,0xf,0xeb,0xcd,0xf,0xdb,0x2d
.long 0x0 + B_XFE

.byte 0xf,0x73,0xd1,0x8,0xf,0x7f,0xe0,0xf,0xdb,0x25
.long 0x0 + B_ONE

.byte 0xf,0x73,0xd5,0x1,0xf,0xdb,0xe1,0xf,0x73,0xd1,0x1,0xf,0xdb,0xd
.long 0x0 + B_X7F

.byte 0xf,0xfc,0xec,0xf,0xfc,0xcd,0xf,0x7f,0xf4,0xf,0x73,0xf3,0x20,0xf,0xeb,0xde,0xf,0xdb,0x25
.long 0x0 + B_ONE

.byte 0xf,0x7f,0xf2,0xf,0xdb,0x35
.long 0x0 + B_XFE

.byte 0xf,0x73,0xd3,0x8,0xf,0xdb,0xe3,0xf,0x73,0xd3,0x1,0xf,0xdb,0x1d
.long 0x0 + B_X7F

.byte 0xf,0x73,0xd6,0x1,0xf,0xfc,0xde,0xf,0x60,0xc1,0xf,0xfc,0xdc,0xf,0x60,0xd3,0x83,0xc1,0x4,0xf,0x6f,0x3e
.byte 0xf,0x7f,0xc4,0xf,0x60,0xc2,0xf,0x68,0xe2,0xf,0x7f,0xc2,0xf,0x6f,0x76,0x8,0xf,0x60,0xd7,0x3,0xf3,0xf
.byte 0x7f,0xc3,0xf,0x68,0xdf,0xf,0x7f,0xe7,0xf,0x7f,0x17,0xf,0x60,0xfe,0xf,0x7f,0x5f,0x8,0xf,0x7f,0xe2,0xf
.byte 0x6f,0x1e,0xf,0x68,0xd6,0xf,0x7f,0xe5,0xf,0x7f,0xc1,0xf,0x7f,0x7f,0x10,0xf,0x7f,0xc6,0xf,0x7f,0x57,0x18
.byte 0xf,0x60,0xc3,0xf,0x68,0xf3,0x3,0xf8,0xf,0x7f,0xe2,0xf,0x6f,0x5e,0x8,0xf,0x7f,0xcf,0xf,0x7f,0x7,0xf
.byte 0x7f,0xc8,0xf,0x7f,0x77,0x8,0xf,0x60,0xd3,0xf,0x6f,0x34,0x1e,0xf,0x68,0xe3,0x3,0xf3,0xf,0x60,0xc6,0xf
.byte 0x7f,0x57,0x10,0xf,0x68,0xfe,0xf,0x6f,0x76,0x8,0xf,0x7f,0xeb,0xf,0x7f,0x67,0x18,0xf,0x60,0xde,0x3,0xf8
.byte 0xf,0x7f,0xea,0xf,0x6f,0x24,0x1e,0xf,0x68,0xd6,0xf,0x7f,0x7,0xf,0x7f,0xce,0xf,0x7f,0x7f,0x8,0xf,0x60
.byte 0xcc,0xf,0x7f,0x5f,0x10,0xf,0x68,0xf4,0xf,0x6f,0x7c,0x1e,0x8,0xf,0x7f,0xec,0xf,0x7f,0x57,0x18,0xf,0x60
.byte 0xef,0xf,0x7f,0xc,0x7,0xf,0x68,0xe7,0xf,0x7f,0x74,0x7,0x8,0xf,0x7f,0x6c,0x7,0x10,0xf,0x7f,0x64,0x7
.byte 0x18,0x3,0xc0,0x3,0xdb,0x2b,0xf8,0x2b,0xf3,0xd1,0xf8,0x83,0xc6,0x10,0xd1,0xfb,0x83,0xc7,0x20,0x4d,0xf,0x85
.byte 0x96,0xfe,0xff,0xff,0x8b,0x6c,0x24,0x18,0x90,0x83,0xe5,0x3,0x75,0x6,0xbd,0x4,0x0,0x0,0x0,0x90,0x4d,0x76
.byte 0x7d,0x33,0xc0,0x33,0xdb,0x8a,0x41,0x1,0x8a,0x19,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb,0xc3,0xf,0x6e,0xc8
.byte 0x33,0xc0,0x8a,0x44,0x11,0x1,0x8a,0x1c,0x11,0x2,0xc3,0xd0,0xd8,0xc1,0xe0,0x8,0xb,0xc3,0xf,0x6e,0xd0,0xf
.byte 0x60,0xd1,0xf,0x7f,0xd0,0xf,0x60,0x6,0xf,0x7f,0x7,0x8b,0x54,0x24,0x30,0x3,0xf2,0xf,0x7f,0xd4,0xf,0x60
.byte 0x26,0xf,0x6f,0x2c,0x16,0xf,0x6f,0x34,0x56,0x2b,0xf2,0x8b,0x54,0x24,0x24,0x3,0xfa,0xf,0x7f,0x27,0x3,0xfa
.byte 0xf,0x7f,0xd4,0xf,0x60,0xd5,0xf,0x60,0xe6,0x83,0xc6,0x4,0x8b,0x54,0x24,0x24,0xf,0x7f,0x17,0xf,0x7f,0x24
.byte 0x17,0x3,0xd2,0x2b,0xfa,0x83,0xc7,0x8,0x41,0x8b,0x54,0x24,0x20,0x4d,0x75,0x83,0x33,0xc0,0x33,0xdb,0x8a,0x1c
.byte 0x11,0x8b,0x54,0x24,0x30,0x8a,0x1,0x8b,0x4c,0x24,0x24,0xc1,0xe0,0x8,0x90,0xb,0xc3,0x90,0x8b,0xd8,0x90,0xc1
.byte 0xe3,0x10,0x90,0xb,0xd8,0x90,0xf,0x6e,0xc3,0xf,0x7f,0xc1,0xf,0x60,0xe,0xf,0x7f,0xc2,0xf,0x60,0x14,0x16
.byte 0xf,0x7f,0xc3,0xf,0x60,0x1c,0x56,0x3,0xf2,0xf,0x7f,0xf,0xf,0x60,0x4,0x56,0x3,0xf9,0xf,0x7f,0x17,0xf
.byte 0x7f,0x1c,0xf,0xf,0x7f,0x4,0x4f,0x33,0xc0,0x8b,0x1c,0x24,0x83,0xc4,0x40,0x3,0xe3,0x5b,0x5e,0x5f,0x5d,0xc3
.byte 0x8b,0x1d
.long 0x0 + family

.byte 0x83,0xfb,0x6,0x74,0x7d,0x8b,0x4c,0x24,0x4,0x8b,0x54,0x24,0x8,0x8b,0x6c,0x24,0x18,0x90,0x33,0xc0,0x33,0xdb
.byte 0x8a,0x66,0x1,0x90,0x8a,0x1,0x8a,0x7e,0x3,0xc1,0xe0,0x10,0x8a,0x19,0xc1,0xe3,0x10,0x8a,0x26,0x8a,0x2,0x8a
.byte 0x7e,0x2,0x8a,0x1a,0x83,0xc6,0x4,0x41,0x42,0x89,0x7,0x4d,0x89,0x5f,0x4,0x8d,0x7f,0x8,0x75,0xd6,0x8b,0x6c
.byte 0x24,0x18,0x8b,0x54,0x24,0x10,0x8b,0x4c,0x24,0xc,0x8b,0x44,0x24,0x1c,0x3,0xf2,0x48,0x89,0x44,0x24,0x1c,0x3
.byte 0xf9,0x8b,0x4c,0x24,0x4,0x8b,0x54,0x24,0x8,0xa9,0x3,0x0,0x0,0x0,0x75,0xaa,0x8b,0x5c,0x24,0x14,0x90,0x3
.byte 0xcb,0x3,0xd3,0x89,0x4c,0x24,0x4,0x89,0x54,0x24,0x8,0xb,0xc0,0x75,0x95,0xe9,0x6b,0xff,0xff,0xff,0x8b,0x4c
.byte 0x24,0x4,0x8b,0x54,0x24,0x8,0x8b,0x6c,0x24,0x18,0x90,0xf,0xb6,0x1e,0x41,0xf,0xb6,0x46,0x1,0x83,0xc7,0x8
.byte 0xc1,0xe0,0x18,0xc1,0xe3,0x8,0x83,0xc6,0x4,0xb,0xd8,0xf,0xb6,0x41,0xff,0xc1,0xe0,0x10,0x42,0xb,0xd8,0xf
.byte 0xb6,0x42,0xff,0xb,0xd8,0xf,0xb6,0x46,0xff,0x89,0x5f,0xf8,0xf,0xb6,0x5e,0xfe,0xc1,0xe0,0x18,0xc1,0xe3,0x8
.byte 0xb,0xd8,0xf,0xb6,0x41,0xff,0xc1,0xe0,0x10,0xb,0xd8,0xf,0xb6,0x42,0xff,0xb,0xd8,0x4d,0x89,0x5f,0xfc,0x75
.byte 0xb2,0x8b,0x6c,0x24,0x18,0x8b,0x54,0x24,0x10,0x8b,0x4c,0x24,0xc,0x8b,0x44,0x24,0x1c,0x3,0xf2,0x48,0x89,0x44
.byte 0x24,0x1c,0x3,0xf9,0x8b,0x4c,0x24,0x4,0x8b,0x54,0x24,0x8,0xa9,0x3,0x0,0x0,0x0,0x75,0x8a,0x8b,0x5c,0x24
.byte 0x14,0x90,0x3,0xcb,0x3,0xd3,0x89,0x4c,0x24,0x4,0x89,0x54,0x24,0x8,0xb,0xc0,0xf,0x85,0x71,0xff,0xff,0xff
.byte 0xe9,0xca,0xfe,0xff,0xff,0x90,0x90,0x90,0x90,0x90
