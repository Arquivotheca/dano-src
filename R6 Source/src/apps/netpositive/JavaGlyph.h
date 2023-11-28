// ===========================================================================
//  JavaGlyph.h
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifndef __JAVAG__
#define __JAVAG__

#include "ObjectGlyph.h"

#include <SupportDefs.h>

class MWAppletBuddy;
class JavaAppletMonitor;


class JavaGlyph : public ObjectGlyph {
public:
						JavaGlyph(Document* htmlDoc);
	virtual				~JavaGlyph();

	virtual void		Draw(DrawPort *drawPort);
	virtual void		Layout(DrawPort *drawPort);

	static void			CleanupJava();

private:
	MWAppletBuddy		*mAppletBuddy;
	JavaAppletMonitor	*mAppletMonitor;
	int32				mLayoutState;

	static int32		sJavaCount;				
};

#endif
