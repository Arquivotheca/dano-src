const uint8 RtoY[] = {
	0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02,
	0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04,
	0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06,
	0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08,
	0x08, 0x08, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0a,
	0x0a, 0x0b, 0x0b, 0x0b, 0x0b, 0x0c, 0x0c, 0x0c,
	0x0c, 0x0d, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e, 0x0e,
	0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x10, 0x10,
	0x10, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12,
	0x12, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x14,
	0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16,
	0x17, 0x17, 0x17, 0x17, 0x18, 0x18, 0x18, 0x18,
	0x19, 0x19, 0x19, 0x19, 0x1a, 0x1a, 0x1a, 0x1a,
	0x1b, 0x1b, 0x1b, 0x1b, 0x1c, 0x1c, 0x1c, 0x1d,
	0x1d, 0x1d, 0x1d, 0x1e, 0x1e, 0x1e, 0x1e, 0x1f,
	0x1f, 0x1f, 0x1f, 0x20, 0x20, 0x20, 0x20, 0x21,
	0x21, 0x21, 0x21, 0x22, 0x22, 0x22, 0x22, 0x23,
	0x23, 0x23, 0x23, 0x24, 0x24, 0x24, 0x24, 0x25,
	0x25, 0x25, 0x25, 0x26, 0x26, 0x26, 0x27, 0x27,
	0x27, 0x27, 0x28, 0x28, 0x28, 0x28, 0x29, 0x29,
	0x29, 0x29, 0x2a, 0x2a, 0x2a, 0x2a, 0x2b, 0x2b,
	0x2b, 0x2b, 0x2c, 0x2c, 0x2c, 0x2c, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2e, 0x2e, 0x2e, 0x2e, 0x2f, 0x2f,
	0x2f, 0x30, 0x30, 0x30, 0x30, 0x31, 0x31, 0x31,
	0x31, 0x32, 0x32, 0x32, 0x32, 0x33, 0x33, 0x33,
	0x33, 0x34, 0x34, 0x34, 0x34, 0x35, 0x35, 0x35,
	0x35, 0x36, 0x36, 0x36, 0x36, 0x37, 0x37, 0x37,
	0x37, 0x38, 0x38, 0x38, 0x38, 0x39, 0x39, 0x39,
	0x3a, 0x3a, 0x3a, 0x3a, 0x3b, 0x3b, 0x3b, 0x3b,
	0x3c, 0x3c, 0x3c, 0x3c, 0x3d, 0x3d, 0x3d, 0x3d,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3f, 0x3f, 0x3f, 0x3f,
	0x40, 0x40, 0x40, 0x40, 0x41, 0x41, 0x41, 0x41
};

const uint8 GtoY[] = {
	0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04,
	0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
	0x08, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c,
	0x0c, 0x0d, 0x0d, 0x0e, 0x0e, 0x0f, 0x0f, 0x10,
	0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x13, 0x14,
	0x14, 0x15, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18,
	0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1b, 0x1b, 0x1c,
	0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f, 0x1f, 0x20,
	0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24,
	0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27, 0x28,
	0x28, 0x29, 0x29, 0x2a, 0x2a, 0x2b, 0x2b, 0x2c,
	0x2c, 0x2d, 0x2d, 0x2e, 0x2e, 0x2f, 0x2f, 0x30,
	0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34,
	0x34, 0x35, 0x35, 0x36, 0x36, 0x37, 0x37, 0x38,
	0x38, 0x39, 0x39, 0x3a, 0x3a, 0x3b, 0x3b, 0x3c,
	0x3c, 0x3d, 0x3e, 0x3e, 0x3f, 0x3f, 0x40, 0x40,
	0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
	0x45, 0x45, 0x46, 0x46, 0x47, 0x47, 0x48, 0x48,
	0x49, 0x49, 0x4a, 0x4a, 0x4b, 0x4b, 0x4c, 0x4c,
	0x4d, 0x4d, 0x4e, 0x4e, 0x4f, 0x4f, 0x50, 0x50,
	0x51, 0x51, 0x52, 0x52, 0x53, 0x53, 0x54, 0x54,
	0x55, 0x55, 0x56, 0x56, 0x57, 0x57, 0x58, 0x58,
	0x59, 0x59, 0x5a, 0x5a, 0x5b, 0x5b, 0x5c, 0x5c,
	0x5d, 0x5d, 0x5e, 0x5e, 0x5f, 0x5f, 0x60, 0x60,
	0x61, 0x61, 0x62, 0x62, 0x63, 0x63, 0x64, 0x64,
	0x65, 0x65, 0x66, 0x66, 0x67, 0x67, 0x68, 0x68,
	0x69, 0x69, 0x6a, 0x6a, 0x6b, 0x6b, 0x6c, 0x6c,
	0x6d, 0x6d, 0x6e, 0x6e, 0x6f, 0x6f, 0x70, 0x70,
	0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x74, 0x74,
	0x75, 0x75, 0x76, 0x76, 0x77, 0x77, 0x78, 0x78,
	0x79, 0x79, 0x7a, 0x7b, 0x7b, 0x7c, 0x7c, 0x7d,
	0x7d, 0x7e, 0x7e, 0x7f, 0x7f, 0x80, 0x80, 0x81
};

const uint8 BtoY[] = {
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x11, 0x11,
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
	0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
	0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13,
	0x13, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x14,
	0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x15, 0x15,
	0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
	0x15, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,
	0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17, 0x17,
	0x17, 0x17, 0x17, 0x17, 0x17, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x19,
	0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19,
	0x19, 0x19, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a,
	0x1a, 0x1a, 0x1a, 0x1a, 0x1b, 0x1b, 0x1b, 0x1b,
	0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1c, 0x1c,
	0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c,
	0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d,
	0x1d, 0x1d, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
	0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1f, 0x1f, 0x1f,
	0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
	0x21, 0x21, 0x21, 0x22, 0x22, 0x22, 0x22, 0x22,
	0x22, 0x22, 0x22, 0x22, 0x22, 0x23, 0x23, 0x23,
	0x23, 0x23, 0x23, 0x23, 0x23, 0x23, 0x23, 0x23,
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24,
	0x24, 0x24, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25,
	0x25, 0x25, 0x25, 0x25, 0x26, 0x26, 0x26, 0x26,
	0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x27, 0x27,
	0x27, 0x27, 0x27, 0x27, 0x27, 0x27, 0x27, 0x27,
	0x27, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
	0x28, 0x28, 0x28, 0x29, 0x29, 0x29, 0x29, 0x29
};

const uint8 R5toY[] = {
	0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e,
	0x10, 0x12, 0x15, 0x17, 0x19, 0x1b, 0x1d, 0x1f,
	0x21, 0x23, 0x25, 0x27, 0x29, 0x2b, 0x2d, 0x2f,
	0x31, 0x33, 0x35, 0x37, 0x3a, 0x3c, 0x3e, 0x40
};

const uint8 G5toY[] = {
	0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
	0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c,
	0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d,
	0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d
};

const uint8 B5toY[] = {
	0x10, 0x11, 0x12, 0x12, 0x13, 0x14, 0x15, 0x15,
	0x16, 0x17, 0x18, 0x19, 0x19, 0x1a, 0x1b, 0x1c,
	0x1d, 0x1d, 0x1e, 0x1f, 0x20, 0x20, 0x21, 0x22,
	0x23, 0x24, 0x24, 0x25, 0x26, 0x27, 0x27, 0x28
};

const uint32 R5toYUV[] = {
	0x003500, 0x023404, 0x043307, 0x06310b,
	0x08300e, 0x0a2f12, 0x0c2e15, 0x0e2d19,
	0x102b1c, 0x122a20, 0x152923, 0x172827,
	0x19272a, 0x1b262e, 0x1d2431, 0x1f2335,
	0x212238, 0x23213c, 0x25203f, 0x271e43,
	0x291d46, 0x2b1c4a, 0x2d1b4d, 0x2f1a51,
	0x311854, 0x331758, 0x35165b, 0x37155f,
	0x3a1462, 0x3c1366, 0x3e1169, 0x40106d
};

const uint32 G5toYUV[] = {
	0x004b5e, 0x04495b, 0x084658, 0x0c4455,
	0x104252, 0x143f4f, 0x183d4c, 0x1c3b49,
	0x203846, 0x243643, 0x283441, 0x2c313e,
	0x302f3b, 0x342d38, 0x382a35, 0x3c2832,
	0x41262f, 0x45232c, 0x492129, 0x4d1f26,
	0x511c23, 0x551a20, 0x59181d, 0x5d161a,
	0x611317, 0x651114, 0x690f11, 0x6d0c0e,
	0x710a0c, 0x750809, 0x790506, 0x7d0303
};

const uint32 B5toYUV[] = {
	0x100022, 0x110421, 0x120721, 0x120b20,
	0x130e20, 0x14121f, 0x15151f, 0x15191e,
	0x161c1d, 0x17201d, 0x18231c, 0x19271c,
	0x192a1b, 0x1a2e1b, 0x1b311a, 0x1c3519,
	0x1d3819, 0x1d3c18, 0x1e3f18, 0x1f4317,
	0x204617, 0x204a16, 0x214d15, 0x225115,
	0x235414, 0x245814, 0x245b13, 0x255f13,
	0x266212, 0x276611, 0x276911, 0x286d10
};
