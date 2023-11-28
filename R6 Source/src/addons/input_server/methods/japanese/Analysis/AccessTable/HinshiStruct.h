/******************************************
ファイル名	:	HinshiStructManager.h

作成日	:	97.12.3
******************************************/
#ifndef __HINSHI_STRUCT_H__
#define __HINSHI_STRUCT_H__

#include<assert.h>
#include"FieldStruct.h"
#include"hinshi.h"

#define kOneHinshiPage	100		// １ページにつき動的に確保する品詞の数

struct Hinshi {
	Hinshi		*nextHinshi;
	Category		category;				// 品詞
	int			cost;				// コスト
	bool			contentWFlag;			// 自立語の時true
	bool			segmentFlag;			// 文節の時true
	short		length;				// この区切りの長さ（バイト数）
	Field		*firstField;			// この区切りに属する形態素ノードのリスト
};

struct HinshiPage {
	HinshiPage	*next;				// 次ページへのポインタ
	Hinshi		cell[1];				// このページにおけるヘッド
};

class HinshiStruct{
public:
	HinshiPage	*pageHead;
	Hinshi		*freeHinshi;
	
	HinshiStruct(void);
	~HinshiStruct(void);
	
	void NewHinshiPage(void);
	Hinshi* GetFreeHinshi(void);
//	Hinshi& SetHinshi(Hinshi *hinshi,Category category);
	void SetFreeHinshi(Hinshi* hinshi);
};




#endif //__HINSHI_STRUCT_H__