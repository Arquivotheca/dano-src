//========================================================================
//	MFileUtils.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFILEUTILS_H
#define _MFILEUTILS_H

#include "MList.h"
#include "MAccessPathsView.h"
#include "CString.h"
#include <dirent.h>
#include <Entry.h>

struct DirectoryInfo
{
	entry_ref		dDir;
	bool			dRecursiveSearch;
};

typedef MList<DirectoryInfo*> DirectoryList;
typedef MList<node_ref*> NodeList;

// Constants for location of a file in the project's access paths
enum FileLocationT
{
	kFileNotFound,
	kFileInRecursiveDir,
	kFileInNonRecursiveDir
};

enum FileWriteAble
{
	kIsWritable,			// normal writable file
	kPermissionLocked,		// file is locked by unix file permissions
	kReadOnly,				// file is on a read-only volume
	kUnsavedFile			// document is unsaved
};

struct myDirent : public dirent
{
	char		data[B_FILE_NAME_LENGTH];
};

class MDirIteratorFunc
{
public:
	
									MDirIteratorFunc() {}
	virtual							~MDirIteratorFunc() {}
	
	// return true to stop the iteration
	virtual bool					ProcessItem(
										BEntry& 			inEntry,
										node_flavor			inFlavor,
										const BDirectory&	inDir) = 0;
};

status_t						IterateDirectory(
									BDirectory& 		inDir,
									MDirIteratorFunc&	inFunc,
									uint32				inNodeFlavors = B_FILE_NODE | B_DIRECTORY_NODE,
									bool				inTraverseLinks = false);

status_t 						FindDirectory(
									BDirectory&		inDir,
									const char *	inPath,
									BDirectory*		inoutDirectory);
status_t 						FindFile(
									BDirectory&		inDir,
									const char *	inPath,
									BEntry*			inEntry);
status_t 						CopyDirectory(
									BDirectory&	inFrom,
									BDirectory&	inTo);
status_t						CopyFile(
									BFile&	inFrom,
									BFile&	inTo);
status_t 						CopyAttrs(
									BNode&			inFrom,
									BNode&			inTo);
void							FixUpSystemPaths(
									AccessPathsPrefs&	inPathsPrefs,
									AccessPathList&		inPathList);
bool							NotFound(
									const AccessPathData*	inPath,
									const AccessPathList&	inPathList);

FileWriteAble 					FileWriteableState(
									BStatable&	inFile);

inline bool 					FileIsWriteable(
									FileWriteAble	inWritable)
								{
									return inWritable == kIsWritable;
								}
								

extern const AccessPathData		accessData[];

class MFileUtils
{
	public:
								MFileUtils();
	static void					SetCompilerPaths();

	static bool					FindFileInDirectoryList(
									const char* 			inFileName,
									const DirectoryList&	inList,
									entry_ref&				outRef);

	static bool					FindFileInDirectory(
									const char* 		inFileName,
									BDirectory& 		inDirectory,
									BEntry&				outEntry);

	static bool					FileIsInDirectoriesList(
									DirectoryList&		inDirectoryList,
									const BEntry&		inFile,
									DirectoryInfo*&		outDirectory);

	static bool					IsDirectory(
									const entry_ref&	inRef);

	static bool					DirectoryFromFullPath(
									const char*		inPathName,
									entry_ref&		outRef);

	static bool		 			DirectoryFromRelPath(
									const char*		inPathName,
									BDirectory&		inFromDir,
									entry_ref&		outRef);

	static bool					BuildRelativePath(
									BDirectory&		inFromDir,
									BDirectory&		inToDir,
									char*			outPath);

	static void					BuildDirectoriesList(
									const AccessPathList&	inPathsList,
									DirectoryList&			inDirectoryList,
									BDirectory*				inProjectDirectory);
	static void					EmptyDirectoryList(
									DirectoryList&				inDirectoryList,
									const BDirectory * const	inProjectDirectory);

	static void					AddAccessPathsToMessage(
									BMessage&	inOutMessage,
									const AccessPathList&		inSystemPathList,
									const AccessPathList&		inProjectPathList);
	static void					GetAccessPathsFromMessage(
									const BMessage&		inOutMessage,
									AccessPathList&		inSystemPathList,
									AccessPathList&		inProjectPathList,
									int32				inSystemPathCount,
									int32				inProjectPathCount);

	static 	int32				GetTempFile(
									BEntry&			inOutFile,
									const BEntry*	inReferenceFile = NULL,
									bool			inCreate = true);
	static void					KillTempFile(
									BEntry&	inFile);

	static bool					Parenthesized(
									const BDirectory&		inDir);
	static bool					Parenthesized(
									const BEntry&			inEntry);
	static bool					Parenthesized(
									const char *	inName);

	static bool					AlreadyScanned(
									const BDirectory&		inDir,
									NodeList&				inNodeList);
	static void					EmptyNodeList(
									NodeList&		inNodeList);

	static BDirectory&			SystemDirectory()
								{
									return sSystemDirectory;
								}
	static BDirectory&			ToolsDirectory()
								{
									return sToolsDirectory;
								}
	static BDirectory&			BinDirectory()
								{
									return sBinDirectory;
								}
	static const char *			BinPath()
								{
									return sBinPath;
								}
	static const char *			BootTmp()
								{
									return sTmpPath;
								}
	static dev_t				BootDev()
								{
									return sBootDevice;
								}


private:
		
	static BDirectory		sSystemDirectory;
	static BDirectory		sToolsDirectory;
	static BDirectory		sBinDirectory;
	static String			sBinPath;
	static String			sTmpPath;
	static dev_t			sBootDevice;

	static bool					FindFileInDirectory(
									const char* 		inFileName,
									BDirectory& 		inDirectory,
									BEntry&				outEntry,
									NodeList&			inNodeList,
									bool&				inoutSymLinkFound);
};

#endif
