//========================================================================
//	MLibraryFileLine.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================

#ifndef _MLIBRARYFILELINE_H
#define _MLIBRARYFILELINE_H

#include "MSourceFileLine.h"

class MProjectView;
class MBlockFile;

// This struct is taken from Obj.h
// it was modified slightly because it wasn't a valid C++ struct
typedef struct LibHeader {
	long			magicword;			/*	always set to LIB_MAGIC_WORD	*/
	long			magicproc;			/*	'PPC '	*/
	long			magicflags;			/*	(reserved for future use)	*/
	long			version;			/*	always set to LIB_VERSION	*/
	long			code_size;			/*	total size of code in library	*/
	long			data_size;			/*	total size of data in library	*/
	long			nobjectfiles;		/*	# of contained object files	*/
//	LibFile			objectfiles[];		/*	index to contained object files	*/
} LibHeader;
#define	LIB_MAGIC_WORD	'MWOB'
#define LIB_VERSION		1


class MLibraryFileLine : public MSourceFileLine
{
public:
								MLibraryFileLine(
									const BEntry&		inFile, 
									bool 				inInSystemTree,
									MSectionLine& 		inSection,
									MProjectView& 		inProjectView,
									ProjectTargetRec*	inRec,
									const char *		inName);

								MLibraryFileLine(
									MSectionLine& 	inSection,
									MProjectView& 	inProjectView,
									MBlockFile&		inFile,
									BlockType		inBlockType,
									uint32			inProjectVersion);

	virtual						~MLibraryFileLine();
					
	virtual	void				Draw(
									BRect inFrame, 
									BRect inIntersection, 
									MProjectView& inView);
		
	virtual bool				DoClick(
									BRect 			inFrame, 
									BPoint 			inPoint,
									bool 			inIsSelected,
									uint32			inModifiers,
									uint32			inButtons);
	virtual void				Invoke();

	virtual MCompile*			BuildCompileObj(
									MakeActionT		inKind);

	virtual bool				NeedsToBeCompiled();

	virtual void				DeleteObjectFile();

private:

	void						GetCodeDataSize();
};

#endif
