//	MCompileTool.h
//	Implementation for handling compiles in the IDE
//	Copyright 1995 Metrowerks Corporation. All rights reserved.
//	Jon Watte

#ifndef _MCOMPILETOOL_H
#define _MCOMPILETOOL_H

#include "BeIDEComm.h"
#include "MCompile.h"
#include <Locker.h>

class MFileCache;
class MCompileGenerator;
struct ErrorNotificationMessageShort;
class BList;

class MCompileTool : public MCompile
{
public:
								MCompileTool(
									const char * 	compiler,			// path to the tool
									BList& 			args,				// other compiler args
									bool 			IDEAware = FALSE,	// does this tool communicate with the IDE?
									int32 			threadPriority = B_NORMAL_PRIORITY);
		virtual					~MCompileTool();

	// Actually start the compile. If it returns an error, the compile
	//	is NOT running, but the IsDone() function will think it is.
	virtual	status_t				Run();

	// This will kill the compile, and set the status to SYS_ERROR
	virtual	status_t				Kill();
	
	// Returns true if the MCompileTool object is IDE-aware
	bool						IsIDEAware() { return fIDEAware; }
		
	void						SetInfo(MCompileGenerator* handler, 
										area_id inAreaID, 
										MFileCache* inFileCache)
								{
									fMyHandler = handler;
									fAreaID = inAreaID;
									fFileCache = inFileCache;
								}

	static void					KillTeam(thread_id	tid);

protected:

	// These two should be implemented to do whatever's right for the message
	virtual	status_t			FindHeader(const HeaderQuery & query, HeaderReply & reply);
	virtual	status_t			DoMessage(const ErrorNotificationMessage & message) = 0;
	virtual	status_t			DoMessage(const ErrorNotificationMessageShort& message) = 0;
	
	virtual	void				DoPreprocesResult(const SendTextMessage& textMessage);
	virtual	status_t			ParseMessageText(const char* inText);
	virtual	void				CodeDataSize(int32& outCodeSize,int32& outDataSize);
	virtual	void				GenerateDependencies();
	virtual	void				GetArea(GetAreaReply& areaMessage);

private:
		thread_id				fCompilerThread;
		thread_id				fServeThread;
		thread_id				fWaitThread;
		port_id					fFromCompiler;
		port_id					fToCompiler;
		int32					fIndex;
		int32					fThreadPriority;
		MCompileGenerator*		fMyHandler;
		bool					fIDEAware;
		
static	status_t				CompileWait(void* arg);
static	status_t				CompileServe(void* arg);
};


#endif
