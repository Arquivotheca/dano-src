// ===========================================================================
//	Translation.cpp
//  Copyright 1999 by Be Incorporated.
// ===========================================================================

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "Translation.h"
#include "NPApp.h"
#include "UResource.h"

#define kGIF87Stamp  "GIF87a"
#define kGIF89Stamp  "GIF89a" 
#define kTranslationBuffSize 1024

GifTranslator::GifTranslator()
{
	translatorName = "NP GIF Translator\0";
	translatorInfo = "Version 1.0\0";
	translatorVersion = 100;

	inputFormats[0].type = B_GIF_FORMAT;
	inputFormats[0].group = B_TRANSLATOR_BITMAP;
	inputFormats[0].quality = 1.0;
	inputFormats[0].capability = 1.0;
	strncpy(inputFormats[0].MIME, "image/gif\0", 10);
	strncpy(inputFormats[0].name, "GIF Image\0", 10);
	
//	inputFormats[1].type = B_TRANSLATOR_BITMAP;
//	inputFormats[1].group = B_TRANSLATOR_BITMAP;
//	inputFormats[1].quality = 1.0;
//	inputFormats[1].capability = 1.0;
//	strncpy(inputFormats[1].MIME, "image/x-be-bitmap\0", 18);
//	strncpy(inputFormats[1].name, "Be Bitmap\0", 10);

	inputFormats[1].type = 0;
	inputFormats[1].group = 0;
	inputFormats[1].quality = 0;
	inputFormats[1].capability = 0;
	strncpy(inputFormats[1].MIME, "\0", 1);
	strncpy(inputFormats[1].name, "\0", 1);
	
//	outputFormats[0].type = B_GIF_FORMAT;
//	outputFormats[0].group = B_TRANSLATOR_BITMAP;
//	outputFormats[0].quality = 1.0;
//	outputFormats[0].capability = 1.0;
//	strncpy(outputFormats[0].MIME, "image/gif\0", 10);
//	strncpy(outputFormats[0].name, "GIF Image\0", 10);
	
	outputFormats[0].type = B_TRANSLATOR_BITMAP;
	outputFormats[0].group = B_TRANSLATOR_BITMAP;
	outputFormats[0].quality = 1.0;
	outputFormats[0].capability = 1.0;
	strncpy(outputFormats[0].MIME, "image/x-be-bitmap\0", 18);
	strncpy(outputFormats[0].name, "Be Bitmap\0", 10);

	outputFormats[1].type = 0;
	outputFormats[1].group = 0;
	outputFormats[1].quality = 0;
	outputFormats[1].capability = 0;
	strncpy(outputFormats[1].MIME, "\0", 1);
	strncpy(outputFormats[1].name, "\0", 1);
}

//status_t
//GifTranslator::BitmapToGIF(BPositionIO *inData, BPositionIO *outData,
//	const translator_info *inInfo)
//{
//	return B_OK;
//}

status_t
GifTranslator::GIFToBitmap(BPositionIO *inData, BPositionIO *outData, const translator_info *,
	uint32 outType)
{
	uchar buff[kTranslationBuffSize];
	uchar tempBuff[kTranslationBuffSize];
	uchar *workingBuff = buff;
	BePixels pixels(NetPositive::MainScreenColorSpace() == B_COLOR_8_BIT);
	GIF gif;
	int imageIndex = 0;
	int numBytes = 0;
	int numRead = 0;
	long writeReturn = 0;
	long totalWritten = 0;
		
	numBytes = inData->Read(buff, kTranslationBuffSize);
	writeReturn = gif.Write(buff, numBytes, &pixels);
	totalWritten = writeReturn;
	//while there is data to be put in buff, translate and write it
	outData->Seek(0, SEEK_SET);
	while(writeReturn >= 0  && numBytes != 0){
		if(gif.GetImageIndex() != imageIndex){ //at end of current frame
			if(imageIndex >= 1){
				BBitmap *bits = pixels.GetBBitmap();
				BMessage bitmapMessage;
				uint32 delay = gif.GetDelay();
				bitmapMessage.AddInt32("delay", delay);
				bits->Archive(&bitmapMessage);
				bitmapMessage.Flatten(outData);
			}
			imageIndex++;
		}
		if(writeReturn > 0){
			workingBuff += writeReturn;
			numBytes -= writeReturn;
			if(workingBuff > (buff + kTranslationBuffSize/2)){ //if the buffer is but half full, shift and reload
				memcpy(tempBuff, workingBuff, numBytes);
				workingBuff = buff;
				memcpy(buff, tempBuff, 	numBytes);
				numRead = inData->Read(buff + numBytes, (kTranslationBuffSize - numBytes)); 
				numBytes += numRead;
			}
		}
		writeReturn = gif.Write(workingBuff, numBytes, &pixels);
		totalWritten += writeReturn;
	}
	BBitmap *bits = pixels.GetBBitmap();
	BMessage bitmapMessage;
	uint32 delay = gif.GetDelay();
	bitmapMessage.AddInt32("delay", delay);
	//the last bitmap message has the loop bool
	if(gif.LoopAnimation())
		bitmapMessage.AddBool("loop", true);
	else
		bitmapMessage.AddBool("loop", false);
		
	bits->Archive(&bitmapMessage);
	bitmapMessage.Flatten(outData);
	if(writeReturn >= 0)
		return B_OK;
	else
		return writeReturn;
}

//status_t
//GifTranslator::GIFToGIF(BPositionIO *inData, BPositionIO *outData,
//	const translator_info *inInfo)
//{
//	return B_OK;
//}

bool
GifTranslator::CanHandle(uint32 inType, uint32 outType)
{
//PRINT(("in type %x, out type %x, "
//	"gif type %x, translatorType %x\n",
//	inType, outType, kNativeType, B_TRANSLATOR_BITMAP));

	switch (inType) {
//		case B_TRANSLATOR_BITMAP:
//			return outType == 0 || outType == B_GIF_FORMAT;

		case B_GIF_FORMAT:
			return outType == 0
				|| outType == B_TRANSLATOR_BITMAP
				|| outType == B_GIF_FORMAT;	
	}
	
	return false;
}

bool
GifTranslator::GuessType(BPositionIO *inData, uint32 &type)
{
	char bytes[16];

	if (inData->Read(&bytes, sizeof(bytes)) != sizeof(bytes))
		return false;
	
	if ((*(TranslatorBitmap *)&bytes).magic
			== B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP)) {
		type = B_TRANSLATOR_BITMAP;
		return true;
	}
	

	if (strncmp(bytes, kGIF87Stamp, 6) == 0
		|| strncmp(bytes, kGIF89Stamp, 6) == 0) {
		type = B_GIF_FORMAT;
		return true;
	}

	return false;
}

status_t
GifTranslator::Identify(BPositionIO *inData, const translation_format *inFormat,
	BMessage *configMessage, translator_info *outInfo, uint32 outType)
{
	uint32 inType;
	bool guessed = false;
	
	if (inFormat) 
		inType = inFormat->type;
	else if (GuessType(inData, inType))
		guessed = true;
	else
		return B_NO_TRANSLATOR;

	/* make sure we can handle the type */
	if (!CanHandle(inType, outType))
		return B_NO_TRANSLATOR;

	// should check header here
	
	outInfo->type = B_TRANSLATOR_BITMAP;
	
	outInfo->type = inType;
	int32 i = 0;
	while (inputFormats[i].type != 0) {
		if (inputFormats[i].type == outInfo->type)
			break;
		i++;
	}

	outInfo->translator	= 0;
	outInfo->group = inputFormats[i].group;
	outInfo->quality = inputFormats[i].quality;
	outInfo->capability	= inputFormats[i].capability;
	strcpy(outInfo->name, inputFormats[i].name);
	strcpy(outInfo->MIME, inputFormats[i].MIME);

	return B_OK;
}

status_t
GifTranslator::Translate(BPositionIO *inData, const translator_info *inInfo,
	BMessage *configMessage, uint32 outType, BPositionIO *outData)
{
	if (!inInfo || !CanHandle(inInfo->type, outType))
		return B_NO_TRANSLATOR;

	if (outType == 0) {
		if (inInfo->type == B_GIF_FORMAT)
			outType = B_TRANSLATOR_BITMAP;
		else
			outType = B_GIF_FORMAT;
	}
	
	if (inInfo->type == B_GIF_FORMAT && outType == B_TRANSLATOR_BITMAP)
		return GIFToBitmap(inData, outData, inInfo, outType);
//	else if (inInfo->type == B_TRANSLATOR_BITMAP && outType == B_GIF_FORMAT)
//		return BitmapToGIF(inData, outData, inInfo);
//	else
//		return GIFToGIF(inData, outData, inInfo);

	return B_NO_TRANSLATOR;
}


Animation::Animation(BPositionIO *rawImageStream)
{
	mNumFrames = 0;
	BMallocIO bitmapIO;
	BBitmap *outMap = NULL;
	gTranslatorRoster->Translate(rawImageStream, NULL, NULL, &bitmapIO, B_TRANSLATOR_BITMAP);
	bitmapIO.Seek(0, SEEK_SET);
	uint32 delayMess1 = 0;
	while(true) {
		BMessage outMess;
		outMess.Unflatten(&bitmapIO);

		outMess.FindInt32("delay", (int32*)(&delayMess1));

		bigtime_t * bigDelay = new bigtime_t(delayMess1 * 10000);
		mDelayList.AddItem(bigDelay);

		if(validate_instantiation(&outMess, "BBitmap")){
			outMap = (BBitmap *)(BBitmap::Instantiate(&outMess));
			if(outMap != NULL)
				mFrameList.AddItem(outMap);
		}
		else
			break;
		
		mNumFrames++;

		//only the last bitmap has the loop attribute, so quit the loop if it exists
		if(outMess.FindBool("loop", &mLoops) == B_OK)
			break;			
	}
}


Animation::~Animation()
{
	while (mFrameList.CountItems()) {
		delete (BBitmap *)mFrameList.ItemAt(0);
		mFrameList.RemoveItem((int32)0);
	}
	while (mDelayList.CountItems()) {
		delete (bigtime_t *)mDelayList.ItemAt(0);
		mDelayList.RemoveItem((int32)0);
	}
}


BBitmap *Animation::GetFrame(uint32 frame) const
{
	return (BBitmap *)mFrameList.ItemAt(frame);
}


bigtime_t Animation::GetDelay(uint32 frame) const
{
	return *((bigtime_t *)mDelayList.ItemAt(frame));
}


uint32 Animation::NumFrames() const
{
	return mNumFrames;
}

bool Animation::Loops() const
{
	return mLoops;
}

BBitmap *TranslateGIF(const char *url)
{
	ResourceIO resIO(url);
	Animation animation(&resIO);
	if (animation.NumFrames() == 0)
		return NULL;
		
	BBitmap *bitmap = new BBitmap(animation.GetFrame(0));
	return bitmap;
}


// Example usage:
//	BFile bootGif;
//	if(bootGif.SetTo("/var/tmp/Desktop/boot.gif", B_READ_ONLY) == B_OK){
//		BMallocIO bitmapIO;
//		BList bitmapList;
//		BBitmap *outMap = NULL;
//		gTranslatorRoster->Translate(&bootGif, NULL, NULL, &bitmapIO, B_TRANSLATOR_BITMAP);
//		bitmapIO.Seek(0, SEEK_SET);
//		uint32 delayMess1 = 0;
//		bool isLooping = false;
//		while(true) {
//			BMessage outMess;
//			outMess.Unflatten(&bitmapIO);
//
//			outMess.FindInt32("delay", (int32*)(&delayMess1));
//			printf("Got %u for delay\n", delayMess1);
//			//in real life, we would save this somewhere
//
//			if(validate_instantiation(&outMess, "BBitmap")){
//				outMap = (BBitmap *)(BBitmap::Instantiate(&outMess));
//				if(outMap != NULL)
//					bitmapList.AddItem(outMap);
//			}
//
//			//only the last bitmap has the loop attribute, so quit the loop if it exists
//			if(outMess.FindBool("loop", &isLooping) == B_OK)
//				break;			
//
//		}
//		printf("Got %i bitmaps into bitmapList\n", bitmapList.CountItems());
//		//shouldn't leave the bitmap hanging around
//		while(bitmapList.CountItems() > 0){
//			int zero = 0;
//			BBitmap *toDie = (BBitmap *)bitmapList.RemoveItem(zero);
//			delete toDie;
//		}			
//	} else {
//		printf("Couldn't open boot Gif\n");
//		fflush(stdout);
//	}		
//
