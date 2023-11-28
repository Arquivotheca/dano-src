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

#ifndef TEXT_STATUS_CONTENT_INSTANCE_H
#define TEXT_STATUS_CONTENT_INSTANCE_H

#include <Content.h>
#include <Binder.h>
#include <String.h>
#include <View.h>

#define UNUSED(x) 		

enum {
	bmsgSetText = 'sTXT',
	bmsgLegacySetText = 'uSTT'	// older clients used this message
};

// properties of a 'sTXT'
#define S_UPDATE_TEXT_DATA 			"text"
#define S_UPDATE_TEXT_LEGACY_DATA 	"status_text"

// below are properties that can be specified in the constructor's
// "params" BMessage (suitable for an EMBED)
#define	S_TEXT_STATUS_PARAM_SIZE	"fontsize"
#define S_TEXT_STATUS_PARAM_FONT	"font"
#define	S_TEXT_STATUS_PARAM_ALIGN	"textalign"
#define	S_TEXT_STATUS_PARAM_VALUE	"value"
#define	S_TEXT_STATUS_PARAM_COLOR	"color"
#define	S_TEXT_STATUS_PARAM_BGCOLOR	"bgcolor"

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
	virtual	status_t		Draw(BView *into, BRect UNUSED(exposed));
	virtual	void			Notification(BMessage *msg);
	virtual	status_t		HandleMessage(BMessage *message);
	virtual	void			Cleanup();
	
	virtual	status_t		OpenProperties(void **UNUSED(cookie), void *UNUSED(copyCookie));
	virtual	status_t		NextProperty(void *UNUSED(cookie), char *UNUSED(nameBuf), int32 *UNUSED(len));
	virtual	status_t		CloseProperties(void *UNUSED(cookie));

	virtual	put_status_t	WriteProperty(const char *name, const property &prop);
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &UNUSED(args = empty_arg_list));

	virtual void			SetText(const char* text);
	virtual bool 			IsTransparent();
	virtual void 			FilterText();
	virtual void 			PositionDisplayText();

protected:

			void		InvalidateCachedData();

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

#endif // TEXT_STATUS_H_

