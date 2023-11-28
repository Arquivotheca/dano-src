// =======================================================================
//  FileSystemHTML.h
//  Copyright 1998 by Be Incorporated.
// =======================================================================


//	HTML display of file system trees (ftp:// or file://)

#ifndef __FILESYSTEMHTML__
#define __FILESYSTEMHTML__

#include "URL.h"
#include "Utils.h"

#include <String.h>

class UResourceImp;

//=======================================================================
//	Class that turns the ftp output into a nice HTML document

class FileSystemHTML {
public:
					FileSystemHTML(UResourceImp* resImp);
virtual				~FileSystemHTML();
			
		void		WriteHeader();
		void		AddFile(char fType, long size, long date, const char* name);
		void		WriteTrailer();
	
protected:
		void		HTML(const char* html, bool cr = true);
		void		WriteFile(char fType, long size, long date, const char* name);
	
	UResourceImp*		mResImp;
	int					mCount;
	
	CArray<BString,64>	mName;
	CArray<char,64>		mType;
	CArray<long,64>		mDate;
	CArray<long,64>		mSize;
};

#endif
