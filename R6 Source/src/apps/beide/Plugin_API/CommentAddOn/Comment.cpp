//-----------------------------------------------------------------------
//	Comment.cpp			Copyright © 1996 Metrowerks Inc.
//-----------------------------------------------------------------------
//	Code for a simple BeIDE Editor add-on.  
//	Select some text, choose the add-on, and the text
//	will have C++ comments added to the beginning of the lines.
//	If you hold down the shift key when choosing this add-on
//	any '//' at the beginning of lines in the selection will be
//	removed.
//	These editor add-ons must reside in a folder: 'plugins/Editor_add_ons'
//	that is in the same folder as BeIDE.
//	The add-ons are run in the text window's thread and shouldn't put up
//	windows (except possibly alerts).  In general they should perform
//	a task and exit.  
//	The current interface is preliminary and may change in future releases.
//	There is currently no support for undo of an action accomplished by
//	an add-on.

#include "MTextAddOn.h"

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

//#pragma export on
extern "C" {
_EXPORT status_t perform_edit(MTextAddOn *addon);
}
//#pragma export reset

// Private prototypes
long AddComments(MTextAddOn *addon);
long RemoveComments(MTextAddOn *addon);

const unsigned char RETURN_CHAR = 10;


/* Public interface for the add-on */
long
perform_edit(
	MTextAddOn *addon)
{
	if ((modifiers() & B_SHIFT_KEY) == 0)
		return AddComments(addon);
	else
		return RemoveComments(addon);
}

/* Create a block comment */
long
AddComments(
	MTextAddOn *addon)
{
	//  error checking
	
	long	selStart;
	long	selEnd;
	addon->GetSelection(&selStart, &selEnd);
	if (selEnd <= selStart)
		return B_ERROR;
	
	//  set-up
	
	const char *txt = addon->Text();
	const char *ptr = txt + selStart;
	const char *end = txt + selEnd;
	int			nls = 1;
	
	//  count number of lines affected (always at least one)
	
	while (ptr < end-1) { // ignore if last line is complete or not
		if (*ptr == RETURN_CHAR)
			nls++;
		ptr++;
	}
	
	//  perform operation into temporary buffer
	
	char *new_text = new char[selEnd-selStart+nls*2+1];
	char *out = new_text;
	int crflag = 1;
	for (ptr=txt + selStart; ptr<end; ptr++) {
		if (crflag) { // we use C++ comments
			*(out++) = '/';
			*(out++) = '/';
		}
		*(out++) = *ptr;
		crflag = (*ptr == RETURN_CHAR);
	}
	*out = 0;
	
	//  remove old text, replace with new, adjust selection
	
	addon->Delete();
	addon->Insert(new_text);
	delete[] new_text;
	addon->Select(selStart, selEnd+nls*2);
	
	return B_NO_ERROR;
}

/* Remove a block comment */
long
RemoveComments(
	MTextAddOn *addon)
{
	//  error checking

	long	selStart;
	long	selEnd;
	addon->GetSelection(&selStart, &selEnd);
	if (selEnd <= selStart)
		return B_ERROR;
	
	//  set-up
	
	const char *txt = addon->Text();
	const char *ptr = txt + selStart;
	const char *end = txt + selEnd;
	
	//  perform operation into new buffer
	//  we know the buffer will always shrink or stay the same size
	
	char *new_text = new char[selEnd-selStart+1];
	char *out = new_text;
	int crflag = 1;
	int del = 0;
	while (ptr < end) {
		if (crflag && ptr <= end-2) { // delete C++ comment
			if ((ptr[0] == '/') && (ptr[1] == '/')) {
				ptr += 2;
				del += 2;
			}
		}
		crflag = (*ptr == RETURN_CHAR);
		*(out++) = *(ptr++);
	}
	*out = 0;
	
	//  replace old text with new text, update selection
	
	addon->Delete();
	addon->Insert(new_text);
	delete[] new_text;
	addon->Select(selStart, selEnd-del);
	
	return B_NO_ERROR;
}
