/************************************************************************
ファイル名	:	DASearch.c

作成日	:	97.8.1
************************************************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include"DoubleArray.h"
#include"DAcommon.h"

//	TransitPreffixKeyの返値
enum{
	EXIST_ALL,				//	入力接尾辞全体がキーとして存在している
	NOT_EXIST,				//	入力接頭辞がトライに含まれていない
	SEARCHING				//	遷移中
};

static DAErr Retrieve(DicRecord *dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey);
static DAErr LongestMatch(DicRecord *dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey);
static DAErr TrieMatch(DicRecord *dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey);
static DAErr PreffixKeyRetrieve(DicRecord *dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey);
static short TransitPreffixKey(DicRecord *dr,unsigned short keyLength,const Byte *key,unsigned char *foundKey,short *foundKey_len, short *s,unsigned char **cont,DataType *contSize);
static void FindSamePreffixKey(DicRecord *dr,unsigned short keyLength,const Byte *key,short s,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey);
static unsigned short LastBCODE(DicRecord *dr,const Byte *key,short len);
static void PutKeySpec(const Byte *foundKey,unsigned short key_len,Byte *recordData,unsigned short recordSize,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey);

/*****************************************************************************************
関数名	:	DAExactMatch
機能		:	完全一致検索を行う
入力		:	unsigned short		dicID				: 辞書ID
			unsigned short		keyLength			: キーの長さ
			Byte			*key				: キー
			MainRecord	mainRecordData	: 出力結果
			unsigned long		numInfo			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	DAErr
作成日	:	97.8.1
*****************************************************************************************/
DAErr DAExactMatch(DicRecord* dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long numInfo,unsigned long *numKey)
{
	DAErr		err;

	/* キーサイズのチェック */
	if(keyLength > kKeyMax || keyLength == 0)return kDANoErr;

	(*numKey) = 0;

	err = Retrieve(dr,keyLength,key,mainRecordData,numInfo,numKey);

	return err;
}
	
/*****************************************************************************************
関数名	:	DALongestMatch
機能		:	最長一致検索を行う
入力		:	unsigned short		dicID				: 辞書ID
			unsigned short		keyLength			: キーの長さ
			Byte			*key				: キー
			MainRecord	mainRecordData	: 出力結果
			unsigned long		numInfo			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	DAErr
作成日	:	97.8.4
*****************************************************************************************/
DAErr DALongestMatch(unsigned short dicID,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long numInfo,unsigned long *numKey)
{
	DAErr		err;
	DicRecord		*dr;

	(*numKey) = 0;

	//err = GetDicRecord2DicTable(dicID,&dr);
	return (-1);	// Be FHL: what the hell?
	if(err != kDANoErr)return err;
	if(dr == NULL || dr->refCount == 0){
		/* 辞書がオープンしていない */
//		return kDADicNotOpenErr;
	}

	/* キーサイズのチェック */
	if(keyLength == 0)return kDANoErr;
	if(keyLength > kKeyMax)keyLength = kKeyMax;

	err = LongestMatch(dr,keyLength,key,mainRecordData,numInfo,numKey);
	
	return err;
}

/*****************************************************************************************
関数名	:	DATrieMatch
機能		:	前方部分一致検索を行う
入力		:	unsigned short		dicID				: 辞書ID
			unsigned short		keyLength			: キーの長さ
			Byte			*key				: キー
			MainRecord	mainRecordData	: 出力結果
			unsigned long		numInfo			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	DAErr
作成日	:	97.8.4
*****************************************************************************************/
DAErr DATrieMatch(DicRecord* dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long numInfo,unsigned long *numKey)
{
	DAErr		err;

	(*numKey) = 0;

	/* キーサイズのチェック */
	if(keyLength == 0)return kDANoErr;
	if(keyLength > kKeyMax)keyLength = kKeyMax;

	err = TrieMatch(dr,keyLength,key,mainRecordData,numInfo,numKey);

	return err;
}

/*****************************************************************************************
関数名	:	DAPrefixMatch
機能		:	共通接頭辞検索を行う
入力		:	unsigned short		dicID				: 辞書ID
			unsigned short		keyLength			: キーの長さ
			Byte			*key				: キー
			MainRecord	mainRecordData	: 出力結果
			unsigned long		numInfo			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	DAErr
作成日	:	97.8.4
*****************************************************************************************/
DAErr DAPrefixMatch(unsigned short dicID,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long numInfo,unsigned long *numKey)
{
	DAErr		err;
	DicRecord		*dr;

	(*numKey) = 0;

	//err = GetDicRecord2DicTable(dicID,&dr);
	return (-1); 	// Be FHL, what the hell?
	if(err != kDANoErr)return err;
	if(dr == NULL || dr->refCount == 0){
		/* 辞書がオープンしていない */
//		return kDADicNotOpenErr;
	}

	/* キーサイズのチェック */
	if(keyLength > kKeyMax)return kDANoErr;

	err = PreffixKeyRetrieve(dr,keyLength,key,mainRecordData,numInfo,numKey);
	
	return err;
}
	
/*****************
	辞書API以外の関数		
*****************/
/*****************************************************************************************
関数名	:	Retrieve
機能		:	完全一致検索を行う
入力		:	DicRecord		*dr				: 辞書レコード
			unsigned short		keyLength			: キーの長さ
			Byte			*key				: キー
			MainRecord	mainRecordData	: 出力結果
			unsigned long		maxKey			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	DAErr
作成日	:	97.8.1
*****************************************************************************************/
static DAErr Retrieve(DicRecord *dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey)
{
	short		h, s, t;
	Byte			S_TEMP[kKeyMax];
	Byte			ch;
	DataType		cont_size;
	short		s_temp_len,nValue;
	Byte			*CONT;
	unsigned short		blkNo;
	DAErr		err;
	
	blkNo = BCODE(dr,key,keyLength);
	err = ChangeBlock(dr,blkNo,true);
	if(err != kDANoErr)return err;

	s = 1;	/* 初期状態 */

	h = -1;
	do {
		++h;

		if(h == keyLength){
			nValue = kEndCode;
			t = dr->B_C[2*s] + nValue;
	
			/* 現時点でキーが存在する */
			if (1 < t && t <= dr->B_C[0] && dr->B_C[2*t + 1] == s && dr->B_C[2*t] < kNoUseCode){
				R_STR((-1) * dr->B_C[2*t], &CONT,&cont_size, S_TEMP,&s_temp_len,dr->TAIL);
				PutKeySpec(key,keyLength,CONT,cont_size,mainRecordData,maxKey,numKey);
			}
			break;
		}

		ch = *(key + h);
		t = dr->B_C[2*s] + CODE((short) ch & 0xff);

		/* 遷移が存在しない */
		if (1 > t || t > dr->B_C[0] || dr->B_C[2*t + 1] != s)break;

		/* セパレート状態 */
		if(dr->B_C[2 * t] < kNoUseCode){
			R_STR((-1) * dr->B_C[2*t], &CONT,&cont_size, S_TEMP,&s_temp_len,dr->TAIL);
			/* 入力文字列全体がキーとして存在している */
			if (keyLength==h+s_temp_len+1  && !memcmp(S_TEMP, key + h + 1, s_temp_len)){
				/* キーの格納 */
				PutKeySpec(key,keyLength,CONT,cont_size,mainRecordData,maxKey,numKey);
			}
			break; /* 接尾辞が異なっていた */
		}
		s = t;	/* 状態遷移 */
	}while(1);
	
	return kDANoErr;
}

/*****************************************************************************************
関数名	:	LongestMatch
機能		:	最長一致検索を行う
入力		:	DicRecord		*dr				: 辞書レコード
			unsigned short		keyLength			: キーの長さ
			Byte			*key				: キー
			MainRecord	mainRecordData	: 出力結果
			unsigned long		maxKey			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	DAErr
作成日	:	97.8.4
*****************************************************************************************/
static DAErr LongestMatch(DicRecord *dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey)
{
	short		h,s,t,i;
	unsigned short		firstBlk;
	long			BL_CODE,oldBlk = -1;
	Byte			S_TEMP[kKeyMax];
	Byte			*CONT;
	Byte			ch;
	DAErr		err;
	DataType		cont_size;
	short		s_temp_len,nValue;
	short		longest,longestState;

	firstBlk = BCODE(dr,key,1);
	if(keyLength == 1)
		BL_CODE = firstBlk;
	else
		BL_CODE = BCODE(dr,key,keyLength);

	err = ChangeBlock(dr,BL_CODE,true);
	if(err != kDANoErr)return err;

	i = keyLength;

	while(1){
		if(i!=keyLength){
			BL_CODE = BCODE(dr,key,i);
			if(BL_CODE >= oldBlk){
				i--;
				if(i == 0)return kDANoErr;
				continue;
			}
			err = ChangeBlock(dr,BL_CODE,true);
			if(err != kDANoErr)return err;
		}
		oldBlk = BL_CODE;

		s = 1;	/* 初期状態 */

		h=0;
		longest = 0;
		do {
			nValue = kEndCode;	/* 終端語 */
			t = dr->B_C[2*s] + nValue;
	
			/* 現時点でキーが存在する */
			if (1 < t && t <= dr->B_C[0] && dr->B_C[2*t + 1] == s && dr->B_C[2*t] < kNoUseCode){
				longest = h;
				longestState = t;
			}
	
			if(keyLength <= h)break;

			ch = *(key + h);
			t = dr->B_C[2*s] + CODE((short) ch & 0xff);
	
			/* 遷移が存在しない */
			if (1 > t || t > dr->B_C[0] || dr->B_C[2*t + 1] != s)break;
	
			/* セパレート状態 */
			if(dr->B_C[2 * t] < kNoUseCode){
				R_STR((-1) * dr->B_C[2*t], &CONT,&cont_size, S_TEMP,&s_temp_len,dr->TAIL);
	
				/* 入力文字列全体がキーとして存在している */
				if (keyLength>=h+s_temp_len+1 && !memcmp(S_TEMP, key + h + 1, s_temp_len)){
					/* キーの格納 */
					PutKeySpec(key,h + s_temp_len+1,CONT,cont_size,mainRecordData,maxKey,numKey);
					return kDANoErr;
				}
				break;
			}
			s = t;	/* 状態遷移 */
			h++;
		}while(1);

		if(longest != 0){
			t = longestState;
			h = longest;
			R_STR((-1) * dr->B_C[2*t], &CONT ,&cont_size,S_TEMP,&s_temp_len,dr->TAIL);
			PutKeySpec(key,h,CONT,cont_size,mainRecordData,maxKey,numKey);
			return kDANoErr;
		}
		if(BL_CODE <= firstBlk)return kDANoErr;
		i--;
	}
}

/*****************************************************************************************
関数名	:	TrieMatch
機能		:	前方部分一致検索を行う
入力		:	DicRecord		*dr				: 辞書レコード
			unsigned short		keyLength			: キーの長さ
			Byte			*key				: キー
			MainRecord	mainRecordData	: 出力結果
			unsigned long		maxKey			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	DAErr
作成日	:	97.8.4
*****************************************************************************************/
static DAErr TrieMatch(DicRecord *dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey)
{
	short		h,s,t,i;
	unsigned short		lastBlk;
	long			BL_CODE,oldBlk = -1;
	Byte			S_TEMP[kKeyMax];
	Byte			*CONT;
	Byte			ch;
	DAErr		err;
	DataType		cont_size;
	short		s_temp_len,nValue;

	if(keyLength == 0)return kDANoErr;

	lastBlk = BCODE(dr,key,keyLength);

	i=1;
	BL_CODE = BCODE(dr,key,i);	
	err = ChangeBlock(dr,BL_CODE,true);
	if(err != kDANoErr)return err;

	while(1){
		if(i!=1){
			BL_CODE = BCODE(dr,key,i);
			if(BL_CODE <= oldBlk){
				i++;
				continue;
			}

			/* ブロックがメモリー上にない */
			err = ChangeBlock(dr,BL_CODE,true);
			if(err != kDANoErr)return err;
		}
		oldBlk = BL_CODE;
		
		s = 1;	/* 初期状態 */

		h=0;
		do {
			if(h>=i){
				nValue = kEndCode;	/* 終端語 */
				t = dr->B_C[2*s] + nValue;
		
				/* 現時点でキーが存在する */
				if (1 < t && t <= dr->B_C[0] && dr->B_C[2*t + 1] == s && dr->B_C[2*t] < kNoUseCode){
					R_STR((-1) * dr->B_C[2*t], &CONT ,&cont_size,S_TEMP,&s_temp_len,dr->TAIL);
					PutKeySpec(key,h,CONT,cont_size,mainRecordData,maxKey,numKey);
					if((*numKey) == maxKey)return kDANoErr;
				}
			}
	
			if(keyLength <= h)break;

			ch = *((unsigned char *)(key) + h);
			t = dr->B_C[2*s] + CODE((short) ch & 0xff);
	
			/* 遷移が存在しない */
			if (1 > t || t > dr->B_C[0] || dr->B_C[2*t + 1] != s){
				if(h>=i)i=h+1;
				else i++;
				break;
			}
	
			/* セパレート状態 */
			if(dr->B_C[2 * t] < kNoUseCode){
				R_STR((-1) * dr->B_C[2*t], &CONT,&cont_size, S_TEMP,&s_temp_len,dr->TAIL);
	
				/* 入力文字列全体がキーとして存在している */
				if (keyLength>=h+s_temp_len+1 && !memcmp(S_TEMP, ((unsigned char *)(key) + h + 1), s_temp_len)){	
					/* キーの格納 */
					PutKeySpec(key,h + s_temp_len+1,CONT,cont_size,mainRecordData,maxKey,numKey);
					if((*numKey) == maxKey)return kDANoErr;
					i = h+s_temp_len+1;
					break;
				}
				i++;
				break;
			}
			s = t;	/* 状態遷移 */
			h++;
		}while(1);
		if(BL_CODE >= lastBlk)return kDANoErr;
	}
}

/*****************************************************************************************
関数名	:	PreffixKeyRetrieve
機能		:	前方部分一致検索を行う
入力		:	DicRecord		*dr				: 辞書レコード
			unsigned short		keyLength			: キーの長さ
			Byte			*key				: キー
			MainRecord	mainRecordData	: 出力結果
			unsigned long		maxKey			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	DAErr
作成日	:	97.8.4
*****************************************************************************************/
static DAErr PreffixKeyRetrieve(DicRecord *dr,unsigned short keyLength,const Byte *key,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey)
{
	short		s;				/* 状態番号 */
	unsigned short		first_index,		/* 最初のブロック番号 */
				last_index,		/* 最後のブロック番号 */
				i;
	short		state,foundKey_len;
	DAErr		err;
	Byte			*cont;
	Byte			foundKey[kKeyMax];
	DataType		contSize;

	/* ブロック番号を求める */
	first_index = BCODE(dr,key,keyLength);

	last_index = LastBCODE(dr,key,keyLength);

	for(i=first_index;i<=last_index;i++){
		err = ChangeBlock(dr,i,true);
		if(err != kDANoErr)return err;

		state = TransitPreffixKey(dr,keyLength,key, foundKey,&foundKey_len, &s,&cont,&contSize);

		if (state == SEARCHING)
			FindSamePreffixKey(dr,keyLength,key, s,mainRecordData,maxKey,numKey);
		else if(state == EXIST_ALL)
			PutKeySpec(foundKey,foundKey_len,cont,contSize,mainRecordData,maxKey,numKey);

		if((*numKey) == maxKey)return kDANoErr;
	}
	
	return kDANoErr;
}

/*****************************************************************************************
関数名	:	TransitPreffixKey
機能		:	接頭辞キー検索を行う前に入力した接頭辞の遷移をたどる
入力値	:	DicRecord	*dr			: 辞書レコード
			DataSpec	*keyDS		: 接頭辞
			unsigned char	*foundKey	: キーが見つかったとき格納(out)
			short	*foundKey_len	: foundKeyの長さ
			short	*s			: 接頭辞を遷移した最後の状態番号(out)
			unsigned char	**cont		: foundKeyのレコード(out)
出力値	:	EXIST_ALL	: 入力接尾辞全体がキーとして存在している
			NOT_EXIST	: 入力した接頭辞がトライに含まれていない
			SEARCHING	: 遷移中
作成日	:	97.8.4
*****************************************************************************************/
static short TransitPreffixKey(DicRecord *dr,unsigned short keyLength,const Byte *key,unsigned char *foundKey,short *foundKey_len, short *s,unsigned char **cont,DataType *contSize)
{
	short		i;				/* ループカウンタ */
	short		t;				/* 状態番号 */
	Byte			ch;				/* 遷移する文字 */
	Byte			suffix[kKeyMax];	/* 接尾辞 */
	short		suffix_len;

	*s = 1;

	for (i = 0; i < keyLength; i++) {
		/* 入力キーに対して状態遷移してみる */
		ch = ((unsigned char *)key)[i];
		t = dr->B_C[2*(*s)] + CODE((short) ch & 0xff);

		/* 遷移が存在しない */
		if (t > dr->B_C[0] || t < 1 || dr->B_C[2*t + 1] != *s)
			return NOT_EXIST;	/* 接頭辞が異なる */

		/* セパレート状態 */
		if(dr->B_C[2 * t] < kNoUseCode) {
			/* 接尾辞の有無の判定 */
			/* 接尾辞とレコードを取り出す */
			R_STR((-1) * dr->B_C[2*t], cont, contSize,suffix,&suffix_len,dr->TAIL);
			/* 入力接尾辞全体がキーとして存在している */
			if (memcmp((unsigned char *)key+i+1, suffix, keyLength-i-1) == 0){
				memmove(foundKey,key,i+1);
				memmove(foundKey+i+1,suffix,suffix_len);
				(*foundKey_len)=i+suffix_len+1;
				return EXIST_ALL;
			}
			else
				return NOT_EXIST;	/* 接頭辞が異なっていた */
		}
		*s = t;	/* 状態遷移 */
	}
	return SEARCHING;
}


/*****************************************************************************************
関数名	:	FindSamePreffixKey
機能		:	共通接頭辞のキーを求める
入力		:	DicRecord		*dr				: 辞書レコード
			unsigned short		keyLength			: キーの長さ
			Byte			*key				: キー
			short		s				: 遷移を開始する状態番号
			MainRecord	mainRecordData	: 出力結果
			unsigned long		maxKey			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	DAErr
作成日	:	97.8.4
*****************************************************************************************/
static void FindSamePreffixKey(DicRecord *dr,unsigned short keyLength,const Byte *key,short s,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey)
{
	short		i;				/* ループカウンター  				*/
	unsigned char		str[kKeyMax];		/* 推移文字列を記憶するスタック		*/
	short		stack[kKeyMax];	/* 状態推移番号を記憶するスタック	*/
	unsigned char		suffix[kKeyMax];	/* 接尾辞						*/
	unsigned char		*c_temp;			/* レコード					*/
	short		t;				/* 次の状態					*/
	short  		pos;				/* スタックポインタ				*/
	DataType		cont_size;
	short		suffix_len,str_len;
	unsigned char		foundKey[kKeyMax];

	memmove(foundKey,key,keyLength);
	/* 初期化 */
	pos = 0;

	if(dr->B_C[2 * s] + kMinCode < 2)
		i = 2-dr->B_C[2 * s];
	else
		i = kMinCode;
//	i = kMinCode;
	/* 可能な範囲の文字に対して調べる */
	while(1){
		/* 文字 i の入力による状態 s の次の状態 */
		t = dr->B_C[2*s] + i;

		if(t > dr->B_C[0]){
			i = kMaxCode;
		}
		else{
			/* 状態遷移可能 */
			if (dr->B_C[2*t+1] == s) {
				str[pos] = DECODE(i);
				stack[pos] = s;		/* スタックへ状態をプッシュ */
	
				/* セパレート状態 */
				if (dr->B_C[2*t] < kNoUseCode) {
					/* 接尾辞・レコードの取り出し */
					R_STR((-1) * dr->B_C[2*t], &c_temp,&cont_size, suffix,&suffix_len,dr->TAIL);
					if(i==kEndCode)
						str_len = pos;
					else
						str_len = pos+1;
					memmove(foundKey+keyLength,str,str_len);
					memmove(foundKey+keyLength+str_len,suffix,suffix_len);
					PutKeySpec(foundKey,keyLength+str_len+suffix_len,c_temp,cont_size,mainRecordData,maxKey,numKey);
					if((*numKey) == maxKey)return;
				}
				else  { /* 次の状態に移る */
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
		}
		if (i >= kMaxCode) {	/* 今の状態からはこれ以上遷移できない */
			if(pos == 0)	/* スタックが空 */
				return;	/* 全てのキーの遷移を終了 */
			/* スタックの内容をポップ */
			i = CODE(str[--pos]);
			s = stack[pos];
		}
		i++;
	}
}

/*****************************************************************************************
関数名	:	LastBCODE
機能		:	検索文字列を接頭語とする語が含まれている最後のブロック番号を求める
入力		:	DicRecord	*dr		: 辞書レコード
			Byte		*key		: 検索文字列
			short	len		: keyの長さ
出力		:	ブロック番号
作成日	:	97.8.1
修正日	:	97.8.4
*****************************************************************************************/
static unsigned short LastBCODE(DicRecord *dr,const Byte *key,short len)
{
	short		i;
	unsigned char		s[kKeyMax];

	memmove(s,key,len);
	for(i=len;i<kKeyMax;i++)
		s[i] = 0xff;

	return BCODE(dr,s,kKeyMax);
}

/*****************************************************************************************
関数名	:	PutKeySpec
機能		:	検索結果を格納する
入力		:	Byte			*foundKey		: 検索されたキー
			unsigned short		key_len			: foundKeyの長さ
			Byte			*recordData		: 検索されたキーに対するデータ
			unsigned short		recordSize		: recordDataの長さ
			MainRecord	mainRecordData	: 出力結果
			unsigned long		maxKey			: 希望する検索結果の最大値
			unsigned long		*numKey			: 検索結果の個数
出力		:	なし
作成日	:	97.8.1
修正日	:	97.12.17
*****************************************************************************************/
static void PutKeySpec(const Byte *foundKey,unsigned short key_len,Byte *recordData,unsigned short recordSize,MainRecord mainRecordData[],unsigned long maxKey,unsigned long *numKey)
{
	int	i;

	if(maxKey != 0 && mainRecordData != NULL){
		mainRecordData[*numKey].keyLength = key_len;
		memmove(mainRecordData[*numKey].key,foundKey,key_len);
		mainRecordData[*numKey].recordSize = recordSize;
		memmove(mainRecordData[*numKey].recordData,recordData,recordSize);
		for(i=0;i<recordSize;i++)mainRecordData[*numKey].recordData[i]^=kDicMask;
	}

	(*numKey) ++;
}
