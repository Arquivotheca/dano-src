//	MWEditUtils.h

#ifndef _MWEDITUTILS_H
#define _MWEDITUTILS_H

#include <SupportDefs.h>

#define NOLIMIT ((const char *) 0xffffffff)

	bool						MWEdit_StrCmp(
									register const char*	name,
									register const char*	text);

	int32						CompareStrings(
									const char * inOne,
									const char * inTwo);
	int32						CompareStringsNoCase(
									const char * inOne,
									const char * inTwo);
	char * 						FindEnd(
									const char * 	inText, 
									int32 			inTextLen);
	
	const char *				MWEdit_LineStart(
									const char *text_pos, 
									const char *buffer_start);
	const char *				MWEdit_NextLineMono(
									const char *text_pos, 
									const char *limit);
	int 						stricmp (
									const char *s1, 
									const char *s2);
	int 						strincmp (
									const char *s1, 
									const char *s2,
									size_t n);

	bool 						MWEdit_IsLetter(int c);
	const char* 				MWEdit_PrevSubWord(const char *text_pos, const char *buffer_start);
	const char* 				MWEdit_NextSubWord(const char *text_pos, const char *limit);

#endif

