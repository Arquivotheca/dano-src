//========================================================================
//	MCompilerObj.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MCOMPILEROBJ_H
#define _MCOMPILEROBJ_H

#include "MCompileTool.h"
#include "MSourceFileList.h"
#include "PlugInPreferences.h"
#include "CString.h"

class MSourceFileLine;
class MProjectView;


class MCompilerObj : public MCompileTool
{
public:
 								MCompilerObj(
 									MSourceFileLine* line,
									const char * 	compiler,
									const char *	file,
									BList&		 	args,
									bool 			IDEAware,
									MProjectView& 	inProjectView,
									MakeActionT		inAction);

virtual	status_t				FindHeader(
									const HeaderQuery& 	inQuery,
									HeaderReply& 		outReply);
virtual	status_t				DoMessage(
									const ErrorNotificationMessage& message);
virtual	status_t				DoMessage(
									const ErrorNotificationMessageShort& message);
virtual	void					DoStatus(
									bool objProduced,
									status_t errCode);
virtual	void					DoStatus(
									const CompilerStatusNotification& inRec);
virtual	void					DoPreprocesResult(
									const SendTextMessage& textMessage);
virtual	void					GetArea(
									GetAreaReply& areaMessage);
virtual	status_t				ParseMessageText(
									const char* inText);
virtual	void					CodeDataSize(
									int32&	outCodeSize,
									int32&	outDataSize);
virtual	void					GenerateDependencies();

protected:

		MSourceFileList			fHeaderFileList;		// Files that have already been included
		MSourceFileLine*		fSourceFileLine;
		MProjectView&			fProjectView;
		int32					fHeaderFileID;
		MakeActionT				fCompileKind;
		String					fFileName;
		bool					fSourceFileCalled;
};

#endif
