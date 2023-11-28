//
//	WORDfkp.cpp
//

#include <stdlib.h>
#include <ctype.h>
#include "TranslatorLib.h"

#include "WORDfkp.h"
#include "WORDstyles.h"

//-------------------------------------------------------------------
// TFKPTbl
//-------------------------------------------------------------------

TFKPTbl::TFKPTbl(TOLEEntryReader *docReader, TOLEEntryReader *tableReader, long binPos, long binBytes, int16 nFib)
{
	mDocReader = docReader;
	mTableReader = tableReader;	
	mBinTblEntries = 0;
	mBinTbl = 0;	
	mTotalPages = 0;
	mMapPN = 0;	
	mFKPCount = 0;
	mFKPTbl = 0;
	mNFib = nFib;
	
	if (!loadBinTable(binPos, binBytes))
	{	
		// mMapPN is going to be a table of page numbers, with the page number being the
		// index, and the result being the index into mFKPTbl.  This will let us quickly
		// get the FKP for a particular PN.
		mTotalPages = (mDocReader->StreamDataSize() / 512) + 1;
		mMapPN = new long[mTotalPages];
		for (long i = 0; i < mTotalPages; i++)
			mMapPN[i] = -2;		
	}
}


TFKPTbl::~TFKPTbl()
{
	if (mBinTbl)
		delete [] mBinTbl;
	if (mMapPN)
		delete [] mMapPN;
	if (mFKPTbl)
		delete [] mFKPTbl;
}


int32 TFKPTbl::loadBinTable(long binPos, long binBytes)
{
	if (binPos < 0 || binBytes <= 0)
	{
		IFDEBUG(fprintf(stderr, "loadBinTable: empty bin table.  See pnFbpChpFirst or pnFbpPapFirst\n"));
		return B_ERROR; 
	}
	
	IFDEBUG(fprintf(stderr, "loadBinTable( binPos=%#6.4hx, binBytes=%#6.4hx )\n", binPos, binBytes));
	mTableReader->SetStreamPos(binPos);
	
	if (mNFib > 190)
		mBinTblEntries = (binBytes - 4) / (4 + SIZEOF_BTE);
	else
		mBinTblEntries = (binBytes - 4) / (4 + sizeof(short));
	
	// We are going to save the bin table in our own memory structure,
	// which will be the BTE page number plus the CP for that PCD
	
	mBinTbl = new BinTbl[mBinTblEntries];
	if (!mBinTbl)
	{
		IFDEBUG(fprintf(stderr, "loadBinTable: no memory for bintbl\n"));
		return B_NO_MEMORY; 
	}
	
	// Read in the array of FCs.  In the pclfbte, number of FCs is binTblEntries + 1.
	// The first FC (lower limit) will be skipped because they are in order and we just
	// need the upper limits.
	long firstFC = mTableReader->ReadLong();
	for (long i = 0; i < mBinTblEntries; i++)
		mBinTbl[i].fcMax = mTableReader->ReadLong();
	
#ifdef DEBUG			
	for ( int i = 0; i < mBinTblEntries; i++)
		fprintf(stderr, "mBinTbl[%d].fcMax = %d\n", i, mBinTbl[i].fcMax);
#endif // DEBUG
	
	// Now read the array of BTEs into our bin tbl
	for (long i = 0; i < mBinTblEntries; i++)
	{
		if (mNFib > 190)
			mBinTbl[i].pn = mTableReader->ReadLong();
		else
			mBinTbl[i].pn = mTableReader->ReadShort();
		mBinTbl[i].pn &= btePN;	// page number is just the first 22 bits of the long
	}

#ifdef DEBUG
	for ( int i = 0; i < mBinTblEntries; i++)
		fprintf(stderr, "mBinTbl[%d].pn = %d\n", i, mBinTbl[i].pn);
#endif // DEBUG

	return mTableReader->Error();
}


// Load each FKP that has a page number in the bin table
bool TFKPTbl::Load(void)
{
	long pn;
	
	mFKPCount = 0;
	
	//printf("there are %d BinTblEntries\n", mBinTblEntries);
	
	// get an exact count of how many FKPs we're going to load	
	for (long i = 0; i < mBinTblEntries; i++)
	{
		pn = mBinTbl[i].pn;
		
		if (pn >= mTotalPages)
		{
			IFDEBUG(fprintf(stderr, "WORD FKP: pn %d > %d total pages\n", pn, mTotalPages));
			return false;
		} 
	
		// see if we've already seen this page
		if (mMapPN[pn] == -1)
			continue;
		
		mMapPN[pn] = -1;	// temp value, will be replaced below	
		mFKPCount++;
	}
	
	IFDEBUG(fprintf(stderr, "mFKPCount = %d\n", mFKPCount));
	
	//IFDEBUG(fprintf(stderr, "WORD FKP: fkpCount: %d\n", mFKPCount));
	
	// allocate space for all of the FKPs
	mFKPTbl = new FKP[mFKPCount];
	if (!mFKPTbl)
	{
		IFDEBUG(fprintf(stderr, "WORD FKP: out of memory for FKP tbl\n"));
		return false;
	}
		
	// now load the FKPs
	long	fkpIndex = 0;
	short	crun; 
	for (long i = 0; i < mBinTblEntries; i++)
	{
		pn = mBinTbl[i].pn;
		IFDEBUG(fprintf(stderr, "loading bytes for fkp in BinTable entry %d\n", i));
		IFDEBUG(fprintf(stderr, "loading page %d\n", pn));
		
		// see if we've already loaded this page
		if (mMapPN[pn] >= 0)
			continue;
		
		// get the crun, which is at the end of the page
		mDocReader->SetStreamPos((pn * 512) + 511);
		mFKPTbl[fkpIndex].fkp[511] = mDocReader->ReadChar();
		crun = (uchar)mFKPTbl[fkpIndex].fkp[511];
		
		if (crun > 128)
			return false;
		IFDEBUG(fprintf(stderr, "WORD FKP: fkpIndex: %d, crun: %d\n", mFKPCount, crun));
		
		// read in the array of FCs 
		mDocReader->SetStreamPos(pn * 512);		// set to beginning of FKP
		long *longPtr = (long *)&(mFKPTbl[fkpIndex].fkp[0]);
	
		*longPtr = mDocReader->ReadLong();
		
		for (short j = 0; j < crun; j++)
		{
			mStartFCList.AddItem(longPtr);
			longPtr++;
			*longPtr = mDocReader->ReadLong();
			
			mEndFCList.AddItem(longPtr);
			//printf("fc[%d] = %d\n", j, *longPtr);
		}
		longPtr++;
		
		if (mDocReader->Error() < 0)
			return false;
		
		// have the child class read the guts of the FKP
		if (!LoadBytes(pn, fkpIndex, crun))
			return false;
		
		mMapPN[pn] = fkpIndex;
		fkpIndex++;	
	}
	
	return true;
}


// Given a FC, return a pointer to it's FKP data.  Also set the start and endFC and the index
// of where this FC occurs in the FC array.
char * TFKPTbl::getFKP(long fc, long& startFC, long& endFC, char& index)
{
	char *fkp = NULL;
	
	// First find the FKP page number
	long pn = -1, fkpIndex = -1;
	for (long i = 0; i < mBinTblEntries; i++)
	{
		if (fc < mBinTbl[i].fcMax)
		{
			pn = mBinTbl[i].pn;
			break;
		}
	}
	
	if (pn >= 0 && pn < mTotalPages)
	{
		long fkpIndex = mMapPN[pn];
		if (fkpIndex >= 0 && fkpIndex < mFKPCount)
			fkp = &(mFKPTbl[fkpIndex].fkp[0]);
		else
			IFDEBUG(fprintf(stderr, "WORD getFKP: bad fkp index: %d\n", fkpIndex));
	}
	else
		IFDEBUG(fprintf(stderr, "WORD getFKP: bad page number: %d\n", pn));

	if (!fkp)
		return fkp;

	// There are crun+1 FCs, the ith FC is the start of the ith record,
	// and the i+1 FC is the next character after the end of the record (the limitFC).
	// The first FC better be <= to our fc, or we're on the wrong page.
	startFC = *((long *)fkp);
	if (startFC > fc)
	{
		IFDEBUG(fprintf(stderr, "WORD getFKP: on the wrong FKP page: %d, fc: %d, firstFC: %d\n", pn, fc, startFC));
		return NULL;
	}		

	char crun = fkp[511];	
	for (index = 1; index <= crun; index++)
	{
		endFC = *((long *)&(fkp[(index * 4)]));
		if (fc < endFC)
			break;
		startFC = endFC;
	}
	if (index > crun)
	{
		IFDEBUG(fprintf(stderr, "WORD getFKP: on the wrong FKP page2: %d, fc: %d, firstFC: %d\n", pn, fc, endFC));
		return NULL;
	}
	
	// back up the index so that we're at the index containing this FC
	index--;
	
	return fkp;
}


//-------------------------------------------------------------------
// TCHPXTbl
//-------------------------------------------------------------------

TCHPXTbl::TCHPXTbl(TOLEEntryReader *docReader, TOLEEntryReader *tableReader, long binPos, long binBytes, int16 nFib)
	: TFKPTbl(docReader, tableReader, binPos, binBytes, nFib)
{
}


// File position is right after the FC array of the FKP			
bool TCHPXTbl::LoadBytes(long pn, long fkpIndex, short crun)
{
	long	fkpPos = 4 * (crun + 1);
	short	tempShort, firstOffset = 9999;

#ifdef DEBUG	
	fprintf(stderr, "loading CHP on page %d, fkpIndex = %d, crun = %d\n", pn, fkpIndex, crun);
	
	long origPosition = mDocReader->GetStreamPos();
	char theFKP[512];
	mDocReader->SetStreamPos((origPosition / 512) * 512);
	mDocReader->ReadBytes(theFKP, 512);
	mDocReader->SetStreamPos(origPosition);
	fprintf(stderr, "CHPX FKP = {");
	for (int i = 0; i < 512; i++)
	{
		if (!(i%16))
			fprintf(stderr, "\n");
		fprintf(stderr, "%#2.2hx, ", ((uchar *)&theFKP)[i]);
	}
	fprintf(stderr, "}\n");
#endif // DEBUG

	// Read in the array of byte offsets to each CHPX.  There are crun of them
	for (long i = 0; i < crun; i++)
	{
		mFKPTbl[fkpIndex].fkp[fkpPos] = mDocReader->ReadChar();
		tempShort = (uchar)mFKPTbl[fkpIndex].fkp[fkpPos];
		//IFDEBUG(fprintf(stderr, "WORD FKP-CHPX: offset: %d\n", tempShort));
		if (tempShort != 0 && tempShort < firstOffset)
			firstOffset = tempShort;
		fkpPos++;
	}
	if (mDocReader->Error() < 0)
		return false;
	//IFDEBUG(fprintf(stderr, "WORD FKP-CHPX: first offset: %d\n", 2 * firstOffset));
	
	// next is a variable sized unused area, and then the CHPXs stored end to end.  I saved the offset
	// of the first CHPX above, so we can skip to it.
	if (firstOffset == 9999)
	{
		IFDEBUG(fprintf(stderr, "returning because all were default\n"));
		// no CHPXs to read (all were default)
		mDocReader->ReadBytes(&(mFKPTbl[fkpIndex].fkp[fkpPos]), 512 - fkpPos);
		return !mDocReader->Error();
	}
	
	// read everything up to the first CHPX
	mDocReader->ReadBytes(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (2 * firstOffset) - fkpPos);
	if (mDocReader->Error() < 0)
		return false;

	fkpPos = 2 * firstOffset;
		
	// read each CHPX, which is a byte-count and then sprms.
	ushort	theSprm, chpxBytes, chpxPos;
	short sprmBytes;
	
	int i = 0;
	while (fkpPos < 510)
	{
		if (mDocReader->GetStreamPos() % 2) // make sure we are on an even byte
		{
			mDocReader->ReadChar();
			mFKPTbl[fkpIndex].fkp[fkpPos] = 0;
			fkpPos++;
		}
		
		// byte count for this CHPX
		mFKPTbl[fkpIndex].fkp[fkpPos] = mDocReader->ReadChar();
		chpxBytes = (uchar)(mFKPTbl[fkpIndex].fkp[fkpPos]);
		fkpPos++;
		
		
		//IFDEBUG(fprintf(stderr, "WORD FKP-CHPX: chpx bytes: %d\n", chpxBytes));
		
		// now read each sprm in this CHPX
		chpxPos = 0;
		
		IFDEBUG(fprintf(stderr, "reading CHPX %d\n", i));
		
		while (chpxPos < chpxBytes)
		{
			sprmBytes = ReadSPRM(mDocReader, &(mFKPTbl[fkpIndex].fkp[fkpPos]), theSprm, mNFib);
			//IFDEBUG(fprintf(stderr, "\tsprm: %04x, sprmBytes: %x\n", theSprm, sprmBytes));
			if (sprmBytes < 0)
				return false;
			
			/*printf("CHPX sprm = {");
			for (int i = 0; i < sprmBytes; i++)
			{
				if (!(i%16))
					printf("\n");
				printf("%#2.2hx ", (&(mFKPTbl[fkpIndex].fkp[fkpPos]))[i]);
			}
			printf("}\n");*/
			
			
			fkpPos += sprmBytes;
			chpxPos += sprmBytes; 
		}
		i++;
		if (mDocReader->Error() < 0)
			return false;
	}
	
	ASSERT(fkpPos <= 512, "TCHPXTbl::LoadBytes: wrote past the end of the fkp!!");
	
	return true;
}


// Given an FC, find the FKP page for it in the bin table, and then search that FKP page
// for the CHPX entry.  Fill in the start and end FCs and sprms for that CHPX.
// Returns true if there were no problems.
bool TCHPXTbl::GetSprms(long fc, long& startFC, long& endFC, char *& sprms, ushort& sprmBytes)
{	
	// get the FKP data for this fc.  It will also get the index of where this FC occurs
	// in the FKP, and will set the limitFC.
	char	index;
	char *	fkp = getFKP(fc, startFC, endFC, index);	
	if (!fkp)
		return false;	// getFKP will do debug errors
	
	// Parse the FKP data to find the CHPX for this FC.
	char crun = fkp[511];	
	
	// Get the ith char word offset.
	short offset = (uchar)fkp[(4 * ((ushort) crun + 1)) + index];
	offset *= 2;	// it's a word offset
	
	if (!offset)
	{
		// para character styles
		sprms = NULL;
		sprmBytes = 0;
		return true;
	}
	
	// We're now at the CHPX, so we can get the sprms
	sprmBytes = *(fkp + offset);
	sprms = fkp + offset + 1;

	return true;
}


//-------------------------------------------------------------------
// TPAPXTbl
//-------------------------------------------------------------------

TPAPXTbl::TPAPXTbl(TOLEEntryReader *docReader, TOLEEntryReader *tableReader, long binPos, long binBytes, int16 nFib)
	: TFKPTbl(docReader, tableReader, binPos, binBytes, nFib)
{
}


// File position is right after the FC array of the FKP			
bool TPAPXTbl::LoadBytes(long pn, long fkpIndex, short crun)
{
	long	tempLong, fkpPos = 4 * (crun + 1);
	short	tempShort, firstOffset = 9999;
	
	//printf("PAPX FKP looks like:\n");
		
#ifdef DEBUG
	long origPosition = mDocReader->GetStreamPos();
	char theFKP[512];
	mDocReader->SetStreamPos((origPosition / 512) * 512);
	mDocReader->ReadBytes(theFKP, 512);
	mDocReader->SetStreamPos(origPosition);
	
	
	fprintf(stderr, "PAPX FKP = {");
	for (int i = 0; i < 512; i++)
	{
		if (!(i%16))
			fprintf(stderr, "\n");
		fprintf(stderr, "%#2.2hx, ", ((uchar *)&theFKP)[i]);
	}
	fprintf(stderr, "}\n");
#endif // DEBUG


	IFDEBUG(fprintf(stderr, "TPAPXTbl::LoadBytes, pn = %d fkpIndex = %d crun = %d\n", pn, fkpIndex, crun));
	
	// Read in the array of BX structures (13 bytes).  There are crun of them.
	for (long i = 0; i < crun; i++)
	{
		IFDEBUG(fprintf(stderr, "starting to read BX[%d]\n", i));
		// offset
		mFKPTbl[fkpIndex].fkp[fkpPos] = mDocReader->ReadChar();
		tempShort = (uchar)mFKPTbl[fkpIndex].fkp[fkpPos];
		//IFDEBUG(fprintf(stderr, "WORD FKP-PAPX: offset: %d\n", tempShort));
		if (tempShort != 0 && tempShort < firstOffset)
			firstOffset = tempShort;
		fkpPos++;
		
		// PHE - short, short, long, long
		if (mNFib > 190)
		{
			tempShort = mDocReader->ReadShort();
			memmove(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (char *)&tempShort, 2);
			//printf("bitfield1 = %d\n", tempShort);
			fkpPos += 2;
			tempShort = mDocReader->ReadShort();
			memmove(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (char *)&tempShort, 2);
			//printf("bitfield2 = %d\n", tempShort);
			fkpPos += 2;
			tempLong = mDocReader->ReadLong();
			memmove(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (char *)&tempLong, 4);
			//printf("dymLine = %d\n", tempLong);
			fkpPos += 4;
			tempLong = mDocReader->ReadLong();
			memmove(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (char *)&tempLong, 4);
			//printf("dymHeight = %d\n", tempLong);
			fkpPos += 4;
		}
		else
		{
			tempShort = mDocReader->ReadShort();
			memmove(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (char *)&tempShort, 2);
			//printf("bitfield1 = %d\n", tempShort);
			fkpPos += 2;
			tempShort = mDocReader->ReadShort();
			memmove(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (char *)&tempShort, 2);
			//printf("dymLine = %d\n", tempLong);
			fkpPos += 2;
			tempShort = mDocReader->ReadShort();
			memmove(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (char *)&tempShort, 2);
			//printf("dymHeight = %d\n", tempLong);
			fkpPos += 2;
		}
		
	}
	if (mDocReader->Error() < 0)
		return false;
	
	//printf("fkppos = %d\n", fkpPos);
	
	//IFDEBUG(fprintf(stderr, "WORD FKP-PAPX: first offset: %d\n", 2 * firstOffset));
	
	// next is a variable sized unused area, and then the PAPXs stored end to end.  I saved the offset
	// of the first PAPX above, so we can skip to it.
	if (firstOffset == 9999)
	{
		// no PAPXs to read (all were default)
		IFDEBUG(fprintf(stderr, "no PAPXs to read (all were default)\n"));
		mDocReader->ReadBytes(&(mFKPTbl[fkpIndex].fkp[fkpPos]), 512 - fkpPos);
		return !mDocReader->Error();
;
	}
	
	// read everything up to the first PAPX
	mDocReader->ReadBytes(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (2 * firstOffset) - fkpPos);
	if (mDocReader->Error() < 0)
		return false;

	fkpPos = 2 * firstOffset;
		
	// read each PAPX, which is a word-count (FKP only, byte-count in STSH) and then sprms.
	char	papxWords;
	ushort	styleIndex, theSprm, papxBytes, papxPos;
	short  sprmBytes;
	while (fkpPos < 510)
	{
		// word-count for this PAPX (if not zero, includes this byte)
		mFKPTbl[fkpIndex].fkp[fkpPos] = papxWords = mDocReader->ReadChar();
		fkpPos++;
		papxPos = 1;
		
		// if previous byte is zero, then this is the real word-count
		if (papxWords == 0)
		{
			// word-count does not include the zero pad or this byte
			mFKPTbl[fkpIndex].fkp[fkpPos] = papxWords = mDocReader->ReadChar();
			fkpPos++;
			papxPos = 2;
		}
		
		
		papxBytes = (2 * (uchar)papxWords);

		// style index
		styleIndex = mDocReader->ReadShort();
		memmove(&(mFKPTbl[fkpIndex].fkp[fkpPos]), (char *)&styleIndex, 2);
		fkpPos += 2;
		papxPos += 2;
		
		
		//IFDEBUG(fprintf(stderr, "WORD FKP-PAPX: papx bytes: %d, style index: %d\n", papxBytes, styleIndex));
		
		// now read each sprm in this CHPX
		while (papxPos < papxBytes)
		{
			sprmBytes = ReadSPRM(mDocReader, &(mFKPTbl[fkpIndex].fkp[fkpPos]), theSprm, mNFib);
			if (sprmBytes < 0)
				return false;
				
			//IFDEBUG(fprintf(stderr, "\tsprm: %04x, sprmBytes: %x\n", theSprm, sprmBytes));

			
#ifdef DEBUG
			fprintf(stderr, "PAPX sprm = {");
			for (int i = 0; i < sprmBytes; i++)
			{
				if (!(i%16))
					fprintf(stderr, "\n");
				fprintf(stderr, "%#2.2hx ", (uchar) (&(mFKPTbl[fkpIndex].fkp[fkpPos]))[i]);
			}
			fprintf(stderr, "}\n");
#endif // DEBUG			
			

			fkpPos += sprmBytes;
			papxPos += sprmBytes;
		}
		
		if (mNFib <= 190)
		{
			if (mDocReader->GetStreamPos() % 2)
			{
				mDocReader->ReadChar(); // in Word 6 there _seems_ to be an extra two bytes between each papx
				fkpPos += 1;
				papxPos += 1;
			}
			else
			{
				mDocReader->ReadShort(); // in Word 6 there _seems_ to be an extra two bytes between each papx
				fkpPos += 2;
				papxPos += 2;
			}
		}
			
		if (mDocReader->Error() < 0)
			return false;

		//printf("exiting TPAPXTbl::LoadBytes\n");
	}
	ASSERT(fkpPos <= 512, "wrote past the end of the fkp!!");
	
	return true;
}


// Given an FC, find the FKP page for it in the bin table, and then search that FKP page
// for the PAPX entry.  Fill in the start and end FCs, istd, and sprms for that PAPX.
// Returns true if there were no problems.
bool TPAPXTbl::GetSprms(long fc, long& startFC, long& endFC, ushort& istd, char *& sprms, ushort& sprmBytes)
{	
	// get the FKP data for this fc.  It will also get the index of where this FC occurs
	// in the FKP, and will set the start and end FCs.
	char	index;
	char *	fkp = getFKP(fc, startFC, endFC, index);	
	if (!fkp)
		return false;	// getFKP will do debug errors
		
	
	// Parse the FKP data to find the PAPX for this FC.
	char crun = fkp[511];	
	
	// Get the ith BX.  The BX struct starts with a char word offset to the PAPX, which is all 
	// we're concerned with here.
	short offset;
	
	if (mNFib > 190)
		offset = (uchar)fkp[(4 * (crun + 1)) + (index * SIZEOF_BX)];
	else // word 6
		offset = (uchar)fkp[(4 * (crun + 1)) + (index * 7)]; // BX is 7 bytes in word 6
	
	offset *= 2;	// it's a word offset
	
	if (!offset)
	{
		istd = istdNormal;
		sprms = NULL;
		sprmBytes = 0;
		return true;
	}
	
	// We're now at the PAPX, so we can get the istd and sprms
	char *papx = fkp + offset;
	
	// word-count for this PAPX
	uchar papxWords = *papx++;
	
	// if previous byte is zero, then this is the real word-count
	if (papxWords == 0)
	{
		papxWords = *papx++;
		
		// word-count does not include the zero pad or this byte
		sprmBytes = (2 * (ushort) papxWords) - 2;	// the istd
	}
	else
	{
	 	// if not zero, includes the word-count byte
		sprmBytes = (2 * (ushort) papxWords) - 3;	// this byte plus the istd
	}
	
	istd = *((ushort *)papx);
	sprms = (papx + 2);
	
	if (sprmBytes > 512) // we'll just assume it rolled under for some reason
	{
		sprmBytes = 0;
	}
	
	return true;
}

