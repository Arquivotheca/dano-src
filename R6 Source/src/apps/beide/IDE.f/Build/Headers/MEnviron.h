// --------------------------------------------------------------------------- 
/* 
	Environment Block handling class
	 
	Copyright (c) 2001 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			March 22, 2001 
 
	Modify a copy of the environment block.
*/ 
// --------------------------------------------------------------------------- 
#ifndef MENVIRON_H
#define MENVIRON_H

#include <String.h>
#include <map>
#include <vector>

class MEnviron
{
	public:
						MEnviron();
						~MEnviron();
		
		void			SetVar(const char* Name, const char* Value);
		const char*		GetVar(const char* Name);
		
		const char**	EnvBlock();
		
	protected:
		BString LeftSide(const char* Value);
		BString RightSide(const char* Value);

	private:
		const char** 						fEnvBlock;
		std::map<const BString, BString>	fEnvMap;
		std::vector<BString>				fEnvStrings;
};

#endif

