#ifndef DICT_WINDOW_H
#define DICT_WINDOW_H

#include <Window.h>

class BTextControl;
class BTextView;
class BListView;

class DictWindow : public BWindow
{
	public:
		DictWindow( void );
		virtual ~DictWindow( void );
		
		virtual void MessageReceived( BMessage *msg );
		virtual bool QuitRequested( void );
		virtual void FrameResized( float width, float height );
		status_t InitChildren( void );
		
	protected:
		BTextControl		*wordField;
		BTextView			*defBox, *synBox;
		BListView			*wordList;
};

enum {
	MSG_WORD_CHANGED = B_SPECIFIERS_END+1,
	MSG_WORD_INVOKED
};

#endif