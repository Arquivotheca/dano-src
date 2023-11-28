// ExtraDicCtrl.h
//
//	かな漢字変換辞書検索クラス
//
//
//		作成日: 97/12/25
//
#ifndef __EXTRADICCTRL_H__
#define __EXTRADICCTRL_H__

#include <assert.h>
#include "DAcommon.h"
#include "hinshi.h"
#include "DicData.h"
#include "DicCtrl.h"

// KMainDicCtrl
//
//	かな漢字変換辞書管理クラス
//
//		機能: かな漢字変換辞書の管理，操作
//
class KExtraDicCtrl:protected KDicCtrl{//DicList?
public:
	KExtraDicCtrl(){fRec = NULL;maxRecNum = 0;}
	~KExtraDicCtrl(){if(maxRecNum!=0)free(fRec);}

	status_t TrieSearch(const uchar* yomi,uint16 offset, MainRecord* &rec,uint32* num);

	status_t Append(const uchar* yomi, uint16 yomiLen,uchar* hyoki, HindoType hindo, Category hinshi);
	
private:
	MainRecord*	fRec;
	uint32		maxRecNum;
};

#endif	//  __EXTRADICCTRL_H__
/*--- end of MainDicCtrl.h ---*/
