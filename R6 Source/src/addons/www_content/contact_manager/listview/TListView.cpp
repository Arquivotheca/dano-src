/*
	TListView.cpp
*/

#include <Binder.h>
#include <Content.h>
#include <ContentView.h>
#include <ColumnTypes.h>
#include <File.h>
#include <Looper.h>
#include <Window.h>
#include <parsedate.h>
#include <stdio.h>
#include <stdlib.h>
#include <TranslatorFormats.h>
#include <MessageFilter.h>
#include <experimental/ResourceSet.h>
#include <www/util.h>
#include "TListView.h"

using namespace Wagner;

// #define DEBUG_OUTPUT   // Un-comment to get printf() output


//*
//***  Class data
//*

struct COLOR                                       // Info on one color
  {
    const char*          name;                     // Name string
    ColumnListViewColor  color;                    // Color number
  };
  
static COLOR colorList[B_COLOR_TOTAL] =            // Color list
  {
    {"backgroundcolor",        B_COLOR_BACKGROUND},
    {"textcolor",              B_COLOR_TEXT},
    {"rowdividercolor",        B_COLOR_ROW_DIVIDER},
    {"selectioncolor",         B_COLOR_SELECTION},
    {"selectiontextcolor",     B_COLOR_SELECTION_TEXT},
    {"nonfocusselectioncolor", B_COLOR_NON_FOCUS_SELECTION},
    {"editbackgroundcolor",    B_COLOR_EDIT_BACKGROUND},
    {"edittextcolor",          B_COLOR_EDIT_TEXT},
    {"headerbackgroundcolor",  B_COLOR_HEADER_BACKGROUND},
    {"headertextcolor",        B_COLOR_HEADER_TEXT},
    {"separatorlinecolor",     B_COLOR_SEPARATOR_LINE},
    {"separatorbordercolor",   B_COLOR_SEPARATOR_BORDER},
  };

struct FONT                                        // Info on one font
  {
    const char*         name;                      // Name string
    ColumnListViewFont  font;                      // Font number
  };
  
static FONT fontList[B_FONT_TOTAL] =               // Font list
  {
    {"normalfont",      B_FONT_ROW},               // Normal font
    {"headerfont",      B_FONT_HEADER}             // Header font
  };

const float kRowHeightDefault = 21.0f;

const uint32 kDummyMessage = 'BEEF';
const uint32 kAcceptChanges = 'OKAY';
const uint32 kCancelChanges = 'CANC';


//*
//***  Class definitions
//*

class ReturnKeyFilter: public BMessageFilter
{
	public:
	
						ReturnKeyFilter(void);
	filter_result       Filter(BMessage* message, BHandler** target);
};


//*
//***  Standalone functions
//*

static void DrawFieldProperly(BRect rect, BView* parent, const char* text)
{
	(void)rect;
	
	if(text != NULL && text[0]) {
		parent->DrawString(text);
	}
}

static float FontHeight(TListView* listview)
{
	font_height  fh;
	BFont        font;
	float        height = kRowHeightDefault;
	
	if (!listview) goto end;
	listview->GetFont(B_FONT_ROW, &font);
	
	font.GetHeight(&fh);
	height = ceil(fh.ascent + fh.descent + fh.leading);
	
	#ifdef DEBUG_OUTPUT
	font.PrintToStream();
	printf("Computed height: %.02f\n", height);
	#endif
	
	end:
	if (height < 10) height = 10;
	return height;
}

static const BBitmap* ResourceBitmap(const char* name)
{
	return Resources().FindBitmap(B_PNG_FORMAT, name);
}


//*
//***  Class functions
//*

ReturnKeyFilter::ReturnKeyFilter(void): BMessageFilter(B_KEY_DOWN) 
{

}

filter_result ReturnKeyFilter::Filter(BMessage* message, BHandler** target)
{
  int8      key;
  status_t  status;
							
  status = message->FindInt8("byte", &key);
  if (status != B_OK) return B_DISPATCH_MESSAGE;
  
  switch (key)
    {
      case B_RETURN:
        Looper()->PostMessage(kAcceptChanges, *target);
        return B_SKIP_MESSAGE;
        
      case B_ESCAPE:
        Looper()->PostMessage(kCancelChanges, *target);
        return B_SKIP_MESSAGE;
        
      default:
        return B_DISPATCH_MESSAGE;
    }
}
	
TButtonColumn::TButtonColumn(const char *title, float width, float minWidth,
 float maxWidth): BTitledColumn(title, width, minWidth, maxWidth)
{
}

TButtonColumn::~TButtonColumn(void)
{
}

int TButtonColumn::CompareFields(BField* field1, BField* field2)
{
  (void)field1;
  (void)field2;
  
  return 0;
}

void TButtonColumn::DrawField(BField* field, BRect rect, BView* parent) 
{
	TButtonField*   bfield = (TButtonField*) field;
	const BBitmap*  bmp;
	BRect           bmpRect;

	// NOTE: I am saving the 'rect' parameter given to this procedure
	// as _areaRect in the TButtonField object because the field_rect
	// given in MouseDown() and MouseMoved() is WRONG.  -- AllenB
	
	bmp = bfield->CurrentBitmap();
	if (!bmp) return;
	
	parent->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	parent->SetDrawingMode(B_OP_ALPHA);
	
    bfield->SetAreaRect(rect);
    bmpRect = bfield->BitmapRect();
	parent->DrawBitmap(bmp, bmpRect.LeftTop());
	
    #ifdef DEBUG_OUTPUT
	BRect mousePoint;
	
	mousePoint.left = bfield->mousePoint.x;
	mousePoint.top = bfield->mousePoint.y;
	mousePoint.right = mousePoint.left + 5;
	mousePoint.bottom = mousePoint.top + 5;
	
	parent->StrokeRect(bfield->BitmapRect(), B_SOLID_LOW);
	parent->StrokeRect(bfield->AreaRect(), B_SOLID_HIGH);
	parent->StrokeRect(mousePoint, B_SOLID_HIGH);
 	#endif
}

void TButtonColumn::MouseDown(BColumnListView* parent, BRow* row,
 BField* field, BRect field_rect, BPoint point, uint32 buttons)
{
	TButtonField*  afield = (TButtonField*) field;
	BRect          hitrect = afield->BitmapRect();
	
	(void)buttons;
	(void)field_rect;  // This rect is SHIT, don't use it
	(void)row;
	
	if (hitrect.Contains(point))
	{
		afield->mousedown = true;
		parent->UpdateRow(row);
	}
	
	#ifdef DEBUG_OUTPUT
	printf("\nMouseDown: %p ", field);
	point.PrintToStream();
	hitrect.PrintToStream();
	#endif
}

void TButtonColumn::MouseMoved(BColumnListView* parent, BRow* row,
 BField* field, BRect field_rect, BPoint point, uint32 buttons, int32 code)
{
	TButtonField*  afield = (TButtonField*) field;
	BRect          hitrect = afield->BitmapRect();
	bool           oldmode = afield->mouseover;
	
	(void)buttons;
	(void)field_rect;  // This rect is SHIT, don't use it
	(void)row;
	
	if (code == B_EXITED_VIEW)
		{
			afield->mouseover = false;
			afield->mousedown = false;
		}	
	else if (hitrect.Contains(point))
		afield->mouseover = true;
	else
		afield->mouseover = false;
		
	if (oldmode != afield->mouseover)
		parent->UpdateRow(row);
		
	#ifdef DEBUG_OUTPUT
	if (oldmode != afield->mouseover || code != 1)
	  {
	    printf("\nMouseMoved: %p %d ", field, afield->mouseover);
	    point.PrintToStream();
	    
	    printf("Hit rect: ");
	    hitrect.PrintToStream();
	    
	    printf("Area rect: ");
	    field_rect.PrintToStream();
	  }
	#endif
}

void TButtonColumn::MouseUp(BColumnListView* parent, BRow* row, BField* field)
{
	TButtonField*  afield = (TButtonField*) field;

	(void)row;
	
	if (afield->mousedown)
	{
		if(afield->mouseover)
			parent->Looper()->PostMessage(afield->message, parent);
			
		afield->mousedown = false;
		parent->UpdateRow(row);
	}
}

TButtonField::TButtonField(uint32 msg, const char* up, const char* over,
 const char* selected, const char* stati): BField(),
 isFolder(false),
 mouseover(false),
 mousedown(false),
 message(msg),
 _active(false),
 _offset(0,0),
 _up(up),
 _over(over),
 _selected(selected),
 _static(stati)
{
}

BRect TButtonField::AreaRect(void) const
{
  return _areaRect;
}

BRect TButtonField::BitmapRect(void)
{
  const BBitmap*  bmp;
  BPoint          pos;
  
  bmp = CurrentBitmap();
  if (!bmp) return _areaRect;
  
  _bitmapRect = bmp->Bounds();
  pos         = _areaRect.LeftTop();
  
  pos.x      += (_areaRect.Width() - _bitmapRect.Width()) / 2;
  pos.x      += _offset.x;
  
  pos.y      += (_areaRect.Height() - _bitmapRect.Height()) / 2;
  pos.y      += _offset.y;
  
  _bitmapRect.OffsetTo(pos);
  return _bitmapRect;
}

const BBitmap* TButtonField::CurrentBitmap(void) const
{
	if (isFolder)   // If it's a folder, ALWAYS use _static
	  {
	    if (!_static) return NULL;
	    return ResourceBitmap(_static);
	  }
	
	if(IsActive())
	{
		if(mouseover)
		{
			if(mousedown) 	
				return (_selected||_static) ? ResourceBitmap(_selected ? _selected : _static) : 0;
			else			
				return (_over||_static) ? ResourceBitmap(_over ? _over : _static) : 0;
		}
		else
		{
			return (_up||_static) ? ResourceBitmap(_up ? _up : _static) : 0;
		}
	}
	else
		return _static ? ResourceBitmap(_static) : 0;
}

bool TButtonField::IsActive(void) const
{
  return _active;
}

BPoint TButtonField::Offset(void) const
{
  return _offset;
}

void TButtonField::SetActive(bool state)
{
  _active = state;
}

void TButtonField::SetAreaRect(BRect rect)
{
  _areaRect = rect;
}

void TButtonField::SetOffset(BPoint off)
{
  _offset = off;
}

void TButtonField::SetStatic(const char* stc)
{
  _static = stc;
}

TCounterColumn::TCounterColumn(const char* title,
 float width, float minWidth, float maxWidth):
 BTitledColumn(title, width, minWidth, maxWidth)
{
}

TCounterColumn::~TCounterColumn(void)
{
}

int TCounterColumn::CompareFields(BField* field1, BField* field2)
{
	return (strcasecmp(((TStringField*)field1)->Value().String(),(((TStringField*)field2)->Value().String())));
}

void TCounterColumn::DrawField(BField* field, BRect rect, BView* parent)
{
	TCounterField *stringField = (TCounterField*) field;
	DrawFieldProperly(rect, parent, stringField->Value().String());
}

TCounterField::TCounterField(TListView* listview, TListItem* parent):
 _parent(parent),
 _listview(listview)
{

}

BString TCounterField::Value(void)
{
  int   c = _listview->CountRows(_parent);
  char  str[16];
  
  if (c > 0)
    sprintf(str, "%d", c);
  else
    str[0] = 0;
   
  return BString(str);
}
								 
TListItem::TListItem(TListView* listview, BinderNode::property& root,
 BString idstr, t_column_info* cinfo):
	BRow(FontHeight(listview) + 8),
	_listview(listview),
	_rootproperty(root),
	_tagged(false),
	_brandnew(false),
	_cinfo(cinfo),
	_idstr(idstr),
	_expandable(false),
	_hasparent(false)
{
	int32  c;
	
	for(c = 0; cinfo[c].type!=COLUMN_END; c++)
	{
		switch(cinfo[c].type)
		{
			case	COLUMN_ICON:
			case	COLUMN_BITMAPBUTTON:
					{
						SetField(new TButtonField(cinfo[c].Button_message,
						 cinfo[c].up_image,
						 cinfo[c].over_image,
						 cinfo[c].selected_image,
						 cinfo[c].static_image), c);
					}
					break;
					
			case	COLUMN_STRINGFIELD:
					{
						SetField(new TStringField(listview, this,
						 cinfo[c].value_name), c);
					}	
					break;
					
			case	COLUMN_COUNTERFIELD:
					{
						SetField(new TCounterField(listview, this), c);
					}	
					break;
					
			default:
					break;
		}
	}
}

TListItem::~TListItem(void)
{
}

bool TListItem::Expandable(void) const
{
  return _expandable;
}

bool TListItem::HasParent(void) const
{
  return _hasparent;
}

BString& TListItem::IDStr(void)
{
  return _idstr;
}

bool TListItem::IsBrandNew(void)
{
	return _brandnew;
}

bool TListItem::IsTagged(void)
{
  return _tagged;
}

BinderNode::property TListItem::ParentProperty(void)
{
 	BRow *parent = 0;
 	bool visible = false;
 	if(_listview->FindParent(this,&parent,&visible) && parent) 
	{
		if((parent = dynamic_cast<TListItem *>(parent)) != 0)
			return dynamic_cast<TListItem *>(parent)->Property();
		else
 			return BinderNode::property();
	}
 	else
		return _rootproperty;
}

BinderNode::property TListItem::Property(void)
{
 	BRow*  parent = 0;
 	bool   visible = false;
 	
 	if(_listview->FindParent(this,&parent,&visible) && parent) 
	{
		if((parent = dynamic_cast<TListItem *>(parent)) != 0)
			return dynamic_cast<TListItem *>(parent)->Property() [_idstr.String()];
		else
 			return BinderNode::property();
	}
 	else
		return _rootproperty [_idstr.String()];
}

BinderNode::property& TListItem::RootProperty(void)
{
  return _rootproperty;
}

void TListItem::SetBrandNew(bool state)
{
	_brandnew = state;
}

void TListItem::SetExpandable(bool state)
{
  _expandable = state;
}

void TListItem::SetHasParent(bool state)
{
  _hasparent = state;
}

void TListItem::Tag(bool state)
{
  _tagged = state;
}

TListView::TListView(BinderNode::property &binderRoot, BRect frame, const BMessage &parameters, t_column_info *cinfo):
	BColumnListView(frame, "*TListView", B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER, false),
	_binderRoot(binderRoot),
	_editmode(false),
	_selectedrow(0),
	_mouseclickfilter(0),
	_cinfo(cinfo),
	_handler(0),
	_message(0),
	_endedit(0),
	_dragable(false)
{
	SetFonts(parameters);
	SetColors(parameters);
	
	SetColumnWidths(parameters);
	SetColumnNames(parameters);
	CreateColumns();
	
	SetColumnFlags((column_flags)(B_ALLOW_COLUMN_NONE));
	SetColumnSort(parameters);

	SyncFromBinder(_binderRoot);
}

TListView::~TListView(void)
{
	// Controls should be deleted by the owner view
}

void TListView::AttachedToWindow(void)
{
	BColumnListView::AttachedToWindow();

	// GENERIC
	for(int32 c=0;_cinfo[c].type!=COLUMN_END;c++)
	{
		if(_cinfo[c].type==COLUMN_STRINGFIELD && _cinfo[c].column)
		{
			((BTextControl *)((TStringColumn *)_cinfo[c].column)->Control())->SetTarget(this);
			((BTextControl *)((TStringColumn *)_cinfo[c].column)->Control())->TextView()->AddFilter(new ReturnKeyFilter());
		}
	}	
}

void TListView::BeginEdit(void)
{
	#ifdef DEBUG_OUTPUT
	printf("BeginEdit: %d\n", InEditMode());
	#endif
	
	// GENERIC
	if(!InEditMode() && _selectedrow)
	{
		SetEditMode(true);
		bool first_text_control = true;
		if(Window())
		{
			Window()->DisableUpdates();
			BRect rowrect;
			if(GetRowRect(_selectedrow,&rowrect))
			{
				float xpos=-1;
				for(int32 c=0;c<CountColumns();c++)
				{
					int32 cinfo_index = -1;
					for(int32 d=0;_cinfo[d].type!=COLUMN_END;d++)
					{
						if(_cinfo[d].column == ColumnAt(c))
						{
							cinfo_index = d;
							break;
						}
					}
					if(cinfo_index>=0)
					{
						switch(_cinfo[cinfo_index].type)
						{
							case	COLUMN_ICON:
							case	COLUMN_BITMAPBUTTON:
									{
										TButtonField* field = (TButtonField *)_selectedrow->GetField(c);
										if(field) field->SetActive(true);
									}
									break;
									
							case	COLUMN_STRINGFIELD:
									{
										TStringColumn *column = (TStringColumn *)ColumnAt(c);
										TStringField *field = (TStringField *)_selectedrow->GetField(c);								
										if(field->Editable())
										{	
											BRect          oldframe;
											BPoint         pos;
											BTextControl*  btc = (BTextControl*) column->Control();
											
									  		btc->ResizeToPreferred(); // BTextControl and BTextView size sync
									  		oldframe = btc->Bounds(); // Save its current size
									  		
									  		if (oldframe.Height() > rowrect.Height())  // If taller than column
									  		  oldframe.bottom = rowrect.Height() - 1;  // Don't allow that
	
											BRect newframe(xpos,rowrect.top,xpos+column->Width(),rowrect.bottom);
											newframe.left += 2;
											newframe.right -= 2;
											
											pos.x = newframe.left;
											pos.y = newframe.top;
											pos.y += (newframe.Height() - oldframe.Height()) / 2;
											
											BPoint org = btc->Parent()->Bounds().LeftTop();  // THIS ACCOUNTS FOR
											pos.y -= org.y;                                  // SCROLLBAR MOVEMENT
											
											btc->MoveTo(pos);  // Centered vertically in this row
											btc->ResizeTo(newframe.IntegerWidth(), oldframe.Height());
											
											btc->SetText(((TStringField *)_selectedrow->GetField(c))->Value().String());
											if(column->Control()->IsHidden())
												column->Control()->Show();
											column->Control()->SetViewColor(EditColor());
											column->Control()->SetLowColor(EditColor());
											if(first_text_control)
											{
												first_text_control = false;
												column->Control()->MakeFocus();
											}
										}
									}
									break;
							default:
									break;
						}
					}
					xpos += ColumnAt(c)->Width()+1;
				}
				_editmode = true;
			}
			Window()->EnableUpdates();
			UpdateRow(_selectedrow);
		}
	}
}	
	
void TListView::CreateColumns(void)
{
	for(int32 c=0;_cinfo[c].type!=COLUMN_END;c++)
	{
		float width = _cinfo[c].width_default;

		switch(_cinfo[c].type)
		{
			case	COLUMN_ICON:
			case	COLUMN_BITMAPBUTTON:
					{
						AddColumn(_cinfo[c].column = new TButtonColumn(_cinfo[c].label, width, width, width), c);
						((TButtonColumn *)_cinfo[c].column)->SetWantsEvents(true);
						_cinfo[c].column->SetShowHeading(false);
					}
					break;
					
			case	COLUMN_STRINGFIELD:
					{
						BTextControl*  editcontrol;
						BFont          font;
						
						GetFont(B_FONT_ROW, &font);
						
						editcontrol = new BTextControl(BRect(0,0,200,FontHeight(this)),"","","",new BMessage(kDummyMessage));
						editcontrol->SetDivider(0);
		  				editcontrol->SetFont(&font);
		  				editcontrol->TextView()->SetFontAndColor(&font);
						if(!editcontrol->IsHidden())
							editcontrol->Hide();
						ScrollView()->AddChild(editcontrol);
						editcontrol->SetTarget(this);
						AddColumn(_cinfo[c].column = new TStringColumn(_cinfo[c].label, width, width, width, B_TRUNCATE_END, editcontrol), c);
					}
					break;
					
			case	COLUMN_COUNTERFIELD:
					{
						AddColumn(_cinfo[c].column = new TCounterColumn(_cinfo[c].label, width, width, width), c);
						_cinfo[c].column->SetShowHeading(false);
					}
					break;
					
			default:
					break;
		}
	}
}

void TListView::DeselectAllAndClear(void)
{
	BColumnListView::Clear();
	SelectionChanged();
}

void TListView::DetachedFromWindow(void)
{
	BColumnListView::DetachedFromWindow();
}

bool TListView::Dragable(void) const
{
  return _dragable;
}

void TListView::DrawLatch(BView*, BRect, LatchType, BRow*)
{
}

void TListView::EndEdit(bool accept)
{
	int32     c, ctotal, d;
	int32     cinfo_index;
	BWindow*  window;
	
	#ifdef DEBUG_OUTPUT
	printf("EndEdit: %d\n", InEditMode());
	#endif
	
	window = Window();
	if (!window) goto end;
	window->DisableUpdates();
	
	ctotal = CountColumns();
	
	for (c = 0; c < ctotal; c++)
	{
		cinfo_index = -1;
		
		for (d = 0; _cinfo[d].type != COLUMN_END; d++)
			if(_cinfo[d].column == ColumnAt(c))
			{
				cinfo_index = d;
				break;
			}
			
		if (cinfo_index < 0) continue;	
		
		switch(_cinfo[cinfo_index].type)
		{
			case	COLUMN_ICON:
			case	COLUMN_BITMAPBUTTON:
					{
						TButtonField *field = (TButtonField *)_selectedrow->GetField(c);
						field->SetActive(false);
					}
					break;
					
			case	COLUMN_STRINGFIELD:
					{
						TStringColumn *column = (TStringColumn *)ColumnAt(c);
						TStringField *field = (TStringField *)_selectedrow->GetField(c);								
						if(field->Editable())
						{	
							if(accept) 
							{
								field->SetTo(((BTextControl *)column->Control())->Text());
							}
								
							if(!column->Control()->IsHidden())
								column->Control()->Hide();
						}
					}	
					break;
					
			default:
					break;
		}                                       // End switch
	}                                           // End column loop
		
	SetEditMode(false);
	MakeFocus();
	window->EnableUpdates();
	UpdateRow(_selectedrow);
	
	end:
	
	if(_handler && _editmode)
	{
	 	BMessage msg(_endedit);
	 	msg.AddBool("accept", accept);
		_handler->PostMessage(msg);
	}
	 
	_editmode = false;
}

bool TListView::InEditMode(void)
{
  return _editmode;
}

bool TListView::InitiateDrag(BPoint point, bool wasSelected)
{
	(void)point;
	
	if(wasSelected && _dragable && CurrentSelection(0)) {
		BMessage msg('bmdg');
		msg.AddPointer("row",CurrentSelection(0));
		BRect rect;
		GetRowRect(CurrentSelection(0),&rect);
		DragMessage(&msg,rect);
		return true;
	}		
	return false;
}

void TListView::ItemInvoked(void)
{
	if(_handler)
		_handler->PostMessage(new BMessage(_message));
	BColumnListView::ItemInvoked();
}

void TListView::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case	kCancelChanges:
				EndEdit();
				break;
		case	kAcceptChanges:
				EndEdit(true);
				break;
		default:
				BColumnListView::MessageReceived(msg);
				break;
	}
}

void TListView::SelectionChanged(void)
{
	// GENERIC
	if(InEditMode())
		if(_selectedrow != dynamic_cast<TListItem *>(CurrentSelection()))
			EndEdit();

	if(_handler)
		_handler->PostMessage(new BMessage(*SelectionMessage()));

	BColumnListView::SelectionChanged();
	_selectedrow = dynamic_cast<TListItem *>(CurrentSelection());
}

void TListView::SetColors(const BMessage& args)
  {
    rgb_color    color;                            // Color to set
    int32        i;                                // Loop counter
    status_t     rval;                             // Function outcome
    const char*  str;                              // Message string pointer
    
    for (i = 0; i < B_COLOR_TOTAL; i++)            // Loop for all colors
      {
        if (!colorList[i].name)                    // If no name string
          {
            ASSERT(false);                         // You fucked up!
            break;                                 // Exit
          }
        
        rval = args.FindString(                    // Find this color
         colorList[i].name, &str);
         
        if (rval != B_OK || !str) continue;        // If not found
        
        color = decode_color(str);                 // Decode it
        SetColor(colorList[i].color, color);       // Set it
      }  
  }
	
void TListView::SetColumnNames(const BMessage& args)
  {
    int32        i;                                // Loop counter
    const char*  ptr;                              // String from message
    int32        size = sizeof(_cinfo[0].label);   // String size
    status_t     status;                           // Error return
    char         str[101];                         // String to test
    
    for (i = 0; _cinfo[i].type != COLUMN_END; i++) // Loop for all columns
      {
        sprintf(str, "column_name_%lu", i);        // Set string to test with
        
        status = args.FindString(str, &ptr);       // Try to find it
        if (status != B_OK || !ptr) continue;      // If not found
        
        strncpy(_cinfo[i].label, ptr, size);       // Copy it over
        _cinfo[i].label[size - 1] = 0;             // Ensure NULL termination
      }
  }
	
void TListView::SetColumnSort(const BMessage& args)
  {
    int32        i;                                // Loop counter
    int32        index;                            // Sort index
    const char*  ptr;                              // String value
    status_t     status;                           // Function return
    
    status = args.FindString("sortby", &ptr);      // Find sortby arg
    if (status != B_OK || !ptr) goto defaultSort;  // If it's not there
    
    index = atoi(ptr);                             // Get sort index
    if (index < 0) goto defaultSort;               // If too small
    
    for (i = 0; _cinfo[i].type != COLUMN_END; i++) // Loop column list
      if (index == i)                              // If index is valid
        {
          SetSortColumn(ColumnAt(i), false, true); // Set new sort column
          return;                                  // That is all
        }
        
    defaultSort:                                   // If nothing else
    SetSortColumn(ColumnAt(1), false, true);       // Use column one
  }
	
void TListView::SetColumnWidths(const BMessage& args)
  {
    int32        i;                                // Loop counter
    const char*  ptr;                              // String from message
    status_t     status;                           // Error return
    char         str[101];                         // String to test
    
    for (i = 0; _cinfo[i].type != COLUMN_END; i++) // Loop for all columns
      {
        sprintf(str, "column_width_%lu", i);       // Set string to test with
        
        status = args.FindString(str, &ptr);       // Try to find it
        if (status != B_OK || !ptr) continue;      // If not found
        sscanf(ptr,"%f",&_cinfo[i].width_default); // Get new width
      }
  }
	
void TListView::SetDragable(bool state)
{
  _dragable = state;
}

void TListView::SetFonts(const BMessage& args)
  {
    BFont        font = be_plain_font;             // Font to set
    int32        i;                                // Loop counter
    status_t     rval;                             // Function outcome
    const char*  str;                              // Message string pointer
    
    for (i = 0; i < B_FONT_TOTAL; i++)             // Loop for all fonts
      {
        if (!fontList[i].name)                     // If no name string
          {
            ASSERT(false);                         // You fucked up!
            break;                                 // Exit
          }
        
        rval = args.FindString(                    // Find this font
         fontList[i].name, &str);
         
        if (rval != B_OK || !str) continue;        // If not found
        
        rval = decode_font(str, &font);            // Create a font
        if (rval != B_OK) continue;                // If it didn't work
        
        #ifdef DEBUG_OUTPUT
        printf("Setting font %s\n", fontList[i].name);
        font.PrintToStream();
        #endif
        
        SetFont(fontList[i].font, &font);          // Set it
      }
  }
	
void TListView::SetHandler(GHandler* handler, uint32 message,
 uint32 endedit)
{
  _handler = handler;
  _message = message;
  _endedit = endedit;
}

void TListView::SyncFromBinder(BinderNode::property& rootprop,
 TListItem* parentitem)
{
	BinderNode::iterator iter = rootprop->Properties();
	BString name;
	for(;(name=iter.Next())!="";)
	{
		BinderNode::property prop;
		if(rootprop->GetProperty(name.String(),prop)==B_OK)
		{
			if(prop.IsObject() && prop->IsValid())
			{
				TListItem *item;
				item = new TListItem(this,_binderRoot,name,_cinfo);
				AddRow(item,parentitem);
				SyncFromBinder(prop,item);
			}
		}
	}
}
 
TStringColumn::TStringColumn(const char *title, float width, float minWidth, float maxWidth, uint32 truncate, BView *control):
	BTitledColumn(title, width, minWidth, maxWidth), _truncate(truncate), 
	_control(control)
{
}

TStringColumn::~TStringColumn()
{
	if(_control)
	{
		if(_control->LockLooper())
		{
			_control->RemoveSelf();
			_control->UnlockLooper();
		}
	}
	delete _control;
}

BView* TStringColumn::Control(void) const
{
  return _control;
}

void TStringColumn::DrawField(BField *field, BRect rect, BView *parent)
{
	TStringField *stringField = (TStringField*) field;
	BFont font;
	parent->GetFont(&font);
	
	if (rect.Width() != stringField->_lastWidth) 
	{
		BString str = stringField->Value();		
		const char *string = str.LockBuffer(str.Length()+1);
		font.GetTruncatedStrings(&string, 1, _truncate, rect.Width() - 10, &(stringField->_clippedString));
		stringField->_lastWidth = rect.Width();		
		str.UnlockBuffer();
	}
	DrawFieldProperly(rect, parent, stringField->_clippedString.String());
}

int TStringColumn::CompareFields(BField *field1, BField *field2)
{
	return (strcasecmp(((TStringField*)field1)->Value().String(),(((TStringField*)field2)->Value().String())));
}

TStringField::TStringField(TListView* listview, TListItem* item,
 const char* name):
 _lastWidth(-1),
 _editable(true),
 _listview(listview),
 _item(item),
 _name(name)
{

}
								
bool TStringField::Editable(void) const
{
  return _editable;
}

void TStringField::SetEditable(bool state)
{
  _editable = state;
}

void TStringField::SetTo(BString value)
{
  _lastWidth = -1;
  if(_item) _item->Property() [_name] = value;
}
								
BString TStringField::Value(void)
{
  if (_item)
    {
      BinderNode::property prop = _item->Property()[_name];
      if (prop.IsString()) return prop;
    }
    
  return BString();
}
								 
