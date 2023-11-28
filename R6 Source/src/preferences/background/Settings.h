//*****************************************************************************
//
//	File:		 Settings.h
//
//	Description: Setting loader header for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#if ! defined SETTING_INCLUDED
#define SETTING_INCLUDED

#include <Path.h>
#include <Entry.h>
#include <Point.h>
#include <GraphicsDefs.h>

class BWindow;

class Settings
{
	struct settings_t
	{
		BPath		imagepath;
		int32		imagemode;
		BPoint		imageoffs;
		bool		erasetext;

		settings_t()
		{
		}
		settings_t(const settings_t &copy)
		{
			imagepath = copy.imagepath;
			imagemode = copy.imagemode;
			imageoffs = copy.imageoffs;
			erasetext = copy.erasetext;
		}
		bool operator==(const settings_t &other) const
		{
			return ((imagepath.Path() == 0 && other.imagepath.Path() == 0) ||
				imagepath == other.imagepath) &&
				imagemode == other.imagemode &&
				imageoffs == other.imageoffs &&
				erasetext == other.erasetext;
		}
		bool operator!=(const settings_t &other) const
		{
			return !(*this == other);
		}
		settings_t & operator=(const settings_t &copy)
		{
			imagepath = copy.imagepath;
			imagemode = copy.imagemode;
			imageoffs = copy.imageoffs;
			erasetext = copy.erasetext;
			return *this;
		}
	};

	settings_t	saved[32];
	settings_t	current;
	rgb_color	currentcolor;

	enum settings_mode
	{
		MODE_NOTSET,
		MODE_CURRWORKSPACE,
		MODE_ALLWORKSPACES,
		MODE_DEFAULTFOLDER,
		MODE_FOLDER
	} mode;

	BPath		current_folder;

	status_t	Load();
	int32		Which();

public:
				Settings();

	status_t	ApplyToAllWorkspaces();
	status_t	ApplyToCurrentWorkspace();
	status_t	ApplyToDefaultFolder();
	status_t	ApplyToFolder(BPath *folder);
	bool		IsApplyToFolder();

	// save options
	status_t	Apply();
	void		Revert();
	bool		Changes();

	// changes
	void		SetImagePath(BPath path);
	void		SetImageMode(int32 mode);
	void		SetImageOffset(BPoint offs);
	void		SetImageEraseText(bool erase);
	void		SetColor(rgb_color col);
	BPath		GetImagePath();
	int32		GetImageMode();
	BPoint		GetImageOffset();
	bool		GetImageEraseText();
	rgb_color	GetColor();

	bool		GetNextPath(BPath *p, uint32 *cookie);
};

#endif
