#ifndef RAW_DATA_WINDOW_H
#define RAW_DATA_WINDOW_H

#include <stdio.h>

#include <Message.h>
#include <Messenger.h>
#include <Window.h>
#include <DataIO.h>
#include <String.h>

#include <ResourceAddon.h>

class BMenuBar;
class BMenuField;
class BTextControl;
class TBitmapView;
class BButton;
class BBox;

class TRawDataItem : public BMallocIO
{
public:
	TRawDataItem(const void* data, size_t size,
				 size_t bytes_per_entry, const char* identifier,
				 const BMessage& meta_data);
	virtual ~TRawDataItem();
	
	BBitmap* MakeBitmap() const;
	
	void SetBytesPerEntry(size_t value);
	size_t BytesPerEntry() const;
	
	void SetIdentifier(const char* value);
	bool HasIdentifier() const;
	const char* Identifier() const;

	void SetWidth(uint32 value);
	uint32 Width(bool* guess=0) const;
	
	void SetHeight(uint32 value);
	uint32 Height(bool* guess=0) const;
	
	void ConstrainDimensions();
	
	void SetColorSpace(color_space value);
	color_space ColorSpace(bool* guess=0) const;
	
	void SetIsCursor(bool isit);
	bool IsCursor(bool* guess=0) const;
	
	bool CanBeCursor() const;
	
	void GuessAttributes() const;
	
	status_t GetDimension(int32 which, uint32* w, uint32* h) const;
	int32 ClosestDimension(uint32 width=0, uint32 height=0) const;
	
	const BList& WidthList() const;
	const BList& HeightList() const;
	
private:
	void MakePossibleDimensions(BList* widths, BList* heights) const;
	void ForgetDimensions();
	
	size_t fBytesPerEntry;
	BString fIdentifier;
	BMessage fMetaData;
	
	uint32 fWidth, fGuessWidth;
	uint32 fHeight, fGuessHeight;
	color_space fColorSpace, fGuessColorSpace;
	bool fIsCursor, fGuessIsCursor;
	bool fHaveGuessed;
	
	BList fWidths;
	BList fHeights;
};

class TRawDataWindow : public BWindow, public BResourceAddonBase
{
public:
	TRawDataWindow(BPoint around,
				   const BResourceAddonArgs& args,
				   BList* items);
	~TRawDataWindow();
	
	virtual void	MessageReceived(BMessage*);
	virtual	void	FrameResized(float new_width, float new_height);
	virtual bool	QuitRequested();

	void			SetCurrentItem(TRawDataItem* item);
	
private:
	void			StepCurrentItem(int32 offset);
	void			StepCurrentWidth(int32 offset);
	
	void			SetCurrentItem(int32 index);
	void			SetCurrentDimension(uint32 w, uint32 h);
	void			SetCurrentColorSpace(color_space cs, bool can_be_cursor);
	void			UpdateIdentifierName();
	void			LayoutViews();
	void			ConstrainFrame(BPoint* init_point = 0);
	
	void			AddResourceItem(BResourceCollection* context,
									const TRawDataItem* item);
									
	BList* fItems;
	BMenuBar* fMenuBar;
	BView* fRoot;
	BBox* fContainer;
	BMenuField* fItemList;
	BMenuField* fDimensions;
	BMenuField* fColorSpaces;
	BTextControl* fName;
	TBitmapView* fPreview;
	BButton* fOkay;
	BButton* fCancel;
	
	TRawDataItem* fCurItem;
	float fPrefWidth, fPrefHeight;
};

status_t parse_text_data(const BResourceAddonArgs& args,
						 BPoint where, const char* text, size_t length);

status_t parse_message_data(const BResourceAddonArgs& args,
							BPoint where, const BMessage* message);

#endif
