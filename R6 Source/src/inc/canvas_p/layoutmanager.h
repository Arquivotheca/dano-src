#ifndef LAYOUTMANAGER_H
#define LAYOUTMANAGER_H

#include <View.h>
#include <Window.h>

#include "graphic.h"

class BLayoutManager
{
public:
	virtual void	Layout() = 0;
	
protected:
private:
};

//================================================================
//================================================================
class BLinearLayoutView : public BView, public BLayoutManager
{
public:
			BLinearLayoutView(const BRect frame, const float gap=1.0, const float margin = 0.0, 
				const GAlignment linement = B_GALIGN_LEFT, const bool sizeToFit=true);
			
	virtual void	AllAttached();
	
	// This is the workhorse for layout
	virtual void	Layout();

	// These are linear layout specific
	virtual void	SetGap(const float);
	virtual void	SetMargin(const float);
	virtual void	SetAlignment(GAlignment);
	
	float	Tallest() {return fTallest;};
	float	Widest() {return fWidest;};
	
protected:
	float		fGap;
	float		fMargin;
	int32		fElements;
	BPoint		fCurrentPosition;
	float		fWidest;
	float		fTallest;
	float		fHeight;
	float		fWidth;
	GAlignment	fAlignment;
	bool		fSizeToFit;
		
private:
};

//================================================================
//================================================================
class BHLayoutView : public BLinearLayoutView
{
public:
			BHLayoutView(const BRect frame, const float gap=1.0, const float aMargin = 0.0, 
				const GAlignment linement = B_GALIGN_TOP, const bool sizeToFit=true);
				
	virtual void	Layout();

protected:
	
private:
};

//================================================================
//================================================================
class BVLayoutView : public BLinearLayoutView
{
public:
			BVLayoutView(const BRect frame, const float gap=1.0, const float aMargin = 0.0, 
				const GAlignment linement = B_GALIGN_LEFT, const bool sizeToFit=true);
				
	virtual void	Layout();

protected:
	
private:
};

//================================================================
//================================================================
class BHPackerLayout : public BView, public BLayoutManager
{
public:
			BHPackerLayout(const BRect frame, const BPoint &gaps, const bool autoSize=true);
				
	virtual void	Layout();

protected:
	BPoint	fGaps;
	bool	fAutoSize;

private:
};

#endif
