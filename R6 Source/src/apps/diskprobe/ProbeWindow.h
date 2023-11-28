//--------------------------------------------------------------------
//	
//	ProbeWindow.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef PROBE_WINDOW_H
#define PROBE_WINDOW_H

#include <Alert.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Drivers.h>
#include <File.h>
#include <Font.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <List.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Point.h>
#include <PrintJob.h>
#include <Rect.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Window.h>

#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			457
#define WIND_HEIGHT			400

enum	MENUS	/* file */	{M_NEW = M_OPEN_DEVICE + 64,
							 M_PRINT_SETUP, M_PRINT,
				/* edit */	 M_UNDO, M_SELECT_ALL, M_BASE,
							 M_FIND, M_FIND_AGAIN, M_FONT_SIZE,
				/* block */	 M_NEXT, M_PREVIOUS, M_LAST, M_GO,
							 M_GO_SWAP, M_WRITE, M_ADD_BOOKMARK,
							 M_BLOCK_SIZE, M_KEY, M_BOOKMARK,
							 M_BLOCK, M_ATTRIBUTE, M_SLIDER,
							 M_NEXT_KEY, M_PREVIOUS_KEY};

#define DEFAULT_BLOCK_SIZE	512
#define BYTES_WIDE			 16
#define GAP					 10

class	THeaderView;
class	TContentView;
class	TSliderView;
class	TAttributesWindow;

extern "C"	int32	_kstatfs_(dev_t, long*, int32, const char*,struct fs_info*);

typedef struct {
	TAttributesWindow	*window;
	uint32				type;
	char				name[B_FILE_NAME_LENGTH];
	size_t				size;
	void				*data;
} attribute;


//====================================================================

class TProbeWindow : public BWindow {

private:

bool			fReadOnly;
uchar			*fBuffer;
uchar			*fSearch;
uchar			*fUndoBuffer;
int32			fDevice;
int32			fEndSel;
int32			fSearchLen;
int32			fStartSel;
int32			fType;
int32			fWindowCount;
off_t			fBlock;
off_t			fBlockSize;
off_t			fDeviceSize;
off_t			fLast;
off_t			fLength;
ssize_t			fBytesRead;
BBitmap			*fBitmap;
BFile			*fFile;
BList			*fAttrList;
BMenu			*fAttributes;
BMenu			*fBookmarks;
BMenu			*fDeviceMenu;
BMenu			*fGoSelection;
BMenuBar		*fMenuBar;
BMenuItem		*fBlockMenu;
BMenuItem		*fCopy;
BMenuItem		*fDefaultSize;
BMenuItem		*fFindAgain;
BMenuItem		*fGoSwap;
BMenuItem		*fGo;
BMenuItem		*fLastMenu;
BMenuItem		*fNext;
BMenuItem		*fPaste;
BMenuItem		*fPrevious;
BMenuItem		*fUndo;
BMenuItem		*fWrite;
BRect			fZoom;
BScrollView		*fScrollView;
TContentView	*fContentView;
THeaderView		*fHeaderView;
TSliderView		*fSliderView;
node_ref		fNode;
prefs			*fPrefs;

public:

entry_ref		*fRef;

				TProbeWindow(BRect, char*, void*, int32, prefs*);
				~TProbeWindow(void);
virtual void	FrameResized(float, float);
virtual void	MenusBeginning(void);
virtual void	MessageReceived(BMessage*);
virtual bool	QuitRequested(void);
virtual void	WindowActivated(bool);
virtual void	Zoom(BPoint, float, float);
void			AttrQuit(bool, TAttributesWindow*);
void			BuildAttrMenu(void);
BRect			CalcMaxSize(void);
status_t		CloseDevice(void);
status_t		CloseFile(void);
status_t		GetSelection(off_t*, off_t*);
void			Init(int32);
void			OpenDevice(char*);
status_t		OpenFile(entry_ref*);
void			Print(void);
status_t		PrintSetup(void);
status_t		Prompt(const char*);
ssize_t			Read(off_t);
void			ScanDir(const char*, BMenu*);
void			Search(void);
void			SetLimits(void);
void			SetOffset(off_t);
ssize_t			Write(void);
};


//====================================================================

class TFilter : public BMessageFilter {

private:

TProbeWindow				*fWindow;

public:
						TFilter(TProbeWindow*);
virtual filter_result	Filter(BMessage*, BHandler**);
};
#endif
