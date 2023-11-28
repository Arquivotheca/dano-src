/************************************************************************
ファイル名	:	DoubleArray.c

作成日	:	97.7.27
************************************************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include"DoubleArray.h"
#include"DAcommon.h"

static void FreeTables(DicRecord *dr);
static void FreeBCTAIL(DicRecord *dr);
static short CmpKey(DicRecord *dr,const Byte *s1,short s1_len,unsigned short blk);

/*****************************************************************************************
関数名	:	DAOpenDictionary
機能		:	辞書をオープンする
入力		:	unsigned short	dicID			: 辞書ID
出力		:	DAErr
作成日	:	97.7.29
修正日	:	97.12.21
*****************************************************************************************/
DAErr DAOpenDictionary(DicRecord *dr)
{
	DAErr		err;
	InitDicRecord(dr);
	dr->dicPtr = NULL;

	if(dr->refCount == 0){
		/* まだオープンしていない */

			
		/* 辞書のセットアップ */
		err = AllocateBCTAIL(dr);
		if(err == kDANoErr){
			err = SetUpDic(dr);
			if(err == kDANoErr)
				err = InitBlockCache(dr);
			if(err != kDANoErr)
				FreeDicRecord(dr);
		}
		if(err != kDANoErr){
		}
		else
			dr->refCount = 1;
	}
	else{
		/* 既にオープンしている */
//		dr->refCount++;
		err = kDANoErr;
	}

	return err;
}

/*****************************************************************************************
関数名	:	DACloseDictionary
機能		:	辞書をクローズする
入力		:	unsigned short	dicID			: 辞書ID
出力		:	DAErr
作成日	:	97.7.30
修正日	:	97.12.21
*****************************************************************************************/
DAErr DACloseDictionary(DicRecord *dr)
{
	DAErr		err = kDANoErr;

	if(dr->refCount == 1){
		if(dr->block.same_c != kNoBlock && dr->block.writeFlg){
			/* メモリー中のデータの書き込み */
			dr->B_C = dr->block.addB_C;
			dr->TAIL = dr->block.addTAIL;
			err = W_BCTF(dr,dr->block.same_c,true);
		}
	}	
	if(err == kDANoErr){
		if(dr->refCount == 1){
			/* リファレンスカウントが 0になる */
	
			/* キャッシュ領域の開放 */
			DisposeAllBlockCache(dr);
			/* 辞書レコードの解放，初期化 */
			FreeDicRecord(dr);
			InitDicRecord(dr);
		}
		else
			dr->refCount --;
	}

	return err;
}

/*****************************************************************************************
関数名	:	InitDicRecord
機能		:	辞書レコードの初期化を行う
入力		:	DicRecord			*dr				: 辞書レコード
出力		:	DAErr
作成日	:	97.7.29
修正日	:	98.2.8
*****************************************************************************************/
void InitDicRecord(DicRecord *dr)
{
	dr->refCount			= 0;
//	dr->DaPos				= -1;
	dr->tablePos			= -1;
	dr->TABLE				= NULL;
//	dr->First				= NULL;
	dr->block.same_c		= kNoBlock;
	dr->block.writeFlg		= false;
	dr->block.addBlockPtr	= NULL;
	dr->block.addB_C		= NULL;
	dr->block.addTAIL		= NULL;
}

/*****************************************************************************************
関数名	:	AllocateBCTAIL
機能		:	辞書レコードのダブル配列の領域を確保する
入力		:	DicRecord		*dr		: 辞書レコード
出力		:	DAErr
修正日	:	97.7.29
*****************************************************************************************/
DAErr AllocateBCTAIL(DicRecord *dr)
{
	DAErr	err = kDANoErr;
	short	flag=0;

	dr->block.addB_C = (short *)malloc(sizeof(short) * kBCSize);
	if(dr->block.addB_C == NULL)return kDAMemErr;

	dr->block.addTAIL = (Byte *)malloc(sizeof(Byte) * kTailSize);
	if(dr->block.addTAIL == NULL)err = kDAMemErr;

	if(err == kDANoErr){
		dr->B_C			= dr->block.addB_C;
		dr->TAIL			= dr->block.addTAIL;
	}
	else{
		free(dr->block.addB_C);
		dr->block.addB_C = NULL;
	}
		
	return err;
}

/*****************************************************************************************
関数名	:	FreeDicRecord
機能		:	辞書のテーブル等の領域を解放する
入力		:	DicRecord	*dr		: 辞書レコード
出力		:	なし
作成日	:	97.7.29
*****************************************************************************************/
void FreeDicRecord(DicRecord *dr)
{
	FreeBCTAIL(dr);
	FreeTables(dr);
}

/*****************************************************************************************
関数名	:	FreeBCTAIL
機能		:	辞書レコードのダブル配列の領域を開放する
入力		:	DicRecord	*dr		: 辞書レコード
出力		:	なし
修正日　	:	97.7.29
*****************************************************************************************/
static void FreeBCTAIL(DicRecord *dr)
{
	if(dr->block.addB_C != NULL){
		free(dr->block.addB_C);
		dr->block.addB_C = NULL;
	}
	if(dr->block.addTAIL != NULL){
		free(dr->block.addTAIL);
		dr->block.addTAIL = NULL;
	}
}

/*****************************************************************************************
関数名	:	FreeTables
機能		:	ブロック管理テーブルなどの領域を解放
入力		:	DicRecord	*dr		: 辞書レコード
出力		:	なし
作成日	:	97.7.29
修正日	:	97.12.18
*****************************************************************************************/
static void FreeTables(DicRecord *dr)
{
	if(dr->TABLE != NULL){
		free(dr->TABLE);
		dr->TABLE = NULL;
	}
}

/*****************************************************************************************
関数名	:	AllocateTables
機能		:	ブロック管理テーブルなどの領域を確保
入力		:	DicRecord *dr		: 辞書レコード
			short	keyMax	: 辞書の最大キー長
出力		:	DAErr
作成日	:	97.7.29
修正日	:	97.12.18
*****************************************************************************************/
DAErr AllocateTables(DicRecord *dr)
{
	dr->TABLE = (struct TBCS *)malloc(sizeof(struct TBCS)*(long)dr->maxBlock);
	if(dr->TABLE == NULL)return kDAMemErr;
		
	return kDANoErr;
}

/*****************************************************************************************
関数名	:	INI_BCT
機能		:	ダブル配列の初期化
入力		:	DicRecord		*dr		: 辞書レコード
出力		:	なし
作成日	:	97.7.29
*****************************************************************************************/
void INI_BCT(DicRecord *dr)
{
	short	i;		//	BASE,CHECK配列ループ用

	for (i = 0; i < kBCSize; i++) dr->B_C[i] = kNoUseCode;
//	dr->TAIL[0] = kNoMoreData;
}

/*****************************************************************************************
関数名	:	INI_BCTWithValue
機能		:	ダブル配列を初期化し、初期値を与える
入力		:	DicRecord		*dr		: 辞書レコード
出力		:	なし
作成日	:	97.7.29
*****************************************************************************************/
void INI_BCTWithValue(DicRecord *dr)
{
	INI_BCT(dr);

	dr->BC_MAX = 1;
	dr->TA_POS = 1;

	W_BASE(dr,1,1);
	W_BASE(dr,0,1);
	W_CHECK(dr,0,1);
}

/*****************************************************************************************
関数名	:	W_BASE
機能		:	ダブル配列上の状態番号の位置に BASE 値を書き込む
入力		:	DicRecord		*dr		: 辞書レコード
			short		pos		: 状態番号
			short		value	: BASE 値
出力		:	書き込み成功時 true
作成日	:	97.7.29
*****************************************************************************************/
Boolean W_BASE(DicRecord *dr,short pos, short value)
{
	/* 現在の使用されているダブル配列のサフィクスの最大値の更新 */
	if (pos > dr->BC_MAX && value != kNoUseCode)
		dr->BC_MAX = pos;
	if(value == kNoUseCode && dr->BC_MAX == pos && dr->BC_MAX > 1)
		dr->BC_MAX --;

	/* ダブル配列の大きさを越えた */
	if((dr->BC_MAX+1)*2 > kBCSize ){
		dr->block.same_c = kNoBlock;
		return false;
	}

	dr->B_C[2 * pos] = value;

	return true;
}

/*****************************************************************************************
関数名	:	W_CHECK
機能		:	ダブル配列上の状態番号の位置に CHECK 値を書き込む
入力		:	DicRecord		*dr		: 辞書レコード
			short		pos		: 状態番号
			short		value	: CHECK 値
出力		:	書き込み成功時 true
作成日	:	97.7.29
*****************************************************************************************/
Boolean W_CHECK(DicRecord *dr,short pos, short value)
{
	/* 現在の使用されているダブル配列のサフィクスの最大値の更新 */
	if (pos > dr->BC_MAX && value != kNoUseCode)
		dr->BC_MAX = pos;
	if(value == kNoUseCode && dr->BC_MAX == pos && dr->BC_MAX > 1)
		dr->BC_MAX --;

	/* ダブル配列の大きさを越えた */
	if((dr->BC_MAX+1)*2 > kBCSize ){
		dr->block.same_c = kNoBlock;
		return false;
	}

	dr->B_C[2 * pos + 1] = value;

	return true;
}

/*****************************************************************************************
関数名	:	SetUpDic
機能		:	ブロック管理用のテーブルを読み込む
入力		:	DicRecord	*dr		: 辞書レコード
出力		:	DAErr
作成日	:	97.7.30
修正日	:	98.2.8
*****************************************************************************************/
DAErr SetUpDic(DicRecord *dr)
{
	DAErr	err;
//	long		fileOffset;
	long		size,dicPos;
	DicHeader	dicHeader;
	DicHeader1	dicHeader1;

	err = DicSetPosition(dr,&dicPos,0);
	if(err == kDANoErr){
		err = DicReadData(dr,&dicPos,&dicHeader1,sizeof(DicHeader1));
		if(err != kDANoErr)return err;
		
		/* バージョンのチェック */
		dicHeader1.vers = B_LENDIAN_TO_HOST_INT32(dicHeader1.vers);	//add for Intel
		if(dicHeader1.vers != kDicVers)return kDAInvalidVersion;
	}

	if(err == kDANoErr)
		err = DicReadData(dr,&dicPos,&dicHeader,sizeof(DicHeader));
	
	if(err != kDANoErr)return err;

	dr->dicPerm = B_LENDIAN_TO_HOST_INT16(dicHeader.dicPerm);					//change for Intel
	dr->maxBlock = B_LENDIAN_TO_HOST_INT16(dicHeader.maxBlock);					//change for Intel
	dr->TblNum = B_LENDIAN_TO_HOST_INT16(dicHeader.TblNum);						//change for Intel
	dr->tablePos = B_LENDIAN_TO_HOST_INT32(dicHeader.tablePos);					//change for Intel
	dr->block.blockCacheNum = B_LENDIAN_TO_HOST_INT16(dicHeader.cacheBlockNum);	//change for Intel
	err = AllocateTables(dr);

//	if(err == kDANoErr)
//		err = DicGetPosition(dr,dicPos,&fileOffset);
	if(err == kDANoErr)
		err = DicSetPosition(dr,&dicPos,dr->tablePos);
	if(err == kDANoErr){
		size = dr->TblNum*sizeof(struct TBCS);
		err = DicReadData(dr,&dicPos,(void *)dr->TABLE,size);
//		dr->DaPos = fileOffset + dr->maxBlock*sizeof(struct TBCS);
	}
	
	if(err == kDANoErr){	//add for Intel
		unsigned short	i;
		for(i = 0;i<dr->TblNum;i++){
			dr->TABLE[i].index = B_LENDIAN_TO_HOST_INT16(dr->TABLE[i].index);
			dr->TABLE[i].bcsize = B_LENDIAN_TO_HOST_INT16(dr->TABLE[i].bcsize);
			dr->TABLE[i].keynum = B_LENDIAN_TO_HOST_INT16(dr->TABLE[i].keynum);
			dr->TABLE[i].bcaddr = B_LENDIAN_TO_HOST_INT32(dr->TABLE[i].bcaddr);
		}
	}
	
	dr->block.same_c = kNoBlock;
	if(err != kDANoErr){
		FreeTables(dr);
	}

	return err;
}

/*****************************************************************************************
関数名	:	R_Add_BCTF
機能		:	ディスクから追加用のメモリ上にダブル配列を読み込む
入力		:	DicRecord		*dr	: 辞書レコード
			unsigned short		block	: ブロック番号
出力		:	DAErr
作成日	:	97.7.31
*****************************************************************************************/
DAErr R_Add_BCTF(DicRecord *dr,unsigned short block)
{
	DAErr	err;
	short	bc_max,tail_size,bc_size,i;
	
	if ((err = R_BCTF(dr,block,(Byte *)dr->block.addB_C)) != kDANoErr){
		dr->block.same_c = kNoBlock;
		return err;
	}
	bc_max = dr->block.addB_C[0];
	tail_size = dr->block.addB_C[1];
	bc_size = (bc_max+1)*2*sizeof(short);
	memmove(dr->block.addTAIL,(Byte *)dr->block.addB_C + bc_size,tail_size);
	for(i=(bc_max+1)*2;i<kBCSize; i++)dr->block.addB_C[i] = kNoUseCode;
	dr->block.same_c = block;

	return kDANoErr;
}

/*****************************************************************************************
関数名	:	R_BCTF
機能		:	ディスクからメモリ上にダブル配列を読み込む
入力		:	DicRecord		*dr		: 辞書レコード
			unsigned short		i		: ブロック番号
			Byte			*data	: 読み込んだデータ(out)
出力		:	DAErr
修正日	:	98.2.8
*****************************************************************************************/
DAErr R_BCTF(DicRecord *dr,unsigned short i,Byte *data)
{
	long		pos	;
	DAErr	err;
	long		size,dicPos;

	/* ダブル配列の読み込み */
	pos = dr->TABLE[i].bcaddr;
//	pos = dr->DaPos + dr->TABLE[i].bcaddr;
	err = DicSetPosition(dr,&dicPos,pos);
	
	if(err == kDANoErr){
		size = dr->TABLE[i].bcsize;
		err = DicReadData(dr,&dicPos,(void *)data,size);
		if(err == kDANoErr){										//add for Intel
			short	*bc,j;
			bc = (short *)data;
			for(j=0;j<size/sizeof(short);j++){
				bc[j] = B_LENDIAN_TO_HOST_INT16(bc[j]);
			}
		}
	}

	if(err == kDANoErr){
		size = ((short *)data)[1];
		err = DicReadData(dr,&dicPos,(void *)(data + dr->TABLE[i].bcsize),size);
	}

//	if(err != noErr)dr->block.same_c = kNoBlock;

	return err;
}

/*****************************************************************************************
関数名	:	W_BCTF
機能		:	ダブル配列をディスク上に書き込む
入力		:	DicRecord	*dr			: 辞書レコード
			unsigned short	i			: ブロック番号
			Boolean	writeTable	: true時テーブルも書き込む
出力		:	DAErr
作成日	:	98.2.8
*****************************************************************************************/
DAErr W_BCTF(DicRecord *dr,unsigned short i,Boolean writeTable)
{
	long		pos;
	DAErr	err;
	long		size,dicPos;

	if(i == kNoBlock){
		dr->block.writeFlg = false;
		return kDANoErr;
	}

	/* ダブル配列の書き込み */
	pos = dr->TABLE[i].bcaddr ;
//	pos = dr->DaPos + dr->TABLE[i].bcaddr ;
	err = DicSetPosition(dr,&dicPos,pos);

	if(err == kDANoErr){
		short	j,bc[kBlockSize];							//add for Intel
		
		size = dr->TABLE[i].bcsize;
			
		for(j=0;j<size/sizeof(short);j++){					//add for Intel
			bc[j] = B_HOST_TO_LENDIAN_INT16(dr->B_C[j]);
		}
		err = DicWriteData(dr,&dicPos,(void *)bc,size);	//change for Intel
//		err = DicWriteData(dr,&dicPos,(void *)dr->B_C,size);
	}

	if(err == kDANoErr){
		size = dr->B_C[1];
		err = DicWriteData(dr,&dicPos,(void *)dr->TAIL,size);
	}

	if(writeTable){
		if(err == kDANoErr){
			pos = dr->tablePos;
//			pos = dr->DaPos-dr->maxBlock*sizeof(struct TBCS);
			err = DicSetPosition(dr,&dicPos,pos+i*sizeof(struct TBCS));
		}
		
		if(err == kDANoErr){
			struct TBCS	table;										//add for Intel

			table.index = B_HOST_TO_LENDIAN_INT16(dr->TABLE[i].index);		//add for Intel
			table.bcsize = B_HOST_TO_LENDIAN_INT16(dr->TABLE[i].bcsize);		//add for Intel
			table.keynum = B_HOST_TO_LENDIAN_INT16(dr->TABLE[i].keynum);		//add for Intel
			table.bcaddr = B_HOST_TO_LENDIAN_INT32(dr->TABLE[i].bcaddr);		//add for Intel
			memmove(table.First,dr->TABLE[i].First,kUpperByte);				//add for Intel

			size = sizeof(struct TBCS);
			err = DicWriteData(dr,&dicPos,(void *)&(table),size);			//change for Intel
//			err = DicWriteData(dr,&dicPos,(void *)&(dr->TABLE[i]),size);
		}
	}
	
	if(err != kDANoErr)dr->block.same_c = kNoBlock;

	dr->block.writeFlg = false;

	return err;
}

/*****************************************************************************************
関数名	:	BCODE
機能		:	検索文字列からダブル配列の格納されているブロック番号を求める
入力		:	DicRecord		*dr		: 辞書レコード
			Byte			*key		: 検索文字列
			short		len		: keyの長さ
出力		:	ブロック番号
作成日	:	97.7.31
*****************************************************************************************/
unsigned short BCODE(DicRecord *dr,const Byte *key,short len)
{
	long		lo,hi,mid;

	lo = 0;
	hi = (long)dr->TblNum-1;

	while(lo<=hi){
		mid = (lo+hi)/2;
		if(CmpKey(dr,key,len,mid)<=0)
			hi = mid-1;
		else
			lo = mid+1;
	}

	if(hi < 0 )
		return 0;
	if(hi >= dr->TblNum-1)
		return (unsigned short)(dr->TblNum-2);
	if(CmpKey(dr,key,len,hi+1)==0)
		hi++;
	return (unsigned short)hi;
}

/*****************************************************************************************
関数名	:	CmpKey
機能		:	ユニークＩＤを含めての２つのバイト列の比較を行う
入力		:	DicRecord	*dr			: 辞書レコード
			Byte		*s1			: 比較文字列１
			short	s1_len		: s1の長さ
			unsigned short	blk			: 比較するブロックのインデックス
出力		:	s1>s2の時正，s1<s2の時負，s1==s2の時０を返す
作成日	:	97.7.31
修正日	:	97.12.18
*****************************************************************************************/
static short CmpKey(DicRecord *dr,const Byte *s1,short s1_len,unsigned short blk)
{
	Byte			c1,*c2;
	short		s2_len;
	int			i=0;

	c1 = *(s1+i);
	c2 = &(dr->TABLE[blk].First[1]);
	s2_len = dr->TABLE[blk].First[0];

	while(c1 == *c2){
		i++;
		c1 = *(s1+i);
		c2 ++;
		s1_len --;
		s2_len --;
		if(!s1_len || !s2_len){
			if(s1_len == s2_len)
				return 0;
			else
				return s1_len - s2_len;
		}
	}
	return c1 - *c2;
}

/*****************************************************************************************
関数名	:	ChangeBlock
機能		:	メモリー中のブロックを変更する
入力		:	DicRecord		*dr		: 辞書レコード
			unsigned short		block		: ブロック番号
			Boolean		isSearch	: 検索時true
出力		:	DAErr
作成日	:	97.7.31
*****************************************************************************************/
DAErr ChangeBlock(DicRecord *dr,unsigned short block,Boolean isSearch)
{
	DAErr		err;
	unsigned short		index;
	BlockCache	*blockCache;
	short		bc_max,bc_size,tail_size,i;

	index = dr->TABLE[block].index;
	
	if(block == dr->block.same_c){
		if(dr->block.addBlockPtr == NULL || dr->block.addBlockPtr->index != index){
			GetLastBlockCache(dr,&blockCache);
			SetFirstBlockCache(dr,blockCache);
			dr->block.addBlockPtr = blockCache;
			SetBlockCache(dr,index,blockCache);
		}
		else
			MoveFirstBlcokCache(dr,dr->block.addBlockPtr);
		/* 変更ブロック中に存在する */
		dr->B_C = dr->block.addB_C;
		dr->TAIL = dr->block.addTAIL;
	}
	else{
		if(isSearch){
			/* 検索時 */
			if(SearchBlockCache(dr,index,&blockCache) == true){
				bc_max = *(short *)blockCache->BCT;
				dr->B_C = (short *)blockCache->BCT;
				dr->TAIL = (blockCache->BCT + (bc_max+1)*2*sizeof(short));
			}
			else{
				err = PutBlock2Cache(dr,block,index);
				if(err != kDANoErr)return err;
			}
		}
		else{
			/* 変更時 */
			if(dr->block.same_c != kNoBlock){
				if(dr->block.writeFlg == true){
					/* 辞書へ書き込む */
					dr->B_C = dr->block.addB_C;
					dr->TAIL = dr->block.addTAIL;
					err = W_BCTF(dr,dr->block.same_c,true);
					if(err != kDANoErr)return err;
				}
				blockCache = dr->block.addBlockPtr;
				if(blockCache != NULL && blockCache->index == dr->TABLE[dr->block.same_c].index){
					bc_max = dr->block.addB_C[0];
					bc_size = (bc_max+1)*2*sizeof(short);
					memmove(blockCache->BCT,dr->block.addB_C,bc_size);
					memmove(blockCache->BCT+bc_size,dr->block.addTAIL,dr->block.addB_C[1]);
				}
			}

			if(SearchBlockCache(dr,index,&blockCache) == true){
				bc_max = *(short *)blockCache->BCT;
				tail_size = ((short *)blockCache->BCT)[1];
				bc_size = (bc_max+1)*2*sizeof(short);
				memmove(dr->block.addB_C,blockCache->BCT,bc_size);
				for(i=(bc_max+1)*2;i<kBCSize; i++)dr->block.addB_C[i] = kNoUseCode;
				memmove(dr->block.addTAIL,blockCache->BCT+bc_size,tail_size);
				dr->block.same_c = block;
				dr->block.addBlockPtr = blockCache;
				MoveFirstBlcokCache(dr,blockCache);
			}
			else{
				if((err = R_Add_BCTF(dr,block)) != kDANoErr)return err;
				GetLastBlockCache(dr,&blockCache);
				SetFirstBlockCache(dr,blockCache);
				SetBlockCache(dr,index,blockCache);
				dr->block.addBlockPtr = blockCache;
				bc_max = dr->block.addB_C[0];
				bc_size = (bc_max+1)*2*sizeof(short);
				memmove(blockCache->BCT,dr->block.addB_C,bc_size);
				memmove(blockCache->BCT+bc_size,dr->block.addTAIL,dr->block.addB_C[1]);
			}
			dr->B_C = dr->block.addB_C;
			dr->TAIL = dr->block.addTAIL;
		}
	}
	
	return kDANoErr;
}

/*****************************************************************************************
関数名	:	R_STR
機能		:	接尾辞とレコードを得る
入力		:	short	pos			: TAIL_Ptr 上の読みだし位置
			Byte		**CONT		: レコードのポインター(out)
			DataType	*cont_size	: CONTのサイズ(out)
			Byte		*S_TEMP		: 接尾辞を格納(out)
			short	*s_temp_size	: S_TEMPのサイズ(out)
			Byte		*TAIL_Ptr	: TAILを指すポインター
出力		:	なし
作成日	:	97.7.31
修正日	:	97.12.17
*****************************************************************************************/
void R_STR(short pos,Byte **CONT, DataType *cont_size,Byte *S_TEMP,short *s_temp_size,Byte *TAIL_Ptr)
{
	short	i;

	pos -= kTailOffset;

	(*s_temp_size) = TAIL_Ptr[pos];
	memmove(S_TEMP,TAIL_Ptr+pos+1,(*s_temp_size));

	/* 接尾辞のマスクをはずす */
	for(i=0;i<(*s_temp_size);i++)S_TEMP[i]^=kDicMask;

	memmove(cont_size,TAIL_Ptr+pos+sizeof(unsigned char)+(*s_temp_size),sizeof(DataType));
#if kData2Bytes																	//add for Intel
	(*cont_size) = B_LENDIAN_TO_HOST_INT16(*cont_size);
#endif
	(*CONT) = TAIL_Ptr+pos+1+(*s_temp_size)+sizeof(DataType);
}

/*****************************************************************************************
関数名	:	W_TABLE
機能		:	num番目以降のブロック管理用のテーブルを書き込む
入力		:	DicRecord		*dr	: 辞書レコード
			unsigned short		num	:ブロック番号
出力		:	DAErr
作成日	:	97.7.31
修正日	:	98.2.9
*****************************************************************************************/
DAErr W_TABLE(DicRecord *dr,unsigned short num)
{
	DAErr			err;
	unsigned short	maxBlock;
	long				dicPos;
	
	maxBlock = dr->maxBlock;

	if(num == kWriteAllTable){
		err = DicSetPosition(dr,&dicPos,dr->tablePos);
		if(err == kDANoErr){
			unsigned short	i;														//add for Intel
			
			for(i=0;i<maxBlock;i++){													//add for Intel	
				dr->TABLE[i].index = B_HOST_TO_LENDIAN_INT16(dr->TABLE[i].index);
				dr->TABLE[i].bcsize = B_HOST_TO_LENDIAN_INT16(dr->TABLE[i].bcsize);
				dr->TABLE[i].keynum = B_HOST_TO_LENDIAN_INT16(dr->TABLE[i].keynum);
				dr->TABLE[i].bcaddr = B_HOST_TO_LENDIAN_INT32(dr->TABLE[i].bcaddr);
			}
				
			err =DicWriteData(dr,&dicPos,(void *)(dr->TABLE),maxBlock*sizeof(struct TBCS));

			for(i=0;i<maxBlock;i++){													//add for Intel	
				dr->TABLE[i].index = B_LENDIAN_TO_HOST_INT16(dr->TABLE[i].index);
				dr->TABLE[i].bcsize = B_LENDIAN_TO_HOST_INT16(dr->TABLE[i].bcsize);
				dr->TABLE[i].keynum = B_LENDIAN_TO_HOST_INT16(dr->TABLE[i].keynum);
				dr->TABLE[i].bcaddr = B_LENDIAN_TO_HOST_INT32(dr->TABLE[i].bcaddr);
			}
		}
	}
	else{
		long		size,pos;

		pos = sizeof(DicHeader1);
		err = DicSetPosition(dr,&dicPos,pos);
	
		if(err == kDANoErr)
			err = WriteHeader(dr,dr->dicPerm,dr->maxBlock,dr->TblNum,dr->block.blockCacheNum,dr->tablePos,&dicPos);
	
		if(err == kDANoErr){
			pos += sizeof(DicHeader);
			err = DicSetPosition(dr,&dicPos,pos+num*sizeof(struct TBCS));
		}
		if(err == kDANoErr){
			unsigned short	i;														//add for Intel

			for(i = num;i<dr->TblNum;i++){												//add for Intel	
				dr->TABLE[i].index = B_HOST_TO_LENDIAN_INT16(dr->TABLE[i].index);
				dr->TABLE[i].bcsize = B_HOST_TO_LENDIAN_INT16(dr->TABLE[i].bcsize);
				dr->TABLE[i].keynum = B_HOST_TO_LENDIAN_INT16(dr->TABLE[i].keynum);
				dr->TABLE[i].bcaddr = B_HOST_TO_LENDIAN_INT32(dr->TABLE[i].bcaddr);
			}

			size = (dr->TblNum-num)*sizeof(struct TBCS);
			err =DicWriteData(dr,&dicPos,(void *)&(dr->TABLE[num]),size);

			for(i=num;i<dr->TblNum;i++){												//add for Intel	
				dr->TABLE[i].index = B_LENDIAN_TO_HOST_INT16(dr->TABLE[i].index);
				dr->TABLE[i].bcsize = B_LENDIAN_TO_HOST_INT16(dr->TABLE[i].bcsize);
				dr->TABLE[i].keynum = B_LENDIAN_TO_HOST_INT16(dr->TABLE[i].keynum);
				dr->TABLE[i].bcaddr = B_LENDIAN_TO_HOST_INT32(dr->TABLE[i].bcaddr);
			}
		}
	}

	return err;
}

/*****************************************************************************************
関数名	:	DeleteState
機能		:	入力された状態番号 s の状態を除く
入力		:	DicRecord		*dr		: 辞書レコード
			short		s		: 状態番号
出力		:	なし
作成日	:	97.7.31
*****************************************************************************************/
void DeleteState(DicRecord *dr,short s)
{
	W_BASE(dr,s, kNoUseCode);
	W_CHECK(dr,s, kNoUseCode);
}

/*****************************************************************************************
関数名	:	TailGab
機能		:	TAIL のゴミ集めを行なう
入力		:	DicRecord		*dr		: 辞書レコード
			Byte			*buf		: ゴミ集めのためのバッファ領域( >= kTailSize )
出力		:	ゴミ集め成功時true
作成日	:	97.8.8
*****************************************************************************************/
Boolean TailGab(DicRecord *dr,Byte *buf)
{
	int			i,moveSize;
	int			pos; 			// 接尾辞とレコードの記憶位置
	int			new_pos; 		// テンポラリー領域上の記憶位置 
	unsigned char		strLen;
	DataType		dataSize;

	/* テンポラリー領域の初期化 */
	new_pos = 1;

	for (i = 1; i <= dr->B_C[0]; i++) {
		/* 接尾辞とレコードへのポインタ */
		if (dr->B_C[2 * i] < kNoUseCode) {
			pos = (-1) * dr->B_C[2 * i]-kTailOffset;

			/* 接尾辞のサイズを求める */
			strLen = dr->TAIL[pos];
			/* レコードのサイズを求める */
			memmove(&dataSize,dr->TAIL+pos+sizeof(unsigned char)+strLen,sizeof(DataType));
			dataSize = B_LENDIAN_TO_HOST_INT16(dataSize);								//add for Intel

			moveSize = sizeof(unsigned char)+strLen+sizeof(DataType)+dataSize;

			if(new_pos+moveSize > kTailSize)return false;

			/* ダブル配列上に新たな記憶位置を書き込む */
			W_BASE(dr,i, (-1) * new_pos-kTailOffset);

			/* テンポラリー領域上に接尾辞とレコードを書き込む */
			memmove(buf+new_pos,dr->TAIL+pos,moveSize);
			new_pos += moveSize;
		}
	}

	/* テンポラリー領域の内容を移す */
	memmove(dr->TAIL,buf,new_pos);

	/* ダブル配列上に大きさを書き込む */
	W_CHECK(dr,0, new_pos);
	dr->TA_POS = new_pos;
	
	return true;
}
