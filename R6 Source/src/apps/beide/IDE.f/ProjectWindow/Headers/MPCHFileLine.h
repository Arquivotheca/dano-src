//========================================================================
//	MPCHFileLine.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MPCHFILELINE_H
#define _MPCHFILELINE_H

#include "MSourceFileLine.h"


class MPCHFileLine : public MSourceFileLine
{
public:
								MPCHFileLine(
									const BEntry&		inFile, 
									bool 				inInSystemTree,
									MSectionLine& 		inSection,
									MProjectView& 		inProjectView,
									ProjectTargetRec*	inRec,
									const char *		inName);

								MPCHFileLine(
									MSectionLine& 	inSection,
									MProjectView& 	inProjectView,
									MBlockFile&		inBlockFile,
									BlockType		inBlockType,
									uint32			inProjectVersion);

	virtual						~MPCHFileLine();

	virtual bool				NeedsToBeCompiled();
	virtual bool				NeedsToBePreCompiled();

	virtual MCompile*			BuildCompileObj(
									MakeActionT		inKind);
	virtual void				CompileDone(
									status_t				errCode,
									int32				inCodeSize = 0,
									int32				inDataSize = 0,
									MSourceFileList*	inList = nil);

	virtual void				DeleteObjectFile();

		int32					GetSize() const
								{
									return -1;
								}
private:

		MSourceFile*			fTargetFile;

		time_t					TargetFileModTime();

		MCompile*				PrecompileSelf();

		void					InitLine();
};

#endif
