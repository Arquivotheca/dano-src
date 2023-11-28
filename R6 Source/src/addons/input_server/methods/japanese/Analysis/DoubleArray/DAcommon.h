/************************************************************************
ファイル名	:	DACommon.h

作成日	:	97.7.27
************************************************************************/

#ifndef __DA_COMMON_HEADER__
#define __DA_COMMON_HEADER__

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include"DoubleArray.h"
#include"DAcache.h"

#define kDicVers			0x00010002

#define kBlockSize			4096			/* １ブロックのサイズ				*/
#define kBCSize			kBlockSize/2		/* ダブル配列のメモリー上の要素数		*/
#define kTailSize			kBlockSize		/* TAILのメモリー上の要素数	*/

#define kNoBlock			0xffff			/* ブロックデータを読み込んで
									   	　　いないときの same_c の値			*/
#define kWriteAllTable		kNoBlock			/* すべてのテーブルを書き込む時に指定する	*/
#define kInitBlockNum		2				/* 初期ブロック数					*/
#define kMinBlockNum		kInitBlockNum		/* 最小ブロック数					*/
#define kMaxBlockNum		0xfffe			/* 最大ブロック数					*/

#define kMinCode			1				/* 最小の内部表現値					*/
#define kMaxCode			257				/* 最大の内部表現値					*/
#define kEndCode			1				/* 端記号の内部表現値				*/
#define kNoUseCode			(-257)			/* 使用していない内部表現値			*/	//old 0 new -257
#define kMinBaseValue		(kNoUseCode+1)	/* ベースの値の最小値				*/	//old 1 new -256
#define kTailOffset			(-kNoUseCode)		/* base値とtail上でのデータ開始位置との差	*/	//old 0 new 257

#define CODE(x)			((short)((Byte)x)+2)/* 文字コードを内部表現値に変換する		*/
#define DECODE(x)			(Byte)(x-2)		/* 内部表現値を文字コードに変換する		*/

#define kUpperByte			(kKeyMax+1)

#define kDicMask			0x7b				/* 辞書データのマスク */

/* UbcInsert の返値 */
enum{
	kAddFail,							/* 追加失敗						*/
	kKeyAddSuccess,					/* キーの追加成功					*/
	kDataAddSuccess					/* データの追加成功					*/
};

/* 辞書のパーミッション */
enum{
	kReadWritePerm,					/* Read Write 可能	*/
	kReadOnlyPerm						/* Read only		*/
};

#define	kDicSignature	"FUKEMORI"

struct DicHeader1{
	unsigned long		vers;				// 辞書のバージョン
	uchar			sig[8];			// 辞書のシグネチャ
	short			dicKind;			// 辞書の種別
};

struct DicHeader{
//	unsigned long		vers;				// 辞書のバージョン
	unsigned short		dicPerm;				// 辞書のパーミッション
	unsigned short		maxBlock;				// 使用できる最大のブロック数
	unsigned short		TblNum;				// 現在使用中のブロック数
	unsigned short		cacheBlockNum;		// キャッシュブロックサイズ
	long					tablePos;				// ブロック管理用テーブルの書き込まれている位置
};
typedef struct DicHeader DicHeader;

struct TBCS{			/* ブロック管理用テーブル構造体				*/
	unsigned short	index;		/* ブロックのインデックス					*/
	short	bcsize;     		/* ダブル配列の大きさ					*/
	unsigned short	keynum;		/* ブロックに入っているキーの数				*/
	long		bcaddr;		/* ブロックの先頭アドレス					*/
	Byte		First[kUpperByte];	/* ブロックの最初のキー配列				*/
};

typedef	BFile	DicRef;

class DicRecord{
	public:
//	unsigned long			vers;				/* 辞書のバージョン								*/
	unsigned short			dicPerm;				/* 辞書のパーミッション							*/
//	long				DaPos;				/* データ部の始まる位置							*/
	unsigned short			refCount;				/* リファレンスカウント							*/
	DicRef			fp;					/* ファイルポインター							*/
	void				*dicPtr;				/* 辞書を指すポインター							*/

	unsigned short			maxBlock;				/* 使用できる最大のブロック数						*/
	unsigned short			TblNum;				/* 現在使用中のブロック数							*/
	long						tablePos;				/* ブロック管理用テーブルの書き込まれている位置	*/

	short			*B_C;				/* 現在使用中のBASE-CHECKを指すポインタ				*/
	Byte				*TAIL;				/* 現在使用中のSingle Stringを指すポインタ				*/

	short			BC_MAX;				/* ダブル配列の使われている最大のindex				*/
	short			TA_POS;				/* 未使用の TAILの位置					*/

	struct TBCS		*TABLE;				/* 各ブロックのデータ 							*/
//	Byte				*First;				/* ブロック配列									*/

	BlockTable		block;				/* ブロックデータ								*/

	DicRecord(void){refCount = 0;}
	~DicRecord(void){}
};

struct Dmp{				/* ダブル配列ダンプ用ノード情報			*/
	short		s;				/* 状態番号 						*/
	short		pos;				/* スタックポインタ					*/
	Byte			str[kKeyMax];		/* ダンプバイト列 					*/
	short		str_len;			/* ダンプバイト列の長さ				*/
	short		stack[kKeyMax];	/* 状態番号スタック					*/
	Byte			*cont;			/* レコード						*/
	DataType		cont_size;			/* cont のサイズ					*/
	short		c;				/* ダンプを開始する文字コードの最小値 	*/
};
typedef struct Dmp Dmp;

/* DoubleArray.c */

extern DAErr AllocateTables(DicRecord *dr);
extern void FreeDicRecord(DicRecord *dr);
extern DAErr AllocateBCTAIL(DicRecord *dr);
extern void InitDicRecord(DicRecord *dr);
extern void INI_BCT(DicRecord *dr);
extern void INI_BCTWithValue(DicRecord *dr);
extern Boolean W_BASE(DicRecord *dr,short pos, short value);
extern Boolean W_CHECK(DicRecord *dr,short pos, short value);
extern DAErr R_BCTF(DicRecord *dr,unsigned short i,Byte *data);
extern DAErr W_BCTF(DicRecord *dr,unsigned short i,Boolean writeTable);
extern unsigned short BCODE(DicRecord *dr,const Byte *key,short len);
extern DAErr ChangeBlock(DicRecord *dr,unsigned short block,Boolean isSearch);
extern void R_STR(short pos,Byte **CONT, DataType *cont_size,Byte *S_TEMP,short *s_temp_size,Byte *TAIL_Ptr);
extern DAErr W_TABLE(DicRecord *dr,unsigned short num);
extern DAErr SetUpDic(DicRecord *dr);
extern void DeleteState(DicRecord *dr,short s);
extern Boolean TailGab(DicRecord *dr,Byte *buf);
extern DAErr R_Add_BCTF(DicRecord *dr,unsigned short block);

/* DAFile.c */

extern DAErr DicWriteData(DicRecord *dr,long *dicPos,const void *buffPtr,long size);
extern DAErr DicReadData(DicRecord *dr,long *dicPos,void *buffPtr,long size);
extern DAErr DicSetPosition(DicRecord *dr,long *dicPos,long posOff);
extern DAErr DicGetPosition(DicRecord *dr,long dicPos,long *posOff);

extern DAErr DicGetSize(DicRecord *dr,long *size);
extern DAErr DicSetSize(DicRecord *dr,long size);

/* DACreate.c */

extern DAErr WriteHeader(DicRecord *dr,unsigned short dicPerm,unsigned short maxBlock,unsigned short tblNum,unsigned short cacheBlockNum,long tablePos,long *dicPos);

/* DADicTable.c */

extern DAErr SetDicRecord2DicTable(unsigned short dicID,DicRecord *dr);
extern DAErr GetDicRecord2DicTable(unsigned short dicID,DicRecord **dr);

/* DACache.c */

extern DAErr InitBlockCache(DicRecord *dr);
extern void DisposeAllBlockCache(DicRecord *dr);
extern Boolean SearchBlockCache(DicRecord *dr,unsigned short index,BlockCache **blockCache);
extern void MoveFirstBlcokCache(DicRecord *dr,BlockCache *blockCache);
extern void SetFirstBlockCache(DicRecord *dr,BlockCache *blockCache);
extern void GetLastBlockCache(DicRecord *dr,BlockCache **blockCache);
extern void SetLastBlockCache(DicRecord *dr,BlockCache *blockCache);
extern void SetBlockCache(DicRecord *dr,unsigned short index,BlockCache *blockCache);
extern DAErr ChangeBlockCacheNum(DicRecord *dr,unsigned short newBlockCacheNum);
extern DAErr PutBlock2Cache(DicRecord *dr,unsigned short block,unsigned short index);

/* DADump.c */
extern Boolean PrintBC(Dmp *node,short *BCBuff,Byte *TailBuff);

/* DAAdd.c */

extern short UbcInsert(DicRecord *dr,const Byte *key,short key_len,const Byte *cont,DataType cont_size);

#endif //__DA_COMMON_HEADER__