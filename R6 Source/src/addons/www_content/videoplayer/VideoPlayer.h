#ifndef VideoPlayer_h
#define VideoPlayer_h

#include <Content.h>
#include <View.h>
#include <Locker.h>
#include <Protocol.h>
#include <list>

#include <Bitmap.h>
#include <ContentView.h>
#include <Debug.h>
#include <Font.h>
#include <Resource.h>
#include <ResourceCache.h>
#include <stdio.h>
#include <TellBrowser.h>
#include <URL.h>
#include <View.h>
#include <www/util.h>
#include <Autolock.h>

#include <Content.h>
#include <Handler.h>
#include <Binder.h>
#include "GHandler.h"

#include "ForwardIO.h"
#include "VideoView.h"
//#include "VideoDecoder.h"

class BBitmap;
class BMediaFile;
class BMediaTrack;
class BSoundPlayer;

using namespace Wagner;
class VideoContentInstance;
class VideoContent;
class VideoContentFactory;
class VideoDecoder;
class AudioDecoder;


enum widget_id {
	kPlayUp,
	kPlayOver,
	kPlayDown,
	kPlayDisabled,
	kPauseUp,
	kPauseOver,
	kPauseDown,
	kPauseDisabled,
	kStopUp,
	kStopOver,
	kStopDown,
	kStopDisabled,
	kInfoLeft,
	kInfoRight,
	
	kWidgetCount
};

class VideoContentInstance : public ContentInstance ,public GHandler {
public:
	VideoContentInstance(VideoContent *content, GHandler *handler);
	~VideoContentInstance();
	virtual	status_t AttachedToView(BView *view, uint32 *contentFlags);
	virtual	status_t DetachedFromView();
	virtual	status_t FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);
	virtual status_t Draw(BView *into, BRect exposed);
	virtual status_t GetSize(int32 *width, int32 *height, uint32 *flags);
	virtual	void MouseDown(BPoint where, const BMessage *event=NULL);
	virtual	void MouseMoved(BPoint where, uint32 code, const BMessage *a_message, const BMessage *event=NULL);
	virtual	void MouseUp(BPoint where, const BMessage *event=NULL);
	
	void Reset();
	void SetTime(bigtime_t t);

protected:
	virtual	status_t HandleMessage(BMessage*);
	virtual void Notification(BMessage*);
	virtual void Cleanup();

private:
	typedef list<URL> url_list;
	url_list fURLList;
	int32 fContentLength;
	
	ForwardIO * 			myIO;
	int32					fWaitFlags;
	thread_id				fReadAheadThread;
	static int32 			StartReadAhead(void *castToVideoContentInstance);
	void 					ReadAhead();
	
	void 					LoadBitmap(widget_id, const char*);

	


	ContentInstance *		fWidgets[kWidgetCount];
	VideoContent *			m_daddy;
	BWindow *				m_window;
	BView	*				m_view;
	int32 					m_downloading;
	int32 					m_hsize,m_vsize;
	int32 					m_boxwidth,m_boxheight;
	int32 					m_xbox,m_ybox;
	
	
	BRect					m_rectfull;
	BFont					m_font;
	Protocol 				*fProtocol;
	int32 					m_loaded;
	BMediaFile *			m_file;
	
	BBitmap *				m_bitmap;
	rgb_color				m_key;
	VideoView *					m_oview;

	bool					m_play;
	bigtime_t				m_time;
	
	VideoDecoder * 			m_videoDecoder;
	AudioDecoder * 			m_audioDecoder;
	
	BLocker					m_lock;
	
	bool					m_useoverlay;
	
	friend class VideoContent;

};




class VideoContent : public Content {
public:
	VideoContent(void* handle);
	virtual ~VideoContent();
	virtual size_t GetMemoryUsage();
	virtual	bool IsInitialized();
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&);

	
private:
	VideoContentInstance * 	m_vci; 
};



class VideoContentFactory : public ContentFactory
{
public:
	VideoContentFactory();
	virtual void GetIdentifiers(BMessage* into);
	
	virtual Content* CreateContent(void* handle, const char* /* mime */, const char* /* extension */);
	virtual bool KeepLoaded() const;
	
private:
	VideoContent *	m_content;
	
};

#endif
