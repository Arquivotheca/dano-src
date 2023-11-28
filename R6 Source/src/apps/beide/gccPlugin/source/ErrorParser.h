// ---------------------------------------------------------------------------
/*
	ErrorParser.h
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			2 October 1998

*/
// ---------------------------------------------------------------------------

#ifndef _ERRORPARSER_H
#define _ERRORPARSER_H

class BList;
class ErrorMessage;

#include <String.h>

// ---------------------------------------------------------------------------
//	Class ErrorParser
// ---------------------------------------------------------------------------

class ErrorParser
{
public:
						ErrorParser(const char* inText, BList& outList);
	virtual				~ErrorParser();

	virtual void		ParseText();

protected:
	virtual bool		GetNextLine();
	
	virtual void		AppendToErrorText(const char* newText);
	virtual void		PostErrorMessage();
	virtual void		CreateNewErrorMessage();

	void				SetErrorWarning(bool isWarning);
	void				SetErrorLineNumber(long lineNumber);
	void				SetErrorFileName(const char* fileName);
	char				GetLastChar();
	
	const char*			GetCurrentErrorText();
	
	// data...
	ErrorMessage*		fErrorMessage;
	char*				fCurrentLine;

private:
	virtual void		DoParse() = 0;
	
	// data...
	BList&				fErrorList;
	const char*			fBufferPosition;
	long				fLineLength;
	BString				fErrorTextBuffer;
};

// ---------------------------------------------------------------------------
//	Class CompilerErrorParser
// ---------------------------------------------------------------------------

class CompilerErrorParser : public ErrorParser
{
public:
					CompilerErrorParser(const char* inText, BList& outList);
				
private:
	virtual void	DoParse();
	void			ParseErrorLine();

	bool			IsThrowAwayLine();

};

// ---------------------------------------------------------------------------
//	Class LinkerErrorParser
// ---------------------------------------------------------------------------

class LinkerErrorParser : public ErrorParser
{
public:
					LinkerErrorParser(const char* inText, BList& outList);

private:
	virtual void	DoParse();
};

#endif
