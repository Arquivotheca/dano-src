//---------------------------------------------------------------------
//
//	File:	AVIUtils.cpp
//
//	Author:	Gene Z. Ragan
//
//	Date:	08.25.98
//
//	Desc:	Application Utilities
//
//
//---------------------------------------------------------------------

//	Includes
#include "BuildApp.h"

#include <string.h>

#include "AppUtils.h"
#include "RIFFConstants.h"
#include "RIFFTypes.h"

#include "AVIUtils.h"

#pragma mark -
#pragma mark === Byte Order Utiities ===

//-------------------------------------------------------------------
//	ReadIntMsb
//-------------------------------------------------------------------
//
//

int32 ReadIntMsb(FILE *in, int size) 
{
	uint32 value = 0;
	
	if (size <= 0) 
		return 0;
		
	//	Clear out bits
	switch(size)
	{
		case 1:
			{
				char charBuf;
				fread(&charBuf, size, 1, in);
				value = charBuf;
			}
			break;

		case 2:
			{
				int16 shortBuf;
				//in->Read(&shortBuf, size);
				fread(&shortBuf, size, 1, in);
				swap_data(B_INT16_TYPE, &shortBuf, sizeof(int32), B_SWAP_HOST_TO_BENDIAN);
				value = shortBuf;
			}
			break;

		case 3:
			{
				int32 intBuf;
				//in->Read(&intBuf, size);
				fread(&intBuf, size, 1, in);
				//	Clear out last byte
				intBuf &= 0xFFFFFF00;
				swap_data(B_INT32_TYPE, &intBuf, sizeof(int32), B_SWAP_HOST_TO_BENDIAN);
				value = intBuf;
			}
			break;
			
		default:
			{
				//in->Read(&value, size);
				fread(&value, size, 1, in);
				swap_data(B_INT32_TYPE, &value, sizeof(int32), B_SWAP_HOST_TO_BENDIAN);
			}
			break;	
	}
			
   	return value;
}


//-------------------------------------------------------------------
//	ReadIntMsb
//-------------------------------------------------------------------
//
//

int32 ReadIntMsb(BFile *in, int size) 
{
	int32 value;
	
	if (size <= 0) 
		return 0;
		
	//	Clear out bits
	switch(size)
	{
		case 1:
			{
				char charBuf;
				in->Read(&charBuf, size);
				value = charBuf;
			}
			break;

		case 2:
			{
				int16 shortBuf;
				in->Read(&shortBuf, size);
				swap_data(B_INT16_TYPE, &shortBuf, sizeof(int16), B_SWAP_HOST_TO_BENDIAN);
				value = shortBuf;
			}
			break;

		case 3:
			{
				int32 intBuf;
				in->Read(&intBuf, size);
				//	Clear out last byte
				intBuf &= 0xFFFFFF00;
				swap_data(B_INT32_TYPE, &intBuf, sizeof(int32), B_SWAP_HOST_TO_BENDIAN);
				value = intBuf;
			}
			break;
			
		default:
			{
				in->Read(&value, size);
				swap_data(B_INT32_TYPE, &value, sizeof(int32), B_SWAP_HOST_TO_BENDIAN);
			}
			break;	
	}
			
   	return value;
}

//-------------------------------------------------------------------
//	ReadIntMsb
//-------------------------------------------------------------------
//
//

int32 ReadIntLsb(FILE *in, int size) 
{
	int32 value;
	
	if (size <= 0) 
		return 0;
		
	//	Clear out bits
	switch(size)
	{
		case 1:
			{
				char charBuf;
				//in->Read(&charBuf, size);
				fread(&charBuf, size, 1, in);
				value = charBuf;
			}
			break;

		case 2:
			{
				int16 shortBuf;
				//in->Read(&shortBuf, size);
				fread(&shortBuf, size, 1, in);
				swap_data(B_INT16_TYPE, &shortBuf, sizeof(int16), B_SWAP_HOST_TO_LENDIAN);
				value = shortBuf;
			}
			break;

		case 3:
			{
				int32 intBuf;
				//in->Read(&intBuf, size);
				fread(&intBuf, size, 1, in);
				//	Clear out last byte
				intBuf &= 0xFFFFFF00;
				swap_data(B_INT32_TYPE, &intBuf, sizeof(int32), B_SWAP_HOST_TO_LENDIAN);
				value = intBuf;
			}
			break;
			
		default:
			{
				//in->Read(&value, size);
				fread(&value, size, 1, in);
				swap_data(B_INT32_TYPE, &value, sizeof(int32), B_SWAP_HOST_TO_LENDIAN);
			}
			break;	
	}
			
   	return value;
}


//-------------------------------------------------------------------
//	ReadIntMsb
//-------------------------------------------------------------------
//
//

int32 ReadIntLsb(BFile *in, int size) 
{
	int32 value;
	
	if (size <= 0) 
		return 0;
		
	//	Clear out bits
	switch(size)
	{
		case 1:
			{
				char charBuf;
				in->Read(&charBuf, size);
				value = charBuf;
			}
			break;

		case 2:
			{
				int16 shortBuf;
				in->Read(&shortBuf, size);
				swap_data(B_INT16_TYPE, &shortBuf, sizeof(int16), B_SWAP_HOST_TO_LENDIAN);
				value = shortBuf;
			}
			break;

		case 3:
			{
				int32 intBuf;
				in->Read(&intBuf, size);
				//	Clear out last byte
				intBuf &= 0xFFFFFF00;
				swap_data(B_INT32_TYPE, &intBuf, sizeof(int32), B_SWAP_HOST_TO_LENDIAN);
				value = intBuf;
			}
			break;
			
		default:
			{
				in->Read(&value, size);
				swap_data(B_INT32_TYPE, &value, sizeof(int32), B_SWAP_HOST_TO_LENDIAN);
			}
			break;	
	}
			
   	return value;
}


//-------------------------------------------------------------------
//	BytesToIntMsb
//-------------------------------------------------------------------
//
//

int32 BytesToIntMsb(void *vBuff, int size) 
{
	unsigned char *buff = reinterpret_cast<unsigned char *>(vBuff);
	
	if (size <= 0) 
		return 0;
		
	int32 l = BytesToIntMsb(buff,size-1) << 8;
	
	l |= static_cast<int32>(buff[size-1]) & 255;
	
	return l;
}

//-------------------------------------------------------------------
//	BytesToIntMsb
//-------------------------------------------------------------------
//
//

int32 BytesToIntLsb(void *vBuff, int size) 
{
   unsigned char *buff = reinterpret_cast<unsigned char *>(vBuff);
   if (size <= 0) return 0;
   int32 l = static_cast<int32>(*buff) & 255;
   l |= BytesToIntLsb(buff+1,size-1)<<8;
   return l;
}


//-------------------------------------------------------------------
//	WriteInt16Lsb
//-------------------------------------------------------------------
//
//

ssize_t WriteInt16Lsb(FILE *theFile, uint16 data) 
{
	uint16 outValue = B_HOST_TO_LENDIAN_INT16(data);
	//ssize_t bytesWritten = theFile->Write(&outValue, sizeof(uint16));
	ssize_t bytesWritten = fwrite(&outValue, sizeof(uint16), 1, theFile);
	
	return bytesWritten;
}

//-------------------------------------------------------------------
//	WriteInt32Lsb
//-------------------------------------------------------------------
//
//

ssize_t WriteInt32Lsb(FILE *theFile, uint32 data) 
{
	uint32 outValue = B_HOST_TO_LENDIAN_INT32(data);
	//ssize_t bytesWritten = theFile->Write(&outValue, sizeof(uint32));
	ssize_t bytesWritten = fwrite(&outValue, sizeof(uint32), 1, theFile);
	
	return bytesWritten;

}


//-------------------------------------------------------------------
//	WriteInt32Msb
//-------------------------------------------------------------------
//
//

ssize_t WriteInt32Msb(FILE *theFile, uint32 data) 
{
	uint32 outValue = B_HOST_TO_BENDIAN_INT32(data);
	//ssize_t bytesWritten = theFile->Write(&outValue, sizeof(uint32));
	ssize_t bytesWritten = fwrite(&outValue, sizeof(uint32), 1, theFile);
	
	return bytesWritten;
}


//---------------------------------------------------------------------
//	DumpAVIHeader
//---------------------------------------------------------------------
//
//	

void DumpAVIHeader(AVIHeader *theHeader)
{
	PROGRESS("=== Start AVIHeader ===\n");
	PROGRESS("TimeBetweenFrames: %d\n", theHeader->TimeBetweenFrames);
	PROGRESS("MaximumDataRate: %d\n", theHeader->MaximumDataRate);
	PROGRESS("PaddingGranularity: %d\n", theHeader->PaddingGranularity);
	PROGRESS("Flags: %d\n", theHeader->Flags);        	
	PROGRESS("TotalNumberOfFrames: %d\n", theHeader->TotalNumberOfFrames);
	PROGRESS("NumberOfInitialFrames: %d\n", theHeader->NumberOfInitialFrames);
	PROGRESS("NumberOfStreams: %d\n", theHeader->NumberOfStreams);      
	PROGRESS("SuggestedBufferSize: %d\n", theHeader->SuggestedBufferSize);  
	PROGRESS("Width: %d\n", theHeader->Width);        		
	PROGRESS("Height: %d\n", theHeader->Height);       		
	PROGRESS("TimeScale: %d\n", theHeader->TimeScale);        	
	PROGRESS("DataRate: %d\n", theHeader->DataRate);         	
	PROGRESS("StartTime: %d\n", theHeader->StartTime);        	
	PROGRESS("DataLength: %d\n", theHeader->DataLength);
	PROGRESS("=== End AVIHeader ===\n\n\n");
}

//-------------------------------------------------------------------
//	DumpAVIStreamHeader
//-------------------------------------------------------------------
//
//

void DumpAVIStreamHeader(AVIStreamHeader *theHeader)
{		
	PROGRESS("=== Start AVIStreamHeader ===\n");    		
	//PROGRESS("DataType: %d\n", theHeader->DataType);   
	PROGRESS("DataType: ");   
	DumpRIFFID(theHeader->DataType);		
	//PROGRESS("DataHandler: %d\n", theHeader->DataHandler); 
	PROGRESS("DataHandler: ");   
	DumpRIFFID(theHeader->DataHandler); 		
	PROGRESS("Flags: %d\n", theHeader->Flags); 
	PROGRESS("Priority: %d\n", theHeader->Priority); 
	PROGRESS("InitialFrames: %d\n", theHeader->InitialFrames); 
	PROGRESS("TimeScale: %d\n", theHeader->TimeScale);   
	PROGRESS("DataRate: %d\n", theHeader->DataRate);    
	PROGRESS("StartTime: %d\n", theHeader->StartTime);  
	PROGRESS("DataLength: %d\n", theHeader->DataLength); 
	PROGRESS("SuggestedBufferSize: %d\n", theHeader->SuggestedBufferSize); 
	PROGRESS("Quality: %d\n", theHeader->Quality);      		
	PROGRESS("SampleSize: %d\n", theHeader->SampleSize); 
	PROGRESS("=== End AVIStreamHeader ===\n\n\n");
}


//-------------------------------------------------------------------
//	DumpVIDSHeader
//-------------------------------------------------------------------
//
//

void DumpVIDSHeader(AVIVIDSHeader *theHeader)
{		
	PROGRESS("=== Start AVIVIDSHeader ===\n");
	PROGRESS("Size: %d\n", theHeader->Size);    		
	PROGRESS("Width: %d\n", theHeader->Width); 
	PROGRESS("Height: %d\n", theHeader->Height); 
	PROGRESS("Planes: %d\n", theHeader->Planes); 
	PROGRESS("BitCount: %d\n", theHeader->BitCount); 
	//PROGRESS("Compression: %d\n", theHeader->Compression);   
	PROGRESS("Compression: ");   
	DumpRIFFID(theHeader->Compression);
	PROGRESS("ImageSize: %d\n", theHeader->ImageSize);    
	PROGRESS("XPelsPerMeter: %d\n", theHeader->XPelsPerMeter);  
	PROGRESS("YPelsPerMeter: %d\n", theHeader->YPelsPerMeter); 
	PROGRESS("Number of Colors: %d\n", theHeader->NumColors); 
	PROGRESS("Important Colors: %d\n", theHeader->ImpColors);
	PROGRESS("=== End AVIVIDSHeader ===\n\n\n");
}

//-------------------------------------------------------------------
//	DumpAUDSHeader
//-------------------------------------------------------------------
//
//

void DumpAUDSHeader(AVIAUDSHeader *theHeader)
{		
	PROGRESS("=== Start AVIAUDSHeader ===\n");
	PROGRESS("Format: %d\n", theHeader->Format);    		
	PROGRESS("Channels: %d\n", theHeader->Channels); 
	PROGRESS("SamplesPerSec: %d\n", theHeader->SamplesPerSec);
	PROGRESS("AvgBytesPerSec: %d\n", theHeader->AvgBytesPerSec);
	PROGRESS("BlockAlign: %d\n", theHeader->BlockAlign);
	PROGRESS("BitsPerSample: %d\n", theHeader->BitsPerSample);
	PROGRESS("ExtensionSize: %d\n", theHeader->ExtensionSize);
	PROGRESS("SamplesPerBlock: %d\n", theHeader->SamplesPerBlock);
	PROGRESS("NumCoefficients: %d\n", theHeader->NumCoefficients);
	//WaveCoefficient *Coefficients
	PROGRESS("Style: %d\n", theHeader->Style); 
	PROGRESS("ByteCount: %d\n", theHeader->ByteCount);
	PROGRESS("=== End AVIAUDSHeader ===\n\n\n");
}


//-------------------------------------------------------------------
//	DumpRIFFID
//-------------------------------------------------------------------
//
//	Print RIFF ID ie: 'RIFF'
//

void DumpRIFFID(int32 theID)
{ 
	PROGRESS("%c",     (char)((theID >> 24) & 0xff)   );
	PROGRESS("%c",     (char)((theID >> 16) & 0xff)   );
	PROGRESS("%c",     (char)((theID >>  8) & 0xff)   );
	PROGRESS("%c(%x)", (char) (theID        & 0xff), theID);
	PROGRESS("\n");
}

//-------------------------------------------------------------------
//	DumpAVIHeaderFlags
//-------------------------------------------------------------------
//
//	Dump bit flag setting in AVIHeader
//

void DumpAVIHeaderFlags(AVIHeader *theHeader)
{ 
	PROGRESS(" AVI flags: ");
	if (theHeader->Flags & kAVIHasIndexFlag) 		PROGRESS("Has_Index ");		
	if (theHeader->Flags & kAVIMustUseIndexFlag) 	PROGRESS("Use_Index ");
	if (theHeader->Flags & kAVIIsInterleavedFlag) 	PROGRESS("Interleaved ");
	if (theHeader->Flags & kAVIWasCaptureFileFlag) 	PROGRESS("Captured ");
	if (theHeader->Flags & kAVICopyrightedFlag) 	PROGRESS("Copyrighted ");
	PROGRESS("\n");
}

//-------------------------------------------------------------------
//	DumpAudioType
//-------------------------------------------------------------------
//
//	Dump bit flag setting in AVIHeader
//

void DumpAudioType(uint16 type)
{
	switch(type)
	{
		case WAVE_FORMAT_PCM: PROGRESS("PCM"); break;
		case WAVE_FORMAT_ADPCM: PROGRESS("MS ADPCM"); break;
		case WAVE_FORMAT_DVI_ADPCM: PROGRESS("DVI ADPCM"); break;
		case WAVE_FORMAT_ALAW: PROGRESS("ALAW"); break;
		case WAVE_FORMAT_MULAW: PROGRESS("ULAW"); break;
		case WAVE_FORMAT_OKI_ADPCM: PROGRESS("OKI_ADPCM"); break;
		case IBM_FORMAT_MULAW: PROGRESS("IBM_ULAW"); break;
		case IBM_FORMAT_ALAW: PROGRESS("IBM_ALAW"); break;
		case IBM_FORMAT_ADPCM: PROGRESS("IBM_ADPCM"); break;
		case WAVE_FORMAT_GSM610: PROGRESS("GSM 6.10"); break;
		case WAVE_FORMAT_DSP_TRUESPEECH: PROGRESS("DSP TrueSpeech"); break;
		default: PROGRESS("Unknown(%x)",type); break;
	}
}
