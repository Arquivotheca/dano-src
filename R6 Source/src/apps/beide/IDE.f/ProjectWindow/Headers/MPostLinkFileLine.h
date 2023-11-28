//========================================================================
//	MPostLinkFileLine.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MPOSTLINKFILELINE_H
#define _MPOSTLINKFILELINE_H

#include "MSourceFileLine.h"


class MPostLinkFileLine : public MSourceFileLine
{
public:
								MPostLinkFileLine(
									const BEntry&		inFile, 
									bool 				inInSystemTree,
									MSectionLine& 		inSection,
									MProjectView& 		inProjectView,
									ProjectTargetRec*	inRec,
									const char *		inName);
								MPostLinkFileLine(
									MSectionLine& 	inSection,
									MProjectView& 	inProjectView,
									MBlockFile&		inBlockFile,
									BlockType		inBlockType,
									uint32			inProjectVersion);

	virtual						~MPostLinkFileLine();

	virtual bool				NeedsToBeCompiled();
	virtual bool				NeedsToBePreCompiled();
	virtual void				DeleteObjectFile();

	virtual MCompile*			BuildCompileObj(
									MakeActionT		inKind);

private:
	
		void					InitLine();
		void					ExecuteSelf(
									sem_id 			inSem,
									area_id			inArea);
		MCompile*				ExecuteSelf();
};

#endif
