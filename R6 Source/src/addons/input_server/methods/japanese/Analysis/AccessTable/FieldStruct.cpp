/******************************************
ファイル名	:	FieldStruct.cp

作成日	:	97.12.3
******************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>
#include "FieldStruct.h"
#include <new.h>

FieldStruct::FieldStruct(void)
{
	pageHead = NULL;
	freeField = NULL;
}

FieldStruct::~FieldStruct(void)
{
	FieldPage	*tmp;
	
	while(pageHead != NULL){
		tmp = pageHead;
		pageHead = pageHead->next;
		free(tmp);
	}
}

Field* FieldStruct::GetFreeField(void)
{
	Field	*tmp;

	if(freeField == NULL){
		/* 新しいページを確保する */
		NewFieldPage();
	}
	
	tmp = freeField;
	freeField = freeField->nextField;
	tmp->nextField = NULL;	//いらん？
	
	return tmp;
}

/***************************************************************
関数名	:	HinshiStruct::NewHinshiPage
機能		:	品詞構造体用のページを新しく1つ作る
入力		:	なし
出力		:	なし
作成日	:	97.12.3
***************************************************************/
void FieldStruct::NewFieldPage(void)
{
	long			pageSize;
	FieldPage	*page;
	int			i;

	pageSize = (sizeof(FieldPage) + sizeof(Field) * (kOneFieldPage - 1));
	
	page = ( FieldPage * ) malloc( pageSize );
#if _SUPPORTS_EXCEPTION_HANDLING
		// malloc never fails on IAD, hplus said it was OK to check the exception-handling define for this feature
	if(page == NULL){
		throw bad_alloc();
		return;
	}
#endif
	
	page->next = pageHead;
	pageHead = page;
	
	/* 確保したすべてを利用可能にする */
	for ( i = 0; i < kOneFieldPage; i++ )
	{
		(page->cell[i]).nextField = freeField;
		page->cell[i].contents = NULL;		//mallocのため
		freeField = &(page->cell[i]);
	}
}

/***************************************************************
関数名	:	FieldStruct::SetFreeField
機能		:	使用しなくなった品詞構造体を再利用可能にする
入力		:	Field*		Field	: 使用しなくなった品詞構造体
出力		:	なし
作成日	:	97.12.3
修正日	:	97.12.4
***************************************************************/
void FieldStruct::SetFreeField(Field* Field)
{
	Field->nextField = freeField;
	Field->contents = NULL;		//mallocのため
	freeField = Field;
}
