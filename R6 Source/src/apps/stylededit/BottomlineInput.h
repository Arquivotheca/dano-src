// ============================================================
//  BottomlineInput.h	by Hiroshi Lockheimer
// ============================================================

#pragma once

#include <View.h>
#include "RK.h"


class BBitmap;


class TBottomlineView : public BView {
public:
						TBottomlineView(BRect frame);
	virtual				~TBottomlineView();

	virtual void		Draw(BRect update);
	virtual void		KeyDown(const char *bytes, int32 numBytes);

	void				SetTarget(BHandler *target);

private:
	void				HandleBackspace();
	void				HandleArrowKey(uint32 inArrowKey);
	void				HandleKouho();
	void				HandleKakutei();
	void				HandleRomanKey(const char *bytes, int32 numBytes);

	void				ResetKouhoString();
	void				DrawKouhoString();

private:
	int					fContext;
	RkRxDic*			fRomaDic;
	char*				fRoman;
	int32				fRomanLen;
	Wchar*				fWide;
	int32				fWideLen;
	char*				fUTF8;
	int32				fUTF8Len;
	float*				fBunWidths;
	int32				fNumBuns;
	int32				fCurBun;
	BHandler*			fTarget;
	BBitmap*			fBitmap;		
};

