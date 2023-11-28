/******************************************
ファイル名	:	AccessTable.cp

作成日	:	97.12.3
******************************************/
#include <new.h>
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>
#include "AccessTable.h"
#include "Grammar.h"
#include "DicTypes.h"

/***************************************************************
関数名	:	AccessTable::AccessTable
機能		:	コンストラクタ．アクセステーブルを作成する
入力		:	なし
出力		:	なし
作成日	:	97.12.3
修正日	:	97.12.27
***************************************************************/
AccessTable::AccessTable(void)
{
	hTableArray = (HinshiTableArray*)new HinshiTableArray[kMaxATSize];
	oldTextLen = 0;
}

/***************************************************************
関数名	:	AccessTable::~AccessTable
機能		:	デストラクタ．アクセステーブルを解放
入力		:	なし
出力		:	なし
作成日	:	97.12.3
***************************************************************/
AccessTable::~AccessTable(void)
{
	AllFree();
	delete[] hTableArray;
}

/***************************************************************
関数名	:	AccessTable::operator[]
入力		:	なし
出力		:	なし
作成日	:	97.12.3
***************************************************************/
HinshiTableArray& AccessTable::operator[](long num)
{
#ifdef HINSHIDEBUG
	assert(!(num < 0 || kMaxATSize < num));
//	cerr << "AccessTableArray::operator[] err!!\t" << num <<'\n';
#endif

	return *(hTableArray+num);	//	hTableArray[num]
}

/***************************************************************
関数名	:	AccessTable::SetWord
機能		:	単語をアクセステーブルに追加する
入力		:	int			pos		: 文字列の始まる位置
			int			length	: contentsの長さ
			unsigned char*	contents	: 追加する文字列に対する表記
			Category		hinshiCode	: 追加する単語の品詞
			short		evaluation	: 追加する単語の頻度
出力		:	なし
作成日	:	97.12.5
修正日	:	98.1.21
***************************************************************/
void AccessTable::SetWord(int pos,int length,unsigned char *contents,Category hinshiCode,short evaluation)
{
	short	postHinshi;

	postHinshi = POST_HINSHI(hinshiCode);
	if(postHinshi == kSahenMeishi){
		/* 後ろがサ変名詞 */
		if(PRE_HINSHI(hinshiCode) == kSahenMeishi){
			/* 前もサ変名詞 */
			SetHinshiAndField(pos,length,contents,HINSHI(kSahenGokan,kSahenGokan),evaluation);
			SetHinshiAndField(pos,length,contents,HINSHI(kFutuuMeishi,kFutuuMeishi),evaluation);
		}
		else{
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kSahenGokan),evaluation);
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kFutuuMeishi),evaluation);
		}
	}
	else if(postHinshi == kZahenMeishi){
		/* 後ろがザ変名詞 */
		if(PRE_HINSHI(hinshiCode) == kZahenMeishi){
			/* 前もザ変名詞 */
			SetHinshiAndField(pos,length,contents,HINSHI(kZahenGokan,kZahenGokan),evaluation);
			SetHinshiAndField(pos,length,contents,HINSHI(kFutuuMeishi,kFutuuMeishi),evaluation);
		}
		else{
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kZahenGokan),evaluation);
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kFutuuMeishi),evaluation);
		}
	}
	else if(postHinshi == kKeidouMeishi){
		/* 後ろが形動名詞 */
		if(PRE_HINSHI(hinshiCode) == kKeidouMeishi){
			/* 前も形動名詞 */
			SetHinshiAndField(pos,length,contents,HINSHI(kKeidouGokan,kKeidouGokan),evaluation);
			SetHinshiAndField(pos,length,contents,HINSHI(kFutuuMeishi,kFutuuMeishi),evaluation);
		}
		else{
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kKeidouGokan),evaluation);
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kFutuuMeishi),evaluation);
		}
	}
	else if(postHinshi == kItidanGokan){
		/* 後ろが一段語幹 */
		SetHinshiAndField(pos,length,contents,hinshiCode,evaluation);
		if(PRE_HINSHI(hinshiCode) == kItidanGokan){
			/* 前も一段語幹 */
			SetHinshiAndField(pos,length,contents,HINSHI(kItidanMizen,kItidanMizen),evaluation);
			SetHinshiAndField(pos,length,contents,HINSHI(kItidanRenyo,kItidanRenyo),evaluation);
		}
		else{
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kItidanMizen),evaluation);
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kItidanRenyo),evaluation);
		}
	}
	else if(postHinshi == kHojoItidanGokan1){
		/* 補助一段１ */
		SetHinshiAndField(pos,length,contents,hinshiCode,evaluation);
		if(PRE_HINSHI(hinshiCode) == kHojoItidanGokan1){
			/* 前も補助一段１ */
			SetHinshiAndField(pos,length,contents,HINSHI(kHojoItidanMizen1,kHojoItidanMizen1),evaluation);
			SetHinshiAndField(pos,length,contents,HINSHI(kHojoItidanRenyo1,kHojoItidanRenyo1),evaluation);
		}
		else{
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kHojoItidanMizen1),evaluation);
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kHojoItidanRenyo1),evaluation);
		}
	}
	else if(postHinshi == kHojoItidanGokan2){
		/* 補助一段２ */
		SetHinshiAndField(pos,length,contents,hinshiCode,evaluation);
		if(PRE_HINSHI(hinshiCode) == kHojoItidanGokan2){
			/* 前も補助一段２ */
			SetHinshiAndField(pos,length,contents,HINSHI(kHojoItidanMizen2,kHojoItidanMizen2),evaluation);
			SetHinshiAndField(pos,length,contents,HINSHI(kHojoItidanRenyo2,kHojoItidanRenyo2),evaluation);
		}
		else{
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kHojoItidanMizen2),evaluation);
			SetHinshiAndField(pos,length,contents,SET_POST_HINSHI(hinshiCode,kHojoItidanRenyo2),evaluation);
		}
	}
	else
		SetHinshiAndField(pos,length,contents,hinshiCode,evaluation);
}	

/***************************************************************
関数名	:	AccessTable::SetHinshiAndField
機能		:	品詞とフィールドをアクセステーブルに追加する
入力		:	int			pos		: 文字列の始まる位置
			int			length	: contentsの長さ
			unsigned char*	contents	: 追加する文字列に対する表記
			Category		hinshiCode	: 追加する単語の品詞
			short		evaluation	: 追加する単語の頻度
出力		:	なし
作成日	:	97.12.3
修正日	:	97.12.12
***************************************************************/
void AccessTable::SetHinshiAndField(int pos,int length,unsigned char *contents,Category hinshiCode,short evaluation)
{
	Hinshi	*tmpHinshi;
	int		i,objLen;
	short	preHinshi,postHinshi;
	bool		newHinshiFlag = false;
	
	preHinshi = PRE_HINSHI(hinshiCode);
	postHinshi = POST_HINSHI(hinshiCode);

	/* 品詞が追加されているか探す */
	tmpHinshi = (*this)[pos][length].firstHinshi;
	SearchHinshi(hinshiCode,&tmpHinshi);
	if(tmpHinshi == NULL){
		/* 品詞がまだ登録されていない */
		tmpHinshi = hinshiStruct.GetFreeHinshi();
		tmpHinshi->length = length;
		tmpHinshi->category = hinshiCode;
		if(preHinshi != postHinshi){
			/* 前品詞と後ろ品詞が違うときには２つのゲタを足したものをコストとする． */
			tmpHinshi->cost = gGrammar.GetBasicCost(preHinshi) + gGrammar.GetBasicCost(preHinshi);
		}
		else{
			tmpHinshi->cost = gGrammar.GetBasicCost(preHinshi);
		}
		if(gGrammar.GetProperty(PRE_HINSHI(hinshiCode)) != content){
			/* 前の品詞が付属語 */
			tmpHinshi->contentWFlag = false;
		}
		else{
			/* 前の品詞が自立語 */
			tmpHinshi->contentWFlag = true;
			(*this)[pos][length].contentWFlag = true;
		}
		tmpHinshi->segmentFlag = false;	

		/* 品詞を追加 */
		SetHinshi(pos,length,tmpHinshi);
		
		newHinshiFlag = true;
	}
	/* 表記を積み込む */
	SetField(tmpHinshi,contents,evaluation);

	/* 文節の作成 */
	if(newHinshiFlag == true && gGrammar.GetProperty(PRE_HINSHI(hinshiCode)) == function){
		/* 前の品詞が付属語なので文節を作成する */
		/* 前の単語との接続を調べる */
		i=pos-kKeyMax;	//
		if(i<0)i=0;		// 接続可能単語の開始位置を計算する．
		for(;i<pos;i++){
			objLen = pos-i;	//接続チェックをする単語のサイズ
			if(objLen <= (*this)[i].nowTable){
				CheckConnection(i,objLen,length,tmpHinshi);
			}
		}
	}
}

/***************************************************************
関数名	:	AccessTable::SetHinshi
機能		:	品詞こコストの小さい順に追加する
入力		:	int			pos			: 文字列の始まる位置
			int			length		: contentsの長さ
			Hinshi		*targetHinshi	: 追加する品詞
出力		:	なし
作成日	:	97.12.6
修正日	:	97.12.7
***************************************************************/
void AccessTable::SetHinshi(int pos,int length,Hinshi *targetHinshi)
{
	Hinshi	**tmpHinshi;

	targetHinshi->firstField = NULL;

	tmpHinshi = &((*this)[pos][length].firstHinshi);
	while((*tmpHinshi) != NULL){
		if((*tmpHinshi)->cost > targetHinshi->cost)break;
		tmpHinshi = &((*tmpHinshi)->nextHinshi);
	}
	targetHinshi->nextHinshi = (*tmpHinshi);
	(*tmpHinshi) = targetHinshi;
}

/***************************************************************
関数名	:	AccessTable::CheckConnection
機能		:	接続を調べ，接続がonの時には文節を作る．
入力		:	int			pos			: 前品詞の文字列の始まる位置
			int			objLen		: 前品詞の文字列の長さ
			int			length		: 後品詞に対する文字列の長さ
			Hinshi		*targetHinsh	: 後品詞
出力		:	なし
作成日	:	97.12.6
修正日	:	98.1.2
***************************************************************/
void AccessTable::CheckConnection(int pos,int objLen,int length,Hinshi *targetHinshi)
{
	Hinshi *tmpHinshi,*newHinshi;

	tmpHinshi = (*this)[pos][objLen].firstHinshi;//接続を調べる最初の品詞
	while(tmpHinshi != NULL){
		if((tmpHinshi->contentWFlag == true || tmpHinshi->segmentFlag == true) && 
				gGrammar.IsConnect(POST_HINSHI(tmpHinshi->category),PRE_HINSHI(targetHinshi->category)) == true){
			/* 接続可能 */
			newHinshi = hinshiStruct.GetFreeHinshi();
			newHinshi->cost = gGrammar.GetCost(POST_HINSHI(tmpHinshi->category),PRE_HINSHI(targetHinshi->category))
							+ tmpHinshi->cost + targetHinshi->cost;
			newHinshi->category = SET_POST_HINSHI(tmpHinshi->category,POST_HINSHI(targetHinshi->category));
			newHinshi->length = objLen+length;
			newHinshi->contentWFlag = false;
			newHinshi->segmentFlag = true;
			newHinshi->firstField = NULL;

			(*this)[pos][objLen+length].segmentFlag = true;
			SetHinshi(pos,objLen+length,newHinshi);

			MakeSegment(newHinshi,tmpHinshi,targetHinshi);			
		}
		tmpHinshi = tmpHinshi->nextHinshi;
	}
}

/***************************************************************
関数名	:	AccessTable::MakeSegment
機能		:	接続を調べ，接続がonの時には文節を作る．
入力		:	Hinshi	*newHinshi		: 文節として作成する新しい品詞
			Hinshi	*preHinshi		: 文節の前の部分
			Hinshi	*postHinshi		: 文節の後ろの部分
出力		:	なし
作成日	:	97.12.6
修正日	:	97.12.7
***************************************************************/
void AccessTable::MakeSegment(Hinshi *newHinshi,Hinshi *preHinshi,Hinshi *postHinshi)
{
	Field	*preField,*postField,*tmpField,**newField;

	/* 最後のフィールドを作成 */
	postField = fieldStruct.GetFreeField();
	postField->nextField = NULL;
	postField->contents = postHinshi;

	if(preHinshi->segmentFlag == true){
		/* 文節とさらに接続 */
		tmpField = preHinshi->firstField;
		newField = &(newHinshi->firstField);
#if _SUPPORTS_EXCEPTION_HANDLING
		try{
#endif
			/* 前の部分をコピーする */
			while(tmpField != NULL){
				preField = fieldStruct.GetFreeField();
				(*preField) = (*tmpField);
				preField->nextField = NULL;
				(*newField) = preField;
				tmpField = tmpField->nextField;
				newField = &(preField->nextField);
			}
#if _SUPPORTS_EXCEPTION_HANDLING
		}
		catch(bad_alloc){
			while(newHinshi->firstField != NULL){
				tmpField = newHinshi->firstField;
				newHinshi->firstField = newHinshi->firstField->nextField;
				fieldStruct.SetFreeField(tmpField);
			}
			fieldStruct.SetFreeField(postField);
			return;
		}
#endif
		preField->nextField = postField;
	}
	else{
#if _SUPPORTS_EXCEPTION_HANDLING
		try{
#endif
			preField = fieldStruct.GetFreeField();
#if _SUPPORTS_EXCEPTION_HANDLING
		}
		catch(bad_alloc){
			fieldStruct.SetFreeField(postField);
			return;
		}
#endif
		newHinshi->firstField = preField;
		preField->nextField = postField;
		preField->contents = preHinshi;
	}
}

/***************************************************************
関数名	:	AccessTable::SearchHinshi
機能		:	品詞が積み込まれているか調べる関数
入力		:	Category	hinshiCode		: 調べる品詞
			Hinshi	**retHinshi	: 調べ始める位置(in)
								: 積み込まれている時にはそのポインター，ないときはNULL(out)
出力		:	なし
作成日	:	97.12.6
***************************************************************/
void AccessTable::SearchHinshi(Category hinshiCode,Hinshi **retHinshi)
{
	while((*retHinshi) != NULL){
		if((*retHinshi)->category == hinshiCode){
			/* 既にhinshiCodeは登録されている */
			return;
		}
		(*retHinshi) = (*retHinshi)->nextHinshi;
	}
}

/***************************************************************
関数名	:	AccessTable::SetField
機能		:	表記を頻度順に追加する
入力		:	unsigned char*	contents	: 表記の前半
			short		evaluation	: 追加する単語の頻度
出力		:	なし
作成日	:	97.12.3
修正日	:	97.12.10
その他	:	同じ表記のものは，頻度の大きいものだけを登録する
***************************************************************/
void AccessTable::SetField(Hinshi* hinshi,unsigned char *contents,short evaluation)	
{
	Field **tmpField,*newField=NULL;

	/* 同じ表記のものを探す */
	tmpField = &(hinshi->firstField);
	while((*tmpField) != NULL){
		if(strcmp((char *)(*tmpField)->contents,(char *)contents) == 0){
			if((*tmpField)->evaluation >= evaluation){
				/* 既に登録されている表記の方が頻度が大きい */
				return;
			}
			else{
				/* そのFieldをリストからはずし，頻度を変更する */
				newField = (*tmpField);
				(*tmpField) = (*tmpField)->nextField;
				newField->evaluation = evaluation;
			}
			break;
		}
		tmpField = &((*tmpField)->nextField);
	}
	if((*tmpField) == NULL && newField == NULL){
		/* まだ登録されていない */
		newField = fieldStruct.GetFreeField();
		newField->evaluation = evaluation;
		newField->contents = malloc(strlen((char *)contents)+1);//malloc
#if _SUPPORTS_EXCEPTION_HANDLING
		// malloc never fails on IAD, hplus said it was OK to check the exception-handling define for this feature
		if(newField->contents == NULL){
			fieldStruct.SetFreeField(newField);
			throw bad_alloc();
			return;
		}
#endif
		strcpy((char *)newField->contents,(char *)contents);
	}

	tmpField = &(hinshi->firstField);
	while((*tmpField) != NULL){
		if((*tmpField)->evaluation < evaluation)break;
		tmpField = &((*tmpField)->nextField);
	}

	newField->nextField = (*tmpField);
	(*tmpField) = newField;
}

/***************************************************************
関数名	:	AccessTable::Free
機能		:	アクセステーブルの使わなくなった部分を解放する
入力		:	int	startOffset	: 使わなくなった最初の位置
			int	endOffset		: 使わなくなった最後の位置
出力		:	なし
作成日	:	97.12.4
修正日	:	97.12.27
***************************************************************/
void AccessTable::Free(int startOffset,int endOffset)
{
	int	i,j;
	
	if(endOffset >= oldTextLen)endOffset = oldTextLen-1;

	i=startOffset-kKeyMax+1;
	if(i<0)i=0;
	/* 品詞テーブルの後ろの方を解放する */
	for(;i<startOffset;i++){
		for(j=(*this)[i].nowTable;i+j > startOffset;j--){
			FreeHinshi(i,j);
//			(*this)[i][j].contentWFlag = false;
//			(*this)[i][j].segmentFlag = false;
		}
		(*this)[i].nowTable = j;
	}
	/* startOffset以降の品詞テーブルのすべての品詞を解放する */
	for(i=startOffset;i<=endOffset;i++){


		for(j=(*this)[i].nowTable;j>0;j--){
			FreeHinshi(i,j);
//			(*this)[i][j].contentWFlag = false;
//			(*this)[i][j].segmentFlag = false;
		}
		(*this)[i].nowTable = 0;
	}
}

/***************************************************************
関数名	:	AccessTable::AllFree
機能		:	アクセステーブルのすべての部分を解放する
入力		:	なし
出力		:	なし
作成日	:	97.12.4
修正日	:	97.12.29
***************************************************************/
void AccessTable::AllFree(void)
{
	Free(0,oldTextLen-1);
	oldTextLen = 0;
}

/***************************************************************
関数名	:	AccessTable::FreeHinshi
機能		:	アクセステーブルの品詞構造体を解放する
入力		:	int			pos		: 解放するアクセステーブルの位置
			int			length	: 解放する品詞テーブルの位置
出力		:	なし
作成日	:	97.12..4
修正日	:	97.12..12
***************************************************************/
void AccessTable::FreeHinshi(int pos,int length)
{
	Hinshi	*tmpHinshi;

	while((*this)[pos][length].firstHinshi != NULL){
		tmpHinshi = (*this)[pos][length].firstHinshi;
		(*this)[pos][length].firstHinshi = (*this)[pos][length].firstHinshi->nextHinshi;
		FreeField(tmpHinshi->firstField,tmpHinshi->segmentFlag);
		hinshiStruct.SetFreeHinshi(tmpHinshi);
	}
//	(*this)[pos][length].firstHinshi = NULL;
	(*this)[pos][length].contentWFlag = false;
	(*this)[pos][length].segmentFlag = false;
}

/***************************************************************
関数名	:	AccessTable::FreeField
機能		:	アクセステーブルの表記構造体を解放する
入力		:	Field		*field		: 解放する最初の構造体
			bool		segmentFlag	: trueのとき文節として解放する
出力		:	なし
作成日	:	97.12..4
修正日	:	97.12..6
***************************************************************/
void AccessTable::FreeField(Field *field,bool segmentFlag)
{
	Field	*tmpField;

	while(field != NULL){
		tmpField = field;
		field = field->nextField;
		if(segmentFlag == false){
			/* 文節でないときには，表記が積まれているので解放する */
			free(tmpField->contents);							//	mallocのため
		}
		fieldStruct.SetFreeField(tmpField);
	}
}

/***************************************************************
関数名	:	AccessTable::SetNanashi
機能		:	品詞”名無し”をアクセステーブルに登録する
入力		:	int			pos		: 文字列の始まる位置
			int			length	: contentsの長さ
			int			preLength	: １つ前の文字の長さ
			unsigned char*	contents	: 追加する文字列に対する表記
出力		:	なし
作成日	:	97.12.10
***************************************************************/
void AccessTable::SetNanashi(int pos,int length,unsigned char *contents)
{
	Hinshi	**tmpHinshi,*newHinshi;
	Field		*newField;
	int		preLength;
	int		i;
	
	i=pos-kKeyMax;	//
	if(i<0)i=0;		// 接続可能単語の開始位置を計算する．
	for(;i<pos;i++){
		preLength = pos-i;	//接続チェックをする単語のサイズ
		if(preLength <= (*this)[i].nowTable){
			tmpHinshi = &((*this)[i][preLength].firstHinshi);//接続を調べる最初の品詞
			while((*tmpHinshi) != NULL){
				if(PRE_HINSHI((*tmpHinshi)->category) == kNanashi && POST_HINSHI((*tmpHinshi)->category) == kNanashi){
					/* 前に品詞”名無し”が存在する */
					newHinshi = (*tmpHinshi);
					(*tmpHinshi) = (*tmpHinshi)->nextHinshi;

					newHinshi->length = preLength+length;

					/* 表記の作成 */
					newField = fieldStruct.GetFreeField();
					newField->evaluation = 0;
					newField->nextField = NULL;

					newField->contents = malloc(strlen((char*)(newHinshi->firstField->contents))+strlen((char *)contents)+1);//malloc
#if _SUPPORTS_EXCEPTION_HANDLING
					// malloc never fails on IAD, hplus said it was OK to check the exception-handling define for this feature
					if(newField->contents == NULL){
						fieldStruct.SetFreeField(newField);
						throw bad_alloc();
						return;
					}
#endif
					strncpy((char*)(newField->contents),(char*)(newHinshi->firstField->contents),strlen((char*)(newHinshi->firstField->contents)));
					strcpy((char*)(newField->contents)+strlen((char*)(newHinshi->firstField->contents)),(char *)contents);

					fieldStruct.SetFreeField(newHinshi->firstField);

					SetHinshi(i,preLength+length,newHinshi);

					newHinshi->firstField = newField;

					return;
				}
				tmpHinshi = &((*tmpHinshi)->nextHinshi);
			}
		}
	}

	/* 前に”名無し”が存在しなかった */
	(*this).SetWord(pos,length,contents,HINSHI(kNanashi,kNanashi),0);
}

/***************************************************************
関数名	:	AccessTable::Shift
機能		:	アクセステーブルを左にずらす
入力		:	uint16		pos		: ずらすバイト数
出力		:	なし
作成日	:	97.12.29
***************************************************************/
void AccessTable::Shift(uint16 pos)
{
	if(pos > oldTextLen){
		AllFree();
		return;
	}
	
	Free(0,pos-1);
	for(uint16 i=pos;i<oldTextLen;i++){
		Move(i,i-pos);
	}

	oldTextLen -= pos;
}

/***************************************************************
関数名	:	AccessTable::Move
機能		:	アクセステーブルのsrc番目の要素をtarget番目に移動する
入力		:	uint16	src			: 移動元
			uint16	target		: 移動先
出力		:	なし
作成日	:	97.12.29
修正日	:	98.1.3
***************************************************************/
void AccessTable::Move(uint16 src,uint16 target)
{
	for(uint16 i=1;i<=(*this)[src].nowTable;i++){
		(*this)[target][i]=(*this)[src][i];
		
		(*this)[src][i].firstHinshi = NULL;
		(*this)[src][i].contentWFlag = false;
		(*this)[src][i].segmentFlag = false;
	}
	(*this)[src].nowTable = 0;
}

/***************************************************************
関数名	:	AccessTable::CheckSuusi
機能		:	posから文字の数詞が存在するか調べる
入力		:	int			pos		: 文字列の始まる位置
			int			length	: 文字列の長さ
出力		:	存在する時 true
作成日	:	98.1.23
修正日	:	98.2.24
***************************************************************/
bool AccessTable::CheckSuusi(int pos,int length)
{
	Hinshi *tmpHinshi;

	if((*this)[pos].nowTable < length)return false;

	tmpHinshi = (*this)[pos][length].firstHinshi;//接続を調べる最初の品詞
	while(tmpHinshi != NULL){
		if(POST_HINSHI(tmpHinshi->category) == suushi && PRE_HINSHI(tmpHinshi->category) == suushi)return true;
		tmpHinshi = tmpHinshi->nextHinshi;
	}
	return false;
}

