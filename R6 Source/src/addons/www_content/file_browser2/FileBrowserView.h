/*-----------------------------------------------------------------*/
//
//	File:		FileBrowserView.h
//
//	Written by:	Robert Polic
//
//	Copyright 2001, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef FILE_BROWSER_VIEW_H
#define FILE_BROWSER_VIEW_H

#include <MessageFilter.h>
#include <URL.h>
#include <View.h>

#include "Parameters.h"
#include "FileListView.h"


//--------------------------------------------------------------------

enum WIDTHS
{
	eSlashWidth = 0,
	eEllipsisWidth,
	eLeftRightMarginWidth,
	eFolderWidth,
	eWidthCount
};

enum COMMANDS
{
	eDisplayVolume = 'cCHG',
	eDisplayIDrive = 'cIDR',
	eOpenItems = 'cOPN',
	eEditItem = 'cEDT',
	eCopyItems = 'cCPY',
	eAttachItems = 'cATH',
	eDeleteItems = 'cDEL',
	eMakeDirectory = 'cMKD'
};

enum DRAW_LEVEL
{
	eDrawIconNameSlash = 0,
	eDrawIconEllipsisSlash,
	eDrawIconSlash,
	eDrawIcon,
	eDrawSlash,
	eDrawNone
};


//--------------------------------------------------------------------

struct node
{
							node(node* parent_node, node* child_node, const char* node_name)
							{
								if (parent_node)
								{
									child = parent_node->child;
									parent = parent_node;
									parent_node->child = this;
									if (child)
										child->parent = this;
								}
								else if (child_node)
								{
									child = child_node;
									parent = child_node->parent;
									child_node->parent = this;
									if (parent)
										parent->child = this;
									
								}
								else
								{
									parent = NULL;
									child = NULL;
								}
								name.SetTo(node_name);
								greyed = false;
								selection = 0;
								position.Set(0, 0);
							}

							~node()
							{
								if (parent)
									parent->child = child;
								if (child)
									child->parent = parent;
							}

	bool					greyed;
	node*					parent;
	node*					child;
	BString					name;
	float					name_width;
	float					offset;
	float					total_width;
	int32					selection;
	BPoint					position;
	DRAW_LEVEL				draw;
};


//========================================================================

using namespace Wagner;

class FileListView;

class FileBrowserView : public BView
{
	public:
							FileBrowserView		(BRect,
												 drawing_parameters*,
												 const char*);
							~FileBrowserView	();
		void				AttachedToWindow	();
		void				Draw				(BRect);
		void				MessageReceived		(BMessage*);
		void				MouseDown			(BPoint);
		void				MouseMoved			(BPoint,
												 uint32,
												 const BMessage*);
		void				MouseUp				(BPoint);
	
		void				AutoScroller		();

	private:
		void				AddDirectoryEntries	(const char*,
												 BMessage*);
		BMessage*			BuildItemList		(bool remove = false,
												 bool recursive = false);
		void				BuildNodeList		(const char*);
		void				DeleteNodeList		(node*);
		void				DrawFolder			(BView*,
												 bool);
		void				DrawPath			(int32 must_show_item = -1,
												 bool hilite_item = false);
		void				Fit					(float,
												 int32);
		void				NewURL				(BString*,
												 int32 selected_item = 0,
												 BPoint position = BPoint(0, 0));
		void				OpenItem			(FileListItem*);
		void				OpenParent			();
		uint32				OpenWith			(const char*);
		void				SetPath				();

		volatile bool		fQuit;
		float				fWidths[eWidthCount];
		volatile int32		fAutoScroll;
		BBitmap*			fOffscreen;
		BRect				fPathBounds;
		BView*				fPathView;
		URL					fURL;
		FileListView*		fFileListView;
		drawing_parameters*	fParameters;
		font_height			fFontInfo;
		thread_id			fAutoScroller;
		node*				fClicked;
		node*				fNodeList;
};
#endif
