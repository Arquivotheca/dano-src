// ===========================================================================
//	SaveText.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "SaveText.h"
#include "DrawPort.h"
#include "UResource.h"
#include "HTMLDoc.h"

#include <malloc.h>
#include <stdio.h>
#include <ctype.h>

#define kTextCharWidth 8
#define kTextCharHeight 12

// ===========================================================================
//	DrawPort that renders into a text buffer

class TextDrawPort : public DrawPort {
public:
					TextDrawPort();
					~TextDrawPort();
					
	virtual	void	SetStyle(Style &style);
	virtual	float		TextWidth(const char *text, long textCount);
	virtual	void	DrawText(float h, float v, const char *text, long textCount, float width, bool hasComplexBG = true);
	virtual	void	DrawRule(BRect *r, bool noShade);

	virtual	float		BlankLineHeight();
	
			void	SetupBuffer(BRect *r);
			void	WriteBuffer(UResourceImp *dst);
			
protected:
			long	mTextTop;
			long	mTextRows;
			long	mTextCols;
			long	mMaxRow;
			char*	mTextBuffer;
			short	mTabWidth;
};

// ===========================================================================

TextDrawPort::TextDrawPort()
{
	mFontAscent = 8;
	mFontDescent = 4;
	mTextBuffer = 0;
	mTextRows = mTextCols = mTextTop = mMaxRow = 0;
	mTabWidth = 0;
}

TextDrawPort::~TextDrawPort()
{
	if (mTextBuffer)
		free(mTextBuffer);
}

//	Allocate a page of text to draw into, base size on pixel rect

void	TextDrawPort::SetupBuffer(BRect *r)
{
	if (mTextBuffer)
		free(mTextBuffer);
		
	mTextCols = (long)((r->right + kTextCharWidth/2)/kTextCharWidth);
	mTextRows = (long)((r->bottom - r->top + (kTextCharHeight/2))/kTextCharHeight);
	mTextTop = (long)(r->top/kTextCharHeight);
	mTextBuffer = (char *)malloc(mTextCols * mTextRows);
	mMaxRow = 0;
	if (mTextBuffer)
		memset(mTextBuffer,' ',mTextCols * mTextRows);
}

//	Write the buffer to a disk file, one line at a time

void	TextDrawPort::WriteBuffer(UResourceImp *dst)
{
	if (mTextBuffer == 0) return;
//	NP_ASSERT(dst);
	
	char str[1024];
	for (short i = 0; i < mMaxRow; i++) {
		char *s = mTextBuffer + i*mTextCols;
		short lastNonSpace = 0;
		
		for (short j = 0; j < mTextCols; j++) {
			str[j] = s[j];
			if (!isspace(s[j]))
				lastNonSpace = j;
		}
		str[lastNonSpace + 1] = 0;	// Trim trailing spaces
		strcat(str,"\r\n");			// Newline,cr
		
		dst->Write(str,strlen(str));
	}
}

void	TextDrawPort::SetStyle(Style &)
{
}

float		TextDrawPort::TextWidth(const char *, long textCount)
{
	return textCount*kTextCharWidth;
}

void	TextDrawPort::DrawText(float h, float v, const char *text, long textCount, float, bool)
{
	if (mTextBuffer == 0) return;
	
	h -= 8;	// Rootglyph in document adds a little bit of space down the left edge
			// remove it so we dont get spaces at the start of each line!
	
	h /= kTextCharWidth;
	v /= kTextCharHeight;
	
//	NP_ASSERT(h >= 0);
	if (v < mTextTop) return;					// Off bottom
	if (v >= mTextTop + mTextRows) return;		// Off Top	
	textCount = (long)(MIN(textCount,mTextCols - h));	// Off right
	if (textCount <= 0) return;
	
	mMaxRow = (long)(MAX(mMaxRow,v - mTextTop));
	char *d = mTextBuffer + (mTextCols * (((long)(v)) - mTextTop)) + ((long)(h));
	memcpy(d,text,textCount);					// Draw into text buffer
}

//	Draw rule as solid line

void TextDrawPort::DrawRule(BRect *r, bool)
{
	long width = (long)((r->right - r->left)/kTextCharWidth);
	char *rule = (char *)malloc(width);
	if (rule == 0) return;
	memset(rule,'-',width);
	DrawText(r->left,r->top,rule,width,0);
	free(rule);
}

float	 TextDrawPort::BlankLineHeight()
{
	return kTextCharHeight;
}

// ===========================================================================
//	Create a new resource from the html one

UResourceImp* SaveHTMLAsText(UResourceImp *src)
{
	int	cols = 80;	// Format to 80 chars wide
	int	rows = 120;	// 120 lines on a 'page'
	
//	create a txt version of this url

	char url[1024];
	strcpy(url,src->GetURL());
	url[strlen(url)-4] = 0;
	strcat(url,".txt");
	UResourceImp* dst = new UResourceImp(url,NULL);
	dst->SetContentType("text/plain");
	
	TextDrawPort *drawPort = new(TextDrawPort);
#ifdef JAVASCRIPT
	Document *doc = Document::CreateFromResource(NULL, src,drawPort,0,0,0, true, NULL, NULL, NULL, NULL);
#else
	Document *doc = Document::CreateFromResource(NULL, src,drawPort,0,0,0, true, NULL, NULL, NULL);
#endif
	if (doc == 0) {
		delete(drawPort);
		return NULL;
	}
	doc->ShowImages(false);
	doc->ShowBGImages(false);
	
//	Create a rectangle that is the size of a 'page'

	long pageHeight = rows*kTextCharHeight;
	long pageWidth = cols*kTextCharWidth;
	
	BRect r,pageR;
	pageR.left = pageR.top = 0;
	pageR.right = pageWidth;
	pageR.bottom = pageHeight;
	
	doc->Layout(pageWidth);
	
//	Idle until layout is complete ?

	while (!doc->IsParsingComplete()) {
		r = pageR;
		doc->Idle(&r);
	}
	float totalHeight = doc->GetHeight();
	
//	Write a page at a time to the destination resource

	short page = 1;
	do {
		
//		Write the actual page

		r = pageR;
		drawPort->SetupBuffer(&r);
		doc->Draw(&r);
		drawPort->WriteBuffer(dst);
		
		pageR.OffsetBy(0,pageHeight);
		pageR.bottom = MIN(pageR.bottom,totalHeight);

//		Write a page footer, if you like

		if (0) {
			char header[1024];
			sprintf(header,"\r\n[%s Page %2d]\r\n",src->GetURL(),page);
			dst->Write(header,strlen(header));
		}
		page++;
	} while (pageR.top < totalHeight);
	doc->Dereference();
//	delete doc;
	
//	return new UResource(dst);
	dst->RefCount(1);
	return dst;
}
