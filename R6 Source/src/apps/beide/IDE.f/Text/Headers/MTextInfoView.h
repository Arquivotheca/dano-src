//========================================================================
//	MTextInfoView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MTEXTINFOVIEW_H
#define _MTEXTINFOVIEW_H

#include <View.h>

#include "MFileUtils.h"

class MIDETextView;
class MTextWindow;

     
class MTextInfoView : public BView
{
public:
								MTextInfoView(
									BRect area,
									MTextWindow & window,
									MIDETextView & view);
								~MTextInfoView();

		void					AttachedToWindow();
		void					Draw(
									BRect area);
		void					Pulse();

virtual	void					MouseDown(
									BPoint where);
		void					SetScroller(
									BScrollBar* inScroller)
								{
									fVScroller = inScroller;
								}


private:

		int32					fLastColumn;
		int32					fLastLine;
		MIDETextView &			fText;
		MTextWindow &			fWindow;
		BScrollBar*				fVScroller;
		int32					fMaxRange;
		float					fProportion;
		FileWriteAble			fState;

		static MList<BBitmap*>	sButtonList;
		static bool				sBitmapInited;
		static BRect			sHeaderRect;
		static BRect			sFunctionRect;
		static BRect			sEOLRect;
		static BRect			sLockedRect;
		static BRect			sLineTextRect;

		void					InitBitmaps();
		void					DoHeaderPopup();
		void					DoEOLTypePopup();
		void					DoFunctionPopup();
		void					DoPathPopup();
		void					DoLockPopup();
		void					GetLockedIndexes(
									int32& 	outUpIndex,
									int32&	outDnIndex);
};

#endif
