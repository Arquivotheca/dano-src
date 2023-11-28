//
//	Browser interface
//
#include <Bitmap.h>
#include <View.h>
#include <Debug.h>
#include <stdio.h>
#include <Message.h>
#include <Screen.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <DataIO.h>
#include <Locker.h>
#include <Autolock.h>
#include <SoundPlayer.h>
#include <ResourceCache.h>
#include <Window.h>

#include "PasswordManager.h"
#include "VideoPlayer.h"
#include "VideoDecoder.h"


#define video_grey 63
#define bar_grey 178

const URL kResourceBase("file://$RESOURCES/MediaBar/");
BLocker videocontentlocker;
// ----------------------- VideoContentInstance -----------------------


VideoContentInstance::VideoContentInstance(VideoContent *content, GHandler *handler) :
	ContentInstance(content, handler),
	GHandler(*handler),
	m_daddy(content),
	m_window(NULL),
	m_view(NULL),
	m_downloading(1),
	m_rectfull(0,0,0,0),
	m_font(),
	fProtocol(0),
	m_loaded(0),
	m_file(NULL),
	m_bitmap(0),
	m_oview(0),
	m_videoDecoder(0),
	m_audioDecoder(0),
	m_play(false),
	m_time(0),
	m_lock(),
	m_useoverlay(false)
{
	//printf("VideoContentInstance::VideoContentInstance() \n");
	//m_view = new BView( BRect(0,0,320-1,240+36+2-1), "Instance", B_FOLLOW_ALL,
	//	B_WILL_DRAW|B_FRAME_EVENTS|B_FULL_UPDATE_ON_RESIZE);
	//m_view->SetViewColor(182,187,166,255);
	m_hsize = 0;
	m_vsize = 0;
	m_boxwidth = 320;
	m_boxheight = 240+36+2;
	for(int i =0;i<kWidgetCount;i++)
	{
		fWidgets[i] = 0;
	}
	LoadBitmap(kPlayUp, "mediaplay_up.png");
	LoadBitmap(kPlayOver, "mediaplay_over.png");
	LoadBitmap(kPlayDown, "mediaplay_down.png");
	LoadBitmap(kPlayDisabled, "mediaplay_disabled.png");
	
	LoadBitmap(kPauseUp, "mediapause_up.png");
	LoadBitmap(kPauseOver, "mediapause_over.png");
	LoadBitmap(kPauseDown, "mediapause_down.png");
	LoadBitmap(kPauseDisabled, "mediapause_disabled.png");
	
	LoadBitmap(kStopUp, "mediastop_up.png");
	LoadBitmap(kStopOver, "mediastop_over.png");
	LoadBitmap(kStopDown, "mediastop_down.png");
	LoadBitmap(kStopDisabled, "mediastop_disabled.png");
	
	LoadBitmap(kInfoLeft, "infoEndcapLeft.png");
	LoadBitmap(kInfoRight, "infoEndcapRight.png");
	
	
	
	
	
	
	
	status_t err = m_font.SetFamilyAndStyle("Swis721 BT","Bold");
	if(err<0)
	{
		printf("erreur font\n");
	}
	else
	{
		//printf("Font OK\n");
	}
	m_font.SetSize(12.0);
	
	
	

	
}


VideoContentInstance::~VideoContentInstance()
{
	//printf("VideoContentInstance::~VideoContentInstance()\n");
	for (int widgetIndex = 0; widgetIndex < kWidgetCount; widgetIndex++)
			if (fWidgets[widgetIndex])
			{
				//printf("releasing widget %ld\n",widgetIndex);
				fWidgets[widgetIndex]->Release();
			}
	//delete m_view;
	if(m_oview)
	{	
		m_oview->RemoveSelf();
		delete m_oview;
	}
}


status_t 
VideoContentInstance::AttachedToView(BView *view, uint32 *contentFlags)
{
	//printf("VideoContentInstance::AttachedToView()%ld %ld\n",FullWidth(),FullHeight());
	
	
	BAutolock lock(m_lock);
	
	SetTime(0);	
	
	//	assuming the window is locked here
	BRect r = FrameInParent();
	//printf("Frame in parent %f %f %f %f\n",r.left,r.right,r.top,r.bottom);
	//view->AddChild(m_view);
	//m_view->SetViewColor(182,187,166,255);
	m_view = view;
	m_window = m_view->Window();
	m_view->SetFont(&m_font);
	
	m_downloading = 1;
	//loading the good URL
	//printf("New protocol \n");
	delete fProtocol;
	fProtocol = 0;
	URL myURL = GetContent()->GetResource()->GetURL();
	
	/*
	URL fRefer = GetContent()->GetResource()->GetURL();
	
	status_t status = B_OK;
	const char *scheme = myURL.GetScheme();
	
	while (fProtocol == NULL) {
		fProtocol = Protocol::InstantiateProtocol(scheme);
		if (fProtocol == NULL)
			break;
		
		BMessage msg;
		status = fProtocol->Open(myURL, fRefer, &msg, PRIVATE_COPY | RANDOM_ACCESS);
		if (status < B_OK) {
			delete fProtocol; fProtocol = 0;
			break;
		}	

		bigtime_t delay;
		if (fProtocol->GetRedirectURL(myURL, &delay)) {
			delete fProtocol; fProtocol = 0;
			continue;
		}
	}
	*/
	
	fURLList.push_front(myURL);
	
	status_t error;
	bool authenticationretry = false;

	for (;;) {
		URL currentURL = fURLList.front();
		// Open a connection
		fProtocol = Protocol::InstantiateProtocol(currentURL.GetScheme());
		if (fProtocol == 0)
			return B_ERROR;
		
		//fProtocol->SetMetaCallback(this, ProcessMeta);

		BMessage msg;
		char contentType[B_MIME_TYPE_LENGTH];
		
		
		//
		// HACK
		//
		// Try to connect twice, once not sending the user agent string and once sending it.
		// Shoutcast will not send mpeg streams if you have the user agent
		// It won't send you the playlist unless you do (it even returns 404 in this case).
		// Try first without it just in case this is an audio stream
		// NOTE: The second rep of this loop only gets executed if this is an invalid URL or for new shoutcast
		// servers.
		//
		for (int retry = 0; retry < 2; retry++) {
			error = fProtocol->Open(currentURL, currentURL, &msg, retry == 0 ? TERSE_HEADER : 0);
			if (error >= B_OK)
				break;
		}
		
		if (error == B_AUTHENTICATION_ERROR && !authenticationretry) {
			// Handle authentication.  This is kind of an inefficient way
			// to do it, as we always fail and retry.
			const char *challenge = "";
			msg.FindString(S_CHALLENGE_STRING, &challenge);
			
			// Look up realm...
			BString user, password;
			if (passwordManager.GetPassword(currentURL.GetHostName(), challenge, &user, &password)) {
				URL augmentedURL(currentURL.GetScheme(), currentURL.GetHostName(), currentURL.GetPath(),
					currentURL.GetPort(), currentURL.GetFragment(), user.String(), password.String(),
					currentURL.GetQuery(), currentURL.GetQueryMethod());
	
				delete fProtocol;
				fProtocol = 0;
				fURLList.pop_front();
				fURLList.push_front(augmentedURL);
				msg.MakeEmpty();
				authenticationretry = true;
				continue; // try again with authentication
			}
		}

		if (error < 0)
			return error;
		authenticationretry = false;
		
		bigtime_t delay;
		if (fProtocol->GetRedirectURL(currentURL, &delay)) {
			delete fProtocol;
			fProtocol = 0;
			// replace the old URL with the redirected version
			fURLList.pop_front();
			fURLList.push_front(currentURL);
			continue;		// This is a redirect, loop and continue
		}
	
		break;
	}

	fURLList.pop_front();
	
	
	fContentLength = fProtocol->GetContentLength();
	
	
	
	//printf("fContentLength %ld\n",(int32)fContentLength);
	if((fContentLength < 1024*1024*50) && (fContentLength>=0))
	{
		myIO = new ForwardIO((ssize_t)fContentLength);
		m_downloading = 3;
	}
	else
	{
		myIO = new ForwardIO((ssize_t)0);
		m_downloading = 1;
	}
	
	
	
	
	
	fWaitFlags = 0;
	fReadAheadThread = spawn_thread(StartReadAhead, "File data", 10 , this);
	resume_thread(fReadAheadThread);
	
	//printf("Thread launched\n");
	return B_OK;
}


int32 VideoContentInstance::StartReadAhead(void *castToVideoContentInstance)
{
	reinterpret_cast<VideoContentInstance*>(castToVideoContentInstance)->ReadAhead();
	return 0;
}

void VideoContentInstance::ReadAhead()
{
	char  * pt = new char [50*B_PAGE_SIZE];

	int32 sizeread, size,tsize;
	
	sizeread = 1;
	size = 50 * B_PAGE_SIZE;
	tsize = 0;
	
	//printf("Begin to load in the thread \n");
	
	while((fWaitFlags == 0) && ( sizeread >0))
	{
		//printf("Loading %ld %ld\n",fWaitFlags,sizeread);
		sizeread = fProtocol->Read((void*)pt,size);
		if(sizeread > 0)
		{
			myIO->Write((void*)pt,sizeread);
			tsize += sizeread;
		}
		//printf("Loading read\n");
		if((fContentLength < 1024*1024*50) && (fContentLength>=0))
		{
			//printf("fContentLength>0 %ld %ld\n",(int32)fContentLength,(int32)tsize);
			float f = (float)(tsize)/(float)(fContentLength)*100.0;
			if(f>(m_loaded+5))
			{
				//printf("f>(m_loaded+5)\n");
				m_loaded = (int32)f;
				if(m_window)
				{
					//printf("Invalidate %f %ld\n",f,m_downloading);
					if(m_window->LockWithTimeout(500000) == B_OK)
					{
						//printf("Win lock\n");
						BRect r(m_xbox+2+13+250,m_ybox+m_boxheight+2-36+4,m_xbox+2+291,m_ybox+m_boxheight+2-36+4+27);
						m_view->SetHighColor(0,0,0,255);
						m_view->FillRect(r);
				
					
						BPoint pt(m_xbox+2+13+250,m_ybox+m_boxheight+2-36+4+20);
						drawing_mode dmode = m_view->DrawingMode();
						m_view->SetDrawingMode(B_OP_COPY);
						m_view->SetHighColor(0,102,255,255);
						m_view->SetLowColor(0,0,0,255);
						char c;
						if(m_loaded<10)
						{
							m_view->DrawChar(' ',pt);
							c = 48 + m_loaded;
							m_view->DrawChar(c);
							m_view->DrawChar('%');
						}
						else
						{
							if(m_loaded<100)
							{
								float f = (float)(m_loaded)/10.0;
								c = 48 + (char)(f);
								m_view->DrawChar(c,pt);
								f = (char)(f);
								f = (float)(m_loaded) - f * 10.0; 
								c = 48 + (char)(f);
								m_view->DrawChar(c);
								m_view->DrawChar('%');
							}
						}
						m_view->SetDrawingMode(dmode);
						m_view->Sync();
						m_window->Unlock();
					}
					else
					{
						//printf("could not lock window\n");
					}
					
				}
			}
		}
		//printf("End loop\n");
	}
	
	if (fProtocol)
		fProtocol->Abort();

	// Free up resources
	delete fProtocol;
	fProtocol = 0;
	
	if(fWaitFlags == 0)
	{
		m_downloading = 1;
		//printf("Video loaded \n");
		myIO->Info();
	
		m_file = new BMediaFile(myIO);
	
		status_t err = m_file->InitCheck();
		if(err < 0)
		{
			printf("BMediaFile error\n");
			delete [] pt;
			return;
		}
		
		//printf("Nb tracks %ld\n",m_file->CountTracks());
		for (int ix=0; ix<m_file->CountTracks(); ix++)
		{
			BMediaTrack * t = m_file->TrackAt(ix);
			media_format fmt;
			if (!t->DecodedFormat(&fmt))
			{
				if (fmt.type == B_MEDIA_RAW_AUDIO)
				{
						//printf("VideoContentInstance::ReadAhead() found audio\n");
						m_audioDecoder = new AudioDecoder(this,fmt.u.raw_audio, t);
				}
				else if (fmt.type == B_MEDIA_RAW_VIDEO)
				{
					fmt.type = B_MEDIA_RAW_VIDEO;
					fmt.u.raw_video = media_raw_video_format::wildcard;
					fmt.u.raw_video.display.format = B_YCbCr422;
					if (!t->DecodedFormat(&fmt) && fmt.type == B_MEDIA_RAW_VIDEO) {
						printf("VideoContentInstance::ReadAhead() found  video %ld %ld\n", fmt.u.raw_video.display.line_width,fmt.u.raw_video.display.line_count);
						
						m_bitmap = new BBitmap(
								BRect(0, 0, fmt.u.raw_video.display.line_width-1, fmt.u.raw_video.display.line_count-1),
								#if ROTATE_DISPLAY
								B_BITMAP_WILL_OVERLAY| B_BITMAP_RESERVE_OVERLAY_CHANNEL | 0x40000000,
								#else
								B_BITMAP_WILL_OVERLAY| B_BITMAP_RESERVE_OVERLAY_CHANNEL ,
								#endif
								B_YCbCr422,
								fmt.u.raw_video.display.line_count*2);
								
					
								
								
						if(m_bitmap->InitCheck()<0)
						{
							//printf("VideoPlayer Bitmap error\n");
							m_useoverlay = false;
							delete m_bitmap;
							fmt.type = B_MEDIA_RAW_VIDEO;
							fmt.u.raw_video = media_raw_video_format::wildcard;
							if (!t->DecodedFormat(&fmt) && fmt.type == B_MEDIA_RAW_VIDEO) {
						
							m_bitmap = new BBitmap(
								BRect(0, 0, fmt.u.raw_video.display.line_width-1, fmt.u.raw_video.display.line_count-1),
								0,
								B_RGBA32,
								fmt.u.raw_video.display.line_count*4);
							

							}
							
						}
						else
						{
							printf("VideoPlayer decodage ok %ld\n",m_bitmap->BytesPerRow());
							m_useoverlay = true;
							
						}
						
					}
				
					if(fmt.u.raw_video.display.line_width <= 320)
					{
						float y = fmt.u.raw_video.display.line_count;
						float x = fmt.u.raw_video.display.line_width;
						y = y * 320.0 / x;
						m_boxwidth = 320;
						m_boxheight = y;
					}
					else
					{
						m_boxwidth = fmt.u.raw_video.display.line_width;
						m_boxheight = fmt.u.raw_video.display.line_count;
					}
					
					//printf("New box size %ld %ld ",m_boxwidth,m_boxheight);
					
					m_boxheight += 36 + 2;
				
					if(m_hsize < m_boxwidth+4+5)
					{
						m_hsize =  m_boxwidth+4+5;
					}
				
					if(m_vsize < m_boxheight+4+5)
					{
						m_vsize =  m_boxheight+4+5;
					}
	
					if(m_hsize>m_boxwidth+4)
					{
						m_xbox = m_hsize - m_boxwidth - 4;
						m_xbox = m_xbox / 2;
					}
					else
					{
						m_xbox = 0;
					}
			
					if(m_vsize>m_boxheight+4)
					{
						m_ybox = m_vsize - m_boxheight - 4;
						m_ybox = m_ybox / 2;
					}
					else
					{
						m_ybox = 0;
					}
				
				
					if(m_useoverlay)
					{
						m_oview = new VideoView(BRect(m_xbox+2,m_ybox+2,m_xbox+m_boxwidth+1,m_ybox+m_boxheight+1-36-2), 
											"Instance",
											 0,
											 B_WILL_DRAW|B_FRAME_EVENTS|B_FULL_UPDATE_ON_RESIZE,
											 NULL);
						if(m_window->Lock())
						{
							m_view->AddChild(m_oview);
							m_oview->SetViewOverlay(m_bitmap,
							   	   			m_bitmap->Bounds(),
							       			m_oview->Bounds(),
							       			&m_key,
							       			B_FOLLOW_ALL,
							       			B_OVERLAY_FILTER_HORIZONTAL|B_OVERLAY_FILTER_VERTICAL);

						//memset(m_bitmap->Bits(),fmt.u.raw_video.display.line_width*fmt.u.raw_video.display.line_count*2,0);
							unsigned char * pt = (unsigned char*)m_bitmap->Bits();
							for(int32 j=0;j<fmt.u.raw_video.display.line_count;j++)
							{
								for(int32 i=0;i<fmt.u.raw_video.display.line_width;i++)
								{
									*pt = 0;
									pt ++;
									*pt = 50;
									pt ++;
								}
							}
	
							m_oview->SetViewColor(m_key);
					
				
							m_downloading = 4 + 32;
							m_oview->Invalidate();
							m_view->Invalidate();
							m_window->Unlock();
						}
					}
					else
					{
						m_oview = new VideoView(BRect(m_xbox+2,m_ybox+2,m_xbox+m_boxwidth+1,m_ybox+m_boxheight+1-36-2), 
											"Instance",
											 0,
											 B_WILL_DRAW|B_FRAME_EVENTS|B_FULL_UPDATE_ON_RESIZE,
											 m_bitmap);
											 
						if(m_window->Lock())
						{
							m_view->AddChild(m_oview);					 
							m_downloading = 4 + 32;
							m_oview->Invalidate();
							m_view->Invalidate();
							m_window->Unlock();
						}
					}
					
					m_videoDecoder = new VideoDecoder(this,m_bitmap,m_oview,m_useoverlay,fmt.u.raw_video, t);	
				}
				else
				{
					//printf("VideoContentInstance::ReadAhead() unknown track\n");	
					m_file->ReleaseTrack(t);
				}
			}
		}
	
	
	
		if (!m_audioDecoder && !m_videoDecoder)
		{
			delete m_file;
			m_file = 0;
			delete myIO;
			myIO = 0;
			//printf("Nothing inside \n");
		}

		ASSERT(m_videoDecoder != NULL);

		if (!m_audioDecoder)
		{
			//atomic_or(&m_complete, kAudioDone);
			//FPRINTF((stderr, "VideoContent::Feed(): creating silent audio decoder\n"));
			media_raw_audio_format fmt = media_raw_audio_format::wildcard;
			fmt.frame_rate = 22050.0;
			fmt.channel_count = 2;
			fmt.format = 0x2;
			m_audioDecoder = new AudioDecoder(this,fmt, NULL);
	
		}
		if(m_videoDecoder==0)
		{
			printf("Video error \n");
		}
	
		m_videoDecoder->DecodeFrame();
		//m_videoDecoder->DecodeFrame();
		//m_videoDecoder->DecodeFrame();
		m_audioDecoder->AddVideoDecoder(m_videoDecoder);
	
		m_play = true;
		m_audioDecoder->SetPlay(m_play);
	
	
	
	
	
	}
	else
	{
		//printf("Thread stopped\n");
	}
	//printf("End thread lauding\n");
	delete [] pt;
}


void VideoContentInstance::Reset()
{
	//printf("VideoContentInstance::Reset\n");
	m_downloading = 4 + 256;
	m_play = false;
	
	
	BRect r1(m_xbox+2+13+96,m_ybox+m_boxheight+2-36+4,m_xbox+2+13+96+50,m_ybox+m_boxheight+2-36+4+27);
	BRect r2(m_xbox+2+55,m_ybox+m_boxheight+2-36+4,m_xbox+2+31+55,m_ybox+m_boxheight+2-36+4+31);
	
	if(m_window)
	{
		if(m_window->Lock())
		{
			if(fWidgets[kPlayUp])
			{
				#if SONY_MPEG_BACKGROUND
				m_view->SetHighColor(182,187,166,255);
				#else
				m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
				#endif
				m_view->FillRect(r2);
				fWidgets[kPlayUp]->FrameChanged(r2,32,32);
				fWidgets[kPlayUp]->Draw(m_view,r2);	
			}
			
			
			m_view->SetHighColor(0,0,0,255);
			m_view->FillRect(r1);
			
			
			BPoint pt(m_xbox+2+13+96,m_ybox+m_boxheight+1-36+4+20);
			m_view->SetDrawingMode(B_OP_COPY);
			m_view->SetHighColor(0,102,255,255);
			m_view->SetLowColor(0,0,0,255);
			m_view->DrawString("Stopped",pt);
			
			
			
			m_view->Sync();
			m_window->Unlock();
		}
	}
			
	if(m_videoDecoder)
		m_videoDecoder->Reset();
	if(m_audioDecoder)
		m_audioDecoder->Reset();
	
	SetTime(0);
	m_audioDecoder->SetPlay(m_play);
}


void VideoContentInstance::SetTime(bigtime_t t)
{
	bigtime_t tt = t / 1000000.0; 
	if(tt == m_time)
	{
		return;
	}
	
	m_time = tt;
	
	if(m_window)
	{
		if(m_window->LockWithTimeout(500000)==B_OK)
		{
			float t = m_time;
			float mn = t / 60.0;
			float h = t / 3600.0;
			int ih = h;
			int imn = mn;
			imn = imn % 60;
			int it = t;
			it = it % 60;
			
			{
				BRect r(m_xbox+m_boxwidth-80,m_ybox+m_boxheight+2-36+4,m_xbox+m_boxwidth+2-30-1,m_ybox+m_boxheight+2-36+4+27);
				m_view->SetHighColor(0,0,0,255);
				m_view->FillRect(r);
			}
	
			
			if(ih != 0)
			{
				BPoint pt(m_xbox+m_boxwidth-78,m_ybox+m_boxheight+2-36+4+20-1);
				m_view->SetDrawingMode(B_OP_COPY);
				m_view->SetHighColor(0,102,255,255);
				m_view->SetLowColor(0,0,0,255);
				char c = (ih / 10) + 48;
				m_view->DrawChar(c,pt);
				c = (ih % 10) + 48;
				m_view->DrawChar(c);
				m_view->DrawChar(':');	
			}
			
			{
				BPoint pt(m_xbox+m_boxwidth-60,m_ybox+m_boxheight+2-36+4+20-1);
				m_view->SetDrawingMode(B_OP_COPY);
				m_view->SetHighColor(0,102,255,255);
				m_view->SetLowColor(0,0,0,255);
				char c = (imn / 10) + 48;
				m_view->DrawChar(c,pt);
				c = (imn % 10) + 48;
				m_view->DrawChar(c);
				m_view->DrawChar(':');
				c = (it / 10) + 48;
				m_view->DrawChar(c);
				c = (it % 10) + 48;
				m_view->DrawChar(c);
			}
			m_view->Sync();
			m_window->Unlock();
		}
	}
}

status_t 
VideoContentInstance::DetachedFromView()
{
	//printf("VideoContentInstance::DetachedFromView()\n");
	
	BAutolock lock(m_lock); 
	//	assuming the window is locked here
	//m_view->RemoveSelf();
	m_view = NULL;
	m_window = NULL;
	
	m_play = false;
	
	m_time = 0;
	
	atomic_or(&fWaitFlags, 1);
	
	status_t err;
	wait_for_thread(fReadAheadThread, &err);
	
	//printf("VideoContentInstance::DetachedFromView thread gone\n");
	
	if(m_audioDecoder)
	{
		m_audioDecoder->RemoveVideoDecoder();
	}
	delete m_audioDecoder;
	m_audioDecoder = 0;
	
	delete m_videoDecoder;
	m_videoDecoder = 0;
	
	if(m_oview)
	{
		m_oview->SetBitmap(NULL);
	}
	//printf("Audio and video gone\n");
	delete m_bitmap;
	m_bitmap = 0;
	
	if(m_oview)
	{
		m_oview->RemoveSelf();
	}
	delete m_oview;
	m_oview = 0;
	//printf("oview gone\n");
	
	delete m_file;
	m_file = NULL;
	
	delete myIO;
	
	if (fProtocol)
		fProtocol->Abort();

	// Free up resources
	delete fProtocol;
	fProtocol = 0;
	
	//printf("protocol gone \n");
	
	return B_OK;
}

status_t 
VideoContentInstance::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
	//	assuming the window is locked here
	//printf("VideoContentInstance::FrameChanged %f %f %f %f %ld %ld\n",newFrame.left,newFrame.top,newFrame.Width(),newFrame.Height(),fullWidth,fullHeight);
	
	//m_view->ResizeTo(fullWidth, fullHeight);
	//m_view->MoveTo(0,0);
	if(fullWidth < m_boxwidth+4+5)
	{
		m_hsize =  m_boxwidth+4+5;
	}
	else
	{
		m_hsize = fullWidth;
	}
	if(fullHeight < m_boxheight+4+5)
	{
		m_vsize =  m_boxheight+4+5;
	}
	else
	{
		m_vsize = fullHeight;
	}
	
	if(m_hsize>m_boxwidth+4)
	{
		m_xbox = m_hsize - m_boxwidth - 4;
		m_xbox = m_xbox / 2;
	}
	else
	{
		m_xbox = 0;
	}
		
	if(m_vsize>m_boxheight+4)
	{
		m_ybox = m_vsize - m_boxheight - 4;
		m_ybox = m_ybox / 2;
	}
	else
	{
		m_ybox = 0;
	}
	
	//m_view->MoveTo(m_xbox+2,m_ybox+2);
	//m_view->ResizeTo(320-1,300-1);
	m_rectfull.Set(0,0,m_hsize,m_vsize);
	ContentInstance::FrameChanged(newFrame, fullWidth, fullHeight);
	
	
	if(m_oview)
	{
		m_oview->MoveTo(m_xbox+2,m_ybox+2);
	}
				
	if(m_window)
	{
		if(m_window->Lock())
		{
			m_view->Invalidate();
			m_window->Unlock();
		}
	}
	//MarkDirty(&m_rectfull);
	return B_OK;
}

status_t 
VideoContentInstance::Draw(BView *into, BRect exposed)
{
	//printf("Draw %f %f %f %f \n",exposed.left,exposed.right,exposed.top,exposed.bottom);
	//printf("Draw %f %f  \n",exposed.right-exposed.left,exposed.top-exposed.bottom);
	
	//into->SetViewColor(128,132,115,255);
	int ymin = exposed.top;
	int ymax = exposed.bottom;
	int hmin = exposed.left;
	int hmax = exposed.right;
		
	into->SetPenSize(1.0);
		
	for(int i=ymin;i<ymax+1;i++)
	{
		#if SONY_MPEG_BACKGROUND
		if((i%3) == 0)
		{
			into->SetHighColor(151,155,142,255);
		}
		else
		{
			into->SetHighColor(128,132,115,255);
		}
		#else
		into->SetHighColor(0,0,0,255);
		#endif
		
		if((i>m_ybox+1) && (i<(m_ybox+m_boxheight+2)))
		{
			if((hmin<(m_xbox+m_boxwidth+2)) && (hmax>m_xbox+1))
			{
				if((hmax>=(m_xbox+m_boxwidth+3)))
				{
					BPoint p1(m_xbox+m_boxwidth+3,i);
					BPoint p2(hmax,i);
					into->StrokeLine(p1,p2);
				}
				if((hmin<=m_xbox))
				{
					BPoint p1(hmin,i);
					BPoint p2(m_xbox,i);
					into->StrokeLine(p1,p2);
				}
			}
			else
			{
				BPoint p1(hmin,i);
				BPoint p2(hmax,i);
				into->StrokeLine(p1,p2);
			}
		}
		else
		{
			BPoint p1(hmin,i);
			BPoint p2(hmax,i);
			into->StrokeLine(p1,p2);
		}	
		
	}
		
		
	int32 x,y;
	x = m_xbox;
	y = m_ybox;
	
		
	//black lines
	//first line horizontal
	into->SetHighColor(0,0,0,255);
	BPoint p1(x+1,y);
	BPoint p2(x+m_boxwidth+2,y);
	into->StrokeLine(p1,p2);
		
	//second line horizontal
	p1.Set(x,y+1);
	p2.Set(x+m_boxwidth+3,y+1);
	into->StrokeLine(p1,p2);
		
	//last - 1 line horizontal
	p1.Set(x,y+m_boxheight+2);
	p2.Set(x+m_boxwidth+3,y+m_boxheight+2);
	into->StrokeLine(p1,p2);
		
	//last line horizontal
	p1.Set(x+1,y+m_boxheight+3);
	p2.Set(x+m_boxwidth+2,y+m_boxheight+3);
	into->StrokeLine(p1,p2);
		
	//first line vertical
	p1.Set(x,y+2);
	p2.Set(x,y+m_boxheight+1);
	into->StrokeLine(p1,p2);
		
	//second line vertical
	p1.Set(x+1,y+2);
	p2.Set(x+1,y+m_boxheight+1);
	into->StrokeLine(p1,p2);
		
	//last - 1 line vertical	
	p1.Set(x+m_boxwidth+2,y+2);
	p2.Set(x+m_boxwidth+2,y+m_boxheight+1);
	into->StrokeLine(p1,p2);
	
	//last line vertical	
	p1.Set(x+m_boxwidth+3,y+2);
	p2.Set(x+m_boxwidth+3,y+m_boxheight+1);
	into->StrokeLine(p1,p2);
		
	
	drawing_mode dmode = into->DrawingMode();
	into->SetDrawingMode(B_OP_ALPHA);
	
	#if SONY_MPEG_BACKGROUND
	//shadow vertical
	into->SetHighColor(0,0,0,120);
	p1.Set(x+m_boxwidth+4,y+6);
	p2.Set(x+m_boxwidth+4,y+m_boxheight+2);
	into->StrokeLine(p1,p2);
	
	into->SetHighColor(0,0,0,104);
	p1.Set(x+m_boxwidth+5,y+6);
	p2.Set(x+m_boxwidth+5,y+m_boxheight+2);
	into->StrokeLine(p1,p2);
	
	into->SetHighColor(0,0,0,80);
	p1.Set(x+m_boxwidth+6,y+6);
	p2.Set(x+m_boxwidth+6,y+m_boxheight+2);
	into->StrokeLine(p1,p2);
	
	into->SetHighColor(0,0,0,48);
	p1.Set(x+m_boxwidth+7,y+6);
	p2.Set(x+m_boxwidth+7,y+m_boxheight+2);
	into->StrokeLine(p1,p2);
	
	into->SetHighColor(0,0,0,24);
	p1.Set(x+m_boxwidth+8,y+6);
	p2.Set(x+m_boxwidth+8,y+m_boxheight+2);
	into->StrokeLine(p1,p2);
	
	//shadow horizontal
		
	into->SetHighColor(0,0,0,120);
	p1.Set(x+6,y+m_boxheight+4);
	p2.Set(x+m_boxwidth+2,y+m_boxheight+4);
	into->StrokeLine(p1,p2);
	
	into->SetHighColor(0,0,0,104);
	p1.Set(x+6,y+m_boxheight+5);
	p2.Set(x+m_boxwidth+2,y+m_boxheight+5);
	into->StrokeLine(p1,p2);
	
	into->SetHighColor(0,0,0,80);
	p1.Set(x+6,y+m_boxheight+6);
	p2.Set(x+m_boxwidth+2,y+m_boxheight+6);
	into->StrokeLine(p1,p2);
	
	into->SetHighColor(0,0,0,48);
	p1.Set(x+6,y+m_boxheight+7);
	p2.Set(x+m_boxwidth+2,y+m_boxheight+7);
	into->StrokeLine(p1,p2);
	
	into->SetHighColor(0,0,0,24);
	p1.Set(x+6,y+m_boxheight+8);
	p2.Set(x+m_boxwidth+2,y+m_boxheight+8);
	into->StrokeLine(p1,p2);	
	
	//shadow bottom left
	into->SetHighColor(0,0,0,7);
	p1.Set(x,y+m_boxheight+3);
	into->StrokeLine(p1,p1);
	
	into->SetHighColor(0,0,0,6);
	p1.Set(x,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,20);
	p1.Set(x+1,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,42);
	p1.Set(x+2,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,71);
	p1.Set(x+3,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,94);
	p1.Set(x+4,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,110);
	p1.Set(x+5,y+m_boxheight+4);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,5);
	p1.Set(x,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,16);
	p1.Set(x+1,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,34);
	p1.Set(x+2,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,59);
	p1.Set(x+3,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,80);
	p1.Set(x+4,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,94);
	p1.Set(x+5,y+m_boxheight+5);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,3);
	p1.Set(x,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,11);
	p1.Set(x+1,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,24);
	p1.Set(x+2,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,42);
	p1.Set(x+3,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,59);
	p1.Set(x+4,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,71);
	p1.Set(x+5,y+m_boxheight+6);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,1);
	p1.Set(x,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,6);
	p1.Set(x+1,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,13);
	p1.Set(x+2,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,24);
	p1.Set(x+3,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,34);
	p1.Set(x+4,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,42);
	p1.Set(x+5,y+m_boxheight+7);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,0);
	p1.Set(x,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,2);
	p1.Set(x+1,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,6);
	p1.Set(x+2,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,11);
	p1.Set(x+3,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,16);
	p1.Set(x+4,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,20);
	p1.Set(x+5,y+m_boxheight+8);
	into->StrokeLine(p1,p1);	
	
		
		
	//shadow top right
	into->SetHighColor(0,0,0,7);
	p1.Set(x+m_boxwidth+3,y);
	into->StrokeLine(p1,p1);
	
	into->SetHighColor(0,0,0,6);
	p1.Set(x+m_boxwidth+4,y);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,20);
	p1.Set(x+m_boxwidth+4,y+1);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,42);
	p1.Set(x+m_boxwidth+4,y+2);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,71);
	p1.Set(x+m_boxwidth+4,y+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,94);
	p1.Set(x+m_boxwidth+4,y+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,110);
	p1.Set(x+m_boxwidth+4,y+5);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,5);
	p1.Set(x+m_boxwidth+5,y);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,16);
	p1.Set(x+m_boxwidth+5,y+1);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,34);
	p1.Set(x+m_boxwidth+5,y+2);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,59);
	p1.Set(x+m_boxwidth+5,y+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,80);
	p1.Set(x+m_boxwidth+5,y+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,94);
	p1.Set(x+m_boxwidth+5,y+5);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,3);
	p1.Set(x+m_boxwidth+6,y);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,11);
	p1.Set(x+m_boxwidth+6,y+1);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,24);
	p1.Set(x+m_boxwidth+6,y+2);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,42);
	p1.Set(x+m_boxwidth+6,y+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,59);
	p1.Set(x+m_boxwidth+6,y+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,71);
	p1.Set(x+m_boxwidth+6,y+5);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,1);
	p1.Set(x+m_boxwidth+7,y);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,6);
	p1.Set(x+m_boxwidth+7,y+1);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,13);
	p1.Set(x+m_boxwidth+7,y+2);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,24);
	p1.Set(x+m_boxwidth+7,y+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,34);
	p1.Set(x+m_boxwidth+7,y+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,42);
	p1.Set(x+m_boxwidth+7,y+5);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,0);
	p1.Set(x+m_boxwidth+8,y);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,2);
	p1.Set(x+m_boxwidth+8,y+1);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,6);
	p1.Set(x+m_boxwidth+8,y+2);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,11);
	p1.Set(x+m_boxwidth+8,y+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,16);
	p1.Set(x+m_boxwidth+8,y+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,20);
	p1.Set(x+m_boxwidth+8,y+5);
	into->StrokeLine(p1,p1);	
	
	//shadow bottom right
	into->SetHighColor(0,0,0,127);
	p1.Set(x+m_boxwidth+3,y+m_boxheight+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,119);
	p1.Set(x+m_boxwidth+4,y+m_boxheight+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,102);
	p1.Set(x+m_boxwidth+5,y+m_boxheight+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,78);
	p1.Set(x+m_boxwidth+6,y+m_boxheight+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,46);
	p1.Set(x+m_boxwidth+7,y+m_boxheight+3);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,23);
	p1.Set(x+m_boxwidth+8,y+m_boxheight+3);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,119);
	p1.Set(x+m_boxwidth+3,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,110);
	p1.Set(x+m_boxwidth+4,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,94);
	p1.Set(x+m_boxwidth+5,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,71);
	p1.Set(x+m_boxwidth+6,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,42);
	p1.Set(x+m_boxwidth+7,y+m_boxheight+4);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,20);
	p1.Set(x+m_boxwidth+8,y+m_boxheight+4);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,102);
	p1.Set(x+m_boxwidth+3,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,94);
	p1.Set(x+m_boxwidth+4,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,80);
	p1.Set(x+m_boxwidth+5,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,59);
	p1.Set(x+m_boxwidth+6,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,34);
	p1.Set(x+m_boxwidth+7,y+m_boxheight+5);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,16);
	p1.Set(x+m_boxwidth+8,y+m_boxheight+5);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,78);
	p1.Set(x+m_boxwidth+3,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,71);
	p1.Set(x+m_boxwidth+4,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,59);
	p1.Set(x+m_boxwidth+5,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,42);
	p1.Set(x+m_boxwidth+6,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,24);
	p1.Set(x+m_boxwidth+7,y+m_boxheight+6);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,11);
	p1.Set(x+m_boxwidth+8,y+m_boxheight+6);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,46);
	p1.Set(x+m_boxwidth+3,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,42);
	p1.Set(x+m_boxwidth+4,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,34);
	p1.Set(x+m_boxwidth+5,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,24);
	p1.Set(x+m_boxwidth+6,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,13);
	p1.Set(x+m_boxwidth+7,y+m_boxheight+7);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,6);
	p1.Set(x+m_boxwidth+8,y+m_boxheight+7);
	into->StrokeLine(p1,p1);	
	
	into->SetHighColor(0,0,0,23);
	p1.Set(x+m_boxwidth+3,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,20);
	p1.Set(x+m_boxwidth+4,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,16);
	p1.Set(x+m_boxwidth+5,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,11);
	p1.Set(x+m_boxwidth+6,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,6);
	p1.Set(x+m_boxwidth+7,y+m_boxheight+8);
	into->StrokeLine(p1,p1);
	into->SetHighColor(0,0,0,2);
	p1.Set(x+m_boxwidth+8,y+m_boxheight+8);
	into->StrokeLine(p1,p1);	
	#endif
	
	//line above control
	into->SetHighColor(0,0,0,255);
	p1.Set(x+2,y+m_boxheight+1-36);
	p2.Set(x+m_boxwidth+1,y+m_boxheight+1-36);
	into->StrokeLine(p1,p2);
	
	into->SetHighColor(0,0,0,255);
	p1.Set(x+2,y+m_boxheight-36);
	p2.Set(x+m_boxwidth+1,y+m_boxheight-36);
	into->StrokeLine(p1,p2);	
	
	BRect rr(x+2,y+m_boxheight+2-36,x+m_boxwidth+1,y+m_boxheight+1);
	#if SONY_MPEG_BACKGROUND
	into->SetHighColor(182,187,166,255);
	#else
	into->SetHighColor(bar_grey,bar_grey,bar_grey,255);
	#endif
	into->FillRect(rr);

	//printf("FullRect %f %f \n",m_rectfull.left,m_rectfull.top);
	//printf("FullRect %f %f \n",m_rectfull.right-m_rectfull.left,m_rectfull.bottom-m_rectfull.top);
	
	if(fWidgets[kInfoLeft] )
	{
		BRect r(x+2+95,y+m_boxheight+2-36+4,x+2+13+95,y+m_boxheight+2-36+4+27);
		fWidgets[kInfoLeft]->FrameChanged(r,m_hsize,m_vsize);
		fWidgets[kInfoLeft]->Draw(into,r);
	}
		
	if(fWidgets[kInfoRight] )
	{
		BRect r(x+m_boxwidth+2-30,y+m_boxheight+2-36+4,x+m_boxwidth+2-30+13,y+m_boxheight+2-36+4+27);
		fWidgets[kInfoRight]->FrameChanged(r,m_hsize,m_vsize);
		fWidgets[kInfoRight]->Draw(into,r);
	}
		
	{
		BRect r(x+2+13+96,y+m_boxheight+2-36+4,x+m_boxwidth+2-30-1,y+m_boxheight+2-36+4+27);
		into->SetHighColor(0,0,0,255);
		into->FillRect(r);
	}
	
	
	
	if((m_downloading&1) == 1)
	{
		BRect overlay(x+2,y+2,x+m_boxwidth+1,y+m_boxheight+1-36-2);
		if(overlay.Intersects(exposed) == true)
		{
			#if SONY_MPEG_BACKGROUND
			into->SetHighColor(0,0,0,255);
			#else
			into->SetHighColor(video_grey,video_grey,video_grey,255);	
			#endif
			into->FillRect(overlay);
		}
	
	
		if(fWidgets[kStopDisabled])
		{
			BRect r(x+2+13,y+m_boxheight+2-36+4,x+2+13+31,y+m_boxheight+2-36+4+31);
			fWidgets[kStopDisabled]->FrameChanged(r,32,32);
			fWidgets[kStopDisabled]->Draw(into,r);
		}
		
		
		if(fWidgets[kPauseDisabled] )
		{
			BRect r(x+2+55,y+m_boxheight+2-36+4,x+2+31+55,y+m_boxheight+2-36+4+31);
			fWidgets[kPlayDisabled]->FrameChanged(r,m_hsize,m_vsize);
			fWidgets[kPlayDisabled]->Draw(into,r);
		}
		
		
		
		
		
		{
			BPoint pt(x+2+13+96,y+m_boxheight+1-36+4+20);
			into->SetDrawingMode(B_OP_COPY);
			into->SetHighColor(0,102,255,255);
			into->SetLowColor(0,0,0,255);
			into->DrawString("Loading",pt);
		}
		if((m_downloading&2)==2)
		{
			//printf("Draw pourcentage\n");
			BPoint pt(x+2+13+250,y+m_boxheight+2-36+4+20);
			into->SetDrawingMode(B_OP_COPY);
			into->SetHighColor(0,102,255,255);
			into->SetLowColor(0,0,0,255);
			char c;
			if(m_loaded<10)
			{
				into->DrawChar(' ',pt);
				c = 48 + m_loaded;
				into->DrawChar(c);
				into->DrawChar('%');
			}
			else
			{
				if(m_loaded<100)
				{
					float f = (float)(m_loaded)/10.0;
					c = 48 + (char)(f);
					into->DrawChar(c,pt);
					f = (char)(f);
					f = (float)(m_loaded) - f * 10.0; 
					c = 48 + (char)(f);
					into->DrawChar(c);
					into->DrawChar('%');
				}	
			}
			//printf("loading %ld\n",m_loaded);
		}
		
		
	}
	
	
	if((m_downloading&4) == 4)
	{
		BRect overlay(x+2,y+2,x+m_boxwidth+1,y+m_boxheight+1-36-2);
		if(overlay.Intersects(exposed) == true)
		{
			into->SetHighColor(m_key);
			into->FillRect(overlay);
		}
		
		
		if((m_downloading&256) == 256)
		{
			BPoint pt(x+2+13+96,y+m_boxheight+1-36+4+20);
			into->SetDrawingMode(B_OP_COPY);
			into->SetHighColor(0,102,255,255);
			into->SetLowColor(0,0,0,255);
			into->DrawString("Stopped",pt);
		}
		else
		{
			if((m_downloading&32) == 32)
			{
				BPoint pt(x+2+13+96,y+m_boxheight+1-36+4+20);
				into->SetDrawingMode(B_OP_COPY);
				into->SetHighColor(0,102,255,255);
				into->SetLowColor(0,0,0,255);
				into->DrawString("Playing",pt);
			}
			else
			{
				BPoint pt(x+2+13+96,y+m_boxheight+1-36+4+20);
				into->SetDrawingMode(B_OP_COPY);
				into->SetHighColor(0,102,255,255);
				into->SetLowColor(0,0,0,255);
				into->DrawString("Paused",pt);
			}
		}
		
		
		{
			float t = m_time;
			//t = t / 1000000.0;
			float mn = t / 60.0;
			float h = t / 3600.0;
			int ih = h;
			int imn = mn;
			imn = imn % 60;
			int it = t;
			it = it % 60;
			
			if(ih != 0)
			{
				BPoint pt(x+m_boxwidth-78,y+m_boxheight+2-36+4+20-1);
				into->SetDrawingMode(B_OP_COPY);
				into->SetHighColor(0,102,255,255);
				into->SetLowColor(0,0,0,255);
				char c = (ih / 10) + 48;
				into->DrawChar(c,pt);
				c = (ih % 10) + 48;
				into->DrawChar(c);
				into->DrawChar(':');	
			}
			
			{
				BPoint pt(x+m_boxwidth-60,y+m_boxheight+2-36+4+20-1);
				into->SetDrawingMode(B_OP_COPY);
				into->SetHighColor(0,102,255,255);
				into->SetLowColor(0,0,0,255);
				char c = (imn / 10) + 48;
				into->DrawChar(c,pt);
				c = (imn % 10) + 48;
				into->DrawChar(c);
				into->DrawChar(':');
				c = (it / 10) + 48;
				into->DrawChar(c);
				c = (it % 10) + 48;
				into->DrawChar(c);
			}
			
			
			
		}
		
		
		
		if((m_downloading&24) == 0)
		{
			if(fWidgets[kStopUp])
			{
				BRect r(x+2+13,y+m_boxheight+2-36+4,x+2+13+31,y+m_boxheight+2-36+4+31);
				fWidgets[kStopUp]->FrameChanged(r,32,32);
				fWidgets[kStopUp]->Draw(into,r);
			}
		}	
		
		if((m_downloading&24) == 8)
		{
			if(fWidgets[kStopOver])
			{
				BRect r(x+2+13,y+m_boxheight+2-36+4,x+2+13+31,y+m_boxheight+2-36+4+31);
				fWidgets[kStopOver]->FrameChanged(r,32,32);
				fWidgets[kStopOver]->Draw(into,r);
			}
		}	
		
		if((m_downloading&24) == 16)
		{
			if(fWidgets[kStopDown])
			{
				BRect r(x+2+13,y+m_boxheight+2-36+4,x+2+13+31,y+m_boxheight+2-36+4+31);
				fWidgets[kStopDown]->FrameChanged(r,32,32);
				fWidgets[kStopDown]->Draw(into,r);
			}
		}	
		
		
		//playing buttom
		if((m_downloading&32) == 0)
		{
			if((m_downloading&192) == 0)
			{
				if(fWidgets[kPlayUp])
				{
					BRect r(x+2+55,y+m_boxheight+2-36+4,x+2+31+55,y+m_boxheight+2-36+4+31);
					fWidgets[kPlayUp]->FrameChanged(r,32,32);
					fWidgets[kPlayUp]->Draw(into,r);
				}
			}	
		
			if((m_downloading&192) == 64)
			{
				if(fWidgets[kPlayOver])
				{
					BRect r(x+2+55,y+m_boxheight+2-36+4,x+2+31+55,y+m_boxheight+2-36+4+31);
					fWidgets[kPlayOver]->FrameChanged(r,32,32);
					fWidgets[kPlayOver]->Draw(into,r);
				}
			}	
		
			if((m_downloading&192) == 128)
			{
				if(fWidgets[kPlayDown])
				{
					BRect r(x+2+55,y+m_boxheight+2-36+4,x+2+31+55,y+m_boxheight+2-36+4+31);
					fWidgets[kPlayDown]->FrameChanged(r,32,32);
					fWidgets[kPlayDown]->Draw(into,r);
				}
			}
		}
		else
		{
			if((m_downloading&192) == 0)
			{
				if(fWidgets[kPauseUp])
				{
					BRect r(x+2+55,y+m_boxheight+2-36+4,x+2+31+55,y+m_boxheight+2-36+4+31);
					fWidgets[kPauseUp]->FrameChanged(r,32,32);
					fWidgets[kPauseUp]->Draw(into,r);
				}
			}	
		
			if((m_downloading&192) == 64)
			{
				if(fWidgets[kPauseOver])
				{
					BRect r(x+2+55,y+m_boxheight+2-36+4,x+2+31+55,y+m_boxheight+2-36+4+31);
					fWidgets[kPauseOver]->FrameChanged(r,32,32);
					fWidgets[kPauseOver]->Draw(into,r);
				}
			}	
		
			if((m_downloading&192) == 128)
			{
				if(fWidgets[kPauseDown])
				{
					BRect r(x+2+55,y+m_boxheight+2-36+4,x+2+31+55,y+m_boxheight+2-36+4+31);
					fWidgets[kPauseDown]->FrameChanged(r,32,32);
					fWidgets[kPauseDown]->Draw(into,r);
				}
			}
		
		}
		
		
	}	
	into->SetDrawingMode(dmode);
	return B_OK;
}

status_t 
VideoContentInstance::GetSize(int32 *width, int32 *height, uint32 *flags)
{
	*width = m_hsize;
	*height = m_vsize;
	*flags = 0;
	//printf( "VideoContentInstance::GetSize() returns %ld,%ld \n", FullWidth(), FullHeight());
	return B_OK;
}

void 
VideoContentInstance::MouseDown(BPoint where, const BMessage *event)
{
	
	if((m_downloading&4) == 4)
	{
		BRect r1(m_xbox+2+13,m_ybox+m_boxheight+2-36+4,m_xbox+2+13+31,m_ybox+m_boxheight+2-36+4+31);
		BRect r2(m_xbox+2+55,m_ybox+m_boxheight+2-36+4,m_xbox+2+31+55,m_ybox+m_boxheight+2-36+4+31);
		if(r1.Contains(where) == true)
		{
			if(((m_downloading&24) == 0))
			{
				m_downloading += 16;
				if(fWidgets[kStopDown])
				{
					if(m_window)
					{
						if(m_window->LockWithTimeout(500000)==B_OK)
						{
							#if SONY_MPEG_BACKGROUND
							m_view->SetHighColor(182,187,166,255);
							#else
							m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
							#endif
							m_view->FillRect(r1);
							fWidgets[kStopDown]->FrameChanged(r1,32,32);
							fWidgets[kStopDown]->Draw(m_view,r1);
							m_view->Sync();
							m_window->Unlock();
						}
					}
				}
			}
			if(((m_downloading&24) == 8))
			{
				m_downloading += 8;
				if(fWidgets[kStopDown])
				{
					if(m_window)
					{
						if(m_window->LockWithTimeout(500000)==B_OK)
						{
							#if SONY_MPEG_BACKGROUND
							m_view->SetHighColor(182,187,166,255);
							#else
							m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
							#endif
							m_view->FillRect(r1);
							fWidgets[kStopDown]->FrameChanged(r1,32,32);
							fWidgets[kStopDown]->Draw(m_view,r1);
							m_view->Sync();
							m_window->Unlock();
						}
					}
				}
			}
			
			m_play = false;
			Reset();
			
			
			
		}
		
		
		if(r2.Contains(where) == true)
		{
			if((m_downloading&192) == 0)
			{
				m_play = ! m_play;
				if(m_audioDecoder)
				{
					m_audioDecoder->SetPlay(m_play);
				}
				m_downloading += 128;
				if((m_downloading&32) == 0)
				{
					m_downloading += 32;
				}
				else
				{
					m_downloading -= 32; 
				}
				if((m_downloading&32) == 0)
				{	
					if(fWidgets[kPlayDown])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPlayDown]->FrameChanged(r2,32,32);
								fWidgets[kPlayDown]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				}
				else
				{
					if(fWidgets[kPauseDown])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPauseDown]->FrameChanged(r2,32,32);
								fWidgets[kPauseDown]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				
				}
			}
			if((m_downloading&192) == 64)
			{
				m_play = ! m_play;
				if(m_audioDecoder)
				{
					m_audioDecoder->SetPlay(m_play);
				}
				m_downloading += 64;
				if((m_downloading&32) == 0)
				{
					m_downloading += 32;
				}
				else
				{
					m_downloading -= 32; 
				}
				if((m_downloading&32) == 0)
				{	
					if(fWidgets[kPlayDown])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPlayDown]->FrameChanged(r2,32,32);
								fWidgets[kPlayDown]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				}
				else
				{
					if(fWidgets[kPauseDown])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPauseDown]->FrameChanged(r2,32,32);
								fWidgets[kPauseDown]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				
				}
			}
			
			BRect rplay(m_xbox+2+13+96,m_ybox+m_boxheight+2-36+4,m_xbox+2+13+96+50,m_ybox+m_boxheight+2-36+4+27);
	
			if(m_window)
			{
				if(m_window->Lock())
				{	
					m_view->SetHighColor(0,0,0,255);
					m_view->FillRect(rplay);
			
			
					BPoint pt(m_xbox+2+13+96,m_ybox+m_boxheight+1-36+4+20);
					m_view->SetDrawingMode(B_OP_COPY);
					m_view->SetHighColor(0,102,255,255);
					m_view->SetLowColor(0,0,0,255);
					
			
					if(m_play == true)
					{
						m_view->DrawString("Playing",pt);
						if((m_downloading&256)==256)
						{
							m_downloading-=256;
						}
					}
					else
					{
						m_view->DrawString("Paused",pt);
					}
			
					m_view->Sync();
					m_window->Unlock();
				}
			}
			
			
		
		}
	
	
	}


}

void						
VideoContentInstance::MouseMoved(BPoint where, uint32 code, const BMessage *a_message, const BMessage *event=NULL)
{
	if((m_downloading&4) == 4)
	{
		BRect r1(m_xbox+2+13,m_ybox+m_boxheight+2-36+4,m_xbox+2+13+31,m_ybox+m_boxheight+2-36+4+31);
		if(r1.Contains(where) == true)
		{
			if(((m_downloading&24) == 0))
			{
				m_downloading += 8;
				if(fWidgets[kStopOver])
				{
					if(m_window)
					{
						if(m_window->LockWithTimeout(500000)==B_OK)
						{
							#if SONY_MPEG_BACKGROUND
							m_view->SetHighColor(182,187,166,255);
							#else
							m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
							#endif
							m_view->FillRect(r1);
							fWidgets[kStopOver]->FrameChanged(r1,32,32);
							fWidgets[kStopOver]->Draw(m_view,r1);
							m_view->Sync();
							m_window->Unlock();
						}
					}
				}
			}
		}
		else
		{
			if((m_downloading&24) != 0)
			{
				m_downloading -= (m_downloading&24);
				if(fWidgets[kStopUp])
				{
					if(m_window)
					{
						if(m_window->LockWithTimeout(500000)==B_OK)
						{
							#if SONY_MPEG_BACKGROUND
							m_view->SetHighColor(182,187,166,255);
							#else
							m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
							#endif
							m_view->FillRect(r1);
							fWidgets[kStopUp]->FrameChanged(r1,32,32);
							fWidgets[kStopUp]->Draw(m_view,r1);
							m_view->Sync();
							m_window->Unlock();
						}
					}
				}
			}
		}
		
		
		
		BRect r2(m_xbox+2+55,m_ybox+m_boxheight+2-36+4,m_xbox+2+31+55,m_ybox+m_boxheight+2-36+4+31);
		if(r2.Contains(where) == true)
		{
			if((m_downloading&192) == 0)
			{
				m_downloading += 64;
				if((m_downloading&32) == 0)
				{	
					if(fWidgets[kPlayOver])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPlayOver]->FrameChanged(r2,32,32);
								fWidgets[kPlayOver]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				}
				else
				{
					if(fWidgets[kPauseOver])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPauseOver]->FrameChanged(r2,32,32);
								fWidgets[kPauseOver]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				
				}
			}	
		}
		else
		{
			if((m_downloading&192) != 0)
			{
				m_downloading -= (m_downloading&192);
			
				if((m_downloading&32) == 0)
				{	
					if(fWidgets[kPlayUp])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPlayUp]->FrameChanged(r2,32,32);
								fWidgets[kPlayUp]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				}
				else
				{
					if(fWidgets[kPauseUp])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPauseUp]->FrameChanged(r2,32,32);
								fWidgets[kPauseUp]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				
				}
			}	
		}
		
	}
	
	
}


void VideoContentInstance::MouseUp(BPoint where, const BMessage *event=NULL)
{
	
	if((m_downloading&4) == 4)
	{
		BRect r1(m_xbox+2+13,m_ybox+m_boxheight+2-36+4,m_xbox+2+13+31,m_ybox+m_boxheight+2-36+4+31);
		if(r1.Contains(where) == true)
		{
			if(((m_downloading&16) == 16))
			{
				m_downloading -= 16;
				if(fWidgets[kStopUp])
				{
					if(m_window)
					{
						if(m_window->LockWithTimeout(500000)==B_OK)
						{
							#if SONY_MPEG_BACKGROUND
							m_view->SetHighColor(182,187,166,255);
							#else
							m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
							#endif
							m_view->FillRect(r1);
							fWidgets[kStopUp]->FrameChanged(r1,32,32);
							fWidgets[kStopUp]->Draw(m_view,r1);
							m_view->Sync();
							m_window->Unlock();
						}
					}
				}
			}
		}
		
		
		BRect r2(m_xbox+2+55,m_ybox+m_boxheight+2-36+4,m_xbox+2+31+55,m_ybox+m_boxheight+2-36+4+31);
		if(r2.Contains(where) == true)
		{
			if((m_downloading&192) == 128)
			{
				m_downloading -= 128;
				if((m_downloading&32) == 0)
				{	
					if(fWidgets[kPlayUp])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPlayUp]->FrameChanged(r2,32,32);
								fWidgets[kPlayUp]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				}
				else
				{
					if(fWidgets[kPauseUp])
					{
						if(m_window)
						{
							if(m_window->LockWithTimeout(500000)==B_OK)
							{
								#if SONY_MPEG_BACKGROUND
								m_view->SetHighColor(182,187,166,255);
								#else
								m_view->SetHighColor(bar_grey,bar_grey,bar_grey,255);
								#endif
								m_view->FillRect(r2);
								fWidgets[kPauseUp]->FrameChanged(r2,32,32);
								fWidgets[kPauseUp]->Draw(m_view,r2);
								m_view->Sync();
								m_window->Unlock();
							}
						}
					}
				
				}
			}	
		}
	
	}
}

void VideoContentInstance::LoadBitmap(widget_id id, const char *name)
{
	//printf("VideoContentInstance::LoadBitmap\n");
	

	const URL resourceUrl(kResourceBase,name);
	

	resourceCache.NewContentInstance(resourceUrl, id, this, 0, BMessage(),
		securityManager.GetGroupID(resourceUrl), 0, "");
	
}

status_t VideoContentInstance::HandleMessage(BMessage *message)
{
	BAutolock lock(&videocontentlocker);
	//printf("VideoContentInstance::HandleMessage %08x\n",this);
	status_t retVal = B_OK;
	switch (message->what) {
		case NEW_CONTENT_INSTANCE: {
			
			int32 id = -1;
			atom<ContentInstance> instance;
			message->FindInt32("id", &id);
			message->FindAtom("instance",instance);
			
			//printf("VideoContentInstance::HandleMessage load bitmap %ld\n",id);
			if (id >= 0 && id < kWidgetCount) {
				fWidgets[id] = instance;
				instance->Acquire();
				
				
				if(m_window)
				{
					if(m_window->LockWithTimeout(500000)==B_OK)
					{
						m_view->Invalidate();
						m_window->Unlock();
					}
				}
				
				//Layout();

			}
			
			break;
		}

		case 'ping': {
			//printf("PING!\n");
			break;
		}

		default:
			//printf("VideoContentInstance::HandleMessage %08x default \n",this);
			Notification(message);
			break;
	}

	//printf("VideoContentInstance::HandleMessage %08x done\n",this);
	return retVal;
}

void VideoContentInstance::Notification(BMessage *message)
{
	ContentInstance::Notification(message);
}


void VideoContentInstance::Cleanup()
{
	BAutolock lock(&videocontentlocker);
	//printf("VideoContentInstance::Cleanup 1 %08x\n",this);
	ContentInstance::Cleanup();
	//printf("VideoContentInstance::Cleanup 2 %08x\n",this);
}
//#pragma mark ----------------------- VideoContent -----------------------

VideoContent::VideoContent(void *handle) :
	Content(handle)
{
	//printf( "VideoContent::VideoContent()\n");
}	


VideoContent::~VideoContent()
{
	//printf("VideoContent::~VideoContent()\n");
}

size_t 
VideoContent::GetMemoryUsage()
{
	//printf("VideoContent::GetMemoryUsage() \n");
	return 0x30000;	// This is the size of the memory cache.
}

bool 
VideoContent::IsInitialized()
{
	//printf("VideoContent::IsInitialized()\n");	
	return true;
}

ssize_t 
VideoContent::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
	//printf("VideoContent::Feed(): done\n");	
	return B_FINISH_STREAM;
}



status_t 
VideoContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &)
{
	//printf( "VideoContent::CreateInstance() \n");
	m_vci = new VideoContentInstance(this, handler);
	*outInstance = m_vci;
	return B_OK;
}




//#pragma mark ----------------------- VideoContentFactory -----------------------

VideoContentFactory::VideoContentFactory()
	: ContentFactory()
{

	m_content = NULL;
}

void VideoContentFactory::GetIdentifiers(BMessage* into)
{
	 /*
	 ** BE AWARE: Any changes you make to these identifiers should
	 ** also be made in the 'addattr' command in the makefile.
	 */
	into->AddString(S_CONTENT_MIME_TYPES, "video/mpeg");
	into->AddString(S_CONTENT_MIME_TYPES, "video/x-mpeg");
	into->AddString(S_CONTENT_EXTENSIONS, "mpeg");
	into->AddString(S_CONTENT_EXTENSIONS, "mpg");
	into->AddString(S_CONTENT_EXTENSIONS, "mpgs");
	into->AddString(S_CONTENT_EXTENSIONS, "mpv");
}
	
Content* VideoContentFactory::CreateContent(void* handle, const char* /* mime */, const char*  extension)
{
	//printf("VideoContentFactory  %\n");
	//Wagner::resourceCache.EmptyCache();
  	if (   strcasecmp(extension, "m2v") == 0
           || strcasecmp(extension, "wmv") == 0)
    {
    	return NULL;
    }
	
	m_content = new VideoContent(handle);

	return m_content;
}

bool VideoContentFactory::KeepLoaded() const
{
	return true;
}



extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if (n == 0) return new VideoContentFactory;
	return 0;
}
