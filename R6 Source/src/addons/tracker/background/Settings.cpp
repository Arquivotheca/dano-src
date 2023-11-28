//*****************************************************************************
//
//	File:		 Settings.cpp
//
//	Description: Setting loader class for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "Settings.h"
#include "TrackerDefs.h"
#include <Path.h>
#include <Node.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Window.h>
#include <Screen.h>
#include <Control.h>
#include <Beep.h>
#include <fs_attr.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Settings::Settings()
 : mode(MODE_NOTSET)
{
}

status_t Settings::Load()
{
	status_t	ret = B_OK;
	BPath		path;
	attr_info	info;
	ssize_t		s;
	void		*buf;
	BMessage	msg;
	const char	*imgpath;

	switch(mode)
	{
		case MODE_NOTSET :
			break;

		case MODE_FOLDER :
		case MODE_DEFAULTFOLDER :
			path = current_folder;
			break;
	}

	if(ret == B_OK)
	{
		BNode	n(path.Path());

		if((ret = n.InitCheck()) == B_OK)
		{
			if((ret = n.GetAttrInfo(kBackgroundImageInfo, &info)) == B_OK)
			{
				if(info.size)	// avoid problems with zero size malloc, ReadAttr, etc.
				{
					if((buf = malloc(info.size)) != 0)
					{
						if((s = n.ReadAttr(kBackgroundImageInfo, B_MESSAGE_TYPE, 0, buf, info.size)) == info.size)
							ret = msg.Unflatten((char *)buf);
						free(buf);
					}
					else
						ret = B_NO_MEMORY;
				}
			}
			else
				// no entry found, fall through ok
				if(ret == B_ENTRY_NOT_FOUND)
					ret = B_OK;
		}
	}

	if(ret == B_OK)
	{
		// clean settings
		for(uint32 i = 0; i < 32; i++)
		{
			saved[i].imagepath = BPath();
			saved[i].imagemode = 0;
			saved[i].imageoffs = BPoint(0, 0);
			saved[i].erasetext = true;
		}

		switch(mode)
		{
			case MODE_NOTSET :
				break;

			case MODE_FOLDER :
			case MODE_DEFAULTFOLDER :
				if(msg.FindString(kBackgroundImageInfoPath, &imgpath) == B_OK)
				{
					saved[0].imagepath = imgpath;
					msg.FindInt32(kBackgroundImageInfoMode, &saved[0].imagemode);
					msg.FindPoint(kBackgroundImageInfoOffset, &saved[0].imageoffs);
					msg.FindBool(kBackgroundImageInfoEraseText, &saved[0].erasetext);
				}
				break;
		}
		Revert();
	}

	return ret;
}

void Settings::Default()
{
	current.imagepath = BPath();
	current.imagemode = 0;
	current.imageoffs = BPoint(0, 0);
	current.erasetext = true;
}

void Settings::Revert()
{
	// copy saved settings over current settings
	switch(mode)
	{
		case MODE_NOTSET :
			break;

		case MODE_FOLDER :
		case MODE_DEFAULTFOLDER :
			current = saved[0];
			break;
	}
}

status_t Settings::Apply()
{
	status_t	ret = B_OK;
	BPath		path;
	ssize_t		s;
	void		*buf;
	BMessage	msg;
	bool		imagechanged = true;

	switch(mode)
	{
		case MODE_NOTSET :
			break;

		case MODE_FOLDER :
		case MODE_DEFAULTFOLDER :
			imagechanged = current != saved[0];

			// copy current settings over saved settings
			saved[0] = current;

			// add to message
			if(current.imagepath.Path() != 0 && strlen(current.imagepath.Path()) > 0)
			{
				msg.AddInt32(kBackgroundImageInfoWorkspaces, 0xffffffffL);
				msg.AddString(kBackgroundImageInfoPath, current.imagepath.Path());
				msg.AddInt32(kBackgroundImageInfoMode, current.imagemode);
				msg.AddPoint(kBackgroundImageInfoOffset, current.imageoffs);
				msg.AddBool(kBackgroundImageInfoEraseText, current.erasetext);
			}

			path = current_folder;
			break;
	}

	if(ret == B_OK)
	{
		BNode	n(path.Path());

		if((ret = n.InitCheck()) == B_OK)
		{
			n.RemoveAttr(kBackgroundImageInfo);

			s = msg.FlattenedSize();
			if((buf = malloc(s)) != 0)
			{
				if((ret = msg.Flatten((char *)buf, s)) == B_OK)
				{
					ssize_t written;
					if((written = n.WriteAttr(kBackgroundImageInfo, B_MESSAGE_TYPE, 0, buf, s)) < 0)
						ret = (status_t)written;
				}
				free(buf);
			}
			else
				ret = B_NO_MEMORY;
		}
	}

	if(ret == B_OK && imagechanged)
	{
		// force settings
		BMessenger tracker("application/x-vnd.Be-TRAK");
		switch(mode)
		{
			case MODE_NOTSET :
				break;

			case MODE_FOLDER :
			case MODE_DEFAULTFOLDER :
				// send 'Tbgr' to all open windows
				// TODO: unfortunately it currently doesn't exclude the desktop
				{
				int32 i = 0;
				BMessage reply;
				int32 err;
				do
				{
					BMessage msg(B_GET_PROPERTY);
					msg.AddSpecifier("Poses");
					msg.AddSpecifier("Window", i++);
					reply.MakeEmpty();
					tracker.SendMessage(&msg, &reply);

					// don't stop for windows that don't
					// understand a request for 'Poses'
					if(reply.what == B_MESSAGE_NOT_UNDERSTOOD &&
						reply.FindInt32("error", &err) == B_OK &&
						err != B_BAD_SCRIPT_SYNTAX)
						break;

					BMessenger m;
					if(reply.FindMessenger("result", &m) == B_OK)
					{
						bool refresh = false;

						if(mode == MODE_FOLDER)
						{
							// we have the messenger for the Tracker window
							// Poses, now check if it has a path and if it's
							// what we're looking for...
							BMessage	askpath(B_GET_PROPERTY);
							BMessage	answer;
							entry_ref	ref;

							askpath.AddSpecifier("Path");
							m.SendMessage(&askpath, &answer);
							if(answer.what != B_MESSAGE_NOT_UNDERSTOOD &&
								answer.FindRef("result", &ref) == B_OK)
							{
								BEntry	e(&ref);
								BPath	p;
								e.GetPath(&p);
								refresh = strcmp(p.Path(), current_folder.Path()) == 0;
							}
						}
						else
							refresh = true;

						if(refresh)
							m.SendMessage('Tbgr');
					}
				}
				while(1);
				}
				break;
		}
	}

	return ret;
}

status_t Settings::ApplyToDefaultFolder()
{
	status_t	ret = B_OK;
	BEntry		e;
	BPath		path;

	if(mode != MODE_DEFAULTFOLDER)
	{
		mode = MODE_DEFAULTFOLDER;
		if((ret = find_directory(B_USER_SETTINGS_DIRECTORY, &current_folder)) == B_OK &&
			(ret = current_folder.Append("Tracker/DefaultFolderTemplate")) == B_OK)
			ret = Load();
	}

	return ret;
}

status_t Settings::ApplyToFolder(BPath *folder)
{
	current_folder = *folder;
	mode = MODE_FOLDER;
	return Load();
}

bool Settings::IsApplyToFolder()
{
	return mode == MODE_FOLDER;
}

bool Settings::Changes()
{
	bool		changed = false;

	switch(mode)
	{
		case MODE_NOTSET :
			break;

		case MODE_FOLDER :
		case MODE_DEFAULTFOLDER :
			changed = saved[0] != current;
			break;
	}

	return changed;
}

void Settings::SetImagePath(BPath path)
{
	current.imagepath = path;
}

void Settings::SetImageMode(int32 mode)
{
	current.imagemode = mode;
}

void Settings::SetImageOffset(BPoint offs)
{
	current.imageoffs = offs;
}

void Settings::SetImageEraseText(bool erase)
{
	current.erasetext = erase;
}

BPath Settings::GetImagePath()
{
	return current.imagepath;
}

int32 Settings::GetImageMode()
{
	return current.imagemode;
}

BPoint Settings::GetImageOffset()
{
	return current.imageoffs;
}

bool Settings::GetImageEraseText()
{
	return current.erasetext;
}

bool Settings::GetNextPath(BPath *p, uint32 *cookie)
{
	bool	found = false;

	if(*cookie == 0xffffffffL)
		return false;

	if((found = (current.imagepath.Path() != 0 && strlen(current.imagepath.Path()) != 0)) == true)
		*p = current.imagepath;
	*cookie = 0xffffffffL;

	return found;
}
