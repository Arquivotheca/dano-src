//========================================================================
//	MBuildersKeeper.cpp
//	Copyright 1997Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "CString.h"

#include "MBuildersKeeper.h"
#include "MPlugInBuilder.h"
#include "MPrefsContainer.h"
#include "MPlugInLinker.h"
#include "MInformationMessageWindow.h"
#include "MMessageItem.h"
#include "MProject.h"
#include "MFileUtils.h"
#include "IDEMessages.h"
#include "MTargetTypes.h"
#include "MLocker.h"

#include <Application.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <String.h>
#include <MenuItem.h>

MList<MPlugInBuilder*>		MBuildersKeeper::fBuilderList;
MList<MPlugInLinker*>		MBuildersKeeper::fLinkerList;
MList<BuilderRec*>			MBuildersKeeper::fBuilderLinkedlist;
MList<AddOnRec*>			MBuildersKeeper::fAddOnlist;
BLocker						MBuildersKeeper::fLock("keeper");

// ---------------------------------------------------------------------------
//		MBuildersKeeper
// ---------------------------------------------------------------------------

MBuildersKeeper::MBuildersKeeper()
{
	MLocker<BLocker>		lock(fLock);

	GetBuilders();
}

// ---------------------------------------------------------------------------
//		~MBuildersKeeper
// ---------------------------------------------------------------------------

MBuildersKeeper::~MBuildersKeeper()
{
	MLocker<BLocker>	lock(fLock);
	MPlugInBuilder*		builder;
	AddOnRec*			rec;
	int32				i = 0;

	// Delete all the builders
	while (fBuilderList.GetNthItem(builder, i++))
		delete builder;

	// Delete all the addonrecs and unload all the add-ons
	i = 0;
	while (fAddOnlist.GetNthItem(rec, i++))
	{
//		unload_add_on(rec->id);	// needs to be done after the views in the prefs window are deleted
		delete rec;
	}

	fBuilderList.MakeEmpty();
	fAddOnlist.MakeEmpty();
}

// ---------------------------------------------------------------------------
//		GetLinker
// ---------------------------------------------------------------------------
//	Get the linker builder whose name is inLinkerToolName.

MPlugInLinker*
MBuildersKeeper::GetLinker(
	const char *	inLinkerToolName)
{
	MPlugInLinker*		linker;
	int32				i = 0;

	while (fLinkerList.GetNthItem(linker, i++))
	{
		char		toolName[B_FILE_NAME_LENGTH] = { '\0' };
		
		if (B_NO_ERROR == linker->GetToolName(nil, toolName, B_FILE_NAME_LENGTH, kLinkStage, kLink)
			&& 0 == strcmp(inLinkerToolName, toolName))
		{
			return linker;		// early exit
		}
	}

	return nil;
}

// ---------------------------------------------------------------------------
//		GetBuilderToolName
// ---------------------------------------------------------------------------
//	Get the correct toolname from a builder.  A toolname may be a partial
//	or full path to the tool.

void
MBuildersKeeper::GetBuilderToolName(
	MPlugInBuilder*		inBuilder,
	String&				inoutToolName,
	MakeStageT			inMakeStage,
	MakeActionT			inAction)
{
	// If no make stage is specified then get a valid one from the builder
	if (inMakeStage == -1)
	{
		MakeStageT		makeStage = inBuilder->MakeStages();
		
		if ((makeStage & kCompileStage) != 0)
		{
			inMakeStage = kCompileStage;
			inAction = kCompile;
		}
		else
		if ((makeStage & kPrecompileStage) != 0)
		{
			inMakeStage = kPrecompileStage;
			inAction = kPrecompile;
		}
		else
		if ((makeStage & kLinkStage) != 0)
		{
			inMakeStage = kLinkStage;
			inAction = kLink;
		}
		else
		if ((makeStage & kPostLinkStage) != 0)
		{
			inMakeStage = kPostLinkStage;
			inAction = kPostLinkExecute;
		}
		else
		{
			// Defaults for builders that return defective values for MakeStages()
			ASSERT(false);
			inMakeStage = kCompileStage;
			inAction = kCompile;			
		}	
	}

	char				buffer[1000] = { '\0' };
	
	ASSERT(inMakeStage != -1 && inAction != -1);

	if (B_NO_ERROR == inBuilder->GetToolName(nil, buffer, 1000, inMakeStage, inAction))
	{
		const char *		toolname = strrchr(buffer, '/');
		if (toolname != nil)
			inoutToolName = ++toolname;
		else
			inoutToolName = buffer;
	}
	else
		inoutToolName = "";
}

// ---------------------------------------------------------------------------
//		GetBuilders
// ---------------------------------------------------------------------------
//	Get all the builders from the plugins.

void
MBuildersKeeper::GetBuilders()
{
	// Plug-ins
	app_info	info;
	be_app->GetAppInfo(&info);
	BEntry 		file(&info.ref);
	BDirectory	appDir;
	file.GetParent(&appDir);
	BDirectory	pluginDir;
	BDirectory	prefsDir;
	status_t	err = FindDirectory(appDir, kPluginsFolderName, &pluginDir);

	if (err == B_OK)
		err = FindDirectory(pluginDir, kPrefsAddOnsName, &prefsDir);

	if (err != B_OK) {
		BString leaf = kPluginsFolderName;
		leaf += "/";
		leaf += kPrefsAddOnsName;
		BPath path(&appDir, leaf.String());
		
		// Post a message here that the installation might be incorrect
		BString text = "Cannot find add-ons directory "B_UTF8_OPEN_QUOTE;
		text += path.Path();
		text += B_UTF8_CLOSE_QUOTE".  Projects cannot be built.";
	
		InfoStruct info;
		info.iTextOnly = true;
		strncpy(info.iLineText, text.String(), kLineTextLength);

		BMessage msg(msgAddInfoToMessageWindow);
		msg.AddData(kInfoStruct, kInfoType, &info, sizeof(info));
		MMessageWindow::GetGeneralMessageWindow()->PostMessage(&msg);
		MMessageWindow::GetGeneralMessageWindow()->PostMessage(msgShowAndActivate);					
	}
	
	while (B_OK == prefsDir.GetNextEntry(&file))
	{
		BPath		path;
		if (file.IsFile() && B_NO_ERROR == file.GetPath(&path))
		{
			AddOnRec			rec;

			rec.id = load_add_on(path.Path());

			if (rec.id >= 0) 
			{
				makeaddonBuilder	makeBuilder = nil;
				makeaddonLinker		makeLinker = nil;
				bool				hasBuilder = false;
				bool				hasLinker = false;

				rec.builder = nil;
				rec.linker = nil;
	
				get_image_symbol(rec.id, "MakeAddOnBuilder", B_SYMBOL_TYPE_TEXT, (void**) &makeBuilder);
				get_image_symbol(rec.id, "MakeAddOnLinker", B_SYMBOL_TYPE_TEXT, (void**) &makeLinker);
				if (makeBuilder) 
				{
					int32				i = 0;
					err = B_NO_ERROR;

					// Iterate through all the builders provided by this plug-in
					// let's assume that 100 is a reasonable maximum number of
					// builders to be returned from any single add-on.
					// Having a maximum protects us from an infinite loop
					// for a defective add-on
					while (err == B_NO_ERROR && i < 100)
					{
						rec.builder = nil;
	
						err = makeBuilder(i++, rec.builder);	// Call the plug-in
						
						if (err == B_NO_ERROR && rec.builder != nil)
						{
							fBuilderList.AddItem(rec.builder);
							hasBuilder = true;
						}
					}
				} 
	
				// Only one linker builder is allowed for each plugin
				if (makeLinker) 
				{
					err = B_NO_ERROR;
	
					err = makeLinker(rec.linker);	// Call the plug-in
					
					if (err == B_NO_ERROR && rec.linker != nil)
					{
						fLinkerList.AddItem(rec.linker);
						hasLinker = true;
					}
				} 
				
				//	Save the record for the add_on
				if (hasBuilder || hasLinker)
				{
					AddOnRec*		record = new AddOnRec;
					*record = rec;
					fAddOnlist.AddItem(record);
				}
				else 
				{
					unload_add_on(rec.id);
					// Post a message here if couldn't use the plug-in
					char		name[B_FILE_NAME_LENGTH] = { '\0' };
					String		text = "Couldn't load the add_on " B_UTF8_OPEN_QUOTE "Prefs_add_ons/";
					file.GetName(name);
					text += name;
					text += B_UTF8_CLOSE_QUOTE".";
				
					InfoStruct info;
					info.iTextOnly = true;
					strncpy(info.iLineText, text, kLineTextLength);
					
					BMessage msg(msgAddInfoToMessageWindow);
					msg.AddData(kInfoStruct, kInfoType, &info, sizeof(info));
					MMessageWindow::GetGeneralMessageWindow()->PostMessage(&msg);
					MMessageWindow::GetGeneralMessageWindow()->PostMessage(msgShowAndActivate);					
				}
			}
		}
	}

	GenerateBuilderLinkedList();
}

// ---------------------------------------------------------------------------
//		BuildTargetList
// ---------------------------------------------------------------------------
//	Build a list of ProjectTargetRecs for mapping a particular filetype/extension
//	to a particular linked list of builders.  For those targets that have
//	no builder we add a record in which the builder is nil.  All uses
//	of this list or of targets must consider the fact that builders can be nil.
//	The contents of the list belong to the caller.
//	Must be called after the constructor.

void
MBuildersKeeper::BuildTargetList(
	const TargetRec*	inTargetRecs,
	int32				inTargetCount,
	ProjectTargetList&	inTargetList)
{
	inTargetList.MakeEmpty();

	ASSERT(inTargetRecs != nil || inTargetCount == 0);
	
	String			builderToolName;
	
	for (int32 i = 0; i < inTargetCount; i++)
	{
		int32				j = 0;
		BuilderRec*			builderRec;

		while (fBuilderLinkedlist.GetNthItem(builderRec, j++))
		{
			if (builderRec->isLinker)
				GetBuilderToolName(builderRec->builder, builderToolName, kLinkStage, kLink);
			else
				GetBuilderToolName(builderRec->builder, builderToolName);		
			ASSERT(builderToolName != "");

			if (inTargetRecs[i].ToolName[0] == 0 ||
				0 == strcmp(inTargetRecs[i].ToolName, builderToolName))
			{
				ProjectTargetRec*	projRec = new ProjectTargetRec;
				projRec->Target = inTargetRecs[i];
				if (inTargetRecs[i].ToolName[0] == 0)
					projRec->Builder = nil;
				else
					projRec->Builder = builderRec;
				inTargetList.AddItem(projRec);
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		GenerateBuilderLinkedList
// ---------------------------------------------------------------------------
//	Generate a list of builders.  Each tool has a single entry in the list.
//	That entry points to a linked list of other builders for that tool.

void
MBuildersKeeper::GenerateBuilderLinkedList()
{
	int32					i = 0;
	MPlugInBuilder*			builder;
	MPlugInLinker*			linker;
	
	while (fBuilderList.GetNthItem(builder, i++))
	{
		AddToBuilderLinkedList(*builder);
	}
	
	i = 0;
	while (fLinkerList.GetNthItem(linker, i++))
	{
		AddToBuilderLinkedList(*linker, true);
	}
}

// ---------------------------------------------------------------------------
//		AddToBuilderLinkedList
// ---------------------------------------------------------------------------
//	Generate a list of builders.  Each tool has a single entry in the list.
//	That entry points to a linked list of other builders for that tool.

void
MBuildersKeeper::AddToBuilderLinkedList(
	MPlugInBuilder&		inBuilder,
	bool				inIsLinker)
{
	String				name;
	MakeStageT			makeStage = kInvalidStage;
	MakeActionT			makeAction = kInvalidAction;

	if (inIsLinker)
	{
		makeStage = kLinkStage;
		makeAction = kLink;
	}

	GetBuilderToolName(&inBuilder, name, makeStage, makeAction);
	if (name == "")
		return;	//	Possibly show an alert here ????

	String				recname;
	BuilderRec*			rec = new BuilderRec;
	BuilderRec*			listrec;
	bool				found = false;
	int32				j = 0;

	rec->builder = &inBuilder;
	rec->next = nil;
	rec->isLinker = inIsLinker;

	// If an entry for this tool already exists add this
	// rec to the end of the linked list
	while (fBuilderLinkedlist.GetNthItem(listrec, j++))
	{
		if (listrec->isLinker)
			GetBuilderToolName(listrec->builder, recname, kLinkStage, kLink);
		else
			GetBuilderToolName(listrec->builder, recname);		
		
		if (recname == name)
		{
			BuilderRec*		nextrec = listrec;
			while (nextrec->next != nil)
				nextrec = nextrec->next;
			
			nextrec->next = rec;
			found = true;
			break;
		}
	}
	
	// If an entry for this tool doesn't exist add it 
	// to the builder list
	if (! found)
	{
		fBuilderLinkedlist.AddItem(rec);
	}
}

// ---------------------------------------------------------------------------
//		ValidateGenericData
// ---------------------------------------------------------------------------
//	Must be called after the constructor.

bool
MBuildersKeeper::ValidateGenericData(
	BMessage&	inMessage)
{
	MPlugInBuilder*		builder;
	MPlugInLinker*		linker;
	int32				i = 0;
	BMessage			msg;
	bool				changed = false;

	while (fBuilderList.GetNthItem(builder, i++))
	{
		uint32		type = builder->MessageDataType();

		msg.MakeEmpty();
		MPrefsContainer::FillMessage(inMessage, msg, type);
		if (builder->ValidateSettings(msg))
		{
			changed = true;
			MPrefsContainer::CopyData(msg, inMessage);
		}
	}
	
	i = 0;
	while (fLinkerList.GetNthItem(linker, i++))
	{
		uint32		type = linker->MessageDataType();

		msg.MakeEmpty();
		MPrefsContainer::FillMessage(inMessage, msg, type);
		if (linker->ValidateSettings(msg))
		{
			changed = true;
			MPrefsContainer::CopyData(msg, inMessage);
		}
	}

	return changed;
}

// ---------------------------------------------------------------------------
//		GetNthAddOnID
// ---------------------------------------------------------------------------
//	Accessor so the addonlist doesn't have to be public.

bool
MBuildersKeeper::GetNthAddOnID(
	image_id&		outID,
	int32			inIndex,
	MPlugInLinker*&	outLinker)
{
	AddOnRec*	rec;
	
	if (fAddOnlist.GetNthItem(rec, inIndex))
	{
		outID = rec->id;
		outLinker = rec->linker;
		return true;
	}
	else
		return false;
}

// ---------------------------------------------------------------------------
//		GetNthBuilder
// ---------------------------------------------------------------------------
//	Accessor so the builderlist doesn't have to be public.

bool
MBuildersKeeper::GetNthBuilder(
	BuilderRec*&	outRec,
	int32			inIndex)
{
	return fBuilderLinkedlist.GetNthItem(outRec, inIndex);
}

// ---------------------------------------------------------------------------
//		GetNthLinker
// ---------------------------------------------------------------------------
//	Accessor so the linkerlist doesn't have to be public.

bool
MBuildersKeeper::GetNthLinker(
	MPlugInLinker*&	outLinker,
	int32			inIndex)
{
	return fLinkerList.GetNthItem(outLinker, inIndex);
}

// ---------------------------------------------------------------------------
//		GetBuilderNames
// ---------------------------------------------------------------------------
//	Fills the list with the names of all the builders for use
//	in the toolname popup in the targets prefs panel.
//	The names are allocated with malloc and should be freed by
//	the caller.

void
MBuildersKeeper::GetBuilderNames(
	MList<char*>&	outList)
{
	String			toolname;
	BuilderRec*		rec;
	int32			i = 0;

	while (fBuilderLinkedlist.GetNthItem(rec, i++))
	{
		if (rec->builder != nil)
		{
			if (rec->isLinker)
				GetBuilderToolName(rec->builder, toolname, kLinkStage, kLink);
			else
				GetBuilderToolName(rec->builder, toolname);		
			outList.AddItem(strdup(toolname));
		}
	}
}

// ---------------------------------------------------------------------------
//		BuildTargetsPopup
// ---------------------------------------------------------------------------
//	Build the popup for the targets prefs panel.

void
MBuildersKeeper::BuildTargetsPopup(
	BPopUpMenu&		inPopup)
{
	BMenuItem*			item;

	if (fLinkerList.CountItems() == 0)
	{
		item = new BMenuItem("<no plugins installed>", new BMessage(msgTargetChosen));
		item->SetEnabled(false);
		item->SetMarked(true);
		inPopup.AddItem(item);
		inPopup.SetEnabled(false);
	}
	else
	{
		MPlugInLinker*		linker;
		int32				i = 0;
	
		while (fLinkerList.GetNthItem(linker, i++))
		{
			item = new BMenuItem(linker->TargetName(), new BMessage(msgTargetChosen));
			inPopup.AddItem(item);
		}
	}
}

// ---------------------------------------------------------------------------
//		ProjectChanged
// ---------------------------------------------------------------------------
//	A project opened or closed or files were added, removed,
//	or rearranged in the project window.
//	A builder will be associated with a given project if it has no
//	linker or its linker matches the project's linker.
 
void
MBuildersKeeper::ProjectChanged(MProject& inProject, ChangeT inChange)
{
	MLocker<BLocker>		lock(fLock);
	MPlugInBuilder*			builder;
	MPlugInLinker*			linker;
	const char *			linkerName = inProject.LinkerName();
	const char *			builderLinker;
	int32					i = 0;

	while (fBuilderList.GetNthItem(builder, i++))
	{
		builderLinker = builder->LinkerName();
		if (builderLinker[0] == '\0' || 0 == strcmp(linkerName, builderLinker))
			builder->ProjectChanged(inProject, inChange);
	}
	
	i = 0;
	while (fLinkerList.GetNthItem(linker, i++))
	{
		builderLinker = linker->LinkerName();
		if (builderLinker[0] == '\0' || 0 == strcmp(linkerName, builderLinker))
			linker->ProjectChanged(inProject, inChange);
	}
}
