//========================================================================
//	MMenuTask.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MMenuTask.h"
#include "MKeyBindingManager.h"
#include "MIDECommandList.h"
#include "ProjectCommands.h"

#if 0
struct CommandMenuGroup
{
	int32		count;
	CommandT	cmd[2];
};

CommandMenuGroup		sCommandGroups[] = {
	// File Menu
	{ 2, { CMD_SAVE, CMD_SAVE_ALL } },
	{ 2, { CMD_CLOSE, CMD_CLOSE_ALL } },

	// Search Menu
	{ 2, { CMD_FIND_NEXT, CMD_FIND_PREVIOUS } },
	{ 2, { CMD_FIND_NEXT_FILE, CMD_FIND_PREVIOUS_FILE } },
	{ 2, { CMD_ENTER_FIND_STRING, CMD_ENTER_REPLACE_STRING } },
	{ 2, { CMD_FIND_SELECTION, CMD_FIND_PREVIOUS_SELECTION } },
	{ 2, { CMD_REPLACE_AND_FIND, CMD_REPLACE_AND_FIND_PREVIOUS } },

	// Project Menu
	{ 2, { CMD_REMOVE_OBJECTS, CMD_REMOVE_OBJECTS_COMPACT } },
};
const int32 kCommandGroupCount = sizeof(sCommandGroups) / sizeof(CommandMenuGroup);
#endif

// ---------------------------------------------------------------------------
//		¥ MMenuTask
// ---------------------------------------------------------------------------
//	Constructor

MMenuTask::MMenuTask(
	BMenuBar&	inMenuBar)
	: MThread("menutask"),
	fMenuBar(inMenuBar), fModifiers(0)
{
}

// ---------------------------------------------------------------------------
//		¥ ~MMenuTask
// ---------------------------------------------------------------------------
//	Destructor

MMenuTask::~MMenuTask()
{
}

// ---------------------------------------------------------------------------
//		¥ LastCall
// ---------------------------------------------------------------------------
//	Don't delete this.

void
MMenuTask::LastCall()
{
}

// ---------------------------------------------------------------------------
//		¥ Execute
// ---------------------------------------------------------------------------
//	Update the menu items in a pulled-down menu to match the modifier
//	keys that the user is holding down.

status_t
MMenuTask::Execute()
{
	while (! Cancelled())
	{
		// Did the user change the modifier keys?
		uint32		modKeys = modifiers() /*| B_COMMAND_KEY & 0x000000FF*/;	// ignore right and left keys
		if (modKeys != fModifiers)
		{
			fModifiers = modKeys;
			modKeys |= B_COMMAND_KEY;		// everything uses the command key
			modKeys &= 0x000000FF;			// ignore right and left keys
			ASSERT(modKeys < 256);

//			MLocker<BLooper>	lock(fMenuBar.Looper());
			
			// Look through all the dynamic menu items for any
			// that should be changed
			for (int i = 0; i < kCommandGroupCount; ++i)
			{
				KeyBinding		binding;
				
				for (int j = 0; j < sCommandGroups[i].count; ++j)
				{
					CommandT		command = sCommandGroups[i].cmd[j];

					if (MKeyBindingManager::Manager().GetBinding(command, binding) &&
						modKeys == binding.modifiers)
					{
//TRACE();
						BMenuItem*	item = nil;
						
						for (int k = 0; k < sCommandGroups[i].count && item == nil; ++k)
						{
							CommandT		aCommand = sCommandGroups[i].cmd[k];
							item = fMenuBar.FindItem(aCommand);
//printf("command = %d, item = %p\n", aCommand, item);
						}

						// item may be nil because that menu item isn't in this
						// window's menus (eg, project window specific items in a text window
						if (item != nil)
						{
							item->SetMessage(new BMessage(command));
							item->SetShortcut(binding.keyCode, binding.modifiers);
							CommandInfo	info;
							bool		found = MIDECommandList::GetCommandInfo(command, info);
							if (found)
								item->SetLabel(info.name);
							ASSERT(found);
//printf("changed binding = %d\n", command);
						}
						else
							printf("nil binding = %d\n", command);
//printf("modKeys = %p, binding.modifiers = %p\n", modKeys, (int) binding.modifiers);
					}
				}
			}
		}
		
		snooze(30000);
	}

	return B_NO_ERROR;
}

