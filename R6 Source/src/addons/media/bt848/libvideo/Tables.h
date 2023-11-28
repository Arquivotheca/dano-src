/*
	
	Tables.h
	
	Copyright 1998 Be Incorporated, All Rights Reserved.
	
*/

uint32 RGB8Map[256] = 
{
/*00*/		0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 
/*08*/		0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,

/*10*/		0x000000, 0x00003f, 0x00007f, 0x0000bf, 0x0000ff, 0x3f0000, 0x3f003f, 0x3f007f,
/*18*/		0x3f00bf, 0x3f00ff, 0x7f0000, 0x7f003f, 0x7f007f, 0x7f00bf, 0x7f00ff, 0xbf0000,
/*20*/		0xbf003f, 0xbf007f, 0xbf00bf, 0xbf00ff, 0xff0000, 0xff003f, 0xff007f, 0xff00bf,
/*28*/		0xff00ff, 0x001f00, 0x001f3f, 0x001f7f, 0x001fbf, 0x001fff, 0x3f1f00, 0x3f1f3f,
/*30*/		0x3f1f7f, 0x3f1fbf, 0x3f1fff, 0x7f1f00, 0x7f1f3f, 0x7f1f7f, 0x7f1fbf, 0x7f1fff,

/*38*/		0xbf1f00, 0xbf1f3f, 0xbf1f7f, 0xbf1fbf, 0xbf1fff, 0xff1f00, 0xff1f3f, 0xff1f7f,
/*40*/		0xff1fbf, 0xff1fff, 0x003f00, 0x003f3f, 0x003f7f, 0x003fbf, 0x003fff, 0x3f3f00,
/*48*/		0x3f3f3f, 0x3f3f7f, 0x3f3fbf, 0x3f3fff, 0x7f3f00, 0x7f3f3f, 0x7f3f7f, 0x7f3fbf,
/*50*/		0x7f3fff, 0xbf3f00, 0xbf3f3f, 0xbf3f7f, 0xbf3fbf, 0xbf3fff, 0xff3f00, 0xff3f3f,
/*58*/		0xff3f7f, 0xff3fbf, 0xff3fff, 0x005f00, 0x005f3f, 0x005f7f, 0x005fbf, 0x005fff,

/*60*/		0x3f5f00, 0x3f5f3f, 0x3f5f7f, 0x3f5fbf, 0x3f5fff, 0x7f5f00, 0x7f5f3f, 0x7f5f7f,
/*68*/		0x7f5fbf, 0x7f5fff, 0xbf5f00, 0xbf5f3f, 0xbf5f7f, 0xbf5fbf, 0xbf5fff, 0xff5f00,
/*70*/		0xff5f3f, 0xff5f7f, 0xff5fbf, 0xff5fff, 0x007f00, 0x007f3f, 0x007f7f, 0x007fbf,
/*78*/		0x007fff, 0x3f7f00, 0x3f7f3f, 0x3f7f7f, 0x3f7fbf, 0x3f7fff, 0x7f7f00, 0x7f7f3f,
/*80*/		0x7f7f7f, 0x7f7fbf, 0x7f7fff, 0xbf7f00, 0xbf7f3f, 0xbf7f7f, 0xbf7fbf, 0xbf7fff,

/*88*/		0xff7f00, 0xff7f3f, 0xff7f7f, 0xff7fbf, 0xff7fff, 0x009f00, 0x009f3f, 0x009f7f,
/*90*/		0x009fbf, 0x009fff, 0x3f9f00, 0x3f9f3f, 0x3f9f7f, 0x3f9fbf, 0x3f9fff, 0x7f9f00,
/*98*/		0x7f9f3f, 0x7f9f7f, 0x7f9fbf, 0x7f9fff, 0xbf9f00, 0xbf9f3f, 0xbf9f7f, 0xbf9fbf,
/*a0*/		0xbf9fff, 0xff9f00, 0xff9f3f, 0xff9f7f, 0xff9fbf, 0xff9fff, 0x00bf00, 0x00bf3f,
/*a8*/		0x00bf7f, 0x00bfbf, 0x00bfff, 0x3fbf00, 0x3fbf3f, 0x3fbf7f, 0x3fbfbf, 0x3fbfff,

/*b0*/		0x7fbf00, 0x7fbf3f, 0x7fbf7f, 0x7fbfbf, 0x7fbfff, 0xbfbf00, 0xbfbf3f, 0xbfbf7f,
/*b8*/		0xbfbfbf, 0xbfbfff, 0xffbf00, 0xffbf3f, 0xffbf7f, 0xffbfbf, 0xffbfff, 0x00df00,
/*c0*/		0x00df3f, 0x00df7f, 0x00dfbf, 0x00dfff, 0x3fdf00, 0x3fdf3f, 0x3fdf7f, 0x3fdfbf,
/*c8*/		0x3fdfff, 0x7fdf00, 0x7fdf3f, 0x7fdf7f, 0x7fdfbf, 0x7fdfff, 0xbfdf00, 0xbfdf3f,
/*d0*/		0xbfdf7f, 0xbfdfbf, 0xbfdfff, 0xffdf00, 0xffdf3f, 0xffdf7f, 0xffdfbf, 0xffdfff,

/*d8*/		0x00ff00, 0x00ff3f, 0x00ff7f, 0x00ffbf, 0x00ffff, 0x3fff00, 0x3fff3f, 0x3fff7f,
/*e0*/		0x3fffbf, 0x3fffff, 0x7fff00, 0x7fff3f, 0x7fff7f, 0x7fffbf, 0x7fffff, 0xbfff00,
/*e8*/		0xbfff3f, 0xbfff7f, 0xbfffbf, 0xbfffff, 0xffff00, 0xffff3f, 0xffff7f, 0xffffbf,

/*f0*/		0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
/*f8*/		0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff
};
