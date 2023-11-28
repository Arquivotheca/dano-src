/******************************************
ファイル名	:	HinshiTableArray.cp

作成日	:	97.12.2
******************************************/
#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "HinshiTableArray.h"
#include "DicTypes.h"

HinshiTableArray::HinshiTableArray(void)
{
	hTable=NULL;

	nowTable=0;
	maxTable=0;
}


HinshiTableArray::~HinshiTableArray(void)
{
	delete[] hTable;
}

HinshiTable& HinshiTableArray::operator[](long num) 
{
#ifdef HINSHIDEBUG
	assert(!(num <= 0));
//	cerr << "HinshiTableArray::operator[] err!!\t" << num <<'\n';
#endif

	if(num > maxTable){
		HinshiTable	*temp;
		int			size,i;
		
		size = (num-1)/kOneAccessTable;
		size ++;
		size *= kOneAccessTable;

		temp = new HinshiTable[size];//error checkは，いらんらしい

		if(hTable != NULL){
			memmove(temp,hTable,sizeof(HinshiTable)*maxTable);
			delete[] hTable;
		}
		hTable = temp;
		for(i=maxTable;i<size;i++){
			hTable[i].contentWFlag = false;
			hTable[i].segmentFlag = false;
		}
			
		maxTable = size;
	}

	if(num > nowTable)nowTable = num;
	
	return hTable[num-1];
}
