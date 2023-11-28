// Replace.h

#ifndef _REPLACE_H
#define _REPLACE_H


enum {
	R_ASK_USER = 0,
	R_NEVER_REPLACE = 1,
	R_RENAME = 2,
	R_REPLACE_VERSION_NEWER = 3,
	R_REPLACE_CREATION_NEWER = 4,
	R_REPLACE_MODIFICATION_NEWER = 5,
	R_MERGE_FOLDER = 6,
	R_INSTALL_IF_EXISTS = 7,

	R_ASK_VERSION_NEWER = 8,
	R_ASK_CREATION_NEWER = 9,
	R_ASK_MODIFICATION_NEWER = 10,
	
	R_END_REPLACE = 11
};

struct reploption{
	char	*name;
	int		code;
};

#ifdef PACKAGEBUILDER
extern struct reploption kReplaceOptions[];
bool IsCompatibleReplacementOption(uint16 version, uint32 replacement);
#endif // PACKAGEBUILDER

#endif // _REPLACE_H
