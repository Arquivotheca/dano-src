/*-----------------------------------------------------------------*/
//
//	File:		FileListView.h
//
//	Written by:	Robert Polic
//
//	Copyright 2001, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef FILE_LIST_VIEW_H
#define FILE_LIST_VIEW_H

#include </projects/iad/headers/controls/ColumnListView.h>
#include </projects/iad/headers/controls/ColumnTypes.h>
#include <MessageFilter.h>
#include <Node.h>
#include <stdlib.h>
#include <URL.h>

#include "Editor.h"
#include "Parameters.h"


#define kEXTENSION_MAP_NAME	"beia_file_extension_map"

enum COLUMNS
{
	eIconColumn = 0,
	eNameColumn,
	eModifiedColumn,
	eSizeColumn
};

enum MESSAGES
{
	eOpenParent = 'lPAR',
	eItemSelected = 'lSEL',
	eSelectAll = 'lALL',
	eMessageDropped = 'lDRP'
};

enum NODE_TYPE
{
	eDirectory = 0,
	eFile,
	eSymLink
};


//--------------------------------------------------------------------

struct extension_map
{
					extension_map(const char* ext, const char* type)
					{
						extension = (char*)malloc(strlen(ext) + 1);
						strcpy(extension, ext);
						mime_type = (char*)malloc(strlen(type) + 1);
						strcpy(mime_type, type);
						next = NULL;
					}

					~extension_map()
					{
						if (next)
							delete next;

						free(extension);
						free(mime_type);
					}

	char*			extension;
	char*			mime_type;
	extension_map*	next;
};


//====================================================================

using namespace Wagner;

class FileListItem;

class FileListView : public BColumnListView
{
	public:
							FileListView		(BRect,
												 drawing_parameters*,
												 BHandler*);
							~FileListView		();
		void				AllAttached			();
		bool				InitiateDrag		(BPoint,
												 bool);
		void				MessageDropped		(BMessage*,
												 BPoint);
		void				MessageReceived		(BMessage*);

		void				Edit				();
		bool				Editing				()
													{ return fEditor; };
		void				MakeDirectory		(const char*);
		status_t			PathProcessor		();
		void				SetURL				(URL*,
												 int32 selected_item = 0,
												 BPoint position = BPoint(0, 0));

	private:
		FileListItem*		AddItem				(const char*,
												 const char*,
												 NODE_TYPE,
												 time_t,
												 off_t,
												 node_ref nref);
		FileListItem*		AddItem				(BEntry*);
		FileListItem*		FindItem			(node_ref);
		void				KillProcessingThread();
		void				LoadExtensionMap	();
		int32				ParseFTPBuffer		(char*,
												 ssize_t);
		void				RemoveEditor		();
		void				Rename				(FileListItem*,
												 const char*);
		void				SniffFile			(const char*,
												 char*);

		volatile bool		fQuit;
		int32				fSelectedItem;
		BHandler*			fTarget;
		BPoint				fPosition;
		Editor*				fEditor;
		EditControls*		fControls;
		FileListItem*		fEditingItem;
		URL*				fURL;
		sem_id				fProcessingSem;
		drawing_parameters*	fParameters;
		extension_map*		fExtensionMap;
};


//====================================================================

class FileListFilter : public BMessageFilter
{
	public:
							FileListFilter		(BHandler*);
		filter_result		Filter				(BMessage*,
												 BHandler**);

	private:
		BHandler*			fTarget;
};


//====================================================================

class FileListItem : public BRow
{
	public:
							FileListItem		(URL*,
												 const char*,
												 const char*,
												 NODE_TYPE,
												 time_t,
												 off_t,
												 node_ref,
												 drawing_parameters*);
		const BBitmap*		Bitmap				()
													{ return fBitmap->Bitmap(); };
		char*				GetURL				();
		bool				IsDirectory			()
													{ return fNodeType == eDirectory; };
		bool				IsFile				()
													{ return fNodeType == eFile; };
		bool				IsSymLink			()
													{ return fNodeType == eSymLink; };
		const char*			Leaf				()
													{ return fLeaf.String(); };
		const char*			MimeType			()
													{ return fMime.String(); };
		time_t				Modified			()
													{ return fModified; };
		node_ref			NodeRef				()
													{ return fNodeRef; };
		const char*			Path				()
													{ return fPath.String(); };
		off_t				Size				()
													{ return fSize; };
		NODE_TYPE			NodeType			()
													{ return fNodeType; };

	private:
		bool				fIsDirectory;
		bool				fIsFile;
		bool				fIsSymLink;
		node_ref			fNodeRef;
		off_t				fSize;
		time_t				fModified;
		BBitmapField*		fBitmap;
		BString				fLeaf;
		BString				fMime;
		BString				fPath;
		NODE_TYPE			fNodeType;
		URL					fURL;
};


//====================================================================

class SizeColumn : public BSizeColumn
{
	public:
							SizeColumn			(const char*,
												 float,
												 float,
												 float,
												 alignment align = B_ALIGN_RIGHT);
		void				DrawField			(BField*,
												 BRect,
												 BView*);
};


//--------------------------------------------------------------------

class SizeField : public BSizeField
{
	public:
							SizeField			(uint32,
												 bool);

		bool				File				()
													{ return fFile; };
	private:
		bool				fFile;
};
#endif
