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
	uint32		mask;
	const char	*imgpath;
	int32		imgmode;
	BPoint		offs;
	bool		erase;

	switch(mode)
	{
		case MODE_NOTSET :
			break;

		case MODE_CURRWORKSPACE :
		case MODE_ALLWORKSPACES :
			ret = find_directory(B_DESKTOP_DIRECTORY, &path);
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

			case MODE_CURRWORKSPACE :
			case MODE_ALLWORKSPACES :
				// extract settings
				for(int32 i = 0; msg.FindInt32(kBackgroundImageInfoWorkspaces, i, (int32 *)&mask) == B_OK; i++)
				{
					msg.FindString(kBackgroundImageInfoPath, i, &imgpath);
					msg.FindInt32(kBackgroundImageInfoMode, i, &imgmode);
					msg.FindPoint(kBackgroundImageInfoOffset, i, &offs);
					msg.FindBool(kBackgroundImageInfoEraseText, i, &erase);
	
					for(int32 j = 0; j < 32; j++)
						if(mask & (1L << j))
						{
							saved[j].imagepath = imgpath;
							saved[j].imagemode = imgmode;
							saved[j].imageoffs = offs;
							saved[j].erasetext = erase;
						}
				}
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

void Settings::Revert()
{
	// copy saved settings over current settings
	switch(mode)
	{
		case MODE_NOTSET :
			break;

		case MODE_ALLWORKSPACES :
		case MODE_CURRWORKSPACE :
			current = saved[current_workspace()];
			currentcolor =  BScreen(B_MAIN_SCREEN_ID).DesktopColor(current_workspace());
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

		case MODE_ALLWORKSPACES :
			{
				imagechanged = false;
				for(int32 i = 0; i < 32; i++)
					if(saved[i] != current)
					{
						imagechanged = true;
						break;
					}

				// copy current settings over saved settings
				for(int32 i = 0; i < 32; i++)
					saved[i] = current;

				// add to message
				if(current.imagepath.Path() != 0 && strlen(current.imagepath.Path()) > 0)
				{
					msg.AddInt32(kBackgroundImageInfoWorkspaces, 0xffffffffL);
					msg.AddString(kBackgroundImageInfoPath, current.imagepath.Path());
					msg.AddInt32(kBackgroundImageInfoMode, current.imagemode);
					msg.AddPoint(kBackgroundImageInfoOffset, current.imageoffs);
					msg.AddBool(kBackgroundImageInfoEraseText, current.erasetext);
				}

				// apply color to all workspaces
				BScreen	s(B_MAIN_SCREEN_ID);
				for(uint32 index = 0; index < 32 ; index++)
					s.SetDesktopColor(currentcolor, index);
				ret = find_directory(B_DESKTOP_DIRECTORY, &path);
			}
			break;

		case MODE_CURRWORKSPACE :
			{
				uint32	savemask = 0;
				uint32	mask;
				uint32	ws = current_workspace();

				imagechanged = current != saved[ws];

				// apply settings
				saved[ws] = current;
		
				// build message
				for(uint32 i = 0; i < 32; i++)
				{
					// skip saved settings
					if(savemask & (1L << i))
						continue;

					// skip empty settings
					if(saved[i].imagepath.Path() == 0 || strlen(saved[i].imagepath.Path()) == 0)
						continue;

					// scan for workspace with the same settings
					mask = 1L << i;
					for(int32 j = i + 1; j < 32; j++)
					{
						if(savemask & (1L << j))
							continue;

						if(saved[i].imagepath == saved[j].imagepath &&
							saved[i].imagemode == saved[j].imagemode &&
							saved[i].imageoffs == saved[j].imageoffs &&
							saved[i].erasetext == saved[j].erasetext)
						{
							mask |= 1L << j;
							savemask |= 1L << j;
						}
					}

					// add to message
					msg.AddInt32(kBackgroundImageInfoWorkspaces, mask);
					msg.AddString(kBackgroundImageInfoPath, saved[i].imagepath.Path());
					msg.AddInt32(kBackgroundImageInfoMode, saved[i].imagemode);
					msg.AddPoint(kBackgroundImageInfoOffset, saved[i].imageoffs);
					msg.AddBool(kBackgroundImageInfoEraseText, saved[i].erasetext);
				}

				// apply color
				BScreen(B_MAIN_SCREEN_ID).SetDesktopColor(currentcolor);
			}
			ret = find_directory(B_DESKTOP_DIRECTORY, &path);
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

			case MODE_ALLWORKSPACES :
			case MODE_CURRWORKSPACE :
				tracker.SendMessage('Tbgr');
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

status_t Settings::ApplyToAllWorkspaces()
{
	if(mode == MODE_CURRWORKSPACE || mode == MODE_ALLWORKSPACES)
	{
		mode = MODE_ALLWORKSPACES;
		return B_OK;
	}
	else
	{
		mode = MODE_ALLWORKSPACES;
		return Load();
	}
}

status_t Settings::ApplyToCurrentWorkspace()
{
	if(mode == MODE_CURRWORKSPACE || mode == MODE_ALLWORKSPACES)
	{
		mode = MODE_CURRWORKSPACE;
		return B_OK;
	}
	else
	{
		mode = MODE_CURRWORKSPACE;
		return Load();
	}
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
	rgb_color	wscolor;

	switch(mode)
	{
		case MODE_NOTSET :
			break;

		case MODE_ALLWORKSPACES :
			for(int32 i = 0; i < 32; i++)
			{
				// compare all settings with current workspace if
				// settings should be applied to all workspaces
				wscolor = BScreen(B_MAIN_SCREEN_ID).DesktopColor(i);
				if(currentcolor.red != wscolor.red ||
					currentcolor.green != wscolor.green ||
					currentcolor.blue != wscolor.blue ||
					saved[i] != current)
				{
					changed = true;
					break;
				}
			}
			break;

		case MODE_CURRWORKSPACE :
			wscolor = BScreen(B_MAIN_SCREEN_ID).DesktopColor();
			changed = currentcolor.red != wscolor.red ||
					currentcolor.green != wscolor.green ||
					currentcolor.blue != wscolor.blue ||
					saved[current_workspace()] != current;
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

void Settings::SetColor(rgb_color col)
{
	currentcolor = col;
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

rgb_color Settings::GetColor()
{
	const rgb_color	white = { 255, 255, 255, 255 };
	return (mode == MODE_CURRWORKSPACE || mode == MODE_ALLWORKSPACES) ? currentcolor : white;
}

bool Settings::GetNextPath(BPath *p, uint32 *cookie)
{
	bool	found = false;

	if(*cookie == 0xffffffffL)
		return false;

	if(mode == MODE_CURRWORKSPACE || mode == MODE_ALLWORKSPACES)
	{
		for(int32 i = 0; i < 32; i++)
		{
			if(! ((*cookie) & (1L << i)))
			{
				settings_t	*seti = (i == current_workspace()) ? &current : &saved[i];
				*p = seti->imagepath;
				*cookie |= 1L << i;

				if(seti->imagepath.Path() == 0 || strlen(seti->imagepath.Path()) == 0)
					continue;

				for(int32 j = i + 1; j < 32; j++)
				{
					settings_t	*setj = (j == current_workspace()) ? &current : &saved[j];
					if(seti->imagepath == setj->imagepath)
						*cookie |= 1L << j;
				}

				found = true;
				break;
			}
		}
	}
	else
	{
		if((found = (current.imagepath.Path() != 0 && strlen(current.imagepath.Path()) != 0)) == true)
			*p = current.imagepath;
		*cookie = 0xffffffffL;
	}

	return found;
}
