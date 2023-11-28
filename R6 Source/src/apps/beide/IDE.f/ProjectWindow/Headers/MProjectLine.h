//========================================================================
//	MProjectLine.h
//========================================================================

#ifndef _MPROJECTLINE_H
#define _MPROJECTLINE_H

#include "PlugInPreferences.h"

#include <Region.h>

class MSectionLine;
class MProjectView;
class MBlockFile;
class MPopupMenu;
class MCompile;
class BBitmap;

const float kTriangleWidth = 12.0;
const float kSourceMargin = kTriangleWidth + 10.0;
const float kArrowWidth = 20.0;
const float kDataWidth = 40.0;
const float kCodeWidth = kDataWidth;
const float kCodeDataArrowWidth = kCodeWidth + kDataWidth + kArrowWidth;
const float kArrowLeftMargin = 4.0;
const float kArrowTopMargin = 1.0;
const float kCheckMarkWidth	= 10.0;
const float kCheckMarkLeft = 5.0;
const float kCheckMarkRight = kCheckMarkLeft + kCheckMarkWidth;
const float kSourceBottomMargin = 3.0;

class MProjectLine
{
public:

								MProjectLine(
									MSectionLine& inSection,
									MProjectView& inProjectView);
	virtual						~MProjectLine();
					
	virtual	void				Draw(
									BRect inFrame, 
									BRect inIntersection, 
									MProjectView& inView) = 0;

	virtual bool				DoClick(
									BRect 			inFrame, 
									BPoint 			inPoint,
									bool 			inIsSelected,
									uint32			inModifiers,
									uint32			inButtons);
	virtual bool				SelectImmediately( 
									BRect 			inFrame, 
									BPoint 			inPoint,
									bool 			inIsSelected,
									uint32			inModifiers,
									uint32			inButtons);
	virtual void				Invoke();

	virtual MCompile*			BuildCompileObj(
									MakeActionT		inKind);
	virtual bool				CanBeExecuted() const;

	virtual void				WriteToFile(
									MBlockFile & inFile) = 0;

	virtual void				BuildPopupMenu(
									MPopupMenu & inMenu) const;

	virtual const char *		Name() const = 0;
	virtual void				ExternalName(char* outName) const = 0;
	
	MSectionLine *				GetSection() const
								{
									return fSection;
								}

	void						SetSection(
									MSectionLine * inSection)
								{
									fSection = inSection;
								}
		
protected:
	friend class MProjectLinePainter;
	
	MProjectView&				fProjectView;
	MSectionLine*				fSection;
	int32						fCodeSize;
	int32						fDataSize;

	virtual	void				DrawName(
									BRect 			inFrame, 
									BRect			inIntersection,
									MProjectView& 	inView,
									const char *	inName);
};

// ---------------------------------------------------------------------------
//	class MProjectLinePainter
//	MProjectLinePainter knows how to draw an MProjectLine dealing with the
//	constraints of the clipping and drawing regions of the view
// ---------------------------------------------------------------------------

class MProjectLinePainter
{
public:
						MProjectLinePainter();
	void				DrawLine(BRect inFrame, 
								 BRect inIntersection, 
								 MProjectView& inView,
								 MProjectLine& line);

	void				DrawName(BRect inFrame, 
								 BRect inIntersection,
								 MProjectView& inView,
								 const char* inName);
	
	enum EBitmapKind	{kDownArrow, kRightArrow, kCheckMark, kTouchedMark};
	void				DrawBitmap(EBitmapKind which, BRect inFrame, MProjectView& inView);
	
	bool				PointInArrow(BPoint& inPoint);
	
	void				AdjustSizes(float inWidth);
	
	float				GetNameRight()
						{
							return fNameRight;
						}

	float				GetDataRight()
						{
							return fDataRight;
						}

	BRect				GetArrowRect()
						{
							return fArrowRect;
						}

private:		
	BRect				fNameRect;
	BRect				fCodeRect;
	BRect				fDataRect;
	BRect				fArrowRect;
	float				fNameRight;
	float				fCodeRight;
	float				fDataRight;
	BRegion				fNameClip;

	static BBitmap* 	sRightBitmap;
	static BBitmap* 	sDownBitmap;
	static BBitmap* 	sCheckMarkBitmap;
	static BBitmap* 	sTouchedBitmap;

private:
	void				InitBitmaps();

};

#endif
