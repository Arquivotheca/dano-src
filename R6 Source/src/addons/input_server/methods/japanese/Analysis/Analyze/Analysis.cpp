/* Analysis.cpp
 *
 *	かな漢字変換エンジンクラス
 *
 *		programmed 1997.
 */
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>
#include <assert.h>
#include <new.h>
#include <string.h>
#include "Analysis.h"

// short max
const short SHORT_MAX = 0x7fff;

// KClause
//
//	変換候補文節クラス
//
//		機能: 変換候補の１文節の操作
//

// データ設定
//
//	input : clause( 文節 )
//	output: エラーコード
//
status_t KClause::SetData(const uchar* clause)
{
	assert( clause );

	size_ = strlen( (const char*)clause );
	assert( size_ );

	clause_ = (uchar*)malloc( size_ + 1 );
	if( !clause_ )
		return B_NO_MEMORY;

	strcpy( (char*)clause_, (const char*)clause );

	return B_NO_ERROR;
}

// データ設定
//
//	input : other( ほかの )
//	output: エラーコード
//
status_t KClause::Init( const KClause& other )
{
	pos_size_ = other.pos_size_;
	pos_ = (POS_*)malloc( sizeof(POS_) * pos_size_ );
	if( !pos_ ) return B_NO_MEMORY;

	memcpy( pos_, other.pos_, sizeof(POS_) * pos_size_ );

	return SetData( other.clause_ );
}

// KAnalysis
//
//	かな漢字変換エンジンクラス
//
//		機能: かな漢字変換操作(変換対象threadに１つ)
//

//KAnalysis::Rep KAnalysis::rep_;		// Repクラスの実体

// 品詞リストの代入
KAnalysis::HinshiList& KAnalysis::HinshiList::operator=(const HinshiList& other)
{
	if( &other == this ) return *this;

	HinshiList* src = (HinshiList*)&other;
	HinshiList* dst = this;
	while( !src->isend_ ){
		dst->hinshi_ = src->hinshi_;
		dst->table_no_ = src->table_no_;
		dst->table_num_ = src->table_num_;
		dst->cost_ = src->cost_;
		dst->isend_ = src->isend_;
		src = src->next_;
		dst = dst->NewNext();
	}

	// 最後
	dst->hinshi_ = src->hinshi_;
	dst->table_no_ = src->table_no_;
	dst->table_num_ = src->table_num_;
	dst->cost_ = src->cost_;
	dst->isend_ = src->isend_;

	return *this;
}

// WordText のコピーコンストラクタ
KAnalysis::WordText::WordText(const WordText& other)
{
	text_ = (uchar*)malloc( sizeof(uchar) * other.size_ );
#if _SUPPORTS_EXCEPTION_HANDLING
	// malloc never fails on IAD, hplus said it was OK to check the exception-handling define for this feature
	if( !text_ ) throw bad_alloc();
#endif
	size_ = other.size_;
	memcpy( text_, other.text_, sizeof(uchar) * size_ );
}

// WordText の代入演算子
KAnalysis::WordText& KAnalysis::WordText::operator=(const WordText& other)
{
	if( &other != this ){
		text_ = (uchar*)realloc( text_, sizeof(uchar) * other.size_ );
#if _SUPPORTS_EXCEPTION_HANDLING
		// malloc never fails on IAD, hplus said it was OK to check the exception-handling define for this feature
		if( !text_ ) throw bad_alloc();
#endif
		size_ = other.size_;
		memcpy( text_, other.text_, sizeof(uchar) * size_ );
	}
	return *this;
}

// 入力文字数を返す
//
//	input : n( 何文字目から数えるか 0~ )
//	output: byte数
//
int KAnalysis::WordText::byte(int n /* =0 */)
{
	int ret = 0;
	for( int i = n; i < size_; i++ ) ret += text_[ i ];
	return ret;
}

// 追加
//
//	input : left,right( 品詞 ), cost( left-right間のコスト )
//
void KAnalysis::ARRAY_::Add( HinshiSet& set )
{
	if( dat_size_ >= all_size_ ){
		HinshiSet* new_t = new HinshiSet[ all_size_ + 10 ];
		for( unsigned int i = 0; i < all_size_; i++ ) new_t[ i ] = dat_[ i ];
		delete [] dat_;
		dat_ = new_t;
		all_size_ += 10;
	}

	uint i = 0;
	for( ; i < dat_size_; ++i ){
//		if( dat_[ i ].left_->category > set.left_->category ){
		if( (PRE_HINSHI( dat_[ i ].left_->category ) > PRE_HINSHI( set.left_->category )) ||
			( PRE_HINSHI( dat_[ i ].left_->category ) == PRE_HINSHI( set.left_->category )
			&& dat_[ i ].cost_ > set.cost_ ) ){

				for( uint j = dat_size_; j > i; --j ){
					dat_[ j ] = dat_[ j - 1 ];
				}
				break;
		}
	}

	dat_[ i ] = set;
	++dat_size_;
}

// 選択した文節setの設定
//
//	input : hin( 品詞 )
//
void KAnalysis::ARRAY_::SetSelect( Hinshi* hin )
{
	assert( dat_size_ );

	sel_ = 0;
	if( !hin ) return;

	short cost = SHORT_MAX;
	for( uint i = 0; i < dat_size_; ++i ){
		if( hin == dat_[ i ].left_ && cost > dat_[ i ].cost_ ){
			sel_ = &( dat_[ i ] );
			cost = dat_[ i ].cost_;
		}
	}
}

// 配列中の左品詞をuniqに取り出す
//
//	input : flg( trueは1回目 )
//
Hinshi* KAnalysis::ARRAY_::UniqGet( bool flg )
{
	assert( dat_size_ );

	if( flg ){
		hin_ = dat_[ 0 ].left_;
		cnt_ = 0;
	}
	else{
		for( ++cnt_; cnt_ < dat_size_; ++cnt_ ){
			if( PRE_HINSHI( hin_->category ) != PRE_HINSHI( dat_[ cnt_ ].left_->category ) ){
				hin_ = dat_[ cnt_ ].left_;
				return hin_;
			}
		}
		return 0;
	}

	return hin_;
}

// 文字数と文字バイト数リストを得る
//
//	input : word_text( 格納用 ), text( テキスト )
//	output: エラーコード
//
status_t KAnalysis::set_word_text( WordText* wt, const uchar* text )
{
	wt->size_ = 0;
	for( const uchar* p = text; *p; ){
		if( *p <= 0x7f ) p++;
		else if( 0xc0 <= *p && *p <= 0xdf ) p += 2;
		else if( 0xe0 <= *p && *p <= 0xef ) p += 3;
		else if( 0xf0 <= *p && *p <= 0xf7 ) p += 4;
		else if( 0xf8 <= *p && *p <= 0xfb ) p += 5;
		else if( 0xfc <= *p && *p <= 0xfd ) p += 6;
		wt->size_++;
	}

	wt->text_ = (uchar*)realloc( wt->text_, sizeof(uchar) * wt->size_ );
	if( !wt->text_ ) return B_NO_MEMORY;

	uchar* pw = wt->text_;
	for( const uchar* p = text; *p; ){
		if( *p <= 0x7f ){ *pw = 1; pw++; p++; }
		else if( 0xc0 <= *p && *p <= 0xdf ){ *pw = 2; pw++; p += 2; }
		else if( 0xe0 <= *p && *p <= 0xef ){ *pw = 3; pw++; p += 3; }
		else if( 0xf0 <= *p && *p <= 0xf7 ){ *pw = 4; pw++; p += 4; }
		else if( 0xf8 <= *p && *p <= 0xfb ){ *pw = 5; pw++; p += 5; }
		else if( 0xfc <= *p && *p <= 0xfd ){ *pw = 6; pw++; p += 6; }
	}

	return B_NO_ERROR;
}

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
status_t KAnalysis::AnalyzeText(const uchar* text)
{
	text_len_ = strlen( (const char*)text );

	assert( 0 < text_len_ && text_len_ < 1024 );

	status_t ret = B_NO_ERROR;

#if _SUPPORTS_EXCEPTION_HANDLING
	try{
#endif
		// 接頭辞が同じか
		int _i = 0;
		if( text_ ){
			const uchar* p = text;
			const uchar* q = text_;
			for( ; *p && *q; ++p, ++q, ++_i ){
				if( *p != *q ) break;
			}
		}

		text_ = (uchar*)realloc( text_, text_len_ + 1 );
		if( !text_ ) return B_NO_MEMORY;
		strcpy( (char*)text_, (char*)text );

		// テスト用
		//dicA_.at.AllFree();

		// 文字数と文字バイト数リストを得る
		ret = set_word_text( &word_text_, text_ );
		if( ret != B_NO_ERROR ) return ret;

		WordTextIterator it_no( word_text_ );
		wt_it_ = it_no;
		while( it_no.byte() < _i ) ++it_no;
		if( it_no.byte() > _i ) --it_no;

		// つむ
		ret = dicA_.MakeAccessTable( text_, word_text_.text_, word_text_.size_, it_no.pos() );
		if( ret != B_NO_ERROR ) return ret;

#if _SUPPORTS_EXCEPTION_HANDLING
	}
	catch( bad_alloc ){
		return B_NO_MEMORY;
	}
#endif
	return ret;
}

// 指定した位置からコスト計算
//
//	input : hinL( 格納場所 接続がなかったときは，hinL->hinshi_ = NULL ),
//			head_pos( 解析範囲の最初の位置 )
//			tail_pos( 解析範囲の最後の位置 )
//			pre( 解析範囲の後の品詞コード )
//			post( 解析範囲の前の品詞コード )
//	output: エラーコード
//
void KAnalysis::calc_cost_sub_( HinshiList* hinL, int head_pos, int tail_pos, ARRAY_& pre, ARRAY_& post )
{
	++comeback_cnt;

	hinL->cost_ = SHORT_MAX;

	// 後ろから順に最長の文節を得る
	WordTextIterator it( wt_it_ );
	int now = 0;
	HinshiTableArray *hta = &dicA_.at[ it.byte() ];
	int other_no = 0;
	int other_num = 0;
	bool flg = true;
	FLAG_ flag = non;
	for( ; it.byte() < tail_pos && !it.isend(); ++it ){
		// 最後までのがあった
		hta = &dicA_.at[ it.byte() ];
		now = tail_pos - it.byte();
		if( hta->nowTable >= now && (*hta)[ now ].firstHinshi ){
			// 自立語があった
			if( flg && (*hta)[ now ].contentWFlag ){
				// 接続がある時位置保存 (最長のもののみ)...A)
				other_no = it.byte();
				other_num = now;
				flg = false;
				flag = content;
			}
			else if( flg && flag == non && !(*hta)[ now ].segmentFlag ){
				// 接続がある時位置保存 (最長のもののみ)...B)
				other_no = it.byte();
				other_num = now;
				flag = other;
			}

			// 文節があった
			if( (*hta)[ now ].segmentFlag ){
				flg = false;
				calc_cost_sub2_( hinL, head_pos, it.byte(), now, pre, post, segment );
			}
		}
	}

	// 最後までいった == 文節がなかった
	if( (it.byte() == tail_pos || it.isend()) && flag != non ){
		// A) または B) のときのコスト計算
		++last_cnt;
		calc_cost_sub2_( hinL, head_pos, other_no, other_num, pre, post, flag );
		--last_cnt;
	}
	
	--comeback_cnt;
}

// 解析　その２
//
//	input : hinL( 格納場所 ),
//			head_pos( 解析範囲の最初の位置 ),
//			no( AccessTableの位置 ), num( HinshiTableの位置 ),
//			pre( 後ろの品詞 ), post( 前の品詞 ), flag( フラグ )
//
void KAnalysis::calc_cost_sub2_( HinshiList* hinL, int head_pos, int no, int num, ARRAY_& pre, ARRAY_& post, KAnalysis::FLAG_ flag )
{
	// 接続がある時位置保存
	HinshiList hl;
	HinshiSet set;
	hl.table_no_ = no;
	hl.table_num_ = num;
	Category old_cate = 0;
	int old_cost = SHORT_MAX;
	ARRAY_ next_pre;
	bool flg = false;

	for( set.right_ = pre.UniqGet( true ); set.right_; set.right_ = pre.UniqGet() ){
		old_cate = 0;
		old_cost = SHORT_MAX;
		set.cost_ = SHORT_MAX;
		for( set.left_ = dicA_.at[ no ][ num ].firstHinshi; set.left_; set.left_ = set.left_->nextHinshi ){
			switch( flag ){
			case segment: flg = set.left_->segmentFlag; break;
			case content: flg = set.left_->contentWFlag; break;
			case other: flg = true; break;
			default: flg = false;
			}
			if( flg && gGrammar.IsConnect( POST_HINSHI( set.left_->category ), PRE_HINSHI( set.right_->category ) ) ){
				// コストが同じ時は何もしない(後との接続に対して)
				set.cost_ = gGrammar.GetCost( POST_HINSHI( set.left_->category ), PRE_HINSHI( set.right_->category ) );
							+ set.right_->cost;
				if( old_cate && PRE_HINSHI( old_cate ) == PRE_HINSHI( set.left_->category ) &&
					old_cost <= set.cost_ ) continue;

				next_pre.Add( set );

				old_cate = set.left_->category;
				old_cost = set.cost_;
			}
		}
	}

	if( !next_pre.GetSize() ) return;

	// 現在地より前の解析
	if( no == head_pos ) calc_cost_head_( hl.NewNext(), post, next_pre );
	else
		calc_cost_sub_( hl.NewNext(), head_pos, no, next_pre, post );

	if( hl.next_->cost_ != SHORT_MAX ){		// 接続があるとき
		hl.hinshi_ = next_pre.GetSelect()->left_;
		hl.cost_ = hl.hinshi_->cost + hl.next_->cost_ + next_pre.GetSelect()->cost_;

		// コスト代入
		if( hinL->cost_ > hl.cost_ ){
			*hinL = hl;
			pre.SetSelect( next_pre.GetSelect()->right_ );
		}
	}
	else if( comeback_cnt == last_cnt ){	// 接続がないとき
		pre.SetSelect( pre.UniqGet( true ) );
		hl.hinshi_ = dicA_.at[ no ][ num ].firstHinshi;

		HinshiList *hl2 = hl.next_;

		WordTextIterator it( wt_it_ );
		while( it.byte() < no ) ++it;

		while( hl2->cost_ == SHORT_MAX ){
			if( it.byte() == head_pos ){
				hl2->isend_ = true;
				break;
			}

			--it;	// １文字手前に戻す

			hl2->hinshi_ = dicA_.at[ it.byte() ][ no - it.byte() ].firstHinshi;
			hl2->table_no_ = it.byte();
			hl2->table_num_ = no - it.byte();

			//
			next_pre.Clear();
			set.right_ = 0;
			set.cost_ = 0;
			for( set.left_ = hl2->hinshi_; set.left_; set.left_ = set.left_->nextHinshi ){
				next_pre.Add( set );
			}
			// 品詞のないときは名無しを当てる
			Hinshi no_hin;
			if( !hl2->hinshi_ ){
				no_hin.nextHinshi = 0;
				no_hin.category = 2;	// 名無しの品詞コード
				no_hin.cost = 0;
				no_hin.contentWFlag = false;
				no_hin.segmentFlag = false;			
				no_hin.length = hl2->table_num_;				
				no_hin.firstField = 0;			

				set.left_ =  &no_hin;
				next_pre.Add( set );
			}

			if( it.byte() == head_pos ) calc_cost_head_( hl2->NewNext(), post, next_pre );
			else
				calc_cost_sub_( hl2->NewNext(), head_pos, it.byte(), next_pre, post );

			hl2 = hl2->next_;
			no = it.byte();
		}
		*hinL = hl;
	}
}

// 先頭のコスト計算
//
//	input : hin( 格納場所 ), post( 前の品詞 ), pre( 後ろの品詞 )
//
void KAnalysis::calc_cost_head_( HinshiList* hin, ARRAY_& post, ARRAY_& pre )
{
	hin->isend_ = true;
	pre.SetSelect( 0 );
	post.SetSelect( 0 );
	int cost;
	hin->cost_ = SHORT_MAX;
	for( Hinshi* post_hin = post.UniqGet( true ); post_hin; post_hin = post.UniqGet() ){
		for( Hinshi* pre_hin = pre.UniqGet( true ); pre_hin; pre_hin = pre.UniqGet() ){
			if( gGrammar.IsConnect( POST_HINSHI( post_hin->category ), PRE_HINSHI( pre_hin->category ) ) ){
				cost = gGrammar.GetCost( POST_HINSHI( post_hin->category ), PRE_HINSHI( pre_hin->category ) );
				if( hin->cost_ > cost ){
					hin->cost_ = cost;
					post.SetSelect( post_hin );
					pre.SetSelect( pre_hin );
				}
			}
		}
	}

	if( !pre.GetSelect() ) hin->cost_ = SHORT_MAX;
}

// ハッシュ表用ハッシュ関数
//
//	input : key( キー )
//	output: ハッシュ値
//
uint KAnalysis::HashFunc::operator()( const HashFunc& key )
{
	int len = strlen( (const char*)key.key_ );
	uint ret = 0;
	for( int i = 0; i < len; i++ ) ret += key.key_[ i ];
	return ret;
}

bool KAnalysis::HashFunc::operator==(const KAnalysis::HashFunc& other)
{
	if( !key_ || !other.key_) return false;
	return (strcmp( (char*)key_, (char*)other.key_ ))? false: true;
}

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
status_t KAnalysis::GetResult(KClauseArrayArray* ret)
{
	// 引数チェック
	assert( ret );

#if _SUPPORTS_EXCEPTION_HANDLING
	try{
#endif
		// コスト計算
//		HinshiList hinL;
		hinshi_list_.cost_ = SHORT_MAX;

		Hinshi hin;
		hin.cost = 0;
		hin.category = HINSHI( HEAD_EDGE, HEAD_EDGE );	// 文節頭端
		Hinshi hin2;
		hin2.cost = 0;
		hin2.category = HINSHI( tail_hinshi_, tail_hinshi_ );
		Hinshi hin3;
		hin3.cost = 0;
		hin3.category = HINSHI( FREE_EDGE, FREE_EDGE );	// 開放端

		HinshiSet set = { &hin3, 0, 0 };
		ARRAY_ pre;
		ARRAY_ post;
		pre.Add( set );
		set.left_ = &hin;
		post.Add( set );
		set.left_ = &hin2;
		post.Add( set );

		wt_it_.init();

		// 計算
		comeback_cnt = last_cnt = 0;
//		calc_cost_sub_( &hinL, text_len_, pre, post );
		calc_cost_sub_( &hinshi_list_, 0, text_len_, pre, post );

		// 接続があった
//		assert( hinshi_list_.cost_ > hinL.cost_ )
//		hinshi_list_ = hinL;

		// 接続が無かった時に全選択
		if( hinshi_list_.table_num_ == 0 ){
			hinshi_list_.table_num_ = text_len_;
			hinshi_list_.NewNext()->isend_ = true;
		}

		// rep_への設定
		set_rep_();
		ret->rep_ = &rep_;
#if _SUPPORTS_EXCEPTION_HANDLING
	}
	catch( bad_alloc ){ return B_NO_MEMORY; }
#endif

	return B_NO_ERROR;
}

// KClauseのソート用比較関数
//
int Kcomp_clause(const void *a, const void *b)
{
	KClause &x = *(KClause *)a;
	KClause &y = *(KClause *)b;

	Field* x_fld = x.pos_[0].hinshi_->firstField;
	short i = 0;
	for( ; i < x.pos_[0].fpos_; ++i ) x_fld = x_fld->nextField;

	Field* y_fld = y.pos_[0].hinshi_->firstField;
	for( i = 0; i < y.pos_[0].fpos_; ++i ) y_fld = y_fld->nextField;

	if (x_fld->evaluation > y_fld->evaluation)
		return (-1);
	else if (x_fld->evaluation < y_fld->evaluation)
		return (1);

	return (0);
}

//null_template
//struct iterator_trait <KClause*> {
//    typedef ptrdiff_t                    distance_type;
//    typedef KClause                         value_type;
//    typedef random_access_iterator_tag   iterator_category;
//};

// rep_への設定
//
void KAnalysis::set_rep_()
{
	// 文節数を得る
	int32 i = 0;
	HinshiList* hin = &hinshi_list_;
	for( ; hin && !hin->isend_; hin = hin->next_ ) i++;

	// rep_のクリア＆再確保
	delete [] rep_.substance_;
	rep_.substance_ = 0;
	rep_.size_ = i;
	rep_.substance_ = new KClauseArray[ i ];
	rep_.clauseses_ = rep_.substance_;

	// 後ろから格納
	--i;
	for( hin = &hinshi_list_; i >= 0; --i, hin = hin->next_ ){
		// 一つの文節での要素数を得る
		int32 f_num = 0;
  		if( dicA_.at[ hin->table_no_ ].nowTable >= hin->table_num_ ){
    		for( Hinshi* h = dicA_.at[ hin->table_no_ ][ hin->table_num_].firstHinshi; h; h = h->nextHinshi ){
				if( h->segmentFlag ) f_num += get_clause_size_( h );
      			else for( Field* f = h->firstField; f; f = f->nextField ) f_num++;
			}
		}

		// KClause の確保
		rep_.clauseses_[ i ].clauses_ = new KClause[ ((f_num)? f_num: 1) ];

		// ハッシュの初期化
		HASH_ hash;

		// 要素の格納
		if( hin->hinshi_ ){
			KClauseArray* kc = &( rep_.clauseses_[ i ] );

			f_num = 0;

			// 解析された品詞から格納する
			f_num = set_clauses_( kc, hin->hinshi_, hin->table_no_, f_num, hash );

			for( Hinshi* h = dicA_.at[ hin->table_no_ ][ hin->table_num_ ].firstHinshi; h; h = h->nextHinshi ){
				if( h == hin->hinshi_ ) continue;
				kc = &( rep_.clauseses_[ i ] );
				f_num = set_clauses_( kc, h, hin->table_no_, f_num, hash );
			}
		}
		else{	// ないとき
			if( f_num ){
				f_num = 0;
				KClauseArray* kc = &( rep_.clauseses_[ i ] );
				for( Hinshi* h = dicA_.at[ hin->table_no_ ][ hin->table_num_ ].firstHinshi; h; h = h->nextHinshi ){
					f_num = set_clauses_( kc, h, hin->table_no_, f_num, hash );
				}
			}
			else{
				f_num = 1;
				KClause& kc = rep_.clauseses_[ i ].clauses_[ 0 ];

				uchar* p = (uchar*)malloc( hin->table_num_ + 1 );
#if _SUPPORTS_EXCEPTION_HANDLING
				// malloc never fails on IAD, hplus said it was OK to check the exception-handling define for this feature
				if( !p ) throw B_NO_MEMORY;
#endif

				p[ hin->table_num_ ] = 0;
				memcpy( p, &( text_[ hin->table_no_ ] ), hin->table_num_ );

				kc.SetData( p );
				kc.pos_ = (KClause::POS_*)realloc( kc.pos_, sizeof(KClause::POS_) );
				kc.pos_[0].hinshi_ = 0;
				kc.pos_[0].fpos_ = 0;
				kc.pos_[0].wpos_ = hin->table_no_;
				kc.pos_size_ = 1;

				free( p );
			}
		}

		rep_.clauseses_[ i ].size_ = f_num;
		rep_.clauseses_[ i ].word_size_ = hin->table_num_;

		// ソート
		qsort(rep_.clauseses_[ i ].clauses_, f_num, sizeof(KClause), Kcomp_clause);
	}
}

// KClauseへの設定
//
//	input : kc( 格納場所 ), hin( 取り出す場所 ), wpos( 入力文の位置 ), num( 現在の格納数 ), hash( ハッシュ )
//	output: 更新されたnum
//
int KAnalysis::set_clauses_( KClauseArray* kca, Hinshi* hin, short wpos, int num, HASH_& hash )
{
	// 文節のとき
	if( hin->segmentFlag ){
		KClause* kc = &( kca->clauses_[ num ] );
		get_clause_( kc, hin, wpos );
		do{
			// ハッシュになければ格納する
			if( !hash.FindKey( HashFunc(kc->GetClause()) ) ){
				hash.AddKey( HashFunc(kc->GetClause()), true );
				num++;
			}

			kc = &( kca->clauses_[ num ] );

		} while( get_clause_( kc ) );
	}
	else{	// 文節でないとき
		short s = 0;
		for( Field* f = hin->firstField; f; f = f->nextField, s++ ){
			KClause* kc = &( kca->clauses_[ num ] );

			// 設定
			kc->SetData( (uchar*)(f->contents) );
			kc->pos_ = (KClause::POS_*)realloc( kc->pos_, sizeof(KClause::POS_) );
			kc->pos_[0].hinshi_ = hin;
			kc->pos_[0].fpos_ = s;
			kc->pos_[0].wpos_ = wpos;
			kc->pos_size_ = 1;

			// ハッシュになければ格納する
			if( !hash.FindKey( HashFunc(kc->GetClause()) ) ){
				hash.AddKey( HashFunc(kc->GetClause()), true );
				num++;
			}
		}
	}

	return num;
}

// 文節数を得る
//
//	input : hin( 参照場所 )
//	output: 文節数
//
int KAnalysis::get_clause_size_( Hinshi* hin )
{
	int ret = 1;
	for( Field* f = hin->firstField; f; f = f->nextField ){
		int i = 0;
		for( Field* g = ((Hinshi*)(f->contents))->firstField; g; g = g->nextField ) i++;
		ret *= i;
	}
	return ret;
}

// 文節の文字列を得る
//
//	input : p( 格納場所 ), hin( 参照場所 2回目以降は0 )
//	output: true( まだ残ってる ) false( 終わり )
//
bool KAnalysis::get_clause_( KClause* kc, const Hinshi* hin /* =0 */, short wpos /* =0 */ )
{
	int i = 0;

	if( hin ){	// 1回目
		// 文節の構成要素数を得る
		word_pos = wpos;
		fld_num = 0;
		for( Field* f = hin->firstField; f; f = f->nextField ){
			fld[ fld_num ] = ((Hinshi*)(f->contents))->firstField;
			fld_top[ fld_num ] = (Hinshi*)(f->contents);
			fld_pos[ fld_num ] = 0;
			fld_num++;
		}
	}
	else{	// 2回目以降 次の位置を得る
		for( i = 0; i < fld_num; i++ ){
			fld[ i ] = fld[ i ]->nextField;
			fld_pos[ i ]++;
			if( fld[ i ] ) break;
			fld[ i ] = fld_top[ i ]->firstField;
			fld_pos[ i ] = 0;
		}
		if( i == fld_num ) return false;
	}

	// KClause に値設定
	int len = 0;
	for( i = 0; i < fld_num; i++ ) len += strlen( (char*)fld[ i ]->contents );
	kc->clause_ = (uchar*)realloc( kc->clause_, sizeof(uchar) * len + 1 );
	*(kc->clause_) = 0;
	kc->size_ = len;
	kc->pos_size_ = fld_num;
	kc->pos_ = (KClause::POS_*)realloc( kc->pos_, sizeof(KClause::POS_) * fld_num );
#if _SUPPORTS_EXCEPTION_HANDLING
	// malloc never fails on IAD, hplus said it was OK to check the exception-handling define for this feature
	if( !kc->pos_ ) throw bad_alloc();
#endif
	short pos = word_pos;
	for( i = 0; i < fld_num; i++ ){
		strcat( (char*)kc->clause_, (char*)(fld[ i ]->contents) );
		kc->pos_[ i ].hinshi_ = fld_top[ i ];
		kc->pos_[ i ].fpos_ = fld_pos[ i ];
		kc->pos_[ i ].wpos_ = pos;
		pos += fld_top[ i ]->length;
	}

	return true;
}

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
status_t KAnalysis::ReAnalyze(int32 no, int32 size, KClauseArrayArray* ret)
{
	// 引数チェック
	assert( ret );
	assert( 0 <= no && no < rep_.size_ );
	assert( 0 <= size );

	status_t rt = B_NO_ERROR;

#if _SUPPORTS_EXCEPTION_HANDLING
	try{
#endif
		// HinshiListの位置設定
		HinshiList* hl = &hinshi_list_;
		for( int32 i = 1; i < rep_.size_ - no; i++ ) hl = hl->next_;

		// 入力文字数より長い時は何もしない
		if( hl->table_no_ + size > text_len_ ) return rt;

		// it_noの位置設定(1次元目)
		wt_it_.init();
		while( wt_it_.byte() < hl->table_no_ + size ) ++wt_it_;

		// 変換位置の解析
		HinshiList ohl;
		ohl = *hl;
		ohl.cost_ = SHORT_MAX;
		ohl.table_num_ = size;

		ARRAY_ pre;
		Hinshi hin;
		hin.cost = 0;
//		hin.category = HINSHI( gGrammar.GetDefaultEdge(), gGrammar.GetDefaultEdge() );
		hin.category = HINSHI( 0, 0 );
		HinshiSet set = { &hin, 0, 0 };
		pre.Add( set );

		ARRAY_ post;
			post.Add( set );
		bool flg = true;
		for( Hinshi* h = dicA_.at[ ohl.table_no_ ][ size ].firstHinshi; h; h = h->nextHinshi ){
			flg = false;
			set.left_ = h;
			post.Add( set );
		}

		// ないとき
		if( flg ){
			ohl.hinshi_ = 0;
//			post.Add( set );
		}

		if( wt_it_.byte() < text_len_ ){
			// 選択文節以降の計算
			comeback_cnt = last_cnt = 0;
			hinshi_list_.table_num_ = 0;
			calc_cost_sub_( &hinshi_list_, wt_it_.byte(), text_len_, pre, post );

			// 接続が無かった時に全選択
			if( hinshi_list_.table_num_ == 0 ){
				hinshi_list_.hinshi_ = 0;
				hinshi_list_.table_no_ = wt_it_.byte();
				hinshi_list_.table_num_ = text_len_ - wt_it_.byte();
				hinshi_list_.NewNext()->isend_ = true;
			}

			// 選択文節までを繋げる
			if( !flg ){
				ohl.hinshi_ = (post.GetSelect())? post.GetSelect()->left_: 0;
				if( ohl.hinshi_ == &hin ) ohl.hinshi_ = 0;
			}
			for( hl = &hinshi_list_; hl && !hl->isend_; ) hl = hl->next_;
			*hl = ohl;
		}
		else{	// 最後まである時
			ohl.hinshi_ = dicA_.at[ ohl.table_no_ ][ size ].firstHinshi;
			hinshi_list_ = ohl;
		}
		
		// 値の設定
		set_rep_();
		ret->rep_ = &rep_;
#if _SUPPORTS_EXCEPTION_HANDLING
	}
	catch( bad_alloc ) { return B_NO_MEMORY; }
#endif
	return B_NO_ERROR;
}

// 確定結果の学習
//
//	変換を確定した場合の変換候補の順序を学習する．
//	変換確定後に各文節に対して呼び出す．
//
//	入力:	no( 何番目の文節か．ret配列の1次元目の添字を与える．)
//			num( 何番目の候補が選ばれたか．ret配列の2次元目の点字を与える．)
//
status_t KAnalysis::LearnResult(int32 no, int32 num /* = 0 */ )
{
	assert( 0 <= no && no < rep_.size_ );
	assert( 0 <= num && num < rep_.clauseses_[ no ].size_ );

	KClause& kc = rep_.clauseses_[ no ].clauses_[ num ];

	// kc 荷は当然値が設定されてる
	assert( kc.InitCheck() == B_NO_ERROR );

	status_t ret = B_NO_ERROR;

	// 表記を探す
	if( kc.pos_[0].hinshi_ ){
		Field* fld = kc.pos_[0].hinshi_->firstField;
		short i = 0;
		while( i < kc.pos_[0].fpos_ ){
			fld = fld->nextField;
			++i;
		}

		// 最大頻度を探す
		KClause& kc_max = rep_.clauseses_[ no ].clauses_[ 0 ];

		// 学習
		ret = dicA_.LearnFrequency( &( text_[ kc.pos_[0].wpos_ ] ), kc.pos_[0].hinshi_->length, kc.pos_[0].hinshi_->category,
									fld, kc_max.pos_[0].hinshi_->firstField->evaluation );
		if( ret != B_NO_ERROR ) return ret;
	}
	
	// 最後の品詞セット
	if( kc.pos_[ kc.pos_size_ - 1 ].hinshi_ )
		tail_hinshi_ = POST_HINSHI( kc.pos_[ kc.pos_size_ - 1 ].hinshi_->category );
	else tail_hinshi_ = 9; // 名無しの品詞コード

	return B_NO_ERROR;
}

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
status_t KAnalysis::LearnResult(KClauseArrayArray* ret, int32 no, int32 num /* =0 */ )
{
	status_t rt = LearnResult( no, num );
	if( rt != B_NO_ERROR ) return rt;

	// 変換候補の順序変更( 最後のときは変更の必要なし )
	if( no < rep_.size_ - 1 ){
		int cnt = 0;
		for( int i = 0; i <= no; ++i ){
			cnt += rep_.clauseses_[ i ].word_size_;
		}

		// 呼出
		dicA_.at.Shift( cnt );

		// rep_の調整
		rep_.clauseses_ = &( rep_.clauseses_[ no + 1 ] );
		ret->rep_ = &rep_;

		cnt = rep_.size_ - (no + 1);
		rep_.size_ = cnt;

		// HInshiLIstをずらす
		HinshiList* hl = &hinshi_list_;
		for( ; cnt > 0; --cnt ) hl = hl->next_;
		hl->isend_ = true;
	}

	ret->GetSize();

	return rt;
}

/*--- end of Analysis.cpp ---*/
