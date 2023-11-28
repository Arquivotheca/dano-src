/************************************************************************
ファイル名	:	DicAccess.h

作成日	:	97.12.21
************************************************************************/
#ifndef __DIC_ACCESS_H__
#define __DIC_ACCESS_H__

#include "AccessTable.h"
#include "ExtraDicCtrl.h"
#include "NumberHyoki.h"

typedef long int32;
typedef int32 status_t;
typedef unsigned char uchar;

class DicAccess{
public:
	AccessTable	at;
	KExtraDicCtrl	eDicCtrl;
	NumberHyoki	nh;

	DicAccess(void);
	~DicAccess(void);

	status_t MakeAccessTable(const uchar *analyzedText,const uchar* textWord,uint16 textWordNum,uint16 textWordOffset=0);
	status_t LearnFrequency(const uchar* yomi,Category category,Field* field,HindoType maxHindo);
	status_t LearnFrequency(const uchar* yomi,short yomiLen,Category category,Field* field,HindoType maxHindo);
	status_t LearnFrequency(const uchar* yomi,Hinshi* hinshi,uint16 num);
	void SetSuusi(const uchar *analyzedText,int pos,int length);
};

#endif //__DIC_ACCESS_H__