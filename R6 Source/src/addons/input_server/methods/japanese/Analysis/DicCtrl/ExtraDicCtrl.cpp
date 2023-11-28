/* ExtraDicCtrl.cpp
 *
 *	かな漢字変換辞書検索クラス
 *
 *
 *		作成日: 97/12/25
 */
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "ExtraDicCtrl.h"
#include <string.h>
#include <new.h>

#include"DoubleArray.h"

/***************************************************************
関数名	:	KExtraDicCtrl::TrieSearch
機能		:	後方部分一致文字列検索を行う
入力		:	uchar*		yomi		: 読み
			uint16		offset	: 読みサイズ
			MainRecord*	&rec		: 検索結果
			uint32		*num		: 検索結果の個数(out)
出力		:	status_t
作成日	:	97.12.25
修正日	:	97.12.31
***************************************************************/
status_t KExtraDicCtrl::TrieSearch(const uchar* yomi,uint16 offset, MainRecord* &rec,uint32 *num)
{
	status_t		err;
	uint32		pos = 0;
	uint16		yomiLen;
	uint32		numKey;
	uchar		key[kKeyMax];

	if(maxRecNum == 0){
		fRec = (MainRecord*)malloc(sizeof(MainRecord)*10);
		if(fRec == NULL)return B_NO_MEMORY;
		maxRecNum = 10;
	}
	rec = fRec;

	(*num) = 0;
	if(offset > kKeyMax)
		yomiLen = kKeyMax;
	else
		yomiLen = offset;
	for(uint16 i=0;i<yomiLen;i++)key[i] = yomi[offset-1-i];

	/* 全ての辞書に対して検索を行う */
	for(int32 i=0;i<entry_size_;i++){
		while(1){
			err = DATrieMatch(&dicList_[i].dr,yomiLen,key,fRec+pos,maxRecNum-pos,&numKey);
			if(err != B_NO_ERROR)return err;
			if(maxRecNum-pos != numKey){
				break;
			}
			else{
				/* 検索結果格納領域を増やす */
				MainRecord	*tmp;
				
				tmp = (MainRecord*)malloc(sizeof(MainRecord)*(maxRecNum+10));
				if(tmp == NULL)return B_NO_MEMORY;
				for(uint32 i=0;i<maxRecNum;i++)tmp[i] = fRec[i];
				free(fRec);
				fRec = tmp;
				rec = fRec;
				maxRecNum += 10;
			}
		}
		pos += numKey;
		(*num) += numKey;
	}
	return B_NO_ERROR;
}

/***************************************************************
関数名	:	KExtraDicCtrl::Append
機能		:	学習用の追加を行う
入力		:	uchar*		yomi	: 読み
			uint16		yomiLen	: 読みサイズ
			uchar		*hyoki	: 表記
			HindoType		hindo	: 頻度
			Category		hinshi	: 品詞
出力		:	status_t
作成日	:	97.12.30
***************************************************************/
status_t KExtraDicCtrl::Append(const uchar* yomi, uint16 yomiLen,uchar* hyoki, HindoType hindo, Category hinshi)
{
	status_t		err;
	MainRecord	mainRec;
	unsigned long	numKey;
	DicData		dicData;
	uchar		key[kKeyMax];
	
	if(userDic == -1){
		/* ユーザー辞書は登録されていないので、学習を行わない */
		return B_NO_ERROR;
	}
	
	if(yomiLen > kKeyMax)return kMADBadKeySizeErr;
	ReverseKey(key,yomi,yomiLen);

	err = DAExactMatch(&dicList_[userDic].dr,yomiLen,key,&mainRec,1,&numKey);
	if(err == B_NO_ERROR){
		if(numKey == 0)mainRec.recordSize = 0;
		err = dicData.AddData(mainRec.recordData,&mainRec.recordSize,hyoki,hinshi,hindo,kLearnedWord);
	}
	if(err == B_NO_ERROR){
		err = DAUpdateMainRecord(&dicList_[userDic].dr,yomiLen,key,mainRec.recordSize,mainRec.recordData);
	}

	return err;
}
/*--- end of DicCtrl.cpp ---*/
