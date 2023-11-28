/*	OptionPopUp.cpp	*/

#include <InterfaceDefs.h>
#include <OptionPopUp.h>

#include <Debug.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <string.h>
#include <algobase.h>
#include <stdio.h>


#if DEBUG
 #define check(x) ((x) ? true : (puts(__FILE__ " check failed: " #x), 0))
#else
 #define check(x) (x)
#endif


BOptionPopUp::BOptionPopUp(
	BRect frame,
	const char * name,
	const char * label, 
	BMessage * message,
	uint32 resize,
	uint32 flags) :
	BOptionControl(frame, name, label, message, resize, flags)
{
	_mField = new BMenuField(Bounds(), "_menu", label, new BPopUpMenu(label), 
		B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(_mField);
	//	make menus extend to right edge
//	BRect r = _mField->MenuBar()->Frame();
//	_mField->MenuBar()->ResizeBy(Bounds().right-r.right, 0);
}


BOptionPopUp::BOptionPopUp(
	BRect frame,
	const char * name,
	const char * label, 
	BMessage * message,
	bool fixed,
	uint32 resize,
	uint32 flags) :
	BOptionControl(frame, name, label, message, resize, flags)
{
	_mField = new BMenuField(Bounds(), "_menu", label, new BPopUpMenu(label), 
		fixed, B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(_mField);
	//	make menus extend to right edge
//	BRect r = _mField->MenuBar()->Frame();
//	_mField->MenuBar()->ResizeBy(Bounds().right-r.right, 0);
}


BOptionPopUp::~BOptionPopUp()
{
}


BMenuField *
BOptionPopUp::MenuField()
{
	return _mField;
}


status_t
BOptionPopUp::AddOptionAt(
	const char * name,
	int32 value,
	int32 index)
{
	bool first = (_mField->Menu()->CountItems() == 0);
	BMessage *msg = MakeValueMessage(value);
	BMenuItem *item = new BMenuItem(name, msg);
	_mField->Menu()->AddItem(item, index);
	if (Window()) item->SetTarget(this);
	if (first) SetValue(value);
	return B_OK;
}


bool
BOptionPopUp::GetOptionAt(
	int32 index,
	const char ** out_name,
	int32 * out_value)
{
	*out_name = NULL;
	*out_value = 0;
	BMenuItem * item = _mField->Menu()->ItemAt(index);
	if (!item) return false;
	if (item->Message()->FindInt32("be:value", out_value)) return false;
	*out_name = item->Label();
	return true;
}


void
BOptionPopUp::RemoveOptionAt(
	int32 index)
{
	BMenuItem * item = _mField->Menu()->ItemAt(index);
	if (!item) return;
	_mField->Menu()->RemoveItem(item);
	delete item;
}

int32
BOptionPopUp::CountOptions() const
{
	return _mField->Menu()->CountItems();
}

int32
BOptionPopUp::SelectedOption(
	const char ** name,
	int32 * value) const
{
	BMenuItem * selected = _mField->Menu()->FindMarked();
	int c = CountOptions();
	for (int ix=0; ix<c; ix++) {
		BMenuItem * other = _mField->Menu()->ItemAt(ix);
		if (other == selected) {
			if (value) other->Message()->FindInt32("be:value", value);
			if (name) *name = other->Label();
			return ix;
		}
	}
	return -1;
}


void
BOptionPopUp::AllAttached()
{
	for (int ix=0; ix<_mField->Menu()->CountItems(); ix++) {
		BMenuItem * item = _mField->Menu()->ItemAt(ix);
		if (check(item != NULL)) item->SetTarget(this);
	}
	_mField->SetDivider(_mField->StringWidth(_mField->Label())*1.1+5);
}


void
BOptionPopUp::SetLabel(
	const char *text)
{
	BOptionControl::SetLabel(text);
	_mField->SetLabel(text);
	if (Window()) _mField->SetDivider(_mField->StringWidth(_mField->Label())*1.1+5);
}


void
BOptionPopUp::SetValue(
	int32 value)
{
	bool found = false;
	BOptionControl::SetValue(value);
	PRINT(("SetValue(%d)\n", value));
	for (int ix=0; ix<_mField->Menu()->CountItems(); ix++) {
		BMenuItem * item = _mField->Menu()->ItemAt(ix);
		int32 comp = 0;
		if (check(item != NULL)) {
			if (item->Message()->FindInt32("be:value", &comp) == B_OK &&
				comp == value) {
				BMenuBar * bar = dynamic_cast<BMenuBar*>(_mField->Menu()->Supermenu());
				BMenuItem * shown = bar->ItemAt(0);
				if (strcmp(shown->Label(), item->Label()))
					shown->SetLabel(item->Label());
				found = true;
				break;
			}
			PRINT(("value %d\n", comp));
		}
	}
	if (!found)
		PRINT(("Found no item for value\n"));
}


void
BOptionPopUp::SetEnabled(
	bool on)
{
	_mField->SetEnabled(on);
}


void
BOptionPopUp::GetPreferredSize(
	float *width, 
	float *height)
{
	const int32 MagicExtraSpaceH = 33; // Allows for gap between label and menu, plus R4's triangle.
	const int32 MagicExtraSpaceV = 10; // Allows for border and drop shadow.
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float w, h;
	
	h = fh.ascent + fh.descent + fh.leading;
	w = 0;
	BMenu* menu = MenuField()->Menu();
	
	for (int x = 0; x < menu->CountItems(); x++) {
		// Will this use the proper font?
		w = max(w, StringWidth(menu->ItemAt(x)->Label()));
	}
	
	*width = w + StringWidth(Label()) + MagicExtraSpaceH;
	*height = h + MagicExtraSpaceV;
#if 0	// old stuff
/*	_mField->GetPreferredSize(width, height); */
	*width = ceil((be_plain_font->StringWidth(_mField->Label()) + 
		be_plain_font->StringWidth(_mField->MenuItem()->Label()))*1.05+24);
	font_height fh;
	be_plain_font->GetHeight(&fh);
	*height = ceil((fh.ascent+fh.descent+fh.leading)*1.05+2);
#endif
}


void
BOptionPopUp::ResizeToPreferred()
{
	float x = Bounds().Width(), y = Bounds().Height();
	GetPreferredSize(&x, &y);
	ResizeTo(x, y);
}


void
BOptionPopUp::MessageReceived(
	BMessage * message)
{
	BOptionControl::MessageReceived(message);
}


status_t
BOptionPopUp::_Reserved_OptionControl_0(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionControl_1(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionControl_2(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionControl_3(void *, ...)
{
	return B_ERROR;
}

		/* Mmmh, stuffing! */
status_t
BOptionPopUp::_Reserved_OptionPopUp_0(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionPopUp_1(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionPopUp_2(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionPopUp_3(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionPopUp_4(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionPopUp_5(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionPopUp_6(void *, ...)
{
	return B_ERROR;
}

status_t
BOptionPopUp::_Reserved_OptionPopUp_7(void *, ...)
{
	return B_ERROR;
}

