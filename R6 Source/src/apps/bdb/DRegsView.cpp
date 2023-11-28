/*	$Id: DRegsView.cpp,v 1.3 1999/03/14 20:27:04 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/08/98 15:21:19
*/

#include "bdb.h"
#include "DRegsView.h"
#include "DMessages.h"

const int
	kLabelWidth = 6,
	kValueWidth = 10,
	kSpacing = 2,
	kRegWidth = kLabelWidth + kValueWidth + kSpacing,
	kBorderWidth = 4;

DRegsView::DRegsView(BRect frame, const char *name, int resID)
	: BView(frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fRegInfo = NULL;
	fCpuState = NULL;
	fRegInfCnt = 0;
	fResID = resID;
	fTarget = NULL;
	
	SetViewColor(kViewColor);
	SetLowColor(kViewColor);
} /* DRegsView::DRegsView */

DRegsView::~DRegsView()
{
	if (fRegInfo) delete fRegInfo;
	if (fCpuState) free(fCpuState);
} /* DRegsView::~DRegsView */

void DRegsView::AttachedToWindow()
{
	size_t size;
	void *data = gAppResFile->FindResource('Regs', fResID, &size);
	FailNilRes(data);

	BMemoryIO buf(data, size);
	ParseResource(buf);
	free(data);
} /* DRegsView::AttachedToWindow */

void DRegsView::GetPreferredSize(float *x, float *y)
{
	BView *child = ChildAt(0);
	while (child)
	{
		BRect f = child->Frame();

		*x = max(*x, f.right);
		*y = max(*y, f.bottom);
		
		child = child->NextSibling();
	}
	
	*x += kBorderWidth;
	*y += kBorderWidth;
} /* DRegsView::GetPreferredSize */

void DRegsView::Draw(BRect update)
{
	BRect b(Bounds());
	
	SetHighColor(kShadow);
	StrokeLine(b.LeftBottom(), b.RightBottom());
	StrokeLine(b.RightTop(), b.RightBottom());
	SetHighColor(kWhite);
	StrokeLine(b.LeftTop(), b.RightTop());
	StrokeLine(b.LeftTop(), b.LeftBottom());

	b.InsetBy(1, 1);

	SetHighColor(kBlack);
	FillRect(b, B_SOLID_LOW);
} /* DRegsView::Draw */

void DRegsView::ParseResource(BPositionIO& data)
{
	BView *b = FindView("main");
	short x, y;
	int cnt, mCnt, mx, my;
	
	data >> x >> y >> cnt;
	
	mx = x;
	my = y;
	fRegInfCnt = mCnt = x * y;
	
	fRegInfo = new RegInfo[mCnt];
	
	memset(fRegInfo, 0, sizeof(RegInfo) * mCnt);
	
	while (cnt--)
	{
		RegInfo ri;

		data >> ri.name >> x >> y >> ri.offset >> ri.size >> ri.bits;
		
		int indx = x + y * mx;
		if (indx >= mCnt || indx < 0) THROW(("Invalid index for reg %s? %d", ri.name, indx));
		
		fRegInfo[indx] = ri;
	}
	
	font_height ffh, pfh;
	be_fixed_font->GetHeight(&ffh);
	be_plain_font->GetHeight(&pfh);
	
	float dy = max(ffh.ascent+ffh.descent+ffh.leading, pfh.ascent+pfh.descent+pfh.leading);
	float xw = be_fixed_font->StringWidth("x");
	float px = kBorderWidth, py;
	
	for (x = 0; x < mx; x++)
	{
		float lw = 0, ew = 0;
		int ix;
		
		for (y = 0, ix = x; y < my; y++, ix += mx)
		{
			if (fRegInfo[ix].name[0])
			{
				lw = max(lw, be_plain_font->StringWidth(fRegInfo[ix].name));
				ew = max(ew, (fRegInfo[ix].bits / 4 + 1) * xw);
			}
		}
		
		py = kBorderWidth;
		
		for (y = 0, ix = x; y < my; y++, ix += mx)
		{
			if (fRegInfo[ix].name[0])
			{
				BRect f(px, py, px + lw + ew + 4, py + dy);

				AddChild(fRegInfo[ix].ctrl = new BTextControl(f, "reg", fRegInfo[ix].name,
					"0", new BMessage(kMsgRegisterModified)));

				fRegInfo[ix].ctrl->SetTarget(this);

				fRegInfo[ix].ctrl->SetDivider(lw + 2);
				fRegInfo[ix].ctrl->TextView()->SetFontAndColor(be_fixed_font);
				fRegInfo[ix].ctrl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
				fRegInfo[ix].ctrl->TextView()->SetMaxBytes(fRegInfo[ix].bits / 4);
				
				for (int i = 0; i < 255; i++)
					fRegInfo[ix].ctrl->TextView()->DisallowChar(i);
				const char hex[] = "0123456789abcdefABCDEF";
				for (int i = 0; i < strlen(hex); i++)
					fRegInfo[ix].ctrl->TextView()->AllowChar(hex[i]);
			}
			py += dy + 8;
		}
		
		px += lw + ew + 4 + kSpacing * xw;
	}
} /* DRegsView::ParseResource */

void DRegsView::MessageReceived(BMessage *msg)
{
	try
	{
		switch (msg->what)
		{
			case kMsgNewRegisterData:
			{
				void *cpu;
				ssize_t size;
				FailOSErr(msg->FindData("registers", 'regs', &cpu, &size));
				
				if (fCpuState == NULL || fCpuStateLen != size)
				{
					if (fCpuState) free(fCpuState);
					fCpuState = (char *)malloc(size);
					fCpuStateLen = size;
					FailNil(fCpuState);
				}

				memcpy(fCpuState, cpu, size);
				NewCpuState();
				break;
			}
			
			case kMsgRegisterModified:
				RegModified(msg);
				break;
	
			default:
				BView::MessageReceived(msg);
				break;
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
} /* DRegsView::MessageReceived */

void DRegsView::NewCpuState()
{
	int i;
	
	Window()->DisableUpdates();
	for (i = 0; i < fRegInfCnt; i++)
	{
		if (fRegInfo[i].ctrl)
		{
			int value;
			char s[32];
			
			if (fRegInfo[i].size <= 32)
			{
				memcpy(&value, fCpuState + fRegInfo[i].offset, fRegInfo[i].size);

				if (fRegInfo[i].bits == 32)
					sprintf(s, "%08x", value);
				else if (fRegInfo[i].bits == 16)
					sprintf(s, "%04x", value);

				fRegInfo[i].ctrl->SetText(s);
			}
			else
				fRegInfo[i].ctrl->SetText("");
		}
	}
	Window()->EnableUpdates();
} /* DRegsView::NewCpuState */

void DRegsView::RegModified(BMessage *msg)
{
	BTextControl *src;
	FailOSErr(msg->FindPointer("source", (void**)&src));

	if (src)
	{
		char *t;
		const char *s;
		int i;
		
		s = src->Text();
		if (strncmp(s, "0x", 2) == 0) s += 2;
		unsigned long ul = strtoul(s, &t, 16);

		for (i = 0; i < fRegInfCnt; i++)
		{
			if (fRegInfo[i].ctrl == src)
			{
				memcpy(fCpuState + fRegInfo[i].offset, &ul, fRegInfo[i].size);
				NewCpuState();
				if (fTarget)
				{
					BMessage m(kMsgRegisterModified);

					FailOSErr(m.AddData("registers", 'regs', fCpuState, fCpuStateLen));

					fTarget->Looper()->PostMessage(&m, fTarget);
				}
				break;
			}
		}
	}
} /* DRegsView::RegModified */

void DRegsView::SetTarget(BHandler *target)
{
	fTarget = target;
} /* DRegsView::SetTarget */
