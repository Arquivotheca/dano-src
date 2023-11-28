/******************************************
ファイル名	:	NumberHyoki.h

作成日	:	98.1.21
******************************************/
#ifndef __NUMBER_HYOKI_H__
#define __NUMBER_HYOKI_H__

#include"DicData.h"
#include"AccessTable.h"

#define strSize		(256 + 1)	
#define NumHyokiMax	40		//NumberYomiの最大個数

#define kLengthZenkaku	3
#define	MaxNumLength			5*kLengthZenkaku		//数字の読みの最大長
#define kHatsuSize		14
#define kYomiRuleSize	10							//ルールの読みの個数
#define kLengthKanji		17*kLengthZenkaku			// ．〇零一壱二弐三参四五六七八九の長さ
#define kLengthNum		10*kLengthZenkaku				// ０１２３４５６７８９の長さ
#define kLengthSpecialJosu		2*kLengthZenkaku		//特殊な接頭助数詞の長さ

struct NumYomiHeader {
	char 	kanjiNum[MaxNumLength] ;	//普通の漢字	三百十二
} ;

struct NumberHeader {
	char 	yomi[MaxNumLength] ;	//数字の読み	さん
	short	yomiLength ;			//読みの長さ	4
	short	oldNum ;				//旧の漢字フラグ
	NumYomiHeader	numHyouki ;		//数字の表記群
} ;

struct jyosuCodeHeader{
    unsigned char	disk_mask;		// マスク
	short	rule1Num[2];			//[0]hatsuのルールの個数,[1]]hatsuの長さ（１ルールあたり）
	short	rule2Num[2];			//[0]yomiのルールの個数,[1]yomiの長さ（１ルールあたり）
	short	rule3Num[2];			//[0]濁点ルールの個数,[1]濁点ルールの長さ
	short	rule4Num[3];			//[0]中間助数詞のルールの長さ,[1]０のルールの長さ,[2]全角の中間助数詞の長さ
	short	rule5Num[2];			//[0]数詞検索列の長さ,[1]rule4Num[0]+rule5Num[0]
	short	rule6Num[2];			//[0]その他の検索列の長さ,[1]rule5Num[1]+rule6Num[0]
	short	rule7Num[2];			//[0]その他の読みの長さ,[1]rule6Num[1]+rule7Num[0]
	short	rule8Num[3];			//[0]半角フィルタ群の長さ,[1]rule7Num[1]+rule8Num[0],[2]半角の長さ
};

class NumberHyoki
{
public:
	NumberHyoki();

	struct {
							//助数詞・数詞の読みの出力用の構造体
		char hyoki[kMaxHyokiSize] ;	//読み
		short hyokiLength ;			//変更部分のバイト数
		short hinshiFlag ;			// 数詞の読み方による品詞変更を示すフラグ
								// 数詞のみの読みを合成した場合はhinshiFlag = 0       ---> 品詞は数詞にする
								// 数詞と助数詞の読みを合成した場合はhinshiFlag = 1 ---> 品詞は普通名詞にする
								// 助数詞を含む数詞変換後の品詞を
								// 普通名詞にすることによる変更
	} numberHyoki[NumHyokiMax];

	int GetYomiNum(AccessTable& at,const uchar *input, short analyzeStrLength, short searchStrLength);

private:
	jyosuCodeHeader	fHeader;

	int CheckHankaku( char *analyzeStr , short  offset );
	int ChangeNumToNum(char *str , int length , int endLength, short flag , short *errorPoint) ;	//半角から全角へのフィルタ（数字のみ）
	int GetNumLength(AccessTable& at , short analyzeStrLength , short *times, int length[] , int cutOverFlowLength) ;
	void SetNumsOut( int returnNum , int returnNumBak , int length , int lengthHankaku) ;
	int NumsToNum (char *check ) ;				//全角の数字をint型に	
	int CheckNumToHyoukiSize(int numLength) ;	
	void SetYomiNum(NumberHeader *data) ;			//数詞表記列のルールの設定
	int GetNumYomi(char *input, short analyzeStrLength ,
				 NumberHeader *header , int *returnNum , int returnNumBak , int turn , int length[] , int lengthHankaku) ;	
	int NumToKnum( char *str, char *out , NumberHeader	*numHeader , short oldNumFlag)  ;				//全角の数字に対して漢数字を返す	
	int NumsSortCheck( int nowNumber , int bakNumber)  ;			//数詞と数詞の整合性を検査する	
	int KnumtoNum(char *input , char *out ) ;				//漢数字（小数点を含む）を全角の数字に変換する				
	int InitYomiDigit(char *code , char yomi[kHatsuSize+1][kYomiRuleSize] ) ;			//読みのルールの初期設定（基本）
	void InitNumDigit(char numDigit[ ( kLengthKanji + kLengthNum + kLengthSpecialJosu ) / kLengthZenkaku + 1 ][ kLengthZenkaku + 1 ]);		//numDigitを設定する
	void FlagClear( int *flag , int *flag2 , int *flag3 , int *flag4 ) ;				//フラグの初期化
	int NextCheck(char *kanjiNumber , int upNumData) ;				//「万」以上の順序をcheckし、KnumtoNumに必要な値を返す
	void GetPointKnum(char *str , char *point ) ;			//小数点以下の漢字を全角の数字に直す
	int CheckPointKonma(char *input ) ;		//２byteのコンマチェック
	int CheckSuushi( AccessTable& at , int loop , int start , int cutOverFlowLength ) ;				//数詞を探す
};

#define MaxNumYomi			32						//数字用ルールの最大個数
#define kInputBakLength		strSize/kLengthZenkaku		//	入力に対する解析可能文字列（それ以上は無視する）
#define kMaxNumKeta		16						//桁数	
#define kLengthNumYomi		27						//「０１２３４５６７８９」までの読みの個数
#define kSpecialJosuNum		2						//特殊な接頭助数詞の数	
#define kMyriadNumDef		24						//万以上のルール
#define kKonmaIchi			37*kLengthZenkaku			//rule5Numにおける，の位置	
#define kDataThousand		3						// 千の係数値
#define kLengthBKanji		4*kLengthZenkaku			// 十拾百千の長さ
#define kYomiSize   		121						//78
#define kLengthJosuElseAdd	7*kLengthZenkaku			//．，　っひびぴの長さ
#define kLengthHankakuNum	20						//変換できる半角の個数	
#define kPointIchi			0						//rule5Numにおける．の位置
#define kNumHenkanLevel		5						//変換レベル
/*kNumHenkanLevelについて*/
//入力		１２３（全角or半角）						読み（ひゃくにじゅうさん）	
//1			全角数字									漢数字の変換
//2			漢数字の変換								旧漢数字の変換							
//3			１２３→一二三の変換						１２３→一二三の変換
//4			未使用
//5			旧漢数字の変換							全角数字

#endif	// __NUMBER_HYOKI_H__
