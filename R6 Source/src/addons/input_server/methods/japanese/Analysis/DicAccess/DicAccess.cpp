/************************************************************************
ファイル名	:	DicAccess.h

作成日	:	97.12.21
************************************************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "DicAccess.h"
#include "AccessTable.h"
#include "DicCtrl.h"
#include "NumberHyoki.h"

DicAccess::DicAccess(void)
{
}

DicAccess::~DicAccess(void)
{
}

/***************************************************************
関数名	:	DicAccess::MakeAccessTable
機能		:	解析文字列に対するアクセステーブルを作成する
入力		:	uchar	*analyzedText		: 解析文字列
			uchar*	textWord			: 解析文字列の各文字の長さ
			uint16	textWordNum		: textWordの要素数
			uint16	textWordOffset	: 前回の解析文字列に追加された部分の位置
出力		:	status_t
作成日	:	97.12.18
修正日	:	97.12.31
***************************************************************/
status_t DicAccess::MakeAccessTable(const uchar *analyzedText,const uchar* textWord,uint16 textWordNum,uint16 textWordOffset)
{
	status_t			err;
	uint32			num;
	uint16			analyzedLength,offset;
	MainRecord		*rec;
	uint16			k;

	analyzedLength = strlen((char *)analyzedText);

	offset = 0;
	for(k=0;k<textWordOffset;k++)offset += textWord[k];
	if(offset < at.oldTextLen)at.Free(offset,kMaxATSize-1);

	at.oldTextLen = analyzedLength;
	for(;k<textWordNum;k++){
		offset += textWord[k];
		if(offset > analyzedLength)return B_NO_ERROR;
	if(offset == 45)	//	debug
		err = B_NO_ERROR;
		err = eDicCtrl.TrieSearch(analyzedText,offset,rec,&num);
		if(err != B_NO_ERROR)return err;

		if(num == 0){
			/* 品詞"名無し"を積み込む */
			char	hyoki[kMaxHyokiSize+1];

			strncpy(hyoki,(char*)analyzedText+offset-textWord[k],textWord[k]);
			hyoki[textWord[k]] = '\0';
			at.SetNanashi(offset-textWord[k],textWord[k],(unsigned char *)hyoki);
		}
		else{
			HinshiNumType		hinNum;
			uchar			*pos;
			DicData			dicData;
			Category			hinshi;
			HyokiNumType		hyokiNum;
			HindoType			hindo;
			WordFlagType		wordFlag;
			HyokiSizeType		hyokiSize;
			uchar*			hyoki;
			char				buf[kMaxHyokiSize+1];
			uint16			size;

			for(uint32 m = 0; m < num; m++ ){
				pos = rec[m].recordData;
				size = rec[m].keyLength;

				hinNum = dicData.GetHinshiNum(&pos);
				for(HinshiNumType i=0;i<hinNum;i++){
					hinshi = dicData.GetHinshiCode(pos);
					pos += sizeof(Category);	
					hyokiNum = dicData.GetHyokiNum(pos);
					pos += sizeof(HyokiNumType);
			
					for(HyokiNumType j=0;j<hyokiNum;j++){
						dicData.GetHyoki(&pos,&hindo,&wordFlag,&hyokiSize,&hyoki);
						/* 検索結果の積み込み */
						memcpy(buf,hyoki,hyokiSize);
						buf[hyokiSize] = '\0';
						
						if(POST_HINSHI(hinshi) == suushi && PRE_HINSHI(hinshi) == suushi){
							/* 前も後ろも数詞の時は、数詞をつなげる。 */							
							SetSuusi(analyzedText,offset-size,size);
						}
						at.SetWord(offset-size,size,(unsigned char *)buf,hinshi,hindo);
					}
				}
			}
		}
	}
	
	return B_NO_ERROR;
}

/***************************************************************
関数名	:	DicAccess::LearnFrequency
機能		:	頻度学習を行う
入力		:	uchar* 	yomi		: 読み
			Hinshi*	hinshi	: 品詞
			uint16	num		: 学習を行う表記の位置(１から始まる)
出力		:	status_t
作成日	:	97.12.30
修正日	:	98.1.2
***************************************************************/
status_t DicAccess::LearnFrequency(const uchar* yomi,Hinshi* hinshi,uint16 num)
{
	Field	*tmpField;
	uint16	i;
	HindoType hindo;
	
	if(num <= 1 || hinshi->firstField == NULL){
		/* 引数がおかしいので学習しない */
		return B_NO_ERROR;
	}

	/* この部分は後から変更する */					//	ここから
	/* 頻度の値の決定 */
	if(hinshi->firstField->evaluation == kMaxHindo)
		hindo = kMaxHindo;
	else
		hindo = hinshi->firstField->evaluation+1;		//	ここまで

	/* 学習を行う表記を探す */
	tmpField = hinshi->firstField;
	for(i=1;i<num;i++){
		tmpField = tmpField->nextField;
		if(tmpField == NULL){
			/* 引数がおかしいので学習しない */
			return B_NO_ERROR;
		}
	}

	return eDicCtrl.Append(yomi, hinshi->length,(uchar*)tmpField->contents, hindo, hinshi->category);
}

/***************************************************************
関数名	:	AccessTable::SetSuusi
機能		:	数詞をつなげてアクセステーブルに追加する
入力		:	int			pos		: 文字列の始まる位置
			int			length	: contentsの長さ
			unsigned char*	contents	: 追加する文字列に対する表記
			Category		hinshiCode	: 追加する単語の品詞
出力		:	なし
作成日	:	98.1.21
修正日	:	98.2.16
***************************************************************/
void DicAccess::SetSuusi(const uchar *analyzedText,int pos,int length)
{
	short		offset;					//解析文字列に対する検索文字列のオフセット
	int			i, ret;					//ループ用変数及び返値用変数
	
	for(i = 0; i < NumHyokiMax; i++){
		nh.numberHyoki[i].hyoki[0] = '\0';			//読み部分の初期化
		nh.numberHyoki[i].hyokiLength = 0;			//変更部分のバイト数の初期化
	}

	//品詞が数詞の際にのみ数字列を纏める作業を行う

	//その後，数字纏め上げルーチン処理を行う
	ret = nh.GetYomiNum(at,analyzedText, pos+length, length);
	//数字を纏めて，読みに変換した文字列がかえってきた場合
	for(i = 0; i < ret && i < NumHyokiMax; i++){
		if(nh.numberHyoki[i].hyoki[0] == '\0' || nh.numberHyoki[i].hyokiLength == 0)continue;
		offset = pos+length - nh.numberHyoki[i].hyokiLength;
		at.SetWord(offset,nh.numberHyoki[i].hyokiLength,(uchar*)nh.numberHyoki[i].hyoki,HINSHI(suushi,suushi),ret-i-1);
	}
}

/***************************************************************
関数名	:	DicAccess::LearnFrequency
機能		:	頻度学習を行う
入力		:	uchar* 	yomi		: 読み
			Category	category	: 品詞
			Field*	field	: 表記
			HindoType	maxHindo	: 同音異義語中で現在使われている頻度の最大値
出力		:	status_t
作成日	:	98.2.10
修正日	:	98.2.18
***************************************************************/
status_t DicAccess::LearnFrequency(const uchar* yomi,Category category,Field* field,HindoType maxHindo)
{
	return LearnFrequency(yomi, strlen((char*)yomi),category, field, maxHindo);
}

/***************************************************************
関数名	:	DicAccess::LearnFrequency
機能		:	頻度学習を行う
入力		:	uchar* 	yomi		: 読み
			Category	category	: 品詞
			Field*	field	: 表記
			HindoType	maxHindo	: 同音異義語中で現在使われている頻度の最大値
出力		:	status_t
作成日	:	98.2.18
***************************************************************/
status_t DicAccess::LearnFrequency(const uchar* yomi,short yomiLen,Category category,Field* field,HindoType maxHindo)
{
	HindoType hindo;

	/* 頻度の値の決定 */
	if(maxHindo == kMaxHindo)
		hindo = kMaxHindo;
	else
		hindo = maxHindo+1;

	return eDicCtrl.Append(yomi, yomiLen,(uchar*)field->contents, hindo, category);
}
