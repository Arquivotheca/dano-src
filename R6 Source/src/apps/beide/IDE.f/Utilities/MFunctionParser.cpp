//========================================================================
//	MFunctionParser.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Modified from CW7 version of MWParseFunctions.c

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "IDEConstants.h"
#include "MFunctionParser.h"
#include "MWMunger.h"
#include "MWEditUtils.h"

#include <Debug.h>

bool			MFunctionParser::sRelaxedCParse;

const unsigned char PREFIX_CHAR = '\t';

// Utility inline functions
// macro to see if a character is valid to start an identifier (C & Pascal)
inline bool isidentf(char c)	
{
	return (isalpha(c) || ((c) == '_'));
}

// macro to see if a character is valid in an identifier (C & Pascal)
inline bool isident(char c)	
{
	return (isalnum(c) || ((c) == '_'));
}

// ---------------------------------------------------------------------------
//		MFunctionParser
// ---------------------------------------------------------------------------
//	Default constructor.  Used for persistent objects.

MFunctionParser::MFunctionParser()
{
	fNumFunctions = -1;
	fFunctionStorage = nil;
}

// ---------------------------------------------------------------------------
//		MFunctionParser
// ---------------------------------------------------------------------------

MFunctionParser::MFunctionParser(
	const char*			inText,
	int32				inCurrentOffset,
	SuffixType			inSuffix,
	bool				inIncludeClassName)
	: fTextStart(inText)
{
	Parse(inText, inCurrentOffset, inSuffix, inIncludeClassName);
}

// ---------------------------------------------------------------------------
//		~MFunctionParser
// ---------------------------------------------------------------------------
//	Destructor

MFunctionParser::~MFunctionParser()
{
	delete fFunctionStorage;
}

// ---------------------------------------------------------------------------
//		Parse
// ---------------------------------------------------------------------------

void
MFunctionParser::SetRelaxedCParse(
	bool	inDoIt)
{
	sRelaxedCParse = inDoIt;
}

// ---------------------------------------------------------------------------
//		Parse
// ---------------------------------------------------------------------------

void
MFunctionParser::Parse(
	const char*			inText,
	int32				inCurrentOffset,
	SuffixType			inSuffix,
	bool				inIncludeClassName)
{
	InitFunctionArray(inText);
	
	fNumFunctions = ParseDeclarations(inSuffix, inCurrentOffset, inIncludeClassName);
}

// ---------------------------------------------------------------------------
//		Sort
// ---------------------------------------------------------------------------

void
MFunctionParser::Sort()
{
	if (fFunctionStorage != nil && fNumFunctions > 0)
		qsort(fFunctionStorage, fNumFunctions, sizeof(DeclarationInfo), (compare_members) FunctionCompare);
}

// ---------------------------------------------------------------------------
//		FunctionCompare
// ---------------------------------------------------------------------------

int 
MFunctionParser::FunctionCompare(
	const DeclarationInfo *a, 
	const DeclarationInfo *b)
{
	return strcmp(a->name, b->name);
}

// ---------------------------------------------------------------------------
//		ParseDeclarations
// ---------------------------------------------------------------------------

int 
MFunctionParser::ParseDeclarations (
	SuffixType 			suffixType, 
	int32 				selectionOffset, 
	bool 				includeClassName)
{
	switch (suffixType) 
	{
		case kCSuffix:
		case kCPSuffix:
		case kHSuffix:
			return ParseCDeclarations(selectionOffset, includeClassName);
			break;

		case kJavaSuffix:
			return ParseJavaDeclarations(selectionOffset, includeClassName);
			break;

		case kDumpSuffix:
			return ParseDump(selectionOffset);
			break;

		default:
			return -1;
			break;
	}
}

// ---------------------------------------------------------------------------
//		IncreaseTableSize
// ---------------------------------------------------------------------------

DeclarationInfo* 
MFunctionParser::IncreaseTableSize()
{
	int		slots = fNumSlots;

	if (fNumSlots == 0)
		fNumSlots = 10;
	else
		fNumSlots *= 2;

	DeclarationInfo* 	newInfoPtr = new DeclarationInfo[fNumSlots];

	if (fFunctionStorage)
	{
		memcpy(newInfoPtr, fFunctionStorage, slots * sizeof(DeclarationInfo));
		delete[] fFunctionStorage;
	}

	fFunctionStorage = newInfoPtr;
	
	return fFunctionStorage;
}

/****************************************************************/
/* Purpose..:                                    				*/
/* Input....:													*/
/* Returns..: ---												*/
/****************************************************************/

int 
MFunctionParser::ParseCDeclarations (
	int32 				selectionOffset, 
	bool 				format)
{
	struct DeclarationInfo *functionInfo;
	char			procName[256];
	int 			n, numSlots;
	const char *	nameStart, *nameEnd;
	const char *	procStart, *procEnd;
	const char *	textStart = fTextStart;
	const char *	currentPosition = textStart;

	functionInfo = fFunctionStorage;
	numSlots = fNumSlots;

	for (n = 0;
		 (currentPosition = findNextCDeclaration (currentPosition, procName, &nameStart,
		 						&nameEnd, &procStart, &procEnd, format)) != 0;
		 n++)
	{
		if (n >= numSlots) {
			functionInfo = IncreaseTableSize();
			numSlots = fNumSlots;
		}
		strcpy(functionInfo[n].name, procName);
		functionInfo[n].selectionStart = nameStart - textStart;
		functionInfo[n].selectionEnd = nameEnd - textStart;
		functionInfo[n].declarationStart = procStart - textStart;
		functionInfo[n].declarationEnd = procEnd - textStart;
		functionInfo[n].style = PROC_STYLE_NORMAL;

		// Check this function if the selection is between the start and end of this function
		if ((selectionOffset >= functionInfo[n].declarationStart) &&
			(selectionOffset < functionInfo[n].declarationEnd))
		{
			functionInfo[n].style |= PROC_STYLE_CHECKED;
		}

		fNumFunctions++;
	}
	
	return n;
}

/****************************************************************************/
/* Purpose..: Initialize the function storage globals						*/
/* Input....: the DeclarationInfo* to store all the info in							*/
/* Input....: ptr to the beginning of the text, used to calc offsets		*/
/* Returns..: ptr to the next non-white char that's not part of a comment	*/
/****************************************************************************/

void
MFunctionParser::InitFunctionArray (
 	const char *	textStart) 
{
	fTextStart = textStart;
	fFormat = true;
	fNumFunctions = 0;
	fNumSlots = 0;
	fFunctionStorage = nil;
	IncreaseTableSize();
}

/****************************************************************************/
/* Purpose..: Skips all white space and comments in C/C++ code				*/
/* Input....: pointer to text												*/
/* Returns..: ptr to the next non-white char that's not part of a comment	*/
/****************************************************************************/

const char *
MFunctionParser::skipCComments(const char *from) const
{
	do {
		// skip white space
		while (isspace (*from))
			from++;
		
		// is this a comment ?
		if (from[0] == '/')
		{
			if (from[1] == '*') // C-style comment;  skip it
			{
				from+=2;
				do
				{
					from = skipto_noescape(from, '*');
					if (*from == '/') // this is the end of the comment.
					{
						from++;
						break;
					}
				} while (*from != '\0');
			}
			else if (from[1] == '/') // C++-style comment;  skip it
			{
				from = asm_skipto(from, EOL_CHAR);
//				from = skipto_noescape(from, EOL_CHAR);
			}
			else // The slash isn't part of a comment, so we break.
				break;
		}
		else // This is a non-white character that isn't a slash, break.
			break;

	} while (*from != '\0');

	return (from);
}

/****************************************************************************/
/* Purpose..: Skips to a matching parenthesis in C/C++ code					*/
/* Input....: pointer to text												*/
/* Input....: The type of paren to match, '(' or '[' or '{'					*/
/* Returns..: ptr to the char *after* the one that matches the passed paren	*/
/****************************************************************************/

const char *
MFunctionParser::skipCParens(
	const char *	from, 
	char 			openPar) const
{
	register short c;
	char closePar;
	
	switch (openPar)
	{
		case '(':	closePar = ')'; break;
		case '{':	closePar = '}'; break;
		case '[':	closePar = ']'; break;
		default:	ASSERT(false); return from;
	}

	while (1) {
		//	skip any white-space & comments.
		from = skipCComments (from);

		//	get the next character
		c = *from++;
		
		//	skip character constants.
		if (c == '\'') 
		{
			from = asm_skipto(from, '\'');
			continue;
		}
		
		//	skip string constants.
		if (c == '"') 
		{
			from = asm_skipto(from, '"');
			continue;
		}
		
		//	skip preprocessor directives
		if (c == '#') 
		{
			from = asm_skipto(from, EOL_CHAR);
			continue;
		}
		
		// found another opening paren, do the recursive thang.
		if (c == openPar)
		{
			from = skipCParens(from, openPar);
			continue;
		}
		
		// is this a match?
		if (c == closePar)
			return (from);
		
		// whoops, end of text, return the pointer to that
		if (c == '\0')
			return (from-1);
	}
}

/********************************************************************/
/* Purpose..: Find the next global declaration in a block of text	*/
/* Input....: pointer to text										*/
/* Input....: pointer to storage for declaration name				*/
/* Output...: pointer to pointer where the proc name starts			*/
/* Output...: pointer to pointer where the proc name ends			*/
/* Output...: pointer to pointer where the proc declaration starts	*/
/* Output...: pointer to pointer where the proc declaration ends	*/
/* Input....: TRUE: only return declarations that generate code		*/
/* Input....: TRUE: include class name								*/
/* Returns..: pointer to current position in text					*/
/********************************************************************/

const char *
MFunctionParser::findNextCDeclaration (
	const char *		from, 
	char *				decName, 
	const char **		nameStart,
	const char **		nameEnd, 
	const char **		decStart, 
	const char **		decEnd,	
	bool 				include_class_name)
{
	const char *		textStart = from;
	char *				name;
	const char *		idstart;
	bool 				destructor;
	int 				size;
	
	*decStart = nil;
	*decEnd = nil;

	while (1) {
		destructor = FALSE;
		
		//	skip any comments & white-space
		from = skipCComments (from);
		
		if (*from == '\0') return nil;

		//	skip character constants.
		if (from[0] == '\'') {
			from++;
			from = asm_skipto(from, '\'');
			continue;
		}

		//	skip string constants.
		if (from[0] == '"') {
			from++;
			from = asm_skipto(from, '"');
			continue;
		}

		// 	skip parens
		if (from[0] == '(' || from[0] == '[' || from[0] == '{') {
			from++;
			from = skipCParens(from, from[-1]);
			continue;
		}

		if (isidentf (from[0])) { // we have the start of an identifier
			if (*decStart == nil)
				*decStart = MWEdit_LineStart (from, textStart);
			*nameStart = from;

			name = decName;
			idstart = decName;
			size = 0;
			
			// get the identifier
			while (isident(*from)) {
				if (size < MAX_FUNC_NAME - 1) {
					*name++ = *from++;
					size++;
				} else from++;
			}
			
			*name = 0;
			*nameEnd = from;
	
			// skip comments/spaces
			from = skipCComments (from);
			
			if (is_template(from)) {  // parse template functions/classes
				if (size < MAX_FUNC_NAME - 1) {
					*name++ = *from++;
					size++;
				} else from++;
				// skip comments/spaces
				from = skipCComments (from);
	
				while (isident(*from)) {
					while (isident(*from)) {
						if (size < MAX_FUNC_NAME - 1) {
							*name++ = *from++;
							size++;
						} else from++;
					}
					// skip comments/spaces
					from = skipCComments (from);

					// skip over the comma
					if (from[0] == ',') {
						from++;
						if (size < MAX_FUNC_NAME - 2) {
							*name++ = ','; *name++ = ' ';
							size += 2;
						}

						// skip comments/spaces
						from = skipCComments (from);
					}
				}
				*name = 0;
				*nameEnd = from;
				if (from[0] == '>') {
					if (size < MAX_FUNC_NAME - 1) {
						*name++ = *from++;
						size++;
					} else from++;
					*name = 0;
					*nameEnd = from;
					from = skipCComments (from);
				}
			}
			else if (strcmp(idstart, "extern") == 0) {
				// it wasn't a function - don't track it
				*decStart = nil;
				// skip comments/spaces
				from = skipCComments (from);
				if (from[0] == '"') {
					from++;
					from = asm_skipto(from, '"');
					from = skipCComments (from);
					if (from[0] == '{') {
						from += 1;
						continue;
					}
				}
				continue;
			}
			else if (strcmp(idstart, "namespace") == 0) {
				// it wasn't a function - don't track it
				*decStart = nil;
				// skip comments/spaces
				from = skipCComments (from);
				// skip over namespace name
				while (isident(*from)) {
					from++;
				}
				from = skipCComments(from);
				if (from[0] == '{') {
					from += 1;
					continue;
				}
				continue;
			}
		
			while (from[0] == ':' && from[1] == ':') {
				
				// scope resolution operator
				
				if (size < MAX_FUNC_NAME - 2) {
					*name++ = ':';
					*name++ = ':';
					size += 2;
				}
				from += 2;

				// skip comments/spaces
				from = skipCComments (from);

				if (!include_class_name) {
					size = 0;
					name = decName;
				} else {
					idstart = name;
				}

				if (*from == '~') {
					// this is a destructor
					if (size < MAX_FUNC_NAME - 1) {
						*name++ = '~';
						size++;
					}
					from++;

					// skip comments/spaces
					from = skipCComments (from);
					
					destructor = true;
				}
				
				if (isidentf(from[0])) {
					// get the identifier
					while (isident(*from)) {
						if (size < MAX_FUNC_NAME - 1) {
							*name++ = *from++;
							size++;
						} else from++;
					}
				}
				*name = 0;
				*nameEnd = from;
	
				// skip comments/spaces
				from = skipCComments (from);

				if (is_template(from)) {  // parse template functions/classes
					if (size < MAX_FUNC_NAME - 1) {
						*name++ = *from++;
						size++;
					} else from++;
					// skip comments/spaces
					from = skipCComments (from);
		
					while (isident(*from)) {
						while (isident(*from)) {
							if (size < MAX_FUNC_NAME - 1) {
								*name++ = *from++;
								size++;
							} else from++;
						}
						// skip comments/spaces
						from = skipCComments (from);
	
						// skip over the comma
						if (from[0] == ',') {
							from++;
							if (size < MAX_FUNC_NAME - 2) {
								*name++ = ','; *name++ = ' ';
								size += 2;
							}
	
							// skip comments/spaces
							from = skipCComments (from);
						}
					}
					*name = 0;
					*nameEnd = from;
					if (from[0] == '>') {
						if (size < MAX_FUNC_NAME - 1) {
							*name++ = *from++;
							size++;
						} else from++;
						*name = 0;
						*nameEnd = from;
						from = skipCComments (from);
					}
				}
			}
			
			if (!destructor && (strcmp(idstart, "operator") == 0)) {
				
				if (size < MAX_FUNC_NAME - 3) {
					// this is an operator, put in a space
					*name++ = ' ';
					size++;					
				}
				
				if(*from == '(') {
					*name++ = '(';
					from++;
					size++;
				}

				from = skipCComments(from);
	
				while ((*from != '(') && (size < MAX_FUNC_NAME)) {
					if(isidentf(*from)) {
						while(isident(*from) && (size < MAX_FUNC_NAME)) {
							*name++ = *from++;
							size++;
						}
					}
					else if(!isspace(*from)) {
						// get all symbol characters, but watch out for comment starts
						while(!isspace(*from) && !isidentf(*from) && (*from != '(')
							&& !((*from == '/') && ((from[1] == '*') || (from[1] == '/')))
							 && (size < MAX_FUNC_NAME))
						{

							*name++ = *from++;
							size++;
						}
					}
					// add a single space after each part of the operator's name
					from = skipCComments(from);
					if((!from != '(') && (size < MAX_FUNC_NAME)) {
						*name++ = SPACE;
						size++;
					}
				}
				*name = 0;
				*nameEnd = from;
	
				from = skipCComments(from);

			}
			
			if (*from == '(') { //	this is a function declaration or prototype.
				from++;
				
				// skip the argument list
				from = skipCParens(from, '(');
				
				// skip comments/spaces
				from = skipCComments (from);

				/*
				 *					We Live in a Crazy World
				 *					^^^^^^^^^^^^^^^^^^^^^^^^
				 *
				 *	Because of the damned C preprocessor it's actually impossible to
				 *	correctly parse all "valid" forms of function.  What you see below
				 *	is my attempt to rectify the problem.
				 *
				 *	The code you see in the first half of the following if-statement is
				 *	copied directly from the CW9 code base.  It was largely correct and
				 *	pleased most people.  HOWEVER, the Microsoft Foundation Classes use
				 *	a sad and frightening set of preprocessor #defines that look kinda like
				 *	functions to this simple parser.
				 *
				 *	So, for CW10 I decided to "end the problem once and for all" and wrote
				 *	the code now found the "else" section.  I sat down with the C++ grammer
				 *	specification and wrote a "better" parser, that was less like to mistake
				 *	freaky macros for functions.
				 *
				 *	Now, in CW11, we got bug reports from people who purposely use freaky
				 *	macros in their functions (the specific case was using a macro for the
				 *	"throw" specifier for functions) so this "better" parser broke their stuff.
				 *
				 *	After much soul-searching and general bitching, I came up with a truly
				 *	ambigous case.  The only solution is to either include some logic to expand
				 *	macros (generally considered by me to not be worth the trouble) or to have
				 *	a pref switch that toggled between the CW9 and CW 10 behaviours.
				 *
				 *	And thus the following code came to pass...  good luck with it!
				 *
				 *	Sincerely,
				 *	Dieter Shirley
				 *
				 */				
				if (sRelaxedCParse)
				{
					if (from[0] == ':') { // initializer list for a constructor
						while ((*from != '{') && (*from != ';')) from++;
						if (*from == '{') {
							from++;
							from = skipCParens(from, '{');
						} else from ++;
						if (*from == EOL_CHAR) from++;		// \r
						*decEnd = from;
						return from;
					}
	
					if(isidentf(*from) || *from == '{')
					{
						// skip over the code
						from = skipto_noescape (from, '{');
						from = skipCParens (from, '{');
						if (*from == EOL_CHAR) from++;		// \r
						*decEnd = from;
						return from;
					}
				}
				else
				{
					// skip over "const"
					// (C++ standard also allows "volatile" keyword here...wierd!)
					if (strncmp (from, "const", 5) == 0)
					{
						from += 5;
	
						// skip comments/spaces
						from = skipCComments (from);
					}
					
					// skip over throw specification
					if (strincmp (from, "throw", 5) == 0)
					{
						from += 5;
	
						// skip comments/spaces
						from = skipCComments (from);
						
						if (*from == '(')
						{
							from++;
	
							// skip the exception list
							from = skipCParens(from, '(');
							
							// skip comments/spaces
							from = skipCComments (from);
						}
					}
	
					if (*from == ':') { // initializer list for a constructor
						do {
	
							from++;
							
							// skip comments/spaces
							from = skipCComments (from);
							
							while (isident(*from))
								from++;
							
							// skip comments/spaces
							from = skipCComments (from);
							
							if (*from == '(')
							{
								from++;
								
								// skip initializer arguments
								from = skipCParens(from, '(');
								
								// skip comments/spaces
								from = skipCComments (from);
							}
							
						// skip the next initializer, if there's a comma
						} while (*from == ',');  
					}
						
					// Skip preprocessor directives between function name and block
					while (*from == '#')
					{
						// skip over preprocessor stuff
						from = asm_skipto (from, EOL_CHAR);
						// skip comments/spaces
						from = skipCComments (from);
					}
					
					if(*from == '{')
					{
						// This is a function!
						
						from++;
	
						// skip over the code
						from = skipCParens (from, '{');
						if (*from == EOL_CHAR) from++;			// \r
						*decEnd = from;
						return from;
					}
					
					// it wasn't a function, continue parsing anything after the name
					from = *nameEnd;					
				}
			}
			*decStart = nil;
			continue;
		}

		if (from[0] == '#') { // preprocessor directive, look for "#pragma mark ..."
			*decStart = from;
			
			// skip the '#' mark
			from++;

			while (isspace(*from))
				from++;
			
			if (strncmp(from, "pragma", 6) == 0) {
				// we have "#pragma"
				from += 6;
				while (isspace(*from))
					from++;
				
				if (strncmp(from, "mark", 4) == 0) {
					// we have "#pragma mark" !
					from += 4;
					while (isspace(*from))
						from++;
					// take everything until the end of line
					size = 0;
					*nameStart = from;
					name = decName;
					while ((size < MAX_FUNC_NAME) && (*from != 0) && (*from != EOL_CHAR)) {
						*name++ = *from++;
						size++;
					}
					*name = '\0';
					*decEnd = from;
					*nameEnd = from;
					return (from);
				}
			}
			// it wasn't #pragma mark, but it *was* a preprocessor symbol,
			// skip the rest of the line.
			*decStart = nil;
			from = asm_skipto(from, EOL_CHAR);
			continue;
		}
		// Didn't find anything we liked, advance and try again...
		from++;
	}
}

/*
 *	is_template
 *
 *	Returns true if the current character location could be the start of a template
 *	declaration. That is, the current character is '<' and the next non-whitespace
 *	character is an identifier start character
 */

bool 
MFunctionParser::is_template(
	const char *from) const
{
	const char *p = from+1;
	if(from[0] == '<') {
		p = skipCComments(p);
		return isidentf(*p);
	}
	return false;
}


int 
MFunctionParser::ParseDump (
	int32 				selectionOffset)
{
	struct DeclarationInfo *	functionInfo;
	int				n = 0;
	int32 			curPosition = 0;
	int32 			textLength;
	int 			numSlots;
	unsigned char 	name_length;
	char			name[256];
	int32 			blockStart, blockEnd;
	int32 			nameStart, nameEnd;
	bool 			localBlock;
	const char *	textStart = fTextStart;

	textLength = strlen (textStart);
	functionInfo = fFunctionStorage;
	numSlots = fNumSlots;

	while (1) {
		if ((curPosition = MWMunger("AL_CODE", 7, textStart, textLength,
			 						curPosition, TRUE, FALSE, TRUE)) == -1)	{
			break;
		}

		if (n >= numSlots) {
			functionInfo = IncreaseTableSize();
			numSlots = fNumSlots;
		}

		if (textStart[curPosition-1] == 'C') 
			localBlock = TRUE;
		else 
			localBlock = FALSE;
		
		// go back to the newline to find the beginning of the block
		blockStart = curPosition;
		while (textStart[--blockStart] != EOL_CHAR)
			;
		blockStart++;
		
		// skip to the first double quote
		while ((textStart[curPosition] != '"') &&
			   (textStart[curPosition] != 0)) {
			curPosition++;
		}
		curPosition++;
		nameStart = curPosition;
		
		// assume that the hunk name is between quotes
		name_length = 0;
		while ((name_length < MAX_FUNC_NAME - 1) &&
			   (textStart[curPosition] != '"') &&
			   (textStart[curPosition] != EOL_CHAR) &&
			   (textStart[curPosition] != 0))
		{
			name[name_length++] = textStart[curPosition++];
		}

		name[name_length] = 0;
		nameEnd = curPosition;
		
		// find the end of the block
		curPosition = MWMunger ("\n\n", 2, textStart, textLength, curPosition, TRUE, FALSE, TRUE);
		
		if (curPosition == -1) 
			blockEnd = textLength;
		else
			blockEnd = curPosition+1;
		
		// fill in the data structure
		strcpy(functionInfo[n].name, name);
		functionInfo[n].selectionStart = nameStart;
		functionInfo[n].selectionEnd = nameEnd;
		functionInfo[n].declarationStart = blockStart;
		functionInfo[n].declarationEnd = blockEnd;
		functionInfo[n].style = PROC_STYLE_NORMAL;
		fNumFunctions++;

		if (localBlock)
			functionInfo[n].style |= PROC_STYLE_ITALIC;
			
		if ((blockStart <= selectionOffset) && (blockEnd >= selectionOffset))
			functionInfo[n].style |= PROC_STYLE_CHECKED;
			
		n++;
		if (curPosition == -1) break;
	}

	return n;
}

/****************************************************************/
/* Purpose..: finds Java declarations in the current file and   */
/*			  adds them to the Function popup 					*/
/* Input....: pointer to text in the current file				*/
/*			  offset to function								*/
/*			  functionInfo data structure						*/
/*			  													*/
/* Returns..: number of functions or error code					*/
/****************************************************************/

int 
MFunctionParser::ParseJavaDeclarations (
	int32 		selectionOffset, 
	bool	 	format)
{
	struct DeclarationInfo *functionInfo;
	char			procName[256];
	int 			n, numSlots;
	const char *	nameStart, *nameEnd;
	const char *	procStart, *procEnd;
	const char *	textStart = fTextStart;
	const char *	currentPosition = textStart;
	const char*		pClassName;
	uint32			classLen, procLen;
	bool	 		isInterface;

	functionInfo = fFunctionStorage;
	numSlots = fNumSlots;
	
	// Get Name of class	
	n=0;
	
	while ( (currentPosition = findNextJavaClass( currentPosition, &pClassName, &isInterface)) !=0) {
		for ( ; findNextJavaDeclaration (&currentPosition, procName, &nameStart,
		 						&nameEnd, &procStart, &procEnd, format, isInterface);n++) {
			if (n >= numSlots) {
				functionInfo = IncreaseTableSize();
				numSlots = fNumSlots;
			}

			classLen = (format) ? getIdentSize( pClassName) : 0;
			procLen = getIdentSize( procName);

			// copy the classname.procname into the menu data structure
			classLen = min(classLen, 126UL);
			memcpy(functionInfo[n].name, pClassName, classLen);
			functionInfo[n].name[classLen] = '.';
			procLen = min(procLen, 127 - classLen - 1);
			memcpy(functionInfo[n].name + classLen + 1, procName, procLen);
			functionInfo[n].name[classLen + procLen + 1] = '\0';

			functionInfo[n].selectionStart 		= nameStart - textStart;	//start of selected text when item is selected
			functionInfo[n].selectionEnd 		= nameEnd - textStart;		//end of selected text when item is selected
			functionInfo[n].declarationStart 	= procStart - textStart;	//start of 1st line of declaration
			functionInfo[n].declarationEnd 		= procEnd - textStart;		//ending of last line of declaration
			functionInfo[n].style 				= PROC_STYLE_NORMAL;

			// Check this function if the selection is between the start and end of this function
			if ((selectionOffset >= functionInfo[n].declarationStart) &&
				(selectionOffset < functionInfo[n].declarationEnd))
			{
				functionInfo[n].style |= PROC_STYLE_CHECKED;
			}
	
			fNumFunctions++;
		}
	}

	return n;
}

/********************************************************************/
/* Purpose..: Find the next global declaration in a block of text	*/
/* Input....: pointer to text										*/
/* Input....: pointer to storage for declaration name				*/
/* Output...: pointer to pointer where the proc name starts			*/
/* Output...: pointer to pointer where the proc name ends			*/
/* Output...: pointer to pointer where the proc declaration starts	*/
/* Output...: pointer to pointer where the proc declaration ends	*/
/* Input....: TRUE: only return declarations that generate code		*/
/* Input....: TRUE: include class name								*/
/* Returns..: pointer to current position in text					*/
/********************************************************************/

bool 
MFunctionParser::findNextJavaDeclaration(
	const char **		ppCurrPos, 
	char *				decName, 
	const char **		nameStart,
	const char **		nameEnd, 
	const char **		decStart, 
	const char **		decEnd,	
	bool 				/*include_class_name*/,	// not used
	bool 				isInterface)
{
	const char *from = *ppCurrPos;
	const char *textStart = from;
	char *name;
	char *idstart;
	const char *modStart;
	bool destructor;
	int size;
		
	*decStart = nil;
	*decEnd = nil;
	
	while (1) {
		destructor = FALSE;
		
		//	skip any comments & white-space
		from = skipCComments (from);
		
		if (*from == '\0') return false;

		//	skip character constants.
		if (from[0] == '\'') {
			from++;
			from = asm_skipto(from, '\'');
			continue;
		}

		//	skip string constants.
		if (from[0] == '"') {
			from++;
			from = asm_skipto(from, '"');
			continue;
		}
		
		// This is the end of a class
		if ( from[0] == '}') {
			from++;
			*ppCurrPos = from;
			return false;
		}
			
		// 	skip parens
		if (from[0] == '(' || from[0] == '[' || from[0] == '{') {
			from++;
			from = skipCParens(from, from[-1]);
			continue;
		}

		if (isidentf (from[0])) { // we have the start of an identifier
			if (*decStart == nil)
				*decStart = MWEdit_LineStart (from, textStart);
			*nameStart = from;

			name = decName;
			idstart = decName;
			size = 0;
			
			// get the identifier
			while (isident(*from)) {
				if (size < MAX_FUNC_NAME - 1) {
					*name++ = *from++;
					size++;
				} else from++;
			}
			
			*name = 0;
			*nameEnd = from;
	
			// skip comments/spaces
			from = skipCComments (from);
			
			if (*from == '(') { //	this is a function declaration or prototype.
				from++;
				
				// skip the argument list
				from = skipCParens(from, '(');
				
				// skip comments/spaces
				from = skipCComments (from);
				
				if (*from != ',' && *from != '=' &&
					*from != '(') { // this is a function declaration!
					
					// Look for the left brace or semicolon, which signals the end of the
					// declaration portion of this method. Square brackets or the throws clause
					// can appear in this space
					while ((*from != ';') && ( *from != '{')) {
						from++;
						from = skipCComments(from);
					}

					if (*from == ';') { // this could be an abstract or native method,
										// or an interface method declaration
												
						// Start at the method's name identifier and step backwards
						// to see if the method modifier is abstract or native
						modStart = *nameStart - 1;
						
						// find end of previous statement
						const char *endAt = findPrevStatement(modStart-1);
						
						// Eliminate an assignment from being considered as a interface method
						// declaration, final, or abstract method declaration by looking for an
						// equals sign
						if (!findChar( endAt+1, modStart, '=')) {
							
							// If this is an interface, accept this statement as a method declaration
							if ( isInterface) {
									from++;
									if (*from == EOL_CHAR) from++;		// \r
									*decEnd = from;
									*ppCurrPos = from;
									return true;
							}							
							
							// Look at the preceding identifiers to see if this method is abstract or native
							do {
								modStart = findPrevIdent( modStart);
								if ( MWEdit_StrCmp("abstract", modStart) ||
									 MWEdit_StrCmp("native", modStart))	{
									from++;
									if (*from == EOL_CHAR) from++;		// \r
									*decEnd = from;
									*ppCurrPos = from;
									return true;
								}
								modStart--;	
							} while ( modStart > endAt);
						}
						
					} else {
							// skip over the code
						from = skipto_noescape (from, '{');
						from = skipCParens (from, '{');
						if (*from == EOL_CHAR) from++;		// \r
						*decEnd = from;
						*ppCurrPos = from;
						return true;
					}
				}
			}
			*decStart = nil;
			continue;
		}

		// Didn't find anything we liked, advance and try again...
		from++;
	}
}

/******************************************************************************/
/* Purpose..: Finds the next declaration of a class in java		 			  */
/* Input....: pointer to text												  */
/* Returns..: a pointer to the class name, 									  */
/*			  if it's an interface											  */
/*			  advances the input pointer to the beginning of the class block  */
/******************************************************************************/
const char* 
MFunctionParser::findNextJavaClass( 
	const char* 	from, 
	const char** 	pClassName, 
	bool*			isInterface)
{

	const char* start = from;
	char  size; 
	
	*isInterface = false;
	
	from = skipCComments( from);
	
	// Find the class definition
	while( *from != '\0') {
		if (!isident(*from))
			from = skipCComments( from);
						
		if ( MWEdit_StrCmp("class", from)) {
			size = 5;
			if (from != start)
				if ( !isident( from[-1]))
					if ( !isident(from[size]))
						break;
		}
						
		if ( MWEdit_StrCmp("interface", from)) {
			size = 9;
			if (from != start)
				if ( !isident( from[-1]))
					if ( !isident(from[size])) {
						*isInterface = true;
						break;
					}
		}
		
		from++;
	}
	
	if (*from == '\0')
		return 0;
	
	// The first identifier after the class keyword is the classname
	from += size;
	
	from = skipCComments( from);
	
	// find the beginning of the classname and return a pointer to it
	while (!isidentf( from[0])) from++;
	*pClassName = from;	

	// Locate start of class or interface block
	while ((*from != '\0') && (*from != '{')) from++;

	if (*from == '\0') return 0;
	
	// Move character into block
	from++;
	
	return from;
}

/*******************************************************/
/* skipto_noescape() is a function that is essentially */
/* identical to asm_skipto() except that it ignores    */
/* backslashes.                                        */
/*******************************************************/

const char *
MFunctionParser::skipto_noescape (const char *from, short c) const
{							/* Skip all characters until we reach a char c */
	while (*from) {
		if (*from == c) {
			from++;
			break;
		}
		from++;
	}
	return(from);
}

const char *
MFunctionParser::asm_skipto(const char *from, short c) const
{							/* Skip all characters until we reach a char c */
	while (*from) {
		if (*from == c) {
			from++;
			break;
		}
		if (*from == '\\' && from[1] != 0)
			from++;
		from++;
	}
	return(from);
}

/************************************************************************/
/*	Purpose..:	Return the position of the first character in line		*/
/*	Input....:	pointer to char position in line						*/
/*	Input....:	pointer to first char in buffer 						*/
/*	Return...:	pointer to first character in line						*/
/************************************************************************/

const char *
MFunctionParser::NE_LineStart(const char *text_pos, const char *buffer_start) const
{
	while (text_pos > buffer_start && * (text_pos -1) != EOL_CHAR)
		text_pos--;
	return(text_pos);
}

/******************************************************************************/
/* Purpose..: finds the size of an identifier							 	  */
/* Input....: pointer to text at first char of identifier					  */
/* Returns..: length of identifier											  */
/******************************************************************************/

char 
MFunctionParser::getIdentSize(const char* pStartChar) const
{
	short n = 1;
	const char* myPtr = pStartChar;
	
	// Find beginning of this identifier. This is the classname
	while( isident( myPtr[0]) && (n<255)) { myPtr++; n++;}
	
	return( n-1);
}

/******************************************************************************/
/* Purpose..: Searches backwards for the beginning of the next identifier	  */
/* Input....: pointer to text					  							  */
/* Returns..: positions pointer at the start of the previous identifier		  */
/******************************************************************************/

const char* 
MFunctionParser::findPrevIdent(const char* pStartChar) const
{

	// skip white space to end of next identifier
	pStartChar = skipCCommentsBack( pStartChar);
	
	// Find tail end of previous identifier
	while( ( *pStartChar != '\0') && !isident( pStartChar[0]) ) { 
		pStartChar--;
		pStartChar = skipCCommentsBack( pStartChar);
	}
	
	// Find the beginning of this identifier
	while( (*pStartChar != '\0') && isident( pStartChar[0]) ) { pStartChar--;}
	
	return( pStartChar+1);
}

/******************************************************************************/
/* Purpose..: looks for a character within a range			 	  			  */
/* Input....: start and end pointers										  */
/* Returns..: ptr to found char or to NULL									  */
/******************************************************************************/

const char* 
MFunctionParser::findChar( const char* start, const char* end, char findChar) const
{
	const char* foundAt = start;	
	
	while( foundAt <= end) {
		foundAt = skipCComments( foundAt); 
		if ( *foundAt == findChar)
			return( foundAt);
		foundAt++;
	}
	return( NULL);
}

/******************************************************************************/
/* Purpose..: Skips all white space and comments in C/C++ code going backwards*/
/* Input....: pointer to text												  */
/* Returns..: ptr to the next non-white char that's not part of a comment	  */
/******************************************************************************/

const char* 
MFunctionParser::skipCCommentsBack (const char *from) const
{
	do {
		// skip white space
		while (isspace (*from))
			from--;
		
		// is this a comment ?
		if (from[0] == '/')
		{
			if (from[-1] == '*') // C-style comment;  skip it
			{
				from-=2;
				do
				{
					while( *from != '*') from--;
					
					if (*(from -1) == '/') // this is the end of the comment.
					{
						from-=2;
						break;
					}
					from--;
				} while (*from != '\0');
			}
			else // The slash isn't part of a comment, so we break.
				break;
		}
		else // This is a non-white character that isn't a slash, break.
			break;

	} while (*from != '\0');

	return (from);
}

/******************************************************************************/
/* Purpose..: finds end of previous statement, method, or class			 	  */
/* Input....: pointer to text												  */
/* Returns..: ptr to last character of previous statement					  */
/******************************************************************************/

const char* 
MFunctionParser::findPrevStatement( const char* startAt) const
{
	const char* curr = startAt;
	
	while (*curr != '\0') { 
		curr = skipCCommentsBack( curr);
		if ( ( *curr == ';') ||
			 ( *curr == '}') ||
			 ( *curr == '{')) {
			// end of prev statement
			return (curr);
		}
		curr--;
	}
	return ( startAt);
}	
