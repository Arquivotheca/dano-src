/******************************************
ファイル名	:	AccessTable.h

作成日	:	97.12.3
******************************************/
#ifndef __ACCESS_TABLE_H__
#define __ACCESS_TABLE_H__

#include <SupportDefs.h>

#include"HinshiTableArray.h"
#include"HinshiStruct.h"
#include"FieldStruct.h"

#define	kMaxATSize	1024

class AccessTable{
public:
	uint16			oldTextLen;
//	HinshiStruct				hinshiStruct;//test
//	FieldStruct				fieldStruct;

	AccessTable(void);
	~AccessTable(void);

	void SetWord(int pos,int length,unsigned char *contents,Category hinshiCode,short evaluation);
	void Free(int startOffset,int endOffset);
	void AllFree(void);
	void SetNanashi(int pos,int length,unsigned char *contents);
	void Shift(uint16 pos);
	bool CheckSuusi(int pos,int length);

	HinshiTableArray& operator[](long num);

private:
	HinshiTableArray	*hTableArray;
	HinshiStruct				hinshiStruct;
	FieldStruct				fieldStruct;

	void SetHinshiAndField(int pos,int length,unsigned char *contents,Category hinshiCode,short evaluation);
	void SetField(Hinshi* hinshi,unsigned char *contents,short evaluation);
	void FreeHinshi(int pos,int length);
	void FreeField(Field *field,bool segmentFlag);
	void SearchHinshi(Category hinshiCode,Hinshi **retHinshi);
	void CheckConnection(int pos,int objLen,int length,Hinshi *targetHinshi);
	void SetHinshi(int pos,int length,Hinshi *targetHinshi);
	void MakeSegment(Hinshi *newHinshi,Hinshi *preHinshi,Hinshi *postHinshi);
	void Move(uint16 src,uint16 target);
};

#endif //__ACCESS_TABLE_H__
