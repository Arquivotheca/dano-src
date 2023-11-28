//****************************************************************************************
//
//	File:		Prefs.h
//
//	Written by:	Daniel Switkin
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
		
		int32 interlacing, res_x, res_y, res_units,
			offset_x, offset_y, offset_units;

	private:
		int32 LoadInt32(const char *name, int32 default_value);
		bool SaveInt32(const char *name, int32 value);

		BNode *node;
};

#endif
