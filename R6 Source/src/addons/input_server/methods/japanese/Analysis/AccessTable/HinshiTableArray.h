/******************************************
ファイル名	:	HinshiTableArray.h

作成日	:	97.12.2
******************************************/

#ifndef __HINSHI_TABLE_ARRAY_H__
#define __HINSHI_TABLE_ARRAY_H__

#include"HinshiStruct.h"

#define HINSHIDEBUG

#define kOneAccessTable	15

class HinshiTable{
public:
	Hinshi		*firstHinshi;	
	bool			contentWFlag;		// 自立語を含んでいるときtrue
	bool			segmentFlag;		// 文節を含んでいるときtrue

	HinshiTable(void)	{firstHinshi=NULL;contentWFlag=false;}
};

class HinshiTableArray{
public:

	short		nowTable;			// 現在，使われている table の数

	HinshiTableArray(void);
	~HinshiTableArray(void);
	
	HinshiTable& operator[](long num);

private:
	HinshiTable	*hTable;		// 品詞テーブル

	short		maxTable;		// 確保されている table の数
};

#endif //__HINSHI_TABLE_ARRAY_H__
