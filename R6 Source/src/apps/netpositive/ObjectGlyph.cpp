// ===========================================================================
//	ObjectGlyph.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "ObjectGlyph.h"
#include "HTMLTags.h"
#include "Builder.h"
#include "BeDrawPort.h"
#include "HTMLView.h"
#include "Cache.h"
#include "PluginSupport.h"
#include <E-mail.h>
#include <Mime.h>
#include <View.h>
#include <malloc.h>


ObjectGlyph* ObjectGlyph::CreateObjectGlyph(Tag* tag, const char *tagText, Document* htmlDoc, BMessage *pluginData)
{
#ifdef PLUGINS
	const char *dataType = NULL;
	BMimeType resultType;
	BMessage tagAttributes;
	float width = 16;
	float height = 16;
	
	if (tag)
		for (Tag *tag2 = (Tag *)tag->First(); tag2; tag2 = (Tag *)tag2->Next()) {
			if(tag2->mOrigAttribute) {
				tagAttributes.AddString("Attribute", tag2->mOrigAttribute);
				if (tag2->mOrigValue)
					tagAttributes.AddString("Value", tag2->mOrigValue);
			}
			if(tag2->mAttributeID == A_TYPE)
				dataType = (const char *)tag2->mValue;
			else if(tag2->mAttributeID == A_WIDTH)
				width = tag2->mValue;
			else if(tag2->mAttributeID == A_HEIGHT)
				height = tag2->mValue;
			else if((tag2->mAttributeID == A_DATA || tag2->mAttributeID == A_SRC) && dataType == NULL)
				BMimeType::GuessMimeType((const char *)tag2->mValue, &resultType);
		}

	//use the filetype if there is no dataType
	if (!dataType || !(*dataType)){
		if(resultType.IsValid()){
			dataType = resultType.Type();
		}
		else
			return NULL;
	}		
	BView *view = InstantiatePlugin(dataType, &tagAttributes, tagText, htmlDoc->GetResource()->GetURL(), new BMessenger(((BeDrawPort *)htmlDoc->GetDrawPort())->GetView()), width, height, pluginData);
	if (view)
		return new ObjectGlyph(view, htmlDoc);
	else
		return NULL;
#else
	return NULL;
#endif
}

ObjectGlyph::ObjectGlyph(BView *pluginView, Document *htmlDoc) :
	SpatialGlyph(htmlDoc)
{
	mDoesDrawing = true;
	mHasBounds = true;
	mBorder = -1;
	mHSpace = -1;	// Default horizontal space outsize tables..
	mVSpace = -1;
	mClickH = -1;
	mClickV = -1;
	
	mObjectView = pluginView;
	
	mAlign = AV_BASELINE;
	mHidden = false;
}

ObjectGlyph::~ObjectGlyph()
{
	if (mHidden)
		delete mObjectView;
}

//	And it might be a floating object

bool ObjectGlyph::Floating()
{
	return (mAlign == AV_LEFT || mAlign == AV_RIGHT);
}

//	Remember where the image was last clicked

bool	ObjectGlyph::Clicked(float h, float v)
{
	BRect r;

	mClickH = h;
	mClickV = v;
	GetObjectBounds(r);
	if (h < r.left || h >= r.right) return 0;
	if (v < r.top || v >= r.bottom) return 0;
	return 1;
}

//	Get the H and V click info

bool	ObjectGlyph::GetClickCoord(float* h, float* v)
{
	BRect r;
	GetObjectBounds(r);
	*h = mClickH - r.left;
	*v = mClickV - r.top;
	return true;
}

//	Include HSpace and VSpace in sizes

float ObjectGlyph::GetWidth()
{
	return  mWidth + (((mHSpace < 0 ? 0 : mHSpace) + GetBorder()) << 1);
}

float ObjectGlyph::GetHeight()
{
	return  mHeight + (((mVSpace < 0 ? 0 : mVSpace) + GetBorder()) << 1);
}

//	Set attributes that images understand

void ObjectGlyph::SetAttribute(long attributeID, long value, bool isPercentage)
{
	switch (attributeID) {
		case A_WIDTH:	mWidth = value;		break;
		case A_HEIGHT:	mHeight = value;	break;
		case A_BORDER:	mBorder = (value == -1 ? 1 : MAX(value,0));	break;
		case A_ALIGN:	mAlign = value;		break;
		case A_HSPACE:	mHSpace = MAX(value, 0);					break;
		case A_VSPACE:	mVSpace = MAX(value, 0);					break;
		default:		Glyph::SetAttribute(attributeID,value,isPercentage);
	}
}

void ObjectGlyph::SetAttributeStr(long attributeID, const char* value)
{
	switch (attributeID) {
		case A_CLASSID:		mClassID = (const char *)value;		break;
		case A_CODEBASE:	mCodeBase = (const char *)value;	break;
		case A_CODE:		mCode = (const char *)value;		break;
		case A_DATA:		mData = (const char *)value;		break;
		case A_NAME:		mName = (const char *)value;		break;
		case A_HIDDEN:		mHidden = true;						break;
		default:		Glyph::SetAttributeStr(attributeID,value);
	}
}

void
ObjectGlyph::SetURL(
	const char	*url)
{
	mURL = url;
}

const char*
ObjectGlyph::URL()
{
	return (mURL.String());
}

short ObjectGlyph::GetAlign()
{
	return mAlign;
}

// Inset by mHSpace, mVSpace and border

void ObjectGlyph::GetObjectBounds(BRect &r)	
{
	GetBounds(&r);
	r.InsetBy((mHSpace < 0 ? 0 : mHSpace) + GetBorder(),(mVSpace < 0 ? 0 : mVSpace) + GetBorder());
}

void ObjectGlyph::Hilite(long value, DrawPort *drawPort)
{
	DrawBorder(value != 0,drawPort);
}

//	If the image is an anchor, default BORDER to 2

int ObjectGlyph::GetBorder()
{
 	int border = mBorder < 0 ? 0 : mBorder;
	return border;
}

//	Draw border around an image is BORDER tag is non-zero

void ObjectGlyph::DrawBorder(bool, DrawPort *drawPort)
{
	int border = GetBorder();
	if (border) {
		BRect r;
		GetObjectBounds(r);
		r.InsetBy(-border,-border);
		drawPort->PenSize(border,border);
		drawPort->SetGray(0);
		drawPort->FrameRect(&r);
		drawPort->PenSize(1,1);
	}
}

void ObjectGlyph::Draw(DrawPort *drawPort)
{
#ifdef PLUGINS
	HTMLView *view = dynamic_cast<HTMLView*>(((BeDrawPort *)drawPort)->GetView());
	BRect frame = mObjectView->Frame();
	BRect viewBounds = view->Bounds();
	
	frame.OffsetBy(viewBounds.LeftTop());
	
	
	BRect r;
	GetObjectBounds(r);

	if (r.left != frame.left || r.top != frame.top)
		mObjectView->MoveTo(r.LeftTop());
		
	if (frame.Height() != mHeight || frame.Width() != mWidth)
		mObjectView->ResizeTo(mWidth, mHeight);	

	if (mObjectView->Window() == NULL && !mHidden) {
		view->AddChild(mObjectView);
	}
#endif
}

#ifdef DEBUGMENU
void ObjectGlyph::PrintStr(BString& print)
{
	SpatialGlyph::PrintStr(print);
	BString name;
	GetName(name);
	if (print.Length() != 0)
		print += " ";
	print += name;
}
#endif

void ObjectGlyph::GetName(BString& name)
{
	name = "";
	if (mName.Length()) {
		name = mName;
	} else {
		if (mCode.Length() > 0)
			name = mCode;
		else {
			const char *n = mCodeBase.String();
			while ((n != NULL) && (strchr(n,'/')))
				n = strchr(n,'/') + 1;
			name = n;
		}
	}
}

//	Try and know size as soon as possible

void ObjectGlyph::Layout(DrawPort *)
{
}

void
ObjectGlyph::GetSRC(
	BString	&string)
{	
	string = mURL;
	string += mCode;
}


void
ObjectGlyph::SetTag(
	const char	*tag)
{
	mTag = tag;
}
