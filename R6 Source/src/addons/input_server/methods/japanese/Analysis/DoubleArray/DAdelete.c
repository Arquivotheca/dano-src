/************************************************************************
ファイル名	:	DADelete.c

作成日	:	97.8.7
************************************************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include"DoubleArray.h"
#include"DAcommon.h"

enum{			//	UbcDeleteの返り値
	kDeleteFail,
	kDeleteSuccess
};

static DAErr k_delete(DicRecord *dr,const Byte *key,unsigned short key_len);
static short UbcDelete(DicRecord *dr,const Byte *key,short key_len);

/*******************************************************************
関数名	:	DADeleteKey
機能		:	辞書からキーを削除する
入力		:	unsigned short	dicID			: 辞書ID
			unsigned short	keyLength		: キーの長さ
			Byte		*key			: キー
出力		:	DAErr
作成日	:	97.8.7
*******************************************************************/
DAErr DADeleteKey(DicRecord* dr,unsigned short keyLength,const Byte *key)
{
	DAErr		err;
	
	/* キーサイズのチェック */
	if(keyLength > kKeyMax || keyLength == 0)return kDANoKeyErr;

	err = k_delete(dr,key,keyLength);

	return err;
}

/*****************************************************************************************
関数名	:	k_delete
機能		:	ダブル配列からキーを削除する
入力		:	DicRecord			*dr		: 辞書レコード
			Byte				*key		: 挿入文字列
			short			key_len	: keyの長さ
出力		:	DAErr
作成日	:	97.8.7
*****************************************************************************************/
static DAErr k_delete(DicRecord *dr,const Byte *key,unsigned short key_len)
{
	unsigned short		blkNo;
	short		delete_ret;
	DAErr		err;

	blkNo = BCODE(dr,key,key_len);
	err = ChangeBlock(dr,blkNo,false);
	if(err != kDANoErr)return err;
		
	dr->BC_MAX = dr->B_C[0];
	dr->TA_POS = dr->B_C[1];

	/* キー削除 */
	if((delete_ret=UbcDelete(dr,key,key_len))==kDeleteSuccess){
		dr->TABLE[blkNo].keynum --;
		dr->block.writeFlg = true;
		dr->TABLE[blkNo].bcsize = (dr->BC_MAX+1)*2*sizeof(short);
		W_BASE(dr,0, dr->BC_MAX);
		W_CHECK(dr,0, dr->TA_POS);
	}
	else
		err = kDANoKeyErr;

	return err;
}

/*****************************************************************************************
関数名	:	UbcDelete
機能		:	ダブル配列の削除を行なう
入力		:	DicRecord	*dr			: 辞書レコード
			Byte		*key			: 登録される文字列
			short	key_len		: keyの長さ
出力		:	DAErr
作成日	:	97.8.7
*****************************************************************************************/
static short UbcDelete(DicRecord *dr,const Byte *key,short key_len)
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
			/* 指定されたキーが存在しない */
			return kDeleteFail;
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
		/* キーを削除する */
		DeleteState(dr,t);
		return kDeleteSuccess;
	}
	else {
		/* 指定されたキーが存在しない */
		return kDeleteFail;
	}
}
