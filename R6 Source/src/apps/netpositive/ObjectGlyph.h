// ===========================================================================
//	ObjectGlyph.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1997 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __OBJECTG__
#define __OBJECTG__

#include "Glyph.h"

class Tag;
class BMessage;
class BView;

// ===========================================================================
//	ImageGlyph is an image!

class ObjectGlyph : public SpatialGlyph {
public:
static ObjectGlyph*	CreateObjectGlyph(Tag *tag, const char *tagText, Document* htmlDoc, BMessage *pluginData);
		virtual		~ObjectGlyph();

#ifdef DEBUGMENU
	virtual	void	PrintStr(BString& print);
#endif
	virtual	void	GetName(BString& name);
	virtual	bool	Floating();

	virtual	bool	Clicked(float h, float v);
			bool	GetClickCoord(float* h, float* v);

	virtual	float	GetWidth();
	virtual	float	GetHeight();
	virtual	short	GetAlign();

			void	GetObjectBounds(BRect &r);	// Inset by mHSpace, mVSpace and border
			int		GetBorder();

	virtual	void	Hilite(long value, DrawPort *drawPort);
			void	DrawBorder(bool selected, DrawPort *drawPort);
			
	virtual	void	Draw(DrawPort *drawPort);
	virtual	void	Layout(DrawPort *drawPort);

	virtual	void	SetAttribute(long attributeID, long value, bool isPercentage);
	virtual	void	SetAttributeStr(long attributeID, const char* value);

			void	SetURL(const char *url);
		const char	*URL();
			void	GetSRC(BString &string);

			void	SetTag(const char *tag);

protected:
					ObjectGlyph(BView *pluginView, Document *htmlDoc);
			short		mBorder;
			short		mHSpace;
			short		mVSpace;
			short		mAlign;
			
			BString		mClassID;
			BString		mURL;
			BString		mCodeBase;
			BString		mCode;
			BString		mData;
			BString		mName;
			BString		mTag;
			BView*		mObjectView;
			
			bool		mHidden;
			
			float	mClickH;
			float	mClickV;
};

/*
class ReplicantGlyph : public ObjectGlyph {
public:
					ReplicantGlyph(Document* htmlDoc);
		virtual		~ReplicantGlyph();
	virtual	void	SetAttributeStr(long attributeID, const char* value);
			void	SetReplicantArchive(BMessage *archive) {mReplicantArchive = archive;}
	virtual	void	Draw(DrawPort *drawPort);
	virtual	void	Layout(DrawPort *drawPort);

protected:
		BView*			mReplicantView;
		BMessage*		mReplicantArchive;
};
*/

#endif
