/************************************************************************
ファイル名	:	DACache.c

作成日	:	97.7.30
************************************************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include"DoubleArray.h"
#include"DAcommon.h"
#include"DAcache.h"

static DAErr NewBlockCache(BlockCache **newBlockCache);
static DAErr ChangeHashNum(DicRecord *dr);
static void RemoveHashList(DicRecord *dr,BlockCache *blockCache);
static unsigned short CalcHacheNum(unsigned short blockCacheNum);

/*****************************************************************************************
関数名	:	InitBlockCache
機能		:	ブロックキャッシュを初期化する
入力		:	DicRecord	*dr			: 辞書レコード
出力		:	DAErr
作成日	:	97.7.30
*****************************************************************************************/
DAErr InitBlockCache(DicRecord *dr)
{
	DAErr		err = kDANoErr;
	unsigned short		i,j;
	BlockCache	*tmpBlockCache;

	dr->block.hashNum = CalcHacheNum(dr->block.blockCacheNum);
	dr->block.hashTable = (BlockCache **)malloc(sizeof(BlockCache *) * dr->block.hashNum);
	if(dr->block.hashTable == NULL)err = kDAMemErr;
	if(err == kDANoErr){
		for(j=0;j<dr->block.hashNum;j++)
			dr->block.hashTable[j] = NULL;
		err = NewBlockCache(&(dr->block.firstBlockCache));
		if(err == kDANoErr){
			tmpBlockCache = dr->block.firstBlockCache;
			for(i=1;i<dr->block.blockCacheNum;i++){
				err = NewBlockCache(&(tmpBlockCache->nextUsed));
				if(err != kDANoErr)break;
				tmpBlockCache->nextUsed->prevUsed = tmpBlockCache;
				tmpBlockCache = tmpBlockCache->nextUsed;
			}
			if(err == kDANoErr)
				dr->block.lastBlockCache = tmpBlockCache;
			else
				DisposeAllBlockCache(dr);
		}
		if(err != kDANoErr)
			free(dr->block.hashTable);
	}

	return err;
}

/*****************************************************************************************
関数名	:	DisposeAllBlockCache
機能		:	全てのブロックキャッシュを削除する
入力		:	DicRecord	*dr			: 辞書レコード
出力		:	なし
作成日	:	97.7.30
*****************************************************************************************/
void DisposeAllBlockCache(DicRecord *dr)
{
	BlockCache	*tmpBlockCache;
	
	while(dr->block.firstBlockCache != NULL){
		tmpBlockCache = dr->block.firstBlockCache;
		dr->block.firstBlockCache = tmpBlockCache->nextUsed;
		if(tmpBlockCache->BCT != NULL)
			free(tmpBlockCache->BCT);
		free(tmpBlockCache);
	}
	free(dr->block.hashTable);
}

/*****************************************************************************************
関数名	:	SearchBlockCache
機能		:	キャッシュの検索を行う
入力		:	DicRecord			*dr			: 辞書レコード
			unsigned short			index		: 検索するブロックのインデックス
			BlockCache		**blockCache	: 検索成功時ブロックキャッシュを格納(out)
出力		:	検索成功時true
作成日	:	97.7.30
*****************************************************************************************/
Boolean SearchBlockCache(DicRecord *dr,unsigned short index,BlockCache **blockCache)
{
	BlockCache	*tmpBlockCache;
	
	tmpBlockCache = dr->block.hashTable[index%(dr->block.hashNum)];
	while(tmpBlockCache != NULL){
		if(tmpBlockCache->index == index){
			(*blockCache) = tmpBlockCache;
			MoveFirstBlcokCache(dr,tmpBlockCache);
			return true;
		}
		tmpBlockCache = tmpBlockCache->nextBlock;
	}
	return false;
}

/*****************************************************************************************
関数名	:	MoveFirstBlcokCache
機能		:	ブロックキャッシュをキャッシュリストからはずし，最初に移動する
入力		:	DicRecord			*dr			: 辞書レコード
			BlockCache		*blockCache	: 移動するインデックス
出力		:	なし
作成日	:	97.7.30
*****************************************************************************************/
void MoveFirstBlcokCache(DicRecord *dr,BlockCache *blockCache)
{	
	if(blockCache->prevUsed == NULL){
		/* 先頭の時はなにもしない */
		return;
	}

	blockCache->prevUsed->nextUsed = blockCache->nextUsed;
	if(blockCache->nextUsed != NULL)
		blockCache->nextUsed->prevUsed = blockCache->prevUsed;
	else
		dr->block.lastBlockCache = blockCache->prevUsed;

	SetFirstBlockCache(dr,blockCache);
}

/*****************************************************************************************
関数名	:	SetFirstBlockCache
機能		:	ブロックキャッシュリストの最初のデータにセットする
入力		:	DicRecord			*dr			: 辞書レコード
			BlockCache		*blockCache	: セットするブロックキャッシュ
出力		:	なし
作成日	:	97.7.30
*****************************************************************************************/
void SetFirstBlockCache(DicRecord *dr,BlockCache *blockCache)
{
	blockCache->nextUsed = dr->block.firstBlockCache;
	blockCache->prevUsed = NULL;
	dr->block.firstBlockCache->prevUsed = blockCache;
	dr->block.firstBlockCache = blockCache;
}

/*****************************************************************************************
関数名	:	GetLastBlockCache
機能		:	ブロックキャッシュリストの最後のブロックデータを取り出す
入力		:	DicRecord		*dr			: 辞書レコード
			BlockCache	**blockCache	: 最後のブロックキャッシュを格納(out)
出力		:	なし
作成日	:	97.7.30
*****************************************************************************************/
void GetLastBlockCache(DicRecord *dr,BlockCache **blockCache)
{
	(*blockCache) = dr->block.lastBlockCache;
	
	dr->block.lastBlockCache = dr->block.lastBlockCache->prevUsed;
	dr->block.lastBlockCache->nextUsed = NULL;
	
	RemoveHashList(dr,(*blockCache));
}

/*****************************************************************************************
関数名	:	RemoveHashList
機能		:	ハッシュのリストからブロックデータを取り外す
入力		:	DicRecord		*dr			: 辞書レコード
			BlockCache	*blockCache	: 外すブロックデータ
出力		:	なし
作成日	:	97.7.30
*****************************************************************************************/
static void RemoveHashList(DicRecord *dr,BlockCache *blockCache)
{
	unsigned short	index;

	if(blockCache->prevBlock != NULL){
		blockCache->prevBlock->nextBlock = blockCache->nextBlock;
		if(blockCache->nextBlock != NULL)
			blockCache->nextBlock->prevBlock = blockCache->prevBlock;
	}
	else{
		index = blockCache->index;
		if(dr->block.hashTable[index%(dr->block.hashNum)] == blockCache){
			dr->block.hashTable[index%(dr->block.hashNum)] = blockCache->nextBlock;
			if(blockCache->nextBlock != NULL)
				dr->block.hashTable[index%(dr->block.hashNum)]->prevBlock = NULL;
		}
	}
	blockCache->prevBlock = NULL;
	blockCache->nextBlock = NULL;
	blockCache->index = kNoBlock;
}

/*****************************************************************************************
関数名	:	SetLastBlockCache
機能		:	ブロックデータをハッシュリストから外し，ブロックキャッシュリストの最後にセットする
入力		:	DicRecord		*dr			: 辞書レコード
			BlockCache	*blockCache	: 外すブロックデータ
出力		:	なし
作成日	:	97.7.30
*****************************************************************************************/
void SetLastBlockCache(DicRecord *dr,BlockCache *blockCache)
{
	RemoveHashList(dr,blockCache);

	if(dr->block.lastBlockCache != blockCache){
		if(blockCache->prevUsed == NULL){
			/* ブロックキャッシュリストの先頭の時 */
			dr->block.firstBlockCache = blockCache->nextUsed;
			dr->block.firstBlockCache->prevUsed = NULL;
		}
		else{
			/* ブロックキャッシュリストの最後以外の時 */
			blockCache->prevUsed->nextUsed = blockCache->nextUsed;
			blockCache->nextUsed->prevUsed = blockCache->prevUsed;
		}
		dr->block.lastBlockCache->nextUsed = blockCache;
		blockCache->prevUsed = dr->block.lastBlockCache;
		blockCache->nextUsed = NULL;
		dr->block.lastBlockCache = blockCache;
	}
}

/*****************************************************************************************
関数名	:	SetBlockCache
機能		:	ブロックデータをキャッシュに格納する
入力		:	DicRecord		*dr			: 辞書レコード
			unsigned short		index		: ブロックのインデックス
			BlockCache	*blockCache	: 格納するブロックデータ
出力		:	なし
作成日	:	97.7.30
*****************************************************************************************/
void SetBlockCache(DicRecord *dr,unsigned short index,BlockCache *blockCache)
{
	blockCache->index = index;

	if(dr->block.hashTable[index%(dr->block.hashNum)] != NULL)							//	fix a serious bug <96.9.17>
		dr->block.hashTable[index%(dr->block.hashNum)]->prevBlock = blockCache;
	blockCache->nextBlock = dr->block.hashTable[index%(dr->block.hashNum)];
	blockCache->prevBlock = NULL;
	dr->block.hashTable[index%(dr->block.hashNum)] = blockCache;
}

/*****************************************************************************************
関数名	:	ChangeBlockCacheNum
機能		:	ブロックキャッシュの数を変更する
入力		:	DicRecord		*dr				: 辞書レコード
			unsigned short		newBlockCacheNum	: 新しいブロックキャッシュ数
出力		:	DAErr
作成日	:	97.7.30
*****************************************************************************************/
DAErr ChangeBlockCacheNum(DicRecord *dr,unsigned short newBlockCacheNum)
{
	unsigned short		i;
	BlockCache	*tmpBlockCache;
	DAErr		err;
	
	if(newBlockCacheNum == dr->block.blockCacheNum)return kDANoErr;
	
	if(newBlockCacheNum > dr->block.blockCacheNum){
		/* ブロックキャッシュ数を増やす */
		tmpBlockCache = dr->block.lastBlockCache;
		for(i=dr->block.blockCacheNum;i!=newBlockCacheNum;i++){
			err = NewBlockCache(&(tmpBlockCache->nextUsed));
			if(err != kDANoErr)break;
			tmpBlockCache->nextUsed->prevUsed = tmpBlockCache;
			tmpBlockCache = tmpBlockCache->nextUsed;
		}
		dr->block.lastBlockCache = tmpBlockCache;
	}
	else{
		/* ブロックキャッシュ数を減らす */
		for(i=dr->block.blockCacheNum;i!=newBlockCacheNum;i--){
			GetLastBlockCache(dr,&tmpBlockCache);
			free(tmpBlockCache);
		}
	}
	dr->block.blockCacheNum = i;
	
	err = ChangeHashNum(dr);
	
	return err;
}

/*****************************************************************************************
関数名	:	CalcHacheNum
機能		:	ブロックキャッシュ数からハッシュ数を計算する
入力		:	unsigned short	blockCacheNum			: ブロックキャッシュ数
出力		:	新しいハッシュ数
作成日	:	97.7.30
*****************************************************************************************/
static unsigned short CalcHacheNum(unsigned short blockCacheNum)
{
	unsigned short	newHashNum;

	newHashNum = blockCacheNum / kAveHashListNum;
	if(newHashNum < kInitHashNum)newHashNum = kInitHashNum;

	return newHashNum;
}

/*****************************************************************************************
関数名	:	ChangeHashNum
機能		:	ハッシュサイズを変更する
入力		:	DicRecord		*dr			: 辞書レコード
出力		:	DAErr
作成日	:	97.7.30
*****************************************************************************************/
static DAErr ChangeHashNum(DicRecord *dr)
{
	unsigned short		newHashNum,num,i;
	BlockCache	**tmpTable,*tmpBlockCache;
	DAErr		err = kDANoErr;

	newHashNum = CalcHacheNum(dr->block.blockCacheNum);
	if(newHashNum == dr->block.hashNum)return kDANoErr;

	tmpTable = (BlockCache **)malloc(sizeof(BlockCache *)*newHashNum);
	if(tmpTable == NULL)return kDAMemErr;
	for(i=0;i<newHashNum;i++)
		tmpTable[i] = NULL;
	
	for(i=0;i<dr->block.hashNum;i++){
		while(dr->block.hashTable[i] != NULL){
			tmpBlockCache = dr->block.hashTable[i];
			dr->block.hashTable[i] = tmpBlockCache->nextBlock;
			
			if(tmpBlockCache->index == kNoBlock){
				/* 新しいハッシュテーブルにつなぐ */
				num = tmpBlockCache->index % newHashNum;
				tmpBlockCache->nextBlock = tmpTable[num];
				if(tmpTable[num] != 	NULL)
					tmpTable[num]->prevBlock = tmpBlockCache;
				tmpTable[num] = tmpBlockCache;
			}
			else{
				tmpBlockCache->nextBlock = NULL;
			}
			tmpBlockCache->prevBlock = NULL;
		}
	}
	
	free(dr->block.hashTable);
	dr->block.hashTable = tmpTable;
	dr->block.hashNum = newHashNum;
	
	return kDANoErr;
}

/*****************************************************************************************
関数名	:	NewBlockCache
機能		:	新しいブロックキャッシュを作成する
入力		:	BlockCache **newBlockCache			: 新しいブロック
出力		:	DAErr
作成日	:	97.7.30
*****************************************************************************************/
static DAErr NewBlockCache(BlockCache **newBlockCache)
{
	DAErr	err = kDANoErr;
	
	(*newBlockCache) = (BlockCache *)malloc(sizeof(BlockCache));
	if((*newBlockCache) == NULL)return kDAMemErr;

	(*newBlockCache)->prevUsed	= NULL;
	(*newBlockCache)->nextUsed	= NULL;
	(*newBlockCache)->nextBlock	= NULL;
	(*newBlockCache)->prevBlock	= NULL;
	(*newBlockCache)->index		= kNoBlock;
	(*newBlockCache)->BCT = (Byte *)malloc(kBlockSize);
	if((*newBlockCache)->BCT == NULL){
		free((*newBlockCache));
		(*newBlockCache) = NULL;
		err = kDAMemErr;
	}
	
	return err;
}

/*****************************************************************************************
関数名	:	PutBlock2Cache
機能		:	ブロックを読み込みキャッシュに格納する
入力		:	DicRecord		*dr		: 辞書レコード
			unsigned short		block		: ブロック番号
			unsigned short		index	: ブロックのインデックス
出力		:	DAErr
作成日	:	97.7.30
*****************************************************************************************/
DAErr PutBlock2Cache(DicRecord *dr,unsigned short block,unsigned short index)
{
	BlockCache	*blockCache;
	DAErr		err;
	short		bc_max;

	GetLastBlockCache(dr,&blockCache);
	SetFirstBlockCache(dr,blockCache);

	if ((err = R_BCTF(dr,block,blockCache->BCT)) != kDANoErr){
		blockCache->index = kNoBlock;
		return err;
	}
	SetBlockCache(dr,index,blockCache);

	bc_max = *(short *)blockCache->BCT;
	dr->B_C = (short *)blockCache->BCT;
	dr->TAIL = (blockCache->BCT + (bc_max+1)*2*sizeof(short));

	return kDANoErr;
}