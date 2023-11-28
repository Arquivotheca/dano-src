// Analysis.h
//
//	かな漢字変換エンジンクラス
//
//		作成日: 97/11/28
//
#ifndef __ANALYSIS_H__
#define __ANALYSIS_H__

#include <assert.h>
#include <SupportDefs.h>	// for BeOS
#include <stdlib.h>
#include "DicAccess.h"
#include "Grammar.h"
#include "Hash.h"
#include "Array.h"

class KClause;
class KClauseArray;
class KClauseArrayArray;
class KAnalysis;

extern KGrammarDic gGrammar;

// KAnalysis
//
//	かな漢字変換エンジンクラス
//
//		機能: かな漢字変換操作(変換対象threadに１つ)
//
class KAnalysis {
public:

	// コンストラクタ
	KAnalysis() : tail_hinshi_( 3 /* 文節端 */ ), text_( 0 ), wt_it_( word_text_ ) {}

	// デストラクタ
	~KAnalysis() { if( text_ ) free( text_ ); }

	// 入力文解析
	//
	//	入力文を解析する．textはひらがなでなければならない．
	//	逐次変換などでは，この関数を連続して呼び出すことで，
	//	順次解析する．
	//	textが，前回の関数呼び出しのときと共通の接頭辞を持つ場合は
	//	逐次変換とみなして前回の解析結果に追加するように解析する．
	//
	//	例
	//		AnalyzeText("か");
	//		AnalyzeText("かん");
	//		AnalyzeText("かんじ");		<-- ここまでは，逐次変換とみなす
	//		AnalyzeText("にほんご");	<-- 新たに解析する
	//
	//	入力: text(入力文．ひらがな，1024byte以内でなければならない)
	//
	status_t AnalyzeText(const uchar* text);

	// 変換候補を得る
	//
	//	前回のAnalyzeTextでの解析結果を変換候補として得る．
	//	retには次のように格納されている．
	//
	//	入力文 "きしゃがきしゃできしゃした"
	//		[ ]------>[ ]------>[ ]
	//		 |         |         |
	//		 V         V         V
	//		[記者が]  [汽車で]  [帰社した]
	//		 |         |         |
	//		 V         V         V
	//		[帰社が]  [記者で]  [汽車した]
	//		 |         |         |
	//		 V         V         V
	//		[汽車が]  [帰社で]  [記者した]
	//
	//	ret配列の1次元目に各文節が入り，2次元目にその文節の変換候補が入る．
	//
	//	入力: ret(変換候補を格納するクラス．この関数を呼び出すたびに更新される)
	//
	status_t GetResult(KClauseArrayArray* ret);

	// 区切り直しによる再解析
	//
	//	変換候補の文節の長さを変えた場合に，再解析を行い，結果を返す．
	//	上の例で，はじめの文節を "きしゃが" から，"きしゃ" に変更した場合は，
	//
	//		ReAnalyze( 0, 3, ret );
	//
	//	のように呼び出す．
	//
	//	入力:	no( 何番目の文節を変更するか．ret配列の1次元目の添字を与える．)
	//			size( 変更後の文節の文字数(バイト数)を与える．)
	//				( UTF-8の場合は3の倍数になるはず．        )
	//			ret( 変換候補を格納するクラス )
	//
	status_t ReAnalyze(int32 no, int32 size, KClauseArrayArray* ret);

	// 確定結果の学習
	//
	//	変換を確定した場合の変換候補の順序を学習する．
	//	変換確定後に各文節に対して呼び出す．
	//
	//	入力:	no( 何番目の文節か．ret配列の1次元目の添字を与える．)
	//			num( 何番目の候補が選ばれたか．ret配列の2次元目の点字を与える．)
	//
	status_t LearnResult(int32 no, int32 num=0);

	// 確定結果の学習(変換候補順序変更版)
	//
	//	変換を確定した場合の変換候補の順序を学習すし，
	//	残りの変換候補の配列順序を変更する．
	//	変換確定後に各文節に対して呼び出す．
	//
	//	入力:	ret( 変換候補を格納するクラス )
	//			no( 何番目の文節か．ret配列の1次元目の添字を与える．)
	//			num( 何番目の候補が選ばれたか．ret配列の2次元目の点字を与える．)
	//
	status_t LearnResult(KClauseArrayArray* ret, int32 no, int32 num=0);

	// 確定結果の学習2
	//
	//	変換を確定した場合の変換候補の文節区切りを学習する．
	//	変換確定後に呼び出す．
	//
	//	今後の拡張のために用意しておくが，現在は何もしない．
	//
	status_t LearnResult2();

private:

	friend class KClauseArrayArray;

	// KClauseArrayArray クラスのrep
	class Rep {
	public:
		KClauseArray*	clauseses_;		// 文節
		int32			size_;			// 文節数

		KClauseArray*	substance_;		// 文節の実体

		Rep() : clauseses_( 0 ), size_( 0 ), substance_( 0 ) {}
		inline ~Rep(); /*{ if( substance_ ) delete [] substance_; }*/
	};

	// 解析結果の位置保存用構造体
	class HinshiList {
	public:
		HinshiList*	next_;
		int			cost_;
		bool		isend_;
		Hinshi*		hinshi_;
		long		table_no_;		// AccessTableの添字
		long		table_num_;		// HinshiTableの添字

		HinshiList() : next_( 0 ), cost_( 0 ), isend_( false ), hinshi_( 0 ), table_no_( 0 ), table_num_( 0 ) {}
		~HinshiList() { delete next_; }
		void Clear() { delete  next_; cost_ = 0; isend_ = false; hinshi_ = 0; table_no_ = 0; table_num_ = 0; }
		HinshiList* NewNext() { if( !next_ ) next_ = new HinshiList; isend_ = false; return next_; }
		HinshiList& operator=(const HinshiList&);
	};

	// 文字バイト数リスト
	class WordText {
	public:
		uchar*	text_;
		int		size_;

		WordText() : text_( 0 ), size_( 0 ) {}
		WordText(const WordText&);
		~WordText() { if( text_ ) free( text_ ); }
		WordText& operator=(const WordText&);
		int byte(int=0);
	};

	// 文字バイト数リスト操作
	class WordTextIterator {
	public:
		WordTextIterator( WordText& wt ) : num_( 0 ), pos_( 0 ), wt_( wt ) {}
		WordTextIterator(const WordTextIterator& other) : num_( other.num_ ), pos_( other.pos_ ), wt_( other.wt_ ) {}
		~WordTextIterator() {}
		WordTextIterator& operator=(const WordTextIterator& other)
		{
			if( &other != this ){ num_ = other.num_; pos_ = other.pos_; wt_ = other.wt_; }
			return *this;
		}
		WordTextIterator& operator++()
		{
			assert( pos_ < wt_.size_ );
			num_ += wt_.text_[ pos_ ];
			pos_++;
			return *this;
		}
		WordTextIterator& operator--()
		{
			assert( pos_ > 0 );
			pos_--;
			num_ -= wt_.text_[ pos_ ];
			return *this;
		}
		WordTextIterator& operator+=(const WordTextIterator& other)
		{
			while( pos_ < other.pos_ ) operator++();
			return *this;
		}
		long byte() { return num_; }
		short pos() { return pos_; }
		void init() { num_ = 0; pos_ = 0; }
		void reset() { num_ = 0; }
		bool isbegin() { return (pos_ == 0); }
		bool isend() {  return (pos_ == wt_.size_); }
	private:
		short		num_;
		short		pos_;
		WordText&	wt_;
	};

	class HashFunc {
	public:
		HashFunc() : key_( 0 ) {}
		HashFunc( uchar* k ) : key_( k ) {}
		uint operator()( const HashFunc& );
		HashFunc& operator=(const HashFunc& o) { key_ = o.key_; return *this;}
		bool operator==(const HashFunc&);
	private:
		uchar*	key_;
	};

	// 文節間のset
	struct HinshiSet {
		Hinshi*		left_;
		Hinshi*		right_;
		short		cost_;
	};

	// 品詞の配列
	class ARRAY_ : public Array< HinshiSet, 10 > {
	public:
		ARRAY_() : sel_( 0 ), hin_( 0 ), cnt_( 0 ) {}
		void Add( HinshiSet& );
		void SetSelect( Hinshi* );
		Hinshi* UniqGet( bool = false );
		HinshiSet* GetSelect() { return sel_; }
	private:
		HinshiSet*	sel_;
		Hinshi* hin_;
		uint cnt_;

	};

	enum FLAG_ { non = 0, segment, content, other };

	typedef Hash< HashFunc, bool, 50, HashFunc > HASH_;

	Rep	rep_;

	DicAccess			dicA_;				// アクセステーブル処理クラス
	short				tail_hinshi_;		// 前回解析時の最後の品詞コード
	HinshiList			hinshi_list_;		// 解析結果の位置保存用
	WordText			word_text_;
	WordTextIterator	wt_it_;
	uchar*				text_;				// 入力文字列
	int					text_len_;			// 入力文字列の長さ

	// get_clause_用
	Field*	fld[ 10 ];
	Hinshi*	fld_top[ 10 ];
	short	fld_pos[ 10 ];
	int		fld_num;
	short	word_pos;

	// calc_cost_sub_用
	int		comeback_cnt;
	int		last_cnt;

	KAnalysis(const KAnalysis&);
	KAnalysis& operator=(const KAnalysis&);

	int set_clauses_( KClauseArray*, Hinshi*, short, int, HASH_& );
	int get_clause_size_( Hinshi* );
	bool get_clause_( KClause*, const Hinshi* = 0, short = 0 );
	status_t set_word_text( WordText*, const uchar* );
	void calc_cost_sub_( HinshiList*, int, int, ARRAY_&, ARRAY_& );
	void calc_cost_sub2_( HinshiList*, int, int, int, ARRAY_&, ARRAY_&, FLAG_ );
	void calc_cost_head_( HinshiList*, ARRAY_&, ARRAY_& );
	void set_rep_();
};


// KClause
//
//	変換候補文節クラス
//
//		機能: 変換候補の１文節の操作
//
class KClause {
public:

	// コンストラクタ
	KClause() : clause_( 0 ), size_( 0 ), pos_( 0 ), pos_size_( 0 ) {}
	KClause(const KClause& other)
		: clause_( 0 ), size_( 0 ), pos_( 0 ), pos_size_( 0 )
	{
		Init( other );
	}

	// デストラクタ
	~KClause()
	{
		if( clause_ ) free( clause_ );
		if( pos_size_ ) free( pos_ );
	}

	// オペレータ
	KClause& operator=(const KClause& other)
	{
		if( &other != this ) Init( other );
		return *this;
	}

	// 文節を得る
	uchar* GetClause() const { return clause_; }

	// 文節サイズを得る
	int32 GetClauseSize() const { return size_; }

	// 初期化チェック
	status_t InitCheck() const
	{
		if( !clause_ || !size_ || !pos_ || !pos_size_ ) return B_NO_INIT;
		return B_NO_ERROR;
	}

private:

	friend class KClauseArray;
	friend class KClauseArrayArray;
	friend class KAnalysis;
	friend int Kcomp_clause(const void *, const void *);

	uchar*		clause_;	// 候補の文節
	int32		size_;		// clause_のサイズ

	struct POS_ {	// 学習のための位置情報
		Hinshi*		hinshi_;	// 品詞
		short		fpos_;		// フィールド位置(0~)
		short		wpos_;		// 入力文位置(0~)
	};
	POS_*		pos_;
	int			pos_size_;	// pos_の要素数

	status_t SetData(const uchar*);
	status_t Init( const KClause& );

};

// KClauseArray
//
//	１文節の変換候補クラス
//
//		機能: １文節に対する変換候補全体の操作
//
class KClauseArray {
public:

	// コンストラクタ
	KClauseArray() : clauses_( 0 ), size_( 0 ) {}

	// デストラクタ
	~KClauseArray()
	{
		if( clauses_ ) delete [] clauses_;
	}

	// 配列要素を得る
	//
	//	0 <= num < size_ でなければならない
	//
	KClause& GetAt(int32 num) const
	{
		assert( 0 <= num && num < size_ );
		return clauses_[ num ];
	}
	KClause& operator[](int32 num) const { return GetAt( num ); }

	// 配列サイズを得る
	int32 GetSize() const { return size_; }

	// この区切りのバイト数を得る
	int32 GetWordSize() const { return word_size_; }

	// 初期化チェック
	status_t InitCheck() const
	{
		if( !clauses_ || !size_ ) return B_NO_INIT;
		return B_NO_ERROR;
	}

private:

	friend class KClauseArrayArray;
	friend class KAnalysis;

	KClause*	clauses_;	// 変換候補
	int32		size_;		// 変換候補数
	int32		word_size_;	// 区切りのバイト数

	KClauseArray(const KClauseArray&);

	KClauseArray& operator=(const KClauseArray& other);
};

// KClauseArrayArray
//
//	変換候補クラス
//
//		機能: 変換候補全体の操作
//
class KClauseArrayArray {
public:

	// コンストラクタ
	KClauseArrayArray() : rep_( 0 ) {}

	// デストラクタ
	~KClauseArrayArray() {}

	// 配列要素を得る
	//
	//	0 <= num < size_ でなければならない
	//
	KClauseArray& GetAt(int32 num) const
	{
		assert( 0 <= num && num < rep_->size_ );
		return rep_->clauseses_[ num ];
	}

	KClauseArray& operator[](int32 num) const { return GetAt( num ); }

	// 配列サイズを得る
	int32 GetSize() const { return rep_->size_; }

	// 初期化チェック
	status_t InitCheck() const
	{
		if( !rep_ || !rep_->clauseses_ || !rep_->size_ ) return B_NO_INIT;
		return B_NO_ERROR;
	}

private:

	friend class KAnalysis;

	KAnalysis::Rep*		rep_;

	KClauseArrayArray(const KClauseArrayArray&);
	KClauseArrayArray& operator=(const KClauseArrayArray&);

};

// インライン関数
KAnalysis::Rep::~Rep() { if( substance_ ) delete [] substance_; }

#endif // __ANALYSIS_H__
/*--- end of Analysis.h ---*/
