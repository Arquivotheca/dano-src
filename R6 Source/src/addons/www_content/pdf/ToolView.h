#ifndef _TOOL_VIEW_H_
#define _TOOL_VIEW_H_

#include <View.h>
#include "DocFramework.h"
#include <Messenger.h>
#include <String.h>

#include <ResourceCache.h>
#include <Content.h>

using namespace Wagner;
extern const float stdHeight;
class BPopUpMenu;
class BTextControl;
class BMessageRunner;
enum {
	iFirstPage = 0,
	iPrevPage = 1,
	iNextPage = 2,
	iLastPage = 3,
	iCurrentPage = 4,
	iScale = 5,
#if PDF_PRINTING > 0
	iPrint = 6,
#endif
	iFirstPageDown = 10,
	iPrevPageDown = 11,
	iNextPageDown = 12,
	iLastPageDown = 13
};
const int32 iArtCount = 4;

#if PDF_PRINTING > 0
const int32 iZoneCount = 7;
#else
const int32 iZoneCount = 6;
#endif

class ToolView : public BView
{
	public:
								ToolView();
								~ToolView();
								
		void					SetFramework(DocFramework *framework);						
		void					MessageReceived(BMessage *msg);
		void					MouseDown(BPoint where);
		void					MouseUp(BPoint where);

		void					DeliverArt(int32 id, ContentInstance *instance);
		void					Draw(BRect updateRect);
		void					AttachedToWindow();
		
		bool					AllArtArrived();
		void					FrameResized(float width, float height);
		void					DocSizeChanged(float scale);
		void					DocPageChanged(uint32 pageNum);
	private:
		DocFramework *			fFramework;
		
		BRect					fBounds;
		BPopUpMenu *			fScaleMenu;
		BTextControl *			fPageNum;
		BRect					fZones[iZoneCount];		
		int32 					fLastButton;

		ContentInstance *		fArt[iArtCount];
		ContentInstance *		fDownArt[iArtCount];
		BMessageRunner *		fRunner;
		bool					fInitialShowing;
		uint32					fPageCount;
		uint32					fCurrentPage;
		float					fScale;
};

#endif
