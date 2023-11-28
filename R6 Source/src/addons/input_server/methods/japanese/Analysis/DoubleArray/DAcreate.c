/************************************************************************
ファイル名	:	DACreate.c

作成日	:	97.7.29
************************************************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include"DoubleArray.h"
#include"DAcommon.h"

static DAErr WriteInitDictionary(DicRecord *dr,DicProperty *dicPropertyData);
static DAErr CreatUdic(DicRecord *dr,DicProperty *dicPropertyData,long *dicPos);

/*******************************************************************
関数名	:	DACreateNewDictionary
機能		:	辞書を作成する
入力		:	char			*fullPathDicName		: 辞書ファイル名
			DicProperty	*dicPropertyData		: 辞書属性
			unsigned short		*dicID				: 辞書ID(out)
出力		:	DAErr
作成日	:	97.7.29
修正日	:	97.10.27
*******************************************************************/
DAErr DACreateNewDictionary(const char *fullPathName,DicProperty *dicPropertyData)
{
	DAErr		err;
	DicRecord		dr;

	err = dr.fp.SetTo(fullPathName,B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if(err == B_NO_ERROR){
		if(err == kDANoErr){
			dr.dicPtr = NULL;
			if(kMaxBlockNum <= dicPropertyData->dicBlockNum)
				dicPropertyData->dicBlockNum = kMaxBlockNum;
			if(kMinBlockNum >= dicPropertyData->dicBlockNum)
				dicPropertyData->dicBlockNum = kMinBlockNum;
			/* 辞書レコードの初期化 */
			InitDicRecord(&dr);

			err = AllocateBCTAIL(&dr);
		}
		/* 辞書の初期データを書き込む */
		if(err == kDANoErr)
			err = WriteInitDictionary(&dr,dicPropertyData);
	
		if(err != kDANoErr){
			/* エラーの場合は辞書を削除 */
			BEntry e(fullPathName);
			
			dr.fp.Unset();
			e.Remove();
		}
	}

	return err;
}

/*************
	static 関数		
*************/
/*****************************************************************************************
関数名	:	WriteInitDictionary
機能		:	辞書の初期情報を書き込む
入力		:	DicRecord			*dr				: 辞書レコード
			DicProperty		*dicPropertyData	: 辞書プロパティー
出力		:	DAErr
作成日	:	97.7.29
修正日	:	98.2.8
*****************************************************************************************/
static DAErr WriteInitDictionary(DicRecord *dr,DicProperty *dicPropertyData)
{
	DAErr	err;
	long		dicPos = 0L;
	DicHeader1	dh;
	
	dh.vers = B_HOST_TO_LENDIAN_INT32(kDicVers);					//change for Intel
	memmove(dh.sig,kDicSignature,8);
	dh.dicKind = B_HOST_TO_LENDIAN_INT16(dicPropertyData->dicKind);	//change for Intel
	err = DicWriteData(dr,&dicPos,&dh,sizeof(DicHeader1));

	/* ヘッダ部の書き込み */
	if(err == kDANoErr)
		err = WriteHeader(dr,kReadWritePerm,dicPropertyData->dicBlockNum,kInitBlockNum,kInitBlockCacheNum,dicPos+sizeof(DicHeader),&dicPos);
	
	/* ブロック情報の書き込み */
	if(err == kDANoErr)
		err = CreatUdic(dr,dicPropertyData,&dicPos);

	FreeDicRecord(dr);

	return err;
}

/*****************************************************************************************
関数名	:	WriteHeader
機能		:	辞書ヘッダー部の書き込みを行う
入力		:	DicRecord	*dr			: 辞書レコード
			long		*dicPos		: 辞書位置
			unsigned short	dicPerm		: 辞書のパーミッション
			unsigned short	maxBlock		: 最大ブロック数
			unsigned short	tblNum		: ブロック数
			unsigned short	cacheBlockNum	: キャッシュブロック数
出力		:	DAErr
作成日	:	97.7.29
修正日	:	98.2.8
*****************************************************************************************/
DAErr WriteHeader(DicRecord *dr,unsigned short dicPerm,unsigned short maxBlock,unsigned short tblNum,unsigned short cacheBlockNum,long tablePos,long *dicPos)
{
	DicHeader	dicHeader;
	DAErr	err;

	dicHeader.dicPerm = B_HOST_TO_LENDIAN_INT16(dicPerm);			//change for Intel
	dicHeader.maxBlock = B_HOST_TO_LENDIAN_INT16(maxBlock);			//change for Intel
	dicHeader.TblNum = B_HOST_TO_LENDIAN_INT16(tblNum);				//change for Intel
	dicHeader.cacheBlockNum = B_HOST_TO_LENDIAN_INT16(cacheBlockNum);	//change for Intel
	dicHeader.tablePos = B_HOST_TO_LENDIAN_INT32(tablePos);			//change for Intel
	
	err = DicWriteData(dr,dicPos,&dicHeader,sizeof(DicHeader));

	return err;
}

/*****************************************************************************************
関数名	:	CreatUdic
機能		:	辞書の作成時にブロック管理用のテーブルを書き込む
入力		:	DicRecord	*dr		: 辞書レコード
			DicProperty		*dicPropertyData	: 辞書プロパティー
			long				*dicPos			: 辞書位置
出力		:	DAErr
作成日	:	97.7.29
修正日	:	98.2.8
*****************************************************************************************/
static DAErr CreatUdic(DicRecord *dr,DicProperty *dicPropertyData,long *dicPos)
{
	short	i;
	long		size;
	DAErr	err;

	/* 初期化 */
	dr->TblNum	= kMinBlockNum;
	dr->maxBlock	= dicPropertyData->dicBlockNum;

	err = AllocateTables(dr);
	if(err != kDANoErr)return err;

	dr->TABLE[0].First[0] = kKeyMax;
	dr->TABLE[1].First[0] = kKeyMax;
	for (i = 1; i <= kKeyMax; i++){
		dr->TABLE[0].First[i] = 0x00;
		dr->TABLE[1].First[i] = 0xff;
	}

	size = dr->maxBlock*sizeof(struct TBCS);
	/* テーブルの初期化 */
	dr->TABLE[0].index		= B_HOST_TO_LENDIAN_INT16(0);				//change for Intel
	dr->TABLE[0].bcaddr	= B_HOST_TO_LENDIAN_INT32((*dicPos) + size);	//change for Intel
	dr->TABLE[0].bcsize	= B_HOST_TO_LENDIAN_INT16(sizeof(short)*4);	//change for Intel
	dr->TABLE[0].keynum	= B_HOST_TO_LENDIAN_INT16(0);				//change for Intel

	err = DicWriteData(dr,dicPos,(void *)(dr->TABLE),size);
	
	{															//add for Intel
		dr->TABLE[0].index = B_LENDIAN_TO_HOST_INT16(dr->TABLE[0].index);
		dr->TABLE[0].bcaddr = B_LENDIAN_TO_HOST_INT32(dr->TABLE[0].bcaddr);
		dr->TABLE[0].bcsize = B_LENDIAN_TO_HOST_INT16(dr->TABLE[0].bcsize);
		dr->TABLE[0].keynum = B_LENDIAN_TO_HOST_INT16(dr->TABLE[0].keynum);
	}

	if(err == kDANoErr){
		err = DicSetSize(dr,(*dicPos) + kBlockSize);
	}

	if(err == kDANoErr){
		/* ダブル配列の初期化 */
		INI_BCTWithValue(dr);

		/* ダブル配列を書き込む */
		size = dr->TABLE[0].bcsize;
		for(i=0;i<size/sizeof(short);i++){							//add for Intel
			dr->B_C[i] = B_HOST_TO_LENDIAN_INT16(dr->B_C[i]);
		}
		err = DicWriteData(dr,dicPos,(void *)(dr->B_C),size);
		for(i=0;i<size/sizeof(short);i++){							//add for Intel
			dr->B_C[i] = B_LENDIAN_TO_HOST_INT16(dr->B_C[i]);
		}
	}
	if(err == kDANoErr){
		size = kBlockSize-dr->TABLE[0].bcsize;
		err = DicWriteData(dr,dicPos,(void *)(dr->TAIL),size);
	}

	return err;
}

