/************************************************************************
ファイル名	:	DoubleArray.h

作成日	:	97.7.27
************************************************************************/

#ifndef __DOUBLE_ARRAY_HEADER__
#define __DOUBLE_ARRAY_HEADER__

#include"DicTypes.h"


class DicRecord;
typedef	status_t	DAErr;

#define	kDANoErr	 			kMADNoErr			//	no error
#define	kDAMemErr			kMADMemErr			//	メモリーエラー
#define	kDABadKeySizeErr		kMADBadKeySizeErr		//	キーのサイズがおかしい
#define	kDABadRecordSizeErr	kMADBadRecordSizeErr	//	レコードのサイズがおかしい
#define	kDABlockFullErr		kMADUnknownErr		//	辞書ブロックがいっぱいでこれ以上追加できない
#define	kDANoKeyErr			kMADNoKeyErr			//	指定されたキーが存在しない
#define	kDAInvalidNodeErr		kMADUnknownErr		//	ノード情報の値がおかしい
#define	kDAInvalidVersion		kMADInvalidDicFile		//	辞書のバージョンがおかしい

#ifdef __cplusplus
extern "C" {
#endif

extern DAErr DACreateNewDictionary(const char *fullPathName,DicProperty *dicPropertyData);
extern DAErr DAOpenDictionary(DicRecord* dr);
extern DAErr DACloseDictionary(DicRecord* dr);
extern DAErr DAExactMatch(DicRecord* dr,unsigned short keyLength,const Byte *key,
							MainRecord mainRecordData[],unsigned long numInfo,unsigned long *numKey);
extern DAErr DAUpdateMainRecord(DicRecord* dr,unsigned short keyLength,const Byte *key,
							unsigned short recordSize,const Byte *recordData);
extern DAErr DAForwardMatch(DicRecord* dr,unsigned short keyLength,const Byte *keyData,MainRecord *mainRecordData);
extern DAErr DATrieMatch(DicRecord* dr,unsigned short keyLength,const Byte *key,
							MainRecord mainRecordData[],unsigned long numInfo,unsigned long *numKey);
extern DAErr DADeleteKey(DicRecord* dr,unsigned short keyLength,const Byte *key);

extern DAErr DALongestMatch(unsigned short dicID,unsigned short keyLength,const Byte *key,
							MainRecord mainRecordData[],unsigned long numInfo,unsigned long *numKey);
extern DAErr DAPrefixMatch(unsigned short dicID,unsigned short keyLength,const Byte *key,
							MainRecord mainRecordData[],unsigned long numInfo,unsigned long *numKey);
extern DAErr DABackwardMatch(unsigned short dicID,unsigned short keyLength,const Byte *keyData,MainRecord *mainRecordData);
extern DAErr DACompactDictionary(DicRecord* dr,const char *fullPathName);

#ifdef __cplusplus
}
#endif

#endif //__DOUBLE_ARRAY_HEADER__
