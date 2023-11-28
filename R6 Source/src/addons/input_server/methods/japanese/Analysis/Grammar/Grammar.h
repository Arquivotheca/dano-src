/*
 *	grammar.dic操作クラス
 *
 *		programmed 1997. 
 */
#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include <SupportDefs.h>	// for BeOS
#include <assert.h>

const short	NHINSHI			= 461;	// 品詞の個数
const short	CONNECT_ROW		= 460;	// 接続表の行数
const short	CONNECT_COL		= 59;	// 接続表の列数
const short	CONNECT_HINSHI	= 469;	// CONNECT_COL にある品詞の数
const short	COST_ROW			= 36;	// コスト表の行数
const short	COST_COL			= 35;	// コスト表の列数
const short	FREE_EDGE			= 0;		// 開放端の品詞番号
const short	HEAD_EDGE			= 1;		// 前接端
const uchar	CONNECT_BIT[8]	= { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };	// 接続表の列のビットマスク

enum HinshiProperty {
	content = 1,		// 自立語
	function = 2,		// 付属語
	other = 3		// その他
};

class KGrammarDic {
public:

	// コンストラクタ
	KGrammarDic(){}

	// デストラクタ
	~KGrammarDic(){}

	// 初期化
	//
	//	このオブジェクトを生成後，必ず呼ぶこと．
	//	他のメンバ関数ではチェックしていないので．
	//
	//	98/03/19	無動作になりました
	//
	status_t Init( const char* ){ return B_NO_ERROR; }

	// 品詞のゲタ値を得る
	//
	//	input : code( 品詞コード )
	//	output: codeに対するゲタ値
	//
	short GetBasicCost( short code ) const {
		assert( 0 <= code && code < NHINSHI );
		return hinshiTable_[ code ].basicCostValue;
	}

	// 品詞の属性を得る
	//
	//	input : code( 品詞コード )
	//	output: code に対する属性( content or function or other )
	//
	HinshiProperty GetProperty( short code ) const {
		assert( 0 <= code && code < NHINSHI );
		return (HinshiProperty)( hinshiTable_[ code ].property );
	}

	short GetCost( short left, short right ) const;
	bool IsConnect( short left, short right ) const;

private:

	struct IndexRange {
		short	left;		// 左接続のインデックス
		short	right;		// 右接続のインデックス
	};

	/* 品詞表  */
	struct HinshiTable {
		short		property;			// 自立語，付属語，その他
		short		basicCostValue;		// ゲタ値
		IndexRange	indexOfConnection;	// 接続表へのインデックス
		IndexRange	indexOfCost;		// コスト表へのインデックス
	};

	static HinshiTable	hinshiTable_[ NHINSHI ];
	static uchar		connectTable_[ CONNECT_ROW ][ CONNECT_COL ];	/* 接続表 */
	static short		costTable_[ COST_ROW ][ COST_COL ];			// コスト表

	KGrammarDic( KGrammarDic& );
	KGrammarDic& operator=( KGrammarDic& );

};

extern KGrammarDic gGrammar;

#endif	// _GRAMMAR_H_
/*--- end of Grammar.h ---*/
