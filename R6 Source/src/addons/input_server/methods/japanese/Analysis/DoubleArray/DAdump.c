/************************************************************************
ファイル名	:	DADump.c

作成日	:	97.7.31
************************************************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include"DoubleArray.h"
#include"DAcommon.h"

enum{
	kNextSearch,
	kPreviousSearch
};

static DAErr KeyLinerRetrieve(DicRecord *dr,unsigned short len,const Byte *key,MainRecord *mainRecordData,short mode);
static DAErr FindNextKey(DicRecord *dr,unsigned short blcode,MainRecord *mainRecordData, short *stack, short pos,short nValue);
static DAErr FindFirstKey(DicRecord *dr,unsigned short block,MainRecord *mainRecordData);
static DAErr FindPreviousKey(DicRecord *dr,unsigned short blcode,MainRecord *mainRecordData, short *stack, short pos,short nValue);
static DAErr FindLastKey(DicRecord *dr,unsigned short block,MainRecord *mainRecordData);

/*****************************************************************************************
関数名	:	DAForwardMatch
機能		:	辞書上で次のキーを検索する
入力		:	unsigned short		dicID				: 辞書ID
			unsigned short		keyLength			: キーの長さ
			Byte			*keyData			: キー
			MainRecord	*mainRecordData	: 次のキーとキーに対するデータ
出力		:	DAErr
作成日	:	97.8.7
*****************************************************************************************/
DAErr DAForwardMatch(DicRecord* dr,unsigned short keyLength,const Byte *keyData,MainRecord *mainRecordData)
{
	DAErr		err;

	/* キーサイズのチェック */
	if(keyLength > kKeyMax)return kDANoKeyErr;

	if(keyLength == 0 || keyData == NULL){
		/* 辞書内で最初のキーを返す */
		err = FindFirstKey(dr,0, mainRecordData);
	}
	else{
		err = KeyLinerRetrieve(dr,keyLength,keyData,mainRecordData,kNextSearch);
	}

	if(err == kDANoErr){
		for(int i=0;i<mainRecordData->recordSize;i++)mainRecordData->recordData[i]^=kDicMask;
	}
	
	return err;
}

/*****************************************************************************************
関数名	:	DABackwardMatch
機能		:	辞書上で前のキーを検索する
入力		:	unsigned short		dicID				: 辞書ID
			unsigned short		keyLength			: キーの長さ
			Byte			*keyData			: キー
			MainRecord	*mainRecordData	: 次のキーとキーに対するデータ
出力		:	DAErr
作成日	:	97.8.7
*****************************************************************************************/
DAErr DABackwardMatch(unsigned short dicID,unsigned short keyLength,const Byte *keyData,MainRecord *mainRecordData)
{
	DAErr		err;
	DicRecord		*dr;

	//err = GetDicRecord2DicTable(dicID,&dr);
	return (-1);	// Be FHL: what the hell?
	if(err != kDANoErr)return err;
	if(dr == NULL || dr->refCount == 0){
		/* 辞書がオープンしていない */
//		return kDADicNotOpenErr;
	}

	/* キーサイズのチェック */
	if(keyLength > kKeyMax)return kDANoKeyErr;

	if(keyLength == 0 || keyData == NULL){
		/* 辞書内で最後のキーを返す */
		err = FindLastKey(dr,dr->TblNum-2, mainRecordData);
	}
	else{
		err = KeyLinerRetrieve(dr,keyLength,keyData,mainRecordData,kPreviousSearch);
	}
	
	return err;
}

/******************************************************************************
関数名	:	KeyLinerRetrieve
機能		:	前単語、後単語検索を行う前に入力したキーの遷移をたどる
入力		:	DicRecord		*dr				: 辞書レコード
			short		len				: keyの長さ
			unsigned char		*key				: 検索文字列
			MainRecord	*mainRecordData	: 検索結果のキーとキーに対するデータ
			short		mode				: 検索のモード
						kNextSearch			: 次の単語を検索する
						kPreviousSearch		: 前の単語を検索する
出力		:	DAErr
作成日	:	97.8.7
******************************************************************************/
static DAErr KeyLinerRetrieve(DicRecord *dr,unsigned short len,const Byte *key,MainRecord *mainRecordData,short mode)
{
	short		s, t,				/* 状態番号							*/
				h,				/* 遷移した文字							*/
    				pos,				/* スタックポインタ						*/
    				stack[kKeyMax];	/* 状態推移番号を記憶するスタfoundKeyック	*/
	unsigned short		blcode;			/* ブロック番号							*/
	Byte			ch;				/* 遷移する文字							*/
	Byte			suffix[kKeyMax];	/* 接尾辞								*/
	DAErr		err;
	DataType		cont_size;
	short		suffix_len,nValue;
	Byte			*cont;

	blcode = BCODE(dr,key,len);

	err = ChangeBlock(dr,blcode,true);
	if(err != kDANoErr)return err;

	pos = 0;	/* 初期状態 */
	s = 1;
	h = -1;
	/* 入力キーに対して状態遷移してみる */
	do {
		++h;

		if(h == len){
			h--;
			nValue = kEndCode;
		}
		else{
			ch = key[h];
			nValue = CODE(ch & 0xff);
		}
		t = dr->B_C[2*s] + nValue;

		/* 遷移が存在しない */
		if (t > dr->B_C[0] || t < 1 || dr->B_C[2*t + 1] != s){
			return kDANoKeyErr;	/* キーが存在しない */
		}

		mainRecordData->key[pos] = ch;		/* 遷移した文字を格納する */
		stack[pos] = s;	/* スタックへ状態をプッシュ */

		/* セパレート状態 */
		if(dr->B_C[2 * t] < kNoUseCode) {
			/* 接尾辞とレコードを取り出す */
			R_STR((-1) * dr->B_C[2*t], &cont,&cont_size, suffix,&suffix_len,dr->TAIL);
			/* 入力文字列全体がキーとして存在している */
			if (len == h+suffix_len+1 && !memcmp(suffix, (key + h + 1), suffix_len)){
				if(mode==kNextSearch){
					/* 次キーを探す */
					err = FindNextKey(dr,blcode, mainRecordData,stack, pos,nValue);
				}
				else{
					/* 前キーを探す */
					err = FindPreviousKey(dr,blcode, mainRecordData,stack, pos,nValue);
				}
				return err;
			}
			else{
				/* 接尾辞が異なっていた */
				return kDANoKeyErr;
			}
		}
		pos++;
		s = t;	/* 状態遷移 */
	}while (1);
}

/******************************************************************************
関数名	:	FindNextKey
機能		:	次単語を検索する
入力		:	DicRecord		*dr				: 辞書レコード
			unsigned short		blcode			: ブロック番号
			MainRecord	*mainRecordData	: 検索結果のキーとキーに対するデータ
			short		*stack			: 遷移した状態番号を格納するスタック
			short		pos				: スタックポインタ
			short		nValue			: キーの検索に使用した最後の内部表現値
出力		:	DAErr
作成日	:	97.8.7
******************************************************************************/
static DAErr FindNextKey(DicRecord *dr,unsigned short blcode,MainRecord *mainRecordData, short *stack, short pos,short nValue)
{
	short		i;					/* 遷移する文字	*/
	Byte			suffix[kKeyMax];		/* 接尾辞		*/
	short		s;					/* 現在の状態	*/
	short		t;					/* 次の状態	*/
	DataType		cont_size;
	short		suffix_len;
	Byte 		*cont;

	s = stack[pos];	/* 遷移を開始する状態番号を取り出す */

	i = nValue+1;
  	/* 可能な範囲の文字に対して調べる */
	while(1){
		if (i > kMaxCode) {	/* 今の状態からはこれ以上遷移できない */
			if(pos == 0) {	/* スタックが空 */
				if (++(blcode) >= dr->TblNum-1){
					/* 次のブロックは存在しない */
					mainRecordData->keyLength = 0;
					mainRecordData->recordSize = 0;
					return kDANoErr;
				}
				return FindFirstKey(dr,blcode,mainRecordData);
			}	
			 /* スタックの内容をポップ */
			 i = CODE(mainRecordData->key[--pos])+1;
			 s = stack[pos];
		}
		/* 状態 s の次の状態 */
		t = dr->B_C[2*s] + i;
		if(t > dr->B_C[0]){
			i = kMaxCode+1;
			continue;
		}

		/* 状態遷移可能 */
		if (dr->B_C[2*t+1] == s) {
			mainRecordData->key[pos] = DECODE(i);	/* 遷移した文字を格納する */
			stack[pos] = s;				/* スタックへ状態をプッシュ */

			/* セパレート状態 */
			if (dr->B_C[2*t] < kNoUseCode) {
				/* 接尾辞・レコードの取り出し */
				R_STR((-1) * dr->B_C[2*t], &cont,&cont_size, suffix,&suffix_len,dr->TAIL);
				if(i == kEndCode)
					mainRecordData->keyLength = pos;
				else
					mainRecordData->keyLength = pos+1;
				memmove(mainRecordData->key+mainRecordData->keyLength,suffix,suffix_len);
				mainRecordData->keyLength += suffix_len;
				memmove(mainRecordData->recordData,cont,cont_size);
				mainRecordData->recordSize = cont_size;
				return kDANoErr;	/* 次単語が存在する */
			}
			/* 次の状態に移る */
			if(pos == kKeyMax-1){
				i = kMaxCode+1;
				continue;
			}
			else{
				pos++;
				if(dr->B_C[2 * t] + kMinCode < 2)
					i = 2-dr->B_C[2 * t];
				else
					i = kMinCode;
//				i = kMinCode;
				s = t;	/* 状態遷移 */
				continue;
			}
		}
		i++;
	}
}

/******************************************************************************
関数名	:	FindFirstKey
機能		:	ブロックの１番最初のキーを見つける
入力		:	DicRecord		*dr				: 辞書レコード
			unsigned short		block				: ブロック番号
			MainRecord	*mainRecordData	: ブロック内で最初のキーとデータ
出力		:	DAErr
作成日	:	97.8.7
******************************************************************************/
static DAErr FindFirstKey(DicRecord *dr,unsigned short block,MainRecord *mainRecordData)
{
	short		i;					/* 遷移する文字		*/
	short 		pos;					/* 遷移した文字数	*/
	Byte			suffix[kKeyMax];		/* 接尾辞			*/
	short		s;					/* 現在の状態		*/
	short		t;					/* 次の状態		*/
	DAErr		err;
	DataType		cont_size;
	short		suffix_len;
	Byte			*cont;

	while (dr->TABLE[block].keynum == 0 && block < dr->TblNum-1)
		block++;
	if (block == dr->TblNum-1){
		mainRecordData->keyLength = 0;
		mainRecordData->recordSize = 0;
		return kDANoErr;
	}

	err = ChangeBlock(dr,block,true);
	if(err != kDANoErr)return err;

	pos = 0;	/* 初期化 */
	s = 1;

	if(dr->B_C[2 * s] + kMinCode < 2)
		i = 2-dr->B_C[2 * s];
	else
		i = kMinCode;
//	i = kMinCode;
	/* 可能な範囲の文字に対して調べる */
	while(1){
		/*状態 s の次の状態 */
		t = dr->B_C[2*s] + i;

		if(t > dr->B_C[0]){
			i = kMaxCode;
		}
		else{
			/* 状態遷移可能 */
			if (dr->B_C[2*t+1] == s) {
				mainRecordData->key[pos] = DECODE(i);
	
				/* セパレート状態 */
				if (dr->B_C[2*t] < kNoUseCode) {
					/* 接尾辞・レコードの取り出し */
					R_STR((-1) * dr->B_C[2*t], &cont,&cont_size, suffix,&suffix_len,dr->TAIL);
					if(i == kEndCode)
						mainRecordData->keyLength = pos;
					else
						mainRecordData->keyLength = pos+1;
					memmove(mainRecordData->key+mainRecordData->keyLength,suffix,suffix_len);
					mainRecordData->keyLength += suffix_len;
					memmove(mainRecordData->recordData,cont,cont_size);
					mainRecordData->recordSize = cont_size;
					return kDANoErr;	/* 単語が存在する */
				}
				else{
					/* 次の状態に移る */
					pos++;
					if(dr->B_C[2 * t] + kMinCode < 2)
						i = 2-dr->B_C[2 * t];
					else
						i = kMinCode;
//					i = kMinCode;
					s = t;
					continue;
				}
			}
		}	/* if */
		if (i >= kMaxCode){
			if(pos != 0){
				pos --;
				i = CODE(mainRecordData->key[pos]);
				s = dr->B_C[2*s+1];
			}
			else
				return kDANoKeyErr;
		}
		i++;
	}	/* for */
}

/******************************************************************************
関数名	:	FindPreviousKey
機能		:	前単語を検索する
入力値	:	DicRecord		*dr				: 辞書レコード
			unsigned short		blcode			: ブロック番号
			MainRecord	*mainRecordData	: 検索結果のキーとキーに対するデータ
			short		*stack			: 遷移した状態番号を格納するスタック
			short		pos				: スタックポインタ
			short		nValue			: キーの検索に使用した最後の内部表現値
出力値	:	DAErr
作成日	:	97.8.7
******************************************************************************/
static DAErr FindPreviousKey(DicRecord *dr,unsigned short blcode,MainRecord *mainRecordData, short *stack, short pos,short nValue)
{
	short		i;					/* 遷移する文字	*/
	Byte			suffix[kKeyMax];		/* 接尾辞		*/
	short		s;					/* 現在の状態	*/
	short		t;					/* 次の状態	*/
	DataType		cont_size;
	short		suffix_len;
	Byte			*cont;

	s = stack[pos];	/* 遷移を開始する状態番号を取り出す */

	i = nValue-1;
	/* 可能な範囲の文字に対して調べる */
	while(1){
		if (i < kMinCode) {	/* 今の状態からはこれ以上遷移できない */
			if(pos == 0) {	/* スタックが空 */
				if (blcode == 0){
					/* 前のブロックは存在しない */
					mainRecordData->keyLength = 0;
					mainRecordData->recordSize = 0;
					return kDANoErr;
				}
				blcode--;
				return FindLastKey(dr,blcode, mainRecordData);
			}
			/* スタックの内容をポップ */
			i = CODE(mainRecordData->key[--pos])-1;
			s = stack[pos];
		}
		/* 状態 s の次の状態 */
		t = dr->B_C[2*s] + i;

		if(1 > t){
			i = kMinCode-1;
			continue;
		}
		if(t > dr->B_C[0]){
			i --;
			continue;
		}
		/* 状態遷移可能 */
		if (dr->B_C[2*t+1] == s) {
			mainRecordData->key[pos] = DECODE(i);	/* 遷移した文字を格納する */
			stack[pos] = s;		/* スタックへ状態をプッシュ */

			/* セパレート状態 */
			if (dr->B_C[2*t] < kNoUseCode) {
				/* 接尾辞・レコードの取り出し */
				R_STR((-1) * dr->B_C[2*t], &cont,&cont_size, suffix,&suffix_len,dr->TAIL);
				if(i == kEndCode)
					mainRecordData->keyLength = pos;
				else
					mainRecordData->keyLength = pos+1;
				memmove(mainRecordData->key+mainRecordData->keyLength,suffix,suffix_len);
				mainRecordData->keyLength += suffix_len;
				memmove(mainRecordData->recordData,cont,cont_size);
				mainRecordData->recordSize = cont_size;
				return kDANoErr;	/* 前単語が存在する */
			}
			/* 次の状態に移る */
			if(pos == kKeyMax-1){
				i --;
				continue;
			}
			else{
				pos++;
				i = kMaxCode;
				s = t;	/* 状態推移 */
				continue;
			}
		}
		i--;
	}
}

/******************************************************************************
関数名	:	FinsLastKey
機能		:	ブロックの１番最後のキーを見つける
入力値	:	DicRecord		*dr				: 辞書レコード
			unsigned short		block				: ブロック番号
			Byte			*foundKey		: １番最後のキーを格納する(out)
			MainRecord	*mainRecordData	: ブロック内で最後のキーとデータ
出力値	:	DAErr
作成日	:	97.8.7
******************************************************************************/
static DAErr FindLastKey(DicRecord *dr,unsigned short block,MainRecord *mainRecordData)
{
	short		i;					/* 遷移する文字		*/
	short 		pos;					/* 遷移した文字数	*/
	Byte			suffix[kKeyMax];		/* 接尾辞			*/
	short		s;					/* 現在の状態		*/
	short		t;					/* 次の状態		*/
	DAErr		err;
	DataType		cont_size;
	short		suffix_len;
	Byte			*cont;

	while (dr->TABLE[block].keynum == 0 && block > 0)
		block--;
	if (block == 0 && dr->TABLE[block].keynum == 0){
		mainRecordData->keyLength = 0;
		mainRecordData->recordSize = 0;
		return kDANoErr;
	}

	err = ChangeBlock(dr,block,true);
	if(err != kDANoErr)return err;

	pos = 0;	/* 初期化 */
	s = 1;

	i = kMaxCode;
	/* 可能な範囲の文字に対して調べる */
	while(1){
		/* 状態 s の次の状態 */
		t = dr->B_C[2*s] + i;

		if(t<1){
			i = kMinCode;
		}
		else{
			if(t <= dr->B_C[0]){
				/* 状態遷移可能 */
				if (dr->B_C[2*t+1] == s) {
					mainRecordData->key[pos] = DECODE(i);
		
					/* セパレート状態 */
					if (dr->B_C[2*t] < kNoUseCode) {
					/* 接尾辞・レコードの取り出し */
						R_STR((-1) * dr->B_C[2*t], &cont,&cont_size, suffix,&suffix_len,dr->TAIL);
						if(i == kEndCode)
							mainRecordData->keyLength = pos;
						else
							mainRecordData->keyLength = pos+1;
						memmove(mainRecordData->key+mainRecordData->keyLength,suffix,suffix_len);
						mainRecordData->keyLength += suffix_len;
						memmove(mainRecordData->recordData,cont,cont_size);
						mainRecordData->recordSize = cont_size;
						return kDANoErr;	/* 前単語が存在する */
					}
					else{
						/* 前の状態に移る */
						pos++;
						i = kMaxCode;
						s = t;	/* 状態遷移 */
						continue;
					}
				}
			}
		}
		if (i <= kMinCode){
			if(pos != 0){
				pos --;
				i = CODE(mainRecordData->key[pos]);
				s = dr->B_C[2*s+1];
			}
			else
				return kDANoKeyErr;
		}
		i--;
	}	/* for */
}

/*****************************************************************************************
関数名	:	PrintBC
機能		:	ダブル配列上の次の内容を検索する
入力		:	Dmp		*node		:	ノード情報(out)
			short	*BCBuff		:	BCを指すポインター
			Byte		*TailBuff		:	TAILを指すポインター
出力		:	検索成功時 true
作成日	:	97.7.31
*****************************************************************************************/
Boolean PrintBC(Dmp *node,short *BCBuff,Byte *TailBuff)
{
	short		i;
	Byte			s_temp[kKeyMax];		/* 接尾辞			*/
	short		t;					/* 次の状態		*/
	short		s_temp_len;

	i = node->c;
	/* 可能な範囲の文字に対して調べる */
	while(1){
		if(node->s > kBCSize)
			return false;

		/* 今の状態からはこれ以上遷移できない */
		if (i > kMaxCode) {
			/* スタックが空 */
			if(node->pos == 0)
				return false;

			/* スタックの内容をポップ */
			i = CODE(node->str[--(node->pos)])+1;
			node->s = node->stack[node->pos];
		}
		/* 文字コード i による状態 s の次の状態 */
		t = BCBuff[2*node->s] + i;

		if(t > BCBuff[0]){
			i = kMaxCode+1;
			continue;
		}
		/* 状態遷移可能 */
		if (BCBuff[2*t+1] == node->s) {
			node->str[node->pos] = DECODE(i);
			node->stack[(node->pos)] = node->s;		/* スタックへ状態をプッシュ */

			/* セパレート状態 */
			if (BCBuff[2*t] < kNoUseCode) {
				/* 接尾辞・レコードの取り出し */
				R_STR((-1) * BCBuff[2*t], &(node->cont), &(node->cont_size),s_temp,&s_temp_len, TailBuff);
				if(i == kEndCode)
					node->str_len = node->pos;
				else{
					memmove(node->str+node->pos+1,s_temp,s_temp_len);
					node->str_len = node->pos + s_temp_len+1;
				}
				
				node->c = i+1;
				return true;
			}
			else  { /* 次の状態に移る */
				if(BCBuff[2 * t] + kMinCode < 2)
					i = 2-BCBuff[2 * t];
				else
					i = kMinCode;
//				i = kMinCode;
				node->pos++;
				node->s = t;
				continue;
			}
		}
		i++;
	}
}

