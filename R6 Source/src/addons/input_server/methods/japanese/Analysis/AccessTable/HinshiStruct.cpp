/******************************************
ファイル名	:	AccessTable.cp

作成日	:	97.12.3
******************************************/
#include "HinshiStruct.h"
#include <new.h>
#include <malloc.h>

HinshiStruct::HinshiStruct(void)
{
	pageHead = NULL;
	freeHinshi = NULL;
}


HinshiStruct::~HinshiStruct(void)
{
	HinshiPage	*tmp;
	
	while(pageHead != NULL){
		tmp = pageHead;
		pageHead = pageHead->next;
		free(tmp);
	}
}

/***************************************************************
関数名	:	HinshiStruct::GetFreeHinshi
機能		:	使われていない品詞構造体を得る
入力		:	なし
出力		:	なし
作成日	:	97.12.3
***************************************************************/
Hinshi* HinshiStruct::GetFreeHinshi(void)
{
	Hinshi	*tmp;

	if(freeHinshi == NULL){
		/* 新しいページを確保する */
		NewHinshiPage();
	}
	
	tmp = freeHinshi;
	freeHinshi = freeHinshi->nextHinshi;
	tmp->nextHinshi = NULL;	//いらん？
	
	return tmp;
}

/***************************************************************
関数名	:	HinshiStruct::NewHinshiPage
機能		:	品詞構造体用のページを新しく1つ作る
入力		:	なし
出力		:	なし
作成日	:	97.12.3
***************************************************************/
void HinshiStruct::NewHinshiPage(void)
{
	long			pageSize;
	HinshiPage	*page;
	int			i;

	pageSize = (sizeof(HinshiPage) + sizeof(Hinshi) * (kOneHinshiPage - 1));
	
	page = ( HinshiPage * ) malloc( pageSize );
#if _SUPPORTS_EXCEPTION_HANDLING
	// malloc never fails on IAD, hplus said it was OK to check the exception-handling define for this feature
	if(page == NULL){
		throw bad_alloc();
		return;
	}
#endif
	
	page->next = pageHead;
	pageHead = page;
	
	for ( i = 0; i < kOneHinshiPage; i++ )
	{
		(page->cell[i]).nextHinshi = freeHinshi;
		freeHinshi = &(page->cell[i]);
	}
}

/***************************************************************
関数名	:	HinshiStruct::SetFreeHinshi
機能		:	使用しなくなった品詞構造体を再利用可能にする
入力		:	Hinshi*		hinshi	: 使用しなくなった品詞構造体
出力		:	なし
作成日	:	97.12.3
***************************************************************/
void HinshiStruct::SetFreeHinshi(Hinshi* hinshi)
{
	hinshi->nextHinshi = freeHinshi;
	freeHinshi = hinshi;
}
