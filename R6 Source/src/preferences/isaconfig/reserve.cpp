#include <app/Application.h>
#include <InterfaceKit.h>
#include <support/Autolock.h>

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jumperedlib.h"

#define M_UPDATE 'updt'

#define IO_MESSAGE_BASE 'io--'
#define MEM_MESSAGE_BASE 'mem-'
#define NEW_RANGE 0
#define DELETE_RANGE 1
#define SELECTED_RANGE 2
#define INVOKED_RANGE 3

static
int32 mask_to_value(uint32 mask)
{
	uint32 value = 0;
	
	if (!mask) return -1;
	
	while (mask) {
		if (mask & 1)
			return (mask == 1) ? value : -1;
		mask >>= 1;
		value++;
	}
	
	return -1;
}

class EnterRangeWindow : public BWindow
{
	BListView *RangeList;

	static const uint32 M_OK = 'm_ok';

	BTextControl *from, *to;
	int32 index;
public:
	EnterRangeWindow(const char *text, BListView *RangeList,
		int32 index = -1, uint32 initial_from = 0, uint32 initial_to = 0);

	void MessageReceived(BMessage *message);
};

EnterRangeWindow::EnterRangeWindow(const char *text, BListView *r,
		int32 index, uint32 initial_from, uint32 initial_to) :
		BWindow(BRect(100,100,300,200), NULL, B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	char str[20];
	BStringView *s;
	BButton *button;
	
	RangeList = r;
	this->index = index;

	BView *top_view = new BView(Bounds(), NULL, B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW);
	top_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(top_view);

	s = new BStringView(BRect(0, 10, 190, 20), NULL, text);
	s->SetAlignment(B_ALIGN_CENTER);
	top_view->AddChild(s);

	sprintf(str, "0x%x", initial_from);
	from = new BTextControl(BRect(10, 30, 100, 50), NULL, "From:", str, NULL);
	sprintf(str, "0x%x", initial_to);
	to = new BTextControl(BRect(110, 30, 190, 50), NULL, "To:", str, NULL);

	from->SetDivider(35); to->SetDivider(22);
	top_view->AddChild(from); top_view->AddChild(to);

	top_view->AddChild(new BButton(BRect(10, 70, 100, 90), "Cancel", "Cancel",
			new BMessage(B_QUIT_REQUESTED)));
	top_view->AddChild(button = new BButton(BRect(110, 70, 190, 90), "Save", "Save",
			new BMessage(M_OK)));

	SetDefaultButton(button);

	Show();
}

void EnterRangeWindow::MessageReceived(BMessage *message)
{
	if (message->what == M_OK) {
		uint32 start, end;
		char s[64];

		if ((*(from->Text()) == '-') || (*(to->Text()) == '-')) {
			(new BAlert("", "Negative numbers not allowed.", "Oops"))->Go();
			return;
		}

		start = strtoul(from->Text(), NULL, 0);
		end = strtoul(to->Text(), NULL, 0);
		if (end < start) {
			(new BAlert("", "End precedes start!", "Oops"))->Go();
			return;
		}

		sprintf(s, "0x%x to 0x%x", start, end);
		RangeList->Window()->Lock();
		if (index == -1)
			RangeList->AddItem(new BStringItem(s));
		else {
			RangeList->RemoveItem(index);
			RangeList->AddItem(new BStringItem(s), index);
//			ReplaceItem seems to be broken!
//			RangeList->ReplaceItem(index, new BStringItem(s));
		}
		RangeList->Window()->Unlock();
		RangeList->Window()->PostMessage(M_UPDATE);

		Quit();
	}
}

class RangeEditView : public BView
{
	BStringView *Title;
	BScrollView *RangeListScroll;
	BListView *RangeList;
	
	BButton *NewButton, *DeleteButton;
	uint32 message_base;
	
	const char *title;
public:
	RangeEditView(uint32 message_base,
			BRect frame, const char *name, uint32 mode, uint32 flags);

	virtual void AttachedToWindow(void);
	virtual void FrameResized(float w, float h);
	virtual void MessageReceived(BMessage *message);

	void SetEnabled(bool enabled);

	BListView *GetRangeList(void) const { return RangeList; }
};

RangeEditView::RangeEditView(uint32 message_base,
	BRect f, const char *name, uint32 mode, uint32 flags) :
	BView(f, name, mode, flags)
{
	this->message_base = message_base;
	title = name;
}

void RangeEditView::AttachedToWindow(void)
{
	BRect f = Frame();

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	AddChild(Title = new BStringView(BRect(), NULL, title));
	Title->SetFont(be_bold_font);

	RangeList = new BListView(BRect(0,0,f.Width()/2, f.Height()/2), NULL,
			B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	RangeList->SetSelectionMessage(new BMessage(message_base + SELECTED_RANGE));
	RangeList->SetInvocationMessage(new BMessage(message_base + INVOKED_RANGE));
	AddChild(RangeListScroll = new BScrollView(NULL, RangeList,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));//, B_PLAIN_BORDER));

	AddChild(NewButton = new BButton(BRect(), "New", "New",
			new BMessage(message_base + NEW_RANGE)));

	AddChild(DeleteButton = new BButton(BRect(), "Delete", "Delete",
			new BMessage(message_base + DELETE_RANGE)));

	FrameResized(f.Width(), f.Height());
}

void RangeEditView::FrameResized(float w, float h)
{
	float divider = w * 0.75, splitter = h / 6;

	Title->ResizeTo(w, splitter);
	Title->MoveTo(0, 0);
	Title->Invalidate();
	
	h -= splitter;

	RangeListScroll->ResizeTo(divider - 10, h);
	RangeListScroll->MoveTo(0, splitter);
	RangeListScroll->Invalidate();

	NewButton->ResizeTo(w - divider - 20, h / 3);
	NewButton->MoveTo(divider + 10, splitter + h / 9);

	DeleteButton->ResizeTo(w - divider - 20, h / 3);
	DeleteButton->MoveTo(divider + 10, splitter + h / 9 + h / 3 + h / 9);

	Invalidate();
}

void RangeEditView::MessageReceived(BMessage *message)
{
	int32 w = message->what - message_base;
	int32 i;

	switch (w) {
		case NEW_RANGE :
			new EnterRangeWindow((message_base == IO_MESSAGE_BASE) ?
					"Reserve IO Range" : "Reserve Memory Range", RangeList);
			break;
		case DELETE_RANGE :
			i = RangeList->CurrentSelection();
			if (i < 0) break;
			RangeList->RemoveItem(i);
			Window()->PostMessage(M_UPDATE);
			break;
		case SELECTED_RANGE :
			i = RangeList->CurrentSelection();
			DeleteButton->SetEnabled(i >= 0);
			break;
		case INVOKED_RANGE :
		{
			uint32 start, end;
			BStringItem *item;
			i = RangeList->CurrentSelection();
			item = (BStringItem *)RangeList->ItemAt(i);
			if (!item) break;
			sscanf(item->Text(), "0x%x to 0x%x", &start, &end);
			new EnterRangeWindow((message_base == IO_MESSAGE_BASE) ?
					"Reserve IO Range" : "Reserve Memory Range", RangeList,
					i, start, end);
		}
			break;
		default :
			BView::MessageReceived(message);
	}
}

void RangeEditView::SetEnabled(bool enabled)
{
	rgb_color c = (enabled) ? (rgb_color){0,0,0,255} :
			tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT);
	Title->SetHighColor(c);
	Title->Invalidate();
	if (enabled)
		RangeList->SetFlags(RangeList->Flags() | B_NAVIGABLE);
	else
		RangeList->SetFlags(RangeList->Flags() & ~B_NAVIGABLE);
	NewButton->SetEnabled(enabled);
	DeleteButton->SetEnabled(RangeList->CurrentSelection() >= 0);
}

class TWindow : public BWindow
{
	BView *top_view;

	BScrollView *DeviceListScroll;
	BListView *DeviceList;
	BButton *NewButton, *DeleteButton, *QuitButton;

	BTextControl *DeviceName;
	BBox *IrqBox, *DmaBox;
	BCheckBox *Irqs[16], *Dmas[8];
	
	RangeEditView *Ios, *Mems;

	int32 current;
	bool update_flag;
	
	struct jumpered_device_node *devices;

	int32 ResetDeviceList(void);
	void UpdateWindow(void);
	void UpdateDeviceList(void);
	
	static const uint32 M_NEW_DEVICE = 'new ';
	static const uint32 M_DELETE_DEVICE = 'dele';
	static const uint32 M_DEVICE_SELECTION = 'slct';
	static const uint32 M_NAME_UPDATE = 'name';
	
public:
	TWindow(BRect frame, const char *title, window_type type,
			uint32 flags, uint32 workspaces = B_CURRENT_WORKSPACE);
	~TWindow();
	
	virtual bool QuitRequested(void);
	virtual void FrameResized(float w, float h);

	virtual void MessageReceived(BMessage *message);
};

int32 TWindow::ResetDeviceList(void)
{
	BAutolock(this);
	struct jumpered_device_node *n;
	int i;

	DeviceList->MakeEmpty();
	for (n=devices,i=0;n;n=n->next,i++)
		DeviceList->AddItem(new BStringItem(n->info->card_name));
	
	UpdateWindow();
	
	return i;
}

void TWindow::UpdateWindow(void)
{
	int32 i, j;
	struct jumpered_device_node *n;
	BAutolock(this);
	
	current = DeviceList->CurrentSelection();

	n = devices;
	for (i=0;(i<current)&&n;i++)
		n = n->next;

	if ((current < 0) || !n) {
		// disable all controls
		DeleteButton->SetEnabled(false);

		DeviceName->SetText("");		
		DeviceName->SetEnabled(false);
		DeviceName->TextView()->SetFlags(
				DeviceName->TextView()->Flags() & ~B_NAVIGABLE);
		
		for (i=0;i<16;i++) {
			Irqs[i]->SetValue(B_CONTROL_OFF);
			Irqs[i]->SetEnabled(false);
		}
		for (i=0;i<8;i++) {
			Dmas[i]->SetValue(B_CONTROL_OFF);
			Dmas[i]->SetEnabled(false);
		}

		rgb_color c = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT);
		IrqBox->SetHighColor(c);
		IrqBox->Invalidate();
		DmaBox->SetHighColor(c);
		DmaBox->Invalidate();

		Ios->GetRangeList()->MakeEmpty();
		Mems->GetRangeList()->MakeEmpty();
		Ios->SetEnabled(false);
		Mems->SetEnabled(false);

		return;
	}

	DeleteButton->SetEnabled(true);

	DeviceName->SetText(n->info->card_name);
	DeviceName->SetEnabled(true);
	DeviceName->TextView()->SetFlags(
			DeviceName->TextView()->Flags() | B_NAVIGABLE);

	rgb_color black = { 0, 0, 0, 255 };
	IrqBox->SetHighColor(black);
	IrqBox->Invalidate();
	for (i=0;i<16;i++) {
		Irqs[i]->SetValue(B_CONTROL_OFF);
		Irqs[i]->SetEnabled(i != 2);
	}
	
	DmaBox->SetHighColor(black);
	DmaBox->Invalidate();
	for (i=0;i<8;i++) {
		Dmas[i]->SetValue(B_CONTROL_OFF);
		Dmas[i]->SetEnabled(i != 4);
	}
	
	Ios->SetEnabled(true);
	Ios->GetRangeList()->MakeEmpty();
	Mems->SetEnabled(true);
	Mems->GetRangeList()->MakeEmpty();

	char s[20];

	for (i=0;i<n->c->num_resources;i++) {
		switch (n->c->resources[i].type) {
			case B_IRQ_RESOURCE :
				j = mask_to_value(n->c->resources[i].d.m.mask);
				if ((j >= 0) && (j < 16))
					Irqs[j]->SetValue(B_CONTROL_ON);
				else
					printf("Invalid IRQ mask (%x)\n",n->c->resources[i].d.m.mask);
				break;
			case B_DMA_RESOURCE :
				j = mask_to_value(n->c->resources[i].d.m.mask);
				if ((j >= 0) && (j < 8))
					Dmas[j]->SetValue(B_CONTROL_ON);
				else
					printf("Invalid DMA mask (%x)\n",n->c->resources[i].d.m.mask);
				break;
			case B_IO_PORT_RESOURCE :
				sprintf(s, "0x%x to 0x%x", n->c->resources[i].d.r.minbase,
						n->c->resources[i].d.r.minbase + 
						n->c->resources[i].d.r.len - 1);
				Ios->GetRangeList()->AddItem(new BStringItem(s));
				break;
			case B_MEMORY_RESOURCE :
				sprintf(s, "0x%x to 0x%x", n->c->resources[i].d.r.minbase,
						n->c->resources[i].d.r.minbase + 
						n->c->resources[i].d.r.len - 1);
				Mems->GetRangeList()->AddItem(new BStringItem(s));
				break;
			default :
				printf("Unhandled resource type (%x)\n", n->c->resources[i].type);
				break;
		}
	}
}

void TWindow::UpdateDeviceList(void)
{
	struct jumpered_device_node *n;
	int32 i, j, num_res;

	if (current < 0) return;

	for (i=0,n=devices;(i<current)&&n;i++,n=n->next)
		;

	if (!n) return;
	
	strncpy(n->info->card_name, DeviceName->Text(), B_OS_NAME_LENGTH - 1);
	((BStringItem *)DeviceList->ItemAt(current))->SetText(DeviceName->Text());
	DeviceList->Invalidate();

	num_res = 0;

	for (i=0;i<16;i++)
		if (Irqs[i]->Value() == B_CONTROL_ON) num_res++;
	for (i=0;i<8;i++)
		if (Dmas[i]->Value() == B_CONTROL_ON) num_res++;
	num_res += Ios->GetRangeList()->CountItems() +
			Mems->GetRangeList()->CountItems();

	free(n->c);
	n->c = (struct device_configuration *)malloc(
			sizeof(struct device_configuration) +
			num_res * sizeof(resource_descriptor));

	n->c->flags = 0;
	n->c->num_resources = num_res;

	i = 0;
	for (j=0;j<16;j++)
		if (Irqs[j]->Value() == B_CONTROL_ON) {
			n->c->resources[i].type = B_IRQ_RESOURCE;
			n->c->resources[i].d.m.mask = 1 << j;
			n->c->resources[i].d.m.flags = 0;
			n->c->resources[i].d.m.cookie = 0;
			i++;
		}
	for (j=0;j<8;j++)
		if (Dmas[j]->Value() == B_CONTROL_ON) {
			n->c->resources[i].type = B_DMA_RESOURCE;
			n->c->resources[i].d.m.mask = 1 << j;
			n->c->resources[i].d.m.flags = 0;
			n->c->resources[i].d.m.cookie = 0;
			i++;
		}
	for (j=0;j<Ios->GetRangeList()->CountItems();j++) {
		int32 start, end;
		
		sscanf(((BStringItem *)(Ios->GetRangeList()->ItemAt(j)))->Text(),
				"0x%x to 0x%x", &start, &end);
		n->c->resources[i].type = B_IO_PORT_RESOURCE;
		n->c->resources[i].d.r.minbase =
			n->c->resources[i].d.r.maxbase = start;
		n->c->resources[i].d.r.len = end - start + 1;
		n->c->resources[i].d.r.basealign = n->c->resources[i].d.r.flags = 
				n->c->resources[i].d.r.cookie = 0;
	}
	for (j=0;j<Mems->GetRangeList()->CountItems();j++) {
		int32 start, end;
		
		sscanf(((BStringItem *)(Mems->GetRangeList()->ItemAt(j)))->Text(),
				"0x%x to 0x%x", &start, &end);
		n->c->resources[i].type = B_IO_PORT_RESOURCE;
		n->c->resources[i].d.r.minbase =
			n->c->resources[i].d.r.maxbase = start;
		n->c->resources[i].d.r.len = end - start + 1;
		n->c->resources[i].d.r.basealign = n->c->resources[i].d.r.flags = 
				n->c->resources[i].d.r.cookie = 0;
	}
}

TWindow::TWindow(BRect f, const char *t, window_type ty,
		uint32 fl, uint32 w) : BWindow(f,t,ty,fl,w)
{
	uint32 i;

	BRect frame = BScreen().Frame();

	#define MINWIDTH 425
	#define MINHEIGHT 325

	SetSizeLimits(MINWIDTH, frame.Width(), MINHEIGHT, frame.Height());
	
	MoveTo(
			(frame.Width() - f.Width()) / 2,
			(frame.Height() - f.Height()) / 2);
	
	current = -1;
	devices = NULL;
	update_flag = false;
	
	top_view = new BView(Bounds(), NULL, B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	top_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(top_view);
	
	DeviceList = new BListView(BRect(0,0,15,15), NULL,
			B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	DeviceList->SetSelectionMessage(new BMessage(M_DEVICE_SELECTION));
	top_view->AddChild(DeviceListScroll = new BScrollView("scroll configs", DeviceList,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));//, B_PLAIN_BORDER));

	top_view->AddChild(NewButton = new BButton(BRect(), "New", "New",
			new BMessage(M_NEW_DEVICE)));
	top_view->AddChild(DeleteButton = new BButton(BRect(), "Delete", "Delete",
			new BMessage(M_DELETE_DEVICE)));
	top_view->AddChild(QuitButton = new BButton(BRect(), "Quit", "Quit",
			new BMessage(B_QUIT_REQUESTED)));
			
	top_view->AddChild(DeviceName = new BTextControl(BRect(), NULL,
		"Name:", "", NULL, B_FOLLOW_ALL_SIDES));
	DeviceName->SetModificationMessage(new BMessage(M_NAME_UPDATE));
	DeviceName->SetFont(be_bold_font);
	DeviceName->TextView()->SetMaxBytes(B_OS_NAME_LENGTH - 1);

	IrqBox = new BBox(BRect(), NULL);
	IrqBox->SetLabel("IRQs");
	for (i=0;i<16;i++) {
		char label[3];
		sprintf(label, "%d", i);
		IrqBox->AddChild(Irqs[i] = new BCheckBox(BRect(),
				NULL, label, NULL));
		Irqs[i]->SetMessage(new BMessage(M_UPDATE));
	}
	top_view->AddChild(IrqBox);

	DmaBox = new BBox(BRect(), "DMAs");
	DmaBox->SetLabel("DMAs");
	for (i=0;i<8;i++) {
		char label[3];
		sprintf(label, "%d", i);
		DmaBox->AddChild(Dmas[i] = new BCheckBox(BRect(),
				NULL, label, NULL));
		Dmas[i]->SetMessage(new BMessage(M_UPDATE));
	}
	top_view->AddChild(DmaBox);
	
	top_view->AddChild(Ios = new RangeEditView(IO_MESSAGE_BASE,
			BRect(0, 0, 100, 100), "IO Port Ranges", B_FOLLOW_ALL_SIDES,
			B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW));

	top_view->AddChild(Mems = new RangeEditView(MEM_MESSAGE_BASE,
			BRect(0, 0, 100, 100), "Memory Ranges", B_FOLLOW_ALL_SIDES,
			B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW));

	FrameResized(f.Width(), f.Height());

	read_settings_file(&devices);

	ResetDeviceList();
}

TWindow::~TWindow()
{
	free_settings(&devices);
}

bool TWindow::QuitRequested(void)
{
	UpdateDeviceList();
	
	if (update_flag)
		switch ((new BAlert("Quit", "Save changes before quitting?",
				"Cancel", "Quit", "Save", B_WIDTH_AS_USUAL,
				B_OFFSET_SPACING))->Go()) {
			case 1 : break;
			case 2 : write_settings_file(devices); break;
			default: return false;
		}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void TWindow::FrameResized(float w, float h)
{
	float middle = w / 3, quantum, divider;
	int i;

	Lock();

	quantum = (h - 40) / 18.0;

	DeviceListScroll->ResizeTo(middle - 20, 20 + 12 * quantum);
	DeviceListScroll->MoveTo(10, 10);

	divider = h - 10 - 3 * quantum;

	NewButton->ResizeTo((middle - 20)/2, 1.5*quantum);
	NewButton->MoveTo(10, divider);
	
	DeleteButton->ResizeTo((middle - 20)/2, 1.5*quantum);
	DeleteButton->MoveTo(10 + (middle - 20)/2, divider);
	
	QuitButton->ResizeTo(middle - 20, 1.5*quantum);
	QuitButton->MoveTo(10, divider + 1.5*quantum);

	DeviceName->ResizeTo(w - middle - 20, 20);
	DeviceName->SetDivider(50);
	DeviceName->MoveTo(middle + 10, 10);

	IrqBox->ResizeTo(w - middle - 20, 3 * quantum);
	IrqBox->MoveTo(middle + 10, 30 + 0.5*quantum);

	for (i=0;i<16;i++) {
		Irqs[i]->ResizeTo((w - middle - 20)/9, 0.9*quantum);
		Irqs[i]->MoveTo(((i & 7) + 0.5) * (w - middle - 20)/9,
			((i > 7) + 0.8)*quantum);
	}
	
	DmaBox->ResizeTo(w - middle - 20, 2*quantum);
	DmaBox->MoveTo(middle + 10, 30 + 4*quantum);
	
	for (i=0;i<8;i++) {
		Dmas[i]->ResizeTo((w - middle - 20)/9, 0.9*quantum);
		Dmas[i]->MoveTo((i + 0.5) * (w - middle - 20)/9,
			0.8 * quantum);
	}
	
	Ios->ResizeTo(w - middle - 20, 5.5 * quantum);
	Ios->MoveTo(middle + 10, 30 + 6.5 * quantum);

	Mems->ResizeTo(w - middle - 20, 5.5 * quantum);
	Mems->MoveTo(middle + 10, 30 + 12.5 * quantum);

	top_view->Invalidate();
	
	Unlock();
}

void TWindow::MessageReceived(BMessage *message)
{
	int32 i, j;
	bigtime_t t;
	struct jumpered_device_node *n, *node;

	if ((message->what >= IO_MESSAGE_BASE) &&
			(message->what <= IO_MESSAGE_BASE + INVOKED_RANGE)) {
		Ios->MessageReceived(message);
		return;
	}

	if ((message->what >= MEM_MESSAGE_BASE) &&
			(message->what <= MEM_MESSAGE_BASE + INVOKED_RANGE)) {
		Mems->MessageReceived(message);
		return;
	}

	switch (message->what) {
		case M_NEW_DEVICE :
			Lock();

			update_flag = true;
			node = (struct jumpered_device_node *)
					malloc(sizeof(struct jumpered_device_node));
			node->info = (struct isa_device_info *)
					calloc(sizeof(struct isa_device_info), 1);
			node->c = (struct device_configuration *)
					calloc(sizeof(struct device_configuration), 1);
			node->next = NULL;
			
			node->info->info.size = sizeof(struct isa_device_info);
			node->info->info.bus = B_ISA_BUS;
			node->info->info.devtype.base = 0;
			node->info->info.devtype.subtype = 0x80;
			node->info->info.devtype.interface = 0;

			t = real_time_clock_usecs();			
			node->info->info.id[0] = 'PMUJ';
			node->info->info.id[1] = 'DERE';
			node->info->info.id[2] = (t >> 32);
			node->info->info.id[3] = t & 0xffffffff;

			node->info->info.flags = B_DEVICE_INFO_CONFIGURED |
					B_DEVICE_INFO_ENABLED;
			node->info->info.config_status = B_OK;
			strcpy(node->info->card_name, "New Device");
			
			if (devices) {
				for (n=devices;n->next;n=n->next) ;
				n->next = node;
			} else {
				devices = node;
			}
			
			i = ResetDeviceList() - 1;
			DeviceList->Select(i);
			UpdateWindow();
			
			Unlock();
			
			break;

		case M_DELETE_DEVICE :
			Lock();

			update_flag = true;
			i = j = DeviceList->CurrentSelection();
			if (i < 0) {
				Unlock();
				break;
			}
			node = n = devices;
			for (n=devices;n;node=n,n=n->next)
				if (i-- == 0) {
					if (node == n)
						devices = n->next;
					else
						node->next = n->next;
					free(n->info);
					free(n->c);
					free(n);
					DeviceList->RemoveItem(j);
					break;
				}
			UpdateWindow();
			Unlock();
			break;
		case M_NAME_UPDATE :
			if ((current >= 0) &&
					strcmp(((BStringItem *)DeviceList->ItemAt(current))->Text(),
					DeviceName->Text()))
				update_flag = true;
DeviceName->TextView()->Invalidate(); // work around refresh bug
			UpdateDeviceList();
			break;
		case M_UPDATE:
			update_flag = true;
			break;
		case M_DEVICE_SELECTION :
			UpdateDeviceList();
			UpdateWindow();
			break;
		default :
			BWindow::MessageReceived(message);
	}
}

main()
{
	TWindow *win;
	BRect screen;
	
	new BApplication("application/x.vnd-Be.reserve");
	screen = BScreen().Frame();
	win = new TWindow(BRect(50,50,500,400), 
			"Jumpered Devices Resource Allocator", B_TITLED_WINDOW, 0);
	win->Show();
	be_app->Run();
}
