/************************************************************************
ファイル名	:	DACache.h

作成日	:	97.7.27
************************************************************************/

#ifndef __DA_CACHE_HEADER__
#define __DA_CACHE_HEADER__

#include"DAtypes.h"

#define kInitBlockCacheNum		2		/* ブロックキャッシュ数の初期値（最低でも２にする）	*/ 
#define kInitHashNum			2		/* ハッシュテーブル数の初期値（最低でも２にする）	*/ 
#define kAveHashListNum			20		/* ハッシュにつなげるリストの平均数				*/

struct BlockCache
{
	unsigned short			index;			/*	ブロックのインデックス					*/
	Byte				*BCT;			/*	BASE,CHECK,TAILのデータ				 	*/
	struct BlockCache	*nextBlock;		/*	ハッシュリスト上の次のブロックを指すポインタ 	*/
	struct BlockCache	*prevBlock;		/*	ハッシュリスト上の前のブロックを指すポインタ 	*/
	struct BlockCache	*nextUsed;		/*	次のブロックキャッシュリストを指すポインタ 	*/
	struct BlockCache	*prevUsed;		/*	前のブロックキャッシュリストを指すポインタ 	*/
};
typedef struct BlockCache BlockCache;

struct BlockTable
{
	unsigned short			same_c;			/* 変更用のブロック番号						*/
	Boolean			writeFlg;			/* 変更用のブロックのデータが変更されたときtrue		*/
	short			*addB_C;			/* 変更用のBASE,CHECK						*/
	Byte				*addTAIL;			/* 変更用のTAIL							*/
	BlockCache		*addBlockPtr;		/* 変更用ブロックと同じ検索用ブロックを指すポインタ　*/
	unsigned short			hashNum;			/* ハッシュ数								*/
	BlockCache		**hashTable;		/* ハッシュのテーブル						*/
	unsigned short			blockCacheNum;	/* ブロックキャッシュ(検索用ブロック)数			*/
	BlockCache		*firstBlockCache;	/* ブロックキャッシュリストの先頭を指すポインタ		*/
	BlockCache		*lastBlockCache;	/* ブロックキャッシュリストの最後を指すポインタ		*/
};
typedef struct BlockTable BlockTable;

#endif	//__DA_CACHE_HEADER__