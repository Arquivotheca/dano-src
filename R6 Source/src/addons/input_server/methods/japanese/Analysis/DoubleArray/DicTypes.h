/************************************************************************
ファイル名	:	DicTypes.h

作成日	:	97.9.16
************************************************************************/

#ifndef __DICTIONARY_TYPE_HEADER__
#define __DICTIONARY_TYPE_HEADER__

typedef	unsigned char	Byte;

#define kKeyMax			60				/* キーの最大長						*/
#define kDataMax			500				/* レコードの最大長					*/

#define B_FEP_ERROR_BASE			B_GENERAL_ERROR_BASE + 0xf000

enum{
	kMADNoErr			= B_NO_ERROR,				//	no error
	kMADMemErr			= B_NO_MEMORY,			//	メモリーエラー
	kMADBadKeySizeErr = B_FEP_ERROR_BASE,		//	キーのサイズがおかしい
	kMADBadRecordSizeErr,					//	レコードのサイズがおかしい
	kMADNoKeyErr,						//	指定されたキーが存在しない
	kMADInvalidDicFile,						//	不当なファイルを指定した
	kAlreadyRegistUserDic,			//	既にユーザー辞書は登録されている
	kMADUnknownErr						//	その他のエラー
};

enum{
	kMainDic,
	kUserDic
};

struct DicProperty{
	unsigned short	dicBlockNum;				//	辞書のブロック数
	short			dicKind;
};
typedef struct DicProperty DicProperty;

struct MainRecord{
	unsigned short	keyLength;
	Byte		key[kKeyMax];
	unsigned short	recordSize;
	Byte		recordData[kDataMax];
};
typedef struct MainRecord MainRecord;

#endif //__DICTIONARY_TYPE_HEADER__

