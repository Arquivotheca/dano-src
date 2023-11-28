//========================================================================
//	MSyntaxStyleView.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MSYNTAXSTYLEVIEW_H
#define _MSYNTAXSTYLEVIEW_H

#include "MBoxControlChild.h"
#include "MPrefsStruct.h"

class String;


class MSyntaxStyleView : public MBoxControlChild
{
public:
								MSyntaxStyleView(
									BRect 		inArea,
									const char*	inName,
									FontInfo&	inFontInfo);
								~MSyntaxStyleView();

	virtual	void				Draw(
									BRect area);
	virtual	void				MouseMoved(	
									BPoint		where,
									uint32		code,
									const BMessage *	a_message);

	virtual void 				MouseDown(BPoint point);
	virtual	void				MessageReceived(
									BMessage * message);

		void					SetData(
									const FontInfo&	inData);
		void					GetData(
									FontInfo&	outData);
		void					SetFontsID(
									const BFont& inFont);
		void					SetFontsID(
									const font_family	inFamily,
									const font_style	inStyle);
		void					SetFontsSize(
									float	inSize);
		float					FontsSize() const
								{
									return fInfo.size;
								}
		void					SetFontsColor(
									rgb_color	inColor);
		void					SetValue(
									rgb_color	inColor);
		rgb_color				FontsColor() const
								{
									return fInfo.color;
								}

		void					GetFontAndColor(
									BFont&		outFont,
									rgb_color&	outColor);
		void					GetFontFamilyAndStyle(
									font_family		outFamily,
									font_style		outStyle);
		void					GetFontMenuName(
									String&		outName);
static	void					GetDefaultFontMenuName(
									String&		outName);
		void					InvalidateColorAndFont();

		bool					MouseMovedWhileDown(BPoint point);
		void					DrawColorBox(const BRect& colorBox, BView* inView);
		void					DrawFontBox(const BRect& inBox, BView* inView);
		BBitmap*				CreateColorBitmap(const BPoint& point);
		BBitmap*				CreateFontBitmap();

private:

		FontInfo&				fInfo;
		BFont					fFont;
		BRect					fColorBox;
		BRect					fFontBox;
};

#endif
