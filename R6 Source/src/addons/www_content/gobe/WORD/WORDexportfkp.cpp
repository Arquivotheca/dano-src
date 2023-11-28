#include "WORDexportfkp.h"


TExportFKPTbl::TExportFKPTbl(TOLEEntryWriter *docWriter, TOLEEntryWriter *tableWriter)
{
	mDocWriter = docWriter;
	mTableWriter = tableWriter;
	mLatestFC = 0;
}

TExportFKPTbl::~TExportFKPTbl()
{
	while (mBinTbl.CountItems())
		delete (BinTbl*) mBinTbl.RemoveItem(0L);

	while (mEndFCTbl.CountItems())
		delete (int32*) mEndFCTbl.RemoveItem(0L);

	while (mBXTbl.CountItems())
		delete (BX*) mBXTbl.RemoveItem(0L);

	while (mSTSHITbl.CountItems())
		delete (short*) mSTSHITbl.RemoveItem(0L);

	while (mSPRMTbl.CountItems())
		delete[] (char*) mSPRMTbl.RemoveItem(0L);

	while (mSPRMsizeTbl.CountItems())
		delete (short*) mSPRMsizeTbl.RemoveItem(0L);

	while (mStyleIndexTbl.CountItems())
		delete (int32*) mStyleIndexTbl.RemoveItem(0L);
}

TPAPXExportTbl::TPAPXExportTbl(TOLEEntryWriter *docWriter, TOLEEntryWriter *tableWriter)
			: TExportFKPTbl(docWriter, tableWriter)
{
}

TCHPXExportTbl::TCHPXExportTbl(TOLEEntryWriter *docWriter, TOLEEntryWriter *tableWriter)
			: TExportFKPTbl(docWriter, tableWriter)
{
}

int32 TExportFKPTbl::firstPN()
{
	if (mBinTbl.CountItems() == 0)
		return -1;
	
	return ((BinTbl*) mBinTbl.ItemAt(0))->pn;
}


int32 TExportFKPTbl::setEntryPosition(TOLEEntryWriter* entryWriter, int32 pos)
{
	char zero = 0;
	if (entryWriter->StreamDataSize() < pos)
	{
		entryWriter->SetStreamPos(entryWriter->StreamDataSize());
		while (entryWriter->StreamDataSize() < pos)
			entryWriter->WriteBytes(&zero, 1);

	}
	else
		entryWriter->SetStreamPos(pos);
	
	
    return 1;
}


int32 TExportFKPTbl::write(long *binPos, unsigned long *binBytes)
{
	writeFKPs();
	
	*binPos = mTableWriter->GetStreamPos();

	 mTableWriter->WriteLong(mfirstFC);
	
	for (long i = 0; i < mBinTbl.CountItems(); i++)
	{
		mTableWriter->WriteLong(((BinTbl*) mBinTbl.ItemAt(i))->fcMax);
		IFDEBUG(fprintf(stderr, "Bintable runs %x\n", ((BinTbl*) mBinTbl.ItemAt(i))->fcMax));
	}
	
	for (long i = 0; i < mBinTbl.CountItems(); i++)
	{
		mTableWriter->WriteLong(((BinTbl*) mBinTbl.ItemAt(i))->pn);
		IFDEBUG(fprintf(stderr, "There is an FKP on page %d\n", ((BinTbl*) mBinTbl.ItemAt(i))->pn));
	}
	*binBytes = (mBinTbl.CountItems() * 8) + 4;  
	
	return 1;
}

int32 TExportFKPTbl::extendLastRun(long length)
{
	if (mEndFCTbl.CountItems())
		*((int32*) mEndFCTbl.ItemAt(mEndFCTbl.CountItems() - 1)) += length;
	mLatestFC += length;
	return 1;
}



int32 TPAPXExportTbl::addRun(int32 charCount, const TTranslatorStyle* style, TFontTblWORD* fontTbl, TStyleSheetWORD* styleTbl)
{
	int32	endFC = (mLatestFC += charCount) + mfirstFC;
	int32*	newEndFC;
	BX*		newBX;
	short*	newSTSHI;
	char*	newSPRMList;
	short*	newSPRMListSize;
	int32*	newStyleIndex;
	const TTranslatorStyle* textStyle;



	//BMessage	charMsg;
	//msg->FindMessage("CharStyle", &charMsg);
	
	textStyle = style;
	newEndFC = new int32;
	newBX = new BX;
	newSTSHI = new short;
	newSPRMListSize = new short;
	newStyleIndex = new int32;

	newBX->bxOffset = 0;
	newBX->bxPHE.bitField1 = 0;
	newBX->bxPHE.bitField2 = 0;
	newBX->bxPHE.dymLine = 0;
	newBX->bxPHE.dymHeight = 0;
	*newSPRMListSize = GetSPRMListSize(textStyle);
	*newStyleIndex = style->Index();
	
	newSPRMList = new char[*newSPRMListSize];
	GetSPRMList(textStyle, newSPRMList, fontTbl, styleTbl);
		
	*newEndFC = endFC;

	const char *applyStyleName;
	const char *baseStyleName;
	bool hasApplyStyle = style->GetApplyStyleName(&applyStyleName);
	bool hasBaseStyle = style->GetBaseStyleName(&baseStyleName);
	short stshi;
	if (hasApplyStyle)
		stshi = styleTbl->GetStyleIndex(applyStyleName);
	else if(hasBaseStyle)
		stshi = styleTbl->GetStyleIndex(baseStyleName);
	else
		stshi = styleTbl->GetStyleIndex("Body");
	
	if (stshi != -1)
		*newSTSHI = stshi;
	else
		*newSTSHI = 0;
		
	mEndFCTbl.AddItem(newEndFC);
	mBXTbl.AddItem(newBX);
	mSTSHITbl.AddItem(newSTSHI);
	mSPRMTbl.AddItem(newSPRMList);
	mSPRMsizeTbl.AddItem(newSPRMListSize);
	mStyleIndexTbl.AddItem(newStyleIndex);
	
	return 1;
}

int32 TExportFKPTbl::writeFKPs() 
{
	int runIndex = 0; //mEndFCTbl->CountItems;
	
	while (runIndex < mEndFCTbl.CountItems())
		runIndex = writeFKP(runIndex);
	return 1;
}

int32 TPAPXExportTbl::writeFKP(int32 firstIndex)  // new version
{
	int bytesLeft = 512;
	int runCount = 0;
	int lastIndex;
	int	pagePosition = 0;
	int pageNumber;
	int firstFC;
	BList styleIdList;
	BList stylePosList;
	long* currentEndFC;
	BX* currentBX;
	short* currentSTSHI;
	char* currentSPRMList;
	short* currentSPRMListSize;
	BinTbl*	newBinTblEntry = new BinTbl;
	
	if (firstIndex == 0)
		firstFC = mfirstFC;
	else
		firstFC = *((int32*) mEndFCTbl.ItemAt(firstIndex - 1));
	
	pageNumber = ((mDocWriter->GetStreamPos() - 1) / 512) + 1;
	pagePosition = pageNumber * 512;
	newBinTblEntry->pn = pageNumber;
	
	bytesLeft -= 2; // crun and dead byte next to it
	bytesLeft -= 4; // firstFC
	
	// figure out how many style will fit into this FKP
	for (int i = firstIndex; bytesLeft > 0 && i < mStyleIndexTbl.CountItems(); i++)
	{
		int32 styleIndex = -1;

		styleIndex = styleIdList.IndexOf(mStyleIndexTbl.ItemAt(i));

		bytesLeft -= 17;  // FC plus BX
		if (styleIndex < 0)
		{
			bytesLeft -= *((short*) mSPRMsizeTbl.ItemAt(i)) + 3; // plus two for the parent style, plus one for the sprm size byte
			if (!(*((short*) mSPRMsizeTbl.ItemAt(i)) % 2))
				bytesLeft--; // one more for the pad byte if needed
			styleIdList.AddItem(mStyleIndexTbl.ItemAt(i));	
		}
		runCount++;
	}
	if (bytesLeft < 0)
		runCount--; // bytesLeft the last run made bytesLeft go negative, and therefore did not fit.
	
	// remove all the items from the styleIdList
	while (styleIdList.CountItems())
		styleIdList.RemoveItem(0L);
	
	// now we know that runCount styles should go into this FKP
	lastIndex = firstIndex + runCount;
//	setEntryPosition(mDocWriter, pagePosition + 4); // + 4 is for the first FC
	
	int endPosition = 510;
	int startPosition = 0;
	for (int i = firstIndex; i < lastIndex; i++)
	{
		currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		currentBX = (BX*) mBXTbl.ItemAt(i);
		currentSTSHI = (short*) mSTSHITbl.ItemAt(i);
		currentSPRMList = (char*) mSPRMTbl.ItemAt(i);
		currentSPRMListSize = (short*) mSPRMsizeTbl.ItemAt(i);
		
		startPosition = endPosition - *currentSPRMListSize - 3; // -3 is the style and the run length
		
		// see if this style has aready been writen out
		int32 styleIndex = -1;
		styleIndex = styleIdList.IndexOf(mStyleIndexTbl.ItemAt(i));
		
		if 	(styleIndex >= 0)
			continue;
		
		if (startPosition % 2)
		{
			startPosition--;
			setEntryPosition(mDocWriter, pagePosition + startPosition);
			mDocWriter->WriteChar(0); 
			mDocWriter->WriteChar((*currentSPRMListSize + 2) / 2); // 2 is the parent style
		}
		else
		{
			setEntryPosition(mDocWriter, pagePosition + startPosition);
			mDocWriter->WriteChar((*currentSPRMListSize + 3) / 2); // 3 is the parent style plus this size byte
		}
		mDocWriter->WriteShort(*currentSTSHI); // this is styleIndex 
		mDocWriter->WriteBytes(currentSPRMList, *currentSPRMListSize);
		
		styleIdList.AddItem(mStyleIndexTbl.ItemAt(i));
		stylePosList.AddItem((void*)startPosition);
		
		endPosition = startPosition;
	}

	setEntryPosition(mDocWriter, pagePosition);
	mDocWriter->WriteLong(firstFC);

	for (int i = firstIndex; i < lastIndex; i++)
	{
		currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		mDocWriter->WriteLong(*currentEndFC);
		newBinTblEntry->fcMax = *currentEndFC;
	}	

	for (int i = firstIndex; i < lastIndex; i++)
	{
		currentBX = (BX*) mBXTbl.ItemAt(i);
		
		int32 styleIndex = -1;
		uchar stylePos;
		styleIndex = (int) styleIdList.IndexOf(mStyleIndexTbl.ItemAt(i));
		ASSERTC(styleIndex >= 0);		
		stylePos = ((long)(stylePosList.ItemAt(styleIndex))) / 2;
		mDocWriter->WriteChar(stylePos); //offset
		mDocWriter->WriteShort(currentBX->bxPHE.bitField1); // this is the PHE
		mDocWriter->WriteShort(currentBX->bxPHE.bitField2);
		mDocWriter->WriteLong(currentBX->bxPHE.dymLine);
		mDocWriter->WriteLong(currentBX->bxPHE.dymHeight);
	}

	setEntryPosition(mDocWriter, pagePosition + 511);
	mDocWriter->WriteChar(runCount);

	mBinTbl.AddItem(newBinTblEntry);
	
	return lastIndex;
} 


// returns the index of the next run that needs to be written out
/*int32 TPAPXExportTbl::writeFKP(int32 firstIndex) 
{
	int bytesLeft = 505; // 511 minus the crun and firstFC and dead byte just before the crun
	int nextRunSize = 0;
	int	runPartOneSize = 17;
	int runPartTwoSize = 0;
	int partOneEnd =  4;
	int partTwoStart = 510;
	int	pagePosition = 0;
	int runCount = 0;
	int pageNumber;
	int firstFC;
	int i;
	BinTbl*	newBinTblEntry = new BinTbl;
	BList styleIdList;
	BList stylePosList;
	//int i = 0;
	
	if (firstIndex == 0)
		firstFC = mfirstFC;
	else
		firstFC = *((int32*) mEndFCTbl.ItemAt(firstIndex - 1));
	
	pageNumber = ((mDocWriter->GetStreamPos() - 1) / 512) + 1;
	pagePosition = pageNumber * 512;
	newBinTblEntry->pn = pageNumber;
	setEntryPosition(mDocWriter, pagePosition);
	mDocWriter->WriteLong(firstFC);



	long* currentEndFC = (long*) mEndFCTbl.ItemAt(firstIndex);
	BX* currentBX = (BX*) mBXTbl.ItemAt(firstIndex);
	short* currentSTSHI = (short*) mSTSHITbl.ItemAt(firstIndex);
	char* currentSPRMList = (char*) mSPRMTbl.ItemAt(firstIndex);
	short* currentSPRMListSize = (short*) mSPRMsizeTbl.ItemAt(firstIndex);

	runPartTwoSize = 3 + *currentSPRMListSize;
	if (runPartTwoSize % 2 != 0)
		runPartTwoSize++; 
	
	nextRunSize = runPartOneSize + runPartTwoSize;
	
	for (i = firstIndex; nextRunSize < bytesLeft && i < mEndFCTbl.CountItems(); i++)
	{
		currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		currentBX = (BX*) mBXTbl.ItemAt(i);
		currentSTSHI = (short*) mSTSHITbl.ItemAt(i);
		currentSPRMList = (char*) mSPRMTbl.ItemAt(i);
		currentSPRMListSize = (short*) mSPRMsizeTbl.ItemAt(i);

		setEntryPosition(mDocWriter, pagePosition + partTwoStart - runPartTwoSize);
		currentBX->bxOffset = (pagePosition + partTwoStart - runPartTwoSize) / 2;
		
		if ((*currentSPRMListSize % 2) == 0)
		{
			mDocWriter->WriteChar(0); 
			mDocWriter->WriteChar((runPartTwoSize / 2) - 1); // this is papxwords,
								// if there is a pad byte then don't count this byte
		}
		else
			mDocWriter->WriteChar(runPartTwoSize / 2); // this is papxwords
			
		mDocWriter->WriteShort(*currentSTSHI); // this is styleIndex 
		mDocWriter->WriteBytes(currentSPRMList, *currentSPRMListSize);
		
		partTwoStart -= runPartTwoSize;
		bytesLeft -= runPartTwoSize + runPartOneSize;
		
		if (i < (mEndFCTbl.CountItems() - 1))
		{
			short* nextSPRMListSize = (short*) mSPRMsizeTbl.ItemAt(i + 1);
			runPartTwoSize = 3 + *nextSPRMListSize;
			if (*nextSPRMListSize % 2 == 0)
				runPartTwoSize++; 
			
			nextRunSize = runPartOneSize + runPartTwoSize;
		}
	}
	
	runCount = i - firstIndex;
	setEntryPosition(mDocWriter, pagePosition + 511);
	mDocWriter->WriteChar(runCount);
	
	
	setEntryPosition(mDocWriter, pagePosition + 4); // + 4 is for the first FC
	for (i = firstIndex; i - firstIndex < runCount; i++)
	{
		currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		mDocWriter->WriteLong(*currentEndFC);
		newBinTblEntry->fcMax = *currentEndFC;
	}
	
	for (i = firstIndex; i - firstIndex < runCount; i++)
	{
		currentBX = (BX*) mBXTbl.ItemAt(i);
		
		mDocWriter->WriteChar((char) currentBX->bxOffset); //offset
		mDocWriter->WriteShort(currentBX->bxPHE.bitField1); // this is the PHE
		mDocWriter->WriteShort(currentBX->bxPHE.bitField2);
		mDocWriter->WriteLong(currentBX->bxPHE.dymLine);
		mDocWriter->WriteLong(currentBX->bxPHE.dymHeight);
	}
	mBinTbl.AddItem(newBinTblEntry);
	
	return i;
}*/


/*int32 TPAPXExportTbl::writeFKPs() // for now we will only have on run per FKP
{
	printf("TPAPXExportTbl::writeFKPs\n");
	for (int i = 0; i < mEndFCTbl.CountItems(); i++)
	{
		long* currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		BX* currentBX = (BX*) mBXTbl.ItemAt(i);
		short* currentSTSHI = (short*) mSTSHITbl.ItemAt(i);
		char* currentSPRMList = (char*) mSPRMTbl.ItemAt(i);
		short* currentSPRMListSize = (short*) mSPRMsizeTbl.ItemAt(i);

		BinTbl*	newBinTblEntry = new BinTbl;
		
		newBinTblEntry->fcMax = *currentEndFC;
		newBinTblEntry->pn = ((mDocWriter->GetStreamPos() - 1) / 512) + 1;
		printf("this FKP will be on page %d\n", newBinTblEntry->pn);
		
		setEntryPosition(mDocWriter, newBinTblEntry->pn * 512 + 511); // this might be bigger than the file
		mDocWriter->WriteChar(1); // only one run per FKP for now
		
		mDocWriter->SetStreamPos(newBinTblEntry->pn * 512);

		if (i == 0)
			mDocWriter->WriteLong(mfirstFC);
		else
			mDocWriter->WriteLong(((BinTbl*) (mBinTbl.ItemAt(i-1)))->fcMax);

		mDocWriter->WriteLong(newBinTblEntry->fcMax);

		currentBX->bxOffset = 255 - 1 -((*currentSPRMListSize) / 2) - 1;

		mDocWriter->WriteChar((char) currentBX->bxOffset); //offset
		mDocWriter->WriteShort(currentBX->bxPHE.bitField1); // this is the PHE
		mDocWriter->WriteShort(currentBX->bxPHE.bitField2);
		mDocWriter->WriteLong(currentBX->bxPHE.dymLine);
		mDocWriter->WriteLong(currentBX->bxPHE.dymHeight);
		
		
		setEntryPosition(mDocWriter, (2 * currentBX->bxOffset) + (newBinTblEntry->pn * 512));
		
		printf("Just started writing at %d\n", (2 * currentBX->bxOffset) + (newBinTblEntry->pn * 512));
		
		if (*currentSPRMListSize % 2 == 0)
			mDocWriter->WriteChar(0); 

		//mDocWriter->WriteChar((*currentSPRMListSize / 2) + 1); // this is papxwords
		mDocWriter->WriteChar(((*currentSPRMListSize + 1) / 2) + 1); // this is papxwords
		
		mDocWriter->WriteShort(*currentSTSHI); // this is styleIndex
		
		// write out *currentSPRMList at some point
		mDocWriter->WriteBytes(currentSPRMList, *currentSPRMListSize);
		
		int32 correctPosition = newBinTblEntry->pn * 512 + 511;
		int32 actualPosition = mDocWriter->GetStreamPos();
		
	//	printf("&d&d\n", correctPosition, actualPosition);
		
		mBinTbl.AddItem(newBinTblEntry);
	}
	
	return 1;
}*/


int32 TCHPXExportTbl::addRun(int32 charCount, const TTranslatorStyle* style, TFontTblWORD* fontTbl, TStyleSheetWORD* styleTbl, bool special)
{
	int32	endFC = (mLatestFC += charCount) + mfirstFC;
	long*	newEndFC;
	BX*		newBX;
	short*	newSTSHI;
	char*	newSPRMList;
	short*	newSPRMListSize;
	int32*	newStyleIndex;
	const TTranslatorStyle* textStyle;
	
	textStyle = style;
	newEndFC = new long;
	newBX = new BX;
	newSTSHI = new short;
	newSPRMListSize = new short;
	newStyleIndex = new int32;

	newBX->bxOffset = 0;
	newBX->bxPHE.bitField1 = 0;
	newBX->bxPHE.bitField2 = 0;
	newBX->bxPHE.dymLine = 0;
	newBX->bxPHE.dymHeight = 0;

	*newStyleIndex = style->Index();
	*newSPRMListSize = GetCharSPRMListSize(textStyle, special);
	
	newSPRMList = new char[*newSPRMListSize];
	GetCharSPRMList(textStyle, newSPRMList, fontTbl, styleTbl, special);
		
	*newEndFC = endFC;
	*newSTSHI = 0;	

	const char *styleName;
	bool hasBaseStyle = style->GetBaseStyleName(&styleName);
	short stshi;
	if (hasBaseStyle)
		stshi = styleTbl->GetStyleIndex(styleName);
	else
		stshi = styleTbl->GetStyleIndex("Body");
	
	if (stshi != -1)
		*newSTSHI = stshi;
	else
		*newSTSHI = 0;
	
//	*newSTSHI = 0;	// BOGUS NO STYLES TEST
	
	mEndFCTbl.AddItem(newEndFC);
	mBXTbl.AddItem(newBX);
	mSTSHITbl.AddItem(newSTSHI);
	mSPRMTbl.AddItem(newSPRMList);
	mSPRMsizeTbl.AddItem(newSPRMListSize);
	mStyleIndexTbl.AddItem(newStyleIndex);
	
	return 1;
}

/*int32 TCHPXExportTbl::writeFKPs() 
{
	int runIndex = 0; //mEndFCTbl->CountItems;
	
	while (runIndex < mEndFCTbl.CountItems())
		runIndex = writeFKP(runIndex);
	return 1;
}*/


// returns the index of the next run that needs to be written out
int32 TCHPXExportTbl::writeFKP(int32 firstIndex)  // new version
{
	int bytesLeft = 512;
	int runCount = 0;
	int lastIndex;
	int	pagePosition = 0;
	int pageNumber;
	int firstFC;
	BList styleIdList;
	BList stylePosList;
	long* currentEndFC;
	BX* currentBX;
	short* currentSTSHI;
	char* currentSPRMList;
	short* currentSPRMListSize;
	BinTbl*	newBinTblEntry = new BinTbl;
	
	if (firstIndex == 0)
		firstFC = mfirstFC;
	else
		firstFC = *((int32*) mEndFCTbl.ItemAt(firstIndex - 1));
	
	pageNumber = ((mDocWriter->GetStreamPos() - 1) / 512) + 1;
	pagePosition = pageNumber * 512;
	newBinTblEntry->pn = pageNumber;
	
	bytesLeft -= 2; // crun and dead byte next to it
	bytesLeft -= 4; // firstFC
	
	// figure out how many style will fit into this FKP
	for (int i = firstIndex; bytesLeft > 0 && i < mStyleIndexTbl.CountItems(); i++)
	{
		int32 styleIndex = -1;

		styleIndex = styleIdList.IndexOf(mStyleIndexTbl.ItemAt(i));

		bytesLeft -= 5;  // FC and pointer to the style
		if (styleIndex < 0 && *((short*) mSPRMsizeTbl.ItemAt(i)) != 0)
		{
			bytesLeft -= *((short*) mSPRMsizeTbl.ItemAt(i)) + 1; // one for the sprm size byte
			if (!(*((short*) mSPRMsizeTbl.ItemAt(i)) % 2))
				bytesLeft--; // one more for the pad byte if needed
			styleIdList.AddItem(mStyleIndexTbl.ItemAt(i));	
		}
		runCount++;
	}
	if (bytesLeft < 0)
		runCount--; // bytesLeft the last run made bytesLeft go negative, and therefore did not fit.
	
	// remove all the items from the styleIdList
	while (styleIdList.CountItems())
		styleIdList.RemoveItem(0L);
	
	// now we know that runCount styles should go into this FKP
	lastIndex = firstIndex + runCount;
//	setEntryPosition(mDocWriter, pagePosition + 4); // + 4 is for the first FC
	
	int endPosition = 510;
	int startPosition = 0;
	for (int i = firstIndex; i < lastIndex; i++)
	{
		currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		currentBX = (BX*) mBXTbl.ItemAt(i);
		currentSTSHI = (short*) mSTSHITbl.ItemAt(i);
		currentSPRMList = (char*) mSPRMTbl.ItemAt(i);
		currentSPRMListSize = (short*) mSPRMsizeTbl.ItemAt(i);
		
		startPosition = endPosition - *currentSPRMListSize - 1; // - 1 is the run length
		
		// see if this style has aready been writen out
		int32 styleIndex = -1;
		styleIndex = styleIdList.IndexOf(mStyleIndexTbl.ItemAt(i));
		
		if 	(styleIndex >= 0)
			continue;
		
		if (startPosition % 2)
			startPosition--;
		
		if (*currentSPRMListSize)
		{
			setEntryPosition(mDocWriter, pagePosition + startPosition);
			mDocWriter->WriteChar(*currentSPRMListSize);
			mDocWriter->WriteBytes(currentSPRMList, *currentSPRMListSize);
			
			styleIdList.AddItem(mStyleIndexTbl.ItemAt(i));
			stylePosList.AddItem((void*)startPosition);
		}
		else
		{
			styleIdList.AddItem(mStyleIndexTbl.ItemAt(i));
			stylePosList.AddItem(0);
			continue;
		}
		
		endPosition = startPosition;
	}

	setEntryPosition(mDocWriter, pagePosition);
	mDocWriter->WriteLong(firstFC);

	for (int i = firstIndex; i < lastIndex; i++)
	{
		currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		mDocWriter->WriteLong(*currentEndFC);
		newBinTblEntry->fcMax = *currentEndFC;
	}	

	for (int i = firstIndex; i < lastIndex; i++)
	{
		int32 styleIndex = -1;
		uchar stylePos;
		styleIndex = (int) styleIdList.IndexOf(mStyleIndexTbl.ItemAt(i));
		ASSERTC(styleIndex >= 0);		
		stylePos = ((long)(stylePosList.ItemAt(styleIndex))) / 2;
		mDocWriter->WriteChar(stylePos); //offset
	}

	setEntryPosition(mDocWriter, pagePosition + 511);
	mDocWriter->WriteChar(runCount);

	mBinTbl.AddItem(newBinTblEntry);
	
	return lastIndex;
} 

/*int32 TCHPXExportTbl::writeFKP(int32 firstIndex) 
{
	int bytesLeft = 505; // 511 minus the crun and firstFC and dead byte just before the crun
	int nextRunSize = 0;
	int	runPartOneSize = 5;
	int runPartTwoSize = 0;
	int partOneEnd =  4;
	int partTwoStart = 510;
	int	pagePosition = 0;
	int runCount = 0;
	int pageNumber;
	int firstFC;
	int i;
	BinTbl*	newBinTblEntry = new BinTbl;
	//int i = 0;
	
	if (firstIndex == 0)
		firstFC = mfirstFC;
	else
		firstFC = *((int32*) mEndFCTbl.ItemAt(firstIndex - 1));
	
	pageNumber = ((mDocWriter->GetStreamPos() - 1) / 512) + 1;
	pagePosition = pageNumber * 512;
	newBinTblEntry->pn = pageNumber;
	setEntryPosition(mDocWriter, pagePosition);
	mDocWriter->WriteLong(firstFC);



	long* currentEndFC = (long*) mEndFCTbl.ItemAt(firstIndex);
	BX* currentBX = (BX*) mBXTbl.ItemAt(firstIndex);
	short* currentSTSHI = (short*) mSTSHITbl.ItemAt(firstIndex);
	char* currentSPRMList = (char*) mSPRMTbl.ItemAt(firstIndex);
	short* currentSPRMListSize = (short*) mSPRMsizeTbl.ItemAt(firstIndex);
		
	if (*currentSPRMListSize)
		runPartTwoSize = 1 + *currentSPRMListSize;
	else
		runPartTwoSize = 0;
	
	if (runPartTwoSize % 2)
		runPartTwoSize++;
	nextRunSize = runPartOneSize + runPartTwoSize;
	
	for (i = firstIndex; nextRunSize < bytesLeft && i < mEndFCTbl.CountItems(); i++)
	{
		currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		currentBX = (BX*) mBXTbl.ItemAt(i);
		currentSTSHI = (short*) mSTSHITbl.ItemAt(i);
		currentSPRMList = (char*) mSPRMTbl.ItemAt(i);
		currentSPRMListSize = (short*) mSPRMsizeTbl.ItemAt(i);

		setEntryPosition(mDocWriter, pagePosition + partTwoStart - runPartTwoSize);
		if (runPartTwoSize)
		{
			currentBX->bxOffset = (pagePosition + partTwoStart - runPartTwoSize) / 2;
			
			mDocWriter->WriteChar((uchar) *currentSPRMListSize);
			mDocWriter->WriteBytes(currentSPRMList, *currentSPRMListSize);
		}
		else
			currentBX->bxOffset = 0;
		
		
		partTwoStart -= runPartTwoSize;
		bytesLeft -= runPartTwoSize + runPartOneSize;
		
		if (i < (mEndFCTbl.CountItems() - 1))
		{
			short* nextSPRMListSize = (short*) mSPRMsizeTbl.ItemAt(i + 1);
			if (*nextSPRMListSize)
				runPartTwoSize = 1 + *nextSPRMListSize;
			else
				runPartTwoSize = 0;
			
			if (runPartTwoSize % 2)
				runPartTwoSize++;
			
			nextRunSize = runPartOneSize + runPartTwoSize;
		}
	}
	
	runCount = i - firstIndex;
	setEntryPosition(mDocWriter, pagePosition + 511);
	mDocWriter->WriteChar(runCount);
	
	
	setEntryPosition(mDocWriter, pagePosition + 4); // + 4 is for the first FC
	for (i = firstIndex; i - firstIndex < runCount; i++)
	{
		currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		mDocWriter->WriteLong(*currentEndFC);
		newBinTblEntry->fcMax = *currentEndFC;
	}
	
	for (i = firstIndex; i - firstIndex < runCount; i++)
	{
		currentBX = (BX*) mBXTbl.ItemAt(i);
		
		mDocWriter->WriteChar((char) currentBX->bxOffset); //offset
	}
	mBinTbl.AddItem(newBinTblEntry);
	
	return i;
}*/



/*int32 TCHPXExportTbl::writeFKPs() // for now we will only have on run per FKP
{
	printf("TCHPXExportTbl::writeFKPs\n");
	for (int i = 0; i < mEndFCTbl.CountItems(); i++)
	{
		long* currentEndFC = (long*) mEndFCTbl.ItemAt(i);
		BX* currentBX = (BX*) mBXTbl.ItemAt(i);
		short* currentSTSHI = (short*) mSTSHITbl.ItemAt(i);
		char* currentSPRMList = (char*) mSPRMTbl.ItemAt(i);
		short* currentSPRMListsize = (short*) mSPRMsizeTbl.ItemAt(i);

		BinTbl*	newBinTblEntry = new BinTbl;
		
		newBinTblEntry->fcMax = *currentEndFC;
		newBinTblEntry->pn = ((mDocWriter->GetStreamPos() - 1) / 512) + 1;
		
		setEntryPosition(mDocWriter, newBinTblEntry->pn * 512 + 511);
		mDocWriter->WriteChar(1); // only one run per FKP for now
		
		mDocWriter->SetStreamPos(newBinTblEntry->pn * 512);

		if (i == 0)
			mDocWriter->WriteLong(mfirstFC);
		else
			mDocWriter->WriteLong(((BinTbl*) (mBinTbl.ItemAt(i-1)))->fcMax);
		mDocWriter->WriteLong(newBinTblEntry->fcMax);
		
		//currentBX->bxOffset = 0; // keep it default for now
		if (*currentSPRMListsize)
		{
			currentBX->bxOffset = (511 - *currentSPRMListsize - 1) / 2;
			mDocWriter->WriteChar(currentBX->bxOffset); //offset
		
			mDocWriter->SetStreamPos((currentBX->bxOffset * 2) + (newBinTblEntry->pn * 512));
			mDocWriter->WriteChar((uchar) *currentSPRMListsize);
			mDocWriter->WriteBytes(currentSPRMList, *currentSPRMListsize);
		}
		else
		{
			currentBX->bxOffset = 0;
			mDocWriter->WriteChar(currentBX->bxOffset); //offset
		}


		mBinTbl.AddItem(newBinTblEntry);

	}
	
	return 1;
}*/





