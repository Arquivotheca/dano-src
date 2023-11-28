//========================================================================
//	MPreCompileObj.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPRECOMPILEOBJ_H
#define _MPRECOMPILEOBJ_H

#include "IDEConstants.h"
#include "MCompilerObj.h"

class MProjectView;
class MSourceFileLine;


class MPreCompileObj : public MCompilerObj
{
public:
 								MPreCompileObj(
									const char * 	compiler,
									const char * 	file,
									BList&		 	args,
									bool 			IDEAware,
									MProjectView& 	inProjectView,
									MSourceFileLine* inSourceLine = nil);

virtual	void					DoStatus(
									bool objProduced,
									long errCode)
								{
									MCompilerObj::DoStatus(objProduced, errCode);
								}
virtual	void					DoStatus(
									const CompilerStatusNotification& inRec);
};

#endif
