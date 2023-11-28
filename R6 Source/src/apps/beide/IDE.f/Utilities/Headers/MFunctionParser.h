//========================================================================
//	MFunctionParser.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFUNCTIONPARSER_H
#define _MFUNCTIONPARSER_H

#include <SupportDefs.h>

const int32 MAX_FUNC_NAME = 127;

struct DeclarationInfo {
	char 	name [MAX_FUNC_NAME + 1];	// The name of the function/variable/class
	int32 	declarationStart;	// The offset where the definition begins
	int32 	declarationEnd;		// The offset where the definition ends
	int32	selectionStart;		// Where to start the selection if we jump to this declaration
	int32 	selectionEnd;		// Where to end the selection if we jump to this declaration
	short 	style;				// A bitfield of the following enumerated flags, used for the function popup
};

#if 0
enum {
	PROC_STYLE_NORMAL			= normal,
	PROC_STYLE_BOLD				= bold,
	PROC_STYLE_ITALIC			= italic,
	PROC_STYLE_UNDERLINE		= underline,
	PROC_STYLE_OUTLINE			= outline,
	PROC_STYLE_SHADOW			= shadow,
	PROC_STYLE_CONDENSE			= condense,
	PROC_STYLE_EXTEND			= extend,
	PROC_STYLE_CHECKED			= 0x100
};
#endif

enum {
	PROC_STYLE_NORMAL,
	PROC_STYLE_BOLD,
	PROC_STYLE_ITALIC,
	PROC_STYLE_UNDERLINE,
	PROC_STYLE_OUTLINE,
	PROC_STYLE_SHADOW,
	PROC_STYLE_CONDENSE,
	PROC_STYLE_EXTEND,
	PROC_STYLE_CHECKED = 0x100
};

#include "Utils.h"		// for SuffixType



class MFunctionParser
{
public:
								MFunctionParser();
								MFunctionParser(
									const char*			inText,
									int32				inCurrentOffset,
									SuffixType			inSuffix,
									bool				inIncludeClassName);
								~MFunctionParser();
		void					Parse(
									const char*			inText,
									int32				inCurrentOffset,
									SuffixType			inSuffix,
									bool				inIncludeClassName);

		void					Sort();

		int						FunctionCount()
								{
									return fNumFunctions;
								}
		const DeclarationInfo*	FunctionStorage()
								{
									return fFunctionStorage;
								}

static void						SetRelaxedCParse(
									bool	inDoIt);

private:

// These globals are used by the functions InitFunctionArray() and storeFunctionInfo().
		const char *		fTextStart;
		int 				fNumFunctions;
		int 				fNumSlots;
		int 				fStackDepth;
		DeclarationInfo* 	fFunctionStorage;
		bool 				fFormat;
static bool					sRelaxedCParse;


	int							ParseDeclarations (
									SuffixType	suffixType, 
									int32 		selectionOffset, 
									bool 		includeClassName);
	int 						ParseCDeclarations (
									int32 selectionOffset, 
									bool format);
	int 						ParseJavaDeclarations (
									int32 selectionOffset, 
									bool format);

	int							ParseDump (
									int32 selectionOffset);

	const char *				findNextCDeclaration (
									const char *	from, 
									char *			decName, 
									const char **	nameStart,
									const char **	nameEnd, 
									const char **	decStart, 
									const char **	decEnd,
									bool include_class_name);
	bool						findNextJavaDeclaration(
									const char **	ppCurrPos, 
									char *			decName, 
									const char **	nameStart,
									const char **	nameEnd, 
									const char **	decStart, 
									const char **	decEnd,	
									bool 			include_class_name, 
									bool 			isInterface);

	const char*					findNextJavaClass( 
									const char* 	from, 
									const char** 	pClassName, 
									bool*			isInterface);

	DeclarationInfo*			IncreaseTableSize();

	void						InitFunctionArray (
									const char *textStart);

	const char *				skipCComments (
									const char *from) const;
	const char *				skipCParens(
									const char *from, 
									char par) const;
	const char *				NE_LineStart(
									const char *text_pos, 
									const char *buffer_start) const;
	const char *				skipto_noescape (
									const char *from, 
									short c) const;
	const char *				asm_skipto (
									const char *from, 
									short c) const;
	bool 						is_template(
									const char *from) const;
	char						getIdentSize(
									const char* pStartChar) const;
	const char*					findPrevIdent(
									const char* pStartChar) const;
	const char*					findChar( 
									const char* start, 
									const char* end, 
									char findChar) const;
	const char*					skipCCommentsBack (
									const char *from) const;
	const char*					findPrevStatement( 
									const char* startAt) const;

	static int					FunctionCompare(
									const DeclarationInfo *a, 
									const DeclarationInfo *b);
};

#endif
