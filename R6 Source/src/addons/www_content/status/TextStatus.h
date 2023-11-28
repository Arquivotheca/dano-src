// TextStatus.h
//
// A text status widget for a Wagner interface.  This widget can be used
// to display text messages, similar to the text display at the bottom
// of a Netscape window.  To embed this widget in a Wagner interface,
// use the appropriate embed tag:
//
// <embed name="plugin_name" textalign="right" value="starting value"
//   width=300 height=20 src="beos:///?signature=application/x-vnd.Be.TextStatus">
//
// To tell it to update its text, send it a message with a what code of 'uSTT' and
// a string field named "status_text" that contains the text to display.  To send
// a plugin a message from Javascript, do something like:
// 
// beos.sendMessage(document["plugin_name"], msg);
//

#ifndef TEXT_STATUS_H
#define TEXT_STATUS_H

#include <Content.h>
#include <String.h>
#include <View.h>

#define UPDATE_STATUS_TEXT	'uSTT'	
#define	STATUS_TEXT_LABEL	"status_text" // the field name in an UPDATE_STATUS_TEXT message

// below are properties that can be specified in an EMBED tag
#define	STATUS_TEXT_SIZE	"fontsize"
#define	STATUS_TEXT_ALIGN	"textalign"
#define	STATUS_TEXT_VALUE	"value"
#define	STATUS_TEXT_COLOR	"color"
#define	STATUS_TEXT_BGCOLOR	"bgcolor"

using namespace Wagner;

class TextStatusContentInstance : public ContentInstance, public BinderNode {
public:
							TextStatusContentInstance(Content *content,
													  GHandler *handler,
													  const BMessage& params);
	virtual					~TextStatusContentInstance();
	virtual status_t		AttachedToView(BView *view, uint32 *contentFlags);
	virtual status_t		DetachedFromView();
	virtual status_t		GetSize(int32 *width, int32 *height, uint32 *flags);
	virtual	status_t		FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);
	virtual	status_t		Draw(BView *into, BRect exposed);
	virtual	void			Notification(BMessage *msg);
	virtual	status_t		HandleMessage(BMessage *message);
	virtual	void			Cleanup();

	virtual	status_t		OpenProperties(void **cookie, void *copyCookie);
	virtual	status_t		NextProperty(void *cookie, char *nameBuf, int32 *len);
	virtual	status_t		CloseProperties(void *cookie);

	virtual	put_status_t	WriteProperty(const char *name, const property &prop);
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
	
			void			SetText(const char* text);

protected:

			void		InvalidateCachedData();
private:
	BString		fText;
	BString		fDisplayText;
	BFont		fFont;
	BPoint		fDisplayPoint;
	BView*		fView;
	rgb_color	fTextColor;
	rgb_color	fBackgroundColor;
	float		fFontSize;
	alignment	fAlign;
	bool		fDrawBackground;
};


class TextStatusContent : public Content {
public:
						TextStatusContent(void* handle);
	virtual				~TextStatusContent();
	virtual ssize_t		Feed(const void *buffer, ssize_t bufferLen,
							 bool done=false);
	virtual size_t		GetMemoryUsage();
	virtual	bool		IsInitialized();

private:
	virtual status_t	CreateInstance(ContentInstance **outInstance,
									   GHandler *handler, const BMessage&);
};

class TextStatusContentFactory : public ContentFactory
{
public:
	virtual void		GetIdentifiers(BMessage* into);
	virtual Content*	CreateContent(void* handle,
									  const char* mime,
									  const char* extension);
};

#endif // TEXT_STATUS_H_
