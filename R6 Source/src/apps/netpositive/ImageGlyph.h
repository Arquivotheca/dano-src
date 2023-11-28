// ===========================================================================
//	ImageGlyph.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __IMAGEG__
#define __IMAGEG__

#include "Glyph.h"

#include <Polygon.h>
#include <String.h>

class ImageHandle;
class ConnectionManager;
class BMessenger;

// ===========================================================================

class Area : public CLinkable {				// Rectangular area
public:
				Area(const char* coords, const char* href, const char *target, int showTitleInToolbar);
			
virtual	const char*	GetHREF(float x, float y);
virtual const char*	GetTarget(float x, float y);
virtual int			GetShowTitleInToolbar(float x, float y);
	
protected:
		BRect	mRect;
		BString	mHREF;
		BString	mTarget;
		int mShowTitleInToolbar;
};

class AreaCircle : public Area {		// Circular Area
public:
				AreaCircle(const char* coords, const char* href, const char *target, int showTitleInToolbar);
				
virtual	const char*	GetHREF(float x, float y);
virtual const char*	GetTarget(float x, float y);

};

class AreaPoly : public Area {
public:
				AreaPoly(const char *coords, const char* href, const char *target, int showTitleInToolbar);
virtual const char* GetHREF(float x, float y);
virtual const char* GetTarget(float x, float y);
		bool		PointInPoly(float x, float y);

protected:
		CArray<long, 16> mXVertices;
		CArray<long, 16> mYVertices;
		int			mVertexCount;
};

//	Imagemap object is a named group of "hot" areas

class ImageMap : public CLinkable {

public:
					ImageMap(char *name);

		void		AddArea(int shape, const char* coords,const char* url, const char *target, int showTitleInToolbar);
		const char*	GetName();
		const char*	GetHREF(float x, float y);
		const char* GetTarget(float x, float y);
		int			GetShowTitleInToolbar(float x, float y);
		
protected:
		BString		mName;
		CLinkedList	mAreas;
};

// ===========================================================================
//	ImageGlyph is an image!

class ImageGlyph : public SpatialGlyph {
public:
					ImageGlyph(ConnectionManager *mgr, bool forceCache, BMessenger *listener, Document *htmlDoc, bool firstFrameOnly = false);
		virtual		~ImageGlyph();

			void	SetAnchor(Glyph* isAnchor);
			Glyph*	GetAnchor();
			
#ifdef DEBUGMENU
			void	PrintStr(BString& print);
#endif
			void	GetName(BString& name);

			void	GetKnownSize(float& width, float& height);

	virtual	bool	IsImage();
	virtual	bool	Floating();
	virtual	bool	Separable();
	virtual bool IsTransparent();
			void	SetNoBreak(bool noBreak);
			
			bool	GetComplete();
			bool	GetResourceComplete();
			bool	Abort();
			
			bool	IsMap();
	virtual	bool	Clicked(float h, float v);
			bool	GetClickCoord(float* h, float* v);

	virtual	float	GetWidth();
	virtual	float	GetHeight();
	virtual	short	GetAlign();

	virtual	void	Invalidate(UpdateRegion &r);	// Update....
	virtual	bool	GetUpdate(UpdateRegion &r);		// Clears update region when it is done
	
			void	GetImageBounds(BRect &r);	// Inset by mHSpace, mVSpace and border
			int		GetBorder();
			int32	GetHSpace() {return mHSpace;}
			int32	GetVSpace() {return mVSpace;}

	virtual	void	Hilite(long value, DrawPort *drawPort);
			void	DrawBorder(bool selected, DrawPort *drawPort);
			
			void	CreateImage();
	virtual	void	Draw(DrawPort *drawPort);
			void	DrawDead(DrawPort *drawPort);
			void	DrawWaiting(DrawPort *drawPort);
			void	DrawAsBox(DrawPort *drawPort, long fgColor, long bgColor);

	virtual	void	SetAttribute(long attributeID, long value, bool isPercentage);
	virtual	void	SetAttributeStr(long attributeID, const char* value);

	virtual	bool	ReadyForLayout();
	virtual bool	IsLayoutComplete();
	virtual bool	IsPositionFinal() {return mPositionIsFinal;}
	virtual void	LayoutComplete(DrawPort *drawPort);
			void	Layout(DrawPort *drawPort);

			const char*	GetSRC();
			const char*	GetUSEMAP();

			void	Load(long docRef);
			bool	KnowSize();
			bool	Idle(DrawPort *drawPort);
			bool	IsDead();
			void	Reset();
			
			ImageHandle*	GetImageHandle() {return mImageHandle;}
			
	virtual	void	SetParent(Glyph* parent);
	const char *	GetNameAttr() {return mName.String();}
	
			void	SetBorder(int32 border) {mBorder = border;}
			void	SetHieght(int32 height)	{mHeight = height; mHasHeightAttr = true;}
			void	SetWidth(float width) {mWidth = width; mHasWidthAttr = true;}
			void	SetHSpace(int32 space) {mHSpace = space;}
			void	SetVSpace(int32 space) {mVSpace = space;}
			void	SetNameAttr(const char *name) {mName = name;}
			void	SetSRC(const char *src) {mSRC = src;}
			void	Unload();
	
protected:
			bool		mIsMap;
			short		mBorder;
			short		mHSpace;
			short		mVSpace;
			short		mAlign;
			bool		mNoBreak;
			
			BString		mSRC;
			BString		mALT;
			BString		mUSEMAP;
			
			UpdateRegion	mUpdate;
			
			ImageHandle* 	mImageHandle;
			bool		mAborted;
			
			bool		mGetSizeLater;
			bool		mLayoutComplete;
//			bool		mLoadImage;

//			bool		mCache;
			float		mClickH;
			float		mClickV;
			Glyph*		mIsAnchor;
			bool		mPositionIsFinal;
			bool		mForceCache;
			
			bool		mHasBGImage;
			bool		mHasBGColor;
			bool		mHasWidthAttr;
			bool		mHasHeightAttr;
			long		mBGColor;
			ImageGlyph	*mBGGlyph;
			ConnectionManager *mConnectionMgr;
			bigtime_t	mAskedForLayout;
			BMessenger *mListener;
			BString		mName;
			bool mFirstFrameOnly;
};

#endif
