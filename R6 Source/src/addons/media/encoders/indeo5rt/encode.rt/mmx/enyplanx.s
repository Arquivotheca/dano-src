
.text
.align 16

.text0:

.globl EncYPlane

EncYPlane:

.byte 0x56,0x57,0x55,0x53,0x81,0xec,0x68,0x3,0x0,0x0,0x8b,0xcc,0x83,0xe4,0xe0,0x89,0x8c,0x24,0xc,0x3,0x0,0x0
.byte 0x90,0x8b,0x81,0x7c,0x3,0x0,0x0,0x8b,0x99,0x80,0x3,0x0,0x0,0x89,0x84,0x24,0x48,0x3,0x0,0x0,0x89,0x9c
.byte 0x24,0x4c,0x3,0x0,0x0,0x8b,0x81,0x84,0x3,0x0,0x0,0x8b,0x99,0x88,0x3,0x0,0x0,0x89,0x84,0x24,0x50,0x3
.byte 0x0,0x0,0x89,0x9c,0x24,0x54,0x3,0x0,0x0,0x8b,0x81,0x8c,0x3,0x0,0x0,0x8b,0x99,0x90,0x3,0x0,0x0,0x89
.byte 0x84,0x24,0x58,0x3,0x0,0x0,0x89,0x9c,0x24,0x5c,0x3,0x0,0x0,0x8b,0x81,0x94,0x3,0x0,0x0,0x8b,0x99,0x98
.byte 0x3,0x0,0x0,0x89,0x84,0x24,0x60,0x3,0x0,0x0,0x89,0x9c,0x24,0x64,0x3,0x0,0x0,0x8b,0xac,0x24,0x48,0x3
.byte 0x0,0x0,0x33,0xd2,0x8b,0x5d,0x10,0x8b,0x45,0x4,0x89,0x9c,0x24,0x14,0x3,0x0,0x0,0x89,0x84,0x24,0x18,0x3
.byte 0x0,0x0,0x8b,0xc8,0x89,0x94,0x24,0x8,0x3,0x0,0x0,0xc1,0xe1,0x3,0x83,0xc0,0xf,0x89,0x8c,0x24,0x38,0x3
.byte 0x0,0x0,0x8b,0x5d,0x0,0xc1,0xe8,0x4,0x89,0x9c,0x24,0x2c,0x3,0x0,0x0,0x89,0x84,0x24,0x24,0x3,0x0,0x0
.byte 0xc1,0xe0,0x5,0x8d,0x1d
.long 0x0 + Slant88NBqtablex

.byte 0x8d,0x35
.long 0x0 + Slant88ZZPos

.byte 0x89,0x84,0x24,0x1c,0x3,0x0,0x0,0xc1,0xe0,0x3,0x89,0x9c,0x24,0x10,0x3,0x0,0x0,0x89,0x84,0x24,0x3c,0x3
.byte 0x0,0x0,0xc7,0x84,0x24,0x28,0x3,0x0,0x0,0x1,0x0,0x0,0x8,0x33,0xc9,0x8b,0x4,0x8e,0x8b,0x5c,0x8e,0x4
.byte 0x89,0x84,0x8c,0x4,0x2,0x0,0x0,0x89,0x9c,0x8c,0x8,0x2,0x0,0x0,0x83,0xc1,0x2,0x83,0xf9,0x40,0x7c,0xe3
.byte 0xb8,0x40,0x0,0x0,0x0,0x89,0x84,0x24,0x4,0x3,0x0,0x0,0x8b,0x84,0x24,0x24,0x3,0x0,0x0,0x33,0xdb,0x89
.byte 0x84,0x24,0x20,0x3,0x0,0x0,0x89,0x9c,0x24,0x30,0x3,0x0,0x0,0x8b,0x94,0x24,0x2c,0x3,0x0,0x0,0x8b,0xac
.byte 0x24,0x4c,0x3,0x0,0x0,0x83,0xfa,0x10,0x7f,0x46,0x8b,0x84,0x24,0x18,0x3,0x0,0x0,0x75,0xb,0x8b,0x9c,0x24
.byte 0x1c,0x3,0x0,0x0,0x3b,0xc3,0x74,0x32,0x8b,0xb4,0x24,0x50,0x3,0x0,0x0,0x8b,0xbc,0x24,0x60,0x3,0x0,0x0
.byte 0x89,0xbc,0x24,0x50,0x3,0x0,0x0,0x8b,0x8c,0x24,0x18,0x3,0x0,0x0,0x8a,0x5c,0x30,0xff,0x88,0x5c,0x38,0xff
.byte 0x48,0x75,0xf5,0x4a,0x74,0x8,0x8b,0xc1,0x3,0xf9,0x3,0xf1,0xeb,0xea,0x8a,0x45,0x9,0x33,0xc9,0x3c,0x0,0x74
.byte 0x1a,0x33,0xd2,0x8a,0x4d,0xa,0x8a,0x55,0xb,0x80,0xc1,0x7,0xc1,0xe2,0x18,0xc1,0xfa,0x16,0x8b,0xc,0x8d
.long 0x0 + MEYRowOffset

.byte 0x3,0xca,0x89,0x8c,0x24,0x34,0x3,0x0,0x0,0xc6,0x45,0x1c,0x0,0x8b,0xbc,0x24,0x58,0x3,0x0,0x0,0x8b,0x9c
.byte 0x24,0x30,0x3,0x0,0x0,0x8b,0xb4,0x24,0x50,0x3,0x0,0x0,0x8b,0x8c,0x24,0x54,0x3,0x0,0x0,0x8b,0x94,0x24
.byte 0x34,0x3,0x0,0x0,0x33,0xc0,0x8a,0x84,0x24,0x28,0x3,0x0,0x0,0x8d,0x3c,0x5f,0x3,0xf3,0x8d,0xc,0x59,0x3
.byte 0xfa,0xff,0x24,0x85
.long 0xfffffffc + BlockOffsetJumpTable

BLOCK_CONTINUE:

.byte 0x8b,0x9c,0x24,0x3c,0x3,0x0,0x0,0x8b,0x94,0x24,0x38,0x3,0x0,0x0,0x3,0xfb,0x3,0xf2,0x3,0xcb,0x8a,0x45
.byte 0x9,0x33,0xed,0x89,0x8c,0x24,0x40,0x3,0x0,0x0,0x8b,0x8c,0x24,0x18,0x3,0x0,0x0,0xf,0x6f,0x3d
.long 0x0 + m128

.byte 0xf,0xef,0xf6,0x89,0xbc,0x24,0x44,0x3,0x0,0x0,0x8d,0x14,0x49,0x3c,0x0,0xf,0x85,0x17,0x1,0x0,0x0,0x8d
.byte 0x3c,0x8e,0xeb,0x1a
BLOCK0:

.byte 0xeb,0xca
BLOCK1:

.byte 0x83,0xc7,0x10,0x83,0xc6,0x8,0x83,0xc1,0x10,0xeb,0xbf
BLOCK2:

.byte 0xeb,0xa9
BLOCK3:

.byte 0x83,0xc7,0x10,0x83,0xc6,0x8,0x83,0xc1,0x10,0xeb,0x9e,0xf,0x6e,0x6,0xf,0x6e,0xc,0x31,0xf,0x60,0xc6,0xf
.byte 0x6e,0x14,0x4e,0xf,0xf9,0xc7,0xf,0x6e,0x1c,0x32,0xf,0x60,0xce,0xf,0x7f,0x4,0x24,0xf,0xf9,0xcf,0xf,0x6e
.byte 0x46,0x4,0xf,0x60,0xd6,0xf,0x7f,0x4c,0x24,0x10,0xf,0xf9,0xd7,0xf,0x6e,0x4c,0x31,0x4,0xf,0x60,0xde,0xf
.byte 0x7f,0x54,0x24,0x20,0xf,0xf9,0xdf,0xf,0x6e,0x54,0x4e,0x4,0xf,0x60,0xc6,0xf,0x7f,0x5c,0x24,0x30,0xf,0xf9
.byte 0xc7,0xf,0x6e,0x5c,0x32,0x4,0xf,0x60,0xce,0xf,0x7f,0x44,0x24,0x8,0xf,0xf9,0xcf,0xf,0x6e,0x7,0xf,0x60
.byte 0xd6,0xf,0x7f,0x4c,0x24,0x18,0xf,0xf9,0xd7,0xf,0x6e,0xc,0x39,0xf,0x60,0xde,0xf,0x7f,0x54,0x24,0x28,0xf
.byte 0xf9,0xdf,0xf,0x6e,0x14,0x4f,0xf,0x60,0xc6,0xf,0x7f,0x5c,0x24,0x38,0xf,0xf9,0xc7,0xf,0x6e,0x1c,0x3a,0xf
.byte 0x60,0xce,0xf,0x7f,0x44,0x24,0x40,0xf,0xf9,0xcf,0xf,0x6e,0x47,0x4,0xf,0x60,0xd6,0xf,0x7f,0x4c,0x24,0x50
.byte 0xf,0xf9,0xd7,0xf,0x6e,0x4c,0x39,0x4,0xf,0x60,0xde,0xf,0x7f,0x54,0x24,0x60,0xf,0xf9,0xdf,0xf,0x6e,0x54
.byte 0x4f,0x4,0xf,0x60,0xc6,0xf,0x7f,0x5c,0x24,0x70,0xf,0xf9,0xc7,0xf,0x6e,0x5c,0x3a,0x4,0xf,0x60,0xce,0xf
.byte 0x7f,0x44,0x24,0x48,0xf,0x60,0xd6,0xf,0xf9,0xcf,0xf,0xf9,0xd7,0xf,0x60,0xde,0xf,0x7f,0x4c,0x24,0x58,0xf
.byte 0xf9,0xdf,0xf,0x7f,0x54,0x24,0x68,0xf,0x7f,0x5c,0x24,0x78,0xe9,0x47,0x1,0x0,0x0,0x8b,0x84,0x24,0x1c,0x3
.byte 0x0,0x0,0xf,0x6e,0x6,0x8d,0x1c,0x40,0xf,0x6e,0xc,0x31,0xf,0x60,0xc6,0xf,0x6e,0x14,0x4e,0xf,0xf9,0xc7
.byte 0xf,0xf9,0x7,0xf,0x60,0xce,0xf,0x6e,0x1c,0x32,0xf,0xf9,0xcf,0xf,0x7f,0x4,0x24,0xf,0x60,0xd6,0xf,0xf9
.byte 0xc,0x38,0xf,0xf9,0xd7,0xf,0xf9,0x14,0x47,0xf,0x60,0xde,0xf,0x7f,0x4c,0x24,0x10,0xf,0xf9,0xdf,0xf,0xf9
.byte 0x1c,0x3b,0xf,0x7f,0x54,0x24,0x20,0xf,0x7f,0x5c,0x24,0x30,0xf,0x6e,0x46,0x4,0xf,0x6e,0x4c,0x31,0x4,0xf
.byte 0x60,0xc6,0xf,0x6e,0x54,0x4e,0x4,0xf,0xf9,0xc7,0xf,0xf9,0x47,0x8,0xf,0x60,0xce,0xf,0x6e,0x5c,0x32,0x4
.byte 0xf,0xf9,0xcf,0xf,0x7f,0x44,0x24,0x8,0xf,0x60,0xd6,0xf,0xf9,0x4c,0x38,0x8,0xf,0xf9,0xd7,0xf,0xf9,0x54
.byte 0x47,0x8,0xf,0x60,0xde,0xf,0x7f,0x4c,0x24,0x18,0xf,0xf9,0xdf,0xf,0xf9,0x5c,0x3b,0x8,0xf,0x7f,0x54,0x24
.byte 0x28,0x8d,0x34,0x8e,0x8d,0x3c,0x87,0xf,0x7f,0x5c,0x24,0x38,0xf,0x6e,0x6,0xf,0x6e,0xc,0x31,0xf,0x60,0xc6
.byte 0xf,0x6e,0x14,0x4e,0xf,0xf9,0xc7,0xf,0xf9,0x7,0xf,0x60,0xce,0xf,0x6e,0x1c,0x32,0xf,0xf9,0xcf,0xf,0x7f
.byte 0x44,0x24,0x40,0xf,0x60,0xd6,0xf,0xf9,0xc,0x38,0xf,0xf9,0xd7,0xf,0xf9,0x14,0x47,0xf,0x60,0xde,0xf,0x7f
.byte 0x4c,0x24,0x50,0xf,0xf9,0xdf,0xf,0xf9,0x1c,0x3b,0xf,0x7f,0x54,0x24,0x60,0xf,0x7f,0x5c,0x24,0x70,0xf,0x6e
.byte 0x46,0x4,0xf,0x6e,0x4c,0x31,0x4,0xf,0x60,0xc6,0xf,0x6e,0x54,0x4e,0x4,0xf,0xf9,0xc7,0xf,0xf9,0x47,0x8
.byte 0xf,0x60,0xce,0xf,0x6e,0x5c,0x32,0x4,0xf,0xf9,0xcf,0xf,0x7f,0x44,0x24,0x48,0xf,0x60,0xd6,0xf,0xf9,0x4c
.byte 0x38,0x8,0xf,0xf9,0xd7,0xf,0xf9,0x54,0x47,0x8,0xf,0x60,0xde,0xf,0x7f,0x4c,0x24,0x58,0xf,0xf9,0xdf,0xf
.byte 0xf9,0x5c,0x3b,0x8,0xf,0x7f,0x54,0x24,0x68,0xf,0x7f,0x5c,0x24,0x78,0x33,0xed,0x90,0x90,0x90,0xf,0x6f,0x4c
.byte 0x2c,0x8,0xf,0x6f,0x5c,0x2c,0x28,0xf,0x6f,0xe9,0xf,0x6f,0x54,0x2c,0x18,0xf,0x6f,0xf3,0xf,0x6f,0x64,0x2c
.byte 0x38,0xf,0x61,0xca,0xf,0x6f,0x3d
.long 0x0 + mask2

.byte 0xf,0x69,0xea,0xf,0x61,0xdc,0xf,0x69,0xf4,0xf,0x6f,0xd1,0xf,0x62,0xcb,0xf,0x6f,0xe5,0xf,0x62,0xee,0xf
.byte 0x6a,0xd3,0xf,0x6f,0xc1,0xf,0x6a,0xe6,0xf,0x6f,0xda,0xf,0xfd,0xc4,0xf,0xfd,0xd5,0xf,0xf9,0xcc,0xf,0xf9
.byte 0xdd,0xf,0x6f,0xe0,0xf,0x6f,0xe9,0xf,0x6f,0xf3,0xf,0x71,0xf5,0x2,0xf,0xfd,0xc2,0xf,0x71,0xf6,0x2,0xf
.byte 0xf9,0xe2,0xf,0xfd,0xe9,0xf,0x7f,0x44,0x2c,0x8,0xf,0xfd,0xf3,0xf,0x7f,0x64,0x2c,0x18,0xf,0x6f,0xf9,0xf
.byte 0xfd,0xc9,0xf,0x6f,0xc3,0xf,0xfd,0xdb,0xf,0x71,0xe7,0x1,0xf,0x71,0xe0,0x1,0xf,0xfd,0xef,0xf,0x71,0xe7
.byte 0x1,0xf,0xfd,0xf0,0xf,0x71,0xe0,0x1,0xf,0xfd,0xcf,0xf,0x6f,0x3d
.long 0x0 + mask2

.byte 0xf,0xfd,0xd8,0xf,0xfd,0xef,0xf,0xfd,0xcf,0xf,0xfd,0xeb,0xf,0x6f,0x54,0x2c,0x10,0xf,0xf9,0xce,0xf,0x6f
.byte 0x64,0x2c,0x30,0xf,0x71,0xe5,0x2,0xf,0x6f,0x5c,0x2c,0x20,0xf,0x71,0xe1,0x2,0xf,0x6f,0x4,0x2c,0xf,0x6f
.byte 0xf3,0xf,0x7f,0x6c,0x2c,0x38,0xf,0x61,0xdc,0xf,0x6f,0xe8,0xf,0x69,0xf4,0xf,0x7f,0x4c,0x2c,0x28,0xf,0x61
.byte 0xc2,0xf,0x69,0xea,0xf,0x6f,0xd0,0xf,0x6f,0xe5,0xf,0x62,0xc3,0xf,0x6a,0xd3,0xf,0x6f,0xc8,0xf,0x62,0xee
.byte 0xf,0x6f,0xda,0xf,0x6a,0xe6,0xf,0xfd,0xd5,0xf,0xfd,0xcc,0xf,0xf9,0xdd,0xf,0xf9,0xc4,0xf,0x6f,0xe1,0xf
.byte 0x6f,0xeb,0xf,0x6f,0xf0,0xf,0x71,0xf5,0x2,0xf,0xfd,0xca,0xf,0x71,0xf6,0x2,0xf,0xf9,0xe2,0xf,0xfd,0xeb
.byte 0xf,0xfd,0xf0,0xf,0x6f,0xfb,0xf,0xfd,0xdb,0xf,0x6f,0xd0,0xf,0xfd,0xc0,0xf,0x71,0xe7,0x1,0xf,0x71,0xe2
.byte 0x1,0xf,0xfd,0xef,0xf,0x71,0xe7,0x1,0xf,0xfd,0xf2,0xf,0x71,0xe2,0x1,0xf,0xfd,0xdf,0xf,0x6f,0x3d
.long 0x0 + mask2

.byte 0xf,0xfd,0xc2,0xf,0xfd,0xf7,0xf,0xfd,0xc7,0xf,0xfd,0xf3,0xf,0xf9,0xc5,0xf,0x6f,0x5c,0x2c,0x8,0xf,0x71
.byte 0xe6,0x2,0xf,0x6f,0x6c,0x2c,0x18,0xf,0x71,0xe0,0x2,0xf,0x6f,0xd1,0xf,0xfd,0xcb,0xf,0xfd,0xd
.long 0x0 + mask2

.byte 0xf,0xf9,0xd3,0xf,0x71,0xe1,0x2,0xf,0x6f,0xdc,0xf,0xfd,0xe5,0xf,0x7f,0xc,0x2c,0xf,0xfd,0x25
.long 0x0 + mask2

.byte 0xf,0x6f,0x4c,0x2c,0x28,0xf,0x71,0xe4,0x2,0xf,0xf9,0xdd,0xf,0x6f,0xe9,0xf,0xfd,0x1d
.long 0x0 + mask2

.byte 0xf,0xfd,0xc8,0xf,0x71,0xe3,0x2,0xf,0xf9,0xe8,0xf,0xfd,0x2d
.long 0x0 + mask2

.byte 0xf,0x6f,0xc4,0xf,0x71,0xe5,0x2,0xf,0xfd,0xd
.long 0x0 + mask2

.byte 0xf,0x6f,0xfd,0xf,0x71,0xe1,0x2,0xf,0x61,0xe3,0xf,0x69,0xc3,0xf,0x6f,0xdc,0xf,0x61,0xe9,0xf,0x69,0xf9
.byte 0xf,0x6f,0xc8,0xf,0x62,0xe5,0xf,0x6a,0xdd,0xf,0x6f,0x6c,0x2c,0x38,0xf,0x62,0xc7,0xf,0x7f,0x64,0x2c,0x8
.byte 0xf,0x6a,0xcf,0xf,0x7f,0x5c,0x2c,0x18,0xf,0x6f,0xe6,0xf,0x7f,0x44,0x2c,0x28,0xf,0xfd,0xe5,0xf,0x7f,0x4c
.byte 0x2c,0x38,0xf,0xf9,0xf5,0xf,0xfd,0x35
.long 0x0 + mask2

.byte 0xf,0x6f,0xea,0xf,0x71,0xe6,0x2,0xf,0xfd,0x15
.long 0x0 + mask2

.byte 0xf,0x6f,0x3d
.long 0x0 + mask4

.byte 0xf,0x71,0xe2,0x2,0xf,0x6f,0xdc,0xf,0xfd,0x15
.long 0x0 + mask2

.byte 0xf,0x71,0xe4,0x2,0xf,0x6f,0xc5,0xf,0x6f,0xcb,0xf,0xf9,0xea,0xf,0xf9,0xdc,0xf,0xfd,0xe8,0xf,0xfd,0xd9
.byte 0xf,0xfd,0xef,0xf,0xfd,0xdf,0xf,0xfd,0xcd,0xf,0xf9,0xd8,0xf,0x6f,0x14,0x2c,0xf,0x71,0xe1,0x3,0xf,0x71
.byte 0xe3,0x3,0xf,0x6f,0xe2,0xf,0x6f,0xee,0xf,0x61,0xd1,0xf,0x69,0xe1,0xf,0x6f,0xca,0xf,0x61,0xeb,0xf,0x69
.byte 0xf3,0xf,0x6f,0xdc,0xf,0x62,0xcd,0xf,0x6a,0xd5,0xf,0x7f,0xc,0x2c,0xf,0x62,0xde,0xf,0x7f,0x54,0x2c,0x10
.byte 0xf,0x6a,0xe6,0xf,0x7f,0x5c,0x2c,0x20,0x83,0xc5,0x40,0xf,0x7f,0x64,0x2c,0xf0,0x81,0xfd,0x80,0x0,0x0,0x0
.byte 0xf,0x85,0x49,0xfd,0xff,0xff,0x33,0xed,0x90,0x90,0x90,0xf,0x6f,0xc,0x2c,0xf,0x6f,0x54,0x2c,0x10,0xf,0x6f
.byte 0xe1,0xf,0x6f,0x6c,0x2c,0x20,0xf,0x6f,0xda,0xf,0x6f,0x74,0x2c,0x30,0xf,0xfd,0xd5,0xf,0x6f,0x3d
.long 0x0 + mask2

.byte 0xf,0xfd,0xce,0xf,0xf9,0xe6,0xf,0xf9,0xdd,0xf,0x6f,0xc1,0xf,0x6f,0xf4,0xf,0x71,0xf6,0x2,0xf,0x6f,0xeb
.byte 0xf,0x71,0xf5,0x2,0xf,0xfd,0xca,0xf,0xfd,0xf4,0xf,0xfd,0xeb,0xf,0xf9,0xc2,0xf,0x6f,0xfc,0xf,0xfd,0xe4
.byte 0xf,0x6f,0xd3,0xf,0xfd,0xdb,0xf,0x71,0xe7,0x1,0xf,0x71,0xe2,0x1,0xf,0xfd,0xf7,0xf,0x71,0xe7,0x1,0xf
.byte 0xfd,0xea,0xf,0x71,0xe2,0x1,0xf,0xfd,0xe7,0xf,0x6f,0x3d
.long 0x0 + mask2

.byte 0xf,0xfd,0xda,0xf,0x6f,0x54,0x2c,0x50,0xf,0x7f,0xc,0x2c,0xf,0xfd,0xf7,0xf,0x7f,0x44,0x2c,0x10,0xf,0xfd
.byte 0xe7,0xf,0x6f,0x4c,0x2c,0x40,0xf,0xfd,0xf3,0xf,0x6f,0x5c,0x2c,0x60,0xf,0xf9,0xe5,0xf,0x6f,0x6c,0x2c,0x70
.byte 0xf,0x71,0xe6,0x2,0xf,0x6f,0xc1,0xf,0x71,0xe4,0x2,0xf,0xfd,0xcd,0xf,0xf9,0xc5,0xf,0x7f,0x74,0x2c,0x30
.byte 0xf,0x6f,0xea,0xf,0x7f,0x64,0x2c,0x20,0xf,0xfd,0xd3,0xf,0xf9,0xeb,0xf,0x6f,0xe1,0xf,0x6f,0xd8,0xf,0x6f
.byte 0xf5,0xf,0xfd,0xca,0xf,0x71,0xf3,0x2,0xf,0xf9,0xe2,0xf,0x71,0xf6,0x2,0xf,0xfd,0xd8,0xf,0xfd,0xf5,0xf
.byte 0x6f,0xf8,0xf,0xfd,0xc0,0xf,0x6f,0xd5,0xf,0xfd,0xed,0xf,0x71,0xe7,0x1,0xf,0x71,0xe2,0x1,0xf,0xfd,0xdf
.byte 0xf,0x71,0xe7,0x1,0xf,0xfd,0xf2,0xf,0x71,0xe2,0x1,0xf,0xfd,0xc7,0xf,0x6f,0x3d
.long 0x0 + mask2

.byte 0xf,0xfd,0xea,0xf,0xfd,0xdf,0xf,0xfd,0xc7,0xf,0xfd,0xdd,0xf,0xf9,0xc6,0xf,0x6f,0x14,0x2c,0xf,0x71,0xe3
.byte 0x2,0xf,0x6f,0x6c,0x2c,0x10,0xf,0x71,0xe0,0x2,0xf,0x6f,0xf2,0xf,0x6f,0xfd,0xf,0xfd,0xd1,0xf,0xfd,0xec
.byte 0xf,0xfd,0x15
.long 0x0 + mask4

.byte 0xf,0xf9,0xf1,0xf,0xfd,0x2d
.long 0x0 + mask4

.byte 0xf,0x71,0xe2,0x3,0xf,0x6f,0x4c,0x2c,0x20,0xf,0x71,0xe5,0x3,0xf,0x7f,0x14,0x2c,0xf,0xf9,0xfc,0xf,0xfd
.byte 0x3d
.long 0x0 + mask4

.byte 0xf,0x6f,0xe0,0xf,0x7f,0x6c,0x2c,0x40,0xf,0x71,0xe7,0x3,0xf,0xfd,0xc1,0xf,0xf9,0xe1,0xf,0xfd,0x5
.long 0x0 + mask4

.byte 0xf,0x7f,0x7c,0x2c,0x50,0xf,0x71,0xe0,0x3,0xf,0x6f,0x54,0x2c,0x30,0xf,0x7f,0x44,0x2c,0x70,0xf,0x6f,0xfa
.byte 0xf,0xfd,0x25
.long 0x0 + mask4

.byte 0xf,0xfd,0xd3,0xf,0xf9,0xfb,0xf,0x71,0xe4,0x3,0xf,0xfd,0x3d
.long 0x0 + mask4

.byte 0xf,0x6f,0xde,0xf,0x6f,0xc6,0xf,0x71,0xe7,0x3,0xf,0x7f,0x64,0x2c,0x60,0xf,0xfd,0x1d
.long 0x0 + mask2

.byte 0xf,0x7f,0x7c,0x2c,0x20,0xf,0x71,0xe3,0x2,0xf,0x6f,0x3d
.long 0x0 + mask4

.byte 0xf,0xf9,0xc3,0xf,0xfd,0xc6,0xf,0x6f,0xe2,0xf,0xfd,0x25
.long 0x0 + mask2

.byte 0xf,0xfd,0xc7,0xf,0x6f,0xda,0xf,0x71,0xe4,0x2,0xf,0xfd,0xdb,0xf,0xfd,0xc2,0xf,0xf9,0xdc,0xf,0x71,0xe0
.byte 0x4,0xf,0xfd,0xdf,0xf,0x7f,0x44,0x2c,0x10,0xf,0xf9,0xde,0xf,0x71,0xe3,0x4,0x83,0xc5,0x8,0xf,0x7f,0x5c
.byte 0x2c,0x28,0x83,0xfd,0x10,0xf,0x85,0xe6,0xfd,0xff,0xff,0x8b,0xbc,0x24,0x4c,0x3,0x0,0x0,0x8b,0xac,0x24,0x10
.byte 0x3,0x0,0x0,0x8b,0x84,0x24,0x14,0x3,0x0,0x0,0x8b,0xf4,0x8a,0x57,0x9,0x3,0x47,0xc,0x69,0xc0,0x80,0x1
.byte 0x0,0x0,0xf,0x6f,0x4,0x24,0x3,0xe8,0x80,0xfa,0x0,0x75,0x11,0xf,0x6e,0x8c,0x24,0x8,0x3,0x0,0x0,0xf
.byte 0xf9,0xc1,0x81,0xc5,0x0,0x24,0x0,0x0,0x8b,0x9c,0x24,0x64,0x3,0x0,0x0,0x83,0xfb,0x1,0xf,0x84,0xaf,0x5
.byte 0x0,0x0,0xf,0x7f,0x6,0xf,0xef,0xff,0xf,0x6f,0xc8,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x8,0xf,0xef,0xc8
.byte 0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0x80,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0xef,0xec,0xf,0xf9
.byte 0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x0,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x0,0x1
.byte 0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0x88,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf
.byte 0xef,0xd0,0xf,0x61,0xdf,0xf,0xf9,0xd0,0xf,0x69,0xcf,0xf,0x7f,0x9e,0x0,0x1,0x0,0x0,0xf,0x6f,0xf5,0xf
.byte 0x7f,0x8e,0x8,0x1,0x0,0x0,0xf,0x6f,0xdd,0xf,0xd5,0x75,0x8,0xf,0x7f,0x96,0x80,0x0,0x0,0x0,0xf,0x75
.byte 0xdf,0xf,0xdf,0x9d,0x8,0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x46,0x10,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf
.byte 0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9e,0x10,0x1,0x0,0x0,0xf
.byte 0x6f,0xc8,0xf,0x7f,0xae,0x18,0x1,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x18,0xf,0xef,0xc8,0xf,0x6f
.byte 0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0x90,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f,0xb6,0x88,0x0,0x0,0x0
.byte 0xf,0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x10,0xf,0x75,0xdf
.byte 0xf,0xdf,0x9d,0x10,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0x98,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd
.byte 0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0xf,0xf9,0xd0,0xf,0x69,0xcf,0xf,0x7f,0x9e,0x20,0x1,0x0
.byte 0x0,0xf,0x6f,0xf5,0xf,0x7f,0x8e,0x28,0x1,0x0,0x0,0xf,0x6f,0xdd,0xf,0xd5,0x75,0x18,0xf,0x7f,0x96,0x90
.byte 0x0,0x0,0x0,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x18,0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x46,0x20,0xf,0xf9
.byte 0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9e
.byte 0x30,0x1,0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f,0xae,0x38,0x1,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x28
.byte 0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0xa0,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f
.byte 0xb6,0x98,0x0,0x0,0x0,0xf,0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5
.byte 0x55,0x20,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x20,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0xa8,0x0,0x0,0x0
.byte 0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0xf,0xf9,0xd0,0xf,0x69,0xcf,0xf
.byte 0x7f,0x9e,0x40,0x1,0x0,0x0,0xf,0x6f,0xf5,0xf,0x7f,0x8e,0x48,0x1,0x0,0x0,0xf,0x6f,0xdd,0xf,0xd5,0x75
.byte 0x28,0xf,0x7f,0x96,0xa0,0x0,0x0,0x0,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x28,0x1,0x0,0x0,0xf,0xfd,0xed,0xf
.byte 0x6f,0x46,0x30,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf
.byte 0x69,0xef,0xf,0x7f,0x9e,0x50,0x1,0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f,0xae,0x58,0x1,0x0,0x0,0xf,0x71,0xe0
.byte 0xf,0xf,0x6f,0x66,0x38,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0xb0,0x0,0x0,0x0,0xf
.byte 0x71,0xe4,0xf,0xf,0x7f,0xb6,0xa8,0x0,0x0,0x0,0xf,0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1
.byte 0xf,0x6f,0xd9,0xf,0xd5,0x55,0x30,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x30,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5
.byte 0xad,0xb8,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0xf,0xf9
.byte 0xd0,0xf,0x69,0xcf,0xf,0x7f,0x9e,0x60,0x1,0x0,0x0,0xf,0x6f,0xf5,0xf,0x7f,0x8e,0x68,0x1,0x0,0x0,0xf
.byte 0x6f,0xdd,0xf,0xd5,0x75,0x38,0xf,0x7f,0x96,0xb0,0x0,0x0,0x0,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x38,0x1,0x0
.byte 0x0,0xf,0xfd,0xed,0xf,0x6f,0x46,0x40,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61
.byte 0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9e,0x70,0x1,0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f,0xae,0x78,0x1
.byte 0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x48,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d
.byte 0xc0,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f,0xb6,0xb8,0x0,0x0,0x0,0xf,0xef,0xec,0xf,0xf9,0xec,0xf
.byte 0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x40,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x40,0x1,0x0,0x0
.byte 0xf,0xfd,0xc9,0xf,0xe5,0xad,0xc8,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0
.byte 0xf,0x61,0xdf,0xf,0xf9,0xd0,0xf,0x69,0xcf,0xf,0x7f,0x9e,0x80,0x1,0x0,0x0,0xf,0x6f,0xf5,0xf,0x7f,0x8e
.byte 0x88,0x1,0x0,0x0,0xf,0x6f,0xdd,0xf,0xd5,0x75,0x48,0xf,0x7f,0x96,0xc0,0x0,0x0,0x0,0xf,0x75,0xdf,0xf
.byte 0xdf,0x9d,0x48,0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x46,0x50,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd
.byte 0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9e,0x90,0x1,0x0,0x0,0xf,0x6f,0xc8
.byte 0xf,0x7f,0xae,0x98,0x1,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x58,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf
.byte 0xf9,0xc8,0xf,0xe5,0x8d,0xd0,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f,0xb6,0xc8,0x0,0x0,0x0,0xf,0xef
.byte 0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x50,0xf,0x75,0xdf,0xf,0xdf
.byte 0x9d,0x50,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0xd8,0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf
.byte 0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0xf,0xf9,0xd0,0xf,0x69,0xcf,0xf,0x7f,0x9e,0xa0,0x1,0x0,0x0,0xf
.byte 0x6f,0xf5,0xf,0x7f,0x8e,0xa8,0x1,0x0,0x0,0xf,0x6f,0xdd,0xf,0xd5,0x75,0x58,0xf,0x7f,0x96,0xd0,0x0,0x0
.byte 0x0,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x58,0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x46,0x60,0xf,0xf9,0xec,0xf
.byte 0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef,0xf,0x7f,0x9e,0xb0,0x1
.byte 0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f,0xae,0xb8,0x1,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x6f,0x66,0x68,0xf,0xef
.byte 0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0xe0,0x0,0x0,0x0,0xf,0x71,0xe4,0xf,0xf,0x7f,0xb6,0xd8
.byte 0x0,0x0,0x0,0xf,0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f,0xd9,0xf,0xd5,0x55,0x60
.byte 0xf,0x75,0xdf,0xf,0xdf,0x9d,0x60,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0xe8,0x0,0x0,0x0,0xf,0xf9
.byte 0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0xf,0xf9,0xd0,0xf,0x69,0xcf,0xf,0x7f,0x9e
.byte 0xc0,0x1,0x0,0x0,0xf,0x6f,0xf5,0xf,0x7f,0x8e,0xc8,0x1,0x0,0x0,0xf,0x6f,0xdd,0xf,0xd5,0x75,0x68,0xf
.byte 0x7f,0x96,0xe0,0x0,0x0,0x0,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x68,0x1,0x0,0x0,0xf,0xfd,0xed,0xf,0x6f,0x46
.byte 0x70,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69,0xef
.byte 0xf,0x7f,0x9e,0xd0,0x1,0x0,0x0,0xf,0x6f,0xc8,0xf,0x7f,0xae,0xd8,0x1,0x0,0x0,0xf,0x71,0xe0,0xf,0xf
.byte 0x6f,0x66,0x78,0xf,0xef,0xc8,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0xf0,0x0,0x0,0x0,0xf,0x71,0xe4
.byte 0xf,0xf,0x7f,0xb6,0xe8,0x0,0x0,0x0,0xf,0xef,0xec,0xf,0xf9,0xec,0xf,0xeb,0xff,0xf,0x6f,0xd1,0xf,0x6f
.byte 0xd9,0xf,0xd5,0x55,0x70,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x70,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xe5,0xad,0xf8
.byte 0x0,0x0,0x0,0xf,0xf9,0xc8,0xf,0xfd,0xd3,0xf,0x6f,0xd9,0xf,0xef,0xd0,0xf,0x61,0xdf,0xf,0xf9,0xd0,0xf
.byte 0x69,0xcf,0xf,0x7f,0x9e,0xe0,0x1,0x0,0x0,0xf,0x6f,0xf5,0xf,0x7f,0x8e,0xe8,0x1,0x0,0x0,0xf,0x6f,0xdd
.byte 0xf,0xd5,0x75,0x78,0xf,0x7f,0x96,0xf0,0x0,0x0,0x0,0xf,0x75,0xdf,0xf,0xdf,0x9d,0x78,0x1,0x0,0x0,0xf
.byte 0xfd,0xed,0xf,0xf9,0xec,0xf,0xfd,0xf3,0xf,0x6f,0xdd,0xf,0xef,0xf4,0xf,0x61,0xdf,0xf,0xf9,0xf4,0xf,0x69
.byte 0xef,0xf,0x7f,0x9e,0xf0,0x1,0x0,0x0,0xf,0x7f,0xae,0xf8,0x1,0x0,0x0,0xf,0x7f,0xb6,0xf8,0x0,0x0,0x0
.byte 0xe9,0x79,0x3,0x0,0x0,0xf,0x7f,0x6,0xf,0xef,0xff,0xf,0x6f,0xc8,0xf,0x71,0xe0,0xf,0xf,0x6f,0x56,0x8
.byte 0xf,0xef,0xc8,0xf,0x6f,0x66,0x10,0xf,0xf9,0xc8,0xf,0xe5,0x8d,0x80,0x0,0x0,0x0,0xf,0x6f,0xda,0xf,0x71
.byte 0xe2,0xf,0xf,0xef,0xda,0xf,0xeb,0xff,0xf,0x6f,0xe9,0xf,0x6f,0xf1,0xf,0xd5,0x6d,0x0,0xf,0x75,0xf7,0xf
.byte 0xdf,0xb5,0x0,0x1,0x0,0x0,0xf,0xfd,0xc9,0xf,0xf9,0xda,0xf,0xf9,0xc8,0xf,0xfd,0xf5,0xf,0x6f,0xec,0xf
.byte 0xe5,0x9d,0x88,0x0,0x0,0x0,0xf,0xef,0xf0,0xf,0x71,0xe4,0xf,0xf,0xf9,0xf0,0xf,0x6f,0xc1,0xf,0xef,0xec
.byte 0xf,0x61,0xc7,0xf,0xf9,0xec,0xf,0xe5,0xad,0x90,0x0,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x86,0x0,0x1,0x0
.byte 0x0,0xf,0xfd,0xdb,0xf,0x7f,0x8e,0x8,0x1,0x0,0x0,0xf,0xf9,0xda,0xf,0x6f,0x46,0x18,0xf,0xfd,0xed,0xf
.byte 0x6f,0xd3,0xf,0x6f,0xc8,0xf,0x61,0xd7,0xf,0xf9,0xec,0xf,0x69,0xdf,0xf,0x6f,0xe5,0xf,0x7f,0x96,0x10,0x1
.byte 0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x7f,0x9e,0x18,0x1,0x0,0x0,0xf,0xef,0xc8,0xf,0x61,0xe7,0xf,0xf9,0xc8
.byte 0xf,0x6f,0x56,0x20,0xf,0x69,0xef,0xf,0xe5,0x8d,0x98,0x0,0x0,0x0,0xf,0x6f,0xda,0xf,0x7f,0xa6,0x20,0x1
.byte 0x0,0x0,0xf,0x71,0xe2,0xf,0xf,0x7f,0xae,0x28,0x1,0x0,0x0,0xf,0xef,0xda,0xf,0x6f,0x66,0x28,0xf,0xfd
.byte 0xc9,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0x71,0xe4,0xf,0xf,0xf9,0xda,0xf,0x7f,0xb6,0x80,0x0,0x0,0x0,0xf
.byte 0xef,0xec,0xf,0xe5,0x9d,0xa0,0x0,0x0,0x0,0xf,0x6f,0xc1,0xf,0x61,0xc7,0xf,0xf9,0xec,0xf,0xe5,0xad,0xa8
.byte 0x0,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x86,0x30,0x1,0x0,0x0,0xf,0xfd,0xdb,0xf,0x7f,0x8e,0x38,0x1,0x0
.byte 0x0,0xf,0xf9,0xda,0xf,0x6f,0x46,0x30,0xf,0xfd,0xed,0xf,0x6f,0xd3,0xf,0x6f,0xc8,0xf,0x61,0xd7,0xf,0xf9
.byte 0xec,0xf,0x69,0xdf,0xf,0x6f,0xe5,0xf,0x7f,0x96,0x40,0x1,0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x7f,0x9e,0x48
.byte 0x1,0x0,0x0,0xf,0xef,0xc8,0xf,0x61,0xe7,0xf,0xf9,0xc8,0xf,0x6f,0x56,0x38,0xf,0x69,0xef,0xf,0xe5,0x8d
.byte 0xb0,0x0,0x0,0x0,0xf,0x6f,0xda,0xf,0x7f,0xa6,0x50,0x1,0x0,0x0,0xf,0x71,0xe2,0xf,0xf,0x7f,0xae,0x58
.byte 0x1,0x0,0x0,0xf,0xef,0xda,0xf,0x6f,0x66,0x40,0xf,0xfd,0xc9,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0x71,0xe4
.byte 0xf,0xf,0xf9,0xda,0xf,0xef,0xec,0xf,0xe5,0x9d,0xb8,0x0,0x0,0x0,0xf,0x6f,0xc1,0xf,0x61,0xc7,0xf,0xf9
.byte 0xec,0xf,0xe5,0xad,0xc0,0x0,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x86,0x60,0x1,0x0,0x0,0xf,0xfd,0xdb,0xf
.byte 0x7f,0x8e,0x68,0x1,0x0,0x0,0xf,0xf9,0xda,0xf,0x6f,0x46,0x48,0xf,0xfd,0xed,0xf,0x6f,0xd3,0xf,0x6f,0xc8
.byte 0xf,0x61,0xd7,0xf,0xf9,0xec,0xf,0x69,0xdf,0xf,0x6f,0xe5,0xf,0x7f,0x96,0x70,0x1,0x0,0x0,0xf,0x71,0xe0
.byte 0xf,0xf,0x7f,0x9e,0x78,0x1,0x0,0x0,0xf,0xef,0xc8,0xf,0x61,0xe7,0xf,0xf9,0xc8,0xf,0x6f,0x56,0x50,0xf
.byte 0x69,0xef,0xf,0xe5,0x8d,0xc8,0x0,0x0,0x0,0xf,0x6f,0xda,0xf,0x7f,0xa6,0x80,0x1,0x0,0x0,0xf,0x71,0xe2
.byte 0xf,0xf,0x7f,0xae,0x88,0x1,0x0,0x0,0xf,0xef,0xda,0xf,0x6f,0x66,0x58,0xf,0xfd,0xc9,0xf,0x6f,0xec,0xf
.byte 0xf9,0xc8,0xf,0x71,0xe4,0xf,0xf,0xf9,0xda,0xf,0xef,0xec,0xf,0xe5,0x9d,0xd0,0x0,0x0,0x0,0xf,0x6f,0xc1
.byte 0xf,0x61,0xc7,0xf,0xf9,0xec,0xf,0xe5,0xad,0xd8,0x0,0x0,0x0,0xf,0x69,0xcf,0xf,0x7f,0x86,0x90,0x1,0x0
.byte 0x0,0xf,0xfd,0xdb,0xf,0x7f,0x8e,0x98,0x1,0x0,0x0,0xf,0xf9,0xda,0xf,0x6f,0x46,0x60,0xf,0xfd,0xed,0xf
.byte 0x6f,0xd3,0xf,0x6f,0xc8,0xf,0x61,0xd7,0xf,0xf9,0xec,0xf,0x69,0xdf,0xf,0x6f,0xe5,0xf,0x7f,0x96,0xa0,0x1
.byte 0x0,0x0,0xf,0x71,0xe0,0xf,0xf,0x7f,0x9e,0xa8,0x1,0x0,0x0,0xf,0xef,0xc8,0xf,0x61,0xe7,0xf,0xf9,0xc8
.byte 0xf,0x6f,0x56,0x68,0xf,0x69,0xef,0xf,0xe5,0x8d,0xe0,0x0,0x0,0x0,0xf,0x6f,0xda,0xf,0x7f,0xa6,0xb0,0x1
.byte 0x0,0x0,0xf,0x71,0xe2,0xf,0xf,0x7f,0xae,0xb8,0x1,0x0,0x0,0xf,0xef,0xda,0xf,0x6f,0x66,0x70,0xf,0xfd
.byte 0xc9,0xf,0x6f,0xec,0xf,0xf9,0xc8,0xf,0x71,0xe4,0xf,0xf,0xf9,0xda,0xf,0xef,0xec,0xf,0xe5,0x9d,0xe8,0x0
.byte 0x0,0x0,0xf,0x6f,0xc1,0xf,0x61,0xc7,0xf,0xf9,0xec,0xf,0xe5,0xad,0xf0,0x0,0x0,0x0,0xf,0x69,0xcf,0xf
.byte 0x7f,0x86,0xc0,0x1,0x0,0x0,0xf,0xfd,0xdb,0xf,0x7f,0x8e,0xc8,0x1,0x0,0x0,0xf,0xf9,0xda,0xf,0x6f,0x46
.byte 0x78,0xf,0xfd,0xed,0xf,0x6f,0xd3,0xf,0x6f,0xc8,0xf,0x71,0xe0,0xf,0xf,0xf9,0xec,0xf,0xef,0xc8,0xf,0x61
.byte 0xd7,0xf,0xf9,0xc8,0xf,0x69,0xdf,0xf,0xe5,0x8d,0xf8,0x0,0x0,0x0,0xf,0x6f,0xe5,0xf,0x7f,0x96,0xd0,0x1
.byte 0x0,0x0,0xf,0x61,0xe7,0xf,0x7f,0x9e,0xd8,0x1,0x0,0x0,0xf,0x69,0xef,0xf,0x7f,0xa6,0xe0,0x1,0x0,0x0
.byte 0xf,0xfd,0xc9,0xf,0x7f,0xae,0xe8,0x1,0x0,0x0,0xf,0xf9,0xc8,0xf,0x6f,0xc1,0xf,0x69,0xcf,0xf,0x61,0xc7
.byte 0xf,0x7f,0x8e,0xf8,0x1,0x0,0x0,0xf,0x7f,0x86,0xf0,0x1,0x0,0x0,0x8b,0x84,0x24,0x5c,0x3,0x0,0x0,0x33
.byte 0xff,0xba,0xff,0xff,0x0,0x0,0xb9,0xf8,0xff,0xff,0xff,0x8b,0x9c,0xbc,0x4,0x2,0x0,0x0,0x89,0x94,0x24,0x0
.byte 0x2,0x0,0x0,0x8b,0x30,0x2e,0x8b,0xc0,0x2e,0x8b,0xc0,0x2e,0x8b,0xc0,0x2e,0x8b,0xc0,0x8b,0x84,0x9c,0x0,0x1
.byte 0x0,0x0,0x83,0xc1,0x8,0x8b,0x9c,0xbc,0x8,0x2,0x0,0x0,0x47,0x83,0xf8,0x1,0x7e,0xe9,0x3d,0x0,0x80,0x0
.byte 0x0,0x7f,0x3e,0x8b,0x91
.long 0x0 + NonEscCoeff

.byte 0x48,0x3b,0xd0,0x7c,0x11,0x8a,0x94,0xc8
.long 0xffffffff + CodeBookIdx

.byte 0xb9,0xf8,0xff,0xff,0xff,0x88,0x16,0x46,0xeb,0xc6,0xc1,0xe9,0x3,0xc6,0x6,0xb,0x8d,0x14,0x85,0x0,0x0,0x0
.byte 0x0,0x88,0x4e,0x1,0x88,0x76,0x3,0x24,0x3f,0xb9,0xf8,0xff,0xff,0xff,0x88,0x46,0x2,0x83,0xc6,0x4,0xeb,0xa4
.byte 0x83,0xff,0x41,0x74,0x50,0x8b,0x9c,0xbc,0x0,0x2,0x0,0x0,0x33,0xc0,0x66,0x8b,0x4,0x5c,0x66,0x89,0x84,0x5c
.byte 0x80,0x0,0x0,0x0,0x8b,0x9c,0xbc,0x4,0x2,0x0,0x0,0x3d,0x0,0x80,0x0,0x0,0x7d,0x15,0x3,0xc0,0xf,0x84
.byte 0x74,0xff,0xff,0xff,0x8b,0x91
.long 0x0 + NonEscCoeff

.byte 0x48,0x3b,0xd0,0x7d,0x92,0xeb,0xa1,0x35,0xff,0xff,0x0,0x0,0x40,0x8b,0x91
.long 0x0 + NonEscCoeff

.byte 0x3,0xc0,0x3b,0xd0,0xf,0x8d,0x7a,0xff,0xff,0xff,0xeb,0x89,0x8b,0xbc,0x24,0x4c,0x3,0x0,0x0,0x8b,0x84,0x24
.byte 0x5c,0x3,0x0,0x0,0x8a,0x57,0x9,0x80,0xfa,0x0,0x75,0x21,0x8b,0x94,0x24,0x80,0x0,0x0,0x0,0x8b,0x9c,0x24
.byte 0x8,0x3,0x0,0x0,0x66,0x3,0xd3,0x66,0x89,0x94,0x24,0x80,0x0,0x0,0x0,0x66,0x89,0x94,0x24,0x8,0x3,0x0
.byte 0x0,0x8b,0x18,0x8b,0x8c,0x24,0x28,0x3,0x0,0x0,0x3b,0xde,0x74,0xe,0x8a,0x5f,0x1c,0xc6,0x6,0x4,0xa,0xd9
.byte 0x46,0x88,0x5f,0x1c,0x89,0x30,0x8b,0xac,0x24,0x4c,0x3,0x0,0x0,0x8b,0x9c,0x24,0x64,0x3,0x0,0x0,0x83,0xfb
.byte 0x1,0xf,0x84,0x15,0x4,0x0,0x0,0x8a,0x45,0x9,0x8d,0xb4,0x24,0x80,0x0,0x0,0x0,0x3c,0x0,0x74,0xa,0xb9
.byte 0x10,0x0,0x0,0x0,0x8d,0x3c,0x24,0xeb,0xe,0x8b,0x8c,0x24,0x1c,0x3,0x0,0x0,0x8b,0xbc,0x24,0x40,0x3,0x0
.byte 0x0,0x8d,0x14,0x49,0x8d,0x9c,0x24,0x0,0x1,0x0,0x0,0x33,0xed,0x90,0x90,0x90,0xf,0x6f,0x74,0x35,0x10,0xf
.byte 0xef,0xc9,0xf,0x6f,0x54,0x35,0x30,0xf,0x6f,0xe6,0xf,0x6f,0xda,0xf,0x71,0xf4,0x2,0xf,0x71,0xf3,0x2,0xf
.byte 0xf9,0xf4,0xf,0xf9,0xd3,0xf,0x6f,0xec,0xf,0xfd,0x25
.long 0x0 + mask4

.byte 0xf,0xf9,0xee,0xf,0xf9,0xeb,0xf,0xf9,0xda,0xf,0xfd,0x2d
.long 0x0 + mask4

.byte 0xf,0xfd,0xe3,0xf,0xfd,0x4c,0x35,0x0,0xf,0x71,0xe4,0x3,0xf,0x6f,0x74,0x35,0x20,0xf,0x71,0xe5,0x3,0xf
.byte 0x6f,0xc4,0xf,0xfd,0xe6,0xf,0xf9,0xc6,0xf,0x6f,0xf9,0xf,0x6f,0x74,0x35,0x40,0xf,0xfd,0xcd,0xf,0x6f,0x5c
.byte 0x35,0x50,0xf,0x6f,0xd6,0xf,0xf9,0xfd,0xf,0xfd,0xd3,0xf,0xf9,0xf3,0xf,0x6f,0xef,0xf,0x6f,0x5c,0x35,0x70
.byte 0xf,0xfd,0xee,0xf,0x7f,0x44,0x35,0x70,0xf,0x6f,0xc1,0xf,0x7f,0x6c,0x35,0x40,0xf,0xf9,0xfe,0xf,0xfd,0xca
.byte 0xf,0xf9,0xc2,0xf,0x6f,0x54,0x35,0x60,0xf,0x6f,0xf3,0xf,0xfd,0xf2,0xf,0xf9,0xda,0xf,0x6f,0xd4,0xf,0x71
.byte 0xf4,0x2,0xf,0xfd,0xe2,0xf,0xfd,0xd2,0xf,0x6f,0xeb,0xf,0x71,0xf3,0x2,0xf,0xfd,0xdd,0xf,0xfd,0xed,0xf
.byte 0xfd,0xe5,0xf,0xf9,0xd3,0xf,0xfd,0x25
.long 0x0 + mask2

.byte 0xf,0x6f,0xe9,0xf,0xfd,0x15
.long 0x0 + mask2

.byte 0xf,0x71,0xe4,0x2,0xf,0x71,0xe2,0x2,0xf,0xfd,0xcc,0xf,0x6f,0xd8,0xf,0xfd,0xc2,0xf,0xf9,0xec,0xf,0xf9
.byte 0xda,0xf,0x7f,0x4c,0x35,0x0,0xf,0x7f,0x44,0x35,0x10,0xf,0x7f,0x6c,0x35,0x30,0xf,0x6f,0xcf,0xf,0x6f,0x64
.byte 0x35,0x70,0xf,0x6f,0xee,0xf,0x7f,0x5c,0x35,0x20,0xf,0x71,0xf6,0x2,0xf,0x6f,0xd4,0xf,0x71,0xf4,0x2,0xf
.byte 0xfd,0xe2,0xf,0xfd,0xf5,0xf,0xfd,0xed,0xf,0xfd,0xe5,0xf,0xfd,0x25
.long 0x0 + mask2

.byte 0xf,0xfd,0xd2,0xf,0xfd,0x15
.long 0x0 + mask2

.byte 0xf,0x71,0xe4,0x2,0xf,0x6f,0x6c,0x35,0x40,0xf,0xf9,0xd6,0xf,0x71,0xe2,0x2,0xf,0x6f,0xdd,0xf,0xfd,0xfa
.byte 0xf,0xf9,0xca,0xf,0xfd,0xec,0xf,0xf9,0xdc,0xf,0x7f,0x7c,0x35,0x50,0xf,0x7f,0x4c,0x35,0x60,0xf,0x7f,0x6c
.byte 0x35,0x40,0xf,0x7f,0x5c,0x35,0x70,0x83,0xc5,0x8,0x83,0xfd,0x10,0xf,0x85,0x8e,0xfe,0xff,0xff,0x33,0xed,0x90
.byte 0x90,0x90,0xf,0x6f,0x4c,0x35,0x0,0xf,0x6f,0x6c,0x35,0x20,0xf,0x6f,0xd9,0xf,0x61,0x4c,0x35,0x10,0xf,0x6f
.byte 0xf5,0xf,0x69,0x5c,0x35,0x10,0xf,0x61,0x6c,0x35,0x30,0xf,0x6f,0xd1,0xf,0x69,0x74,0x35,0x30,0xf,0x62,0xcd
.byte 0xf,0x6f,0xe3,0xf,0x6a,0xd5,0xf,0x62,0xde,0xf,0x6f,0xc1,0xf,0x6a,0xe6,0xf,0x6f,0xea,0xf,0x6f,0xf4,0xf
.byte 0x71,0xf5,0x2,0xf,0x71,0xf6,0x2,0xf,0xf9,0xd5,0xf,0xf9,0xe6,0xf,0x6f,0xfd,0xf,0xfd,0x2d
.long 0x0 + mask4

.byte 0xf,0xf9,0xfa,0xf,0xf9,0xfe,0xf,0xf9,0xf4,0xf,0xfd,0x3d
.long 0x0 + mask4

.byte 0xf,0xfd,0xee,0xf,0x6f,0x54,0x35,0x8,0xf,0x71,0xe7,0x3,0xf,0xfd,0xcf,0xf,0x71,0xe5,0x3,0xf,0xf9,0xc7
.byte 0xf,0x6f,0xe5,0xf,0x6f,0x7c,0x35,0x28,0xf,0xfd,0xe3,0xf,0x6f,0xf7,0xf,0xf9,0xeb,0xf,0x7f,0x4c,0x35,0x0
.byte 0xf,0x6f,0xda,0xf,0x61,0x54,0x35,0x18,0xf,0x69,0x7c,0x35,0x38,0xf,0x6f,0xca,0xf,0x7f,0x64,0x35,0x30,0xf
.byte 0x69,0x5c,0x35,0x18,0xf,0x61,0x74,0x35,0x38,0xf,0x6f,0xe3,0xf,0x6a,0xe7,0xf,0x62,0xdf,0xf,0x6f,0xfc,0xf
.byte 0x62,0xce,0xf,0xfd,0xfb,0xf,0x6a,0xd6,0xf,0x6f,0xf1,0xf,0xf9,0xe3,0xf,0xfd,0xca,0xf,0xf9,0xf2,0xf,0x6f
.byte 0xd0,0xf,0x7f,0x64,0x35,0x20,0xf,0xfd,0xc6,0xf,0x7f,0x4c,0x35,0x10,0xf,0x6f,0xe5,0xf,0xf9,0xd6,0xf,0x71
.byte 0xf4,0x2,0xf,0x6f,0xf7,0xf,0xfd,0xe5,0xf,0x71,0xf6,0x2,0xf,0xfd,0xed,0xf,0xfd,0xf7,0xf,0xfd,0xff,0xf
.byte 0xfd,0xe7,0xf,0xf9,0xee,0xf,0xfd,0x25
.long 0x0 + mask2

.byte 0xf,0x6f,0xc8,0xf,0xfd,0x2d
.long 0x0 + mask2

.byte 0xf,0x71,0xe4,0x2,0xf,0xfd,0xcc,0xf,0x71,0xe5,0x2,0xf,0x6f,0xda,0xf,0xf9,0xc4,0xf,0xfd,0xd5,0xf,0xf9
.byte 0xdd,0xf,0x6f,0xe1,0xf,0x61,0xca,0xf,0x6f,0xeb,0xf,0x69,0xe2,0xf,0x61,0xd8,0xf,0x69,0xe8,0xf,0x6f,0xd1
.byte 0xf,0x62,0xcb,0xf,0x6a,0xd3,0xf,0x6f,0xc4,0xf,0xfd,0xd
.long 0x0 + mask1

.byte 0xf,0x62,0xc5,0xf,0xfd,0x15
.long 0x0 + mask1

.byte 0xf,0x6a,0xe5,0xf,0xfd,0x5
.long 0x0 + mask1

.byte 0xf,0x71,0xe1,0x1,0xf,0xfd,0x25
.long 0x0 + mask1

.byte 0xf,0x71,0xe2,0x1,0xf,0x7f,0x4f,0x8,0xf,0x71,0xe0,0x1,0xf,0x7f,0x54,0x39,0x8,0xf,0x71,0xe4,0x1,0xf
.byte 0x7f,0x44,0x4f,0x8,0xf,0x7f,0x64,0x3a,0x8,0xf,0x6f,0x4c,0x35,0x0,0xf,0x6f,0x54,0x35,0x10,0xf,0x6f,0xc1
.byte 0xf,0x6f,0x5c,0x35,0x20,0xf,0xfd,0xca,0xf,0x6f,0x64,0x35,0x30,0xf,0xf9,0xc2,0xf,0x6f,0xeb,0xf,0x6f,0xf4
.byte 0xf,0x71,0xf5,0x2,0xf,0x6f,0xd1,0xf,0x71,0xf6,0x2,0xf,0xfd,0xeb,0xf,0xfd,0xdb,0xf,0xfd,0xf4,0xf,0xfd
.byte 0xe4,0xf,0xfd,0xf3,0xf,0xfd,0x35
.long 0x0 + mask2

.byte 0xf,0xf9,0xe5,0xf,0xfd,0x25
.long 0x0 + mask2

.byte 0xf,0x71,0xe6,0x2,0xf,0x71,0xe4,0x2,0xf,0x6f,0xd8,0xf,0xfd,0xce,0xf,0xf9,0xd6,0xf,0xfd,0xc4,0xf,0xf9
.byte 0xdc,0xf,0x6f,0xe1,0xf,0x61,0xc8,0xf,0x6f,0xeb,0xf,0x69,0xe0,0xf,0x61,0xda,0xf,0x69,0xea,0xf,0x6f,0xd1
.byte 0xf,0x62,0xcb,0xf,0x6a,0xd3,0xf,0x6f,0xc4,0xf,0xfd,0xd
.long 0x0 + mask1

.byte 0xf,0x6a,0xe5,0xf,0xfd,0x15
.long 0x0 + mask1

.byte 0xf,0x62,0xc5,0xf,0xfd,0x25
.long 0x0 + mask1

.byte 0xf,0x71,0xe1,0x1,0xf,0xfd,0x5
.long 0x0 + mask1

.byte 0xf,0x71,0xe2,0x1,0xf,0x7f,0xf,0xf,0x71,0xe4,0x1,0xf,0x7f,0x14,0x39,0xf,0x71,0xe0,0x1,0xf,0x7f,0x24
.byte 0x3a,0xf,0x7f,0x4,0x4f,0x83,0xc5,0x40,0x8d,0x3c,0x8f,0x81,0xfd,0x80,0x0,0x0,0x0,0xf,0x85,0x99,0xfd,0xff
.byte 0xff,0xeb,0x0,0x8b,0x9c,0x24,0x64,0x3,0x0,0x0,0x8b,0xac,0x24,0x4c,0x3,0x0,0x0,0x83,0xfb,0x1,0xf,0x84
.byte 0x0,0x1,0x0,0x0,0x8a,0x45,0x9,0x8b,0x8c,0x24,0x1c,0x3,0x0,0x0,0x3c,0x0,0xf,0x84,0xee,0x0,0x0,0x0
.byte 0x8b,0xb4,0x24,0x40,0x3,0x0,0x0,0x8b,0xbc,0x24,0x44,0x3,0x0,0x0,0x8d,0x14,0x49,0xf,0x6f,0x7,0xf,0xfd
.byte 0x4,0x24,0xf,0x6f,0x4f,0x8,0xf,0xfd,0x4c,0x24,0x8,0xf,0x7f,0x6,0xf,0x7f,0x4e,0x8,0xf,0x6f,0x4,0x39
.byte 0xf,0xfd,0x44,0x24,0x10,0xf,0x6f,0x4c,0x39,0x8,0xf,0xfd,0x4c,0x24,0x18,0xf,0x7f,0x4,0x31,0xf,0x7f,0x4c
.byte 0x31,0x8,0xf,0x6f,0x4,0x4f,0xf,0xfd,0x44,0x24,0x20,0xf,0x6f,0x4c,0x4f,0x8,0xf,0xfd,0x4c,0x24,0x28,0xf
.byte 0x7f,0x4,0x4e,0xf,0x7f,0x4c,0x4e,0x8,0xf,0x6f,0x4,0x3a,0xf,0xfd,0x44,0x24,0x30,0xf,0x6f,0x4c,0x3a,0x8
.byte 0xf,0xfd,0x4c,0x24,0x38,0xf,0x7f,0x4,0x32,0xf,0x7f,0x4c,0x32,0x8,0x8d,0x3c,0x8f,0x8d,0x34,0x8e,0xf,0x6f
.byte 0x44,0x24,0x40,0xf,0xfd,0x7,0xf,0x6f,0x4f,0x8,0xf,0xfd,0x4c,0x24,0x48,0xf,0x7f,0x6,0xf,0x7f,0x4e,0x8
.byte 0xf,0x6f,0x4,0x39,0xf,0xfd,0x44,0x24,0x50,0xf,0x6f,0x4c,0x39,0x8,0xf,0xfd,0x4c,0x24,0x58,0xf,0x7f,0x4
.byte 0x31,0xf,0x7f,0x4c,0x31,0x8,0xf,0x6f,0x4,0x4f,0xf,0xfd,0x44,0x24,0x60,0xf,0x6f,0x4c,0x4f,0x8,0xf,0xfd
.byte 0x4c,0x24,0x68,0xf,0x7f,0x4,0x4e,0xf,0x7f,0x4c,0x4e,0x8,0xf,0x6f,0x4,0x3a,0xf,0xfd,0x44,0x24,0x70,0xf
.byte 0x6f,0x4c,0x3a,0x8,0xf,0xfd,0x4c,0x24,0x78,0xf,0x7f,0x4,0x32,0xf,0x7f,0x4c,0x32,0x8,0x8b,0x84,0x24,0x28
.byte 0x3,0x0,0x0,0x8b,0xac,0x24,0x4c,0x3,0x0,0x0,0xd1,0xe0,0x8b,0x8c,0x24,0x30,0x3,0x0,0x0,0x8b,0x94,0x24
.byte 0x18,0x3,0x0,0x0,0x78,0x38,0x2b,0xd1,0x8b,0x9c,0x24,0x2c,0x3,0x0,0x0,0x83,0xfa,0x8,0x7e,0x11,0x83,0xfb
.byte 0x8,0x7e,0x21,0x89,0x84,0x24,0x28,0x3,0x0,0x0,0xe9,0x1c,0xe8,0xff,0xff,0xd1,0xe0,0x78,0x15,0x83,0xfb,0x8
.byte 0x7e,0x10,0x89,0x84,0x24,0x28,0x3,0x0,0x0,0xe9,0x7,0xe8,0xff,0xff,0x3c,0x2,0x74,0xdb,0xc7,0x84,0x24,0x28
.byte 0x3,0x0,0x0,0x1,0x0,0x0,0x8,0x83,0xc5,0x20,0x8b,0x8c,0x24,0x30,0x3,0x0,0x0,0x89,0xac,0x24,0x4c,0x3
.byte 0x0,0x0,0x83,0xc1,0x10,0x8b,0x84,0x24,0x20,0x3,0x0,0x0,0x89,0x8c,0x24,0x30,0x3,0x0,0x0,0x48,0x89,0x84
.byte 0x24,0x20,0x3,0x0,0x0,0xf,0x85,0x9a,0xe7,0xff,0xff,0x8b,0x84,0x24,0x2c,0x3,0x0,0x0,0x83,0xe8,0x10,0x89
.byte 0x84,0x24,0x2c,0x3,0x0,0x0,0x7e,0x49,0x8b,0x84,0x24,0x18,0x3,0x0,0x0,0x8b,0x9c,0x24,0x1c,0x3,0x0,0x0
.byte 0xc1,0xe0,0x4,0x8b,0xb4,0x24,0x50,0x3,0x0,0x0,0xc1,0xe3,0x4,0x8b,0xbc,0x24,0x58,0x3,0x0,0x0,0x3,0xf0
.byte 0x8b,0xac,0x24,0x54,0x3,0x0,0x0,0x3,0xfb,0x89,0xb4,0x24,0x50,0x3,0x0,0x0,0x3,0xeb,0x89,0xbc,0x24,0x58
.byte 0x3,0x0,0x0,0x89,0xac,0x24,0x54,0x3,0x0,0x0,0xe9,0xce,0xe6,0xff,0xff,0x8b,0xa4,0x24,0xc,0x3,0x0,0x0
.byte 0xf,0x77,0x81,0xc4,0x68,0x3,0x0,0x0,0x5b,0x5d,0x5f,0x5e,0xc3

.data
.align 16

.data1:

m128:

.byte 0x80,0x0,0x80,0x0,0x80,0x0,0x80,0x0
BlockOffsetJumpTable:

.long 0x0 + BLOCK0

.long 0x0 + BLOCK1

.long 0x0 + BLOCK_CONTINUE

.long 0x0 + BLOCK2

.long 0x0 + BLOCK_CONTINUE

.long 0x0 + BLOCK_CONTINUE

.long 0x0 + BLOCK_CONTINUE

.long 0x0 + BLOCK3

mask4:

.byte 0x4,0x0,0x4,0x0,0x4,0x0,0x4,0x0
mask2:

.byte 0x2,0x0,0x2,0x0,0x2,0x0,0x2,0x0
mask1:

.byte 0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0