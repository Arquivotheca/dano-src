// ============================================================
//  BottomlineInput.cpp	by Hiroshi Lockheimer
// ============================================================

#include "CStyledEditApp.h"	// need this for SOKYO_TUBWAY

#ifdef SOKYO_TUBWAY

#include <Debug.h>
#include <malloc.h>
#include <string.h>
#include <Application.h>
#include <Bitmap.h>
#include <FindDirectory.h>
#include <Path.h>
#include "BottomlineInput.h"
#include "CStyledEditWindow.h"
#include "mappings.h"


const float	kLeftOffset = 4.0;
const float	kVOffset = 2.0;


#define convert_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|((*uni_str++)&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|((*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}	

inline bool
is_initial_sjis_byte(uchar c)
{	
	return ( ((c >= 0x81) && (c <= 0x9F)) || 
			 ((c >= 0xE0) && (c <= 0xEF)) ); 
}

inline bool
is_euc(uchar c)	
	{ return ((c >= 161) && (c <= 254)); }

inline bool
is_hankata(uchar c)	
	{ return ((c >= 161) && (c <= 223)); }

const uchar	kSS2 = 142;


char*	euc_to_utf8(const Wchar *euc, int32 *len);
int32	mbs_to_wcs(Wchar *dest, uchar *src, int32 destLen);


char*
euc_to_utf8(const Wchar *euc, int32 *len)
{
	char	*result = NULL;
	int32	resultLen = 0;

	for (int32 i = 0; i < *len; i++) {
		int e1 = (euc[i] >> 8) & 0xFF;
		int e2 = euc[i] & 0xFF;

		if (is_euc(e1)) {
			if (is_euc(e2)) {
				e1 -= 128;
				e2 -= 128;

				uchar	c1 = e1;
				uchar	c2 = e2;
				int		rowOffset = (c1 < 95) ? 112 : 176;
				int		cellOffset = (c1 % 2) ? ((c2 > 95) ? 32 : 31) : 126;

				e1 = ((c1 + 1) >> 1) + rowOffset;
				e2 += cellOffset;
			}
		}
		else {
			if (e1 == kSS2) {
				if (is_hankata(e2))
					e1 = 0;
			}
		}

		uint16	sjisChar = (e1 << 8) | (e2 & 0xFF);
		uint16	unicode = sjistou[sjisChar];
		uint16	*UNICODE = &unicode;
		uchar	utf8[4] = "";
		uchar	*UTF8 = utf8;

		convert_to_utf8(UTF8, UNICODE);

		result = (char *)realloc(result, resultLen + UTF8 - utf8);
		memcpy(result + resultLen, utf8, UTF8 - utf8);
		resultLen += UTF8 - utf8;
	}

	*len = resultLen;
	return (result);
}


int32
mbs_to_wcs(Wchar *dest, uchar *src, int32 destlen)
{
	register int32			i, j;
	register unsigned char	ec;
 
	for (i = 0, j = 0; (ec = src[i]) && (j < destlen); i++) {
		if (ec & 0x80) {
			switch (ec) {
				case 0x8e: /* SS2 */
					dest[j++] = (Wchar)(0x80 | ((unsigned)src[++i] & 0x7f));
					break;
	
				case 0x8f: /* SS3 */
					dest[j++] = (Wchar)(0x8000 | 
										(((unsigned)src[i + 1] & 0x7f) << 8) |
										((unsigned)src[i + 2] & 0x7f));
					i += 2;
					break;

				default:
					dest[j++] = (Wchar)(0x8080 | 
										(((unsigned)src[i] & 0x7f) << 8) | 
										((unsigned)src[i + 1] & 0x7f));
					i++;
					break;
			}
		}
		else
			dest[j++] = (Wchar)ec;
	}
    
	if (j < destlen)
		dest[j] = (Wchar)0;
 
	return (j);
}


TBottomlineView::TBottomlineView(
	BRect	frame)
		: BView(frame, B_EMPTY_STRING, B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	BPath path;
	find_directory(B_USER_DIRECTORY, &path);

	path.Append("rkdic/");
	fContext = RkwInitialize(path.Path());
	if (fContext < 0)
		printf("RkwInitialize failed!\n");

	path.Append("just.rdic");
	fRomaDic = RkOpenRoma(path.Path());
	if (fRomaDic == NULL)
		printf("RkOpenRoma failed!\n");

	RkwSetDicPath(fContext, "user");
	RkwMountDic(fContext, "chimei", PL_ALLOW);
	RkwMountDic(fContext, "katakana", PL_ALLOW);
	RkwMountDic(fContext, "user", PL_ALLOW);
	RkwSetDicPath(fContext, "canna");
	RkwMountDic(fContext, "bushu", PL_ALLOW);
	RkwMountDic(fContext, "iroha",PL_ALLOW);

	fRoman = NULL;
	fRomanLen = 0;

	fWide = NULL;
	fWideLen = 0;

	fUTF8 = NULL;
	fUTF8Len = 0;

	fBunWidths = NULL;
	fNumBuns = 0;
	fCurBun = 0;

	fTarget = NULL;

	BRect bitmapRect = Bounds();
	bitmapRect.OffsetTo(B_ORIGIN);
	fBitmap = new BBitmap(bitmapRect, B_COLOR_8_BIT, TRUE);
	fBitmap->Lock();
	fBitmap->AddChild(new BView(bitmapRect, B_EMPTY_STRING, 0, 0));
	fBitmap->Unlock();
}


TBottomlineView::~TBottomlineView()
{
	free(fRoman);
	free(fWide);
	free(fUTF8);
	free(fBunWidths);
	delete(fBitmap);

	RkCloseRoma(fRomaDic); 
	RkwUnmountDic(fContext, "iroha");
	RkwUnmountDic(fContext, "hojomwd");
	RkwUnmountDic(fContext, "hojoswd");
	RkwUnmountDic(fContext, "bushu");
	//RkwUnmountDic(fContext, "suffix");
	RkwUnmountDic(fContext, "chimei");
	RkwUnmountDic(fContext, "katakana");
	RkwUnmountDic(fContext, "user");
	RkwFinalize();
}


void
TBottomlineView::Draw(
	BRect	update)
{
	DrawKouhoString();
}


void
TBottomlineView::KeyDown(
	const char	*bytes,
	int32		numBytes)
{
	be_app->ObscureCursor();

	uchar theChar = bytes[0];

	switch (theChar) {
		case B_BACKSPACE:
			HandleBackspace();
			break;
			
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			HandleArrowKey(theChar);
			break;
		
		case B_DELETE:
		case B_HOME:
		case B_END:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
			// nothing
			break;

		case B_SPACE:
			HandleKouho();
			break;

		case B_RETURN:
			HandleKakutei();
			break;

		default:
			HandleRomanKey(bytes, numBytes);				
			break;
	}	
}


void
TBottomlineView::SetTarget(
	BHandler	*target)
{
	BWindow *window = Window();

	if (window != NULL) {
		if (!window->Lock())
			return;

		fTarget = target;

		window->Unlock();
	}
}


void			
TBottomlineView::HandleBackspace()
{
	if ((fNumBuns > 0) || (fRomanLen < 1))
		return;

	fRoman = (char *)realloc(fRoman, fRomanLen);
	fRomanLen--;
	fRoman[fRomanLen] = '\0';

	char *kana = (char *)malloc((fRomanLen * 2) + 1);
	
	int32 len = RkCvtRoma(fRomaDic, 
						  (uchar *)kana, (fRomanLen * 2) + 1, 
						  (uchar *)fRoman, fRomanLen + 1, 
						  RK_XFER | RK_SOKON | RK_FLUSH);
	
	free(fWide);
	fWide = NULL;
	fWide = (Wchar *)malloc(len * sizeof(Wchar));
	fWideLen = mbs_to_wcs(fWide, (uchar *)kana, len);

	free(fUTF8);
	fUTF8 = NULL;
	fUTF8Len = fWideLen;
	fUTF8 = euc_to_utf8(fWide, &fUTF8Len);

	DrawKouhoString();

	free(kana);	
}


void
TBottomlineView::HandleArrowKey(
	uint32	inArrowKey)
{
	if (fNumBuns < 1)
		return; 

	bool		shiftDown = FALSE;
	BMessage	*message = Window()->CurrentMessage();
	int32		modifiers = 0;
	if (message->FindInt32("modifiers", &modifiers) == B_NO_ERROR)
		shiftDown = modifiers & B_SHIFT_KEY;

	switch (inArrowKey) {
		case B_LEFT_ARROW:
			if (shiftDown)
				fNumBuns = RkwShorten(fContext);
			else
				fCurBun = RkwLeft(fContext);
			break;

		case B_RIGHT_ARROW:
			if (shiftDown)
				fNumBuns = RkwEnlarge(fContext);
			else
				fCurBun = RkwRight(fContext);
			break;

		case B_UP_ARROW:
			RkwNext(fContext);
			break;
		
		case B_DOWN_ARROW:
			RkwPrev(fContext);
			break;
	}

	ResetKouhoString();
	DrawKouhoString();
}


void
TBottomlineView::HandleKouho()
{
	if (fNumBuns > 0)
		RkwNext(fContext);
	else {
//		int mode = RK_HENKANMODE(RK_TANBUN |
//								 RK_MAKE_WORD |
//								 RK_MAKE_EISUUJI |
//								 RK_MAKE_KANSUUJI) << (2 * RK_XFERBITS);
		int mode = RK_XFER << RK_XFERBITS | RK_KFER;
//		mode = 0;

		fCurBun = 0;
		fNumBuns = RkwBgnBun(fContext, fWide, fWideLen, mode);

		if (fNumBuns < 1)
			return;

		//RkwResize()
	}

	ResetKouhoString();
	DrawKouhoString();
}


void
TBottomlineView::HandleKakutei()
{
	if ((fUTF8 != NULL) && (fUTF8Len > 0) && (fTarget != NULL)) {
		char *kanji = (char *)malloc(fUTF8Len + 1);
		memcpy(kanji, fUTF8, fUTF8Len);
		kanji[fUTF8Len] = '\0';

		BMessage kanjiDown('KNJI');
		kanjiDown.AddString("kanji", kanji);
		fTarget->Looper()->PostMessage(&kanjiDown, fTarget);

		free(kanji);
	}

	RkwEndBun(fContext, 1);

	free(fBunWidths);
	fBunWidths = NULL;
	fNumBuns = 0;
	fCurBun = 0;

	free(fRoman);
	fRoman = NULL;
	fRomanLen = 0;

	free(fWide);
	fWide = NULL;
	fWideLen = 0;

	free(fUTF8);
	fUTF8 = NULL;
	fUTF8Len = 0;

	DrawKouhoString();
}


void
TBottomlineView::HandleRomanKey(
	const char	*bytes, 
	int32		numBytes)
{
	if (fNumBuns > 0)
		HandleKakutei();

	fRoman = (char *)realloc(fRoman, fRomanLen + numBytes + 1);
	memcpy(fRoman + fRomanLen, bytes, numBytes);
	fRomanLen += numBytes;
	fRoman[fRomanLen] = '\0';

	char *kana = (char *)malloc((fRomanLen * 2) + 1);
	
	int32 len = RkCvtRoma(fRomaDic, 
						  (uchar *)kana, (fRomanLen * 2) + 1, 
						  (uchar *)fRoman, fRomanLen + 1, 
						  RK_XFER | RK_SOKON | RK_FLUSH);
	
	free(fWide);
	fWide = NULL;
	fWide = (Wchar *)malloc(len * sizeof(Wchar));
	fWideLen = mbs_to_wcs(fWide, (uchar *)kana, len);

	free(fUTF8);
	fUTF8 = NULL;
	fUTF8Len = fWideLen;
	fUTF8 = euc_to_utf8(fWide, &fUTF8Len);

	DrawKouhoString();

	free(kana);
}


void
TBottomlineView::ResetKouhoString()
{
	free(fUTF8);
	fUTF8 = NULL;
	fUTF8Len = 0;

	free(fBunWidths);
	fBunWidths = NULL;

	if (fNumBuns < 1)
		return;

	fBunWidths = (float *)malloc(fNumBuns * sizeof(float));

	Wchar	kanji[200];
	BFont	theFont;
	GetFont(&theFont);

	for (int32 i = 0; i < fNumBuns; i++) {
		RkwGoTo(fContext, i);
		int32 len = RkwGetKanji(fContext, kanji, 200);

		char *utf8 = euc_to_utf8(kanji, &len);
		if (utf8 != NULL) {
			fBunWidths[i] = theFont.StringWidth(utf8, len);	

			fUTF8 = (char *)realloc(fUTF8, fUTF8Len + len);
			memcpy(fUTF8 + fUTF8Len, utf8, len);
			fUTF8Len += len;

			free(utf8);
		}
	}
		
	RkwGoTo(fContext, fCurBun);
}


void
TBottomlineView::DrawKouhoString()
{
	fBitmap->Lock();
	BView *drawView = fBitmap->ChildAt(0);

	drawView->FillRect(Bounds(), B_SOLID_LOW);	

	if ((fUTF8 != NULL) && (fUTF8Len > 0)) {
		BFont theFont;
		GetFont(&theFont);
		drawView->SetFont(&theFont);

		font_height fHeight;
		theFont.GetHeight(&fHeight);

		drawView->MovePenTo(kLeftOffset, kVOffset + fHeight.ascent);
		drawView->DrawString(fUTF8, fUTF8Len);

		float leftSide = kLeftOffset;
		float savePenSize = PenSize();
		float lineY = kVOffset + fHeight.ascent + fHeight.descent + 
					  fHeight.leading + kVOffset;
		for (int32 i = 0; i < fNumBuns; i++) { 
			float curWidth = fBunWidths[i];

			if (i == fCurBun)
				drawView->SetPenSize(2.0);

			drawView->StrokeLine(BPoint(leftSide + 1.0, lineY), 
								 BPoint(leftSide + curWidth - 2.0, lineY));

			if (i == fCurBun)
				drawView->SetPenSize(savePenSize);

			leftSide += curWidth;
		}
	}

	drawView->Sync();
	DrawBitmap(fBitmap);
	fBitmap->Unlock();
}

#endif
