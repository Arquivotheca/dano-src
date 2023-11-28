
.data
.align 4

gaDither:

.byte 0x2,0x1,0x1,0x2,0x0,0x3,0x3,0x0

.text
.align 16

.globl ComputeDynamicClut

ComputeDynamicClut:

.byte 0x66,0xa1
.long 0x2 + gpalActivePalette

.byte 0x83,0xec,0x5c,0x66,0x3d,0x1,0x0,0x53,0x55,0x56,0x57,0xf,0x82,0x7a,0xb,0x0,0x0,0x66,0x3d,0x0,0x1,0xf
.byte 0x87,0x70,0xb,0x0,0x0,0x6a,0x1,0x68,0x0,0x8,0x0,0x0
call 0x0 + HiveGlobalAllocPtr

.byte 0x83,0xc4,0x8,0x89,0x44,0x24,0x28,0x85,0xc0,0x75,0xd,0x5f,0x5e,0x5d,0xb8,0x6,0x0,0x0,0x0,0x5b,0x83,0xc4
.byte 0x5c,0xc3,0xb9,0x0,0xff,0xff,0xff,0x8b,0xd1,0x83,0xc0,0x4,0xf,0xaf,0xd1,0x89,0x50,0xfc,0x41,0x81,0xf9,0x0
.byte 0x1,0x0,0x0,0x7c,0xec,0x33,0xc0,0x6a,0x1,0x66,0xa1
.long 0x2 + gpalActivePalette

.byte 0x8d,0x4,0x40,0xc1,0xe0,0x2,0x50
call 0x0 + HiveGlobalAllocPtr

.byte 0x8b,0xe8,0x6a,0x1,0x6a,0x40,0x89,0x6c,0x24,0x28
call 0x0 + HiveGlobalAllocPtr

.byte 0x8b,0xf0,0x6a,0x1,0x68,0x8,0x4,0x0,0x0,0x89,0x74,0x24,0x54
call 0x0 + HiveGlobalAllocPtr

.byte 0x8b,0xd8,0x6a,0x1,0x68,0x8,0x4,0x0,0x0,0x89,0x5c,0x24,0x4c
call 0x0 + HiveGlobalAllocPtr

.byte 0x8b,0xf8,0x6a,0x1,0x68,0x0,0x10,0x0,0x0,0x89,0x7c,0x24,0x68
call 0x0 + HiveGlobalAllocPtr

.byte 0x83,0xc4,0x28,0x89,0x44,0x24,0x44,0x85,0xf6,0xf,0x84,0x70,0xff,0xff,0xff,0x85,0xdb,0xf,0x84,0x68,0xff,0xff
.byte 0xff,0x85,0xff,0xf,0x84,0x60,0xff,0xff,0xff,0x85,0xc0,0xf,0x84,0x58,0xff,0xff,0xff,0x33,0xdb,0x66,0x39,0x1d
.long 0x2 + gpalActivePalette

.byte 0xf,0x86,0x9,0x1,0x0,0x0,0x83,0xc5,0x8,0x8b,0x3d
.long 0x4 + gpalActivePalette

.byte 0x33,0xc9,0x33,0xd2,0x83,0xc5,0xc,0x8a,0xc,0x9f,0x8a,0x54,0x9f,0x2,0x8b,0xf1,0x33,0xc9,0x8a,0x4c,0x9f,0x1
.byte 0x8b,0xfa,0x8d,0x4,0x76,0x69,0xd2,0xcb,0x41,0x0,0x0,0x8d,0x4,0xc0,0xc1,0xe0,0x2,0x2b,0xc6,0x8d,0x4,0x80
.byte 0x8d,0x4,0x86,0x8d,0x4,0x40,0x3,0xd0,0x8b,0xc1,0xc1,0xe0,0x7,0x3,0xc1,0xc1,0xe0,0x6,0x3,0xc1,0x8d,0x4
.byte 0x41,0x8d,0x84,0x42,0x0,0x0,0x10,0x0,0x99,0x81,0xe2,0xff,0xff,0x0,0x0,0x3,0xc2,0xc1,0xf8,0x10,0x89,0x45
.byte 0xec,0x8d,0x4,0xb6,0x8d,0x14,0xc0,0xc1,0xe2,0x4,0x2b,0xd6,0x8d,0x4,0x96,0x8d,0x14,0xc9,0x8d,0x14,0x91,0x8d
.byte 0x4,0x80,0xd1,0xe0,0x8d,0x14,0x91,0xc1,0xe2,0x7,0x2b,0xd1,0x69,0xc9,0x35,0x5e,0x0,0x0,0x2b,0xc2,0x8d,0x14
.byte 0x7f,0xc1,0xe2,0x5,0x3,0xd7,0x8d,0x14,0x92,0x8d,0x14,0x92,0xc1,0xe2,0x2,0x2b,0xd7,0x2b,0xc2,0x5,0x0,0x0
.byte 0x80,0x0,0x99,0x81,0xe2,0xff,0xff,0x0,0x0,0x3,0xc2,0xc1,0xf8,0x10,0x89,0x45,0xf0,0x8d,0x4,0xbf,0x8d,0x4
.byte 0xc0,0xc1,0xe0,0x4,0x2b,0xc7,0x8d,0x4,0x87,0x8d,0x4,0x80,0xd1,0xe0,0x2b,0xc1,0x8b,0xce,0xc1,0xe1,0x7,0x3
.byte 0xce,0x8d,0x34,0x8e,0x8d,0x14,0xf6,0x2b,0xc2,0x5,0x0,0x0,0x80,0x0,0x99,0x81,0xe2,0xff,0xff,0x0,0x0,0x3
.byte 0xc2,0xc1,0xf8,0x10,0x89,0x45,0xf4,0x43,0x33,0xc0,0x66,0xa1
.long 0x2 + gpalActivePalette

.byte 0x3b,0xd8,0xf,0x8c,0x6,0xff,0xff,0xff,0x8b,0x44,0x24,0x44,0x8b,0x6c,0x24,0x18,0x8b,0x74,0x24,0x3c,0x33,0xd2
.byte 0x66,0x39,0x15
.long 0x2 + gpalActivePalette

.byte 0x76,0x2c,0x8b,0xfd,0x8b,0xf,0x83,0xc7,0xc,0xc1,0xf9,0x4,0x8b,0xd9,0x8b,0x2c,0x8e,0xc1,0xe3,0x8,0x3,0xdd
.byte 0x88,0x14,0x3,0x8b,0x1c,0x8e,0x43,0x42,0x89,0x1c,0x8e,0x33,0xc9,0x66,0x8b,0xd
.long 0x2 + gpalActivePalette

.byte 0x3b,0xd1,0x7c,0xd6,0x33,0xc9,0x89,0x4c,0x24,0x14,0x33,0xc0,0xc1,0xf9,0x4,0x89,0x44,0x24,0x10,0x89,0x4c,0x24
.byte 0x1c,0xeb,0x4,0x8b,0x4c,0x24,0x1c,0x8b,0x15
.long 0x0 + gpu8ClutTables

.byte 0x8b,0x74,0x24,0x28,0x24,0xf0,0x3,0xc1,0x33,0xed,0x8d,0x84,0x10,0x0,0x10,0x0,0x0,0x89,0x44,0x24,0x24,0x33
.byte 0xc0,0x33,0xff,0x66,0xa1
.long 0x2 + gpalActivePalette

.byte 0xb9,0xff,0xff,0xff,0xf,0x85,0xc0,0x89,0x44,0x24,0x50,0x7e,0x53,0x8b,0x54,0x24,0x18,0x83,0xc2,0x8,0x8b,0x5a
.byte 0xf8,0x8b,0xc5,0x2b,0xc3,0x8b,0x84,0x86,0x0,0x4,0x0,0x0,0x8d,0x4,0x40,0xd1,0xe8,0x3b,0xc1,0x77,0x29,0x8b
.byte 0x5c,0x24,0x14,0x2b,0x5a,0xfc,0x3,0x84,0x9e,0x0,0x4,0x0,0x0,0x3b,0xc1,0x77,0x17,0x8b,0x5c,0x24,0x10,0x2b
.byte 0x1a,0x3,0x84,0x9e,0x0,0x4,0x0,0x0,0x3b,0xc1,0x73,0x6,0x8b,0xc8,0x89,0x7c,0x24,0x20,0x8b,0x44,0x24,0x50
.byte 0x47,0x83,0xc2,0xc,0x3b,0xf8,0x7c,0xb4,0x8b,0x44,0x24,0x24,0x8a,0x4c,0x24,0x20,0x83,0xc5,0x10,0x88,0x8,0x5
.byte 0x0,0x10,0x0,0x0,0x81,0xfd,0x0,0x1,0x0,0x0,0x89,0x44,0x24,0x24,0xf,0x8c,0x74,0xff,0xff,0xff,0x8b,0x44
.byte 0x24,0x10,0x83,0xc0,0x10,0x3d,0x0,0x1,0x0,0x0,0x89,0x44,0x24,0x10,0xf,0x8c,0x3f,0xff,0xff,0xff,0x8b,0x4c
.byte 0x24,0x14,0x83,0xc1,0x10,0x81,0xf9,0x0,0x1,0x0,0x0,0x89,0x4c,0x24,0x14,0xf,0x8c,0x19,0xff,0xff,0xff,0x33
.byte 0xff,0x89,0x7c,0x24,0x14,0x33,0xf6,0xc1,0xff,0x4,0x89,0x74,0x24,0x10,0x89,0x7c,0x24,0x1c,0xeb,0x4,0x8b,0x7c
.byte 0x24,0x1c,0x83,0xe6,0xf0,0xb8,0x1,0x0,0x0,0x0,0x89,0x44,0x24,0x30,0x89,0x74,0x24,0x48,0x8d,0x94,0x37,0x0
.byte 0x11,0x0,0x0,0xc7,0x44,0x24,0x38,0x6,0x0,0x0,0x0,0x89,0x54,0x24,0x34,0xeb,0x8,0x8b,0x74,0x24,0x48,0x8b
.byte 0x7c,0x24,0x1c,0x8b,0x4c,0x24,0x3c,0x8b,0x5c,0x24,0x44,0xc1,0xf8,0x4,0x8b,0x14,0x81,0x8b,0xc8,0xc1,0xe1,0x8
.byte 0x3,0xcb,0x8d,0x58,0x1,0xc1,0xe3,0xc,0x3,0xdf,0x83,0xc2,0x2,0x3,0xde,0x8b,0x35
.long 0x0 + gpu8ClutTables

.byte 0x8b,0x7c,0x24,0x1c,0x89,0x4c,0x24,0x24,0x8a,0x1c,0x33,0x8b,0x74,0x24,0x48,0x88,0x5c,0x11,0xfe,0x33,0xdb,0x83
.byte 0xf8,0xf,0xf,0x9c,0xc3,0x8d,0x44,0x3,0x1,0xc1,0xe0,0xc,0x3,0xc7,0x8b,0x7c,0x24,0x2c,0x3,0xc6,0x8b,0x35
.long 0x0 + gpu8ClutTables

.byte 0x8a,0x4,0x30,0x33,0xf6,0x85,0xd2,0x88,0x44,0x11,0xff,0x7e,0x76,0x8b,0x44,0x24,0x40,0x2b,0xc7,0x89,0x44,0x24
.byte 0x50,0x33,0xc0,0x8b,0x5c,0x24,0x10,0x8a,0x4,0x31,0x83,0xc7,0x4,0x8d,0xc,0x40,0x8b,0x44,0x24,0x18,0x8b,0x6c
.byte 0x88,0x8,0x8d,0x4,0x88,0x2b,0xdd,0x8b,0x6c,0x24,0x14,0x8b,0x8,0x2b,0x68,0x4,0x8b,0x44,0x24,0x28,0x8b,0x9c
.byte 0x98,0x0,0x4,0x0,0x0,0x3,0x9c,0xa8,0x0,0x4,0x0,0x0,0x8b,0x6c,0x24,0x30,0x2b,0xe9,0x8d,0xc,0x49,0xd1
.byte 0xe1,0x8b,0x84,0xa8,0x0,0x4,0x0,0x0,0x8d,0x4,0x40,0x8d,0x4,0x58,0x89,0x47,0xfc,0x8b,0x44,0x24,0x38,0x2b
.byte 0xc1,0x8b,0x4c,0x24,0x50,0x83,0xc0,0x3,0x46,0x89,0x44,0x39,0xfc,0x8b,0x4c,0x24,0x24,0x3b,0xf2,0x7c,0x94,0xa1
.long 0x0 + gpu8ClutTables

.byte 0x8b,0x74,0x24,0x34,0x8b,0x5c,0x24,0x20,0x3,0xf0,0x89,0x74,0x24,0x24,0xc7,0x44,0x24,0x50,0xf,0x0,0x0,0x0
.byte 0x8b,0x44,0x24,0x2c,0x33,0xff,0x85,0xd2,0xc7,0x44,0x24,0x4c,0xff,0xff,0xff,0xf,0x7e,0x3e,0x8b,0x6c,0x24,0x40
.byte 0x8b,0xf0,0x2b,0xee,0x8b,0x30,0x8b,0x5c,0x24,0x4c,0x3b,0xf3,0x73,0xf,0x33,0xdb,0x89,0x74,0x24,0x4c,0x8a,0x1c
.byte 0x39,0x89,0x5c,0x24,0x20,0xeb,0x4,0x8b,0x5c,0x24,0x20,0x3,0x34,0x28,0x83,0xc0,0x4,0x89,0x70,0xfc,0x8b,0x74
.byte 0x28,0xfc,0x83,0xc6,0x6,0x47,0x89,0x74,0x28,0xfc,0x3b,0xfa,0x7c,0xca,0x8b,0x44,0x24,0x24,0x88,0x18,0x5,0x0
.byte 0x1,0x0,0x0,0x89,0x44,0x24,0x24,0x8b,0x44,0x24,0x50,0x48,0x89,0x44,0x24,0x50,0x75,0x96,0x8b,0x4c,0x24,0x38
.byte 0x8b,0x44,0x24,0x30,0x8b,0x7c,0x24,0x34,0x83,0xc1,0x60,0x83,0xc0,0x10,0x81,0xc7,0x0,0x10,0x0,0x0,0x81,0xf9
.byte 0x0,0x6,0x0,0x0,0x89,0x44,0x24,0x30,0x89,0x7c,0x24,0x34,0x89,0x4c,0x24,0x38,0xf,0x82,0x68,0xfe,0xff,0xff
.byte 0x8b,0x74,0x24,0x10,0x83,0xc6,0x10,0x81,0xfe,0x0,0x1,0x0,0x0,0x89,0x74,0x24,0x10,0xf,0x8c,0x28,0xfe,0xff
.byte 0xff,0x8b,0x7c,0x24,0x14,0x83,0xc7,0x10,0x81,0xff,0x0,0x1,0x0,0x0,0x89,0x7c,0x24,0x14,0xf,0x8c,0x2,0xfe
.byte 0xff,0xff,0x33,0xc0,0x6a,0x1,0x89,0x44,0x24,0x18,0x89,0x44,0x24,0x38,0x89,0x44,0x24,0x3c
call 0x0 + srand

.byte 0x8b,0x6c,0x24,0x2c,0x83,0xc4,0x4,0xc7,0x44,0x24,0x4c,0x20,0x0,0x0,0x0,0xb9,0x6,0x0,0x0,0x0,0xb8,0xff
.byte 0x7f,0x0,0x0,0x8d,0x7c,0x24,0x54,0xf3,0xab
call 0x0 + rand

.byte 0x8d,0xc,0x40,0x33,0xff,0x89,0x7c,0x24,0x50,0x8d,0xc,0x88,0x8d,0x14,0xc9,0x8d,0xc,0x50,0xb8,0x3,0x0,0x1
.byte 0x80,0xf7,0xe9,0x3,0xd1,0xc1,0xfa,0xe,0x8b,0xc2,0xc1,0xe8,0x1f,0x8d,0x5c,0x2,0xa,0x8b,0x54,0x24,0x18,0x8d
.byte 0xc,0x5b,0x8d,0x4,0x8a,0x83,0xc2,0x4,0x8b,0x8,0x89,0x4c,0x24,0x30,0x8b,0x48,0x4,0x8b,0x40,0x8,0x89,0x4c
.byte 0x24,0x24,0x89,0x44,0x24,0x48,0x3b,0xfb,0x74,0x6f,0x8b,0x4c,0x24,0x48,0x8b,0x72,0x4,0x8b,0x42,0xfc,0x2b,0xce
.byte 0x8b,0x74,0x24,0x30,0x2b,0xf0,0x8b,0x84,0x8d,0x0,0x4,0x0,0x0,0x8b,0x8c,0xb5,0x0,0x4,0x0,0x0,0x8b,0x32
.byte 0x3,0xc1,0x8b,0x4c,0x24,0x24,0x2b,0xce,0x8b,0xb4,0x8d,0x0,0x4,0x0,0x0,0x3,0xc6,0x33,0xc9,0x8d,0x74,0x24
.byte 0x54,0x3b,0x6,0x72,0xb,0x41,0x83,0xc6,0x4,0x83,0xf9,0x6,0x7c,0xf3,0xeb,0x27,0x83,0xf9,0x5,0x7d,0x1e,0xbf
.byte 0x5,0x0,0x0,0x0,0x8d,0x74,0x24,0x68,0x2b,0xf9,0x8b,0x6e,0xfc,0x89,0x2e,0x83,0xc6,0xfc,0x4f,0x75,0xf5,0x8b
.byte 0x6c,0x24,0x28,0x8b,0x7c,0x24,0x50,0x89,0x44,0x8c,0x54,0x47,0x83,0xc2,0xc,0x81,0xff,0xff,0x0,0x0,0x0,0x89
.byte 0x7c,0x24,0x50,0xf,0x8c,0x79,0xff,0xff,0xff,0x8b,0x5c,0x24,0x14,0x8d,0x74,0x24,0x54,0xbf,0x6,0x0,0x0,0x0
.byte 0x8b,0x16,0x52
call 0x0 + lsqrt

.byte 0x83,0xc4,0x4,0x3,0xd8,0x83,0xc6,0x4,0x4f,0x75,0xed,0x8b,0x44,0x24,0x4c,0x89,0x5c,0x24,0x14,0x48,0x89,0x44
.byte 0x24,0x4c,0xf,0x85,0xe9,0xfe,0xff,0xff,0xb8,0xab,0xaa,0xaa,0x2a,0xc7,0x44,0x24,0x50,0x80,0x0,0x0,0x0,0xf7
.byte 0xeb,0xc1,0xfa,0x5,0x8b,0xc2,0xc1,0xe8,0x1f,0x3,0xd0,0x89,0x54,0x24,0x14
call 0x0 + rand

.byte 0x8b,0xc8,0xc1,0xe1,0x8,0x2b,0xc8,0xb8,0x3,0x0,0x1,0x80,0xf7,0xe9,0x3,0xd1,0xc1,0xfa,0xe,0x8b,0xca,0xc1
.byte 0xe9,0x1f,0x3,0xd1,0x8b,0xfa
call 0x0 + rand

.byte 0x8b,0xc8,0xc1,0xe1,0x8,0x2b,0xc8,0xb8,0x3,0x0,0x1,0x80,0xf7,0xe9,0x3,0xd1,0xc1,0xfa,0xe,0x8b,0xc2,0xc1
.byte 0xe8,0x1f,0x3,0xd0,0x8b,0xf2
call 0x0 + rand

.byte 0x8b,0xc8,0xc1,0xe1,0x8,0x2b,0xc8,0xb8,0x3,0x0,0x1,0x80,0xf7,0xe9,0x3,0xd1,0xc1,0xfa,0xe,0x8b,0xca,0xc1
.byte 0xe9,0x1f,0x3,0xd1,0x8b,0xca,0x8b,0xd7,0x69,0xd2,0xcb,0x41,0x0,0x0,0x8d,0x4,0x49,0x8d,0x4,0xc0,0xc1,0xe0
.byte 0x2,0x2b,0xc1,0x8d,0x4,0x80,0x8d,0x4,0x81,0x8d,0x4,0x40,0x3,0xd0,0x8b,0xc6,0xc1,0xe0,0x7,0x3,0xc6,0xc1
.byte 0xe0,0x6,0x3,0xc6,0x8d,0x4,0x46,0x8d,0x84,0x42,0x0,0x0,0x10,0x0,0x99,0x81,0xe2,0xff,0xff,0x0,0x0,0x3
.byte 0xc2,0x8b,0xe8,0x8d,0x4,0x89,0xc1,0xfd,0x10,0x8d,0x14,0xc0,0xc1,0xe2,0x4,0x2b,0xd1,0x8d,0x4,0x91,0x8d,0x14
.byte 0xf6,0x8d,0x14,0x96,0x8d,0x4,0x80,0xd1,0xe0,0x8d,0x14,0x96,0xc1,0xe2,0x7,0x2b,0xd6,0x2b,0xc2,0x8d,0x14,0x7f
.byte 0xc1,0xe2,0x5,0x3,0xd7,0x8d,0x14,0x92,0x8d,0x14,0x92,0xc1,0xe2,0x2,0x2b,0xd7,0x2b,0xc2,0x5,0x0,0x0,0x80
.byte 0x0,0x99,0x81,0xe2,0xff,0xff,0x0,0x0,0x69,0xf6,0x35,0x5e,0x0,0x0,0x3,0xc2,0x8b,0xd1,0x8b,0xd8,0x8d,0x4
.byte 0xbf,0xc1,0xe2,0x7,0x8d,0x4,0xc0,0x3,0xd1,0xc1,0xe0,0x4,0x2b,0xc7,0x8d,0xc,0x91,0x83,0xc5,0x10,0x8d,0x3c
.byte 0x87,0x8d,0xc,0xc9,0xc1,0xfb,0x10,0x8d,0x4,0xbf,0xbf
.long 0x1 + gaDither

.byte 0xd1,0xe0,0x2b,0xc6,0x2b,0xc1,0x5,0x0,0x0,0x80,0x0,0x99,0x81,0xe2,0xff,0xff,0x0,0x0,0x3,0xc2,0x8b,0xf0
.byte 0xc1,0xfe,0x10,0xc1,0xe5,0x8,0x33,0xc9,0xb8,0x56,0x55,0x55,0x55,0x8a,0x4f,0xff,0xf,0xaf,0x4c,0x24,0x14,0xf7
.byte 0xe9,0x8b,0xc2,0xc1,0xe8,0x1f,0x3,0xd0,0xb8,0x56,0x55,0x55,0x55,0x3,0xd3,0x8b,0xca,0x33,0xd2,0x8a,0x17,0xf
.byte 0xaf,0x54,0x24,0x14,0xf7,0xea,0x8b,0xc2,0xc1,0xe8,0x1f,0x3,0xd0,0x3,0xd6,0x81,0xf9,0xff,0x0,0x0,0x0,0x7e
.byte 0x5,0xb9,0xff,0x0,0x0,0x0,0x81,0xfa,0xff,0x0,0x0,0x0,0x7e,0x5,0xba,0xff,0x0,0x0,0x0,0x83,0xe2,0xf0
.byte 0x8b,0xc5,0xc1,0xf9,0x4,0x3,0xc2,0x8b,0x15
.long 0x0 + gpu8ClutTables

.byte 0x3,0xc8,0x33,0xc0,0x83,0xc7,0x2,0x8a,0x4,0x11,0x8b,0x4c,0x24,0x18,0x8b,0xd3,0x8d,0x4,0x40,0x8d,0x4,0x81
.byte 0x8b,0x48,0x4,0x2b,0xd1,0x8b,0x4c,0x24,0x38,0x3,0xca,0x8b,0x50,0x8,0x89,0x4c,0x24,0x38,0x8b,0xce,0x2b,0xca
.byte 0x8b,0x54,0x24,0x34,0x3,0xd1,0x81,0xff
.long 0x9 + gaDither

.byte 0x89,0x54,0x24,0x34,0xf,0x8c,0x5f,0xff,0xff,0xff,0x8b,0x44,0x24,0x50,0x48,0x89,0x44,0x24,0x50,0xf,0x85,0x18
.byte 0xfe,0xff,0xff,0x8b,0x74,0x24,0x14,0xb8,0x56,0x55,0x55,0x55,0x8d,0xc,0x36,0xf7,0xe9,0x8b,0xc2,0xc1,0xe8,0x1f
.byte 0x3,0xd0,0xb8,0x56,0x55,0x55,0x55,0x8b,0xfa,0xf7,0xee,0x8b,0xca,0x8d,0x34,0x76,0xc1,0xe9,0x1f,0x3,0xd1,0xb8
.byte 0x56,0x55,0x55,0x55,0x8b,0xca,0xf7,0xee,0x8b,0xc2,0xc1,0xe8,0x1f,0x3,0xd0,0x8b,0xda,0x8b,0x15
.long 0x0 + gpu8ClutTables

.byte 0x8d,0x82,0x0,0x1c,0x1,0x0,0x89,0x44,0x24,0x50,0x5,0x0,0x4,0x0,0x0,0x89,0x44,0x24,0x4c,0x8b,0x44,0x24
.byte 0x38,0x5,0x0,0x1,0x0,0x0,0x99,0x81,0xe2,0xff,0x1,0x0,0x0,0x3,0xc2,0x8b,0x54,0x24,0x34,0x8b,0xf0,0xc7
.byte 0x44,0x24,0x34,0x0,0x1,0x0,0x0,0x8d,0x82,0x0,0x1,0x0,0x0,0x99,0x81,0xe2,0xff,0x1,0x0,0x0,0x3,0xc2
.byte 0xc1,0xfe,0x9,0xc1,0xf8,0x9,0x8b,0xee,0x2b,0xc6,0x8d,0x34,0x28,0x8d,0x14,0x29,0x8d,0x4,0x33,0x89,0x44,0x24
.byte 0x48,0x8d,0x4,0x37,0x89,0x44,0x24,0x38,0x2b,0xf9,0x8d,0x4,0x31,0x2b,0xd9,0x89,0x44,0x24,0x24,0x89,0x7c,0x24
.byte 0x30,0x89,0x5c,0x24,0x20,0x8b,0x4c,0x24,0x30,0x8b,0x5c,0x24,0x24,0x3,0xca,0x81,0xf9,0xff,0x0,0x0,0x0,0x7e
.byte 0x7,0xb8,0xff,0x0,0x0,0x0,0xeb,0xa,0x33,0xc0,0x85,0xc9,0xf,0x9c,0xc0,0x48,0x23,0xc1,0x33,0xc9,0x81,0xfa
.byte 0xff,0x0,0x0,0x0,0x8a,0xe8,0x7e,0x7,0xb8,0xff,0x0,0x0,0x0,0xeb,0xa,0x33,0xc0,0x85,0xd2,0xf,0x9c,0xc0
.byte 0x48,0x23,0xc2,0x25,0xff,0x0,0x0,0x0,0xb,0xc1,0xc1,0xe0,0x8,0x81,0xfd,0xff,0x0,0x0,0x0,0x7e,0x7,0xb9
.byte 0xff,0x0,0x0,0x0,0xeb,0xa,0x33,0xc9,0x85,0xed,0xf,0x9c,0xc1,0x49,0x23,0xcd,0x81,0xe1,0xff,0x0,0x0,0x0
.byte 0xb,0xc8,0x8b,0x44,0x24,0x20,0xc1,0xe1,0x8,0x8d,0x3c,0x10,0x81,0xff,0xff,0x0,0x0,0x0,0x7e,0x7,0xb8,0xff
.byte 0x0,0x0,0x0,0xeb,0xa,0x33,0xc0,0x85,0xff,0xf,0x9c,0xc0,0x48,0x23,0xc7,0x25,0xff,0x0,0x0,0x0,0xb,0xc8
.byte 0x81,0xfb,0xff,0x0,0x0,0x0,0x7e,0x7,0xb8,0xff,0x0,0x0,0x0,0xeb,0xa,0x33,0xc0,0x85,0xdb,0xf,0x9c,0xc0
.byte 0x48,0x23,0xc3,0x8b,0x7c,0x24,0x38,0x33,0xdb,0x81,0xff,0xff,0x0,0x0,0x0,0x8a,0xf8,0x7e,0x7,0xb8,0xff,0x0
.byte 0x0,0x0,0xeb,0xa,0x33,0xc0,0x85,0xff,0xf,0x9c,0xc0,0x48,0x23,0xc7,0x25,0xff,0x0,0x0,0x0,0xb,0xc3,0x8b
.byte 0x5c,0x24,0x48,0xc1,0xe0,0x8,0x81,0xfb,0xff,0x0,0x0,0x0,0x8b,0xf8,0x7e,0x7,0xb8,0xff,0x0,0x0,0x0,0xeb
.byte 0xa,0x33,0xc0,0x85,0xdb,0xf,0x9c,0xc0,0x48,0x23,0xc3,0x25,0xff,0x0,0x0,0x0,0xb,0xc7,0xc1,0xe0,0x8,0x81
.byte 0xfe,0xff,0x0,0x0,0x0,0x89,0x44,0x24,0x48,0x7e,0x7,0xb8,0xff,0x0,0x0,0x0,0xeb,0xa,0x33,0xc0,0x85,0xf6
.byte 0xf,0x9c,0xc0,0x48,0x23,0xc6,0x8b,0x7c,0x24,0x50,0x25,0xff,0x0,0x0,0x0,0xc1,0xe9,0x4,0x81,0xe1,0xf,0xf
.byte 0xf,0xf,0x89,0xf,0x8b,0x4c,0x24,0x4c,0x83,0xc7,0x4,0x83,0xc1,0x4,0x89,0x7c,0x24,0x50,0x8b,0x7c,0x24,0x48
.byte 0xb,0xc7,0x8b,0x7c,0x24,0x38,0x25,0xf0,0xf0,0xf0,0xf0,0x45,0x89,0x41,0xfc,0x8b,0x44,0x24,0x24,0x42,0x46,0x40
.byte 0x47,0x89,0x44,0x24,0x24,0x8b,0x44,0x24,0x34,0x43,0x48,0x89,0x4c,0x24,0x4c,0x89,0x7c,0x24,0x38,0x89,0x5c,0x24
.byte 0x48,0x89,0x44,0x24,0x34,0xf,0x85,0x84,0xfe,0xff,0xff,0x8b,0xd
.long 0x0 + gpu8ClutTables

.byte 0xbf,0x10,0x0,0x0,0x0,0x8d,0x91,0x0,0x1c,0x1,0x0,0x8b,0xf2,0x8d,0x82,0x0,0x4,0x0,0x0,0x8b,0xc8,0x2b
.byte 0xf0,0x8b,0x5a,0x40,0x89,0x1c,0xe,0x8b,0x58,0x40,0x89,0x19,0x83,0xc1,0x4,0x4f,0x75,0xef,0x8d,0x88,0xc4,0x3
.byte 0x0,0x0,0xbf,0xf,0x0,0x0,0x0,0x8b,0x9a,0xc0,0x3,0x0,0x0,0x89,0x1c,0x31,0x8b,0x98,0xc0,0x3,0x0,0x0
.byte 0x89,0x19,0x83,0xc1,0x4,0x4f,0x75,0xe9,0x33,0xed,0x8b,0xdd,0x33,0xff,0xc1,0xfb,0x4,0xa1
.long 0x0 + gpu8ClutTables

.byte 0x8b,0xd7,0x83,0xe2,0xf0,0xbe,0x20,0x0,0x0,0x0,0x3,0xd3,0x8d,0x8c,0x2,0x0,0x20,0x0,0x0,0x33,0xc0,0x8a
.byte 0x1,0x81,0xe9,0x0,0x1,0x0,0x0,0x4e,0x88,0x1,0x75,0xf5,0x8b,0xd
.long 0x0 + gpu8ClutTables

.byte 0x8d,0x84,0xa,0x0,0xfb,0x0,0x0,0x33,0xc9,0xba,0x20,0x0,0x0,0x0,0x8a,0x8,0x5,0x0,0x1,0x0,0x0,0x4a
.byte 0x88,0x8,0x75,0xf6,0x83,0xc7,0x10,0x81,0xff,0x0,0x1,0x0,0x0,0x7c,0xae,0x83,0xc5,0x10,0x81,0xfd,0x0,0x1
.byte 0x0,0x0,0x7c,0x9c,0x8b,0x54,0x24,0x18,0x52
call 0x0 + HiveGlobalFreePtr

.byte 0x8b,0x44,0x24,0x40,0x50
call 0x0 + HiveGlobalFreePtr

.byte 0x8b,0x4c,0x24,0x34,0x51
call 0x0 + HiveGlobalFreePtr

.byte 0x8b,0x54,0x24,0x4c,0x52
call 0x0 + HiveGlobalFreePtr

.byte 0x8b,0x44,0x24,0x54,0x50
call 0x0 + HiveGlobalFreePtr

.byte 0x8b,0x4c,0x24,0x3c,0x51
call 0x0 + HiveGlobalFreePtr

.byte 0x83,0xc4,0x18,0xb8,0x1,0x0,0x0,0x0,0x5f,0x5e,0x5d,0x5b,0x83,0xc4,0x5c,0xc3,0x5f,0x5e,0x5d,0xb8,0x2,0x0
.byte 0x0,0x0,0x5b,0x83,0xc4,0x5c,0xc3,0x90,0x90

.text
.align 16

lsqrt:

.byte 0x56,0x8b,0x74,0x24,0x8,0x57,0x33,0xc0,0xba,0x0,0x2,0x0,0x0,0x8d,0xc,0x2,0xd1,0xe9,0x8b,0xf9,0xf,0xaf
.byte 0xf9,0x3b,0xfe,0x76,0x4,0x8b,0xd1,0xeb,0x2,0x8b,0xc1,0x8b,0xca,0x2b,0xc8,0x83,0xf9,0x1,0x77,0xe3,0x5f,0x5e
.byte 0xc3,0x90,0x90,0x90
