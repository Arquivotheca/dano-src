
#ifndef __TVIEW_H
#define __TVIEW_H 1

#include <View.h>
#include <StringView.h>
#include <Button.h>
#include <Box.h>
#include <TextControl.h>

class TView : public BBox
{
public:
	TView(BRect rect, const char *name, bool rec, bool stop, bool play);
	virtual void MessageReceived(BMessage *msg);
private:
};

#endif // __TVIEW_H
