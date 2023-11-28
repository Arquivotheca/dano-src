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
#include "MediaContent.h"
#include "PlaybackEngine.h"

const float kControlPadding = 5;
const URL kResourceBase("file://$RESOURCES/MediaBar/");
static char const *kProperties[] = { "Title", "Playing", "Genre", "URL", "error", "Elapsed", "TotalTime", "Status" };

//static int32 iter = 0;
//#define debugprint(x) { printf("%6d %6d ",atomic_add(&iter,1),find_thread(NULL)); printf x;}
#define debugprint(x)


BLocker mediacontentlocker;

class MediaContentFactory : public ContentFactory {
public:
	virtual void GetIdentifiers(BMessage* into);
	virtual Content* CreateContent(void* handle, const char*, const char*);
};

// optional EMBED parameters supported:)
//     hidden ("true"|"false")="false" : hide the entire media bar (deprecated)
//     be:ejectable ("true"|"false")="false" : should the closebox be shown?
//     be:xborder ("<float>")="0.0" : margin around the media bar display
//     hidecontrols ("true"|"false")="false" : hide all controls except the
//         progress bar
//     bgcolor ("<htmlcolor>")=<transparent> : bgcolor (the only way to
//         get a transparent-background is to leave this blank
//     titlecolor ("<htmlcolor>")="#FFFFFF" : color of title text
//     font ("<fontspec>")=<be_plain_font> : title font (use CSS syntax as in
//         www/util.h)

MediaContentInstance::MediaContentInstance(Content *content, GHandler *handler,
	const BMessage &msg)
	:	ContentInstance(content, handler),
		BinderNode(),
		fMediaBarHTML(0),
		fParentView(0),
		fPlaybackEngine(0),
		fOldDivider(0),
		fPaused(false),
		fGotTitle(false),
		fGotControlSizes(false),
		fShowControls(true),
		fVisible(true),
		fShowEjectButton(false),
		fAutoStart(true),
		fUpdateRunning(0),
		fBorder(3.0),
		fBackgroundColor(B_TRANSPARENT_32_BIT),
		fPaintBackground(false),
		fTitleColor(),
		fPreferredWidth(10.0),
		fPreferredHeight(2.0),
		fCurrentErrorString("B_OK")
{
debugprint(("MediaContentInstance::ctor 1 %08x\n",this));
//	msg.PrintToStream();
	BAutolock lock(&mediacontentlocker);
	// to retrieve the EMBED parameters, we'll use FindString
	// as we assume that Wagner is downcasing the parameter names
	const char *value;
	if (msg.FindString("hidden", &value) == B_OK)
		fVisible = (strcasecmp(value, "true") != 0);

	/* handle the <object> or <embed> parameters */
	BMessage paramtags;
	msg.FindMessage("paramTags",&paramtags);
	if (msg.FindString("controls",&value) == B_OK ||
		paramtags.FindString("controls",&value) == B_OK)
	{
		fVisible = (strcasecmp(value, "controlpanel") == 0);
		fTitleWidth = 0;	// no title for embedded use
	}
	else
	{
		fTitleWidth = 270;	// for regular docked mediabar
	}
	if(msg.FindString("width",&value) == B_OK)
		fPreferredWidth = atoi(value);
	if(msg.FindString("height",&value) == B_OK)
		fPreferredHeight = atoi(value);

	if (msg.FindString("autostart",&value) == B_OK ||
		msg.FindString("autoplay",&value) == B_OK ||
		paramtags.FindString("autostart",&value) == B_OK ||
		paramtags.FindString("autoplay",&value) == B_OK)
	{
		fAutoStart = (strcasecmp(value, "true") == 0);
	}
	fPaused = !fAutoStart;

	if (msg.FindString("be:ejectable", &value) == B_OK)
		fShowEjectButton = (strcasecmp(value, "true") == 0);

	if (msg.FindString("be:xborder", &value) == B_OK) {
		fBorder = atof(value);
		if (fBorder < 0.0)
			fBorder = 0.0;
	}
	
	if (msg.FindString("hidecontrols", &value) == B_OK)
		fShowControls = (strcasecmp(value, "true") != 0);

	if (msg.FindString("bgcolor", &value) == B_OK) {
		fBackgroundColor = decode_color(value);
		fPaintBackground = true;
	}

	if (msg.FindString("titlecolor", &value) == B_OK)
		fTitleColor = decode_color(value);
	else
		fTitleColor = decode_color("#FFFFFF");

	if (msg.FindString("font", &value) != B_OK
		|| decode_font(value, &fTitleFont, &fTitleFont) != B_OK) {
		fTitleFont = *be_plain_font;
		fTitleFont.SetSize(12.0);
		fTitleFont.SetFace(B_ITALIC_FACE);
	}

	// Clear out all the widgets.  LoadBitmap is asynchronous, meaning
	// these won't be set until the bitmaps are loaded and decoded, which
	// may be after a couple of Draw()s have occured.  Also, some elements may
	// not be shown at all, in which case they'll always be NULL.  The drawing code
	// checks for NULL before drawing.
	for (int widgetIndex = 0; widgetIndex < kWidgetCount; widgetIndex++)
		fWidgets[widgetIndex] = 0;

	if (fVisible) {
		fPlayButtonWidth = 0;
		fCloseButtonWidth = 0;

		LoadBitmap(kEndcapLeft, "endcap_left.png");
		LoadBitmap(kEndcapRight, "endcap_right.png");
		LoadBitmap(kIntersect, "intersect.png");
		LoadBitmap(kProgressBar, "progressbar.png");
		LoadBitmap(kRemainBar, "remainbar.png");
		if (fShowControls) {
			if (fShowEjectButton)
				LoadBitmap(kClosebox, "closebox.png");

			LoadBitmap(kPause, "mediapause_up.png");
			LoadBitmap(kPauseActive, "mediapause_down.png");
			LoadBitmap(kPauseOver, "mediapause_over.png");
			LoadBitmap(kPlay, "mediaplay_up.png");
			LoadBitmap(kPlayActive, "mediaplay_down.png");
			LoadBitmap(kPlayOver, "mediaplay_over.png");
		}
	
		fTitle = "Opening"B_UTF8_ELLIPSIS;
		NotifyListeners(B_PROPERTY_CHANGED, "Title");
		fContentRect.Set(0, 0, fPreferredWidth, fPreferredHeight);
		Layout();
	}

//	const URL resourceUrl(kResourceBase, "media_bar.html");
//	resourceCache.NewContentInstance(resourceUrl, 100, this, 0, BMessage(),
//		securityManager.GetGroupID(resourceUrl), 0, "");

debugprint(("MediaContentInstance::ctor 1 %08x done\n",this));
}

void MediaContentInstance::Cleanup()
{
	BAutolock lock(&mediacontentlocker);
debugprint(("MediaContentInstance::Cleanup 1 %08x\n",this));
	ContentInstance::Cleanup();
debugprint(("MediaContentInstance::Cleanup 2 %08x\n",this));
	BinderNode::Cleanup();
debugprint(("MediaContentInstance::Cleanup done %08x\n",this));
}

MediaContentInstance::~MediaContentInstance()
{
	BAutolock lock(&mediacontentlocker);
debugprint(("MediaContentInstance::dtor 1 %08x\n",this));
	delete fPlaybackEngine;
	if (fVisible) {
		for (int widgetIndex = 0; widgetIndex < kWidgetCount; widgetIndex++)
			if (fWidgets[widgetIndex])
			{
debugprint(("releasing widget\n"));
				fWidgets[widgetIndex]->Release();
			}
	}
debugprint(("MediaContentInstance::dtor 1 %08x done\n",this));
}

status_t MediaContentInstance::AttachedToView(BView *view, uint32 *contentFlags)
{
	BAutolock lock(&mediacontentlocker);
debugprint(("MediaContentInstance::AttachedToView %08x\n",this));
	fUpdateRunning = 0;
	fPlaybackEngine = new PlaybackEngine;
	if (fVisible) {
		fParentView = view;
		*contentFlags = cifHasTransparency;
		fPlaybackEngine->SetUpdateNotify(UpdateNotify, this);
	}

	fContentRect = FrameInParent();
	fPlaybackEngine->SetStopNotify(PostStopMessage, this);
	fPlaybackEngine->SetErrorNotify(PostErrorMessage, this);
	fPlaybackEngine->SetPropertyNotify(UpdateProperty, this);
	if(fAutoStart)
		fPlaybackEngine->Start(GetContent()->GetResource()->GetURL());
	else
	{
		fPlaybackEngine->Start(GetContent()->GetResource()->GetURL(), true); // sync restart
		fPlaybackEngine->TogglePause();
		fPaused = true;
	}
debugprint(("MediaContentInstance::AttachedToView %08x done\n",this));
	return B_OK;
}

status_t MediaContentInstance::DetachedFromView()
{
debugprint(("MediaContentInstance::DetachedFromView %08x\n",this));
	fPlaybackEngine->Stop();
debugprint(("MediaContentInstance::DetachedFromView %08x done\n",this));
	return B_OK;
}

status_t MediaContentInstance::GetSize(int32 *width, int32 *height, uint32 *outResizeFlags)
{
debugprint(("MediaContentInstance::GetSize %08x\n",this));
	// Each of fPreferred* is treated as a *minimum* now.
	if (fContentRect.Width() < fPreferredWidth)
		fContentRect.right = fContentRect.left + fPreferredWidth;

	if (fContentRect.Height() < fPreferredHeight)
		fContentRect.bottom = fContentRect.top + fPreferredHeight;

	*width = static_cast<int32>(fContentRect.Width());
	*height = static_cast<int32>(fContentRect.Height());
	*outResizeFlags = IS_DOCKABLE;
debugprint(("MediaContentInstance::GetSize %08x done\n",this));
	return B_OK;
}

status_t MediaContentInstance::FrameChanged(BRect newRect, int32 fullWidth, int32 fullHeight)
{
debugprint(("MediaContentInstance::FrameChanged %08x\n",this));
	// It is a Bad Thing to argue with the layout engine here
	fContentRect = newRect;
	Layout();
	status_t ret = ContentInstance::FrameChanged(newRect, fullWidth, fullHeight);
debugprint(("MediaContentInstance::FrameChanged %08x done\n",this));
	return ret;
}

status_t MediaContentInstance::Draw(BView *into, BRect exposed)
{
	PRINT(("MediaBar: MediaContentInstance::Draw(<BView>, BRect(%0.1f,%0.1f,%0.1f,%0.1f))\n",
		exposed.left, exposed.top, exposed.right, exposed.bottom));

	if (fShowControls && !fGotControlSizes) {
		int32 width;
		int32 dummy;
		uint32 flags;
		if (fWidgets[fPaused?kPause:kPlay] != NULL) {
			fWidgets[fPaused?kPause:kPlay]->GetSize(&width, &dummy, &flags);
			PRINT(("MediaBar: found play button width: %d\n", width));
			fPlayButtonWidth = width;
		}

		if (fWidgets[kClosebox] != NULL) {
			fWidgets[kClosebox]->GetSize(&width, &dummy, &flags);
			PRINT(("MediaBar: found closebox width: %d\n", width));
			fCloseButtonWidth = width;
		}

		fGotControlSizes = (fCloseButtonWidth != 0 && fPlayButtonWidth != 0);
	}
	
	if (fVisible) {
//printf("visible\n");
		if (fPaintBackground) {
			into->SetLowColor(fBackgroundColor);
			into->SetDrawingMode(B_OP_COPY);
			into->FillRect(FrameInParent() & exposed, B_SOLID_LOW);
		} 

		for (int widgetIndex = 0; widgetIndex < kWidgetCount; widgetIndex++) {
//printf("widget %d - %08x - %d\n",widgetIndex,fWidgets[widgetIndex],fWidgets[widgetIndex]?fWidgetRect[widgetIndex].IsValid():0);
			if (fWidgets[widgetIndex] && fWidgetRect[widgetIndex].IsValid()) {
				// Debugging info.
				PRINT(("MediaBar: "));
				PRINT_OBJECT((fWidgets[widgetIndex]->FrameInParent()));
				PRINT(("MediaBar: fWidgets[%d]->Draw()\n", widgetIndex));
				
				fWidgets[widgetIndex]->Draw(into, exposed); // Whether technically "exposed" or not.
			}
		}
		if(fMediaBarHTML)
			fMediaBarHTML->Draw(into, exposed);
	
		if (fShowControls) {
//printf("show controls\n");
			into->MovePenTo(fTitlePos);
			into->SetLowColor(B_TRANSPARENT_32_BIT);
			into->SetHighColor(fTitleColor);
			into->SetDrawingMode(B_OP_ALPHA);
			into->SetFont(&fTitleFont);
			into->DrawString(fTitle.String());
		}
	}
	
	return B_OK;
}

void MediaContentInstance::MouseDown(BPoint where, const BMessage * /*event*/)
{
	// Point is in my coordinate space, but widget frames are specified relative to
	// view origin.
	where += FrameInParent().LeftTop();
	for (int widgetIndex = 0; widgetIndex < kWidgetCount; widgetIndex++)
		if (fWidgets[widgetIndex] && fWidgetRect[widgetIndex].Contains(where)) {
			switch (widgetIndex) {
				case kClosebox:
					PostStopMessage(this);
					break;
					
				case kPause:
				case kPauseActive:
				case kPauseOver:
				case kPlay:
				case kPlayActive:
				case kPlayOver:
					if (fPlaybackEngine) {
						fPlaybackEngine->TogglePause();
						fPaused = !fPaused;
						NotifyListeners(B_PROPERTY_CHANGED, "Playing");
						Layout();
					}

					break;
					
				default:
					;
			}
			
			break;
		}
}

void MediaContentInstance::Layout()
{
debugprint(("MediaContentInstance::Layout %08x\n",this));
	if (!fVisible)
		return;

	for (int widgetIndex = 0; widgetIndex < kWidgetCount; widgetIndex++)
		fWidgetRect[widgetIndex].Set(-1.0, -1.0, -2.0, -2.0);

	BRect contentArea = fContentRect.InsetByCopy(fBorder, 0.0);
	BRect mainSliderRect(contentArea);

	if (fShowControls) {
		BRect playButtonRect(contentArea);
		playButtonRect.right = playButtonRect.left + fPlayButtonWidth;
		if (fPaused)
			fWidgetRect[kPlay] = playButtonRect;
		else
			fWidgetRect[kPause] = playButtonRect;
			
		BRect closeRect(contentArea);
		closeRect.left = fShowEjectButton ? closeRect.right - fCloseButtonWidth : closeRect.right;
		fWidgetRect[kClosebox] = closeRect;
		
		BRect titleRect(contentArea);
		titleRect.right = closeRect.left - kControlPadding;
		titleRect.left = titleRect.right - fTitleWidth;
	
		font_height fmetrics;
		fTitleFont.GetHeight(&fmetrics);
		float borders = (titleRect.Height() - (fmetrics.ascent + fmetrics.descent
			+ fmetrics.leading)) / 2;
		if (fTitleFont.StringWidth(fTitle.String()) > titleRect.Width() - 7) {
			char out[1024];
			const char *inarray[1];	
			inarray[0] = fTitle.String();
			char *outarray[1];
			outarray[0] = out;
			fTitleFont.GetTruncatedStrings(inarray, 1, B_TRUNCATE_MIDDLE, titleRect.Width() - 7,
				outarray);
			fTitle = out;
		}
			
		fTitlePos.x = titleRect.left + (titleRect.Width() - fTitleFont.StringWidth(fTitle.String())) / 2;
		fTitlePos.y = titleRect.bottom - borders - fmetrics.descent;

		// Adjust the progress bar for the above, including some spacing.
		mainSliderRect.left = playButtonRect.right + kControlPadding;
		mainSliderRect.right = titleRect.left - kControlPadding;
	}
	
	int32 leftCap = 5;	 // behavior of the old system
	int32 rightCap = 5;
	int32 dummy;
	uint32 flags;
	
	if (fWidgets[kEndcapLeft])
		fWidgets[kEndcapLeft]->GetSize(&leftCap, &dummy, &flags);

	if (fWidgets[kEndcapRight])
		fWidgets[kEndcapRight]->GetSize(&rightCap, &dummy, &flags);
	
	fWidgetRect[kEndcapLeft].Set(mainSliderRect.left, mainSliderRect.top,
		mainSliderRect.left + leftCap, mainSliderRect.bottom);
	fWidgetRect[kEndcapRight].Set(mainSliderRect.right - rightCap, mainSliderRect.top,
		mainSliderRect.right, mainSliderRect.bottom);

	mainSliderRect.left += leftCap;
	mainSliderRect.right -= rightCap;
	
	fWidgetRect[kProgressBar] = mainSliderRect;
	fWidgetRect[kRemainBar] = mainSliderRect;
	float divider = mainSliderRect.Width()
		* (fPlaybackEngine ? fPlaybackEngine->GetCompletion() : 0.0);
	fWidgetRect[kProgressBar].right = mainSliderRect.left + divider;
	fWidgetRect[kRemainBar].left = mainSliderRect.left + divider;
	for (int widgetIndex = 0; widgetIndex < kWidgetCount; widgetIndex++) {
		if (fWidgets[widgetIndex])
			fWidgets[widgetIndex]->FrameChanged(fWidgetRect[widgetIndex], fContentRect.Width(),
				fContentRect.Height());
	}
	
	if(fMediaBarHTML)
	{
		//printf("showing mediabar: %f x %f\n",fContentRect.right-fContentRect.left,fContentRect.bottom-fContentRect.top);
		fMediaBarHTML->FrameChanged(fContentRect, fContentRect.Width(),	fContentRect.Height());
	}
	
	if (fParentView)
		MarkDirty();
debugprint(("MediaContentInstance::Layout %08x done\n",this));
}

void MediaContentInstance::PostStopMessage(void *castToInstance)
{
debugprint(("MediaContentInstance::PostStopMessage %08x\n",castToInstance));
	MediaContentInstance *instance = reinterpret_cast<MediaContentInstance*>(castToInstance);
	if (instance->fVisible) {
		BMessage notify('mary');
		BView *top = instance->fParentView;
		while (top->Parent())
			top = top->Parent();
	
		BMessage container(bmsgNotifyInstance);
		container.AddMessage("notification", &notify);
		top->Looper()->PostMessage(&container, top);
	}
debugprint(("MediaContentInstance::PostStopMessage %08x done\n",castToInstance));
}

void MediaContentInstance::PostErrorMessage(void *castToInstance, const char *error)
{
debugprint(("MediaContentInstance::PostErrorMessage %08x\n",castToInstance));
	MediaContentInstance *instance = reinterpret_cast<MediaContentInstance*>(castToInstance);
	instance->fCurrentErrorString = error;
	instance->NotifyListeners(B_PROPERTY_CHANGED, "error");
debugprint(("MediaContentInstance::PostErrorMessage %08x done\n",castToInstance));
}

void MediaContentInstance::UpdateProperty(void *castToInstance, const char *prop)
{
	MediaContentInstance *instance = reinterpret_cast<MediaContentInstance*>(castToInstance);
	
	BMessage msg(B_PROPERTY_CHANGED);
	msg.AddString("prop",prop);
	instance->PostMessage(msg);
//	instance->NotifyListeners(B_PROPERTY_CHANGED, prop);
}

void MediaContentInstance::UpdateProgress()
{
	if (!fGotTitle || fPlaybackEngine->DidNameChange()) {
		fGotTitle = true;
		if (strlen(fPlaybackEngine->GetStreamName()) > 0) {
			// Grab a pretty name for this stream.
			fTitle = fPlaybackEngine->GetStreamName();
		} else {
			// Default to filename or full url.
			const URL &url = fPlaybackEngine->GetUrl();
			char tmp[1024];
			url.GetUnescapedFileName(tmp, 1024);
			if (strlen(tmp) == 0)
				url.GetString(tmp, 1024);
		
			fTitle = tmp;
		}
		NotifyListeners(B_PROPERTY_CHANGED, "Title");
		Layout();
	}

	BRect sliderRect = fWidgetRect[kProgressBar] | fWidgetRect[kRemainBar];
	float divider = sliderRect.left + sliderRect.Width() * fPlaybackEngine->GetCompletion();
	BRect dirty(fOldDivider, sliderRect.top, divider, sliderRect.bottom);
	fWidgetRect[kProgressBar].right = divider;
	fWidgetRect[kRemainBar].left = divider;
	if (fWidgets[kProgressBar])
		fWidgets[kProgressBar]->FrameChanged(fWidgetRect[kProgressBar], fContentRect.Width(), fContentRect.Height());

	if (fWidgets[kRemainBar])
		fWidgets[kRemainBar]->FrameChanged(fWidgetRect[kRemainBar], fContentRect.Width(), fContentRect.Height());

	fOldDivider = divider;
	if (fParentView) {
		// Use only integral values in MarkDirty.
		dirty.left = static_cast<int>(dirty.left) - 1;
		dirty.top = static_cast<int>(dirty.top) - 1;
		dirty.right = static_cast<int>(dirty.right) + 1;
		dirty.bottom = static_cast<int>(dirty.bottom) + 1;

		// status_t markError = MarkDirty(&dirty);
		status_t markError = MarkDirty();
		if (markError != B_OK)
			PRINT(("MediaContentInstance::UpdateProgress: MarkDirty returned %d (%xd)\n",
				markError, markError));
	}

}

void MediaContentInstance::UpdateNotify(void *_mci)
{
	MediaContentInstance *mci= reinterpret_cast<MediaContentInstance*>(_mci);
	if (atomic_or(&mci->fUpdateRunning, 1) == 0)
	{
		BMessage msg('updt');
		mci->PostMessage(msg);
	}
}

void MediaContentInstance::LoadBitmap(widget_id id, const char *name)
{
	const URL resourceUrl(kResourceBase, name);
	resourceCache.NewContentInstance(resourceUrl, id, this, 0, BMessage(),
		securityManager.GetGroupID(resourceUrl), 0, "");
}

status_t MediaContentInstance::HandleMessage(BMessage *message)
{
debugprint(("MediaContentInstance::HandleMessage %08x\n",this));
//message->PrintToStream();
	status_t retVal = B_OK;
	switch (message->what) {
		case B_PROPERTY_CHANGED:
			{
				const char *prop = message->FindString("prop");
				if(prop)
					NotifyListeners(B_PROPERTY_CHANGED, prop);

			}
			break;
	
		case NEW_CONTENT_INSTANCE: {
			BAutolock lock(&mediacontentlocker);
			int32 id = -1;
			atom<ContentInstance> instance;
			message->FindInt32("id", &id);
			message->FindAtom("instance",instance);
			if (id >= 0 && id < kWidgetCount) {
				fWidgets[id] = instance;
				instance->Acquire();
				Layout();
//			} else if (id == 100) {
//				printf("got new HTML document instance\n");
//				fMediaBarHTML = instance;
//				instance->Acquire();
//				Layout();
			}
			
			break;
		}

		case 'ping': {
			PRINT(("PING!\n"));
			break;
		}
		
		case 'updt':
			UpdateProgress();
			atomic_and(&fUpdateRunning, 0);
			break;


		default:
			Notification(message);
			break;
	}

debugprint(("MediaContentInstance::HandleMessage %08x done\n",this));
	return retVal;
}

void MediaContentInstance::Notification(BMessage *message)
{
	ContentInstance::Notification(message);
}

// ----------------------------------------------------------------------
// BinderNode implementation
// ----------------------------------------------------------------------
status_t MediaContentInstance::OpenProperties(void **cookie, void *copyCookie)
{
	PRINT(("MediaContentInstance::OpenProperties\n"));
	int32 *index = new int32;
	*index = 0;
	if (copyCookie)
		*index = *reinterpret_cast<int32*>(copyCookie);

	*cookie = index;
	return B_OK;
}

status_t MediaContentInstance::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	PRINT(("MediaContentInstance::NextProperty\n"));
	uint32 *index = reinterpret_cast<uint32*>(cookie);
	if (*index >= (sizeof(kProperties) / sizeof(char const*)))
		return ENOENT;

	const char *item = kProperties[(*index)++];
	strncpy(nameBuf, item, *len);
	*len = strlen(item);
	return B_OK;
}
	
status_t MediaContentInstance::CloseProperties(void *cookie)
{
	PRINT(("MediaContentInstance::CloseProperties\n"));
	int32 *index = reinterpret_cast<int32*>(cookie);
	delete index;
	return B_OK;
}

put_status_t MediaContentInstance::WriteProperty(const char */*name*/, const property &/*prop*/)
{
	PRINT(("MediaContentInstance::WriteProperty\n"));
	return EPERM;
}

get_status_t MediaContentInstance::ReadProperty(const char *name, property &prop,
	const property_list &args = empty_arg_list)
{
	debugprint(("MediaContentInstance::ReadProperty(\"%s\") %08x\n", name, this));
	if (strcasecmp(name,"Open") == 0) {
		fPlaybackEngine->Start(args[0].String().String());
		fPaused = false;
		NotifyListeners(B_PROPERTY_CHANGED, "Playing");
		NotifyListeners(B_PROPERTY_CHANGED, "Status");
		return B_OK;
	}

	if (strcasecmp(name,"Close") == 0) {
		PostStopMessage(this);
		return B_OK;
	}

	if (strcasecmp(name,"TogglePause") == 0) {
		fPlaybackEngine->TogglePause();
		fPaused = !fPaused;
		NotifyListeners(B_PROPERTY_CHANGED, "Playing");
		NotifyListeners(B_PROPERTY_CHANGED, "Status");
		Layout();
		return B_OK;
	}

	if (strcasecmp(name,"Rewind") == 0) {
		if(fPlaybackEngine)
		{
			fPlaybackEngine->Start(GetContent()->GetResource()->GetURL(), true); // sync restart
			fPlaybackEngine->TogglePause();
			fPaused = true;
			NotifyListeners(B_PROPERTY_CHANGED, "Playing");
			NotifyListeners(B_PROPERTY_CHANGED, "Status");
			Layout();
			return B_OK;
		}
	}

	if (strcasecmp(name,"Elapsed") == 0) {
		prop = fPlaybackEngine ? fPlaybackEngine->GetElapsedTime() : 0;
		return B_OK;
	}

	if (strcasecmp(name,"TotalTime") == 0) {
		prop = fPlaybackEngine ? fPlaybackEngine->GetTotalTime() : -2;
		return B_OK;
	}

	if (strcasecmp(name,"Title") == 0) {
		prop = fTitle; // use BString = operator and prop reference
		return B_OK;
	}

	if (strcasecmp(name,"Playing") == 0) {
		prop = fPaused?0:1; // use BString = operator and prop reference
		return B_OK;
	}

	if (strcasecmp(name,"Genre") == 0) {
		prop = fPlaybackEngine ? fPlaybackEngine->GetStreamGenre() : "";
		return B_OK;
	}

	if (strcasecmp(name, "URL") == 0) {
		prop = fPlaybackEngine ? fPlaybackEngine->GetHomePage() : "";
		return B_OK;
	}

	if (strcasecmp(name, "error") == 0) {
		prop = fCurrentErrorString;
		return B_OK;
	}
	
	if (strcasecmp(name, "Status") == 0) {
		if (fPlaybackEngine && fPlaybackEngine->IsOpening())
			prop = "OPENING";
		else if (fPaused && fPlaybackEngine && fPlaybackEngine->GetElapsedTime()<1)
			prop = "STOPPED";
		else if (fPaused)
		{
			//printf("paused, elapsed = %d\n", fPlaybackEngine? fPlaybackEngine->GetElapsedTime(): -1);
			prop = "PAUSED";
		}
		else if (fPlaybackEngine ? fPlaybackEngine->IsBuffering() : false)
			prop = "BUFFERING";
		else
			prop = "PLAYING";

		return B_OK;
	}

	PRINT(("MediaContentInstance::ReadProperty: no match\n"));
	return ENOENT;
}

// ----------------------------------------------------------------------
// end of BinderNode stuff
// ----------------------------------------------------------------------


MediaContent::MediaContent(void *handle)
	: 	Content(handle)
{
}

size_t MediaContent::GetMemoryUsage()
{
	return 0x30000;	// This is the size of the memory cache.
}

ssize_t MediaContent::Feed(const void */*buffer*/, ssize_t /*count*/, bool /*done*/)
{
	debugprint(("MediaContent::Feed %08x\n",this));
	return B_FINISH_STREAM;
}

status_t MediaContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg)
{
	debugprint(("MediaContent::CreateInstance %08x\n",this));
	*outInstance = new MediaContentInstance(this, handler, msg);
	debugprint(("MediaContent::CreateInstance %08x done: %08x\n",this,*outInstance));
	return B_OK;
}

void MediaContentFactory::GetIdentifiers(BMessage* into)
{
	 /*
	 ** BE AWARE: Any changes you make to these identifiers should
	 ** also be made in the 'addattr' command in the makefile.
	 */

	into->AddString(S_CONTENT_MIME_TYPES, "audio/aiff");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/basic");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/mid");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/midi");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/mpeg");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/rmf");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/wav");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/x-aiff");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/x-midi");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/x-mpeg");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/x-mpegurl");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/x-rmf");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/x-scpls");
	into->AddString(S_CONTENT_MIME_TYPES, "audio/x-wav");

	into->AddString(S_CONTENT_EXTENSIONS, "aiff");
	into->AddString(S_CONTENT_EXTENSIONS, "au");
	into->AddString(S_CONTENT_EXTENSIONS, "m3u");
	into->AddString(S_CONTENT_EXTENSIONS, "mid");
	into->AddString(S_CONTENT_EXTENSIONS, "midi");
	into->AddString(S_CONTENT_EXTENSIONS, "mp3");
	into->AddString(S_CONTENT_EXTENSIONS, "pls");
	into->AddString(S_CONTENT_EXTENSIONS, "rmf");
	into->AddString(S_CONTENT_EXTENSIONS, "wav");

	into->AddString(S_CONTENT_PLUGIN_IDS, "Mediabar");
	into->AddString(S_CONTENT_PLUGIN_DESCRIPTION, "BeIA Mediabar");
}

Content* MediaContentFactory::CreateContent(void* handle, const char*, const char*)
{
	return new MediaContent(handle);
}

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id /*you*/,
	uint32 /*flags*/, ...)
{
	if (n == 0)
		return new MediaContentFactory;

	return 0;
}
