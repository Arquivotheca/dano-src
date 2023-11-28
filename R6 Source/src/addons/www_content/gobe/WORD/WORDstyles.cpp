//
//	WORDstyles.cpp
//

#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <algobase.h>		// for min and max

#include "WORDstyles.h"

const rgb_color word_colors[WORD_COLOR_COUNT] =
{
	{0, 0, 0, 0}, // 0
	{0, 0, 0, 0},
	{0, 0, 255, 0},
	{0, 255, 255, 0},
	{0, 255, 0, 0},
	{255, 0, 255, 0}, // 5
	{255, 0, 0, 0},
	{255, 255, 0, 0},
	{255, 255, 255, 0},
	{0, 0, 128, 0},
	{0, 128, 128, 0}, // 10 
	{0, 128, 0, 0},
	{128, 0, 128, 0},
	{128, 0, 0, 0},
	{128, 128, 0, 0},
	{128, 128, 128, 0}, // 15
	{192, 192, 192, 0}
};

const uint16 StyleIndexes[STYLE_COUNT] =
{
	0,		//	"Normal",
	1,		//	"Heading 1",
	2,		//	"Heading 2",
	3,		//	"Heading 3",
	4,		//	"Heading 4",
	5,		//	"Heading 5",
	6,		//	"Heading 6",
	7,		//	"Heading 7",
	8,		//	"Heading 8",
	9,		//	"Heading 9",
	10,		//	"Index 1",
	11,		//	"Index 2",
	12,		//	"Index 3",
	13,		//	"Index 4",
	14,		//	"Index 5",
	15,		//	"Index 6",
	16,		//	"Index 7",
	17,		//	"Index 8",
	18,		//	"Index 9",
	29,		//	"Footnote Text",
	84,		//	"Block Text",
	34,		//	"Caption",
	28,		//	"Normal Indent",
	31,		//	"Header",
	32,		//	"Footer",
	48,		//	"List Bullet",
	49,		//	"List Number",
	38,		//	"Footnote Reference",
	41,		//	"Page Number",
	43,		//	"Endnote Text",
	42,		//	"Endnote Reference"
};


//-------------------------------------------------------------------
// TFilterStyleWORD
//-------------------------------------------------------------------

TFilterStyleWORD::TFilterStyleWORD()
	: TTranslatorStyle()
{
	mSpecialChar = false;
	mEmbossedImprint = false;
	mPicFC = 0;
	
	mInTable = false;
	mEndTableRow = false;
	mRowHeight = 12;
	//mTableGapHalf = 0;
	mCellsInRow = 0;
}

TFilterStyleWORD::~TFilterStyleWORD()
{
	for (int i = 0; i < mTCList.CountItems(); i++)
		delete ((TC*) mTCList.ItemAt(i));
}

//-------------------------------------------------------------------
// TStyleWORD
//-------------------------------------------------------------------

TStyleWORD::TStyleWORD(TOLEEntryReader *tableReader, long bytesSTD, ulong bytesSTDbase, short istd, int16 nFib)
{
	long origStreamPos = tableReader->GetStreamPos();
	
	mNFib = nFib;
	initFields(istd);
	
	IFDEBUG(fprintf(stderr, "\nReading Style with ISTD %d\n", istd));
	// init STD_BASE to zeros before reading in case this version doesn't have all fields
	memset(&(mSTDbase.bitField1), 0, sizeof(STD_BASE));
	tableReader->ReadBytes((char *)&(mSTDbase.bitField1), min(bytesSTDbase, sizeof(STD_BASE)));
	
	// need to do swapping since we read bytes instead of shorts
	mSTDbase.bitField1 = B_LENDIAN_TO_HOST_INT16(mSTDbase.bitField1);
	mSTDbase.bitField2 = B_LENDIAN_TO_HOST_INT16(mSTDbase.bitField2);
	mSTDbase.bitField3 = B_LENDIAN_TO_HOST_INT16(mSTDbase.bitField3);
	mSTDbase.bchUpe = B_LENDIAN_TO_HOST_INT16(mSTDbase.bchUpe);
	mSTDbase.bitField4 = B_LENDIAN_TO_HOST_INT16(mSTDbase.bitField4);

#ifdef DEBUG
	fprintf(stderr, "mSTDbase = {");
	for (int i = 0; i < (int) sizeof(STD_BASE); i++)
	{
		if (!(i%16))
			fprintf(stderr, "\n");
		fprintf(stderr, "%#2.2hx ", ((uchar *)&(mSTDbase.bitField1))[i]);
	}
	fprintf(stderr, "}\n");
	
	fprintf(stderr, "StyleID = %d\n", GetStyleID());
	fprintf(stderr, "StyleTypeCode = %d\n", GetStyleTypeCode());
	fprintf(stderr, "BaseStyleIndex = %d\n", GetBaseStyleIndex());
	fprintf(stderr, "CUPX = %d\n", GetCUPX());
	fprintf(stderr, "StyleIndexNext = %d\n", GetStyleIndexNext());
	fprintf(stderr, "BchUPE = %d\n", GetBchUPE());
#endif // DEBUG
	// Set the stream up for the variable portion of the STD
	if (tableReader->Error() < 0)
		return;


	tableReader->SetStreamPos(origStreamPos + bytesSTDbase);

	// ALL OF THE VARIABLE PORTIONS OF THE STD MUST BE ON EVEN-BYTE BOUNDARIES

	// set to even byte boundary
/*	long streamPos = tableReader->GetStreamPos();
	if (streamPos % 2)
		tableReader->SetStreamPos(streamPos + 1);
	*/
	if ((tableReader->GetStreamPos() - origStreamPos) % 2)
		tableReader->ReadChar();
	
	//
	// STYLE NAME - this is a two-byte extended character string in pascal format
	//
	
	if (mNFib > 190)
	{
		short 	charsName = tableReader->ReadShort();
		if (tableReader->Error() < 0)
			return;
		int32	srcBytes = (2 * charsName);
		char *	tempName = new char[srcBytes];		// it's a unicode string
		ushort	tempShort;
		// we need to swap the unicode characters as they come in
		for (long i = 0; i < charsName; i++)
		{
			tableReader->ReadBytes((char*)&tempShort, 2);
			tempShort = B_SWAP_INT16(tempShort);
			memcpy((char *)&tempName[i * 2], (char*)&tempShort, 2);
		}
		// read the null terminator	
		tempShort = tableReader->ReadShort();
		// convert name to utf8
		int32 dstBytes = (4 * charsName) + 1;
		mStyleName = new char[dstBytes];		// allow more space for utf8
		int32 state = 0;
		convert_to_utf8(B_UNICODE_CONVERSION, tempName, &srcBytes, mStyleName, &dstBytes, &state);
		mStyleName[dstBytes] = 0;	
		delete [] tempName;
	}
	else
	{
		uchar 	charsName = tableReader->ReadChar();
		if (tableReader->Error() < 0)
			return;
		int32	srcBytes = charsName;
		char *	tempName = new char[srcBytes];
		uchar	tempChar;
		
		for (long i = 0; i < charsName; i++)
			tempName[i] = tableReader->ReadChar();
		
		tempChar = tableReader->ReadChar();
		
		int32 dstBytes = (4 * charsName) + 1;
		mStyleName = new char[dstBytes];		// allow more space for utf8
		int32 state = 0;
		convert_to_utf8(B_MS_WINDOWS_CONVERSION, tempName, &srcBytes, mStyleName, &dstBytes, &state);
		mStyleName[dstBytes] = 0;	
		delete [] tempName;
		
	}
	
	IFDEBUG(fprintf(stderr, "style name is %s\n", mStyleName));
	
	
	//
	// UPX
	//
	ushort	upxCount = GetCUPX();
	ushort	styleType = GetStyleTypeCode();
	
	//IFDEBUG(fprintf(stderr, "WORD STD istd:%d, sti:%d, %s, base: %d, next: %d, %s\n", mISTD, GetStyleID(), ((styleType == 1) ? "Para" : "Char"), GetBaseStyleIndex(), GetStyleIndexNext(), mStyleName));

	// We only understand paragraph and character UPX types,
	// and they'll be the first two.
	// StyleType == 1 is both (par and char), while 2 is char only
	//IFDEBUG(fprintf(stderr, "\tbytesSTD %d\n", bytesSTD));
	for (long i = 0; i < upxCount && i < 2; i++)
	{	
		// Each UPX starts on an even byte boundary
/*		streamPos = tableReader->GetStreamPos();
		if (streamPos % 2)
			tableReader->SetStreamPos(streamPos + 1);*/
		if ((tableReader->GetStreamPos() - origStreamPos) % 2)
			tableReader->ReadChar();
			
		short bytesUPX = tableReader->ReadShort();
		IFDEBUG(fprintf(stderr, "there are %d bytes in the UPX\n", bytesUPX));
		short posUPX = 0;
		char *tempPtr;
		
		if (upxCount == 1 || i == 1)
		{
			mUPXcharBytes = bytesUPX;
			tempPtr = mUPXchar = new char[bytesUPX + 1]; // added one because I found file where mUPXchar is one smaller
		}												// than the actual size of the list of SPRMs. At least this way we don't clobber memory. 
		else
		{
			IFDEBUG(fprintf(stderr, "in else, about to read a short\n"));
			if (bytesUPX >= 2 || mNFib > 190) // it seems that in word 6 if the bytesUPX is 0 then you don't write out the UPXparaISTD
			{
				mUPXparaISTD = tableReader->ReadShort();
				IFDEBUG(fprintf(stderr, "mUPXparaISTD is %d\n", mUPXparaISTD));
				if (bytesUPX >= 2)	// I have run into files where the bytesUPX did not include mUPXparaISTD	
					bytesUPX -= 2;	// allow for istd in paragraph UPX
			}

			mUPXparaBytes = bytesUPX;
			tempPtr = mUPXpara = new char[bytesUPX];			
		}
		
#ifdef DEBUG
		long tempStreamPos = tableReader->GetStreamPos();
		
		char *upxarray = new char[bytesUPX];
		tableReader->ReadBytes(upxarray, bytesUPX);
		
		fprintf(stderr, "sprms for %s = {", (upxCount == 1 || i == 1) ? "UPXchar" : "UPXpara");
		for (int i = 0; i < bytesUPX; i++)
		{
			if (!(i%16))
				fprintf(stderr, "\n");
			fprintf(stderr, "%#2.2hx ", (uchar) upxarray[i]);
		}
		fprintf(stderr, "}\n");
		delete [] upxarray;
		
		tableReader->SetStreamPos(tempStreamPos);
		
		fprintf(stderr, "Steam position is %d", tempStreamPos);
#endif // DEBUG
		
		
		// Read the SPRMs
		ushort	theSprm;
		short	sprmBytes;
		while (posUPX < bytesUPX)
		{
			sprmBytes = ReadSPRM(tableReader, tempPtr, theSprm, mNFib);
			if 	(sprmBytes < 0)
				return;
			//IFDEBUG(fprintf(stderr, "\tsprm: %04x, sprmBytes: %d, sprm+1: 0x%02x\n", theSprm, sprmBytes, (uchar)tempPtr[2]));
			posUPX += sprmBytes;
			tempPtr += sprmBytes;
		}
		if (tableReader->Error() < 0)
			return;
	}
	
	//
	// All done, so make sure stream position is set to the next STD
	//
	tableReader->SetStreamPos(origStreamPos + bytesSTD);
}

TStyleWORD::TStyleWORD(short istd)
{
	initFields(istd);
}


TStyleWORD::~TStyleWORD()
{
		delete [] mStyleName;
		delete [] mUPXpara;
		delete [] mUPXchar;	
	//if (mFilterStyle) // this will now belongs to traslator doc
	//	delete mFilterStyle;		
}

void TStyleWORD::SetStyleName(const char* name)
{
	mStyleName = new char[strlen(name) + 1];
	strcpy(mStyleName, name);
}


void TStyleWORD::SetStyleID(short newSTI)
{
	// clear any old value
	mSTDbase.bitField1 &= ~sti;
	// set new value
	mSTDbase.bitField1 |= (newSTI & sti);
}


void TStyleWORD::SetStyleTypeCode(short newSGC)
{
	// clear any old value
	mSTDbase.bitField2 &= ~sgc;
	// set new value
	mSTDbase.bitField2 |= (newSGC & sgc);
}

int32 TStyleWORD::Write(TOLEEntryWriter *tableWriter)
{
	if(IsNull())
		return B_NO_ERROR;
	
	int32 originalPos = tableWriter->GetStreamPos();
	STD_BASE LEndianBase;
	
	IFDEBUG(fprintf(stderr, "theStyle %s is being writen out at position %d.\n", mStyleName, tableWriter->GetStreamPos()));
	
	// need to do swapping since we write bytes instead of shorts
	LEndianBase.bitField1 = B_HOST_TO_LENDIAN_INT16(mSTDbase.bitField1);
	LEndianBase.bitField2 = B_HOST_TO_LENDIAN_INT16(mSTDbase.bitField2);
	LEndianBase.bitField3 = B_HOST_TO_LENDIAN_INT16(mSTDbase.bitField3);
	LEndianBase.bchUpe = B_HOST_TO_LENDIAN_INT16(mSTDbase.bchUpe);
	LEndianBase.bitField4 = B_HOST_TO_LENDIAN_INT16(mSTDbase.bitField4);

	tableWriter->WriteBytes((char *)&(LEndianBase.bitField1), sizeof(STD_BASE));

	if ((tableWriter->GetStreamPos() - originalPos) % 2)
		tableWriter->WriteChar(0);
	
	
	int32 charsName = strlen(mStyleName);
	int32 charsUTFName = charsName * 2;
	char* UTFName = new char[charsUTFName];
	int32 state = 0;
	convert_from_utf8(B_UNICODE_CONVERSION, mStyleName, &charsName, UTFName, &charsUTFName, &state, '?');
	
	
	tableWriter->WriteShort(charsName);
	for (int i = 0; i < charsName; i++)
	{
		*((short*) &(UTFName[i*2])) = B_SWAP_INT16(*((short*) &(UTFName[i*2])));
		tableWriter->WriteBytes((char*) &(UTFName[i*2]), 2);
	}
	tableWriter->WriteShort(0); // string terminator
	
	
	if ((tableWriter->GetStreamPos() - originalPos) % 2)
		tableWriter->WriteChar(0);
	
	IFDEBUG(int32 getUPXVal = GetCUPX());
	IFDEBUG(fprintf(stderr, "%d\n", getUPXVal));
	
	if (GetCUPX() == 2)
	{
		tableWriter->WriteShort(mUPXparaBytes + 2);
		tableWriter->WriteShort(mUPXparaISTD);
		tableWriter->WriteBytes(mUPXpara, mUPXparaBytes);
	}
	
	if ((tableWriter->GetStreamPos() - originalPos) % 2)
		tableWriter->WriteChar(0);

	tableWriter->WriteShort(mUPXcharBytes);
	tableWriter->WriteBytes(mUPXchar, mUPXcharBytes);

	return B_NO_ERROR;
}

int32 TStyleWORD::GetBytes()
{
	int32 size = 0;
		
	if(IsNull())
		return 0;
	
	size += sizeof(STD_BASE);
	
	if (size % 2)
		size++;
	
	size += 2; // name length
	size += (strlen(mStyleName) + 1) * 2; // length of the name plus null terminator, in Unicode

	if (size % 2)
		size++;

	if (GetCUPX() == 2)
	{
		size += 2;
		size += 2;
		size += mUPXparaBytes;
	}
	
	if (size % 2)
		size++;

	size += 2;
	size += mUPXcharBytes;
	
	return size;
}

void TStyleWORD::SetBaseStyleIndex(short newBase)
{
	// clear any old value
	mSTDbase.bitField2 &= ~istdBase;
	// set new value
	mSTDbase.bitField2 |= ((newBase << 4) & istdBase);
}

void TStyleWORD::SetCUPX(ushort count)
{
	// clear any old value
	mSTDbase.bitField3 &= ~cupx;
	// set new value
	mSTDbase.bitField3 |= (count & cupx);
}

void TStyleWORD::SetStyleIndexNext(ushort index)
{
	// clear any old value
	mSTDbase.bitField3 &= ~istdNext;
	// set new value
	mSTDbase.bitField3 |= ((index << 4) & istdNext);
}

void TStyleWORD::SetHasUPE(ushort value)
{
	// clear any old value
	mSTDbase.bitField1 &= ~fHasUpe;
	// set new value
	mSTDbase.bitField1 |= ((value << 14) & fHasUpe);
}

void TStyleWORD::SetUPXPara(const char* sprms, short bytes)
{
	mUPXparaBytes = bytes;
	mUPXpara = new char[bytes];
	memcpy(mUPXpara, sprms, bytes);
	
	
}
	
void TStyleWORD::SetUPXChar(const char* sprms, short bytes)
{
	mUPXcharBytes = bytes;
	mUPXchar = new char[bytes];
	memcpy(mUPXchar, sprms, bytes);
}
	
void TStyleWORD::initFields(short istd)
{
	memset(&mSTDbase.bitField1, 0, sizeof(STD_BASE));
	mStyleName = 0;
	mUPXparaBytes = 0;
	mUPXpara = 0;
	mUPXcharBytes = 0;
	mUPXchar = 0;
	mFilterStyle = 0;
	mNullStyle = 0;
	mSent = 0;
	mISTD = istd;
	mUPXparaISTD = istd;
	SetBaseStyleIndex(nullBaseStyle);
	// SetBaseStyleIndex(istd); // don't know why this was like this - joel
}


// Create a TFilterStyleWORD for this STD style.  All of the STDs must have been loaded
// when this is called, but this will expand any filterStyles for parents that haven't
// been updated yet.
void TStyleWORD::BuildFilterStyle(BList *tableSTD, TFontTblWORD *fontTblWORD, TStyleSheetWORD* styleTblWORD, ushort defaultFTC)
{
	if (mFilterStyle)
		return;	// already built
		
	ushort	baseStyleIndex = GetBaseStyleIndex();
	if (baseStyleIndex != nullBaseStyle && baseStyleIndex >= tableSTD->CountItems())
	{
		IFDEBUG(fprintf(stderr, "WORD::BuildFilterStyle - invalid baseStyleIndex: %d\n", baseStyleIndex));
		return;
	}	
	
	TStyleWORD *stdParent = 0;
	
	// if this std is based on another, make sure that the parent is built first
	if (baseStyleIndex != nullBaseStyle)
	{
		stdParent = (TStyleWORD *)(tableSTD->ItemAt(baseStyleIndex));
		if (!stdParent)
		{
			IFDEBUG(fprintf(stderr, "WORD::BuildFilterStyle - baseStyleIndex is null STD: %d\n", baseStyleIndex));
			return;
		}
		// build style if not already built
		if (!stdParent->GetFilterStyle())
			stdParent->BuildFilterStyle(tableSTD, fontTblWORD, styleTblWORD, defaultFTC);
	}
	
	// look at the sti for this STD.  There are a bunch of built-in styles that it could be,
	// or a user-defined or null style.
	ushort sti = GetStyleID();
	if (sti > stiMax && sti != stiUser && sti != stiNil)
		IFDEBUG(fprintf(stderr, "WORD::BuildFilterStyle - unknown sti: %d\n", sti));
		
	// At this point, all of the parent filter styles have been built,
	// so we can now build this style.
	mFilterStyle = new TFilterStyleWORD();
	mFilterStyle->SetStyleName(StyleName());
	mFilterStyle->SetStyleType((GetStyleTypeCode() == sgcPara) ? TTranslatorStyle::kParagraphStyle : TTranslatorStyle::kBasicStyle);
	
	if (stdParent)
	{
		ushort parentType = stdParent->GetStyleTypeCode();

		if (GetStyleTypeCode() == sgcPara || parentType == sgcChp)
			mFilterStyle->SetBaseStyleName(stdParent->StyleName());
		
		// Header and Footer conflict with Squirrel styles, so make them MSHeader and MSFooter
		if (sti == stiHeader || sti == stiFooter)
			mFilterStyle->SetStyleName(&(BuiltInStyleNamesTable[sti].name[0]));	
	}
	else
	{							
		switch (sti)
		{
			case stiNormal:
			case stiNormalChar:
				mFilterStyle->SetTextFont(fontTblWORD->ConvertFTCtoBeFontID(defaultFTC));
				mFilterStyle->SetTextSize(10.0);
				break;
						
			default:
				IFDEBUG(fprintf(stderr, "WORD::BuildFilterStyle - unknown sti with no parent: %d\n", sti));
		}
	}
	
	// apply paragraph UPX sprms
	if (mUPXparaBytes && mUPXpara)
		ApplyAllSprms(mUPXpara, mUPXparaBytes, mFilterStyle, S_PARA, fontTblWORD, styleTblWORD, mNFib);
	
	// apply character UPX sprms
	if (mUPXcharBytes && mUPXchar)
		ApplyAllSprms(mUPXchar, mUPXcharBytes, mFilterStyle, S_CHAR, fontTblWORD, styleTblWORD, mNFib);
}

//-------------------------------------------------------------------
// TStyleSheetWORD
//-------------------------------------------------------------------

TStyleSheetWORD::TStyleSheetWORD(TOLEEntryReader *tableReader, long tableStreamSize, long offset, ulong offsetBytes, TFontTblWORD *fontTblWORD, int16 nFib)
{
	mTableReader = tableReader;
	mTableStreamSize = tableStreamSize;
	mOffsetSTSH = offset;
	mBytesSTSH = offsetBytes;
	mNFib = nFib;
	
	// make sure that we don't go off the edge of the stream
	if ((offset + offsetBytes) > (ulong) tableStreamSize)
		IFDEBUG(fprintf(stderr, "WORD TStyleSheetWORD: ERROR: %d offset, %d offsetBytes, %d tableStreamSize\n", offset, offsetBytes, tableStreamSize));
	else
	{
		IFDEBUG(fprintf(stderr, "WORD TStyleSheetWORD: offset %d, offsetBytes %d\n", offset, offsetBytes));
		mTableReader->SetStreamPos(mOffsetSTSH);

		// length of STSHI (this may be longer or shorter than our STSHI definition)
		ushort	bytesSTSHI = mTableReader->ReadShort();
		
		if (readSTSHI(bytesSTSHI))
		{		
			//
			// Now get the STDs
			//
			mTableSTD = new BList(50);
					
			long			emptySTDs = 0;
			short			bytesSTD;
			TStyleWORD *	std;
			mTableReader->SetStreamPos(offset + 2 + bytesSTSHI);	// 2 is the bytesSTSHI that was read in
		
			//IFDEBUG(fprintf(stderr, "WORD Styles - %i STDs, %d stiMax, %d istdMax\n", mHeader.cstd, mHeader.stiMaxWhenSaved, mHeader.istdMaxFixedWhenSaved));
			
			IFDEBUG(fprintf(stderr, "there are %d styles\n", mHeader.cstd));
			
		  	for (short i = 0; i < mHeader.cstd; i++)
			{
				std = 0;
				
				bytesSTD = mTableReader->ReadShort();
				if (bytesSTD > 0)
					std = new TStyleWORD(mTableReader, bytesSTD, mHeader.cbSTDBaseInFile, i, mNFib);
				else
				{
					emptySTDs++;
					//IFDEBUG(fprintf(stderr, "WORD Styles - empty STD %d\n", i));
				}
				
				// stream pos should be in the correct spot after TStyleWORD is done	
		
				mTableSTD->AddItem(std);
			}
		
			//IFDEBUG(fprintf(stderr, "WORD Styles - %d STDs are empty\n", emptySTDs));
			
			// Now, go thru all the STDs we have and create a TFilterStyleWORD for each of them.
			// Need to have a separate loop here, as some styles depend on others,
			// and we need to have them all loaded at this point.
			for (long i = 0; i < mTableSTD->CountItems(); i++)
			{
				std = (TStyleWORD *)(mTableSTD->ItemAt(i));
				if (std)
					// Always getting the 0-ftc (ascii) here.  How do I know when to use 1-ftc (FarEast?)
					std->BuildFilterStyle(mTableSTD, fontTblWORD, this, GetDefaultFTC(0));
			}
		}
	}
}

TStyleSheetWORD::TStyleSheetWORD()
{
	mTableSTD = new BList(50);
	TStyleWORD* newStyle;
	
	newStyle = new TStyleWORD(mTableSTD->CountItems());
	newStyle->SetStyleID(0);
	newStyle->SetStyleTypeCode(1);
	newStyle->SetBaseStyleIndex(4095);
	newStyle->SetCUPX(2);
	newStyle->SetStyleIndexNext(0);
	newStyle->SetStyleName("Normal");
	newStyle->SetHasUPE(1);

	char norm_char_upx[] = {0x6d, 0x48, 0x09, 0x04};
	
	newStyle->SetUPXPara(0, 0);
	newStyle->SetUPXChar(norm_char_upx, sizeof(norm_char_upx));
	
	mTableSTD->AddItem(newStyle);
	
	for (int i = 0; i < 9; i++)
	{
		newStyle = new TStyleWORD(mTableSTD->CountItems());
		newStyle->SetNull(true);
		mTableSTD->AddItem(newStyle);
	}
	
	newStyle = new TStyleWORD(mTableSTD->CountItems());
	newStyle->SetStyleID(65);
	newStyle->SetStyleTypeCode(2);
	newStyle->SetBaseStyleIndex(4095);
	newStyle->SetCUPX(1);
	newStyle->SetStyleIndexNext(10);
	newStyle->SetStyleName("Default Paragraph Font");
	newStyle->SetHasUPE(1);
	
	newStyle->SetUPXChar(0, 0);
	
	mTableSTD->AddItem(newStyle);
	
	for (int i = 0; i < 5; i++)
	{
		newStyle = new TStyleWORD(mTableSTD->CountItems());
		newStyle->SetNull(true);
		mTableSTD->AddItem(newStyle);
	}
}

TStyleSheetWORD::~TStyleSheetWORD()
{		
	if (mTableSTD)
	{	
		//for (long i = mTableSTD->CountItems() - 1; i >= 0; i--)
		//	delete (TStyleWORD *)(mTableSTD->ItemAt(i));
		while(mTableSTD->CountItems())
			delete (TStyleWORD *)mTableSTD->RemoveItem(0L);
		
		delete mTableSTD;
	}
}


bool TStyleSheetWORD::readSTSHI(ushort bytesSTSHI)
{
	// length of STSHI (this may be longer or shorter than our STSHI definition)
	if ((mOffsetSTSH + bytesSTSHI) > mTableStreamSize)
	{
		IFDEBUG(fprintf(stderr, "WORD readSTSHI: ERROR: %d offset, %d bytesSTSHI, %d tableStreamSize\n", mOffsetSTSH, bytesSTSHI, mTableStreamSize));
		return false;
	}

	// init STSHI to zeros before reading in case this version doesn't have all fields
	memset(&(mHeader.cstd), 0, sizeof(STSHI));
	mTableReader->ReadBytes((char *)&(mHeader.cstd), min((ulong)bytesSTSHI, sizeof(STSHI)));

#ifdef DEBUG	
	fprintf(stderr, "bytesSTSHI = %d, sizeof(STSHI) = %d, mHeader = {\n", bytesSTSHI, sizeof(STSHI));
	
	for (int i = 0; i < (int) min((ulong)bytesSTSHI, sizeof(STSHI)); i++)
	{
		if (!(i%16))
			fprintf(stderr, "\n");
		fprintf(stderr, "%#2.2hx, ", ((char*) &(mHeader.cstd))[i]);
	}
	fprintf(stderr, "}\n");
#endif // DEBUG
	
	// need to do swapping since we read bytes
	mHeader.cstd = B_LENDIAN_TO_HOST_INT16(mHeader.cstd);
	mHeader.cbSTDBaseInFile = B_LENDIAN_TO_HOST_INT16(mHeader.cbSTDBaseInFile);
	mHeader.bitField1 = B_LENDIAN_TO_HOST_INT16(mHeader.bitField1);
	mHeader.stiMaxWhenSaved = B_LENDIAN_TO_HOST_INT16(mHeader.stiMaxWhenSaved);
	mHeader.istdMaxFixedWhenSaved = B_LENDIAN_TO_HOST_INT16(mHeader.istdMaxFixedWhenSaved);
	mHeader.nVerBuiltInNamesWhenSaved = B_LENDIAN_TO_HOST_INT16(mHeader.nVerBuiltInNamesWhenSaved);	
	for (long i = 0; i < 3; i++)
		mHeader.rgftcStandardChpStsh[i] = B_LENDIAN_TO_HOST_INT16(mHeader.rgftcStandardChpStsh[i]);
		
	return true;
}		


// Have each style in the table build a new style message and send
// them to the output stream
void TStyleSheetWORD::SendStyles(TTranslatorDoc *outTransDoc)
{
	if (!mTableSTD)
	{
		IFDEBUG(fprintf(stderr, "WORD::SendStyles: no table\n"));
		return;
	}
	
	TStyleWORD *	std;
		
	for (long i = 0; i < mTableSTD->CountItems(); i++)
	{
		std = (TStyleWORD *)(mTableSTD->ItemAt(i));
		// it's valid to have an empty style as a placeholder
		if (std)
			SendStyle(std, outTransDoc);
	}
}

void TStyleSheetWORD::SendStyle(TStyleWORD* theStyle, TTranslatorDoc *outTransDoc)
{
	ASSERT(theStyle, "style is null");
	
	if (theStyle->GetSent())
		return;
	
	ushort baseStyleIndex = theStyle->GetBaseStyleIndex();
	TStyleWORD * baseStyle = StyleFromISTD(baseStyleIndex);
	if (baseStyle)
		SendStyle(baseStyle, outTransDoc);

	TFilterStyleWORD * filterStyle = theStyle->GetFilterStyle();
	
	if (filterStyle)
		outTransDoc->StylesTable()->AddStyle( filterStyle );
	
	theStyle->SetSent(true);
}



int32 TStyleSheetWORD::GetStyleIndex(const char* name)
{
	for (int i = 0; i < mTableSTD->CountItems(); i++)
	{
		TStyleWORD* theStyle = (TStyleWORD*) mTableSTD->ItemAt(i);
		if (theStyle->IsNull())
			continue;
		if (!strcmp(name, theStyle->StyleName()))
			return i;
	}
	return -1;
}


void TStyleSheetWORD::AddStyleTable(TTranslatorStylesTable* styleTable, TFontTblWORD* fontTbl) 
{ 
	int32 countItems = styleTable->CountItems();	
	TStyleWORD** styleArray = new TStyleWORD*[countItems]; 
	TTranslatorStyle* currentTStyle;         
	printf("%d\n", countItems); 
		 
	for(int i = 0; i < countItems; i++) 
	{ 
		currentTStyle = (*styleTable)[i]; 
		styleArray[i] = new TStyleWORD(GetSize()); 
		styleArray[i]->SetStyleID(4094); 
		styleArray[i]->SetHasUPE(1); 
		
		TTranslatorStyle::StyleType styleType = TTranslatorStyle::kBasicStyle; 
		currentTStyle->GetStyleType(styleType); 
		
		if (styleType == TTranslatorStyle::kParagraphStyle) 
		{ 
			styleArray[i]->SetStyleTypeCode(1); 
			styleArray[i]->SetCUPX(2); 
			styleArray[i]->SetBaseStyleIndex(0);                    

//			int styleSprmsSize = GetSPRMListSize(currentTStyle); 
//			char *styleSprms = new char[styleSprmsSize]; 
//			GetSPRMList(currentTStyle, styleSprms, fontTbl, this); 
//			styleArray[i]->SetUPXChar(styleSprms, styleSprmsSize); 
//			delete [] styleSprms; 			
//			styleArray[i]->SetUPXPara(0, 0); 

			// Make sure to put the char sprms in the Char UPX
			int32 charSprmsSize = GetCharSPRMListSize(currentTStyle);
			char *styleSprms = new char[charSprmsSize]; 
			GetCharSPRMList(currentTStyle, styleSprms, fontTbl, this);
			styleArray[i]->SetUPXChar(styleSprms, charSprmsSize);
			delete [] styleSprms;
			
			// Make sure to put the para sprms in the Para UPX
			int32 paraSprmsSize = GetParaSPRMListSize(currentTStyle);
			styleSprms = new char[paraSprmsSize];
			GetParaSPRMList(currentTStyle, styleSprms, fontTbl, this);
			styleArray[i]->SetUPXPara(styleSprms, paraSprmsSize);
			delete [] styleSprms;
		} 
		else if (styleType == TTranslatorStyle::kBasicStyle) 
		{ 
			styleArray[i]->SetStyleTypeCode(2); 
			styleArray[i]->SetCUPX(1); 
			styleArray[i]->SetBaseStyleIndex(10);                   
			
			int styleSprmsSize = GetCharSPRMListSize(currentTStyle); 
			char *styleSprms = new char[styleSprmsSize]; 
			GetCharSPRMList(currentTStyle, styleSprms, fontTbl, this); 
			
			styleArray[i]->SetUPXChar(styleSprms, styleSprmsSize); 
			delete [] styleSprms; 
		} 
		else 
		{ 
			delete styleArray[i]; 
			styleArray[i] = 0; 
			continue; 
		} 
		
		styleArray[i]->SetStyleIndexNext(GetSize()); 
		const char* theStyleName; 
		
		// the table must already have these styles so we don't want to add them again here 
		if (!currentTStyle->GetStyleName(&theStyleName) || strlen(theStyleName) == 0 || 
			!strcmp(theStyleName, "Normal") || !strcmp(theStyleName, "Default Paragraph Font")) 
		{ 
			delete styleArray[i]; 
			styleArray[i] = 0; 
			continue; 
		} 
		styleArray[i]->SetStyleName(theStyleName); 		
		AddStyle(styleArray[i]); 
	}
	 
	// link the styles together 
	for(int i = 0; i < countItems; i++) 
	{ 
		const char* name; 
		if (styleArray[i]) 
		{ 
			//styleArray[i]->SetBaseStyleIndex(4095); 
			if(!(*styleTable)[i]->GetBaseStyleName(&name)) 
				continue; 
			
			ushort istd = GetStyleIndex(name); 
			if (istd > 0) 
				styleArray[i]->SetBaseStyleIndex(istd); 
		} 
	} 
	delete [] styleArray; 
} 

TStyleWORD* TStyleSheetWORD::StyleFromISTD(ushort istd)
{
	TStyleWORD *	std = 0;
	
	// make sure we've got a style with this istd
	if (istd < mTableSTD->CountItems())
		std = (TStyleWORD *)(mTableSTD->ItemAt(istd));
		
	if (!std)
	{
		IFDEBUG(fprintf(stderr, "WORD NameFromISTD: no STD for istd %d\n", istd));
		return NULL;
	}
	
	return std;
}


const char * TStyleSheetWORD::NameFromISTD(ushort istd)
{
	TStyleWORD *	std = 0;
	
	// make sure we've got a style with this istd
	if (istd < mTableSTD->CountItems())
		std = (TStyleWORD *)(mTableSTD->ItemAt(istd));
		
	if (!std)
	{
		IFDEBUG(fprintf(stderr, "WORD NameFromISTD: no STD for istd %d\n", istd));
		return NULL;
	}
	
	return std->StyleName();
}


// Returns a filterStyle whose parent is the STD for this istd
TFilterStyleWORD * TStyleSheetWORD::NewFilterStylePara(ushort istd)
{
	TStyleWORD *	std = 0;
	
	// make sure we've got a style with this istd
	if (istd < mTableSTD->CountItems())
		std = (TStyleWORD *)(mTableSTD->ItemAt(istd));
		
	if (!std)
	{
		IFDEBUG(fprintf(stderr, "WORD NewFilterStylePara: no STD for istd %d\n", istd));
		return NULL;
	}
	
	TFilterStyleWORD *filterStyle = new TFilterStyleWORD();
	filterStyle->SetApplyStyleName(std->StyleName());
	filterStyle->SetStyleType(TTranslatorStyle::kParagraphStyle);
	//filterStyle->SetBaseStyleName("Body");
	
	return filterStyle;
}


int32 TStyleSheetWORD::Write(TOLEEntryWriter *tableWriter, long &offset, ulong &offsetBytes)
{
	offset = tableWriter->GetStreamPos();
	
	tableWriter->WriteShort(sizeof(STSHI));
	
	//char headerData[] = // fill in the header with stuff I greabed out of a default file
	//		{0x00, 0x0f, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x5b, 0x00, 0x0f, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
	//		0x00, 0x00};
	char headerData[] = // fill in the header with stuff I greabed out of a default file
			{0x0f, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x5b, 0x00, 0x0f, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00};
	memcpy((char*) (&mHeader.cstd), headerData, sizeof(headerData));
	
	mHeader.cstd = mTableSTD->CountItems();
	// need to do swapping since we write bytes
	mHeader.cstd = B_HOST_TO_LENDIAN_INT16(mHeader.cstd);
	//mHeader.cbSTDBaseInFile = B_HOST_TO_LENDIAN_INT16(mHeader.cbSTDBaseInFile);
	//mHeader.bitField1 = B_HOST_TO_LENDIAN_INT16(mHeader.bitField1);
	//mHeader.stiMaxWhenSaved = B_HOST_TO_LENDIAN_INT16(mHeader.stiMaxWhenSaved);
	//mHeader.istdMaxFixedWhenSaved = B_HOST_TO_LENDIAN_INT16(mHeader.istdMaxFixedWhenSaved);
	//mHeader.nVerBuiltInNamesWhenSaved = B_HOST_TO_LENDIAN_INT16(mHeader.nVerBuiltInNamesWhenSaved);	
	//for (long i = 0; i < 3; i++)
	//	mHeader.rgftcStandardChpStsh[i] = B_HOST_TO_LENDIAN_INT16(mHeader.rgftcStandardChpStsh[i]);

	tableWriter->WriteBytes((char *)&(mHeader.cstd), sizeof(STSHI));
	
	//printf("the STSHI finished at %d", tableWriter->GetStreamPos());
	TStyleWORD *	std;
	
	for (short i = 0; i < mTableSTD->CountItems(); i++)
	{
		std = (TStyleWORD *)(mTableSTD->ItemAt(i));
		
		IFDEBUG(fprintf(stderr, "starting to write out STD %d at location %d\n", i, tableWriter->GetStreamPos()));
		std->SetBchUPE(std->GetBytes());
		tableWriter->WriteShort(std->GetBchUPE());
		std->Write(tableWriter);
		IFDEBUG(fprintf(stderr, "finished to write out STD %d at location %d\n", i, tableWriter->GetStreamPos()));
	}
	
	offsetBytes = tableWriter->GetStreamPos() - offset;
	return B_NO_ERROR;
}


//---------------------------------------------------------------------------------
//
// UTILITY FUNCTIONS AND STUFF
//
//---------------------------------------------------------------------------------


// Built-in Style Names Table
BuiltInStyleNames BuiltInStyleNamesTable[stiMax] =
{
	{"Normal"},						// 0
	{"Heading 1"},
	{"Heading 2"},
	{"Heading 3"},
	{"Heading 4"},
	{"Heading 5"},
	{"Heading 6"},
	{"Heading 7"},
	{"Heading 8"},
	{"Heading 9"},
	{"Index 1"},					// 10
	{"Index 2"},
	{"Index 3"},
	{"Index 4"},
	{"Index 5"},
	{"Index 6"},
	{"Index 7"},
	{"Index 8"},
	{"Index 9"},
	{"TOC 1"},
	{"TOC 2"},						// 20
	{"TOC 3"},
	{"TOC 4"},
	{"TOC 5"},
	{"TOC 6"},
	{"TOC 7"},
	{"TOC 8"},
	{"TOC 9"},
	{"Normal Indent"},
	{"Footnote Text"},
	{"Annotation Text"},			// 30
	{"MSHeader"},	// renamed these two so they don't conflict with Squirrel style names
	{"MSFooter"},
	{"Index Heading"},
	{"Caption"},
	{"Table of Figures"},
	{"Envelope Address"},
	{"Envelope Return"},
	{"Footnote Reference"},
	{"Annotation Reference"},
	{"Line Number"},				// 40
	{"Page Number"},
	{"Endnote Reference"},
	{"Endnote Text"},
	{"Table of Authorities"},
	{"Macro Text"},
	{"TOA Heading"},
	{"List"},
	{"List Bullet"},
	{"List Number"},
	{"List 2"},						// 50
	{"List 3"},
	{"List 4"},		
	{"List 5"},
	{"List Bullet 2"},
	{"List Bullet 3"},
	{"List Bullet 4"},
	{"List Bullet 5"},
	{"List Number 2"},
	{"List Number 3"},
	{"List Number 4"},				// 60
	{"List Number 5"},
	{"Title"},
	{"Closing"},
	{"Signature"},
	{"Default Paragraph Font"},
	{"Body Text"},
	{"Body Text Indent"},
	{"List Continue"},
	{"List Continue 2"},
	{"List Continue 3"},			// 70
	{"List Continue 4"},
	{"List Continue 5"},
	{"Message Header"},
	{"Subtitle"},
	{"Salutation"},
	{"Date"},
	{"Body Text First Indent"},
	{"Body Text First Indent 2"},
	{"Note Heading"},
	{"Body Text 2"},				// 80
	{"Body Text 3"},
	{"Body Text Indent 2"},
	{"Body Text Indent 3"},
	{"Block Text"},
	{"Hyperlink"},
	{"Followed Hyperlink"},
	{"Strong"},
	{"Emphasis"},
	{"Document Map"},
	{"Plain Text"}					// 90
};


// Table of sprms used from with the PRM
ushort rgsprmPrm[128] =
{
	sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmPIncLvl, sprmPJc, sprmPFSideBySide, 
	sprmPFKeep, sprmPFKeepFollow, sprmPFPageBreakBefore, sprmPBrcl, sprmPBrcp, sprmPIlvl, 
	sprmNoop, sprmPFNoLineNumb, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, 
	sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmPFInTable, sprmPFTtp, sprmNoop, 
	sprmNoop, sprmNoop, sprmPPc, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, 
	sprmNoop, sprmNoop, sprmPWr, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, 
	sprmNoop, sprmPFNoAutoHyph, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, 
	sprmPFLocked, sprmPFWidowControl, sprmNoop, sprmPFKinsoku, sprmPFWordWrap, 
	sprmPFOverflowPunct, sprmPFTopLinePunct, sprmPFAutoSpaceDE, sprmPFAutoSpaceDN, 
	sprmNoop, sprmNoop, sprmPISnapBaseLine, sprmNoop, sprmNoop, sprmNoop, 
	sprmCFStrikeRM, sprmCFRMark, sprmCFFldVanish, sprmNoop, sprmNoop, sprmNoop, 
	sprmCFData, sprmNoop, sprmNoop, sprmNoop, sprmCFOle2, sprmNoop, sprmCHighlight, 
	sprmCFEmboss, sprmCSfxText, sprmNoop, sprmNoop, sprmNoop, sprmCPlain, sprmNoop, 
	sprmCFBold, sprmCFItalic, sprmCFStrike, sprmCFOutline, sprmCFShadow, sprmCFSmallCaps, 
	sprmCFCaps, sprmCFVanish, sprmNoop, sprmCKul, sprmNoop, sprmNoop, sprmNoop, 
	sprmCIco, sprmNoop, sprmCHpsInc, sprmNoop, sprmCHpsPosAdj, sprmNoop, sprmCIss, 
	sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, 
	sprmNoop, sprmNoop, sprmCFDStrike, sprmCFImprint, sprmCFSpec, sprmCFObj, sprmPicBrcl, 
	sprmPOutLvl, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop, sprmNoop 
};

uint16 SprmOpCode6to8(uint8 opcode6)
{
	//if (opcode6 >= 187)
	//	DEBUGSTR("Complex sprm I should be handling, but I am not yet"); // -BOGUS
		
	switch (opcode6)
	{
		case 2: return sprmPIstd;
		case 3: return sprmPIstdPermute;
		case 4: return sprmPIncLv1;
		case 5: return sprmPJc;
		case 6: return sprmPFSideBySide;
		case 7: return sprmPFKeep;
		case 8: return sprmPFKeepFollow;
		case 9: return sprmPPageBreakBefore;
		case 10: return sprmPBrcl;
		case 11: return sprmPBrcp;
		case 12: return sprmPAnld;
		case 13: return sprmPNLvlAnm;
		case 14: return sprmPFNoLineNumb;
		case 15: return sprmPChgTabsPapx;
		case 16: return sprmPDxaRight;
		case 17: return sprmPDxaLeft;
		case 18: return sprmPNest;
		case 19: return sprmPDxaLeft1;
		case 20: return sprmPDyaLine;
		case 21: return sprmPDyaBefore;
		case 22: return sprmPDyaAfter;
		case 23: return sprmPChgTabs;
		case 24: return sprmPFInTable;
		case 25: return sprmPTtp;
		case 26: return sprmPDxaAbs;
		case 27: return sprmPDyaAbs;
		case 28: return sprmPDxaWidth;
		case 29: return sprmPPc;
		case 30: return sprmPBrcTop10;
		case 31: return sprmPBrcLeft10;
		case 32: return sprmPBrcBottom10;
		case 33: return sprmPBrcRight10;
		case 34: return sprmPBrcBetween10;
		case 35: return sprmPBrcBar10;
		case 36: return sprmPFromText10;
		case 37: return sprmPWr;
		case 38: return sprmPBrcTop;
		case 39: return sprmPBrcLeft;
		case 40: return sprmPBrcBottom;
		case 41: return sprmPBrcRight;
		case 42: return sprmPBrcBetween;
		case 43: return sprmPBrcBar;
		case 44: return sprmPFNoAutoHyph;
		case 45: return sprmPWHeightAbs;
		case 46: return sprmPDcs;
		case 47: return sprmPShd;
		case 48: return sprmPDyaFromText;
		case 49: return sprmPDxaFromText;
		case 50: return sprmPFLocked;
		case 51: return sprmPFWidowControl;
		case 52: return sprmPRuler;
		case 65: return sprmCFStrikeRM;
		case 66: return sprmCFRMark;
		case 67: return sprmCFFldVanish;
		case 68: return sprmCPicLocation;
		case 69: return sprmCIbstRMark;
		case 70: return sprmCDttmRMark;
		case 71: return sprmCFData;
		case 72: return sprmCRMReason;
		case 73: return sprmCChse;
		case 74: return sprmCSymbol;
		case 75: return sprmCFOle2;
		case 80: return sprmCIstd;
		case 81: return sprmCIstdPermute;
		case 82: return sprmCDefault;
		case 83: return sprmCPlain;
		case 85: return sprmCFBold;
		case 86: return sprmCFItalic;
		case 87: return sprmCFStrike;
		case 88: return sprmCFOutline;
		case 89: return sprmCFShadow;
		case 90: return sprmCFSmallCaps;
		case 91: return sprmCFCaps;
		case 92: return sprmCFVanish;
		case 93: return sprmCFtc;
		case 94: return sprmCKul;
		case 95: return sprmCSizePos;
		case 96: return sprmCDxaSpace;
		case 97: return sprmCLid;
		case 98: return sprmCIco;
		case 99: return sprmCHps;
		case 100: return sprmCHpsInc;
		case 101: return sprmCHpsPos;
		case 102: return sprmCHpsPosAdj;
		case 103: return sprmCMajority;
		case 104: return sprmCIss;
		case 105: return sprmCHpsNew50;
		case 106: return sprmCHpsInc1;
		case 107: return sprmCHpsKern;
		case 108: return sprmCMajority50;
		case 109: return sprmCHpsMul;
		case 110: return sprmCCondHyhen;
		case 117: return sprmCFSpec;
		case 118: return sprmCFObj;
		case 119: return sprmPicBrcl;
		case 120: return sprmPicScale;
		case 121: return sprmPicBrcTop;
		case 122: return sprmPicBrcLeft;
		case 123: return sprmPicBrcBottom;
		case 124: return sprmPicBrcRight;
		case 131: return sprmSScnsPgn;
		case 132: return sprmSiHeadingPgn;
		case 133: return sprmSOlstAnm;
		case 136: return sprmSDxaColWidth;
		case 137: return sprmSDxaColSpacing;
		case 138: return sprmSFEvenlySpaced;
		case 139: return sprmSFProtected;
		case 140: return sprmSDmBinFirst;
		case 141: return sprmSDmBinOther;
		case 142: return sprmSBkc;
		case 143: return sprmSFTitlePage;
		case 144: return sprmSCcolumns;
		case 145: return sprmSDxaColumns;
		case 146: return sprmSFAutoPgn;
		case 147: return sprmSNfcPgn;
		case 148: return sprmSDyaPgn;
		case 149: return sprmSDxaPgn;
		case 150: return sprmSFPgnRestart;
		case 151: return sprmSFEndnote;
		case 152: return sprmSLnc;
		case 153: return sprmSGprfIhdt;
		case 154: return sprmSNLnnMod;
		case 155: return sprmSDxaLnn;
		case 156: return sprmSDyaHdrTop;
		case 157: return sprmSDyaHdrBottom;
		case 158: return sprmSLBetween;
		case 159: return sprmSVjc;
		case 160: return sprmSLnnMin;
		case 161: return sprmSPgnStart;
		case 162: return sprmSBOrientation;
		case 163: return sprmSBCustomize;
		case 164: return sprmSXaPage;
		case 165: return sprmSYaPage;
		case 166: return sprmSDxaLeft;
		case 167: return sprmSDxaRight;
		case 168: return sprmSDyaTop;
		case 169: return sprmSDyaBottom;
		case 170: return sprmSDzaGutter;
		case 171: return sprmSDMPaperReq;
		case 182: return sprmTJc;
		case 183: return sprmTDxaLeft;
		case 184: return sprmTDxaGapHalf;
		case 185: return sprmTFCantSplit;
		case 186: return sprmTTableHeader;
		case 187: return sprmTTableBorder;
		case 188: return sprmTDefTable10;
		case 189: return sprmTDyaRowHeight;
		case 190: return sprmTDefTable;
		case 191: return sprmTDefTableShd;
		case 192: return sprmTTlp;
		case 193: return sprmTSetBrc;
		case 194: return sprmTInsert;
		case 195: return sprmTDelete;
		case 196: return sprmTDxaCol;
		case 197: return sprmTMerge;
		case 198: return sprmTSplit;
		case 199: return sprmTSetBrc10;
		case 200: return sprmTSetShd;
		//case 208: return sprmMax;
		
		default:
			DEBUGSTR("SprmOpCode6to8(): unknown SPRM");
	}
	return -1;
}

// Utility to read a sprm.  It works in two modes: one to read from a stream reader
// into a pre-allocated buffer, and the other where the sprm is already in the buffer,
// and you're just trying to get the sprmID and the size (pass in 0 for reader).
// Both return the sprm bytes and this sprmID. 
short ReadSPRM(TOLEEntryReader *reader, char *buffer, ushort& theSprm, int16 nFib)
{
	short	parmSize = 0, bytesRead = 0;
	char*	sprmStart = buffer;
	bool	variableLength = false;
	uchar	origSprm = 0xFF;
	// sprm
	if (nFib > 190)
	{
		if (reader)
		{
			theSprm = reader->ReadShort();
			memmove(buffer, (char *)&theSprm, 2);
			//printf("theSprm = %#hx\n", theSprm);
		}
		else
			memmove((char *)&theSprm, buffer, 2);
		bytesRead += 2;
		buffer += 2;
	}
	else // word 6
	{
		if (reader)
		{
			theSprm = reader->ReadChar();
			origSprm = theSprm;
			*buffer = (uchar) theSprm;
			theSprm = SprmOpCode6to8(theSprm);
			//if (theSprm == 0x4000)
			//	DEBUGGER();
			//printf("theSprm = %#hx\n", theSprm);
		}
		else
		{
			origSprm = *buffer;
			theSprm = SprmOpCode6to8(*buffer);
			//if (theSprm == 0x4000)
			//	DEBUGGER();

		}
		bytesRead += 1;
		buffer += 1;
	}
	
	if (reader && reader->Error() < 0)
		return B_ERROR;

	
	// The sprm itself defines how many bytes follow it
	if ((theSprm & sprm_spra) == sprm_0_1 || (theSprm & sprm_spra) == sprm_1_1)
		parmSize = 1;
	else if ((theSprm & sprm_spra) == sprm_2_2 || (theSprm & sprm_spra) == sprm_4_2 || (theSprm & sprm_spra) == sprm_5_2)
		parmSize = 2;
	else if ((theSprm & sprm_spra) == sprm_7_3)
		parmSize = 3;
	else if ((theSprm & sprm_spra) == sprm_3_4)
	{
	//	if (nFib > 190)
			parmSize = 4;
	//	else // word 6
	//		variableLength = true;
	}
	else if ((theSprm & sprm_spra) == sprm_6_v)
		variableLength = true;
	else
	{
		IFDEBUG(fprintf(stderr, "WORD ReadSPRM: unknown sprm parm size\n"));
		return bytesRead;
	}
	
	//if (nFib <= 190 && theSprm == sprmCHps) // this sprm has only a byte as it's parameter in Word 6
	//	parmSize = 1;
	
	if (nFib <= 190) // sprms that wont get converted using the above crap
	{
		if (origSprm == 187) // sprmTTableBorder
		{
			parmSize = 12;
			variableLength = false;			
		}
		else if (theSprm == sprmTTlp)
			parmSize = 4;
		else if (theSprm == sprmTSetBrc)
			parmSize = 5;
		else if (theSprm == sprmTInsert)
			parmSize = 4;
		else if (theSprm == sprmTDxaCol)
			parmSize = 4;
		else if (theSprm == sprmTSetBrc10)
			parmSize = 5;
		else if (theSprm == sprmTSetShd)
			parmSize = 4;
		else if (theSprm == sprmPBrcTop ||
				 theSprm == sprmPBrcLeft ||
				 theSprm == sprmPBrcBottom ||
				 theSprm == sprmPBrcRight ||
				 theSprm == sprmPBrcBetween ||
				 theSprm == sprmPBrcBar ||
				 theSprm == sprmPicBrcTop ||
				 theSprm == sprmPicBrcLeft ||
				 theSprm == sprmPicBrcBottom ||
				 theSprm == sprmPicBrcRight) // these sprms have only a short in them, not a long like in word 8
			parmSize = 2;
		else if (theSprm == sprmCPicLocation)
			variableLength = true;

		//else if (origSprm == 208)
		//	DEBUGSTR("what the hell is this sprm!!");
			
	}

	if (variableLength)
	{
		// VARIABLE - usually the next byte is the size.
		// However, three sprms can be more than 256 bytes, so they're handled as exceptions.
		// Here's the first two
		if (theSprm == sprmTDefTable10 || theSprm == sprmTDefTable) // maybe sprmTDefTableShd should be here too
		{
			// next TWO bytes are the size + 1
			if (reader)
			{
				parmSize = reader->ReadShort();
				if (reader->Error() < 0)
					return B_ERROR;

				memmove(buffer, (char *)&parmSize, 2);
			}
			else
				memmove((char *)&parmSize, buffer, 2);
			
			bytesRead += 2;
			buffer += 2;
			// In it's wisdom, MS made this param + 1, so adjust here
			parmSize--;
		}
		else
		{
			// normal VARIABLE case (maybe, see below)
			if (reader)
				*buffer = reader->ReadChar();
			
			if (reader && reader->Error() < 0)
				return B_ERROR;
			
			parmSize = (uchar) *buffer;
			bytesRead++;
			// buffer++ is done below so we can check FF exception
			
			// Here's the third exception
			if (theSprm == sprmPChgTabs && *buffer == (char) 0xFF) // I don't get this code... It doesn't look like sprmPChgTabs
			{												// is an exeptional sprm, *buffer should be equal to the count of bytes.
															// I don't see why it would be equal to 0xff. maybe carl knew what
															// he was doing... so I'll leave this in for now - Joel
				// we'll read the rest of the sprm here as it contains the real parmSize
				buffer++;				
				char maxValue;
				
				// delete tabs
				if (reader)
					*buffer = reader->ReadChar();
				maxValue = *buffer;
				buffer++;
				bytesRead++;
				
				if (reader)
					reader->ReadBytes(buffer, (4 * maxValue));
				buffer += (4 * maxValue);
				bytesRead += (4 * maxValue);
				
				// add tabs
				if (reader)
					maxValue = *buffer = reader->ReadChar();
				buffer++;
				bytesRead++;
				
				if (reader)
					reader->ReadBytes(buffer, (3 * maxValue));
				buffer += (3 * maxValue);
				bytesRead += (3 * maxValue);
				
				return bytesRead;				
			}
			
			// Normal VARIABLE case
			buffer++;
		}
	}
	
	// Now read the rest of the sprm.			
	// WARNING - nothing after the sprm is swapped (unless parmSize == 2), we just read the bytes
	if (reader)
	{
		if (parmSize == 2)
		{
			ushort tempShort = reader->ReadShort();	// usually a word, so swaps it
			memmove(buffer, (char *)&tempShort, 2);
		}
		else
			reader->ReadBytes(buffer, parmSize);
	}
	
	if (reader && reader->Error() < 0)
		return B_ERROR;

	bytesRead += parmSize;	
	
	return bytesRead;				
}


// Apply this run of sprms to the filterStyle.
void ApplyAllSprms(char *sprmBuffer, ushort sprmRunBytes, TFilterStyleWORD *filterStyle, long styleType, TFontTblWORD *fontTblWORD, TStyleSheetWORD* styleTblWORD, int16 nFib)
{
	if (!filterStyle)
	{
		IFDEBUG(fprintf(stderr, "WORD ApplyAllSprms: no filterStyle\n"));
		return;
	}
	
	if (!sprmBuffer || !sprmRunBytes)
		return;
			
	char *	endPtr = sprmBuffer + sprmRunBytes;
	ushort	sprmBytes, theSprm;
	
	while (sprmBuffer < endPtr)
	{
	
		sprmBytes = ReadSPRM(0, sprmBuffer, theSprm, nFib);			
		ApplySprm(sprmBuffer, sprmBytes, theSprm, filterStyle, styleType, fontTblWORD, styleTblWORD, nFib);			
		sprmBuffer += sprmBytes;
	}
	PostProccessStyle(filterStyle);
}


// Apply this sprm to the filterStyle.  Only need to apply the ones that match the style
// type (either S_PARA or S_CHAR).
void ApplySprm(char *sprmBuffer, ushort sprmBytes, ushort theSprm, TFilterStyleWORD *filterStyle, long styleType, TFontTblWORD *fontTblWORD, TStyleSheetWORD* styleTblWORD, int16 nFib)
{	
	if (nFib <= 190) //word 6
		sprmBuffer--;
	
	uchar	firstByte = sprmBuffer[2];
	short	tempShort;
	short	tempShort2;
	long	tempLong; 
		
	switch (theSprm)
	{
		// for sprmCFBold - sprmCFVanish, firstByte is 0 for off, 1 for on,
		// 0x80 means set to value of style sheet, 0x81 means opposite of style sheet
		// BOGUS - style architecture only allows us to know what the current value of this FilterStyle,
		// but doesn't tell us about anything that it's based on.  For now, just treat 0x81 as 0x01.
		case sprmCFBold:
			if (firstByte == 0x00 || firstByte == 0x01 || firstByte == 0x81)
				filterStyle->SetTextFace(TTranslatorStyle::kBoldFace, (firstByte == 0x00) ? false : true);
			break;
			
		case sprmCFItalic:
			if (firstByte == 0x00 || firstByte == 0x01 || firstByte == 0x81)
				filterStyle->SetTextFace(TTranslatorStyle::kItalicFace, (firstByte == 0x00) ? false : true);
			break;
			
		case sprmCFStrike:
		case sprmCFDStrike:
			if (firstByte == 0x00 || firstByte == 0x01 || firstByte == 0x81)
				filterStyle->SetTextFace(TTranslatorStyle::kStrikeThruFace, (firstByte == 0x00) ? false : true);
			break;
		
		case sprmCFImprint:
		case sprmCFEmboss:
			filterStyle->SetEmbossedImprint(firstByte || filterStyle->IsEmbossedImprint());
			break;

		case sprmCFSpec:
			filterStyle->SetSpecialChar((firstByte == 0x01) ? true : false);
			break;
			
		case sprmCRgFtc0:			// font name (ftc) 0 - ascii
			memmove(&tempShort, &sprmBuffer[2], 2);
			//IFDEBUG(fprintf(stderr, "WORD Apply sprmCRgFtc0: text ftc0 %d\n", tempShort));
			//filterStyle->SetTextFont(fontTblWORD->ConvertFTCtoBeFontID(tempShort));
			filterStyle->SetTextFont(fontTblWORD->ConvertFTCtoName(tempShort), "");
			break;
			
		case sprmCRgFtc1:			// font name (ftc) 1 - far east.  How do I know when to apply this one?
			memmove(&tempShort, &sprmBuffer[2], 2);
			//IFDEBUG(fprintf(stderr, "WORD Apply sprmCRgFtc1: text ftc1 %d\n", tempShort));
			//filterStyle->SetTextFont(fontTblWORD->ConvertFTCtoBeFontID(tempShort));
			break;
			
		case sprmCRgFtc2:			// font name (ftc) 2 - non-far east.
			memmove(&tempShort, &sprmBuffer[2], 2);
			//IFDEBUG(fprintf(stderr, "WORD Apply sprmCRgFtc1: text ftc1 %d\n", tempShort));
			//filterStyle->SetTextFont(fontTblWORD->ConvertFTCtoBeFontID(tempShort));
			break;
			
		case sprmCHps:			// font size
			//if (nFib > 190) // this sprm has only a byte as it's parameter in Word 6
			//{
				memmove(&tempShort, &sprmBuffer[2], 2);
				//IFDEBUG(fprintf(stderr, "WORD ApplySprm: text size %d\n", (tempShort / 2)));
				filterStyle->SetTextSize(tempShort / 2);
			//}
			//else
			//	filterStyle->SetTextSize(firstByte);
			break;
			
		case sprmCKul:			// underline
			if (firstByte == 0x00)
			{
				filterStyle->SetTextFace(TTranslatorStyle::kUnderlineFace, false);
				filterStyle->SetTextFace(TTranslatorStyle::kDoubleUnderlineFace, false);
			}
			filterStyle->SetTextFace((firstByte == 0x03) ? TTranslatorStyle::kDoubleUnderlineFace : TTranslatorStyle::kUnderlineFace, true);
			break;
			
		case sprmCIco:			// text color
		{
			rgb_color	rgbColor;
			rgbColor.alpha = 255;
			
			switch (firstByte)
			{
				case 0x02:	// blue
					rgbColor.blue = 255;
					rgbColor.red = rgbColor.green = 0;
					break;

				case 0x03:	// cyan
					rgbColor.red = 0;
					rgbColor.green = rgbColor.blue = 255;
					break;
				
				case 0x04:	// green
					rgbColor.green = 255;
					rgbColor.red = rgbColor.blue = 0;
					break;

				case 0x05:	// magenta
					rgbColor.green = 0;
					rgbColor.red = rgbColor.blue = 255;
					break;
				
				case 0x06:	// red
					rgbColor.red = 255;
					rgbColor.green = rgbColor.blue = 0;
					break;
					
				case 0x07:	// yellow
					rgbColor.blue = 0;
					rgbColor.red = rgbColor.green = 255;
					break;
				
				case 0x08:	// white
					rgbColor.red = rgbColor.green = rgbColor.blue = 255;
					break;
										
				case 0x09:	// dk blue
					rgbColor.blue = 128;
					rgbColor.red = rgbColor.green = 0;
					break;
				
				case 0x0A:	// dk cyan
					rgbColor.red = 0;
					rgbColor.green = rgbColor.blue = 128;
					break;
				
				case 0x0B:	// dk green
					rgbColor.green = 128;
					rgbColor.red = rgbColor.blue = 0;
					break;
				
				case 0x0C:	// dk magenta
					rgbColor.green = 0;
					rgbColor.red = rgbColor.blue = 128;
					break;
				
				case 0x0D:	// dk red
					rgbColor.red = 128;
					rgbColor.green = rgbColor.blue = 0;
					break;
				
				case 0x0E:	// dk yellow
					rgbColor.blue = 0;
					rgbColor.red = rgbColor.green = 128;
					break;
				
				case 0x0F:	// dk gray
					rgbColor.red = rgbColor.green = rgbColor.blue = 128;
					break;
				
				case 0x10:	// lt gray
					rgbColor.red = rgbColor.green = rgbColor.blue = 192;
					break;
					
				case 0x00:	// auto
				case 0x01:	// black
				default:
					rgbColor.red = rgbColor.green = rgbColor.blue = 0;
					break;					
			}
			
			filterStyle->SetTextColor(rgbColor, rgbColor);
			break;
		}
			
		case sprmCIss:			// sub,super script
			if (firstByte == 0x00)
			{
				filterStyle->SetTextFace(TTranslatorStyle::kSuperScriptFace, false);
				filterStyle->SetTextFace(TTranslatorStyle::kSubScriptFace, false);
			}
			else
				filterStyle->SetTextFace((firstByte == 0x01) ? TTranslatorStyle::kSuperScriptFace : TTranslatorStyle::kSubScriptFace, true);
			break;
		
		case sprmCHpsPos:
			if ((char) firstByte > 0)
				filterStyle->SetTextFace(TTranslatorStyle::kSuperScriptFace, true);
			else if ((char) firstByte < 0)
				filterStyle->SetTextFace(TTranslatorStyle::kSubScriptFace, true);
			
			break;
		
		case sprmPDxaLeft1:		// para first line indent (relative to dxaLeft)
		{
			memmove(&tempShort, &sprmBuffer[2], 2);
			float leftIndent = 0;
			filterStyle->GetLeftIndent(leftIndent);
			filterStyle->SetFirstLineIndent(leftIndent + (tempShort / TWIPS_PER_POINT));			
			break;
		}
			
		case sprmPDxaLeft:		// para indent left
			memmove(&tempShort, &sprmBuffer[2], 2);
			//IFDEBUG(fprintf(stderr, "WORD Apply sprmPDxaLeft: indent left (word style) %d\n", tempShort));
			filterStyle->SetLeftIndent(tempShort / TWIPS_PER_POINT);
			// make first line indent have the same value
			filterStyle->SetFirstLineIndent(tempShort / TWIPS_PER_POINT);
			break;
			
		case sprmPDxaRight:		// para indent right
			memmove(&tempShort, &sprmBuffer[2], 2);
			filterStyle->SetRightIndent(tempShort / TWIPS_PER_POINT);
			break;
		
		case sprmPJc:
			filterStyle->SetParagraphAlignment((firstByte == 0x01) ? TTranslatorStyle::kParAlignCenter : (firstByte == 0x02) ? TTranslatorStyle::kParAlignRight : (firstByte == 0x03) ? TTranslatorStyle::kParAlignJust : TTranslatorStyle::kParAlignLeft);
			break;
		
		case sprmPDyaBefore:	// paragraph vertical spacing before
			memmove(&tempShort, &sprmBuffer[2], 2);
			filterStyle->SetSpaceBefore(tempShort / TWIPS_PER_POINT, false);
			break;	
		
		case sprmPDyaAfter:		// paragraph vertical spacing after
			memmove(&tempShort, &sprmBuffer[2], 2);
			filterStyle->SetSpaceAfter(tempShort / TWIPS_PER_POINT, false);
			break;
		
		case sprmPDyaLine:
			tempShort = B_LENDIAN_TO_HOST_INT16( ((int16*) sprmBuffer)[1] );
			tempShort2 = B_LENDIAN_TO_HOST_INT16( ((int16*) sprmBuffer)[2] 	);
			
			if (tempShort < 0) // this means that word is using "exact" spacing
				tempShort *= -1;
			
			if (tempShort2 == 1) 	filterStyle->SetLineSpacing( ((float) tempShort) / 240, true);
			else 					filterStyle->SetLineSpacing(tempShort / TWIPS_PER_POINT, false);
			break;
				
		case sprmPChgTabsPapx:
		{
			// guaranteed ascending order of tabs
			uchar	delCount = sprmBuffer[3];
			short *	delPosPtr = (short *)&sprmBuffer[4];
			uchar	addCount = sprmBuffer[4 + (2 * delCount)];
			short *	addPosPtr = (short *)&sprmBuffer[5 + (2 * delCount)];
			uchar *	addDescPtr = (uchar *)&sprmBuffer[5 + (2 * delCount) + (2 * addCount)];
			//IFDEBUG(fprintf(stderr, "WORD Apply sprmPChgTabsPapx: delete %d, add %d\n", delCount, addCount));

			uchar 	theDecimalChar	= '.';
			uchar 	theAlignChar		= '.';
			short	newPosition;
			
			// delete tabs				
			for (uchar i = 0; i < delCount; i++)
			{
				// BOGUS - blow off deleting tabs until Tom adds support to the filterStyle
				IFDEBUG(fprintf(stderr, "WORD Apply sprmPChgTabsPapx: deleting tabs not implemented\n"));
				break;
			}
			
			// add tabs
			for (uchar i = 0; i < addCount; i++, addPosPtr++, addDescPtr++)
			{
				// position needs to be swapped
				newPosition = B_LENDIAN_TO_HOST_INT16(*addPosPtr);
				//IFDEBUG(fprintf(stderr, "\tsprmPChgTabsPapx - pos: %d, desc: x%02x\n", newPosition, *addDescPtr));
				filterStyle->SetTab((newPosition / TWIPS_PER_POINT), (TTranslatorStyle::TabType)(*addDescPtr & tbd_jc), 
						((*addDescPtr & tbd_tlc) == tbd_tlc_none) ? TTranslatorStyle::kNoFill : TTranslatorStyle::kCharFill, 
						((*addDescPtr & tbd_jc) == tbd_jc_decimal) ? &theDecimalChar : NULL, 
						((*addDescPtr & tbd_tlc) == tbd_tlc_none) ? NULL : &theAlignChar);
			}
			break;
		}
		
		case sprmPFInTable:
			filterStyle->SetInTable((firstByte == 0x01) ? true : false);
			break;
		
		
		case sprmPFTtp:
			//filterStyle->SetEndTableRow((firstByte == 0x01) ? true : false);
			break;
		
			
		case sprmTDxaGapHalf:
			//memmove(&tempShort, &sprmBuffer[2], 2);
			//filterStyle->SetTableGapHalf(tempShort / TWIPS_PER_POINT);
			break;
			
		case sprmTDefTable:
			// two-byte parm length, next byte is number of cells in row
			{ // we need to set a scope for currentPos
				BList* cellWidthList = filterStyle->GetCellWidths();
				BList* tcList = filterStyle->GetTCList();
				
				short sprmSize = ((short*) sprmBuffer)[1];
				sprmSize += 3; // add the sprm type and size shorts
				
				int currentPos = 5;
				uchar cellCount = sprmBuffer[4];
				filterStyle->SetCellsInRow(cellCount);
				for (int i = 0; i <= cellCount; i++)
				{
					tempShort = B_LENDIAN_TO_HOST_INT16(*((short*)(&sprmBuffer[currentPos])));
					cellWidthList->AddItem((void*) (tempShort / TWIPS_PER_POINT));
					currentPos += 2;
				}
				for (int i = 0; i < cellCount; i++)
				{
					TC* tempTC = new TC;
					memset(tempTC, 0, sizeof(TC));
					if (currentPos < sprmSize)
						memmove(tempTC, &sprmBuffer[currentPos], sizeof(TC));

					for (int j = 0; j < 10; j++)
						((short*) tempTC)[j] = B_LENDIAN_TO_HOST_INT16(((short*) tempTC)[j]);
					tcList->AddItem(tempTC);
					currentPos += sizeof(TC);
				}
	
				filterStyle->SetEndTableRow(true);
			}
			break;
		
		case sprmTDyaRowHeight:
			memmove(&tempShort, &sprmBuffer[2], 2);
			filterStyle->SetRowHeight(tempShort / TWIPS_PER_POINT);
			break;
			
			
		case sprmCPicLocation:
			memmove(&tempLong, &sprmBuffer[2], 4);
			filterStyle->SetPicFC(B_LENDIAN_TO_HOST_INT16(tempLong));
			filterStyle->SetSpecialChar(true);
			IFDEBUG(fprintf(stderr, "WORD Apply sprmCPicLocation: fc %d\n", filterStyle->GetPicFC()));
			break;
		
				
		// sprms I should maybe probably handle, 	Now we handle this sprm - Joel
		case sprmCIstd:
		{
			memmove(&tempShort, &sprmBuffer[2], 2);
			TStyleWORD* applyStyle;
			// make sure this really is a char style
			applyStyle = styleTblWORD->StyleFromISTD(tempShort);
			
			if (applyStyle != 0 && applyStyle->GetStyleTypeCode() == sgcChp)
				filterStyle->SetApplyStyleName(applyStyle->StyleName());
			break;
		}
			
		// sprms I know about and am ignoring for now
		case sprmCRgLid0:
		case sprmPOutLvl:
		case sprmPFKeepFollow:
		case sprmCHpsKern:
		case sprmPIlvl:
		case sprmPIlfo:
		case sprmTTableBorders:
			break;
			  		
		default:
			break;
	}
}

void PostProccessStyle(TFilterStyleWORD *filterStyle)
{
	rgb_color color_high, color_low;
	pattern pat;
	
	if (filterStyle->GetTextColor(color_high, color_low, pat))
	{
		if (color_high.red == 0xFF && color_high.green == 0xFF && color_high.blue == 0xFF && 
							filterStyle->IsEmbossedImprint())
		{
			color_high.red = 0x00;
			color_high.green = 0x00;
			color_high.blue = 0x00;
			filterStyle->SetTextColor(color_high, color_low, pat);
		}
	}
}


/*void WriteStyleSheet(TTranslatorStylesTable* styles)
{
	
}*/

short GetSprm(char* buffer, int16 sprm, ushort value_short, char value_size)
{
	uchar value = value_short;
	char* curIndex = buffer;
	short swapedSprm = B_HOST_TO_LENDIAN_INT16(sprm);
	memmove(curIndex, &swapedSprm, 2);
	curIndex += 2;

	if (value_size == 2)
	{
		ushort swapedValue = B_HOST_TO_LENDIAN_INT16(value_short);
		memmove(curIndex, &swapedValue, 2);
		curIndex += 2;
	}
	else
	{
		*curIndex = value;
		curIndex++;
	}
	 return curIndex - buffer;
}




short GetParaSPRMListSize(const TTranslatorStyle* textStyle)
{
	int32 SPRMListSize = 0;
	
	float leftIndent = 0;
	if (textStyle->GetLeftIndent(leftIndent))
		SPRMListSize += 4;
	
	float leftFirstIndent;
	if (textStyle->GetFirstLineIndent(leftFirstIndent))
		SPRMListSize += 4;

	float rightIndent;
	if (textStyle->GetRightIndent(rightIndent))
		SPRMListSize += 4;

	int32 paraAlign;
	if (textStyle->GetParagraphAlignment(paraAlign) && paraAlign != TTranslatorStyle::kParAlignLeft)
		SPRMListSize += 3;
	
	if (textStyle->CountTabs())
	{
		SPRMListSize += 5;
		SPRMListSize += (textStyle->CountTabs() * 3);
	}


	float spaceBefore;
	bool lines;
	if (textStyle->GetSpaceBefore(spaceBefore, lines))
		SPRMListSize += 4;
	
	float spaceAfter;
	if (textStyle->GetSpaceAfter(spaceAfter, lines))
		SPRMListSize += 4;
		
	float lineSpacing;
	if (textStyle->GetLineSpacing(lineSpacing, lines))
		SPRMListSize += 6;
	
	return SPRMListSize;
}

int32 fontCount = 0;
#define MAXFONTCOUNT 29

short GetCharSPRMListSize(const TTranslatorStyle* textStyle, bool special)
{
	int32 SPRMListSize = 0;
	
	if (special)
		SPRMListSize += 3;
	
	TTranslatorStyle::StyleType styleType = TTranslatorStyle::kBasicStyle;
	textStyle->GetStyleType(styleType);
		
	if (styleType == TTranslatorStyle::kBasicStyle)
	{
// BOGUS NON STYLE TEST
		const char* styleName;
		if (textStyle->GetApplyStyleName(&styleName))
			SPRMListSize += 4;
	}

	int32 textFace = 0, textMask = 0;
	textStyle->GetTextFace(textFace, textMask);
	
	if (textFace & TTranslatorStyle::kBoldFace)
		SPRMListSize += 3;
	if (textFace & TTranslatorStyle::kItalicFace)
		SPRMListSize += 3;
	if (textFace & TTranslatorStyle::kStrikeThruFace)
		SPRMListSize += 3;
	if (textFace & TTranslatorStyle::kUnderlineFace || textFace & TTranslatorStyle::kDoubleUnderlineFace)
		SPRMListSize += 3;
	if (textFace & TTranslatorStyle::kSuperScriptFace || textFace & TTranslatorStyle::kSubScriptFace)
		SPRMListSize += 3;
	
	float textSize;
	if (textStyle->GetTextSize(textSize))
		SPRMListSize += 4;

	font_family fontName;
	font_style fontStyle;
	if (textStyle->GetTextFont(fontName, fontStyle))
		SPRMListSize += 4; // currentdebug
		//SPRMListSize += 12;

	rgb_color color_high, color_low;
	pattern pat;
	if (textStyle->GetTextColor(color_high, color_low, pat))
		SPRMListSize += 3;

	return SPRMListSize;
}


short GetSPRMListSize(const TTranslatorStyle* textStyle)
{
	return GetCharSPRMListSize(textStyle) + GetParaSPRMListSize(textStyle);
}

void GetParaSPRMList(const TTranslatorStyle* textStyle, char* SPRMList, TFontTblWORD* fontTbl, TStyleSheetWORD* styleTblWORD)
{
	char* curIndex = SPRMList;
	
	float leftIndent = 0;
	if (textStyle->GetLeftIndent(leftIndent))
		curIndex += GetSprm(curIndex, sprmPDxaLeft, (ushort) (leftIndent * TWIPS_PER_POINT), sizeof(ushort));
	

	float leftFirstIndent;
	if (textStyle->GetFirstLineIndent(leftFirstIndent))
		curIndex += GetSprm(curIndex, sprmPDxaLeft1, (ushort) ((leftFirstIndent - leftIndent) * TWIPS_PER_POINT), sizeof(ushort));



	float rightIndent;
	if (textStyle->GetRightIndent(rightIndent))
		curIndex += GetSprm(curIndex, sprmPDxaRight, (ushort) (rightIndent * TWIPS_PER_POINT), sizeof(ushort));



	int32 paraAlign;
	if (textStyle->GetParagraphAlignment(paraAlign))
	{
		switch (paraAlign) {
			case TTranslatorStyle::kParAlignCenter: paraAlign = 0x01; break;
			case TTranslatorStyle::kParAlignRight: paraAlign = 0x02; break;
			case TTranslatorStyle::kParAlignJust: paraAlign = 0x03; break;
			case TTranslatorStyle::kParAlignLeft: paraAlign = 0x00; break;
		}
		if (paraAlign)
			curIndex += GetSprm(curIndex, sprmPJc, paraAlign, sizeof(uchar));
	}

	uchar	delCount = 0;
	uchar	addCount = textStyle->CountTabs();
	
	if (textStyle->CountTabs())
	{
		*((short*) curIndex) = B_HOST_TO_LENDIAN_INT16(sprmPChgTabsPapx);
		curIndex += 2;
		*curIndex = (uchar) (delCount * 2) + (addCount * 3) + 2;
		curIndex += 1;
		*curIndex = delCount;
		curIndex += 1;
		*curIndex = addCount;
		curIndex += 1;
	}
#ifdef DEBUG
	const char* styleName;
	fprintf(stderr, "building sprm list for style %s\n", textStyle->GetStyleName(&styleName) ? styleName : "no name");
	fprintf(stderr, "it has %d tabs\n", textStyle->CountTabs());
#endif // DEBUG
	for (int i = 0; i < textStyle->CountTabs(); i++)
	{
		float pos;
		int32 type;
		int32 fillType;
		uchar alignChar;
		uchar fillChar;
		short newPosition = 0;
		
		textStyle->GetTab(i, pos, type, fillType, &alignChar, &fillChar);
		IFDEBUG(fprintf(stderr, "the %dth tab is at position %f\n", i, pos));
		*((short*) curIndex) = B_HOST_TO_LENDIAN_INT16(pos * TWIPS_PER_POINT);
		curIndex += 2;
	}

	for (int i = 0; i < textStyle->CountTabs(); i++)
	{
		float pos;
		int32 type;
		int32 fillType;
		uchar alignChar;
		uchar fillChar;
		short newPosition = 0;
		uchar wordDesc = 0;
		
		textStyle->GetTab(i, pos, type, fillType, &alignChar, &fillChar);
		
		wordDesc |= (type & tbd_jc);
		
		switch(fillType)
		{
			case TTranslatorStyle::kCharFill:
				wordDesc |= tbd_tlc_dotted;
				break;
			case TTranslatorStyle::kPenFill:
				wordDesc |= tbd_tlc_single;
				break;
		}
				
	//	if (*alignChar == (uchar) '.')
	//		wordDesc |= tbd_jc_decimal;
		
		
		//if (fillChar == 

		//((*addDescPtr & tbd_tlc) == tbd_tlc_none) ? NULL : &theAlignChar

		*curIndex = wordDesc;
		curIndex += 1;
	}

	float spaceBefore;
	bool line;
	if (textStyle->GetSpaceBefore(spaceBefore, line))
	{
		if (line)
			curIndex += GetSprm(curIndex, sprmPDyaBefore, (ushort) (spaceBefore * 12 * TWIPS_PER_POINT), sizeof(ushort));
		else
			curIndex += GetSprm(curIndex, sprmPDyaBefore, (ushort) (spaceBefore * TWIPS_PER_POINT), sizeof(ushort));
	}
	
	float spaceAfter;
	if (textStyle->GetSpaceAfter(spaceAfter, line))
	{
		if (line)
			curIndex += GetSprm(curIndex, sprmPDyaAfter, (ushort) (spaceAfter * 12 * TWIPS_PER_POINT), sizeof(ushort));
		else
			curIndex += GetSprm(curIndex, sprmPDyaAfter, (ushort) (spaceAfter * TWIPS_PER_POINT), sizeof(ushort));
	}
	
	float lineSpacing;
	if (textStyle->GetLineSpacing(lineSpacing, line))
	{
		*((int16 *) curIndex) = B_HOST_TO_LENDIAN_INT16(sprmPDyaLine);
		curIndex += 2;
		
		if (line)
		{
			*((int16 *) curIndex) = B_HOST_TO_LENDIAN_INT16((int16) (lineSpacing * 240));
			curIndex += 2;
			
			*((int16 *) curIndex) = B_HOST_TO_LENDIAN_INT16(1);
			curIndex += 2;
		}
		else
		{
			*((int16 *) curIndex) = B_HOST_TO_LENDIAN_INT16((int16) (lineSpacing * TWIPS_PER_POINT));
			curIndex += 2;
			
			*((int16 *) curIndex) = B_HOST_TO_LENDIAN_INT16(0);
			curIndex += 2;

		}
	}
	

}

void GetCharSPRMList(const TTranslatorStyle* textStyle, char* SPRMList, TFontTblWORD* fontTbl, TStyleSheetWORD* styleTblWORD, bool special)
{
	char* curIndex = SPRMList;

	if (special)
		curIndex += GetSprm(curIndex, sprmCFSpec, 0x01, sizeof(char));
		

	TTranslatorStyle::StyleType styleType = TTranslatorStyle::kBasicStyle;
	textStyle->GetStyleType(styleType);
		
	if (styleType == TTranslatorStyle::kBasicStyle)
	{
// BOGUS NON STYLE TEST
		const char* styleName;
		if(textStyle->GetApplyStyleName(&styleName))
		 	curIndex += GetSprm(curIndex, sprmCIstd, styleTblWORD->GetStyleIndex(styleName), sizeof(ushort));
	}

	int32 textFace = 0, textMask = 0;
	textStyle->GetTextFace(textFace, textMask);
	
	if (textFace & TTranslatorStyle::kBoldFace)
		curIndex += GetSprm(curIndex, sprmCFBold, 0x01, sizeof(char));
	if (textFace & TTranslatorStyle::kItalicFace)
		curIndex += GetSprm(curIndex, sprmCFItalic, 0x01, sizeof(char));
	if (textFace & TTranslatorStyle::kStrikeThruFace)
		curIndex += GetSprm(curIndex, sprmCFStrike, 0x01, sizeof(char));
	
	if (textFace & TTranslatorStyle::kUnderlineFace)
		curIndex += GetSprm(curIndex, sprmCKul, 0x01, sizeof(char));
	else if (textFace & TTranslatorStyle::kDoubleUnderlineFace)
		curIndex += GetSprm(curIndex, sprmCKul, 0x03, sizeof(char));

	if (textFace & TTranslatorStyle::kSuperScriptFace)
		curIndex += GetSprm(curIndex, sprmCIss, 0x01, sizeof(char));
	else if (textFace & TTranslatorStyle::kSubScriptFace)
		curIndex += GetSprm(curIndex, sprmCIss, 0x02, sizeof(char));
	
	float textSize;
	if (textStyle->GetTextSize(textSize))
		curIndex += GetSprm(curIndex, sprmCHps, (ushort) (textSize * 2), sizeof(ushort));
	
	font_family fontName;
	font_style fontStyle;
	
	if (textStyle->GetTextFont(fontName, fontStyle))
	{
		int32 fontIndex;
		fontIndex = fontTbl->GetFontIndex(fontName);
		if (fontIndex == -1)
		{
			fontTbl->AddFont(fontName);
			fontIndex = fontTbl->GetFontIndex(fontName);
		}
		curIndex += GetSprm(curIndex, sprmCRgFtc0, (ushort) fontIndex, sizeof(ushort));
		//curIndex += GetSprm(curIndex, sprmCRgFtc1, (ushort) fontIndex, sizeof(ushort));
		//curIndex += GetSprm(curIndex, sprmCRgFtc2, (ushort) fontIndex, sizeof(ushort));
	}
	
	
	rgb_color color_high, color_low;
	pattern pat;
	
	if (textStyle->GetTextColor(color_high, color_low, pat))
	{
		ushort closest_color = 0;
		double closest_distance = 0xfffffff;
		double current_distance;
		
		for (int i = 1; i < WORD_COLOR_COUNT; i++)
		{
			current_distance = sqrt((color_high.red - word_colors[i].red)*(color_high.red - word_colors[i].red) +
							(color_high.green - word_colors[i].green)*(color_high.green - word_colors[i].green) +
							(color_high.blue - word_colors[i].blue)*(color_high.blue - word_colors[i].blue));
			if (current_distance < closest_distance)
			{
				closest_color = i;
				closest_distance = current_distance;
			}
		}
		curIndex += GetSprm(curIndex, sprmCIco, (ushort) closest_color, sizeof(char));
		curIndex += 3;
	} 

		
}

void GetSPRMList(const TTranslatorStyle* textStyle, char* SPRMList, TFontTblWORD* fontTbl, TStyleSheetWORD* styleTblWORD)
{
	int32 charSprmsSize = GetCharSPRMListSize(textStyle);

	GetCharSPRMList(textStyle, SPRMList, fontTbl, styleTblWORD);
	GetParaSPRMList(textStyle, SPRMList + charSprmsSize, fontTbl, styleTblWORD);
}


uint16 STIForName(const char* styleName)
{
	for(int i = 0; i < (int) (sizeof(StyleNames) / sizeof(char*)); i++)
	{
		if (!strcmp(styleName, StyleNames[i]))
			return StyleIndexes[i];
	}
	return 4095;
}

















