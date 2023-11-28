// ---------------------------------------------------------------------------
/*
	MSubProjectFileLine.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			12 July 1999

*/
// ---------------------------------------------------------------------------


#ifndef _MSUBPROJECTFILELINE_H
#define _MSUBPROJECTFILELINE_H

#include "MSourceFileLine.h"
#include "MList.h"

class MSubProjectFileLine : public MSourceFileLine
{
public:
								MSubProjectFileLine(const BEntry& inFile, 
													bool inInSystemTree,
													MSectionLine& inSection,
													MProjectView& inProjectView,
													ProjectTargetRec* inRec,
													const char* inName);
								MSubProjectFileLine(MSectionLine& inSection,
													MProjectView& inProjectView,
													MBlockFile& inBlockFile,
													BlockType inBlockType,
													uint32 inProjectVersion);

	virtual						~MSubProjectFileLine();

	virtual void				Draw(BRect inFrame, 
									 BRect inIntersection,
									 MProjectView& inView);

	virtual bool				DoClick(BRect inFrame, 
										BPoint inPoint,
										bool inIsSelected,
										uint32 inModifiers,
										uint32 inButtons);

	virtual bool				NeedsToBeCompiled();
	virtual bool				NeedsToBePreCompiled();

	virtual MCompile*			BuildCompileObj(MakeActionT		inKind);
	virtual void				DeleteObjectFile();
	virtual void				RemoveObjects(bool inRemoveAll);

	virtual void				CompileDone(status_t errCode,
											int32 inCodeSize,
											int32 inDataSize,
											MSourceFileList* inList);

protected:
	void						InitLine();
};

#endif
