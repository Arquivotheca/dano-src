//========================================================================
//	MNewProjectWindow.cpp
//	Copyright 1998 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>

#include "MNewProjectWindow.h"
#include "MTriangleListView.h"
#include "MKeyFilter.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MainMenus.h"
#include "Utils.h"
#include "MHiliteColor.h"
#include "MFileUtils.h"
#include "MLocker.h"

#include <StorageKit.h>

extern BLocker CursorLock;

// ---------------------------------------------------------------------------
//		 MNewProjectWindow
// ---------------------------------------------------------------------------
//	Constructor

MNewProjectWindow::MNewProjectWindow()
	: BWindow(
		BRect(0, 0, 290, 260),
		"New Project",
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	CenterWindow(this);
	BuildWindow();
}

// ---------------------------------------------------------------------------
//		 ~MNewProjectWindow
// ---------------------------------------------------------------------------

MNewProjectWindow::~MNewProjectWindow()
{
	int32		term = fListView->FullListCountItems();
	
	for (int i = 0; i < term; ++i)
	{
		delete fListView->FullListItemAt(i);
	}
}

// ---------------------------------------------------------------------------
//		 QuitRequested
// ---------------------------------------------------------------------------
//	Determine if it's ok to close.

bool
MNewProjectWindow::QuitRequested()
{
	be_app_messenger.SendMessage(msgNewProjectClosed);
	Hide();
	
	return false;
}

// ---------------------------------------------------------------------------
//		 WindowActivated
// ---------------------------------------------------------------------------
//	Fix up the damn cursor.

void
MNewProjectWindow::WindowActivated(
	bool inActivated)
{
	BWindow::WindowActivated(inActivated);
	if (inActivated)
	{
		// Fix up the cursor
		MLocker<BLocker>	lock(CursorLock);
		if (lock.IsLocked())
		{
			if (be_app->IsCursorHidden())		// just paranoid
				be_app->ShowCursor();
			be_app->SetCursor(B_HAND_CURSOR);
		}
	}
}

// ---------------------------------------------------------------------------
//		 BuildWindow
// ---------------------------------------------------------------------------

void
MNewProjectWindow::BuildWindow()
{
	BRect				r;
	BMessage*			msg;
	BButton*			button;
	BStringView*		caption;
	BScrollView*		frame;
	BCheckBox*			checkBox;
	BOutlineListView*	list;
	BView*				topView;
	float				top = 10.0;
	const float			kListHeight = 160.0;

	// Build a special topview so we can have a grey background for
	// the window
	r = Bounds();
	topView = new BView(r, "ourtopview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	AddChild(topView);
	SetGrey(topView, kLtGray);

	// Static text
	r.Set(20, top, 255, top + 16.0);
	caption = new BStringView(r, "Open", "Select project stationery:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);
	top += 25.0;

	// List box
	r.Set(20, top, 255, top + kListHeight);
	list = new BOutlineListView(r, "listview");
	list->SetTarget(this);
	
	msg = new BMessage(msgSelectionChanged);
	list->SetSelectionMessage(msg);
	msg = new BMessage(msgRowSelected);
	list->SetInvocationMessage(msg);

	frame = new BScrollView("frame", list, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER);		// For the border
	topView->AddChild(frame);
	fListView = list;
	top += kListHeight + 10.0;

	// Create folder checkbox
	r.Set(20, top, 260, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "folder", "Create Folder", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this, nil);
	fCreateFolderCB = checkBox;
	SetGrey(fCreateFolderCB, kLtGray);
	fCreateFolderCB->SetValue(B_CONTROL_ON);
	top += 20.0;

	// Create button
	r.Set(180, top, 260, top + 15.0);
	msg = new BMessage(msgOK);
	button = new BButton(r, "OK", "Create", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetDefaultButton(button);
	SetGrey(button, kLtGray);
	fOKButton = button;

	// Cancel button
	r.Set(80, top, 160, top + 15.0);
	msg = new BMessage(msgCancel);
	button = new BButton(r, "Cancel", "Cancel", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);

	AddCommonFilter(new MTextKeyFilter(this, kBindingGlobal));
	
	// Build the list view contents
	BuildListView();
	
	fListView->MakeFocus();
}

// ---------------------------------------------------------------------------
//		 MessageReceived
// ---------------------------------------------------------------------------

void
MNewProjectWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case msgRowSelected:
			if (ExtractInfo(inMessage))
				PostMessage(B_QUIT_REQUESTED);
			break;

		case msgOK:
			ExtractInfo(inMessage);
			// fall through on purppose

		case msgCancel:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case msgSelectionChanged:
		{
			// Enable/disable the OK button depending on whether the selected row
			// is a project or not
			StationeryRow*		item = (StationeryRow*) fListView->ItemAt(fListView->CurrentSelection());
			
			fOKButton->SetEnabled(item != nil && item->IsProject());
			break;
		}
		
		default:
			if (! SpecialMessageReceived(*inMessage, this))
				BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		 ExtractInfo
// ---------------------------------------------------------------------------
//	return true if we successfully extracted info.  A double click on
//	a folder row will end up here but a folder row isn't a project.

bool
MNewProjectWindow::ExtractInfo(
	BMessage* inMessage)
{
	int32				index;
	bool				result = false;

	if (B_OK != inMessage->FindInt32("index", &index))	// line was double-clicked
		index = fListView->CurrentSelection();			// create button was clicked
		
	if (index >= 0)
	{
		StationeryRow*			row = (StationeryRow*) fListView->ItemAt(index);
		const StationeryItem*	item = row->DataItem();

		if (row->IsProject())
		{
			BMessage				msg(msgCreateProject);
	
			msg.AddRef(kFolder, &item->ref);		// flatten the ref
			msg.AddBool(kCreateFolder, fCreateFolderCB->Value() == B_CONTROL_ON);
			msg.AddBool(kEmptyProject, item->isEmptyProject);
			msg.AddString(kName, item->projectName);
		
			be_app_messenger.SendMessage(&msg);
			
			result = true;
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		 CreateProject
// ---------------------------------------------------------------------------
//	Show a save dialog.  static function called from the app.

BFilePanel*	
MNewProjectWindow::CreateProject(
	BMessage*	inMessage)
{
	entry_ref		ref;
	BFilePanel*		filePanel = nil;

	if (B_OK == inMessage->FindRef(kFolder, &ref))	// make sure it's the right kind of message
	{
		filePanel = new BFilePanel(B_SAVE_PANEL);

		BWindow*		panel  = filePanel->Window();
		BMessage		msg = *inMessage;		// duplicate the message

		msg.what = msgSaveProject;				// change the message's what
	
		filePanel->SetSaveText("Untitled.proj");
		filePanel->SetMessage(&msg);
		panel->SetTitle("Save New Project");

		filePanel->Show();
	}
	
	return filePanel;
}

// ---------------------------------------------------------------------------
//		 SaveProject
// ---------------------------------------------------------------------------
//	Disassemble the message that came from the save panel and create the new
//	project.

void
MNewProjectWindow::SaveProject(
	BMessage*	inMessage)
{
	entry_ref			folderRef;
	entry_ref			stationeryRef;
	const char *		stationeryName;
	const char *		newProjectName;
	bool				createFolder = true;
	bool				createEmptyProject = false;

	if (B_OK == inMessage->FindRef(kFolder, &stationeryRef) &&
		B_OK == inMessage->FindRef("directory", &folderRef) &&
		B_OK == inMessage->FindString(kName, &stationeryName))
	{
		inMessage->FindString("name", &newProjectName);
		inMessage->FindBool(kCreateFolder, &createFolder);
		inMessage->FindBool(kEmptyProject, &createEmptyProject);
		
		CreateNewProject(newProjectName, stationeryName, folderRef, stationeryRef, createFolder, createEmptyProject);
	}
}

class UnlockFunc : public MDirIteratorFunc
{
public:
	
	// return true to continue the iteration
	virtual bool					ProcessItem(
										BEntry& 			inEntry,
										node_flavor			inFlavor,
										const BDirectory&	inDir);
};

// ---------------------------------------------------------------------------
//		 UnlockFunc::ProcessItem
// ---------------------------------------------------------------------------

bool
UnlockFunc::ProcessItem(
	BEntry&				inEntry,
	node_flavor			/*inFlavor*/,
	const BDirectory&	/*inDir*/)
{
	BNode			node(&inEntry);
	struct stat		info;
	status_t		err = node.GetStat(&info);
	
	// unlock the file if it's locked
	if ((info.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		err = node.SetPermissions(info.st_mode & ~(S_IWUSR | S_IWGRP | S_IWOTH));

	return true;
}

// ---------------------------------------------------------------------------
//		 CreateNewProject
// ---------------------------------------------------------------------------

void
MNewProjectWindow::CreateNewProject(
	const char *		inNewProjectName,		// new name from save panel
	const char *		inStationeryName,		// name of project in stationery folder
	const entry_ref&	inFolderRef,			// location from save panel
	const entry_ref&	inStationeryRef,		// ref of stationery folder
	bool				inCreateFolder,			// create folder in new location?
	bool				inCreateEmptyProject)	// create empty project?
{
	status_t		err = B_OK;
	BDirectory		newProjectDir(&inFolderRef);

	// Create the folder
	if (inCreateFolder)
	{
		// generate the new folder name
		char*		ptr = strchr(inNewProjectName, '.');
		int32		len;
		if (ptr)
			len = ptr - inNewProjectName;
		else
			len = strlen(inNewProjectName);
		
		String		folderName(inNewProjectName, len);

		// check if a folder with this name already exists
		// and generate a new name if it does
		int32			count = 0;
		BDirectory		parentDir(&inFolderRef);
		
		while (B_FILE_EXISTS == (err = parentDir.CreateDirectory(folderName, &newProjectDir)))
		{
			folderName.Set(inNewProjectName, len);
			folderName += " copy";
			if (count > 0)
			{
				folderName += " ";
				folderName += count;
			}
			count++;
		}
	}

	if (inCreateEmptyProject)
	{
		// Create the empty project and ask the app to open it
		entry_ref		newProjectRef;
		BFile			projectFile;
		BEntry			projectEntry;

		// create the file
		err = newProjectDir.CreateFile(inNewProjectName, &projectFile);
		err = newProjectDir.FindEntry(inNewProjectName, &projectEntry);
		err = projectEntry.GetRef(&newProjectRef);

		// set the type
		BNodeInfo		mimefile(&projectFile);

		err = mimefile.SetType(kProjectMimeType);

		// tell the app to open this new project
		BMessage		msg(msgCreateEmptyProject);
		
		msg.AddRef(kEmptyProject, &newProjectRef);
		be_app_messenger.SendMessage(&msg);		
	}
	else
	{
		// Copy the contents of the stationery folder to the new location
		BDirectory		stationeryFolder(&inStationeryRef);
		err = CopyDirectory(stationeryFolder, newProjectDir);

		// unlock any locked files
		UnlockFunc			folderFunc;
		err = IterateDirectory(newProjectDir, folderFunc, B_FILE_NODE);

		// rename the copied project file to the name the user entered in the save panel
		// (clobbering the existing file if any - the user already confirmed that)
		BEntry		projectEntry;
		err = newProjectDir.FindEntry(inStationeryName, &projectEntry);
		err = projectEntry.Rename(inNewProjectName, true);

		// tell the app to open it
		entry_ref		newProjectRef;
		BMessage		msg(B_REFS_RECEIVED);
		err = projectEntry.GetRef(&newProjectRef);
		msg.AddRef("refs", &newProjectRef);
		
		be_app_messenger.SendMessage(&msg);
	}
}

// ---------------------------------------------------------------------------
//		 GetStationeryFolder 
// ---------------------------------------------------------------------------

status_t
MNewProjectWindow::GetStationeryFolder(
	entry_ref& outRef)
{
	BEntry			entry;
	status_t		err = MFileUtils::SystemDirectory().FindEntry(
								kStationeryFolderName, &entry, true);
	
	if (B_OK == err)
	{
		err = entry.GetRef(&outRef);	
	}

	return err;
}

class FolderIteratorFunc : public MDirIteratorFunc
{
public:
	
									FolderIteratorFunc(
										StationeryFolderItem& inFolderItem)
										: fFolderItem(inFolderItem) {}
	virtual							~FolderIteratorFunc() {}
	
	// return true to continue the iteration
	virtual bool					ProcessItem(
										BEntry& 			inEntry,
										node_flavor			inFlavor,
										const BDirectory&	inDir);
private:

	StationeryFolderItem& fFolderItem;
};

// ---------------------------------------------------------------------------
//		 FolderIteratorFunc::ProcessItem
// ---------------------------------------------------------------------------

bool
FolderIteratorFunc::ProcessItem(
	BEntry&				inEntry,
	node_flavor			inFlavor,
	const BDirectory&	inDir)
{
	ASSERT(inFlavor == B_DIRECTORY_NODE || inFlavor == B_FILE_NODE);
	StationeryFolderItem*	itemP;
	status_t				err;
	bool					more = true;

	if (inFlavor == B_DIRECTORY_NODE)
	{
		// item we're iterating over is a folder
		// initialize our StationeryItem
		itemP = new StationeryFolderItem;
		inEntry.GetRef(&itemP->fItem.ref);
		itemP->fItem.isEmptyProject = false;
		itemP->fItem.hasProject = false;
		itemP->fItem.subHasProject = false;

		// Iterate over this subfolder
		FolderIteratorFunc		folderFunc(*itemP);
		BDirectory				dir(&inEntry);
		err = IterateDirectory(dir, folderFunc);
		
		// If there's a project in a subdirectory then save
		// this StationeryFolderItem
		int32					count = itemP->fList.CountItems();

		if (B_OK == err && (count > 0 || itemP->fItem.hasProject))
		{
			if (count > 0)
				itemP->fItem.subHasProject = true;
			fFolderItem.fList.AddItem(itemP);
		}
		else
			delete itemP;
	}
	else
	{
		// It's a file
		BNode			node(&inEntry);
		BNodeInfo		info(&node);
		char			mimetype[B_MIME_TYPE_LENGTH];
		
		if (B_OK == info.GetType(mimetype) && 0 == strcmp(mimetype, kProjectMimeType))
		{
			// It's a project file
			// save the info for the file's parent
			BEntry		parentDir;
			inDir.GetEntry(&parentDir);
			parentDir.GetRef(&fFolderItem.fItem.ref);
			mimetype[0] = '\0';
			inEntry.GetName(mimetype);

			fFolderItem.fItem.projectName = mimetype;
			fFolderItem.fItem.isEmptyProject = false;
			fFolderItem.fItem.hasProject = true;
			fFolderItem.fItem.subHasProject = false;

			// Don't search any more once we find a project file
			more = false;
		}
	}
	
	return more;
}

// ---------------------------------------------------------------------------
//		 BuildListView
// ---------------------------------------------------------------------------

void
MNewProjectWindow::BuildListView()
{
	// Add empty project item
	StationeryItem		item;
	item.isEmptyProject	= true;
	item.hasProject	= false;
	item.subHasProject	= false;
	
	StationeryRow*		row = new StationeryRow(item, 0, false);
	
	row->SetParentFont(fListView);
	fListView->AddItem(row);
	fListView->Select(0);

	// Add the rest of the items
	StationeryFolderItem		stationeryFolder;
	
	stationeryFolder.fItem.isEmptyProject = false;
	stationeryFolder.fItem.hasProject = false;
	stationeryFolder.fItem.subHasProject = false;
	
	if (B_OK == GetStationeryFolder(stationeryFolder.fItem.ref))
	{
		BDirectory				stationery(&stationeryFolder.fItem.ref);
		FolderIteratorFunc		folderFunc(stationeryFolder);

		if (B_OK == IterateDirectory(stationery, folderFunc))
		{
			// Build the listview
			stationeryFolder.AddToView(*fListView, -1);
		}
	}
}

// ---------------------------------------------------------------------------
//		 StationeryRow
// ---------------------------------------------------------------------------

StationeryRow::StationeryRow(
	StationeryItem&		inData,
	uint32 				outlineLevel, 
	bool 				expanded)
	: BListItem(outlineLevel, expanded), fData(inData), fHasFont(false)
{
}

// ---------------------------------------------------------------------------
//		 ~StationeryRow
// ---------------------------------------------------------------------------

StationeryRow::~StationeryRow()
{
}

// ---------------------------------------------------------------------------
//		 IsProject
// ---------------------------------------------------------------------------

bool
StationeryRow::IsProject()
{
	return fData.hasProject || fData.isEmptyProject;
}

// ---------------------------------------------------------------------------
//		 SetParentFont
// ---------------------------------------------------------------------------

const char * kItalicFontStyle = "Italic";

void
StationeryRow::SetParentFont(
	BView *owner)
{
	// Set the font
	if (fData.hasProject)
		owner->SetFont(be_plain_font);
	else
	if (fData.subHasProject)
		owner->SetFont(be_bold_font);
	else
	if (fData.isEmptyProject)
	{
		// Make a best guess at choosing an Italic font
		// if be_plain_font doesn't have an Italic version
		// we get be_plain_font by itself
		BFont			font; 

		if (!fHasFont)
		{
			fItalicFont = *be_plain_font;
			fItalicFont.SetFace(B_ITALIC_FACE);
			fHasFont = true;
		}

		font.SetTo(fItalicFont, B_FONT_FAMILY_AND_STYLE);

		owner->SetFont(&font);
	}
}

// ---------------------------------------------------------------------------
//		 DrawItem
// ---------------------------------------------------------------------------

void
StationeryRow::DrawItem(
	BView *owner,
	BRect bounds,
	bool /*complete*/)
{
	// determine the title
	const char *	title;

	if (fData.isEmptyProject)	
	{
		title = "Empty Project";
	}
	else
	{
		title = fData.ref.name;
	}
	
	// set the background color
//	rgb_color 	color;
	rgb_color 	viewColor = owner->ViewColor();
	bool		selected = IsSelected();
	if (selected)
	{
//		color = HiliteColor(); 
		owner->SetLowColor(HiliteColor());
		owner->SetDrawingMode(B_OP_COPY);
		owner->FillRect(bounds, B_SOLID_LOW);
		owner->SetLowColor(viewColor);
		owner->SetDrawingMode(B_OP_OVER);		// uses background color for antialiasing
	}
	else 
	{
//		color = owner->ViewColor(); 
		owner->SetDrawingMode(B_OP_COPY);
		owner->SetHighColor(viewColor); 
		owner->FillRect(bounds); 
		owner->SetHighColor(black); 
	}
	
	SetParentFont(owner);
	owner->MovePenTo(BPoint(bounds.left + 2, bounds.bottom - 2));
	if (title != nil)
		owner->DrawString(title);
}

// ---------------------------------------------------------------------------
//		 ~StationeryFolderItem
// ---------------------------------------------------------------------------
//	Delete the contents of the list

StationeryFolderItem::~StationeryFolderItem()
{
	StationeryFolderItem*		item;
	int32						i = 0;

	while (fList.GetNthItem(item, i++))
	{
		delete item;
	}
}

// ---------------------------------------------------------------------------
//		 AddToView
// ---------------------------------------------------------------------------
//	Recursively add this folder and all its subfolders as rows in the view.

void
StationeryFolderItem::AddToView(
	BOutlineListView& 	inView,
	int32				inDepth)
{
	if (inDepth >= 0)
	{
		// Add a row for this folder to the view
		StationeryRow*		row = new StationeryRow(fItem, inDepth, true);
		
		row->SetParentFont(&inView);
		inView.AddItem(row);
	}
	
	// Add rows for the subfolders to the view
	StationeryFolderItem*	item;
	int32					newDepth = inDepth + 1;
	int32					i = 0;

	while (fList.GetNthItem(item, i++))
	{
		item->AddToView(inView, newDepth);
	}
}
