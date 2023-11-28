/******************************************
ファイル名	:	DicData.c

作成日	:	97.12.17
******************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "DicTypes.h"
#include "DicData.h"
#include <string.h>

/*
	Word = 表記サイズ + 頻度 + フラグ + 表記
	Hinshi = 品詞コード + 表記数 + (Word × 表記数)
	Data = 品詞数 + (Hinshi × 品詞数)
*/

/***************************************************************
関数名	:	DicData::AddData
機能		:	辞書データに単語を追加
入力		:	Byte			*data	: データ開始位置
			uint16		*dataSize	: データサイズ
			uchar		*hyoki	: 表記
			Category		hinshi	: 品詞
			HindoType		hindo	: 頻度
			WordFlagType	wordFlag	: 表記に対するフラグ
出力		:	status_t
作成日	:	97.12.17
修正日	:	97.12.18
***************************************************************/
status_t DicData::AddData(Byte *data,uint16 *dataSize,const uchar *hyoki,Category hinshi,HindoType hindo,WordFlagType wordFlag)
{
	Byte		*pos;

	if(strlen((char *)hyoki) > kMaxHyokiSize)return kMADBadRecordSizeErr;
	pos = data;
	if((*dataSize) == 0){
		/* 新たにデータを作成する */
		/* 品詞数のセット */
		SetHinshiNum(data,1);
		pos += sizeof(HinshiNumType);

		/* 品詞コードと表記数のセット */
		SetHinshiCode(&pos,hinshi,1);
		
		(*dataSize) = kDataHeaderSize + kHinshiHeaderSize + AllHyokiDataSize(strlen((char *)hyoki));
	}
	else{
		HinshiNumType hinNum;
		uint16		size;
		uint16		addSize;
		Byte			*tmpPos;

		addSize = AllHyokiDataSize(strlen((char *)hyoki));
		hinNum = GetHinshiNum(&pos);
		if(SearchHinshi(&pos,hinNum,hinshi,&size) == true){
			/* 品詞が既に存在する */
			HyokiNumType	hyokiNum,i;
			HindoType		oldHindo;
			WordFlagType	oldWordFlag;
			HyokiSizeType	oldHyokiSize;
			uchar		*oldHyoki;

			tmpPos = pos+sizeof(Category);	//	表記数の位置をセット
			hyokiNum = GetHyokiNum(tmpPos);
			pos += kHinshiHeaderSize;

			/* 同じ表記のものが既に登録されているか調べる */
			for(i=0;i<hyokiNum;i++){
				tmpPos = pos;
				GetHyoki(&pos,&oldHindo,&oldWordFlag,&oldHyokiSize,&oldHyoki);
				if(oldHyokiSize == strlen((char *)hyoki) && memcmp(oldHyoki,hyoki,oldHyokiSize)==0)break;
			}
			if(i != hyokiNum){
				/* 既に表記が存在する */
				bool			reWrite = false;

				pos = tmpPos;
				/* 頻度が大きいときには多きい方を書き込む */
				if(oldHindo < hindo)reWrite = true;
				if(reWrite == false)return 0;
			}
			else{
				tmpPos = data+size+sizeof(Category);
				size += kHinshiHeaderSize;
				pos = data+size;
				/* サイズのチェック */
				if((*dataSize) + addSize > kDataMax)return kMADBadRecordSizeErr;
				
				if(hyokiNum == kMaxHyokiNum)return kMADBadRecordSizeErr;

				/* 新しいデータのための領域を空ける */
				memmove(pos+addSize,pos,(*dataSize)-size);
				/* この品詞の表記数を１増やす */
				SetHyokiNum(&tmpPos,hyokiNum+1);
			}
		}
		else{
			/* まだ品詞が存在しない */
			addSize += kHinshiHeaderSize;
			/* サイズのチェック */
			if((*dataSize) + addSize > kDataMax)return kMADBadRecordSizeErr;
			
			/* 新しいデータの品詞コードと表記数のセット */
			SetHinshiCode(&pos,hinshi,1);
			/* このデータの品詞数を１増やす */
			SetHinshiNum(data,hinNum+1);
		}
		(*dataSize) += addSize;
	}
	/* 表記のセット */
	SetHyoki(pos,hindo,wordFlag,strlen((char *)hyoki),hyoki);

	return 0;
}

/***************************************************************
関数名	:	DicData::SetHinshiNum
機能		:	品詞数をセットする
入力		:	Byte			*pos	: データを追加する位置
			HinshiNumType	hinNum	: 品詞数
出力		:	なし
作成日	:	97.12.17
***************************************************************/
void DicData::SetHinshiNum(Byte *pos,HinshiNumType hinNum)
{
	short	tmp;										//	add for Intel
	
	tmp = B_HOST_TO_LENDIAN_INT16(hinNum);				//	add for Intel
	memmove(pos,&tmp,sizeof(HinshiNumType));				//	change for Intel
//	memmove(pos,&hinNum,sizeof(HinshiNumType));
}

/***************************************************************
関数名	:	DicData::GetHinshiNum
機能		:	品詞数を得る
入力		:	Byte			*pos	: 品詞数の格納されているデータ位置
出力		:	品詞数
作成日	:	97.12.17
***************************************************************/
HinshiNumType DicData::GetHinshiNum(Byte **pos)
{
	HinshiNumType	hinNum;

	memmove(&hinNum,*pos,sizeof(HinshiNumType));
	(*pos) += sizeof(HinshiNumType);

	return B_LENDIAN_TO_HOST_INT16(hinNum);				//change for Intel
//	return hinNum;
}

/***************************************************************
関数名	:	DicData::SetHinshiCode
機能		:	品詞コードをセットする
入力		:	Byte			**pos	: データを追加する位置(in)
								: データを追加した後の位置(out)
			Category		hinshi	: 品詞
			HyokiNumType	hyokiNum	: この品詞に対する表記数
出力		:	なし
作成日	:	97.12.17
***************************************************************/
void DicData::SetHinshiCode(Byte **pos,Category hinshi,HyokiNumType hyokiNum)
{
	Category	tmp;										//add for Intel
	
	tmp = B_HOST_TO_LENDIAN_INT32(hinshi);				//add for Intel
	memmove(*pos,&tmp,sizeof(Category));					//change for Intel
//	memmove(*pos,&hinshi,sizeof(Category));
	(*pos) += sizeof(Category);
	SetHyokiNum(pos,hyokiNum);
}

/***************************************************************
関数名	:	DicData:: GetHinshiCode
機能		:	品詞コードを得る
入力		:	Byte			*pos	: 品詞の格納されているデータ位置
			HyokiNumType	hyokiNum	: この品詞に対する表記数
出力		:	品詞コード
作成日	:	97.12.17
***************************************************************/
Category DicData::GetHinshiCode(Byte *pos)
{
	Category	hinshi;

	memmove(&hinshi,pos,sizeof(Category));
	
	return B_LENDIAN_TO_HOST_INT32(hinshi);				//change for Intel
//	return hinshi;
}

/***************************************************************
関数名	:	DicData:: GetHyokiNum
機能		:	表記数を得る
入力		:	Byte			*pos		: 品詞の格納されているデータ位置
			HyokiNumType	hyokiNum	: この品詞に対する表記数
出力		:	表記数
作成日	:	97.12.17
***************************************************************/
HyokiNumType DicData::GetHyokiNum(Byte *pos)
{
	HyokiNumType	hyokiNum;

	memmove(&hyokiNum,pos,sizeof(HyokiNumType));
//	(*pos) += sizeof(HyokiNumType);
	
	return hyokiNum;
}

/***************************************************************
関数名	:	DicData::SetHyokiNum
機能		:	表記数をセットする
入力		:	Byte			**pos	: データを追加する位置(in)
入力		:						: データを追加した後の位置(out)
			Category		hinshi	: 品詞
			HyokiNumType	hyokiNum	: この品詞に対する表記数
出力		:	なし
作成日	:	97.12.17
***************************************************************/
void DicData::SetHyokiNum(Byte **pos,HyokiNumType hyokiNum)
{
	memmove(*pos,&hyokiNum,sizeof(HyokiNumType));
	(*pos) += sizeof(HyokiNumType);
}

/***************************************************************
関数名	:	DicData::SetHyoki
機能		:	表記をセットする
入力		:	Byte			*pos		: データを追加する位置
			HindoType		hindo	: 頻度
			WordFlagType	wordFlag	: この表記に対するフラグ
			HyokiSizeType	hyokiSize	: この表記のサイズ
			uchar		*hyoki	: 表記
出力		:	なし
作成日	:	97.12.17
***************************************************************/
void DicData::SetHyoki(Byte *pos,HindoType hindo,WordFlagType wordFlag,HyokiSizeType hyokiSize,const uchar *hyoki)
{
	/* 表記のサイズのセット */
	memmove(pos,&hyokiSize,sizeof(HyokiSizeType));
	pos += sizeof(HyokiSizeType);

	/* 頻度のセット */
	hindo = B_HOST_TO_LENDIAN_INT16(hindo);						//add for Intel
	memmove(pos,&hindo,sizeof(HindoType));
	pos += sizeof(HindoType);
	
	/* フラグのセット */
	memmove(pos,&wordFlag,sizeof(WordFlagType));
	pos += sizeof(WordFlagType);

	/* 表記のセット */
	memmove(pos,hyoki,hyokiSize);
}

/***************************************************************
関数名	:	DicData::GetHyokiSize
機能		:	表記のサイズを得る
入力		:	Byte			*pos		: データ開始位置
出力		:	表記のサイズ
作成日	:	97.12.17
***************************************************************/
HyokiSizeType DicData::GetHyokiSize(Byte *pos)
{
	HyokiSizeType	hyokiSize;
	/* 表記のサイズのセット */
	memmove(&hyokiSize,pos,sizeof(HyokiSizeType));

	return hyokiSize;
}

/***************************************************************
関数名	:	DicData::SearchHinshi
機能		:	データ中の品詞 hinshi を探す
入力		:	Byte			**pos	: 検索か開始するデータの位置
								: 見つかった時そのデータ位置
			HinshiNumType	hinNum	: データ中の品詞数
			Category		hinshi	: 検索する品詞
			uint16		*size	: 見つかった位置までのサイズ
出力		:	見つかった時 true
作成日	:	97.12.17
***************************************************************/
bool DicData::SearchHinshi(Byte **pos,HinshiNumType hinNum,Category hinshi,uint16 *size)
{
	HinshiNumType	i;
	Category		currentCategory;

	(*size) = kDataHeaderSize;
	for(i=0;i<hinNum;i++){
		currentCategory = GetHinshiCode(*pos);
		if(currentCategory == hinshi){
			/* 品詞が存在した */
			return true;
		}
		(*size) += SkipHinshi(pos);
	}
	/* 品詞が存在しない */
	return false;
}

/***************************************************************
関数名	:	DicData::SkipHinshi
機能		:	１つの品詞分データをスキップする
入力		:	Byte			**pos	: データの開始位置
出力		:	スキップしたデータ
作成日	:	97.12.17
***************************************************************/
uint16 DicData::SkipHinshi(Byte **pos)
{
	uint16		size;
	HyokiNumType	hyokiNum,i;

	(*pos) += sizeof(Category);
	size = kHinshiHeaderSize;
	
	hyokiNum = GetHyokiNum(*pos);
	(*pos) += sizeof(HyokiNumType);

	for(i=0;i<hyokiNum;i++){
		size += SkipHyoki(pos);
	}
	
	return size;
}

/***************************************************************
関数名	:	DicData::SkipHyoki
機能		:	１つの表記分データをスキップする
入力		:	Byte			**pos	: データの開始位置
出力		:	スキップしたデータ
作成日	:	97.12.17
***************************************************************/
uint16 DicData::SkipHyoki(Byte **pos)
{
	HyokiSizeType	hyokiSize;
	uint16		size;
	
	hyokiSize = GetHyokiSize(*pos);
	size = AllHyokiDataSize(hyokiSize);
	(*pos) += size;
	
	return size;
}

/***************************************************************
関数名	:	DicData::GetHyoki
機能		:	１つの表記分データを得る
入力		:	Byte			**pos		: データの開始位置
			uchar		*hindo		: 頻度(out)
			WordFlagType	*wordFlag	: フラグ(out)
			HyokiSizeType	*hyokiSize	: 表記サイズ(out)
			uchar		*hyoki		: 表記を指すポインター(out)
出力		:	なし
作成日	:	97.12.17
***************************************************************/
void DicData::GetHyoki(Byte **pos,HindoType *hindo,WordFlagType *wordFlag,HyokiSizeType *hyokiSize,uchar **hyoki)
{
	/* 表記サイズを得る */
	(*hyokiSize) = GetHyokiSize(*pos);
	(*pos) += sizeof(HyokiSizeType);
	
	/* 頻度を得る */
	memmove(hindo,(*pos),sizeof(HindoType));
	(*hindo) = B_LENDIAN_TO_HOST_INT16(*hindo);			//add for Intel
	(*pos) += sizeof(HindoType);

	/* フラグを得る */
	memmove(wordFlag,(*pos),sizeof(WordFlagType));
	(*pos) += sizeof(WordFlagType);

	/* 表記のポインターを得る */
	(*hyoki) = (*pos);
	(*pos) += (*hyokiSize);
}

/***************************************************************
関数名	:	DicData::DeleteData
機能		:	辞書データから単語を削除
入力		:	Byte			*data	: データ開始位置
			uint16		*dataSize	: データサイズ
			uchar		*hyoki	: 表記
			Category		hinshi	: 品詞
出力		:	status_t
作成日	:	97.12.18
***************************************************************/
status_t DicData::DeleteData(Byte *data,uint16 *dataSize,const uchar *hyoki,Category hinshi)
{
	Byte			*pos;
	HinshiNumType hinNum;
	uint16		size;

	if(strlen((char *)hyoki) > kMaxHyokiSize)return B_NO_ERROR;
	pos = data;
	if((*dataSize) == 0)return B_NO_ERROR;
	
	hinNum = GetHinshiNum(&pos);
	if(SearchHinshi(&pos,hinNum,hinshi,&size) == true){
		Byte			*tmpPos,*hyokiPos;
		HyokiNumType	hyokiNum,i;
		HindoType		oldHindo;
		WordFlagType	oldWordFlag;
		HyokiSizeType	oldHyokiSize;
		uchar		*oldHyoki;
		uint16		tmpSize;

		hyokiPos = pos+sizeof(Category);			//	表記数の位置をセット
		hyokiNum = GetHyokiNum(hyokiPos);
		pos += kHinshiHeaderSize;
		
		tmpSize = 0;
		/* 表記が登録されているか調べる */
		for(i=0;i<hyokiNum;i++){
			tmpPos = pos;
			GetHyoki(&pos,&oldHindo,&oldWordFlag,&oldHyokiSize,&oldHyoki);
			if(oldHyokiSize == strlen((char *)hyoki) && memcmp(oldHyoki,hyoki,oldHyokiSize)==0)break;
			tmpSize += AllHyokiDataSize(oldHyokiSize);
		}
		if(i != hyokiNum){
			/* 登録されている */
			uint16 delSize;
			
			if(hyokiNum == 1){
				if(hinNum == 1){
					/* この単語に対する表記がなくなる */
					(*dataSize) = 0;
				}
				else{
					/* この品詞に対する表記がなくなるので品詞を削除する */
					pos = data+size;
					tmpPos = pos;
					delSize = SkipHinshi(&tmpPos);
					
					memmove(pos,tmpPos,(*dataSize)-size-delSize);
					(*dataSize) -= delSize;
					
					/* 品詞数を１減らす */
					SetHinshiNum(data,hinNum-1);
				}
			}
			else{
				/* この表記のみを削除する */
				delSize = AllHyokiDataSize(strlen((char *)hyoki));
				size += kHinshiHeaderSize + tmpSize;
				
				memmove(tmpPos,pos,(*dataSize)-size-delSize);
				(*dataSize) -= delSize;

				/* この品詞の表記数を１減らす */
				SetHyokiNum(&hyokiPos,hyokiNum-1);
			}
		}
		else{
			/* 表記が存在しない */
			return B_NO_ERROR;
		}
	}
	else{
		/* 品詞が存在しない */
		return B_NO_ERROR;
	}

	return B_NO_ERROR;
}

/***************************************************************
関数名	:	DicData::GetWordNum
機能		:	単語数を得る
入力		:	Byte	*data	: データ開始位置
出力		:	単語数
作成日	:	97.12.23
***************************************************************/
uint32 DicData::GetWordNum(Byte *data)
{
	HinshiNumType	hinNum,i;
	Byte			*pos;
	uint32		count = 0;
	HyokiNumType	hyokiNum,j;
	
	pos = data;

	hinNum = GetHinshiNum(&pos);
	for(i=0;i<hinNum;i++){
		pos += sizeof(Category);	
		hyokiNum = GetHyokiNum(pos);
		pos += sizeof(HyokiNumType);

		for(j=0;j<hyokiNum;j++){
			SkipHyoki(&pos);
		}
		count += hyokiNum;
	}

	return count;
}
