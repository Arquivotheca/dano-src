// ===========================================================================
//	Translation.h
//  Copyright 1999 by Be Incorporated.
// ===========================================================================

#ifndef __TRANSLATIONH__
#define __TRANSLATIONH__

#include <Debug.h>
#include <TranslatorAddOn.h>
#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <DataIO.h>
#include <List.h>
#include <Message.h>
#include <Bitmap.h>

#include "GIF.h"
#include "MessageWindow.h"
#include "Translation.h"
#include "BeDrawPort.h"

BBitmap *TranslateGIF(const char *url);

class GifTranslator : public BTranslator {
public:
	GifTranslator();

	virtual const char * TranslatorName() const{ return translatorName; }
	virtual const char * TranslatorInfo() const { return translatorInfo; }
	virtual int32 TranslatorVersion() const { return translatorVersion; }
	virtual const translation_format * InputFormats(int32 * out_count) const { *out_count = 3; return inputFormats; }
	virtual const translation_format * OutputFormats(int32 * out_count) const { *out_count = 3; return outputFormats; }		
	virtual status_t Identify(BPositionIO * inData,
								const translation_format * inFormat,
								BMessage *configMessage,
								translator_info * outInfo, uint32 outType);

	virtual status_t Translate(BPositionIO * inData,
								const translator_info * inInfo,
								BMessage *configMessage,
								uint32 outType,
								BPositionIO * outData);
									
									
private:
//	status_t 	BitmapToGIF(BPositionIO *inData, BPositionIO *outData,
//									const translator_info *inInfo);
	status_t	GIFToBitmap(BPositionIO *inData, BPositionIO *outData,
									const translator_info *,
									uint32 outType);
//	status_t 	GIFToGIF(BPositionIO *inData, BPositionIO *outData,
//									const translator_info *inInfo);
	bool 		CanHandle(uint32 inType, uint32 outType);
	bool		GuessType(BPositionIO *inData, uint32 &type);



	char *translatorName;
	char *translatorInfo;
	int32 translatorVersion;							
	translation_format inputFormats[3];
	translation_format outputFormats[3];
};


class Animation {
public:
	Animation(BPositionIO *rawImageStream);		// The constructor blocks until the entire image is translated.
	~Animation();
	
	uint32		NumFrames() const;
	bool		Loops() const;
	
	BBitmap*	GetFrame(uint32 frame) const;	// 0-based
	bigtime_t	GetDelay(uint32 frame) const;	// 0-based.  This is how long you should wait before displaying
												// frame <n>, having already displayed frame <n-1>.  The delay
												// for frame 0 will be 0.
	
protected:
	BList		mFrameList;
	BList		mDelayList;
	bool		mLoops;
	uint32		mNumFrames;
};
	

#endif
