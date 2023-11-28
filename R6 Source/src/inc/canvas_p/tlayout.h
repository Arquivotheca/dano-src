#ifndef TLAYOUT_H
#define TLAYOUT_H

//================================================================
//================================================================
template <class T, class CT> class BMTLinearLayout
{
public:
			BMTLinearLayout(CT, const float gap=1.0, const float margin = 0.0, 
				const alignment linement = B_ALIGN_LEFT);
			BMTLinearLayout(const float gap=1.0, const float margin = 0.0, 
				const alignment linement = B_ALIGN_LEFT);
			
	
	// This is the workhorse for layout
	virtual void	Layout(T )=0;

	// These are linear layout specific
	virtual void	SetGap(const float);
	virtual void	SetMargin(const float);
	virtual void	SetAlignment(alignment);
	
	float	Tallest() {return fTallest;};
	float	Widest() {return fWidest;};
	
protected:
	
	float		fGap;
	float		fMargin;
	int32		fElements;
	BPoint		fCurrentPosition;
	float		fWidest;
	float		fTallest;
	alignment	fAlignment;
	CT			fContainer;
	
private:
};

//================================================================
//================================================================
template <class T, class CT>
class BMTHGraphicLayout : public BMTLinearLayout<T,CT>
{
public:
			BMTHGraphicLayout(CT, const float gap=1.0, const float margin = 0.0, 
				const alignment linement = B_ALIGN_LEFT);
			BMTHGraphicLayout(const float gap=1.0, const float aMargin = 0.0, 
				const alignment linement = B_ALIGN_LEFT);
				
	virtual void	Layout(T);

protected:
	
private:
};

//================================================================
//================================================================
template <class T, class CT>
class BMTVGraphicLayout : public BMTLinearLayout<T,CT>
{
public:
			BMTVGraphicLayout(const float gap=1.0, const float aMargin = 0.0, 
				const alignment linement = B_ALIGN_LEFT);
			BMTVGraphicLayout(CT, const float gap=1.0, const float margin = 0.0, 
				const alignment linement = B_ALIGN_LEFT);
				
	virtual void	Layout(T );

protected:
	
private:
};

//================================================================
// Class Implementation:  BMTLinearLayout<T,CT>
//
// A linear layout acts as a base class for all layouts that have
// a standard margin, gap, and alignment.  Sub-classes are the 
// horizontal layout, and the vertical layout.
//================================================================
template <class T, class CT>
BMTLinearLayout<T,CT>::BMTLinearLayout(CT aContainer, const float gap, 
			const float aMargin, 
			const alignment linement)
	: fGap(gap),
	fMargin(aMargin),
	fAlignment(linement),
	fCurrentPosition(0,0),
	fWidest(0.0),
	fTallest(0.0),
	fElements(0),
	fContainer(aContainer)
{
}

template <class T, class CT>
BMTLinearLayout<T,CT>::BMTLinearLayout(const float gap, 
			const float aMargin, 
			const alignment linement)
	: fGap(gap),
	fMargin(aMargin),
	fAlignment(linement),
	fCurrentPosition(0,0),
	fElements(0),
	fContainer(0)
{
}

// These are horizontal layout specific
template <class T, class CT>
void	
BMTLinearLayout<T,CT>::SetGap(const float newGap)
{
	fGap = newGap;
}

template <class T, class CT>
void	
BMTLinearLayout<T,CT>::SetMargin(const float margin)
{
	fMargin = margin;
}


template <class T, class CT>
void	
BMTLinearLayout<T,CT>::SetAlignment(alignment newAlign)
{
	fAlignment = newAlign;
}


//================================================================
// Class Implementation:  BMTHGraphicLayout
//
// A simple horizontal layout.  As graphics are added to the group,
// they are layed out right next to each other.  They start at the
// left margin, and move to the right with a gap in between them.
//================================================================
template <class T, class CT>
BMTHGraphicLayout<T,CT>::BMTHGraphicLayout(const float gap, 
			const float aMargin, 
			const alignment linement)
	: BMTLinearLayout<T,CT>(gap, aMargin, linement)
{
}

template <class T, class CT>
BMTHGraphicLayout<T,CT>::BMTHGraphicLayout(CT aContainer, const float gap, 
			const float aMargin, 
			const alignment linement)
	: BMTLinearLayout<T,CT>(aContainer, gap, aMargin, linement)
{
}

template <class T, class CT>
void	
BMTHGraphicLayout<T,CT>::Layout(T aGraphic)
{
	if (fElements == 0)
		fCurrentPosition.x = fMargin;

	float size = aGraphic->Frame().IntegerWidth();
	float height = aGraphic->Frame().Height();
	
		
	aGraphic->MoveTo(BPoint(fCurrentPosition.x,0.0));

	// Adjust widest, tallest, and new position
	if (height > fTallest)
		fTallest = height;
	fWidest = fCurrentPosition.x+size+1;	
	fCurrentPosition.x += size+1 + fGap;

	// Automatic container addition if present
	//if (fContainer && (aGraphic->Parent() == 0))
	if (fContainer)
		fContainer->AddChild(aGraphic);
	
	fElements++;
}

//================================================================
// Class Implementation:  BMGVLayout
//
// A simple vertical layout.  As the graphics are added to the group,
// they are layed out from top to bottom starting at the specified 
// margin, with a gap in between them.
//================================================================

template <class T, class CT>
BMTVGraphicLayout<T,CT>::BMTVGraphicLayout(const float gap, 
			const float aMargin, 
			const alignment linement)
	: BMTLinearLayout<T,CT>(gap, aMargin, linement)
{
}

template <class T, class CT>
BMTVGraphicLayout<T,CT>::BMTVGraphicLayout(CT aContainer, const float gap, 
			const float aMargin, 
			const alignment linement)
	: BMTLinearLayout<T,CT>(aContainer, gap, aMargin, linement)
{
}

template <class T, class CT>
void	
BMTVGraphicLayout<T,CT>::Layout(T aGraphic)
{	
	if (fElements == 0)
		fCurrentPosition.y = fMargin;

	float size = aGraphic->Frame().IntegerHeight();
	float width = aGraphic->Frame().Width();

	aGraphic->MoveTo(BPoint(fCurrentPosition.x,fCurrentPosition.y));
	if (width > fWidest)
		fWidest = width;
	fTallest = fCurrentPosition.y + size+1;
	fCurrentPosition.y += size+1 + fGap;
		
	// Automatic container addition if present
	if (fContainer)
		fContainer->AddChild(aGraphic);

	fElements++;
}

#endif
