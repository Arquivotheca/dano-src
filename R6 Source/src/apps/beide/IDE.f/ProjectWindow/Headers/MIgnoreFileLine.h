//========================================================================
//	MIgnoreFileLine.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MIGNOREFILELINE_H
#define _MIGNOREFILELINE_H

#include "MSourceFileLine.h"

class MIgnoreFileLine : public MSourceFileLine
{
public:
								MIgnoreFileLine(
									const BEntry&		inFile, 
									bool 				inInSystemTree,
									MSectionLine& 		inSection,
									MProjectView& 		inProjectView,
									ProjectTargetRec*	inRec,
									const char *		inName);
								MIgnoreFileLine(
									MSectionLine& 	inSection,
									MProjectView& 	inProjectView,
									MBlockFile&		inBlockFile,
									BlockType		inBlockType,
									uint32			inProjectVersion);

	virtual						~MIgnoreFileLine();

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
	virtual bool				SelectImmediately( 
									BRect 			inFrame, 
									BPoint 			inPoint,
									bool 			inIsSelected,
									uint32			inModifiers,
									uint32			inButtons);

	virtual bool				NeedsToBeCompiled();
	virtual bool				NeedsToBePreCompiled();

	virtual MCompile*			BuildCompileObj(
									MakeActionT		inKind);
	virtual void				DeleteObjectFile();

protected:
	
		void					InitLine();
};

#endif
