#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dt_utils.h"

float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

// **************************************************************************** //

void
_DirectoryNameFromPath(char *path, char *newpath)
{
	char *slash;
	int length;                   	/* Length of result, not including NUL.  */
	
	slash = strrchr (path, '/');
	if (slash == 0) {				/* File is in the current directory.  */
		path = ".";
		length = 1;
	} else {						/* Remove any trailing slashes from the result.  */
		while (slash > path && *slash == '/')
			--slash;
	
		length = slash - path + 1;
	}
	
	strncpy (newpath, path, length);
	newpath[length] = 0;
}

void
replace_underscores(char *s)
{
	int i;
	for (i = 0; i < NAME_MAX  &&  s[0] != 0; i++) {
		if (s[i] == '_')
			s[i] = ' ';
	}
}
