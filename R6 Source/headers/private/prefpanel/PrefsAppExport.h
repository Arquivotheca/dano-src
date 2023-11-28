#ifndef PREFSAPPEXPORT_H_
#define PREFSAPPEXPORT_H_

#include <Message.h>
#include <Resources.h>
#include <View.h>

#include "PPWindow.h"

///////////////////////////////////////////////////////////
// ReadMessageFromResource
///////////////////////////////////////////////////////////
// return a new'ed BMessage unflattened from the resources,
// from its name or number.
// returns NULL if not found
//
BMessage* ReadMessageFromResource(BResources*,int32);
BMessage* ReadMessageFromResource(BResources*,const char*);

////////////////////////////////////////////////////////
// InstantiateFromResource
////////////////////////////////////////////////////////
// reads a flattened BMessage from a resource, unflattens
// it and tries to instantiate a BArchivable from it.
// returns : success : BArchivable object
// returns : failure : NULL
//
BArchivable* InstantiateFromResource(BResources*,int32);
BArchivable* InstantiateFromResource(BResources*,const char*);

///////////////////////////////////////////////////
// PrintMessageToStream : recursive PrintToStream
///////////////////////////////////////////////////
// prints a BMessage to a stream, in a PrintToStream
// fashion. Also prints the content of any sub-message
// called _msg (model message in a BInvoker), and
// recursively prints any sub-messages called _views
// (children views of a BWindow or BView).
//
void PrintMessageToStream(BMessage*,uint32=0);

//////////////////////////////////////////////////
// BarView : a separator bar
//////////////////////////////////////////////////

enum {
	VERTICAL_BAR_VIEW,
	HORIZONTAL_BAR_VIEW
};

class BarView : public BView {
public:
	BarView(BMessage*);

	void AttachedToWindow();
	void Draw(BRect);
	static BArchivable* Instantiate(BMessage*);

	int32 orientation;
};

#endif // PREFSAPPEXPORT_H_
