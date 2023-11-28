//****************************************************************************************
//
//	File:		Decode.cpp
//
//  Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef PREFS_H
#define PREFS_H

#include <Node.h>
#include <TypeConstants.h>

class Prefs {
	public:
		Prefs(const char *name);
		void Save();
		~Prefs();
		
		int32 quality, progressive;

	private:
		int32 LoadInt32(const char *name, int32 default_value);
		bool SaveInt32(const char *name, int32 value);

		BNode *node;
};

#endif
