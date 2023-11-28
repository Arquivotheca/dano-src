#include "RegInfoView.h"
#include "Util.h"
#include "MyDebug.h"
#include <ClassInfo.h>
#include <TextControl.h>



// size width,
// height = 110


RegInfoView::RegInfoView(BRect r,
						BMessage *input,
						BMessage *output,
						const char *serialValue)
	:	BView(r,"reginfoview",B_FOLLOW_NONE,B_WILL_DRAW),
		fSerial(serialValue)
{
	//SetViewColor(180,250,180);
	SetViewColor(light_gray_background);
	ResizeTo(260,228 + (fSerial ? 22 : 0));
	inMsg = input;
	outMsg = output;
}

void RegInfoView::AttachedToWindow()
{
	BRect r = Bounds();
	r.InsetBy(5,5);
 	r.bottom = r.top + 16;
	
	const long tcoffset = 22;
	const long tcindent = 100;
	/* init */
	// BMessage *regInfo = gSettings->regMsg;
	
	const char *vnames[] = {"sid","name","email","company","address",
							"city","state","zip","country","telephone","fax"};
	const char *vlabels[] = {"Serial Number","Name","EMail","Company","Address","City","State",
								"Zip/Postal Code","Country","Phone","FAX"};

	long i = 0;
	if (fSerial == NULL) i++;
	while (i < nel(vnames)) {
		BMessage *msg = new BMessage(S_REGINFO);
		// msg->AddString("tag",vnames[i]);
		BTextControl *tc = new BTextControl(r,vnames[i],vlabels[i],B_EMPTY_STRING,msg);
		AddChild(tc);
		tc->SetFont(be_plain_font);
		tc->SetDivider(tcindent);
		tc->SetTarget(this);
		
		/* init */
		if (i == 0)
			tc->SetText(fSerial);
		else
			tc->SetText(inMsg->FindString(vnames[i]));
		
		r.OffsetBy(0,tcoffset);
		i++;
	}
}

void RegInfoView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case S_REGINFO:
		{
			BControl *p;
			msg->FindPointer("source",(void **)&p);
			BTextControl *tc = cast_as(p,BTextControl);
			
			const char *tag = tc->Name();
			const char *data = tc->Text();
			
			ReplaceString(outMsg,tag,data);
			
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
			break;
		}
	}
}
