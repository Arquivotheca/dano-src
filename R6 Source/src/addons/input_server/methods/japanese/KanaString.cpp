#include "KanaString.h"
#include "JapaneseCommon.h"

#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <Debug.h>


#define convert_to_unicode(str, uni_str) \
{\
	if ((str[0]&0x80) == 0)\
		*uni_str++ = *str++;\
	else if ((str[1] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=1;\
	} else if ((str[0]&0x20) == 0) {\
		*uni_str++ = ((str[0]&31)<<6) | (str[1]&63);\
		str+=2;\
	} else if ((str[2] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=2;\
	} else if ((str[0]&0x10) == 0) {\
		*uni_str++ = ((str[0]&15)<<12) | ((str[1]&63)<<6) | (str[2]&63);\
		str+=3;\
	} else if ((str[3] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=3;\
	} else {\
		int   val;\
		val = ((str[0]&7)<<18) | ((str[1]&63)<<12) | ((str[2]&63)<<6) | (str[3]&63);\
		uni_str[0] = 0xd7c0+(val>>10);\
		uni_str[1] = 0xdc00+(val&0x3ff);\
		uni_str += 2;\
		str += 4;\
	}\
}

#define convert_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|((*uni_str++)&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|((*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}	


inline int32
UTF8CharLen(uchar c)
{ 
	return (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1);
} 

inline bool
IsInitialUTF8Byte(uchar b)
{
	return ((b & 0xC0) != 0x80);
}

uint16
inline UTF8CharToUnicode(
	const uchar	*src)
{
	uint16	result = 0;
	uint16	*resultPtr = &result;

	convert_to_unicode(src, resultPtr);

	return (result);
}

void
inline UnicodeToUTF8Char(
	uint16	src,
	uchar	*dst)
{
	uint16 *srcPtr = &src;

	convert_to_utf8(dst, srcPtr);		
}


struct KanaMap {
	const int32	romaLen;
	const char	*roma;
	const int32	kanaLen;
	const char	*hira;
	const char	*kata;
};

struct KanaMapInfo {
	const KanaMap	*table;
	const int32		size;
};

struct HanKataMap {
	uint16		zen;
	const int32	hanLen;
	const char	*han;
};

struct KutoutenMap {
	const char	*maru;
	const char	*ten;
};


const uint16 kHiraganaStart			= 0x3040;
const uint16 kHiraganaEnd			= 0x3094;
const uint16 kKatakanaStart			= 0x30A0;
const uint16 kKatakanaEnd			= 0x30F4; 
const uint16 kZenkakuKatakanaOffset	= kKatakanaStart - kHiraganaStart;

const KanaMap kCommonMappings[] = {
	{1, " ", 1, " ", " "},
	{1, ".", 3, "。", "。"},
	{1, ",", 3, "、", "、"},
	{1, "(", 3, "（", "（"},
	{1, ")", 3, "）", "）"},
	{1, "[", 3, "「", "「"},	
	{1, "]", 3, "」", "」"},
	{1, "{", 3, "『", "『"},
	{1, "}", 3, "』", "』"},
	{1, "~", 3, "～", "～"},
	{1, "-", 3, "ー", "ー"},
	{1, "\\", 3, "￥", "￥"},
	{1, "0", 3, "０", "０"},
	{1, "1", 3, "１", "１"},
	{1, "2", 3, "２", "２"},
	{1, "3", 3, "３", "３"},
	{1, "4", 3, "４", "４"},
	{1, "5", 3, "５", "５"},
	{1, "6", 3, "６", "６"},
	{1, "7", 3, "７", "７"},
	{1, "8", 3, "８", "８"},
	{1, "9", 3, "９", "９"},
	{1, "!", 3, "！", "！"},
	{1, "\"", 3, "＂", "＂"},
	{1, "#", 3, "＃", "＃"},
	{1, "$", 3, "＄", "＄"},
	{1, "%", 3, "％", "％"},
	{1, "&", 3, "＆", "＆"},
	{1, "'", 3, "＇", "＇"},
	{1, "*", 3, "＊", "＊"},
	{1, "+", 3, "＋", "＋"},
	{1, "/", 3, "／", "／"},
	{1, ":", 3, "：", "："},
	{1, ";", 3, "；", "；"},
	{1, "<", 3, "＜", "＜"},
	{1, ">", 3, "＞", "＞"},
	{1, "=", 3, "＝", "＝"},
	{1, "?", 3, "？", "？"},
	{1, "@", 3, "＠", "＠"},
	{1, "^", 3, "＾", "＾"},
	{1, "_", 3, "＿", "＿"},
	{1, "`", 3, "｀", "｀"},
	{1, "|", 3, "｜", "｜"}
};
const int32 kNumCommonMappings = sizeof(kCommonMappings) / sizeof(kCommonMappings[0]);

const KanaMap kKanaMappings[] = {
	{2, "bb", 4, "っb", "ッB"},
	{2, "cc", 4, "っc", "ッC"},
	{2, "dd", 4, "っd", "ッD"},
	{2, "ff", 4, "っf", "ッF"},
	{2, "gg", 4, "っg", "ッG"},
	{2, "hh", 4, "っh", "ッH"},
	{2, "jj", 4, "っj", "ッJ"},
	{2, "kk", 4, "っk", "ッK"},
	{2, "ll", 4, "っl", "ッL"},
	{2, "mm", 4, "っm", "ッM"},
	{2, "pp", 4, "っp", "ッP"},
	{2, "qq", 4, "っq", "ッQ"},
	{2, "rr", 4, "っr", "ッR"},
	{2, "ss", 4, "っs", "ッS"},
	{2, "tt", 4, "っt", "ッT"},
	{2, "vv", 4, "っv", "ッV"},
	{2, "ww", 4, "っw", "ッW"},
	{2, "xx", 4, "っx", "ッX"},
	{2, "yy", 4, "っy", "ッY"},
	{2, "zz", 4, "っz", "ッZ"},
	{1, "a", 3, "あ", "ア"},
	{1, "i", 3, "い", "イ"},
	{1, "u", 3, "う", "ウ"},
	{1, "e", 3, "え", "エ"},
	{1, "o", 3, "お", "オ"},
	{2, "la", 3, "ぁ", "ァ"},
	{2, "li", 3, "ぃ", "ィ"},
	{2, "lu", 3, "ぅ", "ゥ"},
	{2, "le", 3, "ぇ", "ェ"},
	{2, "lo", 3, "ぉ", "ォ"},
	{2, "xa", 3, "ぁ", "ァ"},
	{2, "xi", 3, "ぃ", "ィ"},
	{2, "xu", 3, "ぅ", "ゥ"},
	{2, "xe", 3, "ぇ", "ェ"},
	{2, "xo", 3, "ぉ", "ォ"},
	{2, "ka", 3, "か", "カ"},
	{2, "ki", 3, "き", "キ"},
	{2, "ku", 3, "く", "ク"},
	{2, "ke", 3, "け", "ケ"},
	{2, "ko", 3, "こ", "コ"},
	{3, "kya", 6, "きゃ", "キャ"},
	{3, "kyi", 6, "きぃ", "キィ"},
	{3, "kyu", 6, "きゅ", "キュ"},
	{3, "kye", 6, "きぇ", "キェ"},
	{3, "kyo", 6, "きょ", "キョ"},
	{3, "kwa", 6, "くぁ", "クァ"},
	{2, "sa", 3, "さ", "サ"},
	{2, "si", 3, "し", "シ"},
	{2, "su", 3, "す", "ス"},
	{2, "se", 3, "せ", "セ"},
	{2, "so", 3, "そ", "ソ"},
	{3, "shi", 3, "し", "シ"},
	{3, "sya", 6, "しゃ", "シャ"},
	{3, "syi", 6, "しぃ", "シィ"},
	{3, "syu", 6, "しゅ", "シュ"},
	{3, "sye", 6, "しぇ", "シェ"},
	{3, "syo", 6, "しょ", "ショ"},
	{3, "sha", 6, "しゃ", "シャ"},
	{3, "shu", 6, "しゅ", "シュ"},
	{3, "she", 6, "しぇ", "シェ"},
	{3, "sho", 6, "しょ", "ショ"},
	{2, "ta", 3, "た", "タ"},
	{2, "ti", 3, "ち", "チ"},
	{2, "tu", 3, "つ", "ツ"},
	{2, "te", 3, "て", "テ"},
	{2, "to", 3, "と", "ト"},
	{3, "chi", 3, "ち", "チ"},
	{3, "tsu", 3, "つ", "ツ"},
	{3, "ltu", 3, "っ", "ッ"},
	{3, "xtu", 3, "っ", "ッ"},
	{3, "tya", 6, "ちゃ", "チャ"},
	{3, "tyi", 6, "ちぃ", "チィ"},
	{3, "tyu", 6, "ちゅ", "チュ"},
	{3, "tye", 6, "ちぇ", "チェ"},
	{3, "tyo", 6, "ちょ", "チョ"},
	{3, "cya", 6, "ちゃ", "チャ"},
	{3, "cyi", 6, "ちぃ", "チィ"},
	{3, "cyu", 6, "ちゅ", "チュ"},
	{3, "cye", 6, "ちぇ", "チェ"},
	{3, "cyo", 6, "ちょ", "チョ"},
	{3, "cha", 6, "ちゃ", "チャ"},
	{3, "chu", 6, "ちゅ", "チュ"},
	{3, "che", 6, "ちぇ", "チェ"},
	{3, "cho", 6, "ちょ", "チョ"},
	{3, "tsa", 6, "つぁ", "ツァ"},
	{3, "tsi", 6, "つぃ", "ツィ"},
	{3, "tse", 6, "つぇ", "ツェ"},
	{3, "tso", 6, "つぉ", "ツォ"},
	{3, "tha", 6, "てゃ", "テャ"},
	{3, "thi", 6, "てぃ", "ティ"},
	{3, "thu", 6, "てゅ", "テュ"},
	{3, "the", 6, "てぇ", "テェ"},
	{3, "tho", 6, "てょ", "テョ"},
	{3, "twu", 6, "とぅ", "トゥ"},
	{2, "na", 3, "な", "ナ"},
	{2, "ni", 3, "に", "ニ"},
	{2, "nu", 3, "ぬ", "ヌ"},
	{2, "ne", 3, "ね", "ネ"},
	{2, "no", 3, "の", "ノ"},
	{3, "nya", 6, "にゃ", "ニャ"},
	{3, "nyi", 6, "にぃ", "ニィ"},
	{3, "nyu", 6, "にゅ", "ニュ"},
	{3, "nye", 6, "にぇ", "ニェ"},
	{3, "nyo", 6, "にょ", "ニョ"},
	{2, "ha",  3, "は", "ハ"},
	{2, "hi", 3, "ひ", "ヒ"},
	{2, "hu", 3, "ふ", "フ"},
	{2, "he", 3, "へ", "ヘ"},
	{2, "ho", 3, "ほ", "ホ"},
	{2, "fu", 3, "ふ", "フ"},
	{3, "hya", 6, "ひゃ", "ヒャ"},
	{3, "hyi", 6, "ひぃ", "ヒィ"},
	{3, "hyu", 6, "ひゅ", "ヒュ"},
	{3, "hye", 6, "ひぇ", "ヒェ"},
	{3, "hyo", 6, "ひょ", "ヒョ"},
	{2, "fa", 6, "ふぁ", "ファ"},
	{2, "fi", 6, "ふぃ", "フィ"},
	{2, "fe", 6, "ふぇ", "フェ"},
	{2, "fo", 6, "ふぉ", "フォ"},
	{3, "fya", 6, "ふゃ", "フャ"},
	{3, "fyi", 6, "ふぃ", "フィ"},
	{3, "fyu", 6, "ふゅ", "フュ"},
	{3, "fye", 6, "ふぇ", "フェ"},
	{3, "fyo", 6, "ふょ", "フョ"},
	{2, "ma", 3, "ま", "マ"},
	{2, "mi", 3, "み", "ミ"},
	{2, "mu", 3, "む", "ム"},
	{2, "me", 3, "め", "メ"},
	{2, "mo", 3, "も", "モ"},
	{3, "mya", 6, "みゃ", "ミャ"},
	{3, "myi", 6, "みぃ", "ミィ"},
	{3, "myu", 6, "みゅ", "ミュ"},
	{3, "mye", 6, "みぇ", "ミェ"},
	{3, "myo", 6, "みょ", "ミョ"},
	{2, "ya", 3, "や", "ヤ"},
	{2, "yi", 3, "い", "イ"},
	{2, "yu", 3, "ゆ", "ユ"},
	{2, "ye", 6, "いぇ", "イェ"},
	{2, "yo", 3, "よ", "ヨ"},
	{3, "lya", 3, "ゃ", "ャ"},
	{3, "lyi", 3, "ぃ", "ィ"},
	{3, "lyu", 3, "ゅ", "ュ"},
	{3, "lye", 3, "ぇ", "ェ"},
	{3, "lyo", 3, "ょ", "ョ"},
	{3, "xya", 3, "ゃ", "ャ"},
	{3, "xyi", 3, "ぃ", "ィ"},
	{3, "xyu", 3, "ゅ", "ュ"},
	{3, "xye", 3, "ぇ", "ェ"},
	{3, "xyo", 3, "ょ", "ョ"},
	{2, "ra", 3, "ら", "ラ"},
	{2, "ri", 3, "り", "リ"},
	{2, "ru", 3, "る", "ル"},
	{2, "re", 3, "れ", "レ"},
	{2, "ro", 3, "ろ", "ロ"},
	{3, "rya", 6, "りゃ", "リャ"},
	{3, "ryi", 6, "りぃ", "リィ"},
	{3, "ryu", 6, "りゅ", "リュ"},
	{3, "rye", 6, "りぇ", "リェ"},
	{3, "ryo", 6, "りょ", "リョ"},
	{2, "wa", 3, "わ", "ワ"},
	{2, "wi", 6, "うぃ", "ウィ"},
	{2, "wu", 3, "う", "ウ"},
	{2, "we", 6, "うぇ", "ウェ"},
	{2, "wo", 3, "を", "ヲ"},
	{2, "nn", 3, "ん", "ン"},
	{2, "n'", 3, "ん", "ン"},
	{2, "ga", 3, "が", "ガ"},
	{2, "gi", 3, "ぎ", "ギ"},
	{2, "gu", 3, "ぐ", "グ"},
	{2, "ge", 3, "げ", "ゲ"},
	{2, "go", 3, "ご", "ゴ"},
	{3, "gya", 6, "ぎゃ", "ギャ"},
	{3, "gyi", 6, "ぎぃ", "ギィ"},
	{3, "gyu", 6, "ぎゅ", "ギュ"},
	{3, "gye", 6, "ぎぇ", "ギェ"},
	{3, "gyo", 6, "ぎょ", "ギョ"},
	{3, "gwa", 6, "ぐぁ", "グァ"},
	{2, "za", 3, "ざ", "ザ"},
	{2, "zi", 3, "じ", "ジ"},
	{2, "zu", 3, "ず", "ズ"},
	{2, "ze", 3, "ぜ", "ゼ"},
	{2, "zo", 3, "ぞ", "ゾ"},
	{3, "jya", 6, "じゃ", "ジャ"},
	{3, "jyi", 6, "じぃ", "ジィ"},
	{3, "jyu", 6, "じゅ", "ジュ"},
	{3, "jye", 6, "じぇ", "ジェ"},
	{3, "jyo", 6, "じょ", "ジョ"},
	{3, "zya", 6, "じゃ", "ジャ"},
	{3, "zyi", 6, "じぃ", "ジィ"},
	{3, "zyu", 6, "じゅ", "ジュ"},
	{3, "zye", 6, "じぇ", "ジェ"},
	{3, "zyo", 6, "じょ", "ジョ"},
	{2, "ja", 6, "じゃ", "ジャ"},
	{2, "ji", 3, "じ", "ジ"},
	{2, "ju", 6, "じゅ", "ジュ"},
	{2, "je", 6, "じぇ", "ジェ"},
	{2, "jo", 6, "じょ", "ジョ"},
	{2, "da", 3, "だ", "ダ"},
	{2, "di", 3, "ぢ", "ヂ"},
	{2, "du", 3, "づ", "ヅ"},
	{2, "de", 3, "で", "デ"},
	{2, "do", 3, "ど", "ド"},
	{3, "dya", 6, "ぢゃ", "ヂャ"},
	{3, "dyi", 6, "ぢぃ", "ヂィ"},
	{3, "dyu", 6, "ぢゅ", "ヂュ"},
	{3, "dye", 6, "ぢぇ", "ヂェ"},
	{3, "dyo", 6, "ぢょ", "ヂョ"},
	{3, "dha", 6, "でゃ", "デャ"},
	{3, "dhi", 6, "でぃ", "デャ"},
	{3, "dhu", 6, "でゅ", "デュ"},
	{3, "dhe", 6, "でぇ", "デェ"},
	{3, "dho", 6, "でょ", "デョ"},
	{3, "dwu", 6, "どぅ", "ドゥ"},
	{2, "ba", 3, "ば", "バ"},
	{2, "bi", 3, "び", "ビ"},
	{2, "bu", 3, "ぶ", "ブ"},
	{2, "be", 3, "べ", "ベ"},
	{2, "bo", 3, "ぼ", "ボ"},
	{3, "bya", 6, "びゃ", "ビャ"},
	{3, "byi", 6, "びぃ", "ビィ"},
	{3, "byu", 6, "びゅ", "ビュ"},
	{3, "bye", 6, "びぇ", "ビェ"},
	{3, "byo", 6, "びょ", "ビョ"},
	{2, "pa", 3, "ぱ", "パ"},
	{2, "pi", 3, "ぴ", "ピ"},
	{2, "pu", 3, "ぷ", "プ"},
	{2, "pe", 3, "ぺ", "ペ"},
	{2, "po", 3, "ぽ", "ポ"},
	{3, "pya", 6, "ぴゃ", "ピャ"},
	{3, "pyi", 6, "ぴぃ", "ピィ"},
	{3, "pyu", 6, "ぴゅ", "ピュ"},
	{3, "pye", 6, "ぴぇ", "ピェ"},
	{3, "pyo", 6, "ぴょ", "ピョ"},
	{2, "va", 6, "ヴぁ", "ヴァ"},
	{2, "vi", 6, "ヴぃ", "ヴィ"},
	{2, "vu", 3, "ヴ", "ヴ"},
	{2, "ve", 6, "ヴぇ", "ヴェ"},
	{2, "vo", 6, "ヴぉ", "ヴォ"},
	{3, "vya", 6, "ヴゃ", "ヴャ"},
	{3, "vyi", 6, "ヴぃ", "ヴィ"},
	{3, "vyu", 6, "ヴゅ", "ヴュ"},
	{3, "vye", 6, "ヴぇ", "ヴェ"},
	{3, "vyo", 6, "ヴょ", "ヴョ"}
};
const int32 kNumKanaMappings = sizeof(kKanaMappings) / sizeof(kKanaMappings[0]);

const KanaMap kDirectKanaMappings[] = {
	// first row on JIS keyboard
	{1, "1", 3, "ぬ", "ヌ"},	
	{1, "2", 3, "ふ", "フ"},
	{1, "3", 3, "あ", "ア"},
	{1, "#", 3, "ぁ", "ァ"},	// shift-3
	{1, "4", 3, "う", "ウ"},
	{1, "$", 3, "ぅ", "ゥ"},	// shift-4
	{1, "5", 3, "え", "エ"},
	{1, "%", 3, "ぇ", "ェ"},	// shift-5
	{1, "6", 3, "お", "オ"},
	{1, "&", 3, "ぉ", "ォ"},	// shift-6
	{1, "7", 3, "や", "ヤ"},
	{1, "'", 3, "ゃ", "ャ"},	// shift-7
	{1, "8", 3, "ゆ", "ユ"},
	{1, "(", 3, "ゅ", "ュ"},	// shift-8
	{1, "9", 3, "よ", "ヨ"},
	{1, ")", 3, "ょ", "ョ"},	// shift-9
	{1, "0", 3, "わ", "ワ"},
	{1, "~", 3, "を", "ヲ"},	// shift-0
	{1, "-", 3, "ほ", "ホ"},
	{1, "=", 2, "£", "£"},		// shift-'-'
	{1, "^", 3, "へ", "ヘ"},
	{3, "～", 3, "々", "々"},	// shift-^
	{2, "¥", 3, "ー", "ー"},	// macron key (to left of backspace)
	{1, "|", 3, "﹁", "﹁"},	// shift-macron

	// second row 
	{1, "q", 3, "た", "タ"},
	{1, "w", 3, "て", "テ"},
	{1, "e", 3, "い", "イ"},
	{1, "r", 3, "す", "ス"},
	{1, "t", 3, "か", "カ"},
	{1, "y", 3, "ん", "ン"},
	{1, "u", 3, "な", "ナ"},
	{1, "i", 3, "に", "ニ"},
	{1, "o", 3, "ら", "ラ"},
	{1, "p", 3, "せ", "セ"},
	{1, "@", 3, "〝", "〝"},
	{1, "`", 2, "¢", "¢"},		// shift-@
		// missing the '[' key!

	// third row
	{1, "a", 3, "ち", "チ"},
	{1, "s", 3, "と", "ト"},
	{1, "d", 3, "し", "シ"},
	{1, "f", 3, "は", "ハ"},
	{1, "g", 3, "き", "キ"},
	{1, "h", 3, "く", "ク"},
	{1, "j", 3, "ま", "マ"},
	{1, "k", 3, "の", "ノ"},
	{1, "l", 3, "り", "リ"},
	{1, ";", 3, "れ", "レ"},
	{1, ":", 3, "け", "ケ"},
	{1, "]", 3, "む", "ム"},
	
	// fourth row
	{1, "z", 3, "つ", "ツ"},
	{1, "x", 3, "さ", "サ"},
	{1, "c", 3, "そ", "ソ"},
	{1, "v", 3, "ひ", "ヒ"},
	{1, "b", 3, "こ", "コ"},
	{1, "n", 3, "み", "ミ"},
	{1, "m", 3, "も", "モ"},
	{1, ",", 3, "ね", "ネ"},
	{1, "<", 3, "、", "、"},	// shift-,
	{1, ".", 3, "る", "ル"},
	{1, ">", 3, "。", "。"},	// shift-.
	{1, "/", 3, "め", "メ"},	
	{1, "?", 3, "・", "・"},	// shift-/
	{1, "\\", 3, "ろ", "ロ"},	// '\' character	
	{1, "_", 1, "|", "|"}		// shift-\	
};

const int32 kNumDirectKanaMappings = sizeof(kDirectKanaMappings) / sizeof(kDirectKanaMappings[0]);

const KanaMap kRomaMappings[] = {
	{1, "a", 3, "ａ", "Ａ"},
	{1, "b", 3, "ｂ", "Ｂ"},
	{1, "c", 3, "ｃ", "Ｃ"},
	{1, "d", 3, "ｄ", "Ｄ"},
	{1, "e", 3, "ｅ", "Ｅ"},
	{1, "f", 3, "ｆ", "Ｆ"},
	{1, "g", 3, "ｇ", "Ｇ"},
	{1, "h", 3, "ｈ", "Ｈ"},
	{1, "i", 3, "ｉ", "Ｉ"},
	{1, "j", 3, "ｊ", "Ｊ"},
	{1, "k", 3, "ｋ", "Ｋ"},
	{1, "l", 3, "ｌ", "Ｌ"},
	{1, "m", 3, "ｍ", "Ｍ"},
	{1, "n", 3, "ｎ", "Ｎ"},
	{1, "o", 3, "ｏ", "Ｏ"},
	{1, "p", 3, "ｐ", "Ｐ"},
	{1, "q", 3, "ｑ", "Ｑ"},
	{1, "r", 3, "ｒ", "Ｒ"},
	{1, "s", 3, "ｓ", "Ｓ"},
	{1, "t", 3, "ｔ", "Ｔ"},
	{1, "u", 3, "ｕ", "Ｕ"},
	{1, "v", 3, "ｖ", "Ｖ"},
	{1, "w", 3, "ｗ", "Ｗ"},
	{1, "x", 3, "ｘ", "Ｘ"},
	{1, "y", 3, "ｙ", "Ｙ"},
	{1, "z", 3, "ｚ", "Ｚ"}
};
const int32 kNumRomaMappings = sizeof(kRomaMappings) / sizeof(kRomaMappings[0]);

const KanaMapInfo kKanaMappingTables[] = {
	{kCommonMappings, kNumCommonMappings},
	{kKanaMappings, kNumKanaMappings}
};
const int32 kNumKanaMappingTables = sizeof(kKanaMappingTables) / sizeof(kKanaMappingTables[0]);

const KanaMapInfo kRomaMappingTables[] = {
	{kCommonMappings, kNumCommonMappings},
	{kRomaMappings, kNumRomaMappings}
};
const int32 kNumRomaMappingTables = sizeof(kRomaMappingTables) / sizeof(kRomaMappingTables[0]);

const KanaMapInfo kDirectKanaMappingTables[] = {
	{kDirectKanaMappings, kNumDirectKanaMappings},
	{kCommonMappings, kNumCommonMappings} // it is important that the common mappings are last here
};
const int32 kNumDirectKanaMappingTables = sizeof(kDirectKanaMappingTables) / sizeof(kDirectKanaMappingTables[0]);

const HanKataMap kHankakuKatakanas[] = {
	{0x3001, 3, "､"},
	{0x3002, 3, "｡"},
	{0x300C, 3, "｢"},
	{0x300D, 3, "｣"},
	{0x30A1, 3, "ｧ"},
	{0x30A2, 3, "ｱ"},
	{0x30A3, 3, "ｨ"},
	{0x30A4, 3, "ｲ"},
	{0x30A5, 3, "ｩ"},
	{0x30A6, 3, "ｳ"},
	{0x30A7, 3, "ｪ"},
	{0x30A8, 3, "ｴ"},
	{0x30A9, 3, "ｫ"},
	{0x30AA, 3, "ｵ"},
	{0x30AB, 3, "ｶ"},
	{0x30AC, 6, "ｶﾞ"},
	{0x30AD, 3, "ｷ"},
	{0x30AE, 6, "ｷﾞ"},
	{0x30AF, 3, "ｸ"},
	{0x30B0, 6, "ｸﾞ"},
	{0x30B1, 3, "ｹ"},
	{0x30B2, 6, "ｹﾞ"},
	{0x30B3, 3, "ｺ"},
	{0x30B4, 6, "ｺﾞ"},
	{0x30B5, 3, "ｻ"},
	{0x30B6, 6, "ｻﾞ"},
	{0x30B7, 3, "ｼ"},
	{0x30B8, 6, "ｼﾞ"},
	{0x30B9, 3, "ｽ"},
	{0x30BA, 6, "ｽﾞ"},
	{0x30BB, 3, "ｾ"},
	{0x30BC, 6, "ｾﾞ"},
	{0x30BD, 3, "ｿ"},
	{0x30BE, 6, "ｿﾞ"},
	{0x30BF, 3, "ﾀ"},
	{0x30C0, 6, "ﾀﾞ"},
	{0x30C1, 3, "ﾁ"},
	{0x30C2, 6, "ﾁﾞ"},
	{0x30C3, 3, "ｯ"},
	{0x30C4, 3, "ﾂ"},
	{0x30C5, 6, "ﾂﾞ"},
	{0x30C6, 3, "ﾃ"},
	{0x30C7, 6, "ﾃﾞ"},
	{0x30C8, 3, "ﾄ"},
	{0x30C9, 6, "ﾄﾞ"},
	{0x30CA, 3, "ﾅ"},
	{0x30CB, 3, "ﾆ"},
	{0x30CC, 3, "ﾇ"},
	{0x30CD, 3, "ﾈ"},
	{0x30CE, 3, "ﾉ"},
	{0x30CF, 3, "ﾊ"},
	{0x30D0, 6, "ﾊﾞ"},
	{0x30D1, 6, "ﾊﾟ"},
	{0x30D2, 3, "ﾋ"},
	{0x30D3, 6, "ﾋﾞ"},
	{0x30D4, 6, "ﾋﾟ"},
	{0x30D5, 3, "ﾌ"},
	{0x30D6, 6, "ﾌﾞ"},
	{0x30D7, 6, "ﾌﾟ"},
	{0x30D8, 3, "ﾍ"},
	{0x30D9, 6, "ﾍﾞ"},
	{0x30DA, 6, "ﾍﾟ"},
	{0x30DB, 3, "ﾎ"},
	{0x30DC, 6, "ﾎﾞ"},
	{0x30DD, 6, "ﾎﾟ"},
	{0x30DE, 3, "ﾏ"},
	{0x30DF, 3, "ﾐ"},
	{0x30E0, 3, "ﾑ"},
	{0x30E1, 3, "ﾒ"},
	{0x30E2, 3, "ﾓ"},
	{0x30E3, 3, "ｬ"},
	{0x30E4, 3, "ﾔ"},
	{0x30E5, 3, "ｭ"},
	{0x30E6, 3, "ﾕ"},
	{0x30E7, 3, "ｮ"},
	{0x30E8, 3, "ﾖ"},
	{0x30E9, 3, "ﾗ"},
	{0x30EA, 3, "ﾘ"},
	{0x30EB, 3, "ﾙ"},
	{0x30EC, 3, "ﾚ"},
	{0x30ED, 3, "ﾛ"},
	{0x30EF, 3, "ﾜ"},
	{0x30F2, 3, "ｦ"},
	{0x30F3, 3, "ﾝ"},
	{0x30F4, 6, "ｳﾞ"},
	{0x30F7, 6, "ﾜﾞ"},
	{0x30FA, 6, "ｦﾞ"},
	{0x30FB, 3, "･"},
	{0x30FC, 3, "ｰ"}
};
const int32 kNumHankakuKatakanas = sizeof(kHankakuKatakanas) / sizeof(kHankakuKatakanas[0]);

const KutoutenMap kKutoutenTable[] = {
	{"。", "、"},
	{"．", "，"},
	{"。", "，"},
	{"．", "、"}
};

uint32	KanaString::sKutoutenMode = J_DEFAULT_KUTOUTEN_TYPE;
bool	KanaString::sFullSpaceMode = J_DEFAULT_SPACE_TYPE;

struct TransformMap {
	const char*	in;
	const char*	out;
};

struct TransformMapInfo {
	const TransformMap*	table;
	const int32			size;
};

// transforms used with the '[' character
const TransformMap kBracketTransforms[] = {
	{"は", "ぱ"},
	{"ハ", "パ"},
	{"ひ", "ぴ"},
	{"ヒ", "ピ"},
	{"ふ", "ぷ"},
	{"フ", "プ"},
	{"へ", "ぺ"},
	{"ヘ", "ペ"},
	{"ほ", "ぽ"},
	{"ホ", "ポ"}
};
const int32 kNumBracketTransforms = sizeof(kBracketTransforms) / sizeof(kBracketTransforms[0]);

// transforms used with the '@' character
const TransformMap kAtSignTransforms[] = {
	{"か", "が"},
	{"カ", "ガ"},
	{"き", "ぎ"},
	{"キ", "ギ"},
	{"く", "ぐ"},
	{"ク", "グ"},
	{"け", "げ"},
	{"ケ", "ゲ"},
	{"こ", "ご"},
	{"コ", "ゴ"},
	{"さ", "ざ"},
	{"サ", "ザ"},
	{"し", "じ"},
	{"シ", "ジ"},
	{"す", "ず"},
	{"ス", "ズ"},
	{"せ", "ぜ"},
	{"セ", "ゼ"},
	{"そ", "ぞ"},
	{"ソ", "ゾ"},
	{"た", "だ"},
	{"タ", "ダ"},
	{"ち", "ぢ"},
	{"チ", "ヂ"},
	{"つ", "づ"},
	{"ツ", "ヅ"},
	{"て", "で"},
	{"テ", "デ"},
	{"と", "ど"},
	{"ト", "ド"},
	{"は", "ば"},
	{"ハ", "バ"},
	{"ひ", "び"},
	{"ヒ", "ビ"},
	{"ふ", "ぶ"},
	{"フ", "ブ"},
	{"へ", "べ"},
	{"ヘ", "ベ"},
	{"ほ", "ぼ"},
	{"ホ", "ボ"}
};
const int32 kNumAtSignTransforms = sizeof(kAtSignTransforms) / sizeof(kAtSignTransforms[0]);

const TransformMapInfo kTransformMappingTables[] = {
	{kAtSignTransforms, kNumAtSignTransforms},
	{kBracketTransforms, kNumBracketTransforms}
};


KanaString::KanaString()
{
	fStringLen = 0;
	fString = NULL;
	fInsertionPoint = 0;
	fInputMode = HIRA_INPUT;

	Clear();
}


KanaString::~KanaString()
{
	free(fString);
}


void
KanaString::Append(
	const char*	theChar,
	uint32 charLen)
{
	if ((charLen > 1) || (!TransformLastKana(theChar[0]))) {
		fString = (char *)realloc(fString, fStringLen + charLen + 1);
		memmove(fString + fInsertionPoint + charLen, fString + fInsertionPoint, fStringLen - fInsertionPoint);
		memcpy(fString + fInsertionPoint, theChar, charLen);
		fStringLen += charLen;
		fString[fStringLen] = '\0';
		fInsertionPoint += charLen;
	 
	 	switch (fInputMode) {
	 		case HAN_EISUU_INPUT:
				// do nothing
	 			break;
	 		case DIRECT_HIRA_INPUT:
	 		case DIRECT_KATA_INPUT:
	 			DirectKana();
	 			break;
	 		default:
	 			RomaToKana();
	 			break;
	 	}
	}
}


void
KanaString::Backspace()
{
	if (fInsertionPoint < 1)
		return;

	int32 newInsertionPoint = fInsertionPoint;
	for (newInsertionPoint--; !IsInitialUTF8Byte(fString[newInsertionPoint]); newInsertionPoint--)
		;

	memmove(fString + newInsertionPoint, fString + fInsertionPoint, fStringLen - fInsertionPoint);
	fStringLen -= fInsertionPoint - newInsertionPoint;
	fString = (char *)realloc(fString, fStringLen + 1);
	fString[fStringLen] = '\0';
	fInsertionPoint = newInsertionPoint;
}


void
KanaString::Delete()
{
	if (fInsertionPoint >= fStringLen)
		return;

	int32 charLen = UTF8CharLen(fString[fInsertionPoint]);
	int32 charEndOffset = fInsertionPoint + charLen;

	memmove(fString + fInsertionPoint, fString + charEndOffset, fStringLen - charEndOffset);
	fStringLen -= charLen;
	fString = (char *)realloc(fString, fStringLen + 1);
	fString[fStringLen] = '\0';	
}


void
KanaString::Clear()
{
	free(fString);
	fStringLen = 0;
	fString = (char *)calloc(1, sizeof(char));
	fInsertionPoint = 0;
}


int32
KanaString::MoveInsertionPoint(
	bool	left)
{
	if (!left) {
		if (fInsertionPoint >= fStringLen)
			return (fInsertionPoint);
		
		fInsertionPoint += UTF8CharLen(fString[fInsertionPoint]);
	}
	else {
		if (fInsertionPoint < 1)
			return (fInsertionPoint);

		for (fInsertionPoint--; !IsInitialUTF8Byte(fString[fInsertionPoint]); fInsertionPoint--)
			;
	}

	return (fInsertionPoint);
}


int32
KanaString::InsertionPoint() const
{
	return (fInsertionPoint);
}


const char*
KanaString::String(
	bool	confirm) const
{
	if (confirm)
		const_cast<KanaString *>(this)->CheckForN(fStringLen - 1);

	return (fString);
}


int32
KanaString::Length() const
{
	return (fStringLen);
}


void
KanaString::SetMode(
	uint32	mode)
{
	fInputMode = mode;
}


char*
KanaString::ConvertKanaString(
	const char	*kana,
	int32		*kanaLen,
	uint32		type)
{
	char *result = NULL;

	switch (type) {
		case HIRA_TO_KATA:
		case KATA_TO_HIRA:
		{
			result = (char *)malloc(*kanaLen + 1);

			const uint16 kStartLimit = (type == HIRA_TO_KATA) ? kHiraganaStart : kKatakanaStart;
			const uint16 kEndLimit = (type == HIRA_TO_KATA) ? kHiraganaEnd : kKatakanaEnd;
			const uint16 kOffset = (type == HIRA_TO_KATA) ? kZenkakuKatakanaOffset : -kZenkakuKatakanaOffset;
  
			int32 i = 0;
			while (i < *kanaLen) {
				int32	charLen = UTF8CharLen(kana[i]);
				uint16	theChar = UTF8CharToUnicode((uchar *)kana + i);

				if ((theChar > kStartLimit) && (theChar < kEndLimit))
					theChar += kOffset;

				uchar convChar[4] = "";
				UnicodeToUTF8Char(theChar, convChar);	
				memcpy(result + i, convChar, charLen);

				i += charLen;
			}
			break;
		}

		case ZENKATA_TO_HANKATA:
		{
			result = (char *)malloc((*kanaLen * 2) + 1);

			int32 srcLen = *kanaLen;
			*kanaLen = 0;

			int32 i = 0;
			while (i < srcLen) {
				uchar	convChar[4] = "";
				int32	charLen = UTF8CharLen(kana[i]);
				uint16	theChar = UTF8CharToUnicode((uchar *)kana + i);

				for (int32 j = 0; j < kNumHankakuKatakanas; j++) {
					if (theChar == kHankakuKatakanas[j].zen) {
						memcpy(result + *kanaLen, kHankakuKatakanas[j].han, kHankakuKatakanas[j].hanLen);
						*kanaLen += kHankakuKatakanas[j].hanLen;
						goto NextIteration;
					}
				}
				
				UnicodeToUTF8Char(theChar, convChar);
				memcpy(result + i, convChar, charLen);
				*kanaLen += charLen;

NextIteration:
				i += charLen;
			}
			break;
		}

		default:
			*kanaLen = 0;
			break;
	}

	if (result != NULL)
		result[*kanaLen] = '\0';

	return (result);
}


void
KanaString::RomaToKana()
{
	const char	*origRoma = fString + fInsertionPoint;
	int32		origRomaLen = 0;
	bool		isDirect = (fInputMode == DIRECT_KATA_INPUT || fInputMode == DIRECT_HIRA_INPUT);
	
	// we want the non-multibyte characters at the end when in a non-direct mode
	while ((origRoma > fString) && (IsInitialUTF8Byte(*origRoma))) {
		origRoma--;
		origRomaLen++;
	}

	const char	*roma = origRoma;
	int32		romaLen = origRomaLen;
	bool		inKataMode = (fInputMode == ZEN_KATA_INPUT) || (fInputMode == HAN_KATA_INPUT) || (fInputMode == DIRECT_KATA_INPUT);
	bool		twoDeep = false;

	do {
		const KanaMapInfo*	kMappingTables;
		int32			kNumMappingTables;
	
		// choose mapping tables based on input mode
		switch (fInputMode) {
		case ZEN_EISUU_INPUT:
		 	kMappingTables = kRomaMappingTables;
			kNumMappingTables = kNumRomaMappingTables;
			break;
		case DIRECT_KATA_INPUT:
		case DIRECT_HIRA_INPUT:
		 	kMappingTables = kDirectKanaMappingTables;
			kNumMappingTables = kNumDirectKanaMappingTables;;	
			break;
		default:
		 	kMappingTables = kKanaMappingTables;
			kNumMappingTables = kNumKanaMappingTables;;			
			break;
		}

		for (int32 i = 0; i < kNumMappingTables; i++) {
			const KanaMap	*kMappings = kMappingTables[i].table;
			const int32		kNumMappings = kMappingTables[i].size;

			for (int32 i = 0; i < kNumMappings; i++) {
				const char	*mapRoma = kMappings[i].roma;
				bool		isKata = !inKataMode;
				int32		j = 0;
				
				while ((j < romaLen) && (tolower(roma[j]) == mapRoma[j])) {
					twoDeep |= j > 0;

					if (toupper(roma[j]) == tolower(roma[j])) {
						isKata = inKataMode;
					} else if (inKataMode) {
						isKata |= toupper(roma[j]) != roma[j];
					} else {
						isKata &= tolower(roma[j]) != roma[j];
					}
	
					if (j == (romaLen - 1)) {
						if (romaLen != kMappings[i].romaLen)
							break;

						const char	*saveKana = (isKata) ? kMappings[i].kata : kMappings[i].hira;
						int32		kanaLen = kMappings[i].kanaLen;	
						if (romaLen == 1) {
							switch (roma[j]) {
								case ' ':
									saveKana = (sFullSpaceMode) ? "　" : " ";
									kanaLen = (sFullSpaceMode) ? 3 : kanaLen;
									break;
		
								case '.':
									if (!isDirect) {
										saveKana = kKutoutenTable[sKutoutenMode].maru;
									}
									break;

								case ',':
									if (!isDirect) {
										saveKana = kKutoutenTable[sKutoutenMode].ten;
									}
									break;
							}
						}

						const char *kana = saveKana;
						
						if (fInputMode == HAN_KATA_INPUT)
							kana = ConvertKanaString(kana, &kanaLen, ZENKATA_TO_HANKATA);

						int32 convLen = j + 1;
						int32 convStart = (origRoma - fString) + (roma - origRoma);
						int32 convEnd = convStart + convLen;
						int32 lenDelta = kanaLen - convLen;
	 					
						if (lenDelta < 0)
							memmove(fString + convStart + kanaLen, fString + convEnd, fStringLen - convEnd);
						if (lenDelta != 0)
							fString = (char *)realloc(fString, fStringLen + lenDelta + 1);
						if (lenDelta > 0)
							memmove(fString + convStart + kanaLen, fString + convEnd, fStringLen - convEnd);
						memcpy(fString + convStart, kana, kanaLen);
						fStringLen += lenDelta;
						fString[fStringLen] = '\0';
						fInsertionPoint = convStart + kanaLen;
	
						if (romaLen < origRomaLen) {
							if (CheckForN(convStart - 1))
								fInsertionPoint += kanaLen;
						}

						if (kana != saveKana)
							free((char *)kana);	
						return;
					}
		
					j++;
				}
			}
		}

		roma++;
		romaLen--;
	} while (romaLen > 0);

	if (origRomaLen > 1) {
		if ((!twoDeep) && (CheckForN(fInsertionPoint - 2)))
			fInsertionPoint++;
	}
}

#include <Beep.h>

void
KanaString::DirectKana()
{

	// we only want the last inserted character, multi-byte or not when we are
	// in one of the direct modes.
	char *lastChar = fString + fInsertionPoint - 1;
	int32 charLen = 1;
	while ((lastChar > fString) && (!IsInitialUTF8Byte(*lastChar))) {
		charLen++;
		lastChar--;
	}
		
	//_sPrintf("DirectKana() invoked, input = '%s', len = %ld\n", lastChar, charLen);
	//_sPrintf("\tfString='%s', fStringLen=%ld, fInsertionPoint=%ld, \n", fString, fStringLen, fInsertionPoint);

	bool isKata = (fInputMode == DIRECT_KATA_INPUT);
	if (charLen == 1) {
		if (tolower(*lastChar) != *lastChar) { // check for uppercase
			*lastChar = tolower(*lastChar);
			isKata = !isKata;
		}
	}

	const KanaMapInfo* kMappingTables;
	int32 kNumMappingTables;

	kMappingTables = kDirectKanaMappingTables;
	kNumMappingTables = kNumDirectKanaMappingTables;;	

	for (int32 i = 0; i < kNumMappingTables; i++) {
		const KanaMap	*kMappings = kMappingTables[i].table;
		const int32		kNumMappings = kMappingTables[i].size;

		for (int32 i = 0; i < kNumMappings; i++) {
			const char	*mapRoma = kMappings[i].roma;
			int32		j = 0;
			
			if (charLen == kMappings[i].romaLen) {
								
				if (!strncmp(mapRoma, lastChar, charLen)) {
					
					const char	*kana = (isKata) ? kMappings[i].kata : kMappings[i].hira;
					int32		kanaLen = kMappings[i].kanaLen;	
					
					int32 lenDelta = kanaLen - charLen;

					if (lenDelta < 0) {
						// conversion is shorter than input, so move text after insertion point
						// toward the head of the array
						//_sPrintf("\tcompacting: moving %ld bytes from %p to %p\n", fString + fStringLen - (lastChar + charLen),
						//	lastChar + charLen, lastChar + kanaLen);
						memmove(lastChar + kanaLen, lastChar + charLen, fString + fStringLen - (lastChar + charLen));
					}
					if (lenDelta != 0) {
						//_sPrintf("\treallocing fString from length %ld to length %ld\n", fStringLen, fStringLen + lenDelta + 1);
						fString = (char *)realloc(fString, fStringLen + lenDelta + 1);
						lastChar = fString + fInsertionPoint - charLen;
						//_sPrintf("\tfString=%p, lastChar=%p, diff=%ld\n", fString, lastChar, lastChar - fString);
					}
					if (lenDelta > 0) {
						//_sPrintf("\tgrowing: moving %ld bytes from %p to %p\n", fString + fStringLen - (lastChar + charLen),
						//	lastChar + kanaLen, lastChar + charLen);
						memmove(lastChar + kanaLen, lastChar + charLen, fString + fStringLen - (lastChar + charLen));
					}
					memcpy(lastChar, kana, kanaLen);
					fStringLen += lenDelta;
					fString[fStringLen] = '\0';
					fInsertionPoint = (lastChar - fString) + kanaLen;					
					//_sPrintf("\texiting... fString='%s', fStringLen=%ld, fInsertionPoint=%ld, \n", fString, fStringLen, fInsertionPoint);
				}		
			}
		}
	}
}

// Used to transform the previously entered Hiragana or Katakana character to a variation
// of that character.  The user does this by entering a Hiragana or Katakana character, then
// pressing the @ or [ key.
bool
KanaString::TransformLastKana(char theChar)
{
	bool rc = false;
	if ((fInsertionPoint > 0) && (theChar == '[') || (theChar == '@')) {
		const int32 index = (theChar == '@') ? 0 : 1; // choose the @ or [ table
		const TransformMap* map = kTransformMappingTables[index].table;
		int32 mapLen = kTransformMappingTables[index].size;
		
		char *lastChar = fString + fInsertionPoint - 1;
		int32 charLen = 1;
		while ((lastChar > fString) && (!IsInitialUTF8Byte(*lastChar))) {
			charLen++;
			lastChar--;
		}

		for (int32 i = 0; i < mapLen; i++) {
			const char *in = map[i].in;
			// NOTE: this code assumes 3 bytes per character for the in and out strings
			if ((lastChar[0] == in[0]) &&
				(lastChar[1] == in[1]) &&
				(lastChar[2] == in[2]))
			{
				const char *out = map[i].out;
				lastChar[0] = out[0];
				lastChar[1] = out[1];
				lastChar[2] = out[2];
				rc = true;
				break;
			}
				
		}
	}
	return rc;
}

bool
KanaString::CheckForN(
	int32	offset)
{
	uchar nChar = fString[offset];

	if (tolower(nChar) == 'n') {
		bool		inKataMode = (fInputMode == ZEN_KATA_INPUT) || (fInputMode == HAN_KATA_INPUT);
		const char	*saveKana = (tolower(nChar) == nChar) ? ((inKataMode) ? "ン" : "ん") : ((inKataMode) ? "ん" : "ン");
		const char	*kana = saveKana;
		int32		kanaLen = 3;

		if (fInputMode == HAN_KATA_INPUT)
			kana = ConvertKanaString(kana, &kanaLen, ZENKATA_TO_HANKATA);

		int32 convLen = 1;
		int32 convStart = offset;
		int32 convEnd = convStart + convLen;
		int32 lenDelta = kanaLen - convLen;

		fString = (char *)realloc(fString, fStringLen + lenDelta + 1);
		memmove(fString + convStart + kanaLen, fString + convEnd, fStringLen - convEnd);
		memcpy(fString + convStart, kana, kanaLen);
		fStringLen += lenDelta;
		fString[fStringLen] = '\0';
		fInsertionPoint = convStart + kanaLen;

		if (kana != saveKana)
			free((char *)kana);

		return (true);
	}

	return (false);
}
