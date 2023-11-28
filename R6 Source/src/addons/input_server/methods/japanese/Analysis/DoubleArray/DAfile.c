/************************************************************************
ファイル名	:	DAFile.c

作成日	:	97.10.27
************************************************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include"DoubleArray.h"
#include"DAcommon.h"

/****************************************************************************************
関数名	:	DicWriteData
機能		:	ファイルにデータを書き込む
入力		:	DicRecord		*dr			: 辞書レコード
			long			*dicPos		: ファイル位置
			void			*buffPtr		: 書き込むデータ
			long			size			: 書き込むサイズ
出力		:	DAErr
作成日	:	97.7.29
修正日	:	97.10.27
*****************************************************************************************/
DAErr DicWriteData(DicRecord *dr,long *dicPos,const void *buffPtr,long size)
{
	ssize_t	amt_read;
	if((amt_read = dr->fp.Write(buffPtr,size)) < 0)return amt_read;

	(*dicPos)+=size;

	return kDANoErr;
}

/****************************************************************************************
関数名	:	DicReadData
機能		:	辞書からデータを読み込む
入力		:	DicRecord		*dr			: 辞書レコード
			long			*dicPos		: ファイル位置
			void			*buffPtr		: 書き込むデータ
			long			size			: 書き込むサイズ
出力		:	DAErr
作成日	:	97.7.30
修正日	:	97.10.27
*****************************************************************************************/
DAErr DicReadData(DicRecord *dr,long *dicPos,void *buffPtr,long size)
{
	if(dr->dicPtr == NULL){
		/* 辞書ファイルからの読み込み */
		ssize_t	amt_read;
		if((amt_read = dr->fp.Read(buffPtr,size)) < 0)return amt_read;
	}
	else{
		memmove(buffPtr,(void *)((Byte *)dr->dicPtr+(*dicPos)),size);
	}

	(*dicPos) += size;

	return kDANoErr;
}

/*****************************************************************************************
関数名	:	DicSetPosition
機能		:	ファイル又は、メモリー上の辞書オフセットをセットする
入力		:	DicRecord		*dr			: 辞書レコード
			long			*dicPos		: 辞書オフセット(out)
			long			posOff		: セットするポジション
出力		:	DAErr
作成日	:	97.7.30
修正日	:	97.10.27
*****************************************************************************************/
DAErr DicSetPosition(DicRecord *dr,long *dicPos,long posOff)
{
	DAErr	err = kDANoErr;

	if(dr->dicPtr == NULL){
		off_t	pos;
		if((pos = dr->fp.Seek(posOff,SEEK_SET)) < 0)return pos;
	}		

	(*dicPos) = posOff;

	return err;
}

/*****************************************************************************************
関数名	:	DicGetPosition
機能		:	ファイル又は、メモリー上の辞書オフセットを返す
入力		:	DicRecord		*dr		: 辞書レコード
			long			dicPos	: 現在の辞書オフセット
			long			*posOff	: 辞書オフセット(out)
出力		:	DAErr
作成日	:	97.7.30
修正日	:	97.10.27
*****************************************************************************************/
DAErr DicGetPosition(DicRecord *dr,long dicPos,long *posOff)
{
	DAErr	err = kDANoErr;

	if(dr->dicPtr == NULL){
		off_t	pos;
		if((pos = dr->fp.Position()) < 0)return pos;
	}

	(*posOff) = dicPos;

	return err;
}

/*****************************************************************************************
関数名	:	GetDictionarySize
機能		:	ファイル又は、メモリー上の辞書のサイズを調べる
入力		:	DicRecord		*dr			: 辞書レコード
			long			*size		: サイズを格納(out)
出力		:	DAErr
作成日	:	97.10.27
修正日	:	98.2.4
*****************************************************************************************/
DAErr DicGetSize(DicRecord *dr,long *size)
{
	DAErr	result = kDANoErr;
	off_t	dicSize;
	
	if(dr->dicPtr == NULL){
		result = dr->fp.GetSize(&dicSize);
		(*size) = dicSize;
	}

	return result;
}

/*****************************************************************************************
関数名	:	SetDictionarySize
機能		:	ファイル又は、メモリー上の辞書のサイズを変更する
入力		:	DicRecord		*dr			: 辞書レコード
			long			size			: 変更する辞書のサイズ
出力		:	DAErr
作成日	:	97.10.27
修正日	:	98.2.4
*****************************************************************************************/
DAErr DicSetSize(DicRecord *dr,long size)
{
	DAErr	result = kDANoErr;
	
	if(dr->dicPtr == NULL){
		result = dr->fp.SetSize(size);
	}
	return result;
}
