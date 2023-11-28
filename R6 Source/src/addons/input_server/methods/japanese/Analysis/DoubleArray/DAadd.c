/************************************************************************
ファイル名	:	DAAdd.c

作成日	:	97.7.30
************************************************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include"DoubleArray.h"
#include"DAcommon.h"

#define kListMax			257				/*トライのノードからのリンクの最大数		*/

static DAErr k_insert(DicRecord *dr,const Byte *key,unsigned short key_len,const Byte *data,unsigned short dataSize);
static Boolean BC_INSERT(DicRecord *dr,short s,const Byte *b_ch,short b_ch_len,const Byte *cont,DataType cont_size);
static Boolean MODIFY(DicRecord *dr,short current, short s,short nValue,short *list,short list_len,short *newPos);
static Boolean TAIL_INSERT(DicRecord *dr,short s,Byte *a_ch, short a_ch_len,const Byte *b_ch,short b_ch_len,
							Byte *a_cont, DataType a_cont_size,const Byte *b_cont,DataType b_cont_size);
static short SET_LIST(DicRecord *dr,short *list, short s);
static Boolean INS_STR(DicRecord *dr,const Byte *str,short str_len,const Byte *cont, DataType cont_size,short s, short pos);
static short X_CHECK(DicRecord *dr,short *list,short list_len);
static Boolean W_STR(DicRecord *dr,const Byte *str,short str_len,const Byte *cont,DataType cont_size, short pos);
static DAErr BlkOver(DicRecord *dr,unsigned short blkNo);
static Boolean DivideBCT(DicRecord *dr,unsigned short initnum, unsigned short *lastnum, unsigned short blkNo, Dmp *node, long bcaddr,short *BCBuff,Byte *TailBuff);

/*******************************************************************
関数名	:	DAUpdateMainRecord
機能		:	辞書にキー・データを追加する
入力		:	unsigned short	dicID			: 辞書ID
			unsigned short	keyLength		: キーの長さ
			Byte		*key			: キー
			unsigned short	recordSize	: レコードのサイズ
			Byte		*recordData	: レコード
出力		:	DAErr
作成日	:	97.7.27
修正日	:	97.12.22
*******************************************************************/
DAErr DAUpdateMainRecord(DicRecord *dr,unsigned short keyLength,const Byte *key,unsigned short recordSize,const Byte *recordData)
{
	DAErr		err;
	Byte			maskData[kDataMax];
	int			i;
	
	/* キーサイズのチェック */
	if(keyLength > kKeyMax || keyLength == 0)return kDABadKeySizeErr;

	/* データサイズのチェック */
	if(recordSize > kDataMax)return kDABadRecordSizeErr;

	if(dr->TblNum == dr->maxBlock-1)return kDABlockFullErr;

	/* レコードにマスクをかける */
	memcpy(maskData,recordData,recordSize);
	for(i=0;i<recordSize;i++)maskData[i]^=kDicMask;

	err = k_insert(dr,key,keyLength,maskData,recordSize);

	return err;
}

/*****************************************************************************************
関数名	:	k_insert
機能		:	ダブル配列にキーを挿入する
入力		:	DicRecord			*dr		: 辞書レコード
			Byte				*key		: 挿入文字列
			short			key_len	: keyの長さ
			Byte				*data	: データ
			unsigned short			dataSize	: データのサイズ
出力		:	DAErr
作成日	:	97.7.30
修正日	:	97.8.7
*****************************************************************************************/
static DAErr k_insert(DicRecord *dr,const Byte *key,unsigned short key_len,const Byte *data,unsigned short dataSize)
{
	unsigned short		blkNo;
	short		insert_ret;
	DAErr		err;

	blkNo = BCODE(dr,key,key_len);
	err = ChangeBlock(dr,blkNo,false);
	if(err != kDANoErr)return err;
		
	while(1){
		dr->BC_MAX = dr->B_C[0];
		dr->TA_POS = dr->B_C[1];
	
		/* キー挿入 */
		if((insert_ret=UbcInsert(dr,key,key_len,data, dataSize))!=kAddFail){
			if(insert_ret == kKeyAddSuccess)
				dr->TABLE[blkNo].keynum ++;
		}
		dr->block.writeFlg = true;
	
		/* ブロックがいっぱい */
		if((dr->BC_MAX+1)*2*sizeof(short) + dr->TA_POS > kBlockSize || insert_ret == kAddFail){
			W_BASE(dr,0, dr->BC_MAX);
			err = BlkOver(dr,blkNo);
			if(err != kDANoErr){
				dr->block.same_c = kNoBlock;
				return err;
			}
		}
		else{
			dr->TABLE[blkNo].bcsize = (dr->BC_MAX+1)*2*sizeof(short);
			W_BASE(dr,0, dr->BC_MAX);
			W_CHECK(dr,0, dr->TA_POS);
		}

		if(insert_ret != kAddFail)break;

		blkNo = BCODE(dr,key,key_len);

		/* ブロックの変更 */
		err = ChangeBlock(dr,blkNo,false);
		if(err != kDANoErr)return err;
	}

	return err;
}

/*****************************************************************************************
関数名	:	UbcInsert
機能		:	ダブル配列の登録を行なう
入力		:	DicRecord	*dr			: 辞書レコード
			Byte		*key			: 登録される文字列
			short	key_len		: keyの長さ
			Byte		*cont		: 追加するデータ
			short	cont_size		: contのサイズ
出力		:	kAddFail				: 登録失敗
			kKeyAddSuccess		: キーの追加成功
			kDataAddSuccess		: 存在しているキーにデータ追加成功
作成日	:	97.7.31
修正日	:	97.12.25
*****************************************************************************************/
short UbcInsert(DicRecord *dr,const Byte *key,short key_len,const Byte *cont,DataType cont_size)
{
	Byte			s_temp[kKeyMax];				//	接尾辞
	Byte			*c_temp;						//	キーに対して既に格納されているデータ
	Byte			ch;							//	遷移中の文字
	short		h, 							//	遷移しているキー位置
				s, 							//	状態番号
				t;							//	次の状態番号
	short		s_temp_len,					//	s_tempの長さ
				nValue;						//	遷移中の文字に対する内部表現値
	DataType		c_temp_len;					//	c_tempの長さ

	/* 初期化 */
	s = 1;
	h = 0;

	/* 遷移をたどる */
	do {
		/* 入力キーに対して状態遷移してみる */
		if(h == key_len)
			nValue = kEndCode;
		else{
			ch = *(key + h);
			nValue = CODE((short) ch & 0xff);
		}

		t = dr->B_C[2 * s] + nValue;

		/* 状態遷移できない */
		if (t < 1 || dr->B_C[2 * t + 1] != s) {

			/* ダブル配列上に挿入 */
			if(BC_INSERT(dr,s, key + h, key_len-h, cont,cont_size))
				return kKeyAddSuccess;
			return kAddFail;
		}

		/* セパレート状態 */
		if (dr->B_C[2 * t] < kNoUseCode)break;

		/* 状態遷移 */
		s = t;
		++h;
	}
	while(1);

	/* 接尾辞とレコードを取り出す */
	R_STR((-1) * dr->B_C[2*t], &c_temp, &c_temp_len,s_temp,&s_temp_len,dr->TAIL);

	if (h == key_len || ((key_len == h+s_temp_len+1) && !memcmp(s_temp, key + h + 1,s_temp_len))) {
		/* すでにキーが登録されている */

		if(c_temp_len < cont_size){
			/* 新しく追加するデータのサイズの方が大きい */
			if((-1) * dr->B_C[2*t]-kTailOffset + 1+s_temp_len+sizeof(DataType)+c_temp_len == dr->TA_POS){
				/* 元のデータはTAILの最後のデータ */
				if(W_STR(dr,s_temp,s_temp_len, cont,cont_size, (-1) * dr->B_C[2*t]-kTailOffset) == true)
					return kDataAddSuccess;
			}
			else{
				/* 新たなレコードポインタ */
				W_BASE(dr,t, (-1) * dr->TA_POS-kTailOffset);
				/* 新たなレコードを格納	  */
				if(W_STR(dr,s_temp,s_temp_len, cont,cont_size, dr->TA_POS) == true)
					return kDataAddSuccess;
			}
			return kAddFail;
		}
		else{
			/* 元の位置に新しいデータを格納する */
			if(W_STR(dr,s_temp,s_temp_len, cont,cont_size, (-1) * dr->B_C[2*t]-kTailOffset) == true)
				return kDataAddSuccess;
			return kAddFail;
		}
	}
	else {
		/* 取り出した接尾辞と入力されたキーに対し状態遷移を作る */
		if(TAIL_INSERT(dr,t, s_temp,s_temp_len, key+h+1,key_len-h-1, c_temp, c_temp_len,cont,cont_size))
			return kKeyAddSuccess;
		return kAddFail;
	}
}

/*****************************************************************************************
関数名	:	BC_INSERT
機能		:	ダブル配列上で遷移できなくなった場合の挿入を行なう
入力		:	DicRecord	*dr			: 辞書レコード
			short	s			: これ以上遷移できない状態の番号
			Byte		*b_ch		: 残りの文字列
			short	b_ch_len		: b_chの長さ
			Byte		*cont		: レコード
			DataType	cont_size		: contのサイズ
出力		:	挿入成功時 true
作成日	:	97.7.31
*****************************************************************************************/
static Boolean BC_INSERT(DicRecord *dr,short s,const Byte *b_ch,short b_ch_len,const Byte *cont,DataType cont_size)
{
	Byte			ch;				//	遷移中の文字
	short		list[kListMax];		//	状態 s から遷移可能な要素集合
	short		t_list[kListMax];	//	状態 t から遷移可能な要素集合
	short		t;				//	状態 s から ch により遷移する状態の番号
	short		pos; 				//	新たな状態遷移を作る位置
	short		nValue,			//	遷移中の文字に対する内部表現値
				t_list_len,		//	t_listの要素数
				list_len;			//	listの要素数
	Boolean		ret=true;			//	返値


	if(b_ch_len == 0)
		nValue = kEndCode;
	else{
		ch = *b_ch;
		nValue = CODE((short) ch & 0xff);
	}
	/* 状態 s の次の状態を得る */
	t = dr->B_C[2*s] + nValue;

	if(t < 2){
		/* 状態 t は，使えない状態番号 */
		/* sから遷移可能な要素集合を得る */
		list_len = SET_LIST(dr,list, s);
		/* 状態 s を移動する */	
		list[list_len] = nValue;
		list_len++;
		ret = MODIFY(dr,s, s, nValue,list,list_len,&pos);
	}
	else{
		if (dr->B_C[2*t + 1] != kNoUseCode) {
			/* 状態 t はすでに使われている */
			/* s, t それぞれの遷移可能な要素集合を得る */
			list_len	= SET_LIST(dr,list, s);
			t_list_len	= SET_LIST(dr,t_list, dr->B_C[2*t + 1]);

			/* 遷移数の少ないほうを変更する */
			if (list_len < t_list_len){
				/* 状態 s が移動する */		
				list[list_len] = nValue;
				list_len++;
				ret = MODIFY(dr,s, s, nValue,list,list_len,&pos);
			}
			else
				/* 状態 t に移動してもらう */
				ret = MODIFY(dr,s, dr->B_C[2*t + 1],kNoUseCode,t_list,t_list_len,&pos);
		}
		else{
			/* 状態 t は利用可能 */
			pos = s;
		}
	}

	/* pos から新たな状態遷移を作り、接尾辞とレコードを格納する */
	if(ret)
		ret = INS_STR(dr,b_ch,b_ch_len, cont,cont_size, pos, 0);
	
	return ret;
}

/*****************************************************************************************
関数名	:	MODIFY
機能		:	ダブル配列の部分的な修正を行なう
入力		:	DicRecord		*dr			: 辞書レコード
			short		current		: 衝突の起こった状態の番号
			short		s			: 変更される状態の番号
			short		nValue		: 残りの文字列の内部表現値
			short		*list			: 登録前の状態 s の次の状態遷移が可能な要素集合
			short		list_len		: listの長さ
			short		*newPos		: 新しい状態番号(out)
出力		:	修正成功時 true
作成日	:	97.7.31
*****************************************************************************************/
static Boolean MODIFY(DicRecord *dr,short current, short s,short nValue,short *list,short list_len,short *newPos)
{
	short	i,			//	listの要素に対するループ用
			j, 			//	全内部表現値のループ用
			k, 			//	ttからの状態番号
			t, 			//	変更前のsからの遷移に対する状態番号
			tt, 			//	変更後のsからの遷移に対する状態番号
			old_base;		//	変更前の状態sのBASE値

	/* 登録前の状態値を得る */
	old_base = dr->B_C[2*s];

	/* list に含まれるすべての要素による状態遷移を満足する状態値を得る */
	if(W_BASE(dr,s, X_CHECK(dr,list,list_len))==false)return false;

	/* すべての要素に対して新しい遷移を作る */
	for(i=0;list[i]!=nValue && i < list_len;i++){
		/* 前の遷移を得る(すでにsの状態値は書き換えられている) */
		t = old_base + list[i];

		/* 新たな遷移を作る */
		tt = dr->B_C[2 * s] + list[i];
		if(W_BASE(dr,tt, dr->B_C[2 * t])==false)return false;
		if(W_CHECK(dr,tt, s)==false)return false;

		if (dr->B_C[2 * tt] > kNoUseCode) {
			/* 状態ttからの遷移全てを変更する */
			if(dr->B_C[2 * tt] + kMinCode < 2)
				j = 2-dr->B_C[2 * tt];
			else
				j = kMinCode;
			for(;j<=kMaxCode;j++){
				k = dr->B_C[2 * tt] + j;
				if(k>dr->BC_MAX)break;
				if (dr->B_C[2 * k + 1] == t)
					if(W_CHECK(dr,k, tt)==false)break;
			}
		}

		/* 前の遷移を削除する */
		DeleteState(dr,t);
		if (current != s && t == current)
			current = tt;
	}
	(*newPos)=current;

	return true;
}

/*****************************************************************************************
関数名	:	TAIL_INSERT
機能		:	B_C 上に等しい接頭辞を持ち、接尾辞が異なった場合の挿入を行なう。
			新たにセパレート状態を作り、レコードを格納する
入力		:	DicRecord	*dr			: 辞書レコード
			short	s			: 接頭辞に対し遷移を行なったときのの最後の状態
			Byte		*a_ch		: すでに挿入されていたキーの接尾辞
			short	a_ch_len		: a_chの長さ
			Byte		*b_ch		: 新たに挿入されるキーの接尾辞
			short	b_ch_len		: b_ch
			Byte		*a_cont		: すでに挿入されていたキーのレコード
			DataType	a_cont_size	: a_contのサイズ
			Byte		*b_cont		: 新たに挿入されるキーのレコード
			DataType	b_cont_size	: b_contのサイズ
出力		:	挿入成功時 true
作成日	:	97.7.31
*****************************************************************************************/
static Boolean TAIL_INSERT(DicRecord *dr,short s,Byte *a_ch, short a_ch_len,const Byte *b_ch,short b_ch_len,
							Byte *a_cont, DataType a_cont_size,const Byte *b_cont,DataType b_cont_size)
{
	short		list[kListMax],list_len;
	short		t, d_pos;
	short		i, j;
	Boolean		ret;

	/* 初期化 */

	/* 接尾辞の等しい部分の長さを得る */
	j = 0;
	while (j<a_ch_len && j<b_ch_len && *(a_ch+j) == *(b_ch+j))j++;

	/* 既存のキーのレコードの位置を得る */
	d_pos = (-1) * (dr->B_C[2 * s]+kTailOffset);

	/* 接尾辞の等しい部分に対して遷移を作る */
	list_len = 1;
	for(i=0;i < j;i++) {
		list[0] = CODE((*(a_ch + i) & 0xff));
		if(W_BASE(dr,s, X_CHECK(dr,list,list_len))==false)return false;
		t = dr->B_C[2 * s] + list[0];
		if(W_CHECK(dr,t, s)==false)return false;
		s = t;
	}

	/* 異なる接尾辞に対して遷移を作れる状態を探す */
	if(j == a_ch_len)
		list[0] = kEndCode;
	else
		list[0] = CODE((*(a_ch+j) & 0xff));
	if(j == b_ch_len)
		list[1] = kEndCode;
	else
		list[1] = CODE((*(b_ch+j) & 0xff));
	list_len=2;
	ret = W_BASE(dr,s, X_CHECK(dr,list,list_len));

	/* 残りの接尾辞とレコードを挿入する */
	if(ret)
		ret = INS_STR(dr,a_ch+j,a_ch_len-j, a_cont,a_cont_size, s, d_pos);
	if(ret)
		ret = INS_STR(dr,b_ch+j,b_ch_len-j, b_cont,b_cont_size, s, 0);
		
	return ret;
}

/*****************************************************************************************
関数名	:	SET_LIST
機能		:	入力された状態番号 s で示される状態に対して
			遷移可能な要素の集合を求める
入力		:	DicRecord	*dr		: 辞書レコード
			short	*list		: 要素列
			short	s		: 状態番号
出力		:	なし
作成日	:	97.7.31
*****************************************************************************************/
static short SET_LIST(DicRecord *dr,short *list, short s)
{
	short	i; 	/* 要素 					*/
	short	j; 	/* 要素の数				*/
	short	t; 	/* 要素 i に対する s の次の状態	*/

	j = 0;
	/* 要素集合の取りえる範囲で探す */
	if(dr->B_C[2 * s] + kMinCode < 2)
		i = 2-dr->B_C[2 * s];
	else
		i = kMinCode;
	for (; i <= kMaxCode; i++) {
		/* i に対する次の状態を得る */
		t = dr->B_C[2 * s] + i;
		if(t>dr->BC_MAX)break;

		/* 状態 s から遷移できる(t のチェックが s ) */
		if (dr->B_C[2 * t + 1] == s)
			*(list + (j++)) = i;
	}
	return j;
}

/*****************************************************************************************
関数名	:	INS_STR
機能		:	接尾辞 str に対して状態番号 s から新たに遷移可能な
			セパレート状態を作り、接尾辞とレコードを格納する
入力		:	DicRecord		*dr			: 辞書レコード
			Byte			*str			: 接尾辞
			short		str_len		: strの長さ
			Byte			*cont		: レコード
			DataType		cont_size		: contのサイズ
			short		s			: 状態番号
			short		pos			: TAIL 上の接尾辞とレコードの位置、ただし値が
									  ０の場合、最後に挿入される
出力		:	格納成功時 true
作成日	:	97.7.31
*****************************************************************************************/
static Boolean INS_STR(DicRecord *dr,const Byte *str,short str_len,const Byte *cont, DataType cont_size,short s, short pos)
{
	short		t,nValue;
	Boolean		ret;

	/* 状態 s から str の先頭の文字に対して遷移を作る */
	if(str_len == 0)
		nValue = kEndCode;
	else{
		nValue = CODE((short) *str & 0xff);
		str++;
		str_len--;
	}
		
	t = dr->B_C[2 * s] + nValue;
	if(W_CHECK(dr,t, s)==false)return false;

	if (pos == 0) {
		/* TAIL の最後に挿入 */
		ret = W_BASE(dr,t, (-1) * dr->TA_POS-kTailOffset);
		if(ret)
			ret = W_STR(dr,str,str_len, cont,cont_size, dr->TA_POS);
	}
	else {
		/* 指定された位置にポインタをつなぐ */
		ret = W_BASE(dr,t, (-1) * pos-kTailOffset);

		if(ret){
			/* 前の接尾辞から一文字減らして上書きする */
			ret = W_STR(dr,str,str_len, cont,cont_size, pos);
		}
	}
	if(!ret)
		W_BASE(dr,t, kNoUseCode);

	return ret;
}

/*****************************************************************************************
関数名	:	X_CHECK
機能		:	与えられた要素すべてに遷移可能で空いている
			最小の状態番号を求める
入力		:	DicRecord		*dr		: 辞書レコード
			short		*list		: 要素列
			short		list_len	: listの長さ
出力		:	空いている最小の状態番号
作成日	:	97.7.31
*****************************************************************************************/
static short X_CHECK(DicRecord *dr,short *list,short list_len)
{
	short	ss; 	/* 状態 s 次の状態のチェックの値	*/
	short	s; 	/* 状態番号					*/
	short	i; 	/* ループカウンター				*/

#if 1
	short	min;

	min = kMaxCode+1;
	for(i=0;i<list_len; i++)
		if(min > list[i])
			min = list[i];
	/* 初期化 */
	s = 3-min;
#else
	/* 初期化 */
	s = 2;
#endif

	/* 要素集合内のすべての要素に対して */
	for (i = 0; i<list_len; i++) {
		ss = dr->B_C[2 * (s + list[i]) + 1];

		/* コード list[i] に対して遷移した状態がすでに使われている */
		if (ss != kNoUseCode) {

			/* 次の状態に対してやり直し */
			s++;
			i = -1;
		}
	}
	return s;
}

/*****************************************************************************************
関数名	:	W_STR
機能		:	セパレート状態の接尾辞とレコードを TAIL 上の
			pos の位置に書き込む
入力		:	DicRecord	*dr			: 辞書レコード
			Byte		*str			: 接尾辞
			short	str_len		: 接尾辞の長さ
			Byte		*cont		: 追加するレコード
			short	cont_size		: contのサイズ
			short	pos			: TAIL 上の書き込み位置
出力		:	書き込み成功時 true
作成日	:	97.7.31
修正日	:	97.12.17
*****************************************************************************************/
static Boolean W_STR(DicRecord *dr,const Byte *str,short str_len,const Byte *cont,DataType cont_size, short pos)
{
	short	i;
	short	tail_len;
#if kData2Bytes													//add for Intel
	short	tmp;
#endif

	tail_len = 1+str_len+sizeof(DataType)+cont_size;

	/* 挿入すると TAIL の大きさを越える */
	if (pos+tail_len > kTailSize)
		return false;

	dr->TAIL[pos] = str_len;
	memmove(dr->TAIL+pos+1,str,str_len);
	/* 接尾辞にマスクをかける */
	for(i=0;i<str_len;i++)
		dr->TAIL[pos+1+i]^=kDicMask;

#if kData2Bytes													//add for Intel
	tmp = B_HOST_TO_LENDIAN_INT16(cont_size);
	memmove(dr->TAIL+pos+(1+str_len),&tmp,sizeof(DataType));
#else
	memmove(dr->TAIL+pos+(1+str_len),&cont_size,sizeof(DataType));
#endif
	memmove(dr->TAIL+pos+(1+str_len)+sizeof(DataType),cont,cont_size);

	/* 新たな書き込み位置を得る */
	if (pos + tail_len  > dr->TA_POS)
		dr->TA_POS = pos + tail_len;

	return true;
}

/*****************************************************************************************
関数名	:	BlkOver
機能		:	ブロックにダブル配列が書き込めないときの処理を行なう
入力		:	DicRecord		*dr		: 辞書レコード
			unsigned short		blkNo	: あふれを起こしたブロックのブロック番号
出力		:	DAErr
作成日	:	97.7.31
修正日	:	98.2.8
*****************************************************************************************/
static DAErr BlkOver(DicRecord *dr,unsigned short blkNo)
{
	unsigned short		i;			/* ループカウンタ */
	unsigned short		oldkeynum;	/* あふれブロックのキーの数 */
	unsigned short		lastkeynum;
	Dmp			node;		/* ブロックダンプに用いるノード情報 */
	short		*BCBuff;	/* BASE-CHECK Memory	*/
	Byte			*TailBuff;	/* Single String Memory	*/
	DAErr		err = kDANoErr;
	Boolean		ret = true;
	long			old_bcaddr,new_bcaddr;
	unsigned short		dist,oldIndex;

	if(dr->TABLE[blkNo].keynum == 1){
		TailBuff = (Byte *)malloc(sizeof(Byte)*kTailSize);
		if(TailBuff == NULL){
			return kDAMemErr;
		}
		ret = TailGab(dr,TailBuff);
		free(TailBuff);
		if(ret == true){
			dr->block.writeFlg = true;
			return kDANoErr;
		}
			
		return kDABadRecordSizeErr;
	}

	if (dr->TblNum+1 >= dr->maxBlock){
		/* これ以上ブロックを増やせない */
		return kDABlockFullErr;
	}
	
	if(dr->block.addBlockPtr != NULL && dr->block.addBlockPtr->index == dr->TABLE[blkNo].index){
		/* キャッシュブロックの内容をつぶす */
		SetLastBlockCache(dr,dr->block.addBlockPtr);
		dr->block.addBlockPtr = NULL;
	}

	BCBuff = (short *)malloc(sizeof(short)*kBCSize);
	if(BCBuff == NULL)return kDAMemErr;
	TailBuff = (Byte *)malloc(sizeof(Byte)*kTailSize);
	if(TailBuff == NULL){
		free(BCBuff);
		return kDAMemErr;
	}
	
	/* あふれブロックのキーの数等を待避する */
	oldkeynum = dr->TABLE[blkNo].keynum;
	old_bcaddr = dr->TABLE[blkNo].bcaddr;
	oldIndex = dr->TABLE[blkNo].index;

	/* テーブル情報を１つずつずらす */
//	memmove(&dr->First[(blkNo+2)*kUpperByte],&dr->First[(blkNo+1)*kUpperByte],(dr->TblNum-blkNo-1)*kUpperByte);
	memmove(&dr->TABLE[blkNo+2],&dr->TABLE[blkNo+1],(dr->TblNum-blkNo-1)*sizeof(struct TBCS));

	/* あふれブロックが格納されたインデックスと次のインデックスを初期化する */
	for (i = blkNo; i <= blkNo+1; i++) 
		dr->TABLE[i].keynum = 0;

	dist = dr->TblNum - blkNo;
	/* あふれブロックのキーの前半を再構築する */
//	new_bcaddr = (dr->TblNum-1) * kBlockSize;
	err = DicGetSize(dr,&new_bcaddr);
	if(err != kDANoErr)return err;
//	new_bcaddr -= dr->DaPos;

	memmove(BCBuff,dr->B_C,kBCSize*sizeof(short));
	memmove(TailBuff,dr->TAIL,kTailSize);
	/* ノード情報の初期化 */
	node.s = 1;
	node.pos = 0;
	if(BCBuff[2 * node.s] + kMinCode < 2)
		node.c = 2-BCBuff[2 * node.s];
	else
		node.c = kMinCode;

	lastkeynum = oldkeynum/2;
	ret = DivideBCT(dr,0, &lastkeynum, blkNo, &node, new_bcaddr,BCBuff,TailBuff);
	dr->TABLE[blkNo].index = dr->TblNum-1;

	err = W_BCTF(dr,blkNo,true);
	if(err == kDANoErr){
		blkNo = dr->TblNum - dist;
		err = DicSetSize(dr,new_bcaddr + kBlockSize);
//		err = DicSetSize(dr,dr->DaPos + new_bcaddr + kBlockSize);
	}

	if(err == kDANoErr && ret){
		/* あふれブロックのキーの後半を再構築する */
		DivideBCT(dr,lastkeynum, &oldkeynum, blkNo+1, &node, old_bcaddr,BCBuff,TailBuff);
		dr->TABLE[blkNo+1].index = oldIndex;

		/*　現在メモリ上には新しいブロックのダブル配列が存在するので
		　　新しいブロックが格納されたインデックスを oldblkNo に格納する */
		err = W_BCTF(dr,blkNo+1,true);
		dr->block.same_c = blkNo+1;
	
		dr->TblNum++;	/* あふれ処理によりブロック数が１つ増加 */
	
		/* テーブル情報を格納する */
		if(err == kDANoErr)
			err = W_TABLE(dr,blkNo);
		else
			W_TABLE(dr,blkNo);
	}

	free(BCBuff);
	free(TailBuff);

	if(err != kDANoErr || !ret){
		SetUpDic(dr);
	}

	return err;
}

/*****************************************************************************************
関数名	:	DivideBCT
機能		:	ブロックにダブル配列が書き込めないときの処理を行なう
入力		:	DicRecord		*dr			: 辞書レコード
			unsigned long		initnum		: ループ初期値
			unsigned long		*lastnum		: ループ最終値
			unsigned short		blkNo		: インデックス
			Dmp			*node		: ノード情報
			long			bcaddr		: 辞書のブロックを書き込む位置
			short		*BCBuff		: BCを指すポインター
			Byte			*TailBuff		: TAILを指すポインター
出力		:	成功時 true
作成日	:	97.7.31
*****************************************************************************************/
static Boolean DivideBCT(DicRecord *dr,unsigned short initnum, unsigned short *lastnum, unsigned short blkNo, Dmp *node, long bcaddr,short *BCBuff,Byte *TailBuff)
{
	unsigned short	i;
	Boolean	ret = true;

	/* ダブル配列の初期化 */
	INI_BCTWithValue(dr);

	for (i = initnum; i < *lastnum; i++) {
		if(PrintBC(node,BCBuff,TailBuff)==false){
			ret = false;
			break;
		}
		if (i == initnum && blkNo != 0) {
			dr->TABLE[blkNo].First[0] = node->str_len;
			memmove(&(dr->TABLE[blkNo].First[1]),node->str, node->str_len);
		}
		/* キー挿入 */
		if(UbcInsert(dr,node->str,node->str_len, node->cont,node->cont_size)== kAddFail){
			break;
		}

		dr->TABLE[blkNo].keynum ++;
	}

	(*lastnum) = i;

	dr->TABLE[blkNo].bcsize = (dr->BC_MAX+1)*2*sizeof(short);
	dr->TABLE[blkNo].bcaddr = bcaddr;

	W_BASE(dr,0, dr->BC_MAX);
	W_CHECK(dr,0, dr->TA_POS);
	
	return ret;
}
