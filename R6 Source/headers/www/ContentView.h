
#ifndef CONTENTVIEW_H
#define CONTENTVIEW_H

#include <View.h>
#include <URL.h>
#include <ResourceCache.h>

namespace Wagner {

enum {
	bmsgLinkTo			= 'ltct',
	bmsgLinkToFragment	= 'frag',
	bmsgClearHistory	= 'clrh',
	bmsgGoBack			= 'gobk',
	bmsgGoForward		= 'gofw',
	bmsgGoOffset		= 'goof',
	bmsgGoNearest		= 'gonr',
	bmsgReload			= 'reld',
	bmsgStop			= 'stop',
	bmsgJSCloseWindow	= 'jscw',
	bmsgNotifyInstance	= 'ntin',
	bmsgStateChange		= 'stch',
	bmsgBroadcast		= 'bcst',
	bmsgDeselectAll		= 'dsel',
	// this is sent to the application when a new page is being
	// opened.
	bmsgPageOpening		= 'popg'
};

enum {
	cvoDoubleBuffer	= 0x00000001,
	cvoHScrollbar	= 0x00000002,
	cvoVScrollbar	= 0x00000004,
	cvoIsTop		= 0x00000008,
	cvoIsClosable	= 0x00000010
};

const int32 USER_ACTION			=	0x00010000;
const int32 RECORD_IN_HISTORY	=	0x00020000;

class History;
class RealContentView;
class ContentInstance;

class ContentView : public BView
{
				int32					m_width,m_height;
				uint32					m_flags;
				RealContentView *		m_contentView;
				BScrollBar *			m_scrollH;
				BScrollBar *			m_scrollV;
				uint32					m_padding[6];

				void					LayoutChildren();
				void					LayoutChildren(bool h_scroll, bool v_scroll);
				
	public:

										ContentView(BRect r, const char *name, uint32 follow, uint32 flags, uint32 options=0, History *history=NULL);
		virtual							~ContentView();

				status_t				SetContent(
											const URL &url,
											uint32 flags= LOAD_ON_ERROR | USER_ACTION | RECORD_IN_HISTORY,
											GroupID = -1,
											BMessage *userData = NULL);
				status_t				SetContent(ContentInstance *content);
				ContentInstance *		GetContentInstance() const;
				ContentInstance *		GetTopContentInstance() const;
				ContentInstance *		GetParentContentInstance() const;
				RealContentView *		GetRealContentView() { return m_contentView; };
				bool					IsInContentArea() const;
				bool					IsInMailAttachmentArea() const;
				const char *			GetTopContentViewName() const;
				
				void					SetIndex(int32 index);
				int32					Index();

		virtual	void					AttachedToWindow();
		virtual	void					FrameResized(float width, float height);
				void					MessageReceived(BMessage *msg);		
				void					GetAvailSize(int32* width, int32* height);
				void					GetScrollbarDimens(int32* width, int32* height);
				void					SetScrollbarState(bool h, bool v);
				void					GetScrollbarState(bool* h, bool* v);
				
				void					RestoreContent(BMessage *restore);
				
				void					Reload();
				void					StopLoading();
				void					GoForward();
				void					GoBackward();
				void					Rewind();
				void					Print();
				void					GetURL(URL *url);

				void					GetPreviousURL(URL *url);
				void					GetNextURL(URL *url);
				void					GetHistoryLength(int32 *length);
				void					ClearHistory();
				
				
				// This returns true if the frame is part of the local UI.  It checks
				// to see if the frame is below the _be:content frame.
				bool					IsPrivate() const;
};

}

#endif
