//========================================================================
//	MFileSet.h
//	Copyright 1995 -97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MFILESET_H
#define _MFILESET_H

#include "IDEConstants.h"
#include "MList.h"
#include "CString.h"

class MSourceFile;

struct FileSetRec 
{
	short			length;	// length of entire record
	char			flags;
	char			path[1024];
};

#if 0
struct FileSetRecOld 
{
	record_ref		ref;
	char			flags;
	char			name[B_FILE_NAME_LENGTH_DR8+1];
};
#endif

struct FileSetHeaderOld 
{
	int32			RecordCount;
	int32			Version;
	char			filler1;
	char			name[B_FILE_NAME_LENGTH_DR8+1];
};

struct FileSetHeader 
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32			RecordCount;
	int32			Version;
	int32			Size;
	char			name[B_FILE_NAME_LENGTH_DR8+1];
};

struct MFileSet
{
	String			fName;
	int32			fCount;
	int32			fSize;
	FileSetRec*		fRecs;
};

typedef MList<MFileSet*> FileSetList;
//const int32 kFileSetVersion = 0;		// pre-dr9
const int32 kFileSetVersion = 1;

class MSetHolder
{
public:
								MSetHolder(
									void*	inSet = 0,
									int32	inSize = 0);
								~MSetHolder();
								
	void						SetSet(
									void*	inSet,
									int32	inSize);
	int32						Size()
								{
									return fSize;
								}
	void*						Set()
								{
									return fSet;
								}
private:

	void*		fSet;
	int32		fSize;
};

class MFileSetKeeper
{
public:
									MFileSetKeeper();
									~MFileSetKeeper();

	void							AddFileSet(
										const char* inName,
										bool inIsProjectList,
										const MList<MSourceFile*>&	inFileList);
	bool							RemoveFileSet(
										const char *	inName);
	const MFileSet*					GetFileSet(
										const char *	inName) const;
	void							GetProjectSets(
										MSetHolder&		outHolder);
	void							ReplaceProjectSets(
										FileSetRec*		inSets);
	static void						RemovePreferences();

	const MFileSet*					GetNthFileSet(int32 index,
												  bool isProjectList);

private:

	FileSetList			fGlobalSets;
	FileSetList			fProjectSets;

	void							ReadGlobalSets();
	void							WriteGlobalSets();
	void							ListToSetBlock(
										FileSetList&	inList,
										MSetHolder&		outHolder);

	void							SetBlockToList(
										void*			inSet,
										FileSetList&	inList);

	void							EmptyList(
										FileSetList&	inList) const;
};

const char fsIsSource = 0x00;
const char fsIsHeader = 0x01;
const char fsIsInSystem = 0x02;
const char fsSystemHeader = fsIsHeader | fsIsInSystem;
const char fsProjectHeader = fsIsHeader;
const char fsIsOther = 0x04;

inline bool
FSIsSource(const char& inFlag)
{
	return ((inFlag & fsIsHeader) == 0);
}
inline bool
FSIsHeader(const char& inFlag)
{
	return ((inFlag & fsIsHeader) != 0);
}
inline bool
FSIsProjectHeader(const char& inFlag)
{
	return ((inFlag & fsProjectHeader) != 0 && (inFlag & fsIsInSystem) == 0);
}
inline bool
FSIsSystemHeader(const char& inFlag)
{
	return ((inFlag & fsSystemHeader) == fsSystemHeader);
}
inline bool
FSIsOther(const char& inFlag)
{
	return ((inFlag & fsIsOther) != 0);
}
inline void
FSSetSource(char& inFlag)
{
	inFlag |= fsIsSource;
}
inline void
FSSetProjectHeader(char& inFlag)
{
	inFlag = ((inFlag & ~fsIsSource) & ~fsIsInSystem) | fsIsHeader;
}
inline void
FSSetSystemHeader(char& inFlag)
{
	inFlag = ((inFlag & ~fsIsSource) | fsSystemHeader);
}
inline void
FSSetOther(char& inFlag)
{
	inFlag |= fsIsOther;
}

#endif
