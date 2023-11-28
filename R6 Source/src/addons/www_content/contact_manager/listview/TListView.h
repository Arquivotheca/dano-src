/*
	TListView.h
*/
#ifndef _TLISTVIEW_H_
#define _TLISTVIEW_H_
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <experimental/ResourceSet.h>
#include <String.h>
#include <TextControl.h>
#include <MessageFilter.h>
#include <Binder.h>

class TButtonColumn;
class TButtonField;
class TCounterColumn;
class TCounterField;
class TListItem;
class TListView;
class TStringColumn;
class TStringField;

BResourceSet&  Resources(void);

enum t_column_type
{
	COLUMN_BITMAPBUTTON   = 1,
	COLUMN_COUNTERFIELD   = 2,
	COLUMN_ICON           = 3,
	COLUMN_STRINGFIELD    = 4,
	
	COLUMN_END            = 0,
};

struct t_column_info 
{
	t_column_type			type;
	char 					label[61];
	float 					width_default;
	const char*				value_name;
	char					up_image[101];
	char					over_image[101];
	char					selected_image[101];
	char					static_image[101];
	uint32					Button_message;

// runtime data
	BColumn*				column;
};

class TButtonColumn: public BTitledColumn
{
	public:
	
						TButtonColumn(const char* title, float width,
						 float maxWidth, float minWidth);
						~TButtonColumn(void);

	void				DrawField(BField *field, BRect rect, BView *parent);
	int					CompareFields(BField* field1, BField* field2);
	
	void				MouseDown(BColumnListView* parent, BRow* row,
	                     BField *field, BRect field_rect, BPoint point,
						 uint32 buttons);
	void				MouseMoved(BColumnListView* parent, BRow* row,
						 BField* field, BRect field_rect, BPoint point,
						 uint32 buttons, int32 code);
	void				MouseUp(BColumnListView* parent, BRow* row,
						 BField* field);
};                         // End TButtonColumn

class TButtonField: public BField
{
	public:
	
						TButtonField(uint32 msg, const char* up,
						 const char* over, const char* selected,
						 const char* stati);

	const BBitmap*		CurrentBitmap(void) const;
				 
	BRect               AreaRect(void) const;
	BRect				BitmapRect(void);
	void				SetActive(bool state);
	bool				IsActive(void) const;
	void				SetAreaRect(BRect rect);
	void				SetStatic(const char* stc);
	void				SetOffset(BPoint off);
	BPoint			 	Offset(void) const;
				 
	bool				isFolder;
	bool				mouseover;
	bool				mousedown;
	uint32				message;

	private:

	bool				_active;
	BRect               _areaRect;    // My onscreen rectangle
	BRect               _bitmapRect;  // Saved position of drawn graphic
	BPoint				_offset;
	const char* 		_up;
	const char* 		_over;
	const char* 		_selected;
	const char* 		_static;
};                                  // End TButtonField

class TCounterColumn: public BTitledColumn
{
	public:
						TCounterColumn(const char* title, float width,
						 float maxWidth, float minWidth);
						~TCounterColumn(void);
								
	virtual void		DrawField(BField* field, BRect rect, BView* parent);
	virtual int			CompareFields(BField* field1, BField* field2);
};

class TCounterField: public BField
{
	public:
	
						TCounterField(TListView* listview, TListItem* parent);
	virtual	BString		Value(void);
								 
	private:

	TListItem*			_parent;
	TListView*			_listview;
};

class TListItem: public BRow
{
	public:
	
						TListItem(TListView* listview,
						 BinderNode::property &root, BString idstr,
						 t_column_info* cinfo);
						~TListItem(void);

	bool				IsTagged(void);
	void				Tag(bool state = true);
				
	BinderNode::property	Property(void);
	BinderNode::property	ParentProperty(void);
	BinderNode::property&	RootProperty(void);
	BString&				IDStr(void);
		
	void				SetHasParent(bool state = true);
	bool				HasParent(void) const;
		
	void				SetExpandable(bool state = true );
	bool				Expandable(void) const;
	
	bool				IsBrandNew(void);
	void				SetBrandNew(bool state);
				
	private:

	TListView*				_listview;
	BinderNode::property&	_rootproperty;

	bool					_tagged;
	bool					_brandnew;
	t_column_info*			_cinfo;
	BString			 	  	_idstr;
	bool					_expandable;
	bool					_hasparent;
};                                        // End TListItem

class TListView: public BColumnListView
{
	public:
						TListView(BinderNode::property& binderRoot,
						 BRect frame, const BMessage& parameters,
						 t_column_info* cinfo);
				 		~TListView(void);

	void				BeginEdit(void);
	void				EndEdit(bool accept = false);
		
	void				SetHandler(GHandler* handler,
						 uint32 message, uint32 endedit);
	bool				InEditMode(void);
	void				SetDragable(bool state = true);
	bool				Dragable(void) const;
	bool				InitiateDrag(BPoint, bool wasSelected);

	void				DeselectAllAndClear(void);	
	void				SelectionChanged(void);

	private:
	
	void				SyncFromBinder(BinderNode::property &rootprop, TListItem *parentitem=0);
	void				CreateColumns(void);
	void				SetColors(const BMessage& args);
	void				SetColumnNames(const BMessage& args);
	void				SetColumnSort(const BMessage& args);
	void				SetColumnWidths(const BMessage& args);
	void				SetFonts(const BMessage& args);
	void				AttachedToWindow(void);
	void				DetachedFromWindow(void);
	void				ItemInvoked(void);
	void				MessageReceived(BMessage* msg);
	void				DrawLatch(BView*, BRect, LatchType, BRow*);
				
	private:

	BinderNode::property&	_binderRoot;
	bool					_editmode;
	TListItem*				_selectedrow;	
	BMessageFilter*			_mouseclickfilter;
	t_column_info*			_cinfo;
	GHandler*				_handler;
	uint32					_message;
	uint32					_endedit;
	bool					_dragable;
};                                        // End TListView

class TStringColumn: public BTitledColumn
{
	public:
	
						TStringColumn(const char* title,
						 float width, float maxWidth,
						 float minWidth, uint32 truncate,
						 BView* control);
						~TStringColumn(void);
								
	virtual void		DrawField(BField* field, BRect rect, BView* parent);
	virtual int 		CompareFields(BField* field1, BField* field2);
		
	BView*				Control(void) const;
	
	private:
	
	uint32 			 _truncate;
	BView* 			 _control;
};                                    // End TStringColumn

class TStringField: public BField
{
	public:
	
						TStringField(TListView* listview,
						 TListItem* item, const char* name);
								
	void				SetTo(BString value);
	virtual BString		Value(void);
								 
	void				SetEditable(bool state = true);
	bool				Editable(void) const;

	BString				_clippedString;
	float				_lastWidth;		
	bool				_editable;

	private:
	
	TListView*			_listview;
	TListItem*			_item;
	const char*			_name;
};

#endif               // End TListView.h

