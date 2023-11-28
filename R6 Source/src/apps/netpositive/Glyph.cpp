// ===========================================================================
//	Glyph.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Glyph.h"
#include "HTMLTags.h"
#include "HTMLDoc.h"
#include "TableGlyph.h"
#include "MessageWindow.h"

#ifdef DEBUGMENU
#include <InterfaceDefs.h>
#endif

#include <stdio.h>

void *AlignmentGlyph::sFreeList;
void *LineBreakGlyph::sFreeList;
// ===========================================================================
// ===========================================================================
//	Glyph base class

Glyph::Glyph() : mBreakType(kNoBreak), mParent(NULL)
{
	mDoesDrawing = false;
	mHasBounds = false;
}

Glyph::~Glyph()
{
}

//	Display the contents of the glyph

#ifdef DEBUGMENU
void Glyph::Print(int level)
{
	BString print;
	for (int i = 0; i < level; i++)						// Depth
		print += ".";
	PrintStr(print);
	pprint(print.String());
	
	Glyph* g;
	for (g = GetChildren(); g; g = (Glyph *)g->Next())
		g->Print(level + 1);
}

void Glyph::PrintStr(BString& print)
{
#ifndef NPOBJECT
	print += "No Object Info";
#else
	print += GetType();									// Type of Glyph
#endif
	print += " ";
	
	if (IsSpatial()) {
		static char pos[256];
		sprintf(pos,"[%3f,%3f]",GetTop(),GetWidth());	// Position
		print += pos;
		print += " ";
	}
	
	if (GetBreakType() != kNoBreak) {					// Break Type
		print += BreakStr();
		print += " ";
	}
}

const char* Glyph::BreakStr()
{
	switch (GetBreakType()) {
		case kNoBreak: return "kNoBreak";
		case kPREBreak: return "kPREBreak";
		case kSoft: return "kSoft";
		case kHard: return "kHard";
		case kParagraph: return "kParagraph";
		case kHardLeft: return "kHardLeft";
		case kHardRight: return "kHardRight";
		case kHardAll: return "kHardAll";
		case kHardAlign: return "kHardAlign";
	}
	return "BAD BREAK";
}
#endif

//	Bounds is a read only concept, undefined until layout

void Glyph::GetBounds(BRect *r)
{
	float top = GetTop();
	float left = GetLeft();
	r->Set(left,top,left + GetWidth(),top + GetHeight());
}

void Glyph::SetTop(float)
{
}

void Glyph::SetLeft(float)
{
}

void Glyph::OffsetBy(const BPoint&)
{
}

void Glyph::SetWidth(float)
{
}

void Glyph::SetHeight(float)
{
}

float Glyph::GetTop()
{
	return 0;
}

float Glyph::GetLeft()
{
	return 0;
}

float Glyph::GetWidth()
{
	return 0;
}

float Glyph::GetMinUsedWidth(DrawPort *)
{
	return GetWidth();
}

float Glyph::GetMaxUsedWidth(DrawPort *)
{
	return GetWidth();
}

float Glyph::GetHeight()
{
	return 0;
}

void Glyph::ResetLayout()
{
}

void Glyph::SpacingChanged()
{
}

void Glyph::SetAlign(short)
{
}

short Glyph::GetAlign()
{
	return AV_BASELINE;
}

void Glyph::SetPRE(bool)
{
}

void Glyph::SetStyle(Style)
{
}

const Style& Glyph::GetStyle()
{
	return gDefaultStyle;
}

//	Stack locations of style paramaters for page glyphs

int Glyph::GetListStackDepth()
{
	return 0;
}

int Glyph::GetStyleStackDepth()
{
	return 0;
}

int Glyph::GetAlignStackDepth()
{
	return 0;
}

void Glyph::SetStack(int,int,int,int, int)
{
}

void Glyph::GetStack(int*,int*,int*,int*, int*)
{
}

//	Draw the glyph and all its children after layout

void Glyph::Draw(DrawPort *drawPort)
{
	BRect r;
	GetBounds(&r);
	if (r.right && r.bottom)
		drawPort->DrawBevel(&r);	// Only draw if something is there
}

//	Determine the dimensions of the glyph and all its childern. Position glyph for drawing

void Glyph::Layout(DrawPort *)
{
}

//	After glyph has been positioned

void Glyph::LayoutComplete(DrawPort *)
{
}

bool Glyph::IsLayoutComplete()
{
	return true;
}

//	Not all that happy about these methods

bool	Glyph::IsLineBreak()
{
	return (GetBreakType() != kNoBreak) && (GetBreakType() != kPREBreak);
}

bool	Glyph::IsExplicitLineBreak()
{
	switch (GetBreakType()) {
		case kHard:
		case kHardLeft:
		case kHardRight:
		case kHardAll:
		case kParagraph:
			return true;
	}
	return false;
}

short Glyph::GetBreakType()
{
	return mBreakType;
}

void Glyph::SetBreakType(BreakType breakType)
{
	mBreakType = breakType;
}

bool	Glyph::IsImage()
{
	return false;
}

bool Glyph::IsText()
{
	return false;
}

bool Glyph::IsAnchor()
{
	return false;
}

bool Glyph::IsAnchorEnd()
{
	return false;
}

bool	Glyph::Floating()
{
	return false;
}

bool	Glyph::IsSpatial()
{
	return false;
}

//	Can this glyph be separated from previous in layout?

bool Glyph::Separable()
{
	return true;
//	return IsLineBreak();
}

bool	Glyph::IsRuler()			//
{
	return false;
}

bool Glyph::IsPage()
{
	return false;
}

bool	Glyph::IsTable()			// False
{
	return false;
}

bool	Glyph::IsCell()				// False
{
	return false;
}

bool	Glyph::IsBullet()				// False
{
	return false;
}

bool	Glyph::IsDocument()				// False
{
	return false;
}

bool	Glyph::IsMargin()				// False
{
	return false;
}

bool	Glyph::ReadyForLayout()		// Images may not be ready ...
{
	return true;
}

//	Used to set html attributes in subclasses

void Glyph::SetAttribute(long, long, bool)
{
}

void Glyph::SetAttributeStr(long, const char*)
{
}

//	Returns the list of children

Glyph* Glyph::GetChildren()
{
	return 0;
}

Glyph*	Glyph::GetLastChild()
{
	return NULL;
}

//	Adds a child to the end of the list

void Glyph::AddChild(Glyph* child)
{
	child->SetParent(this);
}

void Glyph::AddChildAfter(Glyph*,Glyph*)
{
}

void Glyph::DeleteChild(Glyph* child)
{
	child->SetParent(NULL);
}

Glyph* Glyph::GetParent()
{
	return mParent;
}

void Glyph::SetParent(Glyph *parent)
{
	mParent = parent;
}

bool	Glyph::Clicked(float h, float v)
{
	BRect r;
	GetBounds(&r);
	if (h < r.left || h >= r.right) return 0;
	if (v < r.top || v >= r.bottom) return 0;
	return 1;
}

//	While tracking, hilite the selection

void Glyph::Hilite(long, DrawPort *drawPort)
{
	BRect r;
	GetBounds(&r);
	drawPort->InvertRect(&r);
}

//	Once a selection has been made, commit to that value

void Glyph::Commit(long, DrawPort *)
{
}

// ===========================================================================
//	SpatialGlyph has instance data for top, left, width, height

SpatialGlyph::SpatialGlyph(Document* htmlDoc) :
	mHTMLDoc(htmlDoc)
{
	mTop = mLeft = mWidth = mHeight = 0;
}

SpatialGlyph::~SpatialGlyph()
{
}

void SpatialGlyph::SetPRE(bool isPre)
{
	mStyle.pre = isPre;
}

const Style& SpatialGlyph::GetStyle()
{
	return mStyle;
}

void SpatialGlyph::SetStyle(Style style)
{
	mStyle = style;
}

bool	SpatialGlyph::IsSpatial()
{
	return true;
}

void SpatialGlyph::SetTop(float top)
{
	mTop = top;
}

void SpatialGlyph::SetLeft(float left)
{
	mLeft = left;
}

void SpatialGlyph::OffsetBy(const BPoint& offset)
{
	mTop += offset.y;
	mLeft += offset.x;
}

void SpatialGlyph::SetWidth(float width)
{
	mWidth = width;
}

void SpatialGlyph::SetHeight(float height)
{
	mHeight = height;
}

void SpatialGlyph::ResetLayout()
{
	mTop = mLeft = 0;
}

float SpatialGlyph::GetTop()
{
	return mTop;
}

float SpatialGlyph::GetLeft()
{
	return mLeft;
}

float SpatialGlyph::GetWidth()
{
	return mWidth;
}

float SpatialGlyph::GetHeight()
{
	return mHeight;
}

// ===========================================================================
// ===========================================================================
//	CompositeGlyph can contain other glyphs

CompositeGlyph::CompositeGlyph(Document *htmlDoc) : SpatialGlyph(htmlDoc), mAlign(AV_BASELINE)
{
	mDoesDrawing = true;
	mHasBounds = true;
}

CompositeGlyph::~CompositeGlyph()
{
}

//	Setting top or left adjusts the position of children

void CompositeGlyph::SetTop(float top)
{
	BPoint	offset(0,top - mTop);	// Offset all children this much
	Glyph*	g;

	for (g = GetChildren(); g; g = (Glyph *)g->Next())
//		g->SetTop(g->GetTop() + offset);
		if (g->HasBounds())
			g->OffsetBy(offset);
	mTop += offset.y;
}

//	Setting top or left adjusts the position of children

void CompositeGlyph::SetLeft(float left)
{
	BPoint offset(left - mLeft, 0);	// Offset all children this much
	Glyph*	g;
	for (g = GetChildren(); g; g = (Glyph *)g->Next())
//		g->SetLeft(g->GetLeft() + offset);
		if (g->HasBounds())
			g->OffsetBy(offset);
	mLeft += offset.x;
}

void CompositeGlyph::OffsetBy(const BPoint& offset)
{
	Glyph*	g;
	for (g = GetChildren(); g; g = (Glyph *)g->Next()) {
		if (g->HasBounds())
			g->OffsetBy(offset);
	}
	mLeft += offset.x;
	mTop += offset.y;
}

void CompositeGlyph::SpacingChanged()
{
	Glyph*	g;
	for (g = GetChildren(); g; g = (Glyph *)g->Next()) {
		if (g->HasBounds())
			g->SpacingChanged();
	}
}

//	Don't pass align to its offspring, use it for layout

void CompositeGlyph::SetAlign(short align)
{
	mAlign = align;
}

short CompositeGlyph::GetAlign()
{
	return mAlign;
}

//	Draw all the children before me

void CompositeGlyph::Draw(DrawPort *drawPort)
{
	Glyph*	g;
	for (g = GetChildren(); g; g = (Glyph *)g->Next())
		g->Draw(drawPort);
		
#ifdef DEBUGMENU
	if (modifiers() & B_CAPS_LOCK)
		Glyph::Draw(drawPort);				// Draw a border ¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥
#endif
}

//	Layout all the children before me

void CompositeGlyph::Layout(DrawPort *drawPort)
{
	Glyph*	g;
	for (g = GetChildren(); g; g = (Glyph *)g->Next())
		g->Layout(drawPort);
}

void CompositeGlyph::ResetLayout()
{
	Glyph*	g;
	for (g = GetChildren(); g; g = (Glyph *)g->Next())
		g->ResetLayout();
}

//	Return a list of children

Glyph*	CompositeGlyph::GetChildren()
{
	return (Glyph *)mChildren.First();
}

//	Return a list of children

Glyph*	CompositeGlyph::GetLastChild()
{
	return (Glyph *)mChildren.Last();
}

//	Add a child to the end of the list

void CompositeGlyph::AddChild(Glyph* child)
{
//	NP_ASSERT(child);
	mChildren.Add(child);
	SpatialGlyph::AddChild(child);
}

//	Add a child to the middle of the list

void CompositeGlyph::AddChildAfter(Glyph* child,Glyph* afterThis)
{
//	NP_ASSERT(child);
	mChildren.AddAfter(child,afterThis);
	SpatialGlyph::AddChild(child);
}

//	Delete a child from the list

void CompositeGlyph::DeleteChild(Glyph* child)
{
//	NP_ASSERT(child);
	child->SetParent(NULL);
	mChildren.Delete(child);
}

Glyph *CompositeGlyph::GetParent()
{
	return mParent;
}

void CompositeGlyph::SetParent(Glyph *parent)
{
	mParent = parent;
}

// Used by PageGlyph to manage anchors

void CompositeGlyph::SetCurrentAnchor(Glyph*)
{
}

// ===========================================================================
// ===========================================================================
// Line breaks are fairly simple

LineBreakGlyph::LineBreakGlyph()
{
	mBreakType = kHard;	// Hard break by default
}

// ===========================================================================
//	AlignmentGlyph defines alignment of subsequent glyphs
//	It has a parent so it can set alignment of its parent when it lays out

AlignmentGlyph::AlignmentGlyph()
{
	mParent = NULL;
	mAlign = AV_BASELINE;
	mBreakType = kHardAlign;
}

#ifdef DEBUGMENU
void AlignmentGlyph::PrintStr(BString& print)
{
	LineBreakGlyph::PrintStr(print);
	print += " ";
	print += AttributeValueName(mAlign);
}
#endif

void AlignmentGlyph::SetAlign(short align)
{
	mAlign = align;
}

void AlignmentGlyph::Layout(DrawPort *)	// Set the alignment of parent
{
	if (GetParent())
		GetParent()->SetAlign(mAlign);
}

Glyph* AlignmentGlyph::GetParent()
{
	return mParent;
}

void AlignmentGlyph::SetParent(Glyph *parent)
{
	mParent = parent;
}
