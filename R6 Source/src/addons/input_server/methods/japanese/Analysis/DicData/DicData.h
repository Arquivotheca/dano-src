/************************************************************************
ファイル名	:	DicData.h

作成日	:	97.12.17
************************************************************************/

#ifndef __DICDATA_HEADER__
#define __DICDATA_HEADER__

#include "hinshi.h"
#include "DicTypes.h"

typedef short		HinshiNumType;		//	品詞数を表すタイプ
typedef uchar		HyokiNumType;			//	表記数を表すタイプ
typedef uint16	HindoType;			//	頻度を表すタイプ
typedef uchar		WordFlagType;			//	表記に対するフラグを表すタイプ
typedef uchar		HyokiSizeType;		//	表記のサイズを表すタイプ

#define	kHyokiHeaderSize	(sizeof(HindoType)+sizeof(WordFlagType)+sizeof(HyokiSizeType))
#define	kHinshiHeaderSize	(sizeof(Category)+sizeof(HyokiNumType))
#define	kDataHeaderSize	(sizeof(HinshiNumType))

#define	AllHyokiDataSize(hyokiSize)	(kHyokiHeaderSize+(hyokiSize))

#define	kMaxHyokiNum		255
#define	kMaxHyokiSize		60

#define	kMaxHindo			0x7fff

#define	kLearnedWord		(uchar)0x01

class DicData{
public:
//	DicData(void)	{}
	status_t AddData(Byte *data,uint16 *dataSize,const uchar *hyoki,Category hinshi,HindoType hindo,WordFlagType wordFlag);
	void GetHyoki(Byte **pos,HindoType *hindo,WordFlagType *wordFlag,HyokiSizeType *hyokiSize,uchar **hyoki);
	status_t DeleteData(Byte *data,uint16 *dataSize,const uchar *hyoki,Category hinshi);

	uint32 GetWordNum(Byte *data);
	HinshiNumType GetHinshiNum(Byte **pos);
	Category GetHinshiCode(Byte *pos);
	HyokiNumType GetHyokiNum(Byte *pos);
private:
	void SetHinshiNum(Byte *pos,HinshiNumType hinNum);
	void SetHinshiCode(Byte **pos,Category hinshi,HyokiNumType hyokiNum);
	void SetHyokiNum(Byte **pos,HyokiNumType hyokiNum);
	void SetHyoki(Byte *pos,HindoType hindo,WordFlagType wordFlag,HyokiSizeType hyokiSize,const uchar *hyoki);
	bool SearchHinshi(Byte **pos,HinshiNumType hinNum,Category hinshi,uint16 *size);
	uint16 SkipHinshi(Byte **pos);
	uint16 SkipHyoki(Byte **pos);
	HyokiSizeType GetHyokiSize(Byte *pos);
};

#endif //__DICDATA_HEADER__
