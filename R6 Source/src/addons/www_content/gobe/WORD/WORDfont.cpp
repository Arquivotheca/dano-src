//
//	WORDfont.cpp
//

#include "WORDfont.h"


//-------------------------------------------------------------------
// TFontTblWORD
//-------------------------------------------------------------------

TFontTblWORD::TFontTblWORD(TOLEEntryReader *tableReader, long tableStreamSize, long offset, ulong offsetBytes, int16 nFib)
	: TFontTable()
{		
	mFontTableWORD = new BList(50);
	
	mNFib = nFib;
	
	// make sure that we don't go off the edge of the stream
	if ((offset + offsetBytes) > (ulong) tableStreamSize)
		IFDEBUG(fprintf(stderr, "WORD TFontTblWORD: ERROR: %d offset, %d offsetBytes, %d tableStreamSize\n", offset, offsetBytes, tableStreamSize));
	else
	{				
		tableReader->SetStreamPos(offset);
		
		// it's an sttbf, so it starts with a count of entries
		ushort	tableItems = tableReader->ReadShort();
		IFDEBUG(fprintf(stderr, "there are %d fonts\n", tableItems));
		//IFDEBUG(fprintf(stderr, "WORD TFontTblWORD: %d table items\n", tableItems));

		// next is bytes of extra data.  This should be zero for font table
		ushort	extraDataBytes = 0;
		if (mNFib > 190)
			extraDataBytes = tableReader->ReadShort();
			
		if (!extraDataBytes)
		{	
			TFontWORD *	fontWORD;
			uchar		bytesFFN;
		
			for (ushort i = 0; i < tableItems && (ulong) tableReader->GetStreamPos() < offset + offsetBytes; i++)
			{
				bytesFFN = (uchar)tableReader->ReadChar();
				
				fontWORD = new TFontWORD(this, tableReader, bytesFFN, mNFib);
				if (fontWORD)
					mFontTableWORD->AddItem(fontWORD);
				else
					IFDEBUG(fprintf(stderr, "WORD TFontTblWORD: ERROR creating TFontWORD\n"));
				if (tableReader->Error())
					return;
			}
		}
		else
			IFDEBUG(fprintf(stderr, "WORD TFontTblWORD: ERROR - %d extra data bytes\n", extraDataBytes));
	}
}

TFontTblWORD::TFontTblWORD()
{
	mFontTableWORD = new BList(50);
	
	TFontWORD *times, *symbol, *arial;
	times = new TFontWORD();
	symbol = new TFontWORD();
	arial = new TFontWORD();
	
	times->SetFontName("Times New Roman");
	symbol->SetFontName("Symbol");
	arial->SetFontName("Arial");
	
	times->SetFontFamilyCode(1);
	symbol->SetFontFamilyCode(1);
	arial->SetFontFamilyCode(2);

	times->SetFontCharSet(0);
	symbol->SetFontCharSet(2);
	arial->SetFontCharSet(0);
	
	AddFont(times);
	AddFont(symbol);
	AddFont(arial);

}


TFontTblWORD::~TFontTblWORD()
{
	if (mFontTableWORD)
	{	
		for (long i = mFontTableWORD->CountItems() - 1; i >= 0; i--)
			delete (TFontWORD *)(mFontTableWORD->ItemAt(i));
		
		delete mFontTableWORD;
	}
}

int32 TFontTblWORD::AddFont(char* fontName)
{
	TFontWORD *newFont = new TFontWORD();
	newFont->SetFontName(fontName);
	newFont->SetFontFamilyCode(0);
	newFont->SetFontCharSet(0);
	return AddFont(newFont);
}

uint32 TFontTblWORD::ConvertFTCtoBeFontID(ushort ftc)
{
	if (ftc >= mFontTableWORD->CountItems())
		return be_plain_font->FamilyAndStyle();
	
	TFontWORD * theFont = (TFontWORD *)(mFontTableWORD->ItemAt(ftc));
	return theFont->GetBeFontID();
}

const char* TFontTblWORD::ConvertFTCtoName(ushort ftc)
{
	if (ftc >= mFontTableWORD->CountItems())
		return "Default";
		
	TFontWORD * theFont = (TFontWORD *)(mFontTableWORD->ItemAt(ftc));
	return theFont->GetFontName();
}

int32 TFontTblWORD::AddFont(TFontWORD* newFont)
{
	mFontTableWORD->AddItem(newFont);
		
	return B_NO_ERROR;
}


int32 TFontTblWORD::Write(TOLEEntryWriter *tableWriter, long &offset, ulong &offsetBytes)
{	
	offset = tableWriter->GetStreamPos();
	
	
	tableWriter->WriteShort(mFontTableWORD->CountItems());
	tableWriter->WriteShort(0); //extra data bytes
	
	TFontWORD *	fontWORD;
	uchar		bytesFFN;

	for (ushort i = 0; i < mFontTableWORD->CountItems(); i++)
	{
		fontWORD = (TFontWORD *) mFontTableWORD->ItemAt(i);
		bytesFFN = fontWORD->GetBytesFFN();
		IFDEBUG(fprintf(stderr, "bytesFFN = %d\n", bytesFFN));
		tableWriter->WriteChar((uchar) bytesFFN);
		fontWORD->Write(tableWriter);
	}
	
	offsetBytes = tableWriter->GetStreamPos() - offset;
	
	return B_NO_ERROR;
}

int32 TFontTblWORD::GetFontIndex(char* name)
{
	TFontWORD *	fontWORD;
	for (int i = 0; i < mFontTableWORD->CountItems(); i++)
	{
		fontWORD = (TFontWORD *) mFontTableWORD->ItemAt(i);
		if (!strcmp(fontWORD->GetFontName(), name))
			return i;
	}	
	
	return -1;
}


//-------------------------------------------------------------------
// TFontWORD
//-------------------------------------------------------------------

TFontWORD::TFontWORD(TFontTblWORD *fontTbl, TOLEEntryReader *tableReader, uchar bytesFFN, int16 nFib)
{
	int32 startingPos = tableReader->GetStreamPos();
	
	mNFib = nFib;
	
	// FFN structure
	mFFN.cbFfnM1 = bytesFFN;
	mFFN.bitField = (uchar)tableReader->ReadChar();	
	mFFN.wWeight = (uchar)tableReader->ReadShort();	
	mFFN.chs = (uchar)tableReader->ReadChar();	
	mFFN.ixchSzAlt = (uchar)tableReader->ReadChar();
	if (mNFib > 190)
	{
		tableReader->ReadBytes(&(mFFN.panose[0]), SIZEOF_PANOSE);	
		tableReader->ReadBytes(&(mFFN.fs[0]), SIZEOF_FONTSIGNATURE);
	}

	// Next is the XCHAR name(s), max of 65 characters (includes null terminator)
	uchar nameBytes = bytesFFN - SIZEOF_FFN_FIXED;
	if (nameBytes > FFN_MAX_NAME_BYTES)
	{
		nameBytes = FFN_MAX_NAME_BYTES - 1;
		IFDEBUG(fprintf(stderr, "WORD TFontWORD: ERROR: name too long %u\n", nameBytes));		
	}
	
	if (tableReader->Error() < 0)
		return;
	
	if (mNFib > 190)
	{
		uchar	nameChars = nameBytes / 2;
		ushort	tempShort;
		
		// we need to swap the unicode characters as they come in,
		for (uchar i = 0; i < nameChars; i++)
		{
			tableReader->ReadBytes((char*)&tempShort, sizeof(short));
			tempShort = B_SWAP_INT16(tempShort);
			memmove((char *)&(mFFN.xszFfn[i * 2]), (char *)&tempShort, sizeof(short));
		}
			
		// convert name to utf8
		int32	srcBytes = nameBytes;
		int32 	dstBytes = 255;
		int32 	state = 0;
		convert_to_utf8(B_UNICODE_CONVERSION, &(mFFN.xszFfn[0]), &srcBytes, &mFontName[0], &dstBytes, &state);
	}
	else
	{
		char tempChar = 0;
		int i;
		for(i = 0; (tempChar = tableReader->ReadChar(), tempChar && tableReader->Error() == B_OK); i++)
			mFFN.xszFfn[i] = tempChar;
		
		mFFN.xszFfn[i] = 0;
		
		// convert name to utf8
		int32	srcBytes = i + 1;
		int32 	dstBytes = 255;
		int32 	state = 0;
		convert_to_utf8(B_MS_WINDOWS_CONVERSION, &(mFFN.xszFfn[0]), &srcBytes, &mFontName[0], &dstBytes, &state);
		
	}
	mFontName[255] = 0;		// in case string is too long or has no terminator
	
	if(tableReader->Error() < 0)
		return;
	
	// Determine which Be font most closely matches this WORD font
	// WORD font family code seems to match RTF family code directly, so just pass it in
	mBeFontID = fontTbl->FindBestMatch(&mFontName[0], GetFontFamilyCode(), GetFontCharSet());	

	//IFDEBUG(fprintf(stderr, "WORD TFontWORD: family %02x, charSet %02x, BeID %d, %s\n", GetFontFamilyCode(), GetFontCharSet(), GetBeFontID(), GetFontName()));

#ifdef DEBUG
	tableReader->SetStreamPos(startingPos);
	
	char * fontRecord = new char[bytesFFN];
	
	tableReader->ReadBytes(fontRecord, bytesFFN);

	fprintf(stderr, "font name is %s, family code is %d, charset is %d\n", GetFontName(), GetFontFamilyCode(), GetFontCharSet());
	
	fprintf(stderr, "font recod looks like = {");
	for (int i = 0; i < bytesFFN; i++)
	{
		if (!(i%16))
			fprintf(stderr, "\n");
		fprintf(stderr, "%#2.2hx, ", fontRecord[i]);
	}
	fprintf(stderr, "}\n");
	delete [] fontRecord;
#endif // DEBUG

	tableReader->SetStreamPos(startingPos + bytesFFN);

}

int32 TFontWORD::GetBytesFFN()
{
	int32 nameLength = strlen(mFontName);
	int32 state = 0;
	int32 dstBytes = FFN_MAX_NAME_BYTES;

	
	//nameLength++;
	
	IFDEBUG(fprintf(stderr, "the font name is %s\n", mFontName));
	
	convert_from_utf8(B_UNICODE_CONVERSION, mFontName, &nameLength, mFFN.xszFfn, &dstBytes, &state, '?');
	
	mFFN.xszFfn[dstBytes * 2] = 0;
	mFFN.xszFfn[(dstBytes * 2) + 1] = 0;
	
	return SIZEOF_FFN_FIXED + dstBytes + 2;
}


int32 TFontWORD::Write(TOLEEntryWriter *tableWriter)
{
	//GetBytesFFN(); // -BOGUS this call makes sure the mFFN.xszFfn is initialized,
						// the foint table will have had to call it before though
	
	tableWriter->WriteChar(mFFN.bitField);
	tableWriter->WriteShort(mFFN.wWeight);
	tableWriter->WriteChar(mFFN.chs);
	tableWriter->WriteChar(mFFN.ixchSzAlt);
	tableWriter->WriteBytes(&(mFFN.panose[0]), SIZEOF_PANOSE);	
	tableWriter->WriteBytes(&(mFFN.fs[0]), SIZEOF_FONTSIGNATURE);
	
	int32 nameLength = strlen(mFontName);
		
	for(int i = 0; i <= nameLength; i++)
	{
		*((short*) ( mFFN.xszFfn + (i * 2))) = B_SWAP_INT16(*((short*) ( mFFN.xszFfn + (i * 2))));
	}
	tableWriter->WriteBytes(mFFN.xszFfn, (nameLength + 1) * 2);
	
	return B_NO_ERROR;
}

TFontWORD::~TFontWORD(void)
{
}

