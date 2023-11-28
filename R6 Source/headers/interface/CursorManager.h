/*******************************************************************************
/
/	File:			CursorManager.h
/
/	Description:	BCursorManager manages a prioritized cursor list.  Cursors
/					are added and removed from the list and BCursorManager
/					controls which cursor is actually displayed.
/
/	Copyright 2001, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _CURSORMANAGER_H
#define _CURSORMANAGER_H

#include <GHandler.h>
#include <GraphicsDefs.h>
#include <Locker.h>
#include <String.h>

#define MAX_CURSOR_ANIM_FRAMES 16

class BCursorManager;

class BCursorAnimator : public GHandler {
public:
							BCursorAnimator(BCursorManager *manager);
	virtual					~BCursorAnimator();
	
	virtual	status_t		HandleMessage(BMessage *message);
	
private:

	BCursorManager*			fManager;
};

typedef enum {
	B_HAND_CURSOR_TYPE		= 5,
	B_I_BEAM_CURSOR_TYPE	= 6
} cursor_type;

class BCursorManager {
public:

	typedef int32 cursor_token;
	typedef int32 queue_token;
	class cursor_data;
	struct queue_node;

							BCursorManager();
							~BCursorManager();


			void			Initialize();
								// call Initialize before trying to use a BCursorManager instance.
								// Initialize will load the default cursor and will insert it into
								// the queue with a priority of zero.
			
			status_t		GetCursorToken(cursor_type typeCode, cursor_token *outToken);
								// Retrieves the token for one of the built-in cursors
								
			status_t		GetCursorToken(cursor_data* data, cursor_token *outToken);
								// Compares the passed in cursor_data against the currently
								// loaded cursors, and either returns the token of an existing
								// cursor or creates a new cursor and returns its token.  The
								// cursor_data struct passed in is copied, so ownership stays
								// with the caller and it can be deleted after GetCursorToken
								// returns.
		
			status_t		SetCursor(cursor_token cursor, uint8 priority, queue_token *outToken);
								// Adds a cursor to the queue with the specified priority.
								// The highest priority cursor is always at the head of the queue
								// which means that it is currently showing.  The most recently
								// added cursor of a certain priority takes precedence over
								// previously added cursors of the same priority.  To prevent
								// cursor bugs where a cursor will stick around forever you
								// MUST balance every call to SetCursor with a call to RemoveCursor 
								// at the appropriate time!
								
			bool			RemoveCursor(queue_token token);
								// Pass in a queue token that was returned by SetCursor to remove
								// that cursor entry from the queue
private:
			void			SetNextCursor();
			cursor_data*	FindCursor(cursor_data *data);
			cursor_data*	FindCursor(cursor_token token);
			status_t		LoadCursor(cursor_data* data);

	BLocker					fLock;		// protects fCursors, fQueue lists as well as fInitialized
	atom<BCursorAnimator>	fAnimator;		
	cursor_data*			fCursors;
	queue_node*				fQueue;
	queue_token				fLastCursorToken;
	uint8					fNextAnimFrame;
	bool					fInitialized;
		
	friend class BCursorAnimator;
};

class BCursorManager::cursor_data {
public:
					cursor_data();
					cursor_data(const char *name, uint8 hotspotX, uint8 hotspotY);
					~cursor_data();

	void			Init();
	
	bool 			operator==(const cursor_data &) const;
	cursor_data& 	operator=(const cursor_data &);

	BString			name;			// corresponds to base name of PNG files in cursor directory
	bigtime_t		animationDelay;	// microseconds between cursor changes during animation
	rgb_color		highColor;
	rgb_color		lowColor;
	uint8			hotspotX;
	uint8			hotspotY;

private:
	void*			rawData[MAX_CURSOR_ANIM_FRAMES];
						// pointers to raw cursor data (passed to be_app->SetCursor)
						// for up to MAX_CURSOR_ANIM_FRAMES frames of animation
	cursor_data*	next;
	cursor_token	token;
	
	friend class BCursorManager;
};

// declaration of the global BCursorManager
extern BCursorManager cursorManager;

#endif // _CURSORMANAGER_H
