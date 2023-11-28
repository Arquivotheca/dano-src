//========================================================================
//	MCompileState.h
//	Copyright 1995 - 96 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MCOMPILESTATE_H
#define _MCOMPILESTATE_H

enum CompileState {
	sNotCompiling,				// idle
	sCancelling,				// a cancel is in progress
	sCompilingOne,				// handling cmd-k from a source window
	sCompilingSelection,		// compiling started from the project window
	sPreCompilingOne,			// precompiling from a source window
	sPreCompilingSelection,		// precompiling from the project window
	sPreProcessingOne,			// preprocess from a source window
	sPreProcessingSelection,	// preprocess from the project window
	sDisassemblingOnePartOne,	// disassemble/compile from a source window
	sDisassemblingOnePartTwo,	// disassemble from a source window
	sDisassemblingSelection,	// disassemble from the project window
	sCheckingSyntaxOne,			// checking syntax from a source window
	sCheckingSyntaxSelection,	// checking syntax from the project window

	// four step make
	sBeginMake,					// while searching for files
	sMakePrecompile,			// first step
	sMakeSubproject,			// continue first step - building subprojects
	sMakeCompile,				// second step
	sLinking,					// one step link or third step in make
	sCopyingResources,			// resource copy step in a four step make or 2 step link
	sPostLinkExecute,			// execute postlink files step in a four step make or 2 step link
	sPostLinkFinal,				// file system stuff step in a four step make or 2 step link

	// two step bring up to date (bring up to date only does local project)
	sUpdatePrecompile,			// first step
	sUpdateCompile				// second step
};

#endif
