
.text
.align 16

.text1:

.globl RTCompressBegin

RTCompressBegin:

.byte 0x83,0xec,0xc,0x8b,0x4c,0x24,0x14,0x53,0x55,0x56,0x8b,0xc1,0x57,0xc1,0xe8,0x2,0x33,0xff,0x81,0xf9,0x80,0x2
.byte 0x0,0x0,0x89,0x44,0x24,0x18,0x89,0x7c,0x24,0x10,0xf,0x87,0x80,0x4,0x0,0x0,0x8b,0x5c,0x24,0x20,0x81,0xfb
.byte 0xe0,0x1,0x0,0x0,0xf,0x87,0x70,0x4,0x0,0x0,0x8b,0x2d
.long 0x0 + _imp__GlobalAlloc

.byte 0x6a,0x64,0x6a,0x40,0xff,0xd5,0x8b,0xf0,0x3b,0xf7,0x75,0xd,0xc7,0x44,0x24,0x10,0x1,0x0,0x0,0x0,0xe9,0x53
.byte 0x4,0x0,0x0,0x8b,0x44,0x24,0x24,0x8b,0x54,0x24,0x2c,0x89,0x46,0x18,0x89,0x46,0x14,0x8b,0x44,0x24,0x28,0x89
.byte 0x5e,0x10,0x8b,0x5c,0x24,0x30,0x33,0xc9,0x89,0x46,0x4c,0x3b,0xd7,0x8b,0xc3,0x89,0x7e,0x50,0xf,0x95,0xc1,0xf7
.byte 0xd8,0x1b,0xc0,0x89,0x4e,0x54,0x8b,0x15
.long 0x0 + BRCInitGlobalQuant

.byte 0x83,0xe0,0x3,0x40,0x89,0x56,0x58,0x89,0x7e,0x5c,0x89,0x5e,0x60,0x89,0x7e,0x44,0x89,0x7e,0x48,0x89,0x7e,0xc
.byte 0x89,0x6,0xa1
.long 0x0 + NUM_FRAME_BUFS

.byte 0x8b,0xc8,0xc1,0xe1,0x3,0x2b,0xc8,0xc1,0xe1,0x3,0x51,0x6a,0x40,0xff,0xd5,0x3b,0xc7,0x89,0x46,0x4,0x75,0xd
.byte 0xc7,0x44,0x24,0x10,0x1,0x0,0x0,0x0,0xe9,0xde,0x3,0x0,0x0,0x89,0x7e,0x1c,0xc7,0x46,0x20,0x1,0x0,0x0
.byte 0x0,0x89,0x78,0x2c,0x8b,0x15
.long 0x0 + NUM_FRAME_BUFS

.byte 0x33,0xc9,0x3b,0xd7,0x76,0x33,0x33,0xc0,0x8b,0x56,0x4,0x41,0x89,0x7c,0x10,0x34,0x8b,0x56,0x4,0x89,0x7c,0x10
.byte 0x8,0x8b,0x56,0x4,0x89,0x7c,0x10,0xc,0x8b,0x56,0x4,0x89,0x7c,0x10,0x10,0x8b,0x56,0x4,0x89,0x7c,0x10,0x4
.byte 0x8b,0x15
.long 0x0 + NUM_FRAME_BUFS

.byte 0x83,0xc0,0x38,0x3b,0xca,0x72,0xcf,0x3b,0xd7,0x89,0x7c,0x24,0x20,0xf,0x86,0x2c,0x2,0x0,0x0,0x33,0xdb,0x89
.byte 0x7c,0x24,0x28,0xeb,0x2,0x33,0xff,0x8b,0x6,0x83,0xc0,0x2,0x8b,0xc8,0xc1,0xe1,0x3,0x2b,0xc8,0xc1,0xe1,0x2
.byte 0x51,0x6a,0x40,0xff,0xd5,0x8b,0x56,0x4,0x89,0x44,0x13,0x4,0x8b,0x46,0x4,0x39,0x7c,0x3,0x4,0xf,0x84,0x12
.byte 0x3,0x0,0x0,0x8b,0xe,0x83,0xf9,0x1,0xf,0x86,0xae,0x0,0x0,0x0,0x8d,0x81,0x5f,0x9,0x0,0x0,0xc1,0xe0
.byte 0x6,0x25,0x0,0xf0,0xff,0xff,0x5,0x0,0x10,0x0,0x0,0x89,0x44,0x24,0x14,0xf,0xaf,0xc1,0x5,0x60,0x10,0x0
.byte 0x0,0x8b,0xf8,0x57,0x6a,0x40,0xff,0xd5,0x8b,0x4e,0x4,0x89,0x44,0xb,0x8,0x8b,0x56,0x4,0x8b,0x54,0x13,0x8
.byte 0x85,0xd2,0xf,0x84,0x28,0x3,0x0,0x0,0x8b,0xcf,0xd1,0xe9,0x74,0x10,0xb8,0x0,0x4,0x0,0x4,0x8b,0xfa,0xd1
.byte 0xe9,0xf3,0xab,0x13,0xc9,0x66,0xf3,0xab,0x8b,0x46,0x4,0x3,0xc3,0x8b,0x48,0x8,0x81,0xc1,0xff,0xf,0x0,0x0
.byte 0x81,0xe1,0x0,0xf0,0xff,0xff,0x83,0xc1,0x40,0x89,0x48,0x14,0x8b,0x6,0xb9,0x1,0x0,0x0,0x0,0x3b,0xc1,0xf
.byte 0x86,0x92,0x0,0x0,0x0,0x8b,0x54,0x24,0x14,0x8d,0x7a,0x40,0x8d,0x53,0x18,0x89,0x7c,0x24,0x14,0x8b,0x46,0x4
.byte 0x83,0xc2,0x4,0x8b,0x6c,0x3,0x14,0x3,0xef,0x41,0x89,0x6c,0x2,0xfc,0x8b,0x6c,0x24,0x14,0x8b,0x6,0x3,0xfd
.byte 0x3b,0xc8,0x72,0xe3,0x8b,0x2d
.long 0x0 + _imp__GlobalAlloc

.byte 0xeb,0x5f,0x8b,0x7e,0x14,0x8b,0x46,0x10,0x83,0xc7,0xf,0x83,0xc0,0xf,0xc1,0xef,0x4,0xc1,0xe8,0x4,0xf,0xaf
.byte 0xf8,0xc1,0xe7,0x9,0x83,0xc7,0x20,0x57,0x6a,0x40,0xff,0xd5,0x8b,0x4e,0x4,0x89,0x44,0xb,0x8,0x8b,0x56,0x4
.byte 0x8b,0x4c,0x13,0x8,0x8d,0x4,0x13,0x85,0xc9,0xf,0x84,0x7d,0x2,0x0,0x0,0x83,0xc1,0x1f,0x83,0xe1,0xe0,0x89
.byte 0x48,0x14,0x8b,0x46,0x4,0xd1,0xef,0x8b,0x54,0x3,0x8,0x74,0x12,0x8b,0xcf,0xb8,0x0,0x4,0x0,0x4,0x8b,0xfa
.byte 0xd1,0xe9,0xf3,0xab,0x13,0xc9,0x66,0xf3,0xab,0x8b,0x46,0x14,0x8b,0xd
.long 0x0 + UVSUBSAMPLE

.byte 0x33,0xd2,0xf7,0xf1,0x33,0xd2,0x8b,0xf8,0x8b,0x46,0x10,0xf7,0xf1,0x83,0xc7,0x7,0xc1,0xef,0x3,0x83,0xc0,0x3
.byte 0xc1,0xe8,0x2,0xf,0xaf,0xf8,0x47,0xc1,0xe7,0x5,0x57,0x6a,0x40,0xff,0xd5,0x8b,0x4e,0x4,0x57,0x6a,0x40,0x89
.byte 0x44,0xb,0xc,0xff,0xd5,0x8b,0x56,0x4,0x89,0x44,0x13,0x10,0x8b,0x46,0x4,0x8b,0x4c,0x3,0xc,0x85,0xc9,0xf
.byte 0x84,0x7,0x2,0x0,0x0,0x8b,0x54,0x3,0x10,0x85,0xd2,0xf,0x84,0xfb,0x1,0x0,0x0,0x8b,0x16,0x8b,0x7c,0x24
.byte 0x28,0x83,0xc1,0x1f,0x3,0xd7,0x83,0xe1,0xe0,0x89,0x4c,0x90,0x14,0x8b,0x46,0x4,0x8b,0x16,0x8b,0x4c,0x3,0x10
.byte 0x3,0xd7,0x83,0xc1,0x1f,0x83,0xe1,0xe0,0x89,0x4c,0x90,0x18,0x8b,0x46,0x14,0x8b,0x4e,0x10,0xd1,0xe8,0xd1,0xe9
.byte 0x83,0xc0,0x7,0x83,0xc1,0x7,0xc1,0xe8,0x3,0xc1,0xe9,0x3,0xf,0xaf,0xc1,0x40,0xc1,0xe0,0x5,0x50,0x6a,0x40
.byte 0xff,0xd5,0x8b,0x56,0x4,0x89,0x44,0x13,0x34,0x8b,0x46,0x4,0x8b,0x4c,0x3,0x34,0x85,0xc9,0xf,0x84,0x9c,0x1
.byte 0x0,0x0,0x8b,0x44,0x24,0x20,0x8b,0xd
.long 0x0 + NUM_FRAME_BUFS

.byte 0x40,0x83,0xc7,0xe,0x83,0xc3,0x38,0x3b,0xc1,0x89,0x44,0x24,0x20,0x89,0x7c,0x24,0x28,0xf,0x82,0xe2,0xfd,0xff
.byte 0xff,0x8b,0x5c,0x24,0x30,0x33,0xff,0xa1
.long 0x0 + NUM_MOTION_BUFS

.byte 0x8d,0xc,0x80,0xc1,0xe1,0x3,0x2b,0xc8,0xc1,0xe1,0xb,0x81,0xc1,0x0,0x10,0x0,0x0,0x51,0x6a,0x40,0xff,0xd5
.byte 0x3b,0xc7,0x89,0x46,0x34,0x75,0xd,0xc7,0x44,0x24,0x10,0x1,0x0,0x0,0x0,0xe9,0x2b,0x1,0x0,0x0,0x8b,0x56
.byte 0x4,0xb9,0x1,0x0,0x0,0x0,0x89,0x42,0x30,0x8b,0x46,0x34,0x5,0xff,0xf,0x0,0x0,0x25,0x0,0xf0,0xff,0xff
.byte 0x89,0x46,0x34,0xa1
.long 0x0 + NUM_MOTION_BUFS

.byte 0x3b,0xc1,0x76,0x1d,0x8d,0x46,0x38,0x8b,0x50,0xfc,0x83,0xc0,0x4,0x81,0xc2,0x0,0x38,0x1,0x0,0x41,0x89,0x50
.byte 0xfc,0x8b,0x15
.long 0x0 + NUM_MOTION_BUFS

.byte 0x3b,0xca,0x72,0xe6,0x8b,0x46,0x4,0x8b,0x4e,0x34,0x89,0x48,0x2c,0x8b,0x46,0x4,0x8b,0x50,0x2c,0x8b,0x48,0x30
.byte 0x2b,0xd1,0x89,0x50,0x30,0x8b,0x46,0x4,0x8b,0x4e,0x38,0x33,0xd2,0x89,0x48,0x64,0x8b,0x46,0x14,0xf,0xaf,0x46
.byte 0x10,0xc1,0xe0,0x2,0xf7,0x36,0x50,0x6a,0x40,0xff,0xd5,0x3b,0xc7,0x89,0x46,0x44,0x75,0xd,0xc7,0x44,0x24,0x10
.byte 0x1,0x0,0x0,0x0,0xe9,0xa3,0x0,0x0,0x0,0x3b,0xdf,0x74,0x10,0x8b,0x46,0x14,0x33,0xd2,0xf7,0x35
.long 0x0 + UVSUBSAMPLE

.byte 0x8d,0x4,0x80,0xeb,0xa,0x8b,0x4e,0x14,0x8b,0xc1,0xc1,0xe0,0x4,0x3,0xc1,0x50,0x6a,0x40,0xff,0xd5,0x3b,0xc7
.byte 0x89,0x46,0x48,0x75,0xa,0xc7,0x44,0x24,0x10,0x1,0x0,0x0,0x0,0xeb,0x6f,0x6a,0xc,0x6a,0x40,0xff,0xd5,0x3b
.byte 0xc7,0x89,0x46,0x8,0x75,0xa,0xc7,0x44,0x24,0x10,0x1,0x0,0x0,0x0,0xeb,0x58,0x50
call 0x0 + VersionInit

.byte 0x83,0xc4,0x4,0x6a,0x54,0x6a,0x40,0xff,0xd5,0x3b,0xc7,0x89,0x46,0xc,0x75,0xa,0xc7,0x44,0x24,0x10,0x1,0x0
.byte 0x0,0x0,0xeb,0x38,0x8b,0x54,0x24,0x2c,0x52,0x50
call 0x0 + BRCInit

.byte 0x83,0xc4,0x8
call 0x0 + BuildQuantTables

call 0x0 + InitQuantTables

call 0x0 + InitRVTables

.byte 0x8b,0x44,0x24,0x24,0x8b,0x4c,0x24,0x18,0x50,0x51
call 0x0 + InitMERowOffset

.byte 0x83,0xc4,0x8
call 0x0 + InitMETables

.byte 0xeb,0x2,0x33,0xf6,0x39,0x7c,0x24,0x10,0x74,0xb,0x56
call 0x0 + RTCompressEnd

.byte 0x83,0xc4,0x4,0x33,0xf6,0x8b,0xc6,0x5f,0x5e,0x5d,0x5b,0x83,0xc4,0xc,0xc3,0xc7,0x44,0x24,0x10,0x1,0x0,0x0
.byte 0x0,0x33,0xff,0xeb,0xd9,0x90

.text
.align 16

.text2:

.globl RTCompressReallyBegin

RTCompressReallyBegin:

.byte 0x8b,0x4c,0x24,0x8,0x8b,0x44,0x24,0x4,0x33,0xd2,0x3b,0xca,0x74,0x18,0xc7,0x40,0x54,0x1,0x0,0x0,0x0,0x89
.byte 0x50,0x5c,0x8b,0x40,0xc,0x51,0x50
call 0x0 + BRCInit

.byte 0x83,0xc4,0x8,0xc3,0x8b,0x48,0xc,0x52,0x51,0x89,0x50,0x54,0x89,0x50,0x5c
call 0x0 + BRCInit

.byte 0x83,0xc4,0x8,0xc3,0x90,0x90,0x90,0x90,0x90,0x90

.text
.align 16

.text3:

.globl RTCompressEnd

RTCompressEnd:

.byte 0x57,0x8b,0x7c,0x24,0x8,0x85,0xff,0xf,0x84,0xb4,0x0,0x0,0x0,0x8b,0x47,0xc,0x56,0x55,0x53,0x8b,0x1d
.long 0x0 + _imp__GlobalFree

.byte 0x85,0xc0,0x74,0x3,0x50,0xff,0xd3,0x8b,0x47,0x8,0x85,0xc0,0x74,0x3,0x50,0xff,0xd3,0x8b,0x47,0x48,0x85,0xc0
.byte 0x74,0x3,0x50,0xff,0xd3,0x8b,0x47,0x44,0x85,0xc0,0x74,0x3,0x50,0xff,0xd3,0x8b,0x47,0x4,0x85,0xc0,0x74,0x76
.byte 0xa1
.long 0x0 + NUM_FRAME_BUFS

.byte 0x33,0xed,0x85,0xc0,0x76,0x55,0x33,0xf6,0x8b,0x47,0x4,0x8b,0x44,0x6,0x34,0x85,0xc0,0x74,0x3,0x50,0xff,0xd3
.byte 0x8b,0x4f,0x4,0x8b,0x44,0xe,0x8,0x85,0xc0,0x74,0x3,0x50,0xff,0xd3,0x8b,0x57,0x4,0x8b,0x44,0x16,0xc,0x85
.byte 0xc0,0x74,0x3,0x50,0xff,0xd3,0x8b,0x47,0x4,0x8b,0x44,0x6,0x10,0x85,0xc0,0x74,0x3,0x50,0xff,0xd3,0x8b,0x4f
.byte 0x4,0x8b,0x44,0xe,0x4,0x85,0xc0,0x74,0x3,0x50,0xff,0xd3,0xa1
.long 0x0 + NUM_FRAME_BUFS

.byte 0x45,0x83,0xc6,0x38,0x3b,0xe8,0x72,0xad,0x8b,0x4f,0x4,0x8b,0x41,0x2c,0x85,0xc0,0x74,0x6,0x2b,0x41,0x30,0x50
.byte 0xff,0xd3,0x8b,0x57,0x4,0x52,0xff,0xd3,0x57,0xff,0xd3,0x5b,0x5d,0x5e,0x5f,0xc3,0x90,0x90,0x90,0x90,0x90,0x90
.byte 0x90,0x90,0x90,0x90,0x90,0x90,0x90

.text
.align 16

.text4:

.globl RTCompress

RTCompress:

.byte 0x83,0xec,0x38,0x53,0x55,0x56,0x8b,0x74,0x24,0x48,0x33,0xc9,0xba,0x1,0x0,0x0,0x0,0x8b,0x46,0x44,0x57,0x89
.byte 0x44,0x24,0x4c,0x8b,0x44,0x24,0x70,0x89,0x44,0x24,0x1c,0x89,0x54,0x24,0x14,0x88,0x8,0x8b,0x46,0x60,0x3b,0xc1
.byte 0xc6,0x44,0x24,0x20,0x0,0x89,0x4c,0x24,0x18,0x74,0x20,0x8b,0x44,0x24,0x60,0x83,0xf8,0x14,0x76,0x9,0xc7,0x46
.byte 0x30,0x3,0x0,0x0,0x0,0xeb,0x30,0x83,0xf8,0xa,0x76,0x23,0xc7,0x46,0x30,0x2,0x0,0x0,0x0,0xeb,0x22,0x8b
.byte 0x46,0x10,0xf,0xaf,0x46,0x14,0x3d,0xc8,0xaf,0x0,0x0,0x73,0x5,0x89,0x4e,0x30,0xeb,0xf,0x83,0x7c,0x24,0x60
.byte 0xa,0x77,0x5,0x89,0x4e,0x30,0xeb,0x3,0x89,0x56,0x30,0x8b,0x5c,0x24,0x64,0x8d,0x4c,0x24,0x60,0x51,0x8b,0x13
.byte 0x52,0x56
call 0x0 + SetFrameInfo

.byte 0x89,0x3,0x8b,0x56,0x1c,0x8b,0x4e,0x4,0x8b,0xfa,0xc1,0xe7,0x3,0x2b,0xfa,0x8b,0x56,0x20,0x8b,0xea,0x83,0xc4
.byte 0xc,0xc1,0xe5,0x3,0x8d,0x3c,0xf9,0x2b,0xea,0x8d,0xc,0xe9,0x89,0x7,0x8b,0x44,0x24,0x6c,0x33,0xed,0x89,0x4c
.byte 0x24,0x70,0x8b,0xd
.long 0x0 + PicTypeK

.byte 0x3b,0xc5,0x74,0xe,0x8b,0x13,0xc7,0x44,0x24,0x6c,0x1,0x0,0x0,0x0,0x3b,0xd1,0x74,0x4,0x89,0x6c,0x24,0x6c
.byte 0x3b,0xc5,0x74,0xe,0x8b,0x3,0xc7,0x44,0x24,0x10,0x1,0x0,0x0,0x0,0x3b,0xc1,0x75,0x4,0x89,0x6c,0x24,0x10
.byte 0x8b,0x44,0x24,0x74,0x33,0xd2,0x8b,0x8,0x3b,0xcd,0x8b,0x4e,0xc,0xf,0x95,0xc2,0x89,0x56,0x54,0x8b,0x0,0x50
.byte 0x51
call 0x0 + BRCInitIfRequired

.byte 0x8b,0x46,0x54,0x83,0xc4,0x8,0x3b,0xc5,0x74,0x21,0x8b,0x17,0x8b,0x44,0x24,0x5c,0x8b,0x4e,0x58,0x52,0x8b,0x56
.byte 0x5c,0x50,0x8b,0x46,0xc,0x51,0x52,0x50
call 0x0 + BRCAdjustGlobalQuant

.byte 0x83,0xc4,0x14,0x89,0x46,0x58,0xeb,0x18,0x8b,0x4c,0x24,0x68,0xb8,0x1f,0x85,0xeb,0x51,0xf,0xaf,0xd
.long 0x0 + MaxQuantLevel

.byte 0xf7,0xe1,0xc1,0xea,0x5,0x89,0x56,0x58,0x8b,0x54,0x24,0x60,0x8b,0x46,0x58,0x8b,0x4e,0x10,0x52,0x8b,0x56,0x14
.byte 0x50,0x8b,0x6,0x51,0x52,0x50,0x57
call 0x0 + SetBandInfo

.byte 0x8b,0x5f,0x4,0x83,0xc4,0x18,0x8b,0x4b,0x8,0x8b,0x13,0x8d,0x44,0xa,0xff,0x33,0xd2,0xf7,0xf1,0x33,0xd2,0x8b
.byte 0xe8,0x8b,0x43,0x4,0x8d,0x44,0x8,0xff,0xf7,0xf1,0xf,0xaf,0xe8,0x8b,0x46,0x60,0x89,0x6c,0x24,0x5c,0x85,0xc0
.byte 0x74,0x2a,0x8b,0x4f,0x2c,0x8b,0x57,0x14,0x8b,0x46,0x18,0x51,0x8b,0x4e,0x14,0x52,0x8b,0x56,0x10,0x50,0x8b,0x44
.byte 0x24,0x5c,0x51,0x52,0x50
call 0x0 + FwdGlblXfrm

.byte 0x8b,0x9c,0x24,0x84,0x0,0x0,0x0,0x83,0xc4,0x18,0xeb,0x25,0x8b,0x5c,0x24,0x6c,0x85,0xdb,0x75,0x1d,0x8b,0x4f
.byte 0x2c,0x8b,0x56,0x18,0x8b,0x46,0x14,0x51,0x8b,0x4e,0x10,0x52,0x8b,0x54,0x24,0x58,0x50,0x51,0x52
call 0x0 + SSCopyYPlane

.byte 0x83,0xc4,0x14,0x8b,0x46,0x60,0x8b,0xf,0x85,0xc0,0x8b,0x47,0x4,0x51,0x8b,0x50,0x8,0x74,0x9,0x8b,0x48,0x4
.byte 0x52,0x8b,0x10,0x51,0xeb,0xd,0x8b,0x48,0x4,0xd1,0xea,0x52,0x8b,0x10,0xd1,0xe9,0x51,0xd1,0xea,0x8b,0x47,0x34
.byte 0x52,0x50
call 0x0 + SetBlockInfo

.byte 0x8b,0x46,0x60,0x83,0xc4,0x14,0x85,0xc0,0x74,0xc,0x8b,0x4e,0x58,0x8b,0xc,0x8d
.long 0x0 + MEClassifyRatioScalable

.byte 0xeb,0xa,0x8b,0x56,0x58,0x8b,0xc,0x95
.long 0x0 + MEClassifyRatioNonScalable

.byte 0x85,0xdb,0x75,0x27,0x8b,0x47,0x2c,0x8d,0x54,0x24,0x14,0x52,0x8b,0x54,0x24,0x74,0x51,0x8b,0x4c,0x24,0x18,0x51
.byte 0x8b,0x4a,0x2c,0x50,0x2b,0xc1,0x50,0x8b,0x47,0x34,0x50
call 0x0 + MotionEstimation

.byte 0x83,0xc4,0x18,0xeb,0x4,0x8b,0x44,0x24,0x74,0x85,0xdb,0x75,0x12,0x8b,0x4e,0x58,0x8b,0x57,0x34,0x51,0x50,0x52
.byte 0x55
call 0x0 + QuantDeltas

.byte 0x83,0xc4,0x10,0x8b,0x4e,0x30,0x8b,0x5c,0x24,0x14,0x8b,0x15
.long 0x0 + PicTypeP

.byte 0x85,0xc9,0x75,0xa,0x8b,0x7,0x3b,0xc2,0x75,0x4,0x85,0xdb,0x74,0x14,0x85,0xc9,0x74,0x30,0x8b,0x7,0x8b,0xd
.long 0x0 + PicTypeD

.byte 0x3b,0xc1,0x75,0x24,0x85,0xdb,0x75,0x20,0x3b,0xc2,0x75,0x9,0x56
call 0x0 + UnSetFrameIndices

.byte 0x83,0xc4,0x4,0xa1
.long 0x0 + PicTypeR

.byte 0x8b,0x54,0x24,0x64,0x89,0x7,0x8b,0xd
.long 0x0 + PicTypeR

.byte 0x89,0xa,0x8b,0x7,0x8b,0x4f,0x4,0x50,0x51,0x8d,0x54,0x24,0x44,0x56,0x8d,0x44,0x24,0x24,0x52,0x50
call 0x0 + WritePicHeader

.byte 0x8b,0xf,0xa1
.long 0x0 + PicTypeR

.byte 0x83,0xc4,0x14,0x3b,0xc8,0xf,0x84,0xe2,0x2,0x0,0x0,0x8b,0x6,0x8b,0x5f,0x4,0x33,0xed,0x8d,0x50,0x2,0x85
.byte 0xd2,0xf,0x86,0x66,0x2,0x0,0x0,0x8b,0x4e,0x44,0x3b,0xe8,0x89,0x4c,0x24,0x4c,0xf,0x83,0x8c,0x0,0x0,0x0
.byte 0x8b,0x46,0x60,0x85,0xc0,0x74,0x4e,0x8b,0x43,0xc,0x8b,0x54,0x24,0x60,0x8b,0x4c,0x24,0x70,0x83,0xf8,0x3,0x52
.byte 0x8d,0x44,0x24,0x50,0x8b,0x54,0xa9,0x14,0x8b,0x4f,0x34,0x55,0x50,0x8b,0x44,0xaf,0x14,0x52,0x50,0x51,0x53,0x74
.byte 0x14
call 0x0 + EncYBands

.byte 0x83,0xc4,0x1c,0x33,0xc0,0xa0
.long 0x0 + COLOR_Y

.byte 0xe9,0xa0,0x0,0x0,0x0
call 0x0 + EncYBandNoXform

.byte 0x83,0xc4,0x1c,0x33,0xc0,0xa0
.long 0x0 + COLOR_Y

.byte 0xe9,0x8c,0x0,0x0,0x0,0x8b,0x54,0x24,0x60,0x8b,0x46,0x48,0x52,0x8b,0x54,0x24,0x74,0x8d,0x4c,0x24,0x50,0x50
.byte 0x8b,0x44,0xaa,0x14,0x8b,0x54,0x24,0x58,0x51,0x8b,0x4c,0xaf,0x14,0x50,0x8b,0x47,0x34,0x51,0x52,0x50,0x53
call 0x0 + EncYPlane

.byte 0x83,0xc4,0x20,0x33,0xc0,0xa0
.long 0x0 + COLOR_Y

.byte 0xeb,0x55,0x75,0x12,0x8b,0x44,0x24,0x54,0x33,0xc9,0x8a,0xd
.long 0x0 + COLOR_V

.byte 0x89,0x4c,0x24,0x64,0xeb,0x10,0x8b,0x44,0x24,0x58,0x33,0xd2,0x8a,0x15
.long 0x0 + COLOR_U

.byte 0x89,0x54,0x24,0x64,0x8b,0x4c,0x24,0x60,0x8b,0x56,0x48,0x51,0x52,0x8b,0x54,0x24,0x78,0x8d,0x4c,0x24,0x54,0x51
.byte 0x8b,0x4c,0xaa,0x14,0x8b,0x54,0xaf,0x14,0x51,0x52,0x50,0x8b,0x47,0x34,0x50,0x53
call 0x0 + EncUVBands

.byte 0x8b,0x84,0x24,0x84,0x0,0x0,0x0,0x83,0xc4,0x20,0x8b,0x17,0x8b,0x4c,0x24,0x18,0x52,0x53,0x50,0x89,0x4c,0x24
.byte 0x78,0x8d,0x44,0x24,0x3c,0x55,0x8d,0x4c,0x24,0x28,0x50,0x51
call 0x0 + WriteBandHeader

.byte 0xa0
.long 0x0 + Tile_Empty_Width

.byte 0x8b,0x54,0x24,0x30,0x83,0xc4,0x18,0x8d,0x4c,0x24,0x18,0x89,0x54,0x24,0x64,0x50,0x6a,0x0,0x51
call 0x0 + BitBuffWrite

.byte 0x83,0xc4,0xc,0x8d,0x54,0x24,0x18,0x6a,0x1,0x6a,0x1,0x52
call 0x0 + BitBuffWrite

.byte 0xa0
.long 0x0 + Small_TD_Width

.byte 0x33,0xc9,0x8a,0xd
.long 0x0 + Max_Small_TD_Size

.byte 0x83,0xc4,0xc,0x41,0x8d,0x54,0x24,0x18,0x50,0x51,0x52
call 0x0 + BitBuffWrite

.byte 0x8b,0x44,0x24,0x24,0x8b,0x4c,0x24,0x28,0x8b,0x54,0x24,0x2c,0x83,0xc4,0xc,0x89,0x44,0x24,0x24,0xa0
.long 0x0 + Large_TD_Width

.byte 0x89,0x4c,0x24,0x28,0x50,0x8d,0x4c,0x24,0x1c,0x6a,0x0,0x51,0x89,0x54,0x24,0x38
call 0x0 + BitBuffWrite

.byte 0x83,0xc4,0xc,0x8d,0x54,0x24,0x18,0x52
call 0x0 + BitBuffByteAlign

.byte 0x8b,0x46,0x60,0x83,0xc4,0x4,0x85,0xc0,0x75,0x1f,0x85,0xed,0x75,0x1b,0x8b,0x7,0x8b,0x4f,0x34,0x8b,0x54,0x24
.byte 0x5c,0x50,0x51,0x8d,0x44,0x24,0x20,0x52,0x50
call 0x0 + WriteBlockInfo16x16

.byte 0x83,0xc4,0x10,0xeb,0x1a,0x8b,0xf,0x8b,0x57,0x34,0x8b,0x44,0x24,0x5c,0x51,0x52,0x50,0x8d,0x4c,0x24,0x24,0x55
.byte 0x51
call 0x0 + WriteBlockInfo

.byte 0x83,0xc4,0x14,0x8d,0x54,0x24,0x18,0x52
call 0x0 + BitBuffByteAlign

.byte 0x8b,0x46,0x44,0x8b,0x4c,0x24,0x50,0x83,0xc4,0x4,0x2b,0xc8,0x8d,0x54,0x24,0x1c,0x51,0x52,0x50
call 0x0 + HuffEnc

.byte 0x8b,0x54,0x24,0x24,0x83,0xc4,0xc,0x3,0xd0,0x8d,0x44,0x24,0x18,0x50,0x89,0x54,0x24,0x1c
call 0x0 + BitBuffByteAlign

.byte 0x8b,0x54,0x24,0x1c,0x8b,0x44,0x24,0x68,0x8a,0xd
.long 0x0 + Large_TD_Width

.byte 0x83,0xc4,0x4,0x2b,0xd0,0x8d,0x44,0x24,0x24,0xc1,0xea,0x3,0x51,0x52,0x50
call 0x0 + BitBuffWrite

.byte 0x8b,0x54,0x24,0x24,0x8b,0x44,0x24,0x78,0x8a,0xd
.long 0x0 + LenBandDataSize

.byte 0x83,0xc4,0xc,0x2b,0xd0,0x8d,0x44,0x24,0x30,0xc1,0xea,0x3,0x51,0x52,0x50
call 0x0 + BitBuffWrite

.byte 0x8b,0x6,0x83,0xc4,0xc,0x83,0xc3,0x1c,0x45,0x8d,0x48,0x2,0x3b,0xe9,0xf,0x82,0x9a,0xfd,0xff,0xff,0x8b,0x4c
.byte 0x24,0x20,0x8b,0x7c,0x24,0x18,0x81,0xe1,0xff,0x0,0x0,0x0,0x8b,0x5c,0x24,0x1c,0x6a,0x18,0x8d,0x4,0xf,0x99
.byte 0x33,0xc2,0x2b,0xc2,0x83,0xe0,0x1f,0x33,0xc2,0x2b,0xc2,0xb2,0x20,0x2a,0xd0,0x88,0x54,0x24,0x54,0x8b,0x44,0x24
.byte 0x54,0x25,0xff,0x0,0x0,0x0,0x8b,0xd0,0xc1,0xea,0x3,0x8d,0x44,0x7,0x20,0x8d,0x54,0x13,0x4,0x89,0x44,0x24
.byte 0x1c,0x3,0xc1,0x89,0x54,0x24,0x20,0x99,0x8b,0x4c,0x24,0x78,0x83,0xe2,0x7,0x3,0xc2,0x8d,0x54,0x24,0x40,0xc1
.byte 0xf8,0x3,0x50,0x52,0x89,0x1
call 0x0 + BitBuffWrite

.byte 0x83,0xc4,0xc,0xeb,0x1f,0x8b,0x4c,0x24,0x20,0x8b,0x44,0x24,0x18,0x81,0xe1,0xff,0x0,0x0,0x0,0x3,0xc1,0x99
.byte 0x83,0xe2,0x7,0x3,0xc2,0x8b,0x54,0x24,0x74,0xc1,0xf8,0x3,0x89,0x2,0x8b,0x5e,0x5c,0x8b,0x7e,0x50,0x8b,0x46
.byte 0x4c,0x43,0x47,0x89,0x5e,0x5c,0x85,0xc0,0x89,0x7e,0x50,0x8b,0xcf,0x74,0xb,0x3b,0xc8,0x72,0x7,0xc7,0x46,0x50
.byte 0x0,0x0,0x0,0x0,0x8b,0x46,0x54,0x85,0xc0,0x74,0x13,0x8b,0x44,0x24,0x74,0x8b,0x56,0xc,0x8b,0x8,0x51,0x52
call 0x0 + BRCUpdate

.byte 0x83,0xc4,0x8,0xa1
.long 0x0 + NoError

.byte 0x5f,0x5e,0x5d,0x5b,0x83,0xc4,0x38,0xc3,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90

.text
.align 16

.text5:

HuffEnc:

.byte 0x8b,0x44,0x24,0xc,0x56,0x8b,0x74,0x24,0xc,0x57,0x85,0xc0,0x8b,0x3e,0x74,0x1b,0x8b,0x4c,0x24,0xc,0x8a,0x15
.long 0x0 + BlkEndStream

.byte 0x88,0x14,0x8,0x40,0x50,0x6a,0x0,0x56,0x51
call 0x0 + VLCEncode0

.byte 0x83,0xc4,0x10,0x8b,0x6,0x2b,0xc7,0x5f,0xc1,0xe0,0x3,0x5e,0xc3,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90
.byte 0x90,0x90

.text
.align 16

.text6:

SetBandInfo:

.byte 0x8b,0x4c,0x24,0x4,0x53,0x8b,0x5c,0x24,0x18,0x55,0x8b,0x6c,0x24,0x18,0x56,0x57,0x8b,0x7c,0x24,0x18,0x33,0xd2
.byte 0x85,0xff,0xf,0x86,0xaf,0x0,0x0,0x0,0x8b,0xef,0x83,0xed,0x4,0xf7,0xdd,0x1b,0xed,0x83,0xe5,0x8,0x83,0xc5
.byte 0x8,0x8b,0x71,0x4,0x8b,0xc2,0xc1,0xe0,0x3,0x2b,0xc2,0xc1,0xe0,0x2,0x83,0xff,0x4,0x89,0x6c,0x30,0x8,0x8b
.byte 0x71,0x4,0xc7,0x44,0x30,0x14,0x0,0x0,0x0,0x0,0x8b,0x71,0x4,0x89,0x5c,0x30,0x10,0x8b,0x74,0x24,0x1c,0x75
.byte 0x2,0xd1,0xee,0x8b,0x59,0x4,0x83,0xff,0x4,0x89,0x74,0x18,0x4,0x8b,0x74,0x24,0x20,0x75,0x2,0xd1,0xee,0x8b
.byte 0x59,0x4,0x83,0xfa,0x3,0x89,0x34,0x18,0x77,0x44,0xff,0x24,0x95
.long 0x0 + _L22809

_L22552:

.byte 0xa1
.long 0x0 + Default_Y_Xfrm

.byte 0x8b,0x71,0x4,0x25,0xff,0x0,0x0,0x0,0x89,0x46,0xc,0xeb,0x2b
_L22553:

.byte 0x8b,0x71,0x4,0x33,0xc0,0xa0
.long 0x1 + Default_Y_Xfrm

.byte 0x89,0x46,0x28,0xeb,0x1c
_L22554:

.byte 0x8b,0x71,0x4,0x33,0xc0,0xa0
.long 0x2 + Default_Y_Xfrm

.byte 0x89,0x46,0x44,0xeb,0xd
_L22555:

.byte 0x8b,0x71,0x4,0x33,0xc0,0xa0
.long 0x3 + Default_Y_Xfrm

.byte 0x89,0x46,0x60,0x8b,0x5c,0x24,0x24,0x42,0x3b,0xd7,0xf,0x82,0x64,0xff,0xff,0xff,0x8b,0x6c,0x24,0x20,0x8d,0x47
.byte 0x2,0x3b,0xf8,0x73,0x63,0x8b,0xf7,0xc1,0xe6,0x3,0x2b,0xf7,0xc1,0xe6,0x2,0x2b,0xc7,0x8b,0xf8,0x8b,0x51,0x4
.byte 0x83,0xc6,0x1c,0xc7,0x44,0x16,0xec,0x4,0x0,0x0,0x0,0x8b,0x41,0x4,0xc7,0x44,0x6,0xf8,0x0,0x0,0x0,0x0
.byte 0x8b,0x51,0x4,0x8b,0x44,0x24,0x1c,0x89,0x5c,0x16,0xf4,0x33,0xd2,0xf7,0x35
.long 0x0 + UVSUBSAMPLE

.byte 0x8b,0x51,0x4,0x89,0x44,0x16,0xe8,0x8b,0xc5,0x33,0xd2,0xf7,0x35
.long 0x0 + UVSUBSAMPLE

.byte 0x8b,0x51,0x4,0x89,0x44,0x16,0xe4,0x8b,0x51,0x4,0x33,0xc0,0x4f,0xa0
.long 0x0 + Default_VU9_Xfrm

.byte 0x89,0x44,0x16,0xf0,0x75,0xab,0x5f,0x5e,0x5d,0x5b,0xc3
_L22809:

.long 0x0 + _L22552

.long 0x0 + _L22553

.long 0x0 + _L22554

.long 0x0 + _L22555

.byte 0x90,0x90,0x90,0x90

.text
.align 16

.text7:

SetBlockInfo:

.byte 0x83,0xec,0x8,0x53,0x55,0x56,0x8b,0x74,0x24,0x24,0x57,0x8b,0x7c,0x24,0x24,0x33,0xd2,0xbd,0x40,0x1,0x0,0x0
.byte 0x8d,0x44,0x3e,0xff,0x8b,0x5c,0x24,0x20,0xf7,0xf6,0x8b,0x4c,0x24,0x1c,0xc7,0x44,0x24,0x10,0x0,0x0,0x0,0x0
.byte 0x2b,0xe8,0x33,0xc0,0xf,0xaf,0xee,0x85,0xdb,0x89,0x6c,0x24,0x24,0xf,0x86,0xfb,0x0,0x0,0x0,0x85,0xc0,0x75
.byte 0x6,0x89,0x44,0x24,0x1c,0xeb,0x27,0x8b,0xd6,0xd1,0xea,0x3,0xd0,0x3,0xd6,0x3b,0xd3,0x76,0xa,0xc7,0x44,0x24
.byte 0x1c,0xc,0x0,0x0,0x0,0xeb,0x11,0x8d,0x14,0x70,0x3b,0xda,0x1b,0xd2,0x83,0xe2,0x4,0x83,0xc2,0x4,0x89,0x54
.byte 0x24,0x1c,0x8d,0x14,0x30,0x3b,0xd3,0x89,0x54,0x24,0x14,0x76,0x7,0x8d,0x46,0xff,0x23,0xc3,0xeb,0x2,0x8b,0xc6
.byte 0xd1,0xe8,0x89,0x44,0x24,0x28,0x33,0xc0,0x85,0xff,0xf,0x86,0x92,0x0,0x0,0x0,0x85,0xc0,0x75,0x4,0x33,0xd2
.byte 0xeb,0x1d,0x8b,0xd6,0xd1,0xea,0x3,0xd0,0x3,0xd6,0x3b,0xd7,0x76,0x7,0xba,0x3,0x0,0x0,0x0,0xeb,0xa,0x8d
.byte 0x14,0x70,0x3b,0xfa,0x1b,0xd2,0xf7,0xda,0x42,0x8d,0x2c,0x30,0x3b,0xef,0x76,0x7,0x8d,0x46,0xff,0x23,0xc7,0xeb
.byte 0x2,0x8b,0xc6,0xd1,0xe8,0x8b,0xd8,0x8b,0x44,0x24,0x2c,0x3b,0x5
.long 0x0 + PicTypeK

.byte 0x8b,0x44,0x24,0x1c,0x75,0x9,0x8d,0x94,0x2,0x1,0x0,0x1,0x0,0xeb,0x4,0x8d,0x54,0x2,0x1,0x8a,0x44,0x24
.byte 0x28,0x89,0x51,0x4,0xf6,0xeb,0x88,0x41,0x8,0x8b,0x44,0x24,0x10,0x89,0x1,0x3,0xc6,0x89,0x44,0x24,0x10,0xc6
.byte 0x41,0x9,0x0,0xc7,0x41,0xc,0x0,0x0,0x0,0x0,0x8b,0xc5,0x83,0xc1,0x20,0x3b,0xc7,0xf,0x82,0x7a,0xff,0xff
.byte 0xff,0x8b,0x5c,0x24,0x20,0x8b,0x6c,0x24,0x24,0x8b,0x54,0x24,0x14,0x8b,0x44,0x24,0x10,0x3,0xc5,0x89,0x44,0x24
.byte 0x10,0x8b,0xc2,0x3b,0xc3,0xf,0x82,0x5,0xff,0xff,0xff,0x5f,0x5e,0x5d,0xc7,0x41,0x4,0xff,0xff,0xff,0xff,0x5b
.byte 0x83,0xc4,0x8,0xc3,0x90,0x90,0x90,0x90,0x90,0x90,0x90

.text
.align 16

.text8:

SetFrameInfo:

.byte 0x8b,0x4c,0x24,0x4,0x53,0x56,0x57,0x8b,0x41,0x30,0x48,0xf,0x84,0x7,0x1,0x0,0x0,0x48,0xf,0x84,0xe9,0x0
.byte 0x0,0x0,0x48,0xf,0x84,0xcb,0x0,0x0,0x0,0x8b,0x41,0x4c,0x33,0xf6,0x3b,0xc6,0xf,0x84,0x8e,0x0,0x0,0x0
.byte 0x8b,0x44,0x24,0x14,0x8b,0x15
.long 0x0 + PicTypeK

.byte 0x3b,0xc2,0x75,0x2b,0x8b,0x51,0x1c,0x8b,0x59,0x34,0x8b,0xfa,0x89,0x71,0x50,0xc1,0xe7,0x3,0x2b,0xfa,0x8b,0x51
.byte 0x4,0x89,0x5c,0xfa,0x2c,0x8b,0x51,0x20,0x8b,0x59,0x38,0x8b,0xfa,0xc1,0xe7,0x3,0x2b,0xfa,0x8b,0x51,0x4,0x89
.byte 0x5c,0xfa,0x2c,0x3b,0x5
.long 0x0 + FrameTypeAuto

.byte 0x75,0x39,0x39,0x71,0x50,0x75,0x2f,0x8b,0x51,0x1c,0x8b,0x79,0x34,0xa1
.long 0x0 + PicTypeK

.byte 0x8b,0xf2,0xc1,0xe6,0x3,0x2b,0xf2,0x8b,0x51,0x4,0x89,0x7c,0xf2,0x2c,0x8b,0x51,0x20,0x8b,0x79,0x38,0x8b,0xf2
.byte 0xc1,0xe6,0x3,0x2b,0xf2,0x8b,0x51,0x4,0x89,0x7c,0xf2,0x2c,0xeb,0x5,0xa1
.long 0x0 + PicTypeP

.byte 0x8b,0x51,0x4c,0x8b,0x79,0x50,0x8b,0x74,0x24,0x18,0x4a,0x3b,0xfa,0x1b,0xd2,0x42,0x89,0x16,0xeb,0x1a,0x8b,0x44
.byte 0x24,0x18,0x89,0x71,0x50,0x89,0x30,0x8b,0x44,0x24,0x14,0x3b,0x5
.long 0x0 + FrameTypeAuto

.byte 0x75,0x5,0xa1
.long 0x0 + PicTypeP

.byte 0x83,0x79,0x4c,0x1,0x74,0x51,0x8b,0x51,0x1c,0x8b,0x71,0x20,0x89,0x71,0x1c,0x89,0x51,0x20,0x5f,0x5e,0x5b,0xc3
.byte 0x8b,0x54,0x24,0x18,0x8b,0x44,0x24,0x14,0x52,0x50,0x51
call 0x0 + SetSequenceC

.byte 0x83,0xc4,0xc,0x5f,0x5e,0x5b,0xc3,0x8b,0x54,0x24,0x18,0x8b,0x44,0x24,0x14,0x52,0x50,0x51
call 0x0 + SetSequenceB

.byte 0x83,0xc4,0xc,0x5f,0x5e,0x5b,0xc3,0x8b,0x54,0x24,0x18,0x8b,0x44,0x24,0x14,0x52,0x50,0x51
call 0x0 + SetSequenceA

.byte 0x83,0xc4,0xc,0x5f,0x5e,0x5b,0xc3,0x90

.text
.align 16

.text9:

SetSequenceA:

.byte 0x83,0xec,0x8,0x8b,0x4c,0x24,0xc,0x53,0x33,0xd2,0xbb,0x1,0x0,0x0,0x0,0x8b,0x41,0x4c,0x56,0x3b,0xc2,0x57
.byte 0x88,0x54,0x24,0xc,0x88,0x5c,0x24,0xd,0x88,0x5c,0x24,0xe,0x88,0x54,0x24,0xf,0x88,0x5c,0x24,0x10,0x88,0x54
.byte 0x24,0x11,0x88,0x54,0x24,0x12,0x88,0x5c,0x24,0x13,0xf,0x84,0x1,0x1,0x0,0x0,0x3b,0xc3,0x74,0x3a,0x8b,0x41
.byte 0x1c,0x8b,0x79,0x4,0x8d,0x34,0xc5,0x0,0x0,0x0,0x0,0x2b,0xf0,0x8b,0x34,0xf7,0x8b,0x3d
.long 0x0 + PicTypeK

.byte 0x3b,0xf7,0x74,0x12,0x3b,0x35
.long 0x0 + PicTypeP

.byte 0x74,0xa,0x3b,0x35
.long 0x0 + PicTypeP2

.byte 0x75,0x8,0xeb,0x3,0x89,0x41,0x24,0x89,0x41,0x28,0x8b,0x41,0x2c,0x89,0x41,0x1c,0x8b,0x74,0x24,0x1c,0xa1
.long 0x0 + PicTypeK

.byte 0x3b,0xf0,0x75,0x19,0x8b,0x41,0x4,0x8b,0x79,0x34,0x89,0x51,0x1c,0x89,0x51,0x50,0x89,0x78,0x2c,0xa1
.long 0x0 + PicTypeK

.byte 0x89,0x59,0x2c,0xeb,0x4,0x8b,0x44,0x24,0x20,0x3b,0x35
.long 0x0 + FrameTypeAuto

.byte 0x75,0x78,0x8b,0x41,0x50,0x3b,0xc2,0x75,0x16,0xa1
.long 0x0 + PicTypeK

.byte 0x8b,0x71,0x34,0x89,0x51,0x1c,0x8b,0x51,0x4,0x89,0x72,0x2c,0x89,0x59,0x2c,0xeb,0x5b,0x8b,0xd0,0x83,0xe2,0x3
.byte 0x84,0xc3,0x75,0xd,0x8b,0x71,0x24,0xa1
.long 0x0 + PicTypeP

.byte 0x89,0x71,0x20,0xeb,0xe,0x8b,0x71,0x28,0xa1
.long 0x0 + PicTypeD

.byte 0x89,0x71,0x20,0x8b,0x71,0x1c,0xf,0xbe,0x7c,0x14,0xc,0x89,0x71,0x2c,0x8b,0x71,0x1c,0x8b,0x7c,0xb9,0x34,0x8b
.byte 0xde,0xc1,0xe3,0x3,0x2b,0xde,0x8b,0x71,0x4,0x89,0x7c,0xde,0x2c,0xf,0xbe,0x74,0x14,0x10,0x8b,0x51,0x20,0x8b
.byte 0x74,0xb1,0x34,0x8b,0xfa,0xc1,0xe7,0x3,0x2b,0xfa,0x8b,0x51,0x4,0x89,0x74,0xfa,0x2c,0x8b,0x51,0x4c,0x8b,0x71
.byte 0x50,0x4a,0x3b,0xf2,0x8b,0x54,0x24,0x20,0x1b,0xc9,0x41,0x89,0xa,0x5f,0x5e,0x5b,0x83,0xc4,0x8,0xc3,0x8b,0x44
.byte 0x24,0x20,0x55,0x89,0x10,0x8b,0x44,0x24,0x20,0x3b,0x5
.long 0x0 + FrameTypeAuto

.byte 0xf,0x85,0xae,0x0,0x0,0x0,0x8b,0x41,0x1c,0x8b,0x79,0x4,0x8b,0x35
.long 0x0 + PicTypeK

.byte 0x8d,0x14,0xc5,0x0,0x0,0x0,0x0,0x2b,0xd0,0x8b,0x14,0xd7,0x3b,0xd6,0x74,0x12,0x3b,0x15
.long 0x0 + PicTypeP

.byte 0x74,0xa,0x3b,0x15
.long 0x0 + PicTypeP2

.byte 0x75,0x8,0xeb,0x3,0x89,0x41,0x24,0x89,0x41,0x28,0x8b,0x41,0x50,0x8b,0x71,0x2c,0x8b,0xd0,0x89,0x71,0x1c,0x83
.byte 0xe2,0x3,0x84,0xc3,0x75,0x1d,0x8b,0x71,0x24,0xa1
.long 0x0 + PicTypeP

.byte 0xf,0xbe,0x5c,0x14,0x10,0x89,0x71,0x20,0x89,0x71,0x2c,0x8b,0x71,0x1c,0x8b,0xee,0xc1,0xe5,0x3,0xeb,0x17,0x8b
.byte 0x59,0x28,0xa1
.long 0x0 + PicTypeD

.byte 0x89,0x59,0x20,0x8d,0x2c,0xf5,0x0,0x0,0x0,0x0,0xf,0xbe,0x5c,0x14,0x10,0x2b,0xee,0x8b,0x74,0x99,0x34,0x89
.byte 0x74,0xef,0x2c,0x5d,0xf,0xbe,0x74,0x14,0x10,0x8b,0x51,0x20,0x8b,0xfa,0xc1,0xe7,0x3,0x2b,0xfa,0x8b,0x51,0x4
.byte 0x8b,0x4c,0xb1,0x34,0x89,0x4c,0xfa,0x2c,0x8b,0x4c,0x24,0x20,0x33,0xd2,0x89,0x11,0x5f,0x5e,0x5b,0x83,0xc4,0x8
.byte 0xc3,0x8b,0x41,0x4,0x8b,0x71,0x34,0x89,0x51,0x1c,0x89,0x51,0x50,0x89,0x70,0x2c,0xa1
.long 0x0 + PicTypeK

.byte 0x89,0x59,0x2c,0x8b,0x4c,0x24,0x24,0x5d,0x5f,0x5e,0x89,0x11,0x5b,0x83,0xc4,0x8,0xc3,0x90,0x90,0x90,0x90,0x90
.byte 0x90,0x90,0x90,0x90

.text
.align 16

.text10:

SetSequenceB:

.byte 0x83,0xec,0x18,0xb1,0x2,0x53,0x88,0x4c,0x24,0x6,0x88,0x4c,0x24,0x8,0x88,0x4c,0x24,0x11,0x88,0x4c,0x24,0x15
.byte 0x88,0x4c,0x24,0x17,0x8b,0x4c,0x24,0x20,0x33,0xc0,0x55,0x8b,0x51,0x4c,0xbb,0x1,0x0,0x0,0x0,0x56,0x3b,0xd0
.byte 0x57,0x88,0x44,0x24,0x10,0x88,0x5c,0x24,0x11,0x88,0x5c,0x24,0x13,0xc6,0x44,0x24,0x15,0x3,0x88,0x5c,0x24,0x18
.byte 0x88,0x44,0x24,0x19,0x88,0x5c,0x24,0x1a,0x88,0x44,0x24,0x1b,0x88,0x5c,0x24,0x1c,0x88,0x5c,0x24,0x20,0x88,0x5c
.byte 0x24,0x22,0x88,0x44,0x24,0x24,0x88,0x44,0x24,0x25,0xf,0x84,0x25,0x1,0x0,0x0,0x3b,0xd3,0x74,0x3a,0x8b,0x51
.byte 0x1c,0x8b,0x79,0x4,0x8d,0x34,0xd5,0x0,0x0,0x0,0x0,0x2b,0xf2,0x8b,0x34,0xf7,0x8b,0x3d
.long 0x0 + PicTypeK

.byte 0x3b,0xf7,0x74,0x12,0x3b,0x35
.long 0x0 + PicTypeP

.byte 0x74,0xa,0x3b,0x35
.long 0x0 + PicTypeP2

.byte 0x75,0x8,0xeb,0x3,0x89,0x51,0x24,0x89,0x51,0x28,0x8b,0x51,0x2c,0x89,0x51,0x1c,0x8b,0x74,0x24,0x30,0x8b,0x15
.long 0x0 + PicTypeK

.byte 0x3b,0xf2,0x75,0x19,0x8b,0x51,0x4,0x8b,0x79,0x34,0x89,0x41,0x1c,0x89,0x41,0x50,0x89,0x7a,0x2c,0x8b,0x15
.long 0x0 + PicTypeK

.byte 0x8b,0xfa,0xeb,0x4,0x8b,0x7c,0x24,0x34,0x3b,0x35
.long 0x0 + FrameTypeAuto

.byte 0xf,0x85,0x81,0x0,0x0,0x0,0x8b,0x79,0x50,0x3b,0xf8,0x75,0x13,0x89,0x41,0x1c,0x8b,0x41,0x4,0x8b,0xfa,0x8b
.byte 0x51,0x34,0x89,0x50,0x2c,0x89,0x59,0x2c,0xeb,0x67,0x8b,0xc7,0x33,0xd2,0xbe,0x6,0x0,0x0,0x0,0xf7,0xf6,0x8b
.byte 0xc7,0xbf,0x3,0x0,0x0,0x0,0x8b,0xf2,0x33,0xd2,0xf7,0xf7,0x85,0xd2,0x75,0xb,0x8b,0x3d
.long 0x0 + PicTypeP

.byte 0x8b,0x41,0x24,0xeb,0x9,0x8b,0x3d
.long 0x0 + PicTypeP2

.byte 0x8b,0x41,0x28,0xf,0xbe,0x54,0x34,0x10,0x89,0x41,0x20,0x8b,0x41,0x1c,0x8b,0x54,0x91,0x34,0x8b,0xd8,0xc1,0xe3
.byte 0x3,0x2b,0xd8,0x8b,0x41,0x4,0x89,0x54,0xd8,0x2c,0x8b,0x41,0x20,0xf,0xbe,0x54,0x34,0x18,0x8b,0xf0,0x8b,0x54
.byte 0x91,0x34,0xc1,0xe6,0x3,0x2b,0xf0,0x8b,0x41,0x4,0x89,0x54,0xf0,0x2c,0x8b,0x71,0x50,0x33,0xd2,0x8b,0xc6,0xbb
.byte 0x6,0x0,0x0,0x0,0xf7,0xf3,0xf,0xbe,0x44,0x14,0x20,0x89,0x41,0x2c,0x8b,0x49,0x4c,0x8b,0x44,0x24,0x34,0x49
.byte 0x3b,0xf1,0x1b,0xd2,0x42,0x89,0x10,0x8b,0xc7,0x5f,0x5e,0x5d,0x5b,0x83,0xc4,0x18,0xc3,0x8b,0x54,0x24,0x34,0x89
.byte 0x2,0x8b,0x54,0x24,0x30,0x3b,0x15
.long 0x0 + FrameTypeAuto

.byte 0xf,0x85,0xbf,0x0,0x0,0x0,0x8b,0x41,0x1c,0x8b,0x69,0x4,0x8b,0x35
.long 0x0 + PicTypeK

.byte 0x8d,0x14,0xc5,0x0,0x0,0x0,0x0,0x2b,0xd0,0x8b,0x54,0xd5,0x0,0x3b,0xd6,0x74,0x12,0x3b,0x15
.long 0x0 + PicTypeP

.byte 0x74,0xa,0x3b,0x15
.long 0x0 + PicTypeP2

.byte 0x75,0x8,0xeb,0x3,0x89,0x41,0x24,0x89,0x41,0x28,0x8b,0x79,0x50,0x33,0xd2,0x8b,0xc7,0xbb,0x6,0x0,0x0,0x0
.byte 0xf7,0xf3,0x8b,0xc7,0xbf,0x3,0x0,0x0,0x0,0x8b,0x71,0x2c,0x89,0x71,0x1c,0x8b,0xda,0x33,0xd2,0xf7,0xf7,0x85
.byte 0xd2,0x75,0xb,0x8b,0x3d
.long 0x0 + PicTypeP

.byte 0x8b,0x41,0x24,0xeb,0x9,0x8b,0x3d
.long 0x0 + PicTypeP2

.byte 0x8b,0x41,0x28,0xf,0xbe,0x54,0x1c,0x10,0x89,0x41,0x20,0x8d,0x4,0xf5,0x0,0x0,0x0,0x0,0x8b,0x54,0x91,0x34
.byte 0x2b,0xc6,0x89,0x54,0xc5,0x2c,0x8b,0x41,0x20,0xf,0xbe,0x54,0x1c,0x18,0x8b,0xf0,0x8b,0x54,0x91,0x34,0xc1,0xe6
.byte 0x3,0x2b,0xf0,0x8b,0x41,0x4,0x89,0x54,0xf0,0x2c,0x8b,0x41,0x50,0x33,0xd2,0xbe,0x6,0x0,0x0,0x0,0xf7,0xf6
.byte 0xf,0xbe,0x44,0x14,0x20,0x89,0x41,0x2c,0x8b,0xc7,0x5f,0x5e,0x5d,0x5b,0x83,0xc4,0x18,0xc3,0x8b,0x51,0x4,0x89
.byte 0x41,0x1c,0x89,0x41,0x50,0x8b,0x41,0x34,0x89,0x42,0x2c,0x8b,0x3d
.long 0x0 + PicTypeK

.byte 0x8b,0xc7,0x5f,0x5e,0x89,0x59,0x2c,0x5d,0x5b,0x83,0xc4,0x18,0xc3,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90
.byte 0x90,0x90,0x90

.text
.align 16

.text11:

SetSequenceC:

.byte 0x83,0xec,0x24,0xb1,0x3,0x53,0x88,0x4c,0x24,0x7,0x88,0x4c,0x24,0x8,0x88,0x4c,0x24,0x15,0x8b,0x4c,0x24,0x2c
.byte 0xb0,0x2,0x55,0x56,0x88,0x44,0x24,0xd,0x88,0x44,0x24,0xe,0x88,0x44,0x24,0x11,0x88,0x44,0x24,0x12,0x88,0x44
.byte 0x24,0x18,0x88,0x44,0x24,0x1b,0x88,0x44,0x24,0x1c,0x88,0x44,0x24,0x1f,0x88,0x44,0x24,0x20,0x88,0x44,0x24,0x26
.byte 0x88,0x44,0x24,0x27,0x88,0x44,0x24,0x2c,0x88,0x44,0x24,0x2d,0x8b,0x41,0x4c,0x33,0xf6,0xbb,0x1,0x0,0x0,0x0
.byte 0x3b,0xc6,0x57,0x88,0x5c,0x24,0x10,0x88,0x5c,0x24,0x17,0x88,0x5c,0x24,0x18,0xc6,0x44,0x24,0x19,0x0,0xc6,0x44
.byte 0x24,0x1a,0x0,0x88,0x5c,0x24,0x1b,0x88,0x5c,0x24,0x1d,0x88,0x5c,0x24,0x1e,0x88,0x5c,0x24,0x22,0x88,0x5c,0x24
.byte 0x25,0x88,0x5c,0x24,0x26,0xc6,0x44,0x24,0x27,0x0,0x88,0x5c,0x24,0x28,0x88,0x5c,0x24,0x29,0x88,0x5c,0x24,0x2c
.byte 0x88,0x5c,0x24,0x2d,0xc6,0x44,0x24,0x2e,0x0,0xc6,0x44,0x24,0x2f,0x0,0xc6,0x44,0x24,0x32,0x0,0xc6,0x44,0x24
.byte 0x33,0x0,0xf,0x84,0x45,0x1,0x0,0x0,0x3b,0xc3,0x74,0x3a,0x8b,0x41,0x1c,0x8b,0x79,0x4,0x8d,0x14,0xc5,0x0
.byte 0x0,0x0,0x0,0x2b,0xd0,0x8b,0x14,0xd7,0x8b,0x3d
.long 0x0 + PicTypeK

.byte 0x3b,0xd7,0x74,0x12,0x3b,0x15
.long 0x0 + PicTypeP

.byte 0x74,0xa,0x3b,0x15
.long 0x0 + PicTypeP2

.byte 0x75,0x8,0xeb,0x3,0x89,0x41,0x24,0x89,0x41,0x28,0x8b,0x41,0x2c,0x89,0x41,0x1c,0x8b,0x54,0x24,0x3c,0xa1
.long 0x0 + PicTypeK

.byte 0x3b,0xd0,0x75,0x18,0x8b,0x41,0x4,0x8b,0x79,0x38,0x89,0x71,0x1c,0x89,0x71,0x50,0x89,0x78,0x2c,0xa1
.long 0x0 + PicTypeK

.byte 0x8b,0xe8,0xeb,0x4,0x8b,0x6c,0x24,0x40,0x3b,0x15
.long 0x0 + FrameTypeAuto

.byte 0xf,0x85,0xa3,0x0,0x0,0x0,0x8b,0x79,0x50,0x3b,0xfe,0x75,0x13,0x8b,0x51,0x4,0x8b,0xe8,0x8b,0x41,0x38,0x89
.byte 0x71,0x1c,0x89,0x42,0x2c,0xe9,0x89,0x0,0x0,0x0,0x8b,0xc7,0x33,0xd2,0xbe,0xc,0x0,0x0,0x0,0xf7,0xf6,0x8b
.byte 0xc7,0x23,0xc3,0x3b,0xc3,0x89,0x44,0x24,0x3c,0x8b,0xf2,0x75,0xe,0x8b,0x51,0x28,0x8b,0x2d
.long 0x0 + PicTypeD

.byte 0x89,0x51,0x20,0xeb,0x2e,0x8b,0xc7,0x33,0xd2,0xbf,0x6,0x0,0x0,0x0,0xf7,0xf7,0x85,0xd2,0x75,0xb,0x8b,0x2d
.long 0x0 + PicTypeP

.byte 0x8b,0x41,0x24,0xeb,0x11,0x8b,0x44,0x24,0x3c,0x85,0xc0,0x75,0x40,0x8b,0x2d
.long 0x0 + PicTypeP2

.byte 0x8b,0x41,0x28,0x89,0x41,0x20,0x8b,0x41,0x1c,0xf,0xbe,0x54,0x34,0x10,0x8b,0xf8,0x8b,0x54,0x91,0x34,0xc1,0xe7
.byte 0x3,0x2b,0xf8,0x8b,0x41,0x4,0x89,0x54,0xf8,0x2c,0x8b,0x41,0x20,0xf,0xbe,0x54,0x34,0x1c,0x8b,0xf0,0x8b,0x54
.byte 0x91,0x34,0xc1,0xe6,0x3,0x2b,0xf0,0x8b,0x41,0x4,0x89,0x54,0xf0,0x2c,0x8b,0x71,0x50,0x33,0xd2,0x8b,0xc6,0xbf
.byte 0xc,0x0,0x0,0x0,0xf7,0xf7,0xf,0xbe,0x44,0x14,0x28,0x89,0x41,0x2c,0x8b,0x49,0x4c,0x8b,0x44,0x24,0x40,0x49
.byte 0x3b,0xf1,0x1b,0xd2,0x42,0x89,0x10,0x8b,0xc5,0x5f,0x5e,0x5d,0x5b,0x83,0xc4,0x24,0xc3,0x8b,0x54,0x24,0x40,0x8b
.byte 0x44,0x24,0x3c,0x89,0x32,0x8b,0x15
.long 0x0 + FrameTypeAuto

.byte 0x3b,0xc2,0xf,0x85,0x2f,0x1,0x0,0x0,0x8b,0x41,0x1c,0x8b,0x51,0x4,0x8d,0x34,0xc5,0x0,0x0,0x0,0x0,0x2b
.byte 0xf0,0x8b,0x14,0xf2,0x8b,0x35
.long 0x0 + PicTypeK

.byte 0x3b,0xd6,0x74,0x12,0x3b,0x15
.long 0x0 + PicTypeP

.byte 0x74,0xa,0x3b,0x15
.long 0x0 + PicTypeP2

.byte 0x75,0x8,0xeb,0x3,0x89,0x41,0x24,0x89,0x41,0x28,0x8b,0x69,0x50,0x33,0xd2,0x8b,0xc5,0xbf,0xc,0x0,0x0,0x0
.byte 0xf7,0xf7,0x8b,0x71,0x2c,0x23,0xeb,0x3b,0xeb,0x89,0x71,0x1c,0x8b,0xfa,0x75,0x27,0x8b,0x51,0x28,0x8b,0x2d
.long 0x0 + PicTypeD

.byte 0xf,0xbe,0x44,0x3c,0x10,0x89,0x51,0x20,0x8d,0x14,0xf5,0x0,0x0,0x0,0x0,0x2b,0xd6,0x8b,0x74,0x81,0x34,0x8b
.byte 0x41,0x4,0x89,0x74,0xd0,0x2c,0xeb,0x7a,0x8b,0x41,0x50,0x33,0xd2,0xbb,0x6,0x0,0x0,0x0,0xf7,0xf3,0x85,0xd2
.byte 0x75,0x41,0xf,0xbe,0x54,0x3c,0x10,0x8b,0x41,0x24,0x8b,0x2d
.long 0x0 + PicTypeP

.byte 0x8d,0x1c,0xf5,0x0,0x0,0x0,0x0,0x89,0x41,0x20,0x8b,0x41,0x4,0x8b,0x54,0x91,0x34,0x2b,0xde,0x89,0x54,0xd8
.byte 0x2c,0x8b,0x41,0x20,0xf,0xbe,0x54,0x3c,0x1c,0x8b,0xf0,0x8b,0x54,0x91,0x34,0xc1,0xe6,0x3,0x2b,0xf0,0x8b,0x41
.byte 0x4,0x89,0x54,0xf0,0x2c,0xeb,0x49,0x85,0xed,0x75,0x41,0xf,0xbe,0x54,0x3c,0x10,0x8b,0x41,0x28,0x8b,0x2d
.long 0x0 + PicTypeP2

.byte 0x8d,0x1c,0xf5,0x0,0x0,0x0,0x0,0x89,0x41,0x20,0x8b,0x41,0x4,0x8b,0x54,0x91,0x34,0x2b,0xde,0x89,0x54,0xd8
.byte 0x2c,0x8b,0x41,0x20,0xf,0xbe,0x54,0x3c,0x1c,0x8b,0xf0,0x8b,0x54,0x91,0x34,0xc1,0xe6,0x3,0x2b,0xf0,0x8b,0x41
.byte 0x4,0x89,0x54,0xf0,0x2c,0xeb,0x4,0x8b,0x6c,0x24,0x40,0x8b,0x41,0x50,0x33,0xd2,0xbe,0xc,0x0,0x0,0x0,0xf7
.byte 0xf6,0xf,0xbe,0x44,0x14,0x28,0x89,0x41,0x2c,0x8b,0xc5,0x5f,0x5e,0x5d,0x5b,0x83,0xc4,0x24,0xc3,0x8b,0x51,0x4
.byte 0x8b,0x41,0x38,0x89,0x71,0x1c,0x89,0x71,0x50,0x89,0x42,0x2c,0x8b,0x2d
.long 0x0 + PicTypeK

.byte 0x5f,0x8b,0xc5,0x5e,0x89,0x59,0x2c,0x5d,0x5b,0x83,0xc4,0x24,0xc3,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90
.byte 0x90

.text
.align 16

.text12:

UnSetFrameIndices:

.byte 0x8b,0x44,0x24,0x4,0x8b,0x48,0x1c,0x8b,0x50,0x20,0x89,0x50,0x1c,0x89,0x48,0x20,0xc3,0x90,0x90,0x90,0x90,0x90
.byte 0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90

.text
.align 16

.text13:

.globl VersionInit

VersionInit:

.byte 0x8b,0x44,0x24,0x4,0xc6,0x0,0x3e,0xc6,0x40,0x1,0x30,0xc6,0x40,0x2,0x1,0xc6,0x40,0x3,0x2,0xc7,0x40,0x4
.byte 0x0,0x0,0x0,0x0,0xc3,0x90,0x90,0x90,0x90,0x90
