// DicCtrl.h
//
//	かな漢字変換辞書管理クラス
//
//
//		作成日: 97/11/28
//
#ifndef __DICCTRL_H__
#define __DICCTRL_H__

#include <assert.h>
#include <stdlib.h>
#include "DAcommon.h"
#include "hinshi.h"

// 品詞コード
enum KHinshiCode {
	KH_NOUN_NORMAL = 1,		// 普通名詞        （ペン，パソコン）
	KH_NOUN_OWN,			// 固有名詞        （カシオペア座，徳島大学）
	KH_PLACE,				// 地名    （アメリカ，日本）
	KH_NAME,				// 人名    （森田，順一）
	KH_PREFIX,				// 接頭語  （再，不）
	KH_SUFFIX,				// 接尾語  （的，化）
	KH_NUMERAL_HEAD,		// 冠数詞  （約，第）
	KH_NUMERAL_TAIL,		// 助数詞      （メートル，匹）
	KH_ADNOUN,				// 連体詞  （この，ちいさな）
	KH_ADVERB,				// 副詞    （ゆっくり，かなり）
	KH_CONJUNCTION,			// 接続詞  （そして，また）
	KH_EXCLAMATION,			// 感動詞  （えっ，はい）
	KH_VERB_KA5,			// カ行五段        （浮+く，咲+く）+の後ろは活用語尾で，+の前が語幹．辞書には語幹を登録する
	KH_VERB_GA5,			// ガ行五段        （泳+ぐ，あお+ぐ）
	KH_VERB_SA5,			// サ行五段        （貸+す，照ら+す）
	KH_VERB_TA5,			// タ行五段        （立+つ，待+つ）
	KH_VERB_NA5,			// ナ行五段        （死+ぬ）
	KH_VERB_BA5,			// バ行五段        （遊+ぶ，滅+ぶ）
	KH_VERB_MA5,			// マ行五段        （噛+む，はげ+む）
	KH_VERB_RA5,			// ラ行五段        （送+る，走+る）
	KH_VERB_WA5,			// ワ行五段        （会+う，買+う）
	KH_VERB_A,				// 一段／語幹      （着+る，混ぜ+る）
	KH_VERB_SA_IC,			// サ変動詞        （熱+する，徹+する）
	KH_NOUN_SA_IC,			// サ変名詞        （検索+する，運動+する）サ変動詞にも普通名詞にもなる単語
	KH_VERB_ZA_IC,			// ザ変動詞        （命+ずる，存+ずる）
	KH_NOUN_ZA_IC,			// ザ変名詞        （禁+ずる，談+ずる）ザ変動詞にも普通名詞にもなる単語
	KH_ADJECTIVE,			// 形容詞  （美し+い，大き+い）
	KH_ADJECTIVE_VERB,		// 形容動詞        （適切+だ，きれい+だ）
	KH_ADJECTIVE_VERB_NOUN,	// 形容動詞名詞    （安全+だ，丈夫+だ）形容動詞にも普通名詞にもなる単語
	KH_KANJI				// 単漢字
};

class KDicRecArray;
class KDicCtrl;

// KDicRec
//
//	かな漢字変換辞書レコードクラス
//
//		機能: 辞書のレコードの操作
//
class KDicRec {
public:

	// コンストラクタ
	KDicRec()
		: yomi_( 0 ), yomi_size_( 0 ), hyoki_( 0 ), hyoki_size_( 0 ),
		  hinshi_( KH_NOUN_NORMAL )
	{}

	KDicRec(const uchar* yomi, const uchar* hyoki, KHinshiCode hinshi)
		: yomi_( 0 ), yomi_size_( 0 ), hyoki_( 0 ), hyoki_size_( 0 ),
		  hinshi_( KH_NOUN_NORMAL )
	{
		SetData( yomi, hyoki, hinshi );
	}

	KDicRec(const KDicRec& other)
		: yomi_( 0 ), yomi_size_( 0 ), hyoki_( 0 ), hyoki_size_( 0 ),
		  hinshi_( KH_NOUN_NORMAL )
	{
		SetData( other.GetYomi(), other.GetHyoki(), other.GetHinshi() );
	}

	// デストラクタ
	~KDicRec() {
		if( yomi_ ) free( yomi_ );
		if( hyoki_ ) free( hyoki_ );
	}

	// 状態取得
	//
	//	よみ，表記，品詞コードの値，文字列サイズを得る．
	//	値が設定されていないときはNULLまたは0が返る．
	//
	const uchar* GetYomi() const{return yomi_;}
	int32 GetYomiSize() const{return yomi_size_;}
	const uchar* GetHyoki() const{return hyoki_;}
	int32 GetHyokiSize() const{return hyoki_size_;}
	KHinshiCode GetHinshi() const{return hinshi_;}

	// 状態設定
	//
	//	よみ，表記，品詞コードの値を設定する．
	//
	status_t SetYomi(const uchar*);
	status_t SetYomi(const uchar*,int32);
	status_t SetHyoki(const uchar*);
	status_t SetHyoki(const uchar*,int32);
	void SetHinshi(KHinshiCode hinshi) { hinshi_ = hinshi; }
	status_t SetData(const uchar* yomi, const uchar* hyoki, KHinshiCode hinshi)
	{
		status_t ret = SetYomi( yomi );
		if( ret == B_NO_ERROR ) ret = SetHyoki( hyoki );
		if( ret == B_NO_ERROR ) SetHinshi( hinshi );
		return ret;
	}
	status_t SetData(const uchar* yomi,int32 yomiSize, const uchar* hyoki,int32 hyokiSize, KHinshiCode hinshi)
	{
		status_t ret = SetYomi( yomi ,yomiSize);
		if( ret == B_NO_ERROR ) ret = SetHyoki( hyoki ,hyokiSize);
		if( ret == B_NO_ERROR ) SetHinshi( hinshi );
		return ret;
	}

	// チェック
	//
	//	よみ，表記，品詞コードの値が設定されているかどうかを得る．
	//	すべての値が設定されていなければ，エラーとなる．
	//
	status_t InitCheck() const {
		if( !yomi_ || !yomi_size_ || !hyoki_ || !hyoki_size_ ) return B_NO_INIT;
		return B_NO_ERROR;
	}

	// オペレータ
	KDicRec& operator=(const KDicRec& other)
	{
		if( &other != this )
			SetData( other.GetYomi(), other.GetHyoki(), other.GetHinshi() );
		return *this;
	}

private:

	uchar*		yomi_;			// よみ
	int32		yomi_size_;		// よみサイズ
	uchar*		hyoki_;			// 表記
	int32		hyoki_size_;	// 表記サイズ
	KHinshiCode	hinshi_;		// 品詞コード
};

// KDicRecArray
//
//	レコード配列クラス
//
//		機能: 配列操作
//
class KDicRecArray {
public:

	// コンストラクタ
	KDicRecArray() : recs_( 0 ), size_( 0 ) {}

	// デストラクタ
	~KDicRecArray() { delete [] recs_; }

	// 要素取得
	//
	//	指定した位置のKDicRecを取得する．
	//	0 <= offset < size_ でなければならない．
	//	offset < 0 || size_ <= offset，初期化されてないときは，assertをおこす．
	//
	const KDicRec& GetAt(int32 offset) const
	{
		assert( recs_ && 0 <= offset && offset < size_ );
		return recs_[ offset ];
	}
	const KDicRec& operator[](int32 offset) const { return GetAt( offset ); }

	// 要素サイズを得る．
	int32 GetSize() const { return size_; }

	// 初期化チェック
	//
	//	要素が設定されているかどうかを得る．
	//	要素が設定されていなければ，エラーとなる．
	//
	status_t InitCheck() const {
		if( !recs_ || !size_ ) return B_NO_INIT;
		return B_NO_ERROR;
	}

private:

	friend class KDicCtrl;

	KDicRec*	recs_;	// 要素ポインタ
	int32		size_;	// 要素サイズ

	KDicRecArray(const KDicRecArray&);
	KDicRecArray& operator=(const KDicRecArray&);
};

class DicList{
public:
	// デストラクタ
	~DicList();
protected:
	static class DicNode{
	public:
		DicRecord	dr;
		BEntry	entry_;
		
	} *dicList_;
	static int32	entry_size_;
	static int32	entry_all_size_;

	static int32	userDic;

	static uint32	fCount;
};

// KDicCtrl
//
//	かな漢字変換辞書管理クラス
//
//		機能: かな漢字変換辞書の管理，操作
//
class KDicCtrl:protected DicList {
public:

	// コンストラクタ
	KDicCtrl(){fCount++;}

	// デストラクタ
	~KDicCtrl(){fCount--;}

	// データ追加
	//
	//	ユーザー辞書にデータを追加する．
	//
	//	入力: path(辞書ファイルのパス), rec(データ)
	//
	status_t Append(const char* path, KDicRec* rec);
	// データ削除
	//
	//	ユーザー辞書からrecに一致するデータを削除する．
	//
	//	入力: path(辞書ファイルのパス), rec(データ)
	//
	status_t Delete(const char* path, KDicRec* rec);

	// データ検索
	//
	//	辞書からyomiに一致するデータを検索する．
	//
	//	入力:	path(辞書ファイルのパス),
	//			yomi(よみ)
	//			rec(検索結果を上書き格納する)
	//
	status_t Search(const char* path, const uchar* yomi, KDicRecArray* rec);

	// ユーザ辞書作成
	status_t CreateDic(const char* path);

	// 辞書登録
	//
	//	pathで指定した辞書をかな漢字変換で使用するように設定する．
	//
	status_t RegistDic(const char* path);

	// 辞書登録チェック
	//
	//	pathで指定した辞書をかな漢字変換で使用するように
	//	設定されているかを得る．
	//
	bool IsRegist(const char* path);

	// 辞書登録解除
	//
	//	pathで指定した辞書をかな漢字変換で使用しないように設定する．
	//
	status_t UnRegist(const char* path);


protected:
	void ReverseKey(uchar *revKey,const uchar *key,int yomiLen);

private:
	status_t CheckRegistUserDic(const char* path);
	Category ChangeDicHinshi(KHinshiCode hinCode);
	KHinshiCode ChangeHinshi(Category dicHinCode);

	KDicCtrl(const KDicCtrl&);
	KDicCtrl& operator=(const KDicCtrl&);
};

extern KDicCtrl		gDicCtrl;	// OSで唯一の辞書操作クラス

#endif	//  __DICCTRL_H__
/*--- end of DicCtrl.h ---*/
