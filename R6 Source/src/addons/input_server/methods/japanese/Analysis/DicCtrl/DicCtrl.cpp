/* DicCtrl.cpp
 *
 *	かな漢字変換辞書管理クラス
 *
 *
 *		作成日: 97/12/11
 */
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "DicCtrl.h"
#include <string.h>
#include <new.h>

#include"DoubleArray.h"
#include"DicData.h"

DicList::DicNode* DicList::dicList_ = NULL;
int32 DicList::entry_size_ = 0;
int32 DicList::entry_all_size_ = 0;
int32 DicList::userDic = -1;
uint32 DicList::fCount = 0;

#define kMaxHinTable	30

struct hinshiRelationTable{
	uint16	dicHinshi;
	KHinshiCode	hinCode;
} hinTable[] = {
	kFutuuMeishi,KH_NOUN_NORMAL,// 普通名詞
	kKoyuMeishi,KH_NOUN_OWN,// 固有名詞
	kTimei,KH_PLACE,// 地名
	kJinmei,KH_NAME,// 人名
	kSetto,KH_PREFIX,// 接頭語
	kSetubi,KH_SUFFIX,// 接尾語
	preJyosuushi,KH_NUMERAL_HEAD,// 冠数詞
	postJyosuushi,KH_NUMERAL_TAIL,// 助数詞
	kRentai,KH_ADNOUN,// 連体詞
	kFukushi,KH_ADVERB,// 副詞
	kSetuzoku,KH_CONJUNCTION,// 接続詞
	kKando,KH_EXCLAMATION,// 感動詞
	kKa5Gokan,KH_VERB_KA5,// カ行五段
	kGa5Gokan,KH_VERB_GA5,// ガ行五段
	kSa5Gokan,KH_VERB_SA5,// サ行五段
	kTa5Gokan,KH_VERB_TA5,// タ行五段
	kNa5Gokan,KH_VERB_NA5,// ナ行五段
	kBa5Gokan,KH_VERB_BA5,// バ行五段
	kMa5Gokan,KH_VERB_MA5,// マ行五段
	kRa5Gokan,KH_VERB_RA5,// ラ行五段
	kWa5Gokan,KH_VERB_WA5,// ワ行五段
	kItidanGokan,KH_VERB_A,// 一段／語幹
	kSahenGokan,KH_VERB_SA_IC,// サ変動詞
	kSahenMeishi,KH_NOUN_SA_IC,// サ変名詞
	kZahenGokan,KH_VERB_ZA_IC,// ザ変動詞
	kZahenMeishi,KH_NOUN_ZA_IC,// ザ変名詞
	kKeiyoGokan,KH_ADJECTIVE,// 形容詞
	kKeidouGokan,KH_ADJECTIVE_VERB,// 形容動詞
	kKeidouMeishi,KH_ADJECTIVE_VERB_NOUN,// 形容動詞名詞
	TANJI,KH_KANJI// 単漢字
};

// KDicRec
//
//	かな漢字変換辞書レコードクラス
//
//		機能: 辞書のレコードの操作
//

// 状態設定
//
//	よみ，表記，品詞コードの値を設定する．
//
status_t KDicRec::SetYomi(const uchar* yomi)
{
	assert( yomi );

	int32 len = strlen( (char*)yomi );
	assert( len );

	if( !yomi_ || yomi_size_ < len ){
		free( yomi_ );
		yomi_ = (uchar*)malloc( len + 1 );
		if( !yomi_ ) return B_NO_MEMORY;
	}

	strcpy( (char*)yomi_, (char*)yomi );
	yomi_size_ = len;

	return B_NO_ERROR;
}

status_t KDicRec::SetYomi(const uchar* yomi,int32 len)
{
	assert( yomi );
	assert( len );

	if( !yomi_ || yomi_size_ < len ){
		free( yomi_ );
		yomi_ = (uchar*)malloc( len + 1 );
		if( !yomi_ ) return B_NO_MEMORY;
	}

	memcpy( (char*)yomi_, (char*)yomi ,len);
	yomi_[len] = '\0';
	yomi_size_ = len;

	return B_NO_ERROR;
}

status_t KDicRec::SetHyoki(const uchar* hyoki)
{
	assert( hyoki );

	int32 len = strlen( (char*)hyoki );
	assert( len );

	if( !hyoki_ || hyoki_size_ < len ){
		free( hyoki_ );
		hyoki_ = (uchar*)malloc( len + 1 );
		if( !hyoki_ )return B_NO_MEMORY;
	}

	strcpy( (char*)hyoki_, (char*)hyoki );
	hyoki_size_ = len;

	return B_NO_ERROR;
}

status_t KDicRec::SetHyoki(const uchar* hyoki,int32 len)
{
	assert( hyoki );
	assert( len );

	if( !hyoki_ || hyoki_size_ < len ){
		free( hyoki_ );
		hyoki_ = (uchar*)malloc( len + 1 );
		if( !hyoki_ )return B_NO_MEMORY;
	}

	memcpy( (char*)hyoki_, (char*)hyoki,len );
	hyoki_[len] = '\0';
	hyoki_size_ = len;

	return B_NO_ERROR;
}

// DicList

DicList::~DicList()
{
	if(fCount == 0 && entry_size_ != 0){
		for(uint32 i=0;i<entry_size_;i++){
			DACloseDictionary(&dicList_[i].dr);
		}
		if( dicList_ ) delete [] dicList_;
		entry_size_ = 0;
		dicList_ = NULL;
	}
}

// KDicCtrl
//
//	かな漢字変換辞書管理クラス
//
//		機能: かな漢字変換辞書の管理，操作
//

// データ追加
//
//	ユーザー辞書にデータを追加する．
//
//	入力: path(辞書ファイルのパス), rec(データ)
//
status_t KDicCtrl::Append(const char* path, KDicRec* rec)
{
	status_t		err;
	MainRecord	mainRec;
	unsigned long	numKey;
	DicData		dicData;
	Category		hinshi;
	uchar		key[kKeyMax];
	uint16		yomiLen;
	
	/* 環境に登録されているか調べる */
	err = CheckRegistUserDic(path);
	if(err != B_NO_ERROR)return err;

	/* 追加データのチェック */
	err = rec->InitCheck();
	if(err != B_NO_ERROR)return err;

	yomiLen = rec->GetYomiSize();
	if(yomiLen > kKeyMax)return kMADBadKeySizeErr;

	ReverseKey(key,rec->GetYomi(),yomiLen);

	err = DAExactMatch(&dicList_[userDic].dr,rec->GetYomiSize(),key,&mainRec,1,&numKey);
	if(err == B_NO_ERROR){
		if(numKey == 0)mainRec.recordSize = 0;
		hinshi = ChangeDicHinshi(rec->GetHinshi());
		err = dicData.AddData(mainRec.recordData,&mainRec.recordSize,rec->GetHyoki(),hinshi,1,0);
	}
	if(err == B_NO_ERROR){
		err = DAUpdateMainRecord(&dicList_[userDic].dr,rec->GetYomiSize(),key,
							mainRec.recordSize,mainRec.recordData);
	}

	return err;
}

// データ削除
//
//	ユーザー辞書からrecに一致するデータを削除する．
//
//	入力: path(辞書ファイルのパス), rec(データ)
//
status_t KDicCtrl::Delete(const char* path, KDicRec* rec)
{
	status_t		err;
	MainRecord	mainRec;
	unsigned long	numKey;
	DicData		dicData;
	Category		hinshi;
	uchar		key[kKeyMax];
	
	/* 環境に登録されているか調べる */
	err = CheckRegistUserDic(path);
	if(err != B_NO_ERROR)return err;

	/* 追加データのチェック */
	err = rec->InitCheck();
	if(err != B_NO_ERROR)return err;

	if(rec->GetYomiSize() > kKeyMax)return kMADBadKeySizeErr;
	ReverseKey(key,rec->GetYomi(),rec->GetYomiSize());

	err = DAExactMatch(&dicList_[userDic].dr,rec->GetYomiSize(),key,&mainRec,1,&numKey);
	if(err == B_NO_ERROR){
		if(numKey == 0)return B_NO_ERROR;
		hinshi = ChangeDicHinshi(rec->GetHinshi());
		err = dicData.DeleteData(mainRec.recordData,&mainRec.recordSize,rec->GetHyoki(),hinshi);
	}
	if(err == B_NO_ERROR){
		if(mainRec.recordSize == 0){
			/* このキーを削除する */
			DADeleteKey(&dicList_[userDic].dr,rec->GetYomiSize(),key);
		}
		else{
			err = DAUpdateMainRecord(&dicList_[userDic].dr,rec->GetYomiSize(),key,
								mainRec.recordSize,mainRec.recordData);
		}
	}

	return err;
}

// データ検索
//
//	辞書からyomiに一致するデータを検索する．
//
//	入力:	path(辞書ファイルのパス),
//			yomi(よみ)
//			rec(検索結果を上書き格納する)
//
/* 修正日	:	98.2.16 */
status_t KDicCtrl::Search(const char* path, const uchar* yomi, KDicRecArray* rec)
{
	status_t		err;
	MainRecord	mainRec;
	unsigned long	numKey;
	uchar		key[kKeyMax];
	uint16		yomiLen;

	/* 環境に登録されているか調べる */
	err = CheckRegistUserDic(path);
	if(err != B_NO_ERROR)return err;

	yomiLen = strlen((char*)yomi);
	if(yomiLen > kKeyMax)return kMADBadKeySizeErr;
	ReverseKey(key,yomi,yomiLen);

	// よみからデータを検索する
	err = DAExactMatch(&dicList_[userDic].dr,yomiLen,key,&mainRec,1,&numKey);
	if(err != B_NO_ERROR)return err;

	// 検索結果をKDicRecArrayに乗せる
	if( rec->recs_ ) delete [] rec->recs_;

	if(numKey == 0){
		/* 辞書に登録されていない */
		rec->size_ = 0;
		rec->recs_ = NULL;
	}
	else{
		DicData		dicData;
		Byte*		pos;
		uint32		count;
		HinshiNumType	hinNum,i;
		HyokiNumType	hyokiNum,j;
		HindoType		hindo;
		WordFlagType	wordFlag;
		HyokiSizeType	hyokiSize;
		uchar*		hyoki;
		Category		hinshi;

		rec->size_ = dicData.GetWordNum(mainRec.recordData);

#if _SUPPORTS_EXCEPTION_HANDLING
		try{
#endif
			rec->recs_ = new KDicRec[ rec->size_ ];
#if _SUPPORTS_EXCEPTION_HANDLING
		}
		catch( bad_alloc ){
			rec->recs_ = NULL;
			rec->size_ = 0;
			return B_NO_MEMORY;
		}
#endif

		count = 0;
		pos = mainRec.recordData;
	
		hinNum = dicData.GetHinshiNum(&pos);
		for(i=0;i<hinNum;i++){
			hinshi = dicData.GetHinshiCode(pos);
			pos += sizeof(Category);	
			hyokiNum = dicData.GetHyokiNum(pos);
			pos += sizeof(HyokiNumType);
	
			for(j=0;j<hyokiNum;j++){
				dicData.GetHyoki(&pos,&hindo,&wordFlag,&hyokiSize,&hyoki);
				/* 検索結果のデータ設定 */
				if(wordFlag != kLearnedWord){
					/* 自動学習された語以外を格納する */
					err = rec->recs_[ count ].SetData( yomi,yomiLen, hyoki,hyokiSize, ChangeHinshi(hinshi));
					if(err != B_NO_ERROR)return err;
					count++;
				}
			}
		}
	}

	return B_NO_ERROR;
}

// ユーザ辞書作成
status_t KDicCtrl::CreateDic(const char* path)
{
	status_t		err;
	DicProperty	dicProp;

	dicProp.dicBlockNum = 1000;
	dicProp.dicKind = kUserDic;

	err = DACreateNewDictionary(path,&dicProp);
	
	return err;
}

// 辞書登録
//
//	pathで指定した辞書をかな漢字変換で使用するように設定する．
//
status_t KDicCtrl::RegistDic(const char* path)
{
	BEntry		e;
	DicRecord		dr;
	DicHeader1	head;

	status_t ret = e.SetTo( path );
	if( ret != B_NO_ERROR ) return ret;

	/* 既に登録されているかチェックする */
	for(int32 i = 0; i < entry_size_; i++ ){
		if( dicList_[ i ].entry_ == e ){
			/* 登録されていた */
			return B_NO_ERROR;
		}
	}

	/* ファイルのチェック */
	ret = dr.fp.SetTo(&e,B_READ_ONLY);
	if(ret != B_NO_ERROR ) return ret;

	ret = dr.fp.Read(&head,sizeof(DicHeader1));
	if(ret < B_NO_ERROR)return ret;
	if(ret != sizeof(DicHeader1))return kMADInvalidDicFile;
	
	if(memcmp(head.sig,kDicSignature,8)!=0)return kMADInvalidDicFile;
	dr.fp.Unset();

	head.dicKind = B_LENDIAN_TO_HOST_INT16(head.dicKind);					//add for Intel
	if(head.dicKind == kUserDic){
		if(userDic != -1){
			/* 既にユーザー辞書は登録されている */
			return kAlreadyRegistUserDic;
		}
		/* 指定された辞書はユーザー辞書 */
		ret = dr.fp.SetTo(&e,B_READ_WRITE);
	}
	else{
		/* 指定された辞書は基本辞書 */
		ret = dr.fp.SetTo(&e,B_READ_ONLY);
	}
	if(ret != B_NO_ERROR ) return ret;

	ret = DAOpenDictionary(&dr);
	if(ret != B_NO_ERROR ) return ret;
	
	if( !dicList_  || entry_all_size_ == entry_size_ ){
#if _SUPPORTS_EXCEPTION_HANDLING
		try{
#endif
			DicNode	*list = new DicNode[ entry_all_size_ + 5 ];
			for( int32 i = 0; i < entry_all_size_; i++ ) list[ i ] = dicList_[ i ];
			delete [] dicList_;
			dicList_ = list;
			entry_all_size_ += 5;
#if _SUPPORTS_EXCEPTION_HANDLING
		}
		catch( bad_alloc ){
			return B_NO_MEMORY;
		}
#endif
	}

	dicList_[entry_size_].entry_ = e;
	dicList_[entry_size_].dr = dr;

	if(head.dicKind == kUserDic)userDic = entry_size_;

	entry_size_ ++;

	return B_NO_ERROR;
}

// 辞書登録チェック
//
//	pathで指定した辞書をかな漢字変換で使用するように
//	設定されているかを得る．
//
bool KDicCtrl::IsRegist(const char* path)
{
	BEntry e;
	status_t ret = e.SetTo( path );
	if( ret != B_NO_ERROR ) return ret;

	for( int32 i = 0; i < entry_size_; i++ ){
		if( dicList_[i].entry_ == e ) return true;
	}

	return false;
}

// 辞書登録解除
//
//	pathで指定した辞書をかな漢字変換で使用しないように設定する．
//
//	修正日	:	98.1.2
status_t KDicCtrl::UnRegist(const char* path)
{
	BEntry e;
	status_t ret = e.SetTo( path );
	if( ret != B_NO_ERROR ) return ret;

	for( int32 i = 0; i < entry_size_; i++ ){
		if( dicList_[i].entry_ == e ){
			ret = DACloseDictionary(&dicList_[i].dr);
			if(ret != B_NO_ERROR)return ret;
			for( ;i < entry_size_ - 1; i++ ){
				dicList_[i].entry_ = dicList_[i+1].entry_;
				dicList_[i].dr = dicList_[i+1].dr;
			}
			entry_size_ --;
			if(i == userDic){
				/* 指定された辞書はユーザー辞書 */
				userDic = -1;
			}
			else if(userDic > i)
				userDic --;
			return B_NO_ERROR;
		}
	}

	return B_NO_ERROR;		// ファイルが登録されてなかった
}

/***************************************************************
関数名	:	KDicCtrl::CheckRegistUserDic
機能		:	ファイルが登録されているか調べ、
			登録されていたら、それがユーザー辞書かどうか調べる
入力		:	const char*	path		: 調べるファイル
出力		:	ユーザー辞書が登録されていれば B_NO_ERROR
作成日	:	97.12.23
***************************************************************/
status_t KDicCtrl::CheckRegistUserDic(const char* path)
{
	BEntry	e;
	int32	i;
	
	status_t ret = e.SetTo( path );
	if( ret != B_NO_ERROR ) return ret;
	/* 環境に登録されているか調べる */
	for(i = 0; i < entry_size_; i++ ){
		if( dicList_[ i ].entry_ == e ){
			/* 登録されていた */
			break;
		}
	}
	if(i == entry_size_ )return kMADInvalidDicFile;
	if(i != userDic)return kMADInvalidDicFile;

	return B_NO_ERROR;
}

/***************************************************************
関数名	:	KDicCtrl::ChangeHinshiCode
機能		:	品詞コードを辞書用の品詞コードに変換する
入力		:	KHinshiCode	hinCode		: 品詞コード
出力		:	変換された品詞コード
作成日	:	97.12.23
***************************************************************/
Category KDicCtrl::ChangeDicHinshi(KHinshiCode hinCode)
{
	uint16		tmp;

	for(int i=0;i<kMaxHinTable;i++){
		if(hinTable[i].hinCode == hinCode){
			tmp = hinTable[i].dicHinshi;
			break;
		}
	}
	return HINSHI(tmp,tmp);
}

/***************************************************************
関数名	:	KDicCtrl::ChangeHinshi
機能		:	辞書用の品詞コードを品詞コードに変換する
入力		:	Category	dicHinCode		: 辞書用品詞コード
出力		:	変換された品詞コード
作成日	:	97.12.23
***************************************************************/
KHinshiCode KDicCtrl::ChangeHinshi(Category dicHinCode)
{
	uint16		preHinshi;
	
	preHinshi = PRE_HINSHI(dicHinCode);

	for(int i=0;i<kMaxHinTable;i++){
		if(hinTable[i].dicHinshi == preHinshi)
			return hinTable[i].hinCode;
	}
	return KH_NOUN_NORMAL;
}

/***************************************************************
関数名	:	KDicCtrl::ReverseKey
機能		:	キーを逆順にする
入力		:	uchar	*revKey		: 逆順にしたキー(out)
			uchar	*key			: 逆順にするキー
			int		yomiLen		: keyの長さ
出力		:	なし
作成日	:	98.2.13
***************************************************************/
void KDicCtrl::ReverseKey(uchar *revKey,const uchar *key,int yomiLen)
{
	for(uint16 i=0;i<yomiLen;i++)revKey[i] = key[yomiLen-i-1];
	revKey[yomiLen]='\0';
}

/*--- end of DicCtrl.cpp ---*/
