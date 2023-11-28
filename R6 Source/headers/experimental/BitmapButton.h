/*******************************************************************************
/
/	File:			BitmapButton.h
/
/   Description:    Experimental class for a BButton control whose image
/					is taken from a BBitmap.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#ifndef _BITMAP_BUTTON_H
#define _BITMAP_BUTTON_H

#include <Button.h>
#include <Bitmap.h>

namespace BExperimental {

class BBitmapButton : public BButton
{
public:
	// Create a new bitmap button.  The given bitmaps must exist for
	// the entire time this control exists; they will not be deallocated
	// when it is destroyed.  If no bitmaps are supplied, this control
	// will look like a normal BButton.  If the 'bmNormal' bitmap is
	// supplied and one or more of the other bitmaps are not, the image
	// for the missing bitmaps will automatically be generated by the
	// control.
	//
	// NOTE: Automatic bitmap generation only works for 32-bit source
	// bitmaps.
	
	BBitmapButton(BRect frame, const char* name,
				  const char* label,
				  BMessage* message,
				  const BBitmap* bmNormal = 0,
				  const BBitmap* bmOver = 0,
				  const BBitmap* bmPressed = 0,
				  const BBitmap* bmDisabled = 0,
				  const BBitmap* bmDisabledPressed = 0,
				  uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual ~BBitmapButton();

	virtual void AttachedToWindow();
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage *drag);
	virtual void MouseUp(BPoint where);
	virtual void Draw(BRect updateRect);
	virtual void GetPreferredSize(float* width, float* height);

	virtual	void MakeFocus(bool focusState = true);
	virtual void SetValue(int32 value);
	virtual void SetEnabled(bool on);
	
	// Return the bitmap for the corresponding state.  If 'create'
	// is true, a bitmap will be generated if possible and a specific
	// one wasn't supplied.
	
	const BBitmap* NormalBitmap(bool create=true) const;
	const BBitmap* OverBitmap(bool create=true) const;
	const BBitmap* PressedBitmap(bool create=true) const;
	const BBitmap* DisabledBitmap(bool create=true) const;
	const BBitmap* DisabledPressedBitmap(bool create=true) const;
	
	void SetNormalBitmap(const BBitmap* bmap);
	void SetOverBitmap(const BBitmap* bmap);
	void SetPressedBitmap(const BBitmap* bmap);
	void SetDisabledBitmap(const BBitmap* bmap);
	void SetDisabledPressedBitmap(const BBitmap* bmap);
	
	// Set location of textual label around the drawn bitmap.
	// This is only used if the bmNormal bitmap is supplied as
	// well as a textual label for the control.
	
	enum label_position {
		LABEL_LEFT = 0,
		LABEL_TOP,
		LABEL_RIGHT,
		LABEL_BOTTOM
	};
	
	void SetLabelPosition(label_position pos);
	label_position LabelPosition() const;
	
private:
	typedef BButton inherited;
	
	const BBitmap* RawBitmap() const;
	
	const BBitmap* fRawBitmap;
	const BBitmap* fNormalBitmap;
	const BBitmap* fOverBitmap;
	const BBitmap* fPressedBitmap;
	const BBitmap* fDisabledBitmap;
	const BBitmap* fDisabledPressedBitmap;
	
	bool fMadeNormal;
	bool fMadeOver;
	bool fMadePressed;
	bool fMadeDisabled;
	bool fMadeDisabledPressed;
	
	label_position fLabelPosition;
	
	bool fMousePressed;
	bool fMouseOver;
	
	bool fSettingValue;
};

}

#endif