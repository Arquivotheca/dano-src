// --------------------------------------------------------------------------- 
/* 
	Environment Block handling class
	 
	Copyright (c) 2001 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			March 22, 2001 
 
	Modify a copy of the environment block.
*/ 
// --------------------------------------------------------------------------- 
#include "MEnviron.h"

MEnviron::MEnviron()
		 :fEnvBlock(NULL)
{
	// This is as good a time as any to set up the environ map.
	
	for( int i = 0; environ[i] != NULL; i++ )
	{
		fEnvMap[LeftSide(environ[i])] = RightSide(environ[i]);
	}
}

// --------------------------------------------------------------------------- 

MEnviron::~MEnviron()
{
	delete[] fEnvBlock;
}

// --------------------------------------------------------------------------- 

void MEnviron::SetVar(const char *Name, const char *Value)
{
	// This forces the EnvBlock to be re-evaluated in the
	// const char** operator, perhaps not the most efficient
	// thing, but certianly the most safe. If performance
	// becomes an issue, this can be changed with a bit of
	// work.
	delete[] fEnvBlock;
	fEnvBlock = NULL;
	
	fEnvMap[BString(Name)] = BString(Value);
}

// --------------------------------------------------------------------------- 

const char* MEnviron::GetVar(const char *Name)
{
	return fEnvMap[Name].String();
}

// --------------------------------------------------------------------------- 
const char** MEnviron::EnvBlock()
{
	if( !fEnvBlock )
	{
		fEnvStrings.clear();
	
		// the extra is for the NULL at the end.
		uint32 size = fEnvMap.size();
		fEnvBlock = new const char*[size + 1];
		fEnvBlock[size] = NULL;
		
		// EnvBlock Index
		uint32 bi = 0;
		
		// WOW! The comma operator in a 'for' statement, how often do you get a
		// chance to do that?
		for( std::map<const BString, BString>::const_iterator i = fEnvMap.begin();
		     i != fEnvMap.end(), bi < size;
		     i++, bi++
		   )
		{
			BString line((*i).first);
			line << "=";
			line << (*i).second;
			
			fEnvStrings.push_back(line);
			fEnvBlock[bi] = fEnvStrings[bi].String();
		}
	}
	
	return fEnvBlock;
}

// --------------------------------------------------------------------------- 

BString MEnviron::LeftSide(const char *Value)
{
	BString result;
	
	while( (*Value != '=') && (*Value != '\0') )
	{
		result += *Value;
		Value++;
	}
	
	return result;
}

// --------------------------------------------------------------------------- 

BString MEnviron::RightSide(const char *Value)
{
	BString result;
	
	while( (*Value != '=') && (*Value != '\0') )
	{
		Value++;
	}

	if( '=' == *Value )
	{
		Value++;
		while( *Value != '\0' )
		{
			result += *Value;
			Value++;
		}
	}
	
	return result;
}

// --------------------------------------------------------------------------- 

