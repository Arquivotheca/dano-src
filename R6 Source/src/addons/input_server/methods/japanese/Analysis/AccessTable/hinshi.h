/************************************************************************
ファイル名	:	hinshi.h

作成日	:	97.12.5
************************************************************************/

#ifndef __HINSHI_HEADER__
#define __HINSHI_HEADER__

typedef long	Category;

#define	HINSHI(pre,post)				(((pre) << 16) | ((post) & 0xffff))
#define	PRE_HINSHI(hinshi)				(short)(((hinshi) >> 16) & 0xffff)
#define	POST_HINSHI(hinshi)				(short)((hinshi) & 0xffff)
#define	SET_PRE_HINSHI(hinshi,pre)		(((hinshi) & 0xffff) | ((post) << 16))
#define	SET_POST_HINSHI(hinshi,post)		(((hinshi) & 0xffff0000) | ((post) & 0xffff))

enum{
	kSahenMeishi		= -1,	/* サ変動詞＋名詞		*/
	kZahenMeishi		= -2,	/* ザ変動詞＋名詞		*/
	kKeidouMeishi		= -3,	/* 形容動詞＋名詞		*/
	kSahenGokan		= 56,	/* サ変動詞語幹		*/
	kZahenGokan		= 57,	/* ザ変動詞語幹		*/
	kKeidouGokan		= 92,	/* 形容動詞語幹		*/

	TANJI			= 3,		/* 単字				*/
	kNanashi			= 2,		/* 名無し				*/
	kFutuuMeishi		= 4,		/* 普通名詞			*/
	kKoyuMeishi		= 7,		/* 固有名詞			*/
	suushi			= 8,		/* 数詞				*/
	kTimei			= 10,	/* 地名				*/
	kJinmei			= 11,	/* 人名				*/
	kSetto			= 12,	/* 接頭語				*/
	kSetubi			= 13,	/* 接尾語				*/
	preJyosuushi		= 17,	/* 接頭助数詞			*/
	postJyosuushi		= 18,	/* 接尾助数詞			*/
	
	kRentai			= 24,	/* 連体詞				*/
	kFukushi			= 25,	/* 副詞				*/
	kSetuzoku			= 26,	/* 接続詞				*/
	kKando			= 27,	/* 感動詞				*/

	kWa5Gokan			= 28,	/* アワ行五段／語幹	*/
	kKa5Gokan			= 29,	/* カ行五段／語幹		*/
	kGa5Gokan			= 30,	/* ガ行五段／語幹		*/
	kSa5Gokan			= 31,	/* サ行五段／語幹		*/
	kTa5Gokan			= 32,	/* タ行五段／語幹		*/
	kNa5Gokan			= 33,	/* ナ行五段／語幹		*/
	kBa5Gokan			= 34,	/* バ行五段／語幹		*/
	kMa5Gokan			= 35,	/* マ行五段／語幹		*/
	kRa5Gokan			= 36,	/* ラ行五段／語幹		*/

	kItidanGokan		= 39,	/* 一段／語幹			*/
	kItidanMizen		= 40,	/* 一段／未然			*/
	kItidanRenyo		= 41,	/* 一段／連用			*/

	kKeiyoGokan		= 91,	/* 形容詞／語幹			*/

	kHojoItidanGokan1	= 76,	/* 補助動詞一段／語幹１	*/
	kHojoItidanGokan2	= 77,	/* 補助動詞一段／語幹２	*/
	kHojoItidanMizen1	= 78,	/* 補助動詞一段／未然１	*/
	kHojoItidanMizen2	= 79,	/* 補助動詞一段／未然２	*/
	kHojoItidanRenyo1	= 80,	/* 補助動詞一段／連用１	*/
	kHojoItidanRenyo2	= 81		/* 補助動詞一段／連用２	*/
};

#endif //__HINSHI_HEADER__
