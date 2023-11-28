/*
 *	grammar.dic操作クラス
 *
 *		programmed 1997. 
 */
#include "Grammar.h"

// 品詞間のコストを得る
//
//	input : left( 左の品詞コード ), right( 右の品詞コード )
//	output: left - right間のコスト値
//
short KGrammarDic::GetCost( short left, short right ) const
{
	// 引数チェック
	assert( 0 <= left && left < NHINSHI );
	assert( 0 <= right && right < NHINSHI );

	// コスト表のインデックスを得る．
	left = hinshiTable_[ left ].indexOfCost.left;
	right = hinshiTable_[ right ].indexOfCost.right;
	if( left == -1 || right == -1 )
		return -1;

	// コスト表の範囲チェック
	assert( 0 <= left && left < COST_ROW );
	assert( 0 <= right && right < COST_COL );

	return costTable_[ left ][ right ];
}

// 品詞間の接続を調べる
//
//	input : left( 左の品詞コード ), right( 右の品詞コード )
//	output: left - right間で接続があるか true=ある false=ない
//
bool KGrammarDic::IsConnect( short left, short right ) const
{
	// 引数チェック
	assert( 0 <= left && left < NHINSHI );
	assert( 0 <= right && right < NHINSHI );

	// 接続表のインデックスを得る．
	left = hinshiTable_[ left ].indexOfConnection.left;
	right = hinshiTable_[ right ].indexOfConnection.right;
	if( left == -1 || right == -1 )
		return false;

	// 接続表の範囲チェック
	assert( 0 <= left && left < CONNECT_ROW );
	assert( 0 <= right && right < CONNECT_HINSHI );

	// right(列)のビット位置を得る
	short pos = right / 8;
	short bit = right % 8;

	uchar value = connectTable_[ left ][ pos ];

	return ( value & CONNECT_BIT[ bit ] )? true: false;
}

/*--- end of Grammar.cpp ---*/
