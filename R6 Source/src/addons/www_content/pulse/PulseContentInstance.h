// Pulse.h
//
// A text status widget for a Wagner interface.  This widget can be used
// to display text messages, similar to the text display at the bottom
// of a Netscape window.  To embed this widget in a Wagner interface,
// use the appropriate embed tag:
//
// <embed name="plugin_name" textalign="right" value="starting value"
//   width=300 height=20 src="beos:///?signature=application/x-vnd.Be.Pulse">
//
// To tell it to update its text, send it a message with a what code of 'uSTT' and
// a string field named "status_text" that contains the text to display.  To send
// a plugin a message from Javascript, do something like:
// 
// beos.sendMessage(document["plugin_name"], msg);
//

#ifndef PULSE_CONTENT_INSTANCE_H
#define PULSE_CONTENT_INSTANCE_H

#include <Content.h>
#include <Binder.h>
#include <String.h>
#include <View.h>

enum {
	bmsgUpdatePulse = 'uPUL'
};

typedef enum {
	pdCPU,
	pdMem
} pulseDisplayMode;

using namespace Wagner;

class PulseContentInstance : public ContentInstance, public BinderNode {
public:
							PulseContentInstance(Content *content,
													  GHandler *handler,
													  const BMessage& params);
	virtual					~PulseContentInstance();
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

	virtual bool			IsTransparent();
	virtual void 			Update();

protected:

			void			InvalidateCachedData();

	int 		fNumBars;
	double 		fBarValue[B_MAX_CPU_COUNT];
	bigtime_t 	prevactive[B_MAX_CPU_COUNT];
	bigtime_t 	prevtime;
	rgb_color 	framecolor;
	rgb_color 	activecolor;
	rgb_color 	idlecolor;

	BPoint		fDisplayPoint;
	BView*		fView;

	pulseDisplayMode	fDisplayMode;
	bool		fDrawBackground;
	
	double fUpdatePeriodSeconds;
};

#endif // PULSE_CONTENT_INSTANCE_H
