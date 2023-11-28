// disp ((struct zzzCall*)jsecontext)->session.GlobalVariable->userCount
// p ((struct zzzCall*)mContextWrapper->mContext)->session.GlobalVariable->userCount

/* sebrowse.c
 */

/* (c) COPYRIGHT 1993-98				NOMBAS, INC.
 *											64 SALEM ST.
 *											MEDFORD, MA 02155	USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.	This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

/*
 * Written 12/02/97 - 12/08/97	Richard Robinson
 *			12/19/98				 Richard Robinson, updated to C
 *			10/23/98 - 11/05/98	Richard Robinson, rewrite
 *											(with interruption for vacation)
 *
 *
 * Netscape-compatible object stuff. Routines are provided
 * allow a browser to expose objects in its Javascript like
 * Netscape does.
 *
 *
 * This version assumes a single-threaded browser (as all browsers
 * our customers have to date are.) This assumption, along with
 * the experience of solving customer problems, will make this
 * version much faster, though it can use a bit more memory. I think
 * the new, non-dynamic design is also far clearer and easier for
 * you (the customer) to understand. The old version was implemented
 * the way it was to be more generic than it had to be.
 *
 * 
 * Adding it to your browser
 *
 *			-You must be including the ECMA objects in your project.
 *			See the API manual for information on this.
 *
 *			-define JSE_BROWSEROBJECTS in your jseopt.h.
 *
 *			-define JSE_MULTIPLE_GLOBAL in your jseopt.h
 *
 *			-your MayIContinue() function can pop up a dialog
 *			each 1,000,000 calls (saying the program is eating
 *			a lot of processor time and should it be terminated?)
 *			This is the internet explorer's behavior, though Netscape
 *			appears not to do this. It is up to you.
 *
 *			-Initialize a new jseContext to hold your browser window
 *			hierarchy.
 *
 *			-after creating the context, call browserGeneralInfo() on it.
 *			-call browserAddMimeType() to add the mime types the browser
 *			knows how to handle
 *			-call browserAddPlugin() to add the plugins
 *
 *			-Call 'browserInitWindow()' whenever your browser pops
 *			up a new window. Do this for all windows and frames.
 *			The order doesn't matter as a placeholder for any window
 *			referred to is filled in when you call this routine for this
 *			window.
 *
 *			-do Javascript using browserInterpret() and browserCallFunction().
 *
 *			-do calls to browserUpdate() as needed when browser information
 *			changes outside control of the script.
 *
 *			-During runtime as windows close, you should call
 *			'browserTermWindow()' on it. This applies to your browserCloseWindow()
 *			routine as well (you must explicitly close the window in it.)
 *
 *
 *			-destroy the context when you are done. Right before destroying the
 *			context, call browserCleanup().
 *
 *
 *
 *			-after you have modified your program as above, make sure you
 *			compile and link this file with your program.
 *
 *			-you need to add all of the functions that this program calls.
 *			They represent the communication between the Javascript objects
 *			and your browser. In the companion file, 'sebrowse.h', you should
 *			search and find this text:
 *
 *	* The following functions you must provide! Each should be pretty
 *
 *
 *			all following text is a series of function prototypes
 *			along with a short description of what the function does. You
 *			must write this function and link it with your program. For
 *			example, the first prototype reads:
 *
 *
 *	struct BrowserWindow *browserGetTopWindow(jseContext jsecontext,
 *															struct BrowserWindow *current_window);
 *
 *			you must therefore write a function which returns a magic
 *			BrowserWindow cookie representing the top window your browser
 *			is displaying. It might look like this:
 *
 *
 *	struct BrowserWindow *browserGetTopWindow(jseContext jsecontext,
 *															struct BrowserWindow *current_window)
 *	{
 *	 return (struct BrowserWindow *)(my_global_window_list.top_window);
 *	 //alternately it might be:
 *	 //return (struct BrowserWindow *)((struct Mywindow *)current_window)->top;
 *	}
 *
 *			Some functions may take more code to write than others. All functions
 *			are passed the current context they are executing in, for you may
 *			have many uses for it (such as to store/retrieve data.)
 *
 *			Please read the comments in the accompanying header file "sebrowse.h"
 *			for information on the magic cookies, data structures, and
 *			functions involved. Also, make sure you have a Javascript
 *			reference (I've been using Javascript: The Definitive Guide 2nd edition)
 *			as I have not duplicated information that can be found there in
 *			many cases (For example, the 'struct Image' represents the fields
 *			found in the Image object. You should look up Image in a
 *			Javascript book for an extensive description of each one.)
 *
 *
 * What is here?
 *
 *			This file implements the Netscape browser objects as defined
 *			for Netscape 3.0 with the exception of anything taint-related
 *			and anything Java-related. In addition, there are no security
 *			blocks - if you decide you think some data should not be
 *			allowed, then don't give it when asked for (for example, you
 *			might want to only let 'window.document.location' be read
 *			if the Javascript is executing in that window.)
 *
 *
 *			As of this writing, this code is completely untested. I don't
 *			have a browser to plug it into and test it. It has been compiled.
 *			This makes the code more a framework or sample for you to use.
 *			Customer suggestions and fixes have been implemented from past
 *			versions, but this is a new version.
 *
 *
 *			Search on "NOTE:" for areas that may need your attention.
 */


#include <new.h>

#include "jseopt.h"
#include "seall.h"
#include "sebrowse.h"
extern "C" {
	#include "globldat.h"
	//seb 98.11.14 -- Added this to load up the ECMA library.
	#include "seliball.h"
	#include "seecma.h"
}


#if defined(JSE_DEBUGGABLE)
   struct debugMe * debugme;
#endif

struct zzzAllVarmem { /* all varmem types share this data in the same place */
   uint userCount;         /* how many variables are using this */
//   VarType dataType;       /* what variable type represented here */
//   uword8 attributes;      /* variable attributes */
};

struct zzzVarmem
{
   /* Union for data access. The same structure is copied at the
    * beginning of each in the *exact same location* for each so that we can
    * swith between fast mempool or least-used memory
    */
   union {
      struct zzzAllVarmem vall; /* VUndefined, VNull, or generic checking */
#if 0
      struct {
         struct zzzAllVarmem vall;
         jsenumber value;  /* VBoolean, VNumber */
      } vnumber;
      struct {
         struct zzzAllVarmem vall;
         struct Varobj members;
         struct Function *function;     /* !=NULL for an object */
#        if (0!=JSE_CYCLIC_GC)
            struct Varmem **gcPrevPtr; /* links in varmem chain */
            struct Varmem *gcNext;
#        endif
         uword8 been_here;         /* many recursive calls that must not
                                    * redo the same varmem twice. */
      } vobject;
      struct {
         struct AllVarmem vall;
         void _HUGE_ *memory;           /* actually memory for string/buffer */
         JSE_POINTER_UINDEX alloced;    /* space available */
         JSE_POINTER_UINDEX count;      /* number of items used */
         JSE_POINTER_SINDEX zeroIndex;  /* for 0 < ArrayDimension */
      } vpointer;
#endif
   } data;
};


struct zzzVar
{
   struct zzzVarmem *varmem;
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      ubyte cookie;
#  endif
   uint userCount;    /* Number of current users of this structure */

   struct {
      void *parentObject;
      const char* memberName;
   } reference;
};

struct zzzSession_
{
#  if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
      void *SecurityGuard;
#  endif
   zzzVar * GlobalVariable;
};

struct zzzCall
{
   void *Global;
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      ubyte cookie;
#  endif
   struct zzzSession_ session;
};

// From var.h.  We need this.
struct Var;
typedef struct Var VarRead;
extern "C" VarRead * NEAR_CALL reallyGetReadableVar(struct Var *t,struct Call *call);

#define VAR_HAS_DATA(V)       (NULL!=((struct zzzVar*)V)->varmem) /* can read and write to this var */
#define varAddUser(This)      (((struct zzzVar*)This)->userCount++)
#define GET_READABLE_VAR(VAR,CALL) \
   ( VAR_HAS_DATA(VAR) ? (varAddUser(VAR),(VAR)) : reallyGetReadableVar(VAR,CALL) )


// This is a handy utility class that wraps a jseVariable.	When the class is
// destroyed, it calls jseDestroyVariable on its wrapped variable.	If we use
// this class instead of using the tempVar space that ScriptEase provides, we
// will prevent cluttering the tempVar space and speed things up.
class VarWrapper {
public:
				VarWrapper() : mContext(0), mVar(0) {}
				VarWrapper(jseContext context, jseVariable var) : mContext(context), mVar(var) {}
				VarWrapper(jseContext context, jseVariable obj, const jsechar *name, jseDataType t);
				~VarWrapper();
				operator jseVariable()		{return mVar;}
	void		operator=(jseVariable var);

	jseVariable	Set(jseContext context, jseVariable obj, const jsechar *name, jseDataType t);
	jseVariable Set(jseContext context, jseVariable to);
	void		Clear();
private:
	jseContext mContext;
	jseVariable mVar;
};

#if defined(JSE_BROWSEROBJECTS)

/* ---------------------------------------------------------------------- */

#define WINDOW_PROP	"[[BrowserWindowAddress]]"
#define LOCATION_PROP "[[BrowserLocationAddress]]"
#define DOCUMENT_PROP "[[BrowserDocumentAddress]]"
#define IMAGE_PROP	 "[[BrowserImageAddress]]"
#define FORM_PROP		"[[BrowserFormAddress]]"
#define ELEMENT_PROP	"[[BrowserElementAddress]]"
#define LAYER_PROP		"[[BrowserLayerAddress]]"


#define BROWSER_WINDOW_NAME "__browserWindow%x"

#define BROWSER_UPDATE_ROUTINE "browserUpdate"

// seb 99.2.3 -- Be more lenient about converting various things to strings.
// Instead of using JSE_VN_STRING in various places, use this macro to allow
// conversion of anything to a string

#define CONVERT_ANY_TO_STRING JSE_VN_CONVERT(JSE_VN_ANY, JSE_VN_STRING) | JSE_VN_COPYCONVERT


static void *get_object(jseContext jsecontext,jsechar *prop)
{
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	void *ret = (void *)
	 jseGetLong(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,thisvar,prop,jseTypeNumber,jseCreateVar)));

	return ret;
}

#define get_my_object(t,p) ((struct t *)get_object(jsecontext,p))


/* ---------------------------------------------------------------------- */


// seb 98.11.12 -- Made these static.
static jseVariable browserCreateLocationObject(jseContext jsecontext,
													 struct BrowserLocation *loc);

static jseVariable browserCreateHistoryObject(jseContext jsecontext,
													struct BrowserWindow *win);

// seb 98.11.12 -- Corrected mistyping of "BrowerDocument"
static jseVariable browserCreateDocumentObject(jseContext jsecontext,
													 struct BrowserDocument *doc);

static jseVariable browserCreateFormArray(jseContext jsecontext,
												struct BrowserDocument *document);

static jseVariable browserCreateElementArray(jseContext jsecontext,
													struct BrowserForm *form,
													jseVariable container_form);
static jseVariable browserCreateImageArray(jseContext jsecontext,
												struct BrowserDocument *document);

static jseVariable browserCreateOptionsArray(jseContext jsecontext,
													struct BrowserElement *elem,
													ulong num_options,
													jseVariable parent);

static jseVariable browserCreateMimeObject(jseContext jsecontext,struct SEMimeType *type,
												jseVariable referer);
// seb 98.12.28 -- Added this.
static jseVariable browserCreateImageObject(jseContext jsecontext,
															struct BrowserImage *image);


// seb 98.11.12 -- Added the following prototypes:
static void browserSetUpWindowObject(jseContext jsecontext,jseVariable where,
												 struct BrowserWindow *window);

static void browserSetUpLocationObject(jseContext jsecontext,jseVariable where,
													struct BrowserLocation *loc);

static void browserSetUpHistoryObject(jseContext jsecontext,
													jseVariable where,
													struct BrowserWindow *win);

static void browserSetUpDocumentObject(jseContext jsecontext,
													jseVariable where,
													struct BrowserDocument *doc);

static void browserSetUpFormObject(jseContext jsecontext,
												jseVariable where,
												struct BrowserForm *form);

static void browserSetUpElementObject(jseContext jsecontext,
													jseVariable where,
													struct BrowserElement *elem,
													jseVariable container_form);
													
static void browserSetUpLayerObject(jseContext jsecontext,
												jseVariable where,
												struct BrowserLayer *layer);

jseVariable browserWindowObject(jseContext jsecontext,struct BrowserWindow *window);


// seb 99.1.23 -- Add the parent in browserSetUpElementOption to get around dynamic put.
static void browserSetUpElementOption(jseContext jsecontext,
													jseVariable where,
													struct BrowserElement *elem,
													struct SEOption *opt,
										jseVariable parent);

static void browserSetUpImageObject(jseContext jsecontext,
												jseVariable where,
												struct BrowserImage *image);

static jseLibFunc(OptionArrayput);

/* ---------------------------------------------------------------------- */


// seb 98.11.16 -- A useful utility function.
static void DumpObject(jseContext jsecontext, jseVariable where, int numTabs, int maxRecursion,
				jseVariable old1, jseVariable old2, jseVariable old3, jseVariable old4,
				jseVariable old5, jseVariable old6, jseVariable old7, jseVariable old8,
				jseVariable put1, jseVariable put2, jseVariable put3, jseVariable put4,
				jseVariable put5, jseVariable put6, jseVariable put7, jseVariable put8)
{
	int j;
	jseVariable prev = NULL;
	jseVariable put = NULL;
	const char *name;
	jseDataType type;
	ulong tmp;

	if (!where)
		return;

	if (!numTabs) {
		for (j = 0; j < numTabs; j++) printf("	");
		printf("---\n");
	}
	do {
		prev = jseGetNextMember(jsecontext, where, prev, &name);
		if (prev) {
			type = jseGetType(jsecontext, prev);
			switch(type) {
				case jseTypeNumber:
					for (j = 0; j < numTabs; j++) printf("	");
					printf("0x%x %s\tLong\t0x%x %ld\n", (unsigned int)prev, name, (unsigned int)jseGetLong(jsecontext, prev), jseGetLong(jsecontext, prev));
					break;
				case jseTypeString:
					for (j = 0; j < numTabs; j++) printf("	");
					printf("0x%x %s\tString\t%s\n", (unsigned int)prev, name, jseGetString(jsecontext, prev, &tmp));
					break;
				case jseTypeObject:
					if (strcmp(name, "_put") == 0) {
						put = prev;
						if (put && (put == put1 || put == put2 || put == put3 || put == put4 ||
						 			put == put5 || put == put6 || put == put7 || put == put8)) {
							for (j = 0; j < numTabs; j++) printf("	");
							printf("Got _put 0x%x, Skipping recursion\n", (unsigned int)put);
							return;
						}
					}
					for (j = 0; j < numTabs; j++) printf("	");
					if (prev == where || prev == old1 || prev == old2 || prev == old3 || prev == old4
										|| prev == old5 || prev == old6 || prev == old7 || prev == old8) {
						printf("0x%x %s\tObject\tRecursive reference!\n", (unsigned int)prev, name);
					} else {
						if (strcmp(name, "_prototype") == 0 || strcmp(name, "__parent__") == 0 ||
							strcmp(name, "self") == 0 || strcmp(name, "opener") == 0)
							printf("0x%x %s\tObject\tSkipping recursion\n", (unsigned int)prev, name);
						else if (numTabs < maxRecursion) {
							printf("0x%x %s\tObject\n", (unsigned int)prev, name);
							DumpObject(jsecontext, prev, numTabs + 1, maxRecursion,
										where, old1, old2, old3, old4, old5, old6, old7,
										put,		put1, put2, put3, put4, put5, put6, put7);
						}
					}
					break;
				case jseTypeUndefined:
					for (j = 0; j < numTabs; j++) printf("	");
					printf("0x%x %s\tUndefined\n", (unsigned int)prev, name);
					break;
				case jseTypeNull:
					for (j = 0; j < numTabs; j++) printf("	");
					printf("0x%x %s\tNull\n", (unsigned int)prev, name);
					break;
				case jseTypeBoolean:
					for (j = 0; j < numTabs; j++) printf("	");
					printf("0x%x %s\tBoolean %ld\n", (unsigned int)prev, name, (long)jseGetBoolean(jsecontext, prev));
					break;
				case jseTypeBuffer:
					for (j = 0; j < numTabs; j++) printf("	");
					printf("0x%x %s\tBuffer\n", (unsigned int)prev, name);
					break;
				default:
					for (j = 0; j < numTabs; j++) printf("	");
					printf("0x%x %s\tType %ld\n", (unsigned int)prev, name, (long)type);
					break;
			}
		}
	} while (prev);
	if (!numTabs) {
		for (j = 0; j < numTabs; j++) printf("	");
		printf("---\n");
	}
}


void DumpVariable(jseContext jsecontext, jseVariable what, int maxRecursion)
{
	DumpObject(jsecontext, what, 0, maxRecursion,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

void DumpContextUserCount(jseContext jsecontext)
{
	printf("Context 0x%x userCount %d", (unsigned int)jsecontext,((struct zzzCall*)jsecontext)->session.GlobalVariable->userCount);
}


VarWrapper::VarWrapper(jseContext context, jseVariable obj, const jsechar *name, jseDataType t)
	: mContext(0), mVar(0)
{
	Set(context, obj, name, t);
}

VarWrapper::~VarWrapper()
{
	Clear();
}

jseVariable VarWrapper::Set(jseContext context, jseVariable obj, const jsechar *name, jseDataType t)
{
	if (mContext && mVar)
		jseDestroyVariable(mContext, mVar);
	
	mContext = context;
	
	mVar = jseMemberEx(context,obj,name,t, jseCreateVar);
	jseSetAttributes(context,mVar,(jseVarAttributes)
                    (jseGetAttributes(context,mVar)&~jseReadOnly)); /* make sure not readonly */
	if( t!=jseGetType(context,mVar)) jseConvert(context,mVar,t);
	return mVar;
}

jseVariable VarWrapper::Set(jseContext context, jseVariable to)
{
	if (mContext && mVar)
		jseDestroyVariable(mContext, mVar);
	
	mContext = context;
	
	mVar = to;
	return mVar;
}

void VarWrapper::Clear()
{
	if (mContext && mVar)
		jseDestroyVariable(mContext, mVar);
#ifdef DEBUGMENU
	if (mContext && ((struct zzzCall*)mContext)->session.GlobalVariable->userCount <= 0)
		printf("Oops!  Have completely dereferenced globals.\n");
#endif
	mVar = 0;
}

void VarWrapper::operator=(jseVariable var)
{
#ifdef DEBUGMENU
	if (!mContext)
		printf("Invalid VarWrapper::operator=\n");
#endif
	Clear();
	mVar = var;
}

void AssignString(jseContext jsecontext, jseVariable where, const jsechar *member, const jsechar *value,
	jseVarAttributes valueAttrs, jseVarAttributes memberAttrs)
{
	VarWrapper v(jsecontext, jseCreateVariable(jsecontext,jseTypeString));
	jsePutString(jsecontext,v,value);

	VarWrapper tmp(jsecontext,jseMemberEx(jsecontext,where,member,jseTypeString,jseCreateVar));
	jseAssign(jsecontext,tmp,v);
	jseSetAttributes(jsecontext,v,valueAttrs);
	jseSetAttributes(jsecontext,tmp,memberAttrs);
}

void AssignLong(jseContext jsecontext, jseVariable where, const jsechar *member, const int value,
	jseVarAttributes valueAttrs, jseVarAttributes memberAttrs)
{
	VarWrapper v(jsecontext, jseCreateVariable(jsecontext,jseTypeNumber));
	jsePutLong(jsecontext,v,value);

	VarWrapper tmp(jsecontext,jseMemberEx(jsecontext,where,member,jseTypeUndefined,jseCreateVar));
	jseAssign(jsecontext,tmp,v);
	jseSetAttributes(jsecontext,v,valueAttrs);
	jseSetAttributes(jsecontext,tmp,memberAttrs);
}

void AssignWrapper(jseContext jsecontext, jseVariable where, const jsechar *name, int num, jseLibraryFunction funcPtr)
{
/*
	jseDestroyVariable(jsecontext,
			jseMemberWrapperFunction(jsecontext,where,name,
									 funcPtr,num,num,jseDontEnum | jseDontDelete,
									 jseFunc_Secure,NULL));
*/
			jseMemberWrapperFunction(jsecontext,where,name,
									 funcPtr,num,num,jseDontEnum | jseDontDelete,
									 jseFunc_Secure,NULL);
}

/* Call the 'browserUpdate()' function of the given object if it has one.
 */
// seb 99.2.19 -- Added browserWindow object.
void browserUpdate(jseContext jsecontext,struct BrowserWindow *window,jseVariable var_to_update)
{
	jseVariable func;
	jseStack stack;
	
	// seb 99.2.19 -- Added the swapping in of the new global variable.	I think we need it.
	jseVariable oldglob = jseGlobalObject(jsecontext);
	VarWrapper newglob(jsecontext, browserWindowObject(jsecontext,window));
	VarWrapper readablenewglob(jsecontext, GET_READABLE_VAR(((jseVariable)newglob), jsecontext));
	
	/* swap in the new window as the global object effectively running
	 * the script in that window. All window's prototypes point back to
	 * the real global object so functions and objects will be found as
	 * normal.
	 */
	jseSetGlobalObject(jsecontext,readablenewglob);

	func = jseGetMember(jsecontext,var_to_update,BROWSER_UPDATE_ROUTINE);
	if( func!=NULL )
	{
		stack = jseCreateStack(jsecontext);
		jseCallFunction(jsecontext,func,stack,NULL,var_to_update);
		jseDestroyStack(jsecontext,stack);
	}

	jseSetGlobalObject(jsecontext,oldglob);
}


/* ----------------------------------------------------------------------
 * Some simple utility functions for doing the string encasing
 * ---------------------------------------------------------------------- */


/* Encases the string thisvar in a special string that is passed */
static void encase_formatted(jseContext jsecontext,char *format)
{
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	jseVariable ret;
	jseVariable val =
		jseCreateConvertedVariable(jsecontext,jseFuncVar(jsecontext,0),jseToString);

	ulong size;
	const jsechar *str = jseGetString(jsecontext,
												jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeString),
												&size);
	jsechar *buffer = jseMustMalloc(jsechar,
		sizeof(jsechar)*(1+strlen(format)+size+jseGetArrayLength(jsecontext,val,NULL)));
	sprintf(buffer,format,jseGetString(jsecontext,val,NULL),
				str);
	ret = jseCreateVariable(jsecontext,jseTypeString);
	jsePutString(jsecontext,ret,buffer);
	jseReturnVar(jsecontext,ret,jseRetTempVar);

	jseMustFree(buffer);

	jseDestroyVariable(jsecontext,val);
}


static void encase_generic(jseContext jsecontext,char *tag)
{
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	ulong size;
	const jsechar *str = jseGetString(jsecontext,
												jseMember(jsecontext,thisvar,VALUE_PROPERTY,jseTypeString),
												&size);
	jsechar *buffer = jseMustMalloc(jsechar,sizeof(jsechar)*(100+size));
	jseVariable ret;
	sprintf(buffer,"<%s>%s</%s>",tag,str,tag);
	ret = jseCreateVariable(jsecontext,jseTypeString);
	jsePutString(jsecontext,ret,buffer);
	jseReturnVar(jsecontext,ret,jseRetTempVar);

	jseMustFree(buffer);
}


/* the string extensions */


static jseLibFunc(Stringanchor)
{
	encase_formatted(jsecontext,"<A NAME=\"%s\">%s</A>");
}

static jseLibFunc(Stringbig)
{
	encase_generic(jsecontext,"BIG");
}

static jseLibFunc(Stringblink)
{
	encase_generic(jsecontext,"BLINK");
}

static jseLibFunc(Stringbold)
{
	encase_generic(jsecontext,"BOLD");
}

static jseLibFunc(Stringfixed)
{
	encase_generic(jsecontext,"TT");
}

static jseLibFunc(Stringfontcolor)
{
	encase_formatted(jsecontext,"<FONT COLOR=\"%s\">%s</FONT>");
}

static jseLibFunc(Stringfontsize)
{
	encase_formatted(jsecontext,"<FONT SIZE=\"%s\">%s</FONT>");
}

static jseLibFunc(Stringitalics)
{
	encase_generic(jsecontext,"I");
}

static jseLibFunc(Stringlink)
{
	encase_formatted(jsecontext,"<A HREF=\"%s\">%s</A>");
}

static jseLibFunc(Stringsmall)
{
	encase_generic(jsecontext,"SMALL");
}

static jseLibFunc(Stringstrike)
{
	encase_generic(jsecontext,"STRIKE");
}

static jseLibFunc(Stringsub)
{
	encase_generic(jsecontext,"sub");
}

static jseLibFunc(Stringsup)
{
	encase_generic(jsecontext,"sup");
}


/* ---------------------------------------------------------------------- *
 * window object and its dynamic put routine
 *
 * We keep a list of them in the global object (DontEnum/ReadOnly/DontDelete)
 * so when any function is run, we know its BrowserWindow and can find its
 * appropriate window_obj
 * ---------------------------------------------------------------------- */

/* Get the window object associated with your magic cookie.
 */
jseVariable browserWindowObject(jseContext jsecontext,struct BrowserWindow *window)
{
	char buffer[64];
	
	sprintf(buffer,BROWSER_WINDOW_NAME,(unsigned int)window);
	/* this is because it could already exist as a placeholder previously created.
	 * We want to update the object so that all the old links to it are still
	 * valid.
	 */
	return jseMemberEx(jsecontext,NULL,buffer,jseTypeObject,jseCreateVar);
}


/* Use new Function() to add this, then add jseImplicitThis,jseImplicitParents,
 * also store it in the given container. Because the given function has these
 * flags set, it will find its variable and those of its containing objects, and
 * with JSE_MULTIPLE_GLOBAL set, the correct global object will be swapped in
 * when it is run. Thus, it can be called with jseCallFunction() and no additional
 * changes.
 */
jseVariable browserEventHandler(jseContext jsecontext,const jsechar *eventname,
											struct BrowserWindow *window,
											const jsechar *event_source_txt,
											jseVariable container)
{
//printf("browserEventHandler  ");
//DumpContextUserCount(jsecontext);
	jseVariable oldglob = jseGlobalObject(jsecontext);
	VarWrapper newglob(jsecontext, browserWindowObject(jsecontext,window));
	VarWrapper readablenewglob(jsecontext, GET_READABLE_VAR(((jseVariable)newglob), jsecontext));
	jseVariable ret,param,func;
	jseStack stack;
{	
	
	stack = jseCreateStack(jsecontext);
	param = jseCreateVariable(jsecontext,jseTypeString);
	jsePutString(jsecontext,param,event_source_txt);
	jsePush(jsecontext,stack,param,True); /* delete when popped */

	func = jseGetMemberEx(jsecontext,NULL,"Function",jseCreateVar);
	if( func==NULL )
	{
		/* ECMA objects not there, a browser needs the ECMA objects */
		return NULL;
	}

	/* swap in the new window as the global object effectively running
	 * the script in that window. All window's prototypes point back to
	 * the real global object so functions and objects will be found as
	 * normal.
	 */
	jseSetGlobalObject(jsecontext,readablenewglob);

	/* it shouldn't fail, perhaps it is not the real ECMA function? */
	if( !jseCallFunction(jsecontext,func,stack,&ret,NULL) )
		ret = NULL;
		
	// seb 98.12.29
	// The return value is gonna get blown away when we destroy the stack.
	// Save off a copy of it.
	ret = jseCreateSiblingVariable(jsecontext, ret, 0);
	
	/* Put back the old global and give the user the result */
	jseSetGlobalObject(jsecontext,oldglob);

	jseDestroyStack(jsecontext,stack);

// seb 98.12.30 - We need to extract the function structure out of the function
// variable we got back and reassign its stored_in field.	This was set to a temporary
// in the ECMA Function constructor that has since been destroyed, and we will crash if
// we try to call this function and we access this now deleted variable.	We'll re-set
// the value to the function's new home as a property in the container.

// seb 99.1.24 - We need to set the global object in the function, too, so it points to the
// window.
	if (ret != NULL) {
		jseSetFunctionParent(jsecontext, container, eventname, ret);
		jseSetFunctionGlobal(jsecontext, readablenewglob, ret);
	}

//	jseDestroyVariable(jsecontext, param);
	jseDestroyVariable(jsecontext, func);

/*
	if (ret != NULL) {
		struct Function* func = varGetFunction(ret, jsecontext);
		jseVariable newloc = jseMember(jsecontext, container, eventname, jseTypeUndefined);
		jseSetAttributes(jsecontext, newloc, jseImplicitThis | jseImplicitParents);
		
		func->stored_in = ret;
		jseAssign(jsecontext, newloc, ret);
	}
*/
//	if( ret!=NULL )
//		jseAssign(jsecontext,
//					 jseMember(jsecontext,container,eventname,jseTypeUndefined),
//					 ret);
	
}
//printf("     done  ");
//DumpContextUserCount(jsecontext);
//printf("\n");

	return ret;
}


// seb 98.12.31 -- Changing to const jsechar *
jseVariable browserInterpret(jseContext jsecontext,struct BrowserWindow *window,
										const jsechar *txt)
{
//printf("browserInterpret  ");
//DumpContextUserCount(jsecontext);
	jseVariable oldglob = jseGlobalObject(jsecontext);
	VarWrapper newglob(jsecontext, browserWindowObject(jsecontext,window));
	VarWrapper readablenewglob(jsecontext, GET_READABLE_VAR(((jseVariable)newglob), jsecontext));
//	jseVariable newglob = GET_READABLE_VAR(browserWindowObject(jsecontext,window), jsecontext);
	jseVariable ret = NULL;
{
	
	/* swap in the new window as the global object effectively running
	 * the script in that window. All window's prototypes point back to
	 * the real global object so functions and objects will be found as
	 * normal.
	 */
	jseSetGlobalObject(jsecontext,readablenewglob);

	/* we don't call main() as that is not traditional browser behavior */
	if( !jseInterpret(jsecontext,NULL,txt,NULL,jseNewNone,JSE_INTERPRET_DEFAULT,
							NULL,&ret) )
		assert( ret==NULL );

	/* Put back the old global and give the user the result */
	jseSetGlobalObject(jsecontext,oldglob);
}
//printf("     done  ");
//DumpContextUserCount(jsecontext);
//printf("\n");

	return ret;
}

void browserCallFunction(jseContext jsecontext, BrowserWindow *window, jseVariable what, jseVariable func)
{
	jseStack stack = jseCreateStack(jsecontext);
	jseVariable ret;
	jseVariable oldglob = jseGlobalObject(jsecontext);
	jseVariable newglob = browserWindowObject(jsecontext,window);
	VarWrapper readablenewglob(jsecontext, GET_READABLE_VAR(((jseVariable)newglob), jsecontext));
	jseSetGlobalObject(jsecontext,readablenewglob);
	jseCallFunction(jsecontext, func, stack, &ret, what);
	jseSetGlobalObject(jsecontext,oldglob);
	jseDestroyStack(jsecontext, stack);
}


void RecursiveDeleteMembers(jseContext jsecontext, jseVariable var)
{
static int ns = 0;
	jseVariable tmp;
	const jsechar *name;
	while ((tmp = jseGetNextMember(jsecontext, var, NULL, &name)) != NULL) {
for (int i = 0; i < ns; i++)
printf("  ");
printf("Trying to delete member %s\n", name);
ns++;
//		if (tmp != var && name && *name && *name != '_')
//			RecursiveDeleteMembers(jsecontext, tmp);
ns--;
		jseDeleteMember(jsecontext, var, name);
	}
}

// seb 98.11.12 -- Added void
void browserTermWindow(jseContext jsecontext,struct BrowserWindow *window)
{
	char buffer[64];
	
	// In browserSetUpWindowObject, we explained how linking the prototype of the window back
	// to the globals added a reference to the global object.  We'll free it now.
	VarWrapper windVar(jsecontext, browserWindowObject(jsecontext,window));
//	jseDeleteMember(jsecontext,windVar,PROTOTYPE_PROPERTY);

	sprintf(buffer,BROWSER_WINDOW_NAME,(unsigned int)window);
printf("Trying to delete window %s from context 0x%x\n", buffer, (unsigned int)jsecontext);
//	RecursiveDeleteMembers(jsecontext, windVar);
	jseDeleteMember(jsecontext,NULL,buffer);
printf("Done trying to delete window\n");
}


// seb 99.2.8 - Added params for isSubview and frame name.	Needed to set up
// frame docs properly.
void browserInitWindow(jseContext jsecontext,struct BrowserWindow *window,
						jsebool isSubframe, const jsechar *frameName,
						int frameNumber, struct BrowserWindow *topWindow)
{
	assert( window_obj!=NULL );

	/* save it */
	VarWrapper place(jsecontext, browserWindowObject(jsecontext,window));
	assert( place!=NULL );
	jseSetAttributes(jsecontext,place,jseDontEnum|jseDontDelete);

	/* and initialize it */
	browserSetUpWindowObject(jsecontext,place,window);

{
char buffer[64];
sprintf(buffer,BROWSER_WINDOW_NAME,(unsigned int)window);
printf("Set up window %s in context 0x%x\n", buffer, (unsigned int)jsecontext);
}


	if (isSubframe) {
			VarWrapper top(jsecontext, browserWindowObject(jsecontext,topWindow));
		jseVariable v = jseMember(jsecontext, top, frameName, jseTypeUndefined);
		jseAssign(jsecontext, v, place);
		jseSetAttributes(jsecontext, v, jseDontDelete);

		jseVariable frames = jseMember(jsecontext, top, "frames", jseTypeUndefined);
// seb 99.2.17 -- We want to use jseIndexMember, not jseGetIndexMember.	We should create a
// new one if it doesn't exist.
		jseAssign(jsecontext, jseIndexMember(jsecontext, frames, frameNumber, jseTypeUndefined), place);
	}

// seb 98.11.16 - Add links to the document and window objects at the top level.
/*
	v1 = jseMember(jsecontext,NULL,"window",jseTypeUndefined);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,place);
	jseSetAttributes(jsecontext,image,jseDontDelete | jseReadOnly);

	v1 = jseMember(jsecontext,NULL,"document",jseTypeUndefined);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,jseMember(jsecontext,place,"document",jseTypeObject));
	jseSetAttributes(jsecontext,image,jseDontDelete | jseReadOnly);
*/
}


static jseVariable browserBuildFrameObject(jseContext jsecontext,
														 struct BrowserWindow *win)
{
	jseVariable obj, pl;
	ulong count;
	struct BrowserWindow *iter;

	
	// seb 98.11.12 -- Added cast to jseVariable
	obj = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);
	count = 0;

	iter = NULL;
	while( (iter = browserGetNextFrame(jsecontext,win,iter))!=NULL )
	{
		/* find this window or create a blank template object for it */
//		value = browserWindowObject(jsecontext,browserGetTopWindow(jsecontext,iter));
// seb 99.2.8 - Don't we want to put this in the window object for this window,
// not the top window?
		VarWrapper value(jsecontext, browserWindowObject(jsecontext, iter));

		BString string = browserGetNameOfWindow(jsecontext,iter);
		pl = jseMember(jsecontext,obj,string.String(),
							jseTypeUndefined);
		jseAssign(jsecontext,pl,value);

		pl = jseIndexMember(jsecontext,obj,count++,jseTypeUndefined);
		jseAssign(jsecontext,pl,value);
	}

	return obj;
}


static jseLibFunc(windowUpdate)
{
	struct BrowserWindow *bw;
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	
	bw = get_my_object(BrowserWindow,WINDOW_PROP);
	browserSetUpWindowObject(jsecontext,thisvar,bw);
}


static jseLibFunc(Windowput)
{
	const jsechar *prop, *text;
	jseVariable url;
	struct BrowserWindow *bw;

			
	prop = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	if( strcmp(prop,"location")==0 )
	{
		/* assigning to location means something like:
		 *	window.location = "http://...";
		 */
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(url,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(url,jsecontext,1,JSE_VN_STRING);
		text = jseGetString(jsecontext,url,NULL);
		browserGotoURL(jsecontext,browserGetLocation(jsecontext,bw),text,False);
	}
	else if( strcmp(prop,"defaultStatus")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(url,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(url,jsecontext,1,JSE_VN_STRING);
		text = jseGetString(jsecontext,url,NULL);
		browserSetDefaultStatus(jsecontext,bw,text);
	}
	else if( strcmp(prop,"status")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(url,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(url,jsecontext,1,JSE_VN_STRING);
		text = jseGetString(jsecontext,url,NULL);
		browserSetStatus(jsecontext,bw,text);
	}


	/* we've notified the browser of any change, make sure to store it in
	 * our own structure tree.
	 */
	jseAssign(jsecontext,jseMember(jsecontext,thisvar,prop,jseTypeUndefined),
				 jseFuncVar(jsecontext,1));
}


static void browserSetUpWindowObject(jseContext jsecontext,jseVariable where,
												 struct BrowserWindow *window)
{
//zzzVar *zzzW = (zzzVar*)GET_READABLE_VAR(((jseVariable)where), jsecontext);
//{
	VarWrapper nproto;
// seb 98.11.15
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	VarWrapper v(jsecontext, jseMemberEx(jsecontext,thisvar,WINDOW_PROP,jseTypeNumber,jseCreateVar));
	VarWrapper v1;
//	jseVariable v = jseMember(jsecontext,where,WINDOW_PROP,jseTypeNumber),v1;
	VarWrapper tmp;
	struct BrowserWindow *opener;
	BString string;
	int x,limit;
	
	/* make sure if we are updating, we turn off the dynamic put so we
	 * can just directly store members without wasting the time to tell
	 * the browser to change to the state its already in for everything.
	 */
	jseDeleteMember(jsecontext,where,PUT_PROPERTY);


	/* store the magic cookie */
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)window);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

// seb 99.2.19 -- I'm going to putting this in both places.	It may work better with
// accessing things inside of framesets.
	v = jseMemberEx(jsecontext,where,WINDOW_PROP,jseTypeNumber,jseCreateVar);
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)window);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);
	
	/* set the prototype to point back to the global so all of its members
	 * end up in the scope chain.
	 */
	// This adds an extra reference to the global variable's varmem structure.  We have
	// to make sure that when we tear down this window object that we unset the prototype
	// object from the global so that this reference is released and the global can be
	// properly freed.  We do this in browserTermWindow.
	v=jseMemberEx(jsecontext,where,PROTOTYPE_PROPERTY,jseTypeUndefined,jseCreateVar);
	nproto.Set(jsecontext,v);
	jseAssign(jsecontext,nproto,jseGlobalObject(jsecontext));
	jseSetAttributes(jsecontext,nproto,jseDontDelete|jseDontEnum|jseReadOnly);


	/* set up some read-only self-referencing properties */
#warning Put_this_back_in
	v = jseMemberEx(jsecontext,where,"self",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v,0);
	jseAssign(jsecontext,v,where);
	jseSetAttributes(jsecontext,v,jseReadOnly | jseDontDelete);

	
#warning Put_this_back_in
	v = jseMemberEx(jsecontext,where,"window",jseTypeUndefined, jseCreateVar);
	jseSetAttributes(jsecontext,v,0);
	jseAssign(jsecontext,v,where);
	jseSetAttributes(jsecontext,v,jseReadOnly | jseDontDelete);


	/* assign top to the 'top' window. Even if it isn't defined yet, we'll make
	 * a placeholder object for it to point to. The browser writer will make sure
	 * to do a browserInitWindow() on it before too long.
	 */
#warning Put_this_back_in
	v = jseMemberEx(jsecontext,where,"top",jseTypeUndefined, jseCreateVar);
	jseSetAttributes(jsecontext,v,0);
	jseAssign(jsecontext,v,VarWrapper(jsecontext, browserWindowObject(jsecontext,
															 browserGetTopWindow(jsecontext,window))));
	jseSetAttributes(jsecontext,v,jseReadOnly | jseDontDelete);

	
	/* ditto for parent */
#warning Put_this_back_in
	v = jseMemberEx(jsecontext,where,"parent",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v,0);
	jseAssign(jsecontext,v,VarWrapper(jsecontext, browserWindowObject(jsecontext,
															 browserGetParentWindow(jsecontext,window))));
	jseSetAttributes(jsecontext,v,jseReadOnly | jseDontDelete);

	
	/* opener is the similar, except it can be NULL (i.e. this window) */
	opener = browserGetOpenerWindow(jsecontext,window);
// seb 99.4.13 -- No, don't do this.  Scripts rely on opener being NULL if there isn't an opener window.
//	if( opener==NULL ) opener = window;
	if (opener) {
		v = jseMemberEx(jsecontext,where,"opener",jseTypeUndefined,jseCreateVar);
		jseSetAttributes(jsecontext,v,0);
		jseAssign(jsecontext,v,VarWrapper(jsecontext, browserWindowObject(jsecontext,opener)));
		jseSetAttributes(jsecontext,v,jseReadOnly | jseDontDelete);
	}


	/* add in the frames. Add also each frame under its name as well as
	 * the number of frames as 'length'.
	 */
#warning Put_this_back_in
//	v=jseMemberEx(jsecontext,where,"frames",jseTypeUndefined,jseCreateVar);
//	jseAssign(jsecontext,tmp.Set(jsecontext,v),
//				 browserBuildFrameObject(jsecontext,window));
	// seb 99.2.16 - We need to get the array length of tmp, not v.
	limit = jseGetArrayLength(jsecontext,tmp,NULL);
	for( x=0;x<limit;x++ )
	{
// seb 99.2.17 -- We want to use jseIndexMember, not jseGetIndexMember.	We should create a
// new one if it doesn't exist.
		VarWrapper namevar(jsecontext, jseIndexMemberEx(jsecontext,tmp,x,jseTypeUndefined,jseCreateVar));
#warning Put_this_back_in
//		jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,namevar,PARENT_PROPERTY,jseTypeUndefined,jseCreateVar)),
//					 where);
		assert( namevar!=NULL );
		v = jseGetMemberEx(jsecontext,namevar,"name",jseCreateVar);
		assert( v!=NULL );
		assert( jseGetType(jsecontext,v)==jseTypeString );
		v = jseGetMemberEx(jsecontext,where,jseGetString(jsecontext,v,NULL),jseCreateVar);
		jseAssign(jsecontext,v,namevar);
	}
	v = jseMemberEx(jsecontext,where,"length",jseTypeNumber,jseCreateVar);
	assert( v!=NULL );
	jseConvert(jsecontext,v,jseTypeNumber);
	jsePutLong(jsecontext,v,limit);

			
	/* default status can be changed so it is not read only. Also needs to be
	 * handled in the dynamic put routine
	 */
	v = jseCreateVariable(jsecontext,jseTypeString);
	string = browserGetDefaultStatus(jsecontext,window);
	jsePutString(jsecontext,v,string.String());
	v1.Set(jsecontext,jseMemberEx(jsecontext,where,"defaultStatus",jseTypeUndefined,jseCreateVar));
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete);


	/* and the name */
	v = jseCreateVariable(jsecontext,jseTypeString);
	string = browserGetNameOfWindow(jsecontext,window);
	jsePutString(jsecontext,v,string.String());
	v1 = jseMemberEx(jsecontext,where,"name",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseReadOnly | jseDontDelete);

	
	/* similar to defaultStatus */
	v = jseCreateVariable(jsecontext,jseTypeString);
	string = browserGetStatus(jsecontext,window);
	jsePutString(jsecontext,v,string.String());
	v1 = jseMemberEx(jsecontext,where,"status",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete);


	/* add in the history, documents, and location */
#warning Put_this_back_in
	v = jseMemberEx(jsecontext,where,"history",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v,0);
	jseAssign(jsecontext,v,
				 VarWrapper(jsecontext,browserCreateHistoryObject(jsecontext,window)));
//	jseAssign(jsecontext,jseMember(jsecontext,v,PARENT_PROPERTY,jseTypeUndefined),where);
//	jseSetAttributes(jsecontext,v,jseReadOnly | jseDontDelete);

	v = jseMemberEx(jsecontext,where,"document",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v,0);
	jseAssign(jsecontext,v,
				 VarWrapper(jsecontext,browserCreateDocumentObject(jsecontext,browserGetDocument(jsecontext,window))));
#warning Put_this_back_in
//	jseAssign(jsecontext,jseMember(jsecontext,v,PARENT_PROPERTY,jseTypeUndefined),where);
//	jseSetAttributes(jsecontext,v,jseReadOnly | jseDontDelete);

#warning Put_this_back_in
	v = jseMemberEx(jsecontext,where,"location",jseTypeUndefined,jseCreateVar);
	jseAssign(jsecontext,v,browserCreateLocationObject(jsecontext,
				browserGetLocation(jsecontext,window)));
//	jseAssign(jsecontext,jseMember(jsecontext,v,PARENT_PROPERTY,jseTypeUndefined),where);
//	jseSetAttributes(jsecontext,v,jseDontDelete);

	
	/* is it closed */
	v = jseCreateVariable(jsecontext,jseTypeBoolean);
	// seb 99.01.10 - Changed to Boolean type.
	jsePutBoolean(jsecontext,v,browserIsWindowClosed(jsecontext,window));
	v1 = jseMemberEx(jsecontext,where,"closed",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseReadOnly | jseDontDelete);

	
	/* add in the browserUpdate() routine. */
#warning Put_this_back_in
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 windowUpdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
//	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,windowUpdate);
																 
	/* Finally, make sure the dynamic put is set. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 Windowput,2,2,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
//	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,Windowput);
//}
//printf("Done\n");
}


/* window routines */


static jseLibFunc(Windowopen)
{
	jseVariable v;
	struct BrowserWindow *bw, *win;
	const jsechar *url, *name, *features;
	jsebool replace = False;
	uint args;

	
//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	url = "";	name = NULL;	features = NULL;
	replace = False;

	args = jseFuncVarCount(jsecontext);
	if( 0 < args )
	{
		url = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
		if ( 1 < args )
		{
			name = jseGetString(jsecontext,jseFuncVar(jsecontext,1),NULL);
			if ( 2 < args )
			{
				features = jseGetString(jsecontext,jseFuncVar(jsecontext,2),NULL);
				if ( 3 < args )
				{
					replace = jseGetLong(jsecontext,jseFuncVar(jsecontext,3));
				}
			}
		}
	}

	win = browserOpenWindow(jsecontext,bw,url,name,features,replace);
	if( win )
	{
		VarWrapper place(jsecontext, browserWindowObject(jsecontext,win));
		assert( place!=NULL );

		v = jseCreateSiblingVariable(jsecontext,place,0);
	}
	else
	{
		v = jseCreateVariable(jsecontext,jseTypeNull);
	}
	jseReturnVar(jsecontext,v,jseRetTempVar);
}


static jseLibFunc(Windowscroll)
{
	jseVariable x, y;
	struct BrowserWindow *bw;

	JSE_FUNC_VAR_NEED(x,jsecontext,0,JSE_VN_NUMBER);
	JSE_FUNC_VAR_NEED(y,jsecontext,1,JSE_VN_NUMBER);
//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserScrollWindow(jsecontext,bw,jseGetLong(jsecontext,x),
								jseGetLong(jsecontext,y));
}


static jseLibFunc(Windowmoveby)
{
	jseVariable x, y;
	struct BrowserWindow *bw;

	JSE_FUNC_VAR_NEED(x,jsecontext,0,JSE_VN_NUMBER);
	JSE_FUNC_VAR_NEED(y,jsecontext,1,JSE_VN_NUMBER);
//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserMoveWindowBy(jsecontext,bw,jseGetLong(jsecontext,x),
								jseGetLong(jsecontext,y));
}

static jseLibFunc(Windowalert)
{
	jseVariable msg;
	const jsechar *txt;
	struct BrowserWindow *bw;

// seb 99.2.3 - Be more lenient about string conversion
	JSE_FUNC_VAR_NEED(msg,jsecontext,0,CONVERT_ANY_TO_STRING);
//	JSE_FUNC_VAR_NEED(msg,jsecontext,0,JSE_VN_STRING);
	txt = jseGetString(jsecontext,msg,NULL);
///	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserDisplayAlertDialog(jsecontext,bw,txt);
}


// seb 99.2.3 - Added this
static jseLibFunc(Windowprompt)
{
	jseVariable msg,ret;
	const jsechar *txt;
	const jsechar *defaultValue;
	struct BrowserWindow *bw;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	JSE_FUNC_VAR_NEED(msg,jsecontext,0,CONVERT_ANY_TO_STRING);
	txt = jseGetString(jsecontext,msg,NULL);

	if( jseFuncVarCount(jsecontext)==2 ) {
		JSE_FUNC_VAR_NEED(msg,jsecontext,1,CONVERT_ANY_TO_STRING);
		defaultValue = jseGetString(jsecontext,msg,NULL);
	} else
		defaultValue = "";

	BString result;
	browserDisplayPromptDialog(jsecontext,bw,txt,defaultValue,&result);
	ret = jseCreateVariable(jsecontext,jseTypeString);
	jsePutString(jsecontext,ret,result.String());
	jseReturnVar(jsecontext,ret,jseRetTempVar);
}


static jseLibFunc(Windowconfirm)
{
	jseVariable msg, ret;
	const jsechar *txt;
	struct BrowserWindow *bw;
	jsebool retval;

// seb 99.2.3 - Be more lenient about string conversion
	JSE_FUNC_VAR_NEED(msg,jsecontext,0,CONVERT_ANY_TO_STRING);
//	JSE_FUNC_VAR_NEED(msg,jsecontext,0,JSE_VN_STRING);
	txt = jseGetString(jsecontext,msg,NULL);
///	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	retval = browserDisplayQuestionDialog(jsecontext,bw,txt);
	ret = jseCreateVariable(jsecontext,jseTypeBoolean);
	// seb 99.01.10 - Changed to Boolean type.
	jsePutBoolean(jsecontext,ret,retval);
	jseReturnVar(jsecontext,ret,jseRetTempVar);
}


static jseLibFunc(Windowblur)
{
	struct BrowserWindow *bw;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserBlurWindow(jsecontext,bw);
}


static jseLibFunc(Windowfocus)
{
	struct BrowserWindow *bw;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserGiveFocusToWindow(jsecontext,bw);
}


static jseLibFunc(Windowsettimeout)
{
	jseVariable code, tm;
	struct BrowserWindow *bw;
	struct BrowserTimeout * tout;
	const jsechar *txt;

// seb 99.2.3 - Be more lenient about string conversion
	JSE_FUNC_VAR_NEED(code,jsecontext,0,CONVERT_ANY_TO_STRING);
//	JSE_FUNC_VAR_NEED(code,jsecontext,0,JSE_VN_STRING);
	JSE_FUNC_VAR_NEED(tm,jsecontext,1,JSE_VN_NUMBER);
	txt = jseGetString(jsecontext,code,NULL);

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	tout = browserSetTimeout(jsecontext,bw,txt,jseGetLong(jsecontext,tm));
	jseReturnLong(jsecontext,(ulong)tout);
}


static jseLibFunc(Windowcleartimeout)
{
	jseVariable tm;
	struct BrowserWindow *bw;

	JSE_FUNC_VAR_NEED(tm,jsecontext,0,JSE_VN_NUMBER);
//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserClearTimeout(jsecontext,bw,(struct BrowserTimeout *)jseGetLong(jsecontext,tm));
}


static jseLibFunc(Windowclose)
{
	struct BrowserWindow *bw;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserCloseWindow(jsecontext,bw);
}


/* ---------------------------------------------------------------------- */
/* Location object and its dynamic put routine									 */
/* ---------------------------------------------------------------------- */


static jseLibFunc(locationUpdate)
{
	struct BrowserLocation *bl;
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	
	bl = get_my_object(BrowserLocation,LOCATION_PROP);
	browserSetUpLocationObject(jsecontext,thisvar,bl);
}


static jseLibFunc(Locationput)
{
	const jsechar *prop, *host, *text;
	jsechar *c;
	jsechar tmp;
	jseVariable txt;
	struct BrowserLocation *bl;
	struct URL url;

	
	prop = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bl = get_my_object(BrowserLocation,LOCATION_PROP);

	// seb 99.1.24 -- Added extra anchorContainer param.
	browserGetLocationValue(jsecontext,bl,&url,NULL);

	if( strcmp(prop,"hash")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		url.hash = jseGetString(jsecontext,txt,NULL);
		browserSetLocationValue(jsecontext,bl,&url);
	}
	else if( strcmp(prop,"host")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		host = (char *)jseGetString(jsecontext,txt,NULL);
		c = strchr(host,':');

		url.hostname = host;
		if( c )
		{
			tmp = *c;
			*c = '\0';
			url.port = c+1;
			browserSetLocationValue(jsecontext,bl,&url);
			*c = tmp;
		} else {
			browserSetLocationValue(jsecontext,bl,&url);
		}
	}
	else if( strcmp(prop,"hostname")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		url.hostname = jseGetString(jsecontext,txt,NULL);
		browserSetLocationValue(jsecontext,bl,&url);
	}
	else if( strcmp(prop,"port")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		url.port = jseGetString(jsecontext,txt,NULL);
		browserSetLocationValue(jsecontext,bl,&url);
	}
	else if( strcmp(prop,"pathname")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		url.pathname = jseGetString(jsecontext,txt,NULL);
		browserSetLocationValue(jsecontext,bl,&url);
	}
	else if( strcmp(prop,"protocol")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		url.protocol = jseGetString(jsecontext,txt,NULL);
		browserSetLocationValue(jsecontext,bl,&url);
	}
	else if( strcmp(prop,"search")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		url.search = jseGetString(jsecontext,txt,NULL);
		browserSetLocationValue(jsecontext,bl,&url);
	}
	else if( strcmp(prop,"href")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		text = jseGetString(jsecontext,txt,NULL);
		browserGotoURL(jsecontext,bl,text,False);
	}
	else if( strcmp(prop,"target")==0 && url.target!=NULL )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		url.target = jseGetString(jsecontext,txt,NULL);
		browserSetLocationValue(jsecontext,bl,&url);
	}

	
	/* we've notified the browser of any change, make sure to store it in
	 * our own structure tree.
	 */
	jseAssign(jsecontext,jseMember(jsecontext,thisvar,prop,jseTypeUndefined),
				 jseFuncVar(jsecontext,1));
}


static jseVariable browserCreateLocationObject(jseContext jsecontext,
																struct BrowserLocation *loc)
{
	jseVariable where;

	
	where = jseCreateVariable(jsecontext,jseTypeObject);
	browserSetUpLocationObject(jsecontext,where,loc);
	return where;
}


static void browserSetUpLocationObject(jseContext jsecontext,jseVariable where,
													struct BrowserLocation *loc)
{
	struct URL url;

	
	/* make sure if we are updating, we turn off the dynamic put so we
	 * can just directly store members without wasting the time to tell
	 * the browser to change to the state its already in for everything.
	 */
	jseDeleteMember(jsecontext,where,PUT_PROPERTY);

	
	VarWrapper v(jsecontext,jseMemberEx(jsecontext,where,LOCATION_PROP,jseTypeNumber,jseCreateVar));
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)loc);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

	VarWrapper proto(jsecontext,jseMemberEx(jsecontext,NULL,"location",jseTypeObject,jseCreateVar));
	proto = jseMemberEx(jsecontext,proto,PROTOTYPE_PROPERTY,jseTypeObject,jseCreateVar);
	VarWrapper nproto(jsecontext,jseMemberEx(jsecontext,where,PROTOTYPE_PROPERTY,jseTypeUndefined,jseCreateVar));
	jseAssign(jsecontext,nproto,proto);
	jseSetAttributes(jsecontext,nproto,jseDontDelete|jseDontEnum|jseReadOnly);

	// seb 99.1.24 -- Added extra anchorContainer param.
	browserGetLocationValue(jsecontext,loc,&url,where);

	/* simply copy all the values from the url structure into our structure
	 * of course turning them into the appropriate ScriptEase variables
	 */
	AssignString(jsecontext,where,"hash",url.hash.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"host",url.host.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"hostname",url.hostname.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"port",url.port.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"pathname",url.pathname.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"protocol",url.protocol.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"search",url.search.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"href",url.href.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"target",url.target.String(),jseDontDelete,0);
	
	/* add in the browserUpdate() routine. */
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 locationUpdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
#warning Dont do this
//	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,locationUpdate);

	/* Last, make sure the dynamic put is set. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 Locationput,2,2,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
//	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,Locationput);
}


static jseVariable browserCreateLinkArray(jseContext jsecontext,
														struct BrowserDocument *document,
														jseVariable parent)
{
	// seb 98.11.12 -- Added cast to jseVariable
	jseVariable links = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);

	struct BrowserLocation *loc = NULL;
	ulong count = 0;

	while( (loc = browserGetNextLink(jsecontext,document,loc))!=NULL )
	{
		jseVariable it = browserCreateLocationObject(jsecontext,loc);
#warning Put_this_back_in
//		VarWrapper v(jsecontext,jseMemberEx(jsecontext,it,PARENT_PROPERTY,jseTypeUndefined,jseCreateVar));
//		jseAssign(jsecontext,v,parent);

//		jseAssign(jsecontext,jseMember(jsecontext,it,PARENT_PROPERTY,jseTypeUndefined),
//					 parent);
		VarWrapper m(jsecontext,jseIndexMemberEx(jsecontext,links,count++,jseTypeUndefined,jseCreateVar));
//		jseAssign(jsecontext,jseIndexMember(jsecontext,links,count++,jseTypeUndefined),
//					 it);
		jseAssign(jsecontext,m,it);
		jseDestroyVariable(jsecontext,it);
	}

	return links;
}


/* location routines */


static jseLibFunc(Locationreload)
{
	struct BrowserLocation *bl;
	jsebool reload;

	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bl = get_my_object(BrowserLocation,LOCATION_PROP);

	reload = False;
	if( jseFuncVarCount(jsecontext)==1 )
	{
		reload = jseGetLong(jsecontext,jseFuncVar(jsecontext,0));
	}

	browserReloadLocation(jsecontext,bl,reload,thisvar);
}


static jseLibFunc(Locationreplace)
{
	jseVariable url;
	struct BrowserLocation *bl;
	const jsechar *txt;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bl = get_my_object(BrowserLocation,LOCATION_PROP);

// seb 99.2.3 - Be more lenient about string conversion
	JSE_FUNC_VAR_NEED(url,jsecontext,0,CONVERT_ANY_TO_STRING);
//	JSE_FUNC_VAR_NEED(url,jsecontext,0,JSE_VN_STRING);
	txt = jseGetString(jsecontext,url,NULL);

	browserGotoURL(jsecontext,bl,txt,True);
}


/* ---------------------------------------------------------------------- */
/* History object and its dynamic put routine										*/
/* ---------------------------------------------------------------------- */


static jseLibFunc(historyUpdate)
{
	struct BrowserWindow *bw;
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	
	bw = get_my_object(BrowserWindow,WINDOW_PROP);
	browserSetUpHistoryObject(jsecontext,thisvar,bw);
}


static jseVariable browserCreateHistoryObject(jseContext jsecontext,
															 struct BrowserWindow *win)
{
	jseVariable where;

	
	where = jseCreateVariable(jsecontext,jseTypeObject);
	browserSetUpHistoryObject(jsecontext,where,win);
	return where;
}


static void browserSetUpHistoryObject(jseContext jsecontext,
													jseVariable where,
													struct BrowserWindow *win)
{
	ulong len,x;
	

	VarWrapper v(jsecontext,jseMemberEx(jsecontext,where,WINDOW_PROP,jseTypeNumber,jseCreateVar));
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)win);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

	VarWrapper proto(jsecontext,jseMemberEx(jsecontext,NULL,"history",jseTypeObject,jseCreateVar));
	proto = jseMemberEx(jsecontext,proto,PROTOTYPE_PROPERTY,jseTypeObject,jseCreateVar);
	VarWrapper nproto(jsecontext,jseMemberEx(jsecontext,where,PROTOTYPE_PROPERTY,jseTypeUndefined,jseCreateVar));
	jseAssign(jsecontext,nproto,proto);
	jseSetAttributes(jsecontext,nproto,jseDontDelete|jseDontEnum|jseReadOnly);


	/* add in length */
	len = browserGetHistoryLength(jsecontext,win);
	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,len);
	VarWrapper v1(jsecontext,jseMemberEx(jsecontext,where,"length",jseTypeUndefined,jseCreateVar));
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete|jseReadOnly);

	for( x=0;x<len;x++ )
	{
		v = jseCreateVariable(jsecontext,jseTypeString);
		BString string = browserGetHistoryItem(jsecontext,win,x);
		jsePutString(jsecontext,v,string.String());
		jseAssign(jsecontext,VarWrapper(jsecontext,jseIndexMemberEx(jsecontext,where,x,jseTypeUndefined,jseCreateVar)),v);
		jseSetAttributes(jsecontext,v,jseDontDelete|jseReadOnly);
	}

	/* add in the browserUpdate() routine. */
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 historyUpdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,historyUpdate);

		
	/* history is completely read-only, so just let the normal put take
	 * care of everything.
	 */
}


/* history routines */


static jseLibFunc(Historyback)
{
	struct BrowserWindow *bw;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserHistoryGo(jsecontext,bw,-1);
}


static jseLibFunc(Historyforward)
{
	struct BrowserWindow *bw;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserHistoryGo(jsecontext,bw,1);
}


static jseLibFunc(Historygo)
{
	jseVariable offset;
	struct BrowserWindow *bw;

	JSE_FUNC_VAR_NEED(offset,jsecontext,0,JSE_VN_NUMBER);
//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bw = get_my_object(BrowserWindow,WINDOW_PROP);

	browserHistoryGo(jsecontext,bw,jseGetLong(jsecontext,offset));
}


/* ---------------------------------------------------------------------- */
/* Document object and its dynamic put routine									 */
/* ---------------------------------------------------------------------- */


static jseLibFunc(documentUpdate)
{
	// seb 98.11.12 -- Changed the type to BrowserDocument
	struct BrowserDocument *bd;
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	
	// seb 98.11.12 -- Added cast to struct BrowserDocument *
	bd = (struct BrowserDocument *)get_my_object(BrowserDocument,DOCUMENT_PROP);
	browserSetUpDocumentObject(jsecontext,thisvar,bd);
}


// seb 98.11.12 -- Corrected mistyping of "BrowerDocument"
static jseVariable browserCreateDocumentObject(jseContext jsecontext,
																struct BrowserDocument *doc)
{
	jseVariable where;

	
	where = jseCreateVariable(jsecontext,jseTypeObject);
	browserSetUpDocumentObject(jsecontext,where,doc);
	return where;
}


static jseLibFunc(Documentput)
{
	const jsechar *prop;
	jseVariable txt;
	struct BrowserDocument *bd;
	enum DocumentColors color = none;
	const jsechar *text;

	prop = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bd = get_my_object(BrowserDocument,DOCUMENT_PROP);

	if ( strcmp(prop,"cookie")==0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		text = jseGetString(jsecontext,txt,NULL);
		browserSetCookie(jsecontext,bd,text);
	}
	else if( strcmp(prop,"alinkColor")==0 ) color = AlinkColor;
	else if( strcmp(prop,"linkColor")==0 ) color = LinkColor;
	else if( strcmp(prop,"bgColor")==0 ) color = BgColor;
	else if( strcmp(prop,"fgColor")==0 ) color = FgColor;

	if( color!=-1 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		text = jseGetString(jsecontext,txt,NULL);
		browserSetDocumentColor(jsecontext,bd,color,text);
	}
	
	/* we've notified the browser of any change, make sure to store it in
	 * our own structure tree.
	 */
	jseAssign(jsecontext,jseMember(jsecontext,thisvar,prop,jseTypeUndefined),
				 jseFuncVar(jsecontext,1));
}


static void browserSetUpDocumentObject(jseContext jsecontext,
													jseVariable where,
													struct BrowserDocument *doc)
{
	VarWrapper image, form;
	struct BrowserLocation *loc;
	struct URL url;
	ulong limit, x;
	const jsechar *name;
// seb 98.11.15
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	
	/* make sure if we are updating, we turn off the dynamic put so we
	 * can just directly store members without wasting the time to tell
	 * the browser to change to the state its already in for everything.
	 */
	jseDeleteMember(jsecontext,where,PUT_PROPERTY);

	browserSetDocumentVariable(jsecontext, where, doc);

	
	/* store the magic cookie */
// seb 98.11.15
	VarWrapper v(jsecontext,jseMemberEx(jsecontext,thisvar,DOCUMENT_PROP,jseTypeNumber,jseCreateVar));
//	v = jseMember(jsecontext,where,DOCUMENT_PROP,jseTypeNumber);
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)doc);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

// seb 98.2.1 -- Set it here, too.
	v = jseMemberEx(jsecontext,where,DOCUMENT_PROP,jseTypeNumber,jseCreateVar);
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)doc);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

	/* find and copy the document prototype object to our prototype */
	VarWrapper proto(jsecontext,jseMemberEx(jsecontext,NULL,"document",jseTypeObject,jseCreateVar));
	proto = jseMemberEx(jsecontext,proto,PROTOTYPE_PROPERTY,jseTypeObject,jseCreateVar);
	VarWrapper nproto(jsecontext,jseMemberEx(jsecontext,where,PROTOTYPE_PROPERTY,jseTypeUndefined,jseCreateVar));
	jseAssign(jsecontext,nproto,proto);
	jseSetAttributes(jsecontext,nproto,jseDontDelete|jseDontEnum|jseReadOnly);


	/* add in all the properties */ 

	/* anchors: this is NYI in Netscape 3.0/IE 3.0 (I don't know about 4.0),
	 *			 so for now we do the same.
	 *
	 * applets: we do not yet support all of the Java connection stuff
	 *			 which would be needed to implement this. embeds is an
	 *			 alias for this (or vice versa) as is plugins.
	 */
	// seb 98.11.12 -- Added cast to "jseVariable"
	v = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);
	VarWrapper v1(jsecontext,jseMemberEx(jsecontext,where,"anchors",jseTypeUndefined,jseCreateVar));
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v); 
	jseSetAttributes(jsecontext,v,jseDontDelete|jseReadOnly);
	// seb 98.11.12 -- Added cast to "jseVariable"
	v = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);
	v1 = jseMemberEx(jsecontext,where,"applets",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,NULL,"applets",jseTypeUndefined,jseCreateVar)),v);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,NULL,"embeds",jseTypeUndefined,jseCreateVar)),v);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,NULL,"plugins",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete|jseReadOnly);
		
	/* add in the color properties */
	BString string = browserGetDocumentColor(jsecontext,doc,AlinkColor);
	AssignString(jsecontext,where,"alinkColor",string.String(),jseDontDelete,0);
	
	string = browserGetDocumentColor(jsecontext,doc,LinkColor);
	AssignString(jsecontext,where,"linkColor",string.String(),jseDontDelete,0);
	
	string = browserGetDocumentColor(jsecontext,doc,VlinkColor);
	AssignString(jsecontext,where,"vlinkColor",string.String(),jseDontDelete,0);
	
	string = browserGetDocumentColor(jsecontext,doc,BgColor);
	AssignString(jsecontext,where,"bgColor",string.String(),jseDontDelete,0);
	
	string = browserGetDocumentColor(jsecontext,doc,FgColor);
	AssignString(jsecontext,where,"fgColor",string.String(),jseDontDelete,0);

	/* add in some more information */
	string = browserGetCookie(jsecontext,doc);
	AssignString(jsecontext,where,"cookie",string.String(),jseDontDelete,0);

	/* the rest are read-only */
	loc = browserGetDocumentLocation(jsecontext,doc);
	// seb 99.1.24 -- Added extra anchorContainer param.
	browserGetLocationValue(jsecontext,loc,&url,NULL);
	AssignString(jsecontext,where,"location",url.href.String(),jseDontDelete | jseReadOnly,0);
	AssignString(jsecontext,where,"URL",url.href.String(),jseDontDelete | jseReadOnly,0);

	string = browserGetReferrer(jsecontext,doc);
	AssignString(jsecontext,where,"referrer",string.String(),jseDontDelete | jseReadOnly,0);

	string = browserGetTitle(jsecontext,doc);
	AssignString(jsecontext,where,"title",string.String(),jseDontDelete | jseReadOnly,0);

	string = browserGetLastModified(jsecontext,doc);
	AssignString(jsecontext,where,"lastModified",string.String(),jseDontDelete | jseReadOnly,0);

	v = browserCreateFormArray(jsecontext,doc);
	v1 = jseMemberEx(jsecontext,where,"forms",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete | jseReadOnly);

	/* NOTE: NYI: get rid of any existing names */
	
	/* link their names in too */
	limit = jseGetArrayLength(jsecontext,v,NULL);
	for( x = 0;x<limit;x++ )
	{
// seb 99.2.17 -- We want to use jseIndexMember, not jseGetIndexMember.	We should create a
// new one if it doesn't exist.
		form.Set(jsecontext,jseIndexMemberEx(jsecontext,v,x,jseTypeUndefined,jseCreateVar));
#warning Put_this_back_in
//		jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,form,PARENT_PROPERTY,jseTypeUndefined,jseCreateVar)),
//					 where);
		name = jseGetString(jsecontext,jseGetMember(jsecontext,form,"name"),NULL);
		v1 = jseMemberEx(jsecontext,where,name,jseTypeUndefined,jseCreateVar);
		jseSetAttributes(jsecontext,v1,0);
		jseAssign(jsecontext,v1,form);
		jseSetAttributes(jsecontext,image,jseDontDelete | jseReadOnly);
	}

	v = browserCreateImageArray(jsecontext,doc);
	v1 = jseMemberEx(jsecontext,where,"images",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete | jseReadOnly);

	/* link their names in too */
	limit = jseGetArrayLength(jsecontext,v,NULL);
	for( x = 0;x<limit;x++ )
	{
// seb 99.2.17 -- We want to use jseIndexMember, not jseGetIndexMember.	We should create a
// new one if it doesn't exist.
		image.Set(jsecontext,jseIndexMemberEx(jsecontext,v,x,jseTypeUndefined,jseCreateVar));
#warning Put_this_back_in
//		jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,image,PARENT_PROPERTY,jseTypeUndefined,jseCreateVar)),
//					 where);
		name = jseGetString(jsecontext,jseGetMember(jsecontext,image,"name"),NULL);
		v1 = jseMemberEx(jsecontext,where,name,jseTypeUndefined,jseCreateVar);
		jseSetAttributes(jsecontext,v1,0);
		jseAssign(jsecontext,v1,image);
		jseSetAttributes(jsecontext,image,jseDontDelete | jseReadOnly);
	}

	v = browserCreateLinkArray(jsecontext,doc,where);
	v1 = jseMemberEx(jsecontext,where,"links",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete | jseReadOnly);


	/* add in the browserUpdate() routine. */
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 documentUpdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,documentUpdate);

		
	/* Last, make sure the dynamic put is set. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 Documentput,2,2,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,Documentput);
}

static jseLibFunc(Layerupdate)
{
	struct BrowserLayer *layer;
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	
	layer = get_my_object(BrowserLayer,LAYER_PROP);
	browserSetUpLayerObject(jsecontext,thisvar,layer);
}


static jseVariable browserCreateLayerObject(jseContext jsecontext,
															 struct BrowserLayer *layer)
{
	jseVariable where;
	
	where = jseCreateVariable(jsecontext,jseTypeObject);
	browserSetUpLayerObject(jsecontext,where,layer);
	return where;
}



static jseLibFunc(Layerput)
{
	const jsechar *prop;
	jseVariable txt;
	struct BrowserLayer *bl;
	struct SELayer lo;
	BString *partStr = NULL;
	int	*partInt = NULL;
	

	prop = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bl = get_my_object(BrowserLayer,FORM_PROP);

	// seb 99.1.24 -- Added extra formContainer param.
	browserGetLayer(jsecontext,bl,&lo,NULL);

	if( strcmp(prop,"src")==0 ) partStr = &(lo.src);
	else if (strcmp(prop,"visibility")==0) partStr = &(lo.visibility);
	else if (strcmp(prop,"background")==0) partStr = &(lo.background);
	else if (strcmp(prop,"bgColor")==0) partStr = &(lo.bgColor);
	else if (strcmp(prop,"left")==0) partInt = &(lo.left);
	else if (strcmp(prop,"top")==0) partInt = &(lo.top);
	else if (strcmp(prop,"pageX")==0) partInt = &(lo.pageX);
	else if (strcmp(prop,"pageY")==0) partInt = &(lo.pageY);
	else if (strcmp(prop,"zIndex")==0) partInt = &(lo.zIndex);
	else if (strcmp(prop,"clip.left")==0) partInt = &(lo.clipLeft);
	else if (strcmp(prop,"clip.right")==0) partInt = &(lo.clipRight);
	else if (strcmp(prop,"clip.top")==0) partInt = &(lo.clipTop);
	else if (strcmp(prop,"clip.bottom")==0) partInt = &(lo.clipBottom);
	else if (strcmp(prop,"clip.width")==0) partInt = &(lo.clipWidth);
	else if (strcmp(prop,"clip.height")==0) partInt = &(lo.clipHeight);

	if(partStr)	{
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
		*partStr = jseGetString(jsecontext,txt,NULL);
		browserSetLayer(jsecontext,bl,&lo);
	} else if (partInt) {
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_NUMBER);
		*partInt = jseGetLong(jsecontext,txt);
		browserSetLayer(jsecontext,bl,&lo);
	}
	
	/* we've notified the browser of any change, make sure to store it in
	 * our own structure tree.
	 */
	jseAssign(jsecontext,jseMember(jsecontext,thisvar,prop,jseTypeUndefined),
				 jseFuncVar(jsecontext,1));
}


static void browserSetUpLayerObject(jseContext jsecontext, jseVariable where, struct BrowserLayer *layer)
{
	struct SELayer layerInfo;

	
	/* make sure if we are updating, we turn off the dynamic put so we
	 * can just directly store members without wasting the time to tell
	 * the browser to change to the state its already in for everything.
	 */
	jseDeleteMember(jsecontext,where,PUT_PROPERTY);

	
//	VarWrapper v(jsecontext,jseMemberEx(jsecontext,where,LAYER_PROP,jseTypeNumber,jseCreateVar));
//	jseSetAttributes(jsecontext,v,0);
//	jsePutLong(jsecontext,v,(ulong)loc);
//	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);
	AssignLong(jsecontext,where,LAYER_PROP,(int)layer,jseDontEnum | jseDontDelete | jseReadOnly, 0);

	VarWrapper proto(jsecontext,jseMemberEx(jsecontext,NULL,"layer",jseTypeObject,jseCreateVar));
	proto = jseMemberEx(jsecontext,proto,PROTOTYPE_PROPERTY,jseTypeObject,jseCreateVar);
	VarWrapper nproto(jsecontext,jseMemberEx(jsecontext,where,PROTOTYPE_PROPERTY,jseTypeUndefined,jseCreateVar));
	jseAssign(jsecontext,nproto,proto);
	jseSetAttributes(jsecontext,nproto,jseDontDelete|jseDontEnum|jseReadOnly);

	browserGetLayer(jsecontext,layer,&layerInfo,where);

	VarWrapper v(jsecontext,jseMemberEx(jsecontext,where,"document",jseTypeUndefined,jseCreateVar));
	jseSetAttributes(jsecontext,v,0);
	jseAssign(jsecontext,v, VarWrapper(jsecontext,browserCreateDocumentObject(jsecontext,layerInfo.document)));

	AssignString(jsecontext,where,"name",layerInfo.name.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"src",layerInfo.src.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"visibility",layerInfo.visibility.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"background",layerInfo.background.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"bgColor",layerInfo.bgColor.String(),jseDontDelete,0);
	AssignLong(jsecontext,where,"left",layerInfo.left,jseDontDelete,0);
	AssignLong(jsecontext,where,"top",layerInfo.top,jseDontDelete,0);
	AssignLong(jsecontext,where,"pageX",layerInfo.pageX,jseDontDelete,0);
	AssignLong(jsecontext,where,"pageY",layerInfo.pageY,jseDontDelete,0);
	AssignLong(jsecontext,where,"zIndex",layerInfo.zIndex,jseDontDelete,0);
	AssignLong(jsecontext,where,"clip.left",layerInfo.clipLeft,jseDontDelete,0);
	AssignLong(jsecontext,where,"clip.right",layerInfo.clipRight,jseDontDelete,0);
	AssignLong(jsecontext,where,"clip.top",layerInfo.clipTop,jseDontDelete,0);
	AssignLong(jsecontext,where,"clip.bottom",layerInfo.clipBottom,jseDontDelete,0);
	AssignLong(jsecontext,where,"clip.width",layerInfo.clipWidth,jseDontDelete,0);
	AssignLong(jsecontext,where,"clip.height",layerInfo.clipHeight,jseDontDelete,0);
#warning Not implemented properly
// above
// below
// parentLayer
// siblingAbove
// siblingBelow
	
	/* add in the browserUpdate() routine. */
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 Layerupdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,Layerupdate);


	/* Last, make sure the dynamic put is set. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 Layerput,2,2,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,Layerput);
}


static jseLibFunc(Layercaptureevents)
{
#warning Not implemented
/*
	struct BrowserLayer *lay;

	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	browserLayerCaptureEvents(jsecontext,lay);
*/
}


static jseLibFunc(Layerhandleevent)
{
#warning Not implemented
/*
	struct BrowserLayer *lay;

	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	browserLayerHandleEvent(jsecontext,lay);
*/
}


static jseLibFunc(Layerload)
{
	jseVariable var;
	struct BrowserLayer *lay;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	const char *src = "";
	int width = 0;
	if (jseFuncVarCount(jsecontext) == 2) {
		JSE_FUNC_VAR_NEED(var,jsecontext,0,CONVERT_ANY_TO_STRING);
		src = jseGetString(jsecontext,var,NULL);
		JSE_FUNC_VAR_NEED(var,jsecontext,1,JSE_VN_NUMBER);
		width = jseGetLong(jsecontext,var);
	}
	browserLayerLoad(jsecontext,lay,src,width);
}

static jseLibFunc(Layermoveabove)
{
#warning Not implemented yet
/*
	struct BrowserLayer *lay;

	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	browserLayerMoveAbove(jsecontext,lay);
*/
}

static jseLibFunc(Layermovebelow)
{
#warning Not implemented yet
/*
	struct BrowserLayer *lay;

	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	browserLayerMoveBelow(jsecontext,lay);
*/
}

static jseLibFunc(Layermoveby)
{
	jseVariable var;
	struct BrowserLayer *lay;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	int x = 0;
	int y = 0;
	if (jseFuncVarCount(jsecontext) == 2) {
		JSE_FUNC_VAR_NEED(var,jsecontext,0,JSE_VN_NUMBER);
		x = jseGetLong(jsecontext,var);
		JSE_FUNC_VAR_NEED(var,jsecontext,1,JSE_VN_NUMBER);
		y = jseGetLong(jsecontext,var);
	}
	browserLayerMoveBy(jsecontext,lay,x,y);
}


static jseLibFunc(Layermoveto)
{
	jseVariable var;
	struct BrowserLayer *lay;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	int x = 0;
	int y = 0;
	if (jseFuncVarCount(jsecontext) == 2) {
		JSE_FUNC_VAR_NEED(var,jsecontext,0,JSE_VN_NUMBER);
		x = jseGetLong(jsecontext,var);
		JSE_FUNC_VAR_NEED(var,jsecontext,1,JSE_VN_NUMBER);
		y = jseGetLong(jsecontext,var);
	}
	browserLayerMoveTo(jsecontext,lay,x,y);
}


static jseLibFunc(Layermovetoabsolute)
{
	jseVariable var;
	struct BrowserLayer *lay;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	int x = 0;
	int y = 0;
	if (jseFuncVarCount(jsecontext) == 2) {
		JSE_FUNC_VAR_NEED(var,jsecontext,0,JSE_VN_NUMBER);
		x = jseGetLong(jsecontext,var);
		JSE_FUNC_VAR_NEED(var,jsecontext,1,JSE_VN_NUMBER);
		y = jseGetLong(jsecontext,var);
	}
	browserLayerMoveToAbsolute(jsecontext,lay,x,y);
}


static jseLibFunc(Layerreleaseevents)
{
#warning Not implemented yet
/*
	struct BrowserLayer *lay;

	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	browserLayerReleaseEvents(jsecontext,lay);
*/
}

static jseLibFunc(Layerresizeby)
{
	jseVariable var;
	struct BrowserLayer *lay;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	int x = 0;
	int y = 0;
	if (jseFuncVarCount(jsecontext) == 2) {
		JSE_FUNC_VAR_NEED(var,jsecontext,0,JSE_VN_NUMBER);
		x = jseGetLong(jsecontext,var);
		JSE_FUNC_VAR_NEED(var,jsecontext,1,JSE_VN_NUMBER);
		y = jseGetLong(jsecontext,var);
	}
	browserLayerResizeBy(jsecontext,lay,x,y);
}


static jseLibFunc(Layerresizeto)
{
	jseVariable var;
	struct BrowserLayer *lay;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	int x = 0;
	int y = 0;
	if (jseFuncVarCount(jsecontext) == 2) {
		JSE_FUNC_VAR_NEED(var,jsecontext,0,JSE_VN_NUMBER);
		x = jseGetLong(jsecontext,var);
		JSE_FUNC_VAR_NEED(var,jsecontext,1,JSE_VN_NUMBER);
		y = jseGetLong(jsecontext,var);
	}
	browserLayerResizeTo(jsecontext,lay,x,y);
}


static jseLibFunc(Layerrouteevent)
{
#warning Not implemented yet
/*
	struct BrowserLayer *lay;

	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	lay = get_my_object(BrowserLayer,LAYER_PROP);

	browserLayerCaptureEvents(jsecontext,lay);
*/
}

/* document routines */


static jseLibFunc(Documentclose)
{
	struct BrowserDocument *bd;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bd = get_my_object(BrowserDocument,DOCUMENT_PROP);

	browserCloseDocument(jsecontext,bd);
}


static jseLibFunc(Documentopen)
{
	jseVariable txt;
	struct BrowserDocument *bd;
	const jsechar *mime;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bd = get_my_object(BrowserDocument,DOCUMENT_PROP);
	mime = "text/html";

	if( jseFuncVarCount(jsecontext)==1 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,0,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,0,JSE_VN_STRING);
		mime = jseGetString(jsecontext,txt,NULL);
	}

	browserOpenDocument(jsecontext,bd,mime);
}


static jseLibFunc(Documentwrite)
{
	jseVariable v;
	struct BrowserDocument *bd;
	ulong x;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bd = get_my_object(BrowserDocument,DOCUMENT_PROP);

	for( x=0; x < jseFuncVarCount(jsecontext); x++ )
	{
		v = jseCreateConvertedVariable(jsecontext,
												 jseFuncVar(jsecontext,x),
												 jseToString);
		browserDocumentWrite(jsecontext,bd,jseGetString(jsecontext,v,NULL));
		jseDestroyVariable(jsecontext,v);
	}
}


static jseLibFunc(Documentwriteln)
{
	struct BrowserDocument *bd;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bd = get_my_object(BrowserDocument,DOCUMENT_PROP);

	Documentwrite(jsecontext);
	browserDocumentWrite(jsecontext,bd,"\n");
}


/* ---------------------------------------------------------------------- */
/* Form object and its dynamic put routine											*/
/* ---------------------------------------------------------------------- */


static jseLibFunc(formUpdate)
{
	struct BrowserForm *bf;
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	
	bf = get_my_object(BrowserForm,FORM_PROP);
	browserSetUpFormObject(jsecontext,thisvar,bf);
}


static jseLibFunc(Formput)
{
	const jsechar *prop;
	jseVariable txt;
	struct BrowserForm *bf;
	// seb 98.11.12 -- Changed name to SEForm
	struct SEForm fo;
	BString *part;

	prop = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bf = get_my_object(BrowserForm,FORM_PROP);

	// seb 99.1.24 -- Added extra formContainer param.
	browserGetForm(jsecontext,bf,&fo,NULL);

	if( strcmp(prop,"action")==0 ) part = &(fo.action);
	else if( strcmp(prop,"encoding")==0 ) part = &(fo.encoding);
	else if( strcmp(prop,"method")==0 ) part = &(fo.method);
	else if( strcmp(prop,"target")==0 ) part = &(fo.target);
	else part = NULL;

	if( part )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		*part = jseGetString(jsecontext,txt,NULL);
		browserSetForm(jsecontext,bf,&fo);
	}
	
	/* we've notified the browser of any change, make sure to store it in
	 * our own structure tree.
	 */
	jseAssign(jsecontext,jseMember(jsecontext,thisvar,prop,jseTypeUndefined),
				 jseFuncVar(jsecontext,1));
}


// seb 98.11.12 -- Corrected mistyping of "BrowerForm"
static jseVariable browserCreateFormObject(jseContext jsecontext,
														 struct BrowserForm *form)
{
	jseVariable where;

	
	where = jseCreateVariable(jsecontext,jseTypeObject);
	browserSetUpFormObject(jsecontext,where,form);

	return where;
}


static void browserSetUpFormObject(jseContext jsecontext,
												jseVariable where,
												struct BrowserForm *form)
{
	ulong limit, x;
	const jsechar *name;
	// seb 98.11.12 -- Changed name to SEForm
	struct SEForm fo;
	
zzzVar *fee = (zzzVar *)where;

	
	/* make sure if we are updating, we turn off the dynamic put so we
	 * can just directly store members without wasting the time to tell
	 * the browser to change to the state its already in for everything.
	 */
	jseDeleteMember(jsecontext,where,PUT_PROPERTY);

	
	/* store the magic cookie */
	VarWrapper v(jsecontext,jseMemberEx(jsecontext,where,FORM_PROP,jseTypeNumber,jseCreateVar));
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)form);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

	/* set up its prototype */
	VarWrapper proto(jsecontext,jseMemberEx(jsecontext,NULL,"form",jseTypeObject,jseCreateVar));
	proto = jseMemberEx(jsecontext,proto,PROTOTYPE_PROPERTY,jseTypeObject,jseCreateVar);
	VarWrapper nproto(jsecontext, jseMemberEx(jsecontext,where,PROTOTYPE_PROPERTY,jseTypeUndefined,jseCreateVar));
	jseAssign(jsecontext,nproto,proto);
	jseSetAttributes(jsecontext,nproto,jseDontDelete|jseDontEnum|jseReadOnly);

	/* add the elements array */
	v = browserCreateElementArray(jsecontext,form,where);
	VarWrapper v1(jsecontext, jseMemberEx(jsecontext,where,"elements",jseTypeUndefined,jseCreateVar));
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete | jseReadOnly);

	/* NOTE: NYI: get rid of any existing names */
	/* link their names in too */
	limit = jseGetArrayLength(jsecontext,v,NULL);
	for( x = 0;x<limit;x++ )
	{
// seb 99.2.17 -- We want to use jseIndexMember, not jseGetIndexMember.	We should create a
// new one if it doesn't exist.
		VarWrapper elem(jsecontext,jseIndexMemberEx(jsecontext,v,x,jseTypeUndefined,jseCreateVar));
		name = jseGetString(jsecontext,jseGetMember(jsecontext,elem,"name"),NULL);
		v1 = jseMemberEx(jsecontext,where,name,jseTypeUndefined,jseCreateVar);
		jseSetAttributes(jsecontext,v1,0);
		jseAssign(jsecontext,v1,elem);
		jseSetAttributes(jsecontext,elem,jseDontDelete | jseReadOnly);
	}

	/* add form items */
	// seb 99.1.24 -- Added extra formContainer param.
	browserGetForm(jsecontext,form,&fo,where);
	
	AssignString(jsecontext,where,"action",fo.action.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"encoding",fo.encoding.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"method",fo.method.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"target",fo.target.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"name",fo.name.String(),jseDontDelete|jseReadOnly,0);

	/* add in the browserUpdate() routine. */
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 formUpdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
#warning Put_these_back_in
//	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,formUpdate);

	/* Next, make sure the dynamic put is set. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 Formput,2,2,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
//	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,Formput);
}


static jseVariable browserCreateFormArray(jseContext jsecontext,
														struct BrowserDocument *document)
{
	// seb 98.11.12 -- Added cast to jseVariable
	jseVariable forms = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);
	struct BrowserForm *loop = NULL;
	ulong count = 0;

	while( (loop = browserGetNextForm(jsecontext,document,loop))!=NULL )
	{
		jseVariable it = browserCreateFormObject(jsecontext,loop);
		jseAssign(jsecontext,
					jseIndexMember(jsecontext,forms,count++,jseTypeUndefined),
					it);
		jseDestroyVariable(jsecontext,it);
	}
	
	return forms;
}


// seb 98.11.16 -- Added this function.
jseVariable browserAddForm(jseContext jsecontext, struct BrowserForm *form, struct BrowserWindow *window)
{
	jseVariable it;
	struct BrowserDocument *bd;
	struct BrowserWindow *bw;
	int position;
	const char *name;

	jseVariable oldglob = jseGlobalObject(jsecontext);
	VarWrapper newglob(jsecontext, browserWindowObject(jsecontext,window));
	VarWrapper readablenewglob(jsecontext, GET_READABLE_VAR(((jseVariable)newglob), jsecontext));
	jseSetGlobalObject(jsecontext,readablenewglob);
{

	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	if (!thisvar) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
	bd = (struct BrowserDocument *)get_my_object(BrowserDocument,DOCUMENT_PROP);
	bw = (struct BrowserWindow *)get_my_object(BrowserWindow,WINDOW_PROP);


//	VarWrapper windowvar(jsecontext, browserWindowObject(jsecontext,bw));

//	docvar = jseGetMember(jsecontext, windowvar, "document");
	VarWrapper docvar(jsecontext, jseGetMemberEx(jsecontext, newglob, "document",jseCreateVar));
	if (!docvar) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}

	VarWrapper formsvar(jsecontext, jseGetMemberEx(jsecontext, docvar, "forms",jseCreateVar));
	if (!formsvar) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
			
	it = browserCreateFormObject(jsecontext, form);
	if (!it) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
		
	position = jseGetArrayLength(jsecontext,formsvar,NULL);
	jseAssign(jsecontext, VarWrapper(jsecontext,jseIndexMemberEx(jsecontext, formsvar, position, jseTypeUndefined,jseCreateVar)), it);
	jseDestroyVariable(jsecontext, it);

// seb 99.2.17 -- We want to use jseIndexMember, not jseGetIndexMember.	We should create a
// new one if it doesn't exist.
	it = jseIndexMemberEx(jsecontext,formsvar,position,jseTypeUndefined,jseCreateVar);
#warning Put_this_back_in
//	jseAssign(jsecontext,jseMember(jsecontext,it,PARENT_PROPERTY,jseTypeUndefined),docvar);
	name = jseGetString(jsecontext,jseGetMember(jsecontext,it,"name"),NULL);
	VarWrapper v1(jsecontext, jseMemberEx(jsecontext,docvar,name,jseTypeUndefined,jseCreateVar));
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,it);
	jseSetAttributes(jsecontext,it,jseDontDelete | jseReadOnly);

	jseSetGlobalObject(jsecontext,oldglob);
}
zzzVar *foo = (zzzVar *)it;
printf("Foo is 0x%x\n", foo);
	return it;
}

// seb 98.12.28 -- Added this function.
jseVariable browserAddImage(jseContext jsecontext, struct BrowserImage *image, struct BrowserWindow *window)
{
	jseVariable it;
	struct BrowserDocument *bd;
	struct BrowserWindow *bw;
	int position;
	const char *name;

	jseVariable oldglob = jseGlobalObject(jsecontext);
	VarWrapper newglob(jsecontext, browserWindowObject(jsecontext,window));
	VarWrapper readablenewglob(jsecontext, GET_READABLE_VAR(((jseVariable)newglob), jsecontext));
	jseSetGlobalObject(jsecontext,readablenewglob);
{
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	if (!thisvar) {
printf("Bail1\n");
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
	bd = (struct BrowserDocument *)get_my_object(BrowserDocument,DOCUMENT_PROP);
	bw = (struct BrowserWindow *)get_my_object(BrowserWindow,WINDOW_PROP);


//	VarWrapper windowvar(jsecontext, browserWindowObject(jsecontext,bw));

//	docvar = jseGetMember(jsecontext, windowvar, "document");
	VarWrapper docvar(jsecontext, jseGetMemberEx(jsecontext, readablenewglob, "document", jseCreateVar));
	if (!docvar) {
printf("Bail2\n");
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
	
	VarWrapper imagesvar(jsecontext, jseGetMemberEx(jsecontext, docvar, "images", jseCreateVar));
	if (!imagesvar) {
printf("Bail3\n");
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
			
	it = browserCreateImageObject(jsecontext, image);
	if (!it) {
printf("Bail4\n");
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
			
	position = jseGetArrayLength(jsecontext,imagesvar,NULL);
	jseAssign(jsecontext, VarWrapper(jsecontext,jseIndexMemberEx(jsecontext, imagesvar, position, jseTypeUndefined, jseCreateVar)), it);
	jseDestroyVariable(jsecontext, it);

// seb 99.2.17 -- We want to use jseIndexMember, not jseGetIndexMember.	We should create a
// new one if it doesn't exist.
	it = jseIndexMemberEx(jsecontext,imagesvar,position,jseTypeUndefined,jseCreateVar);
#warning Put_this_back_in
//	jseAssign(jsecontext,jseMember(jsecontext,it,PARENT_PROPERTY,jseTypeUndefined),docvar);
	name = jseGetString(jsecontext,jseGetMember(jsecontext,it,"name"),NULL);
	VarWrapper v1(jsecontext, jseMemberEx(jsecontext,docvar,name,jseTypeUndefined, jseCreateVar));
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,it);
	jseSetAttributes(jsecontext,it,jseDontDelete | jseReadOnly);
	
	jseSetGlobalObject(jsecontext,oldglob);

}
	return it;
}


// seb 98.12.28 -- Added this function.
jseVariable browserAddAnchor(jseContext jsecontext, struct BrowserLocation *anchor, struct BrowserWindow *window)
{
	jseVariable it;
	struct BrowserDocument *bd;
	struct BrowserWindow *bw;
	int position;

	jseVariable oldglob = jseGlobalObject(jsecontext);
	VarWrapper newglob(jsecontext, browserWindowObject(jsecontext,window));
	VarWrapper readablenewglob(jsecontext, GET_READABLE_VAR(((jseVariable)newglob), jsecontext));
	jseSetGlobalObject(jsecontext,readablenewglob);

{
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	jseSetGlobalObject(jsecontext,oldglob);
	if (!thisvar)
		return NULL;
	bd = (struct BrowserDocument *)get_my_object(BrowserDocument,DOCUMENT_PROP);
	bw = (struct BrowserWindow *)get_my_object(BrowserWindow,WINDOW_PROP);

//	VarWrapper windowvar(jsecontext, browserWindowObject(jsecontext,bw));
	jseSetGlobalObject(jsecontext,readablenewglob);

//	VarWrapper docvar(jsecontext, jseGetMemberEx(jsecontext, windowvar, "document",jseCreateVar));
	VarWrapper docvar(jsecontext, jseGetMemberEx(jsecontext, readablenewglob, "document",jseCreateVar));
	if (!docvar) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
	
	VarWrapper linksvar(jsecontext, jseGetMemberEx(jsecontext, docvar, "links",jseCreateVar));
	if (!linksvar) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
			
	it = browserCreateLocationObject(jsecontext, anchor);
	if (!it) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
			
	position = jseGetArrayLength(jsecontext,linksvar,NULL);
	VarWrapper v(jsecontext,jseIndexMemberEx(jsecontext, linksvar, position, jseTypeUndefined,jseCreateVar));
	jseAssign(jsecontext, v, it);

// seb 99.2.17 -- We want to use jseIndexMember, not jseGetIndexMember.	We should create a
// new one if it doesn't exist.
//	it = jseIndexMemberEx(jsecontext,linksvar,position,jseTypeUndefined,jseCreateVar);
//	v = jseMemberEx(jsecontext,it,PARENT_PROPERTY,jseTypeUndefined,jseCreateVar);
//	jseAssign(jsecontext,v,docvar);

#warning Put_this_back_in
//	v = jseMemberEx(jsecontext,it,PARENT_PROPERTY,jseTypeUndefined,jseCreateVar);
//	jseAssign(jsecontext,v,docvar);

//	name = jseGetString(jsecontext,jseGetMember(jsecontext,it,"name"),NULL);
//	v1 = jseMember(jsecontext,docvar,name,jseTypeUndefined);
//	jseSetAttributes(jsecontext,v1,0);
//	jseAssign(jsecontext,v1,it);
//	jseSetAttributes(jsecontext,it,jseDontDelete | jseReadOnly);
}
	
	jseSetGlobalObject(jsecontext,oldglob);

	return it;
}


jseVariable browserAddLayer(jseContext jsecontext, struct BrowserLayer *layer, struct BrowserWindow *window)
{
	jseVariable docvar, layersvar, it, v1;
	struct BrowserDocument *bd;
	struct BrowserWindow *bw;
	int position;
	const char *name;

	jseVariable oldglob = jseGlobalObject(jsecontext);
	VarWrapper newglob(jsecontext, browserWindowObject(jsecontext,window));
	VarWrapper readablenewglob(jsecontext, GET_READABLE_VAR(((jseVariable)newglob), jsecontext));
	jseSetGlobalObject(jsecontext,readablenewglob);

	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	if (!thisvar) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
	bd = (struct BrowserDocument *)get_my_object(BrowserDocument,DOCUMENT_PROP);
	bw = (struct BrowserWindow *)get_my_object(BrowserWindow,WINDOW_PROP);


//	VarWrapper windowvar(jsecontext, browserWindowObject(jsecontext,bw));

//	docvar = jseGetMember(jsecontext, windowvar, "document");
	docvar = jseGetMember(jsecontext, readablenewglob, "document");
	if (!docvar) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
	
	layersvar = jseGetMember(jsecontext, docvar, "layers");
	if (!layersvar) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
			
	it = browserCreateLayerObject(jsecontext, layer);
	if (!it) {
		jseSetGlobalObject(jsecontext,oldglob);
		return NULL;
	}
			
	position = jseGetArrayLength(jsecontext,layersvar,NULL);
	jseAssign(jsecontext, jseIndexMember(jsecontext, layersvar, position, jseTypeUndefined), it);
	jseDestroyVariable(jsecontext, it);

// seb 99.2.17 -- We want to use jseIndexMember, not jseGetIndexMember.	We should create a
// new one if it doesn't exist.
	it = jseIndexMember(jsecontext,layersvar,position,jseTypeUndefined);
#warning Put_this_back_in
//	jseAssign(jsecontext,jseMember(jsecontext,it,PARENT_PROPERTY,jseTypeUndefined),docvar);
	name = jseGetString(jsecontext,jseGetMember(jsecontext,it,"name"),NULL);
	v1 = jseMemberEx(jsecontext,docvar,name,jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,it);
	jseSetAttributes(jsecontext,it,jseDontDelete | jseReadOnly);

	jseSetGlobalObject(jsecontext,oldglob);

	return it;
}



/* form methods */


static jseLibFunc(Formreset)
{
	struct BrowserForm *bf;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bf = get_my_object(BrowserForm,FORM_PROP);

	browserFormReset(jsecontext,bf);
}


static jseLibFunc(Formsubmit)
{
	struct BrowserForm *bf;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	bf = get_my_object(BrowserForm,FORM_PROP);

	browserFormSubmit(jsecontext,bf);
}


/* ---------------------------------------------------------------------- */
/* element object and its dynamic put routine										*/
/* ---------------------------------------------------------------------- */


static jseLibFunc(elementUpdate)
{
	struct BrowserElement *be;
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	
	be = get_my_object(BrowserElement,ELEMENT_PROP);
	browserSetUpElementObject(jsecontext,thisvar,be,NULL);
}


// seb 98.11.12 -- Corrected mistyping of "BrowerElement"
static jseVariable browserCreateElementObject(jseContext jsecontext,
															 struct BrowserElement *element,
															 jseVariable container)
{
	jseVariable where;

	
	where = jseCreateVariable(jsecontext,jseTypeObject);
	browserSetUpElementObject(jsecontext,where,element,container);
	return where;
}


static jseLibFunc(Elementput)
{
	const jsechar *prop;
	jseVariable val;
	struct BrowserElement *be;
	struct Element el;

	prop = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	be = get_my_object(BrowserElement,ELEMENT_PROP);

	browserGetElement(jsecontext,be,&el,0);
	if( el.checked_used && strcmp(prop,"checked")==0 )
	{
		browserGetElement(jsecontext,be,&el,0);
		JSE_FUNC_VAR_NEED(val,jsecontext,1,JSE_VN_NUMBER);
		el.checked = jseGetLong(jsecontext,val);
		browserSetElement(jsecontext,be,&el);
	}
	else if( el.si_used && strcmp(prop,"selectedIndex")==0 )
	{
		browserGetElement(jsecontext,be,&el,0);
		JSE_FUNC_VAR_NEED(val,jsecontext,1,JSE_VN_NUMBER);
		el.selectedIndex = jseGetLong(jsecontext,val);
		browserSetElement(jsecontext,be,&el);
	}
	else if( el.value_used && strcmp(prop,"value")==0 )
	{
		browserGetElement(jsecontext,be,&el,0);
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(val,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(val,jsecontext,1,JSE_VN_STRING);
		el.value = jseGetString(jsecontext,val,NULL);
		browserSetElement(jsecontext,be,&el);
	}
	
	/* we've notified the browser of any change, make sure to store it in
	 * our own structure tree.
	 */
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,thisvar,prop,jseTypeUndefined,jseCreateVar)),
				 jseFuncVar(jsecontext,1));
}


/* container_form==NULL = no update (already set) */
static void browserSetUpElementObject(jseContext jsecontext,
													jseVariable where,
													struct BrowserElement *elem,
													jseVariable container_form)
{
	VarWrapper optionsArray;
	struct Element el;


	/* make sure if we are updating, we turn off the dynamic put so we
	 * can just directly store members without wasting the time to tell
	 * the browser to change to the state its already in for everything.
	 */
	jseDeleteMember(jsecontext,where,PUT_PROPERTY);

	
	/* save magic cookie */
	VarWrapper v(jsecontext,jseMemberEx(jsecontext,where,ELEMENT_PROP,jseTypeNumber,jseCreateVar));
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)elem);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

	/* set up prototype */
	VarWrapper proto(jsecontext,jseMemberEx(jsecontext,NULL,"element",jseTypeObject,jseCreateVar));
	proto = jseMemberEx(jsecontext,proto,PROTOTYPE_PROPERTY,jseTypeObject,jseCreateVar);
	VarWrapper nproto(jsecontext,jseMemberEx(jsecontext,where,PROTOTYPE_PROPERTY,jseTypeUndefined,jseCreateVar));
	jseAssign(jsecontext,nproto,proto);
	jseSetAttributes(jsecontext,nproto,jseDontDelete|jseDontEnum|jseReadOnly);

	
	/* store containing form */
	if( container_form )
	{
		jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"form",jseTypeUndefined,jseCreateVar)),
					 container_form);
#warning Put_this_back_in
//		jseAssign(jsecontext,jseMember(jsecontext,where,PARENT_PROPERTY,jseTypeUndefined),
//					 container_form);
	}

	// seb 98.12.31 -- Added a parameter to pass where into browserGetElement.	It makes my
	// life easier.
	browserGetElement(jsecontext,elem,&el,where);

	/* copy all the information into the structure */

	v = jseCreateVariable(jsecontext,jseTypeBoolean);
	// seb 99.01.10 - Changed to Boolean type.
	jsePutBoolean(jsecontext,v,el.defaultChecked);
	VarWrapper v1(jsecontext,jseMemberEx(jsecontext,where,"defaultChecked",jseTypeUndefined,jseCreateVar));
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete | jseReadOnly);

	v = jseCreateVariable(jsecontext,jseTypeBoolean);
	// seb 99.01.10 - Changed to Boolean type.
	jsePutBoolean(jsecontext,v,el.checked);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"checked",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);

	if( el.options_used )
	{
		v = jseCreateVariable(jsecontext,jseTypeNumber);
		jsePutLong(jsecontext,v,el.num_options);
		jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"length",jseTypeUndefined,jseCreateVar)),v);
		jseSetAttributes(jsecontext,v,jseDontDelete);

		v = browserCreateOptionsArray(jsecontext,elem,el.num_options,where);
//seb 98.12.31. -- Changed to optionsArray 
		optionsArray.Set(jsecontext,jseMemberEx(jsecontext,where,"options",jseTypeUndefined,jseCreateVar));
		jseSetAttributes(jsecontext,optionsArray,0);
		jseAssign(jsecontext,optionsArray,v);
		jseSetAttributes(jsecontext,v,jseDontDelete | jseReadOnly);
	}

	if( el.dv_used )
	{
		v = jseCreateVariable(jsecontext,jseTypeString);
		jsePutString(jsecontext,v,el.defaultValue.String());
		v1 = jseMemberEx(jsecontext,where,"defaultValue",jseTypeUndefined,jseCreateVar);
		jseSetAttributes(jsecontext,v1,0);
		jseAssign(jsecontext,v1,v);
		jseSetAttributes(jsecontext,v,jseDontDelete | jseReadOnly);
	}
	
	v = jseCreateVariable(jsecontext,jseTypeString);
	jsePutString(jsecontext,v,el.type.String());
	v1 = jseMemberEx(jsecontext,where,"type",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete | jseReadOnly);

	v = jseCreateVariable(jsecontext,jseTypeString);
	jsePutString(jsecontext,v,el.name.String());
	v1 = jseMemberEx(jsecontext,where,"name",jseTypeUndefined,jseCreateVar);
	jseSetAttributes(jsecontext,v1,0);
	jseAssign(jsecontext,v1,v);
	jseSetAttributes(jsecontext,v,jseDontDelete | jseReadOnly);

	/* selectedIndex,value are writable */

	if( el.si_used )
	{
		v = jseCreateVariable(jsecontext,jseTypeNumber);
		jsePutLong(jsecontext,v,el.selectedIndex);
		jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"selectedIndex",jseTypeUndefined,jseCreateVar)),v);
		jseSetAttributes(jsecontext,v,jseDontDelete);

// seb 98.12.31 -- Added.	Some scripts expect selectedIndex to be in the options array, too.
// We need to temporarily get rid of the dynamic put so that we can change this member.
		jseDeleteMember(jsecontext,optionsArray,PUT_PROPERTY);
	
		v = jseMemberEx(jsecontext,optionsArray,"selectedIndex",jseTypeNumber,jseCreateVar);
		assert( v!=NULL );
		jseConvert(jsecontext,v,jseTypeNumber);
		jsePutLong(jsecontext,v,el.selectedIndex);

//		jseMemberWrapperFunction(jsecontext,optionsArray,PUT_PROPERTY,
//										 OptionArrayput,2,2,jseDontEnum | jseDontDelete | jseReadOnly,
//										 jseFunc_Secure,NULL);
		AssignWrapper(jsecontext,where,PUT_PROPERTY,2,OptionArrayput);
	}

	if( el.value_used )
	{
		v = jseCreateVariable(jsecontext,jseTypeString);
		jsePutString(jsecontext,v,el.value.String());
		jseAssign(jsecontext,jseMemberEx(jsecontext,where,"value",jseTypeUndefined,jseCreateVar),v);
		jseSetAttributes(jsecontext,v,jseDontDelete);
	}


	/* add in the browserUpdate() routine. */
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 elementUpdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,elementUpdate);

	/* Finally, make sure the dynamic put is set. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 Elementput,2,2,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,Elementput);
}


static jseVariable browserCreateElementArray(jseContext jsecontext,
															struct BrowserForm *form,
															jseVariable container_form)
{
	jseVariable where, it;
	struct BrowserElement *loop;
	ulong count;

	// seb 98.11.12 -- Added cast to jseVariable
	where = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);

	loop = NULL;
	count = 0;
	while( (loop = browserGetNextElement(jsecontext,form,loop))!=NULL )
	{
	 // seb 98.12.4 -- Look to see if an element with the same name already exists.
	 // If so, look to see if that element is an array.	If it's an array, add the new
	 // element to the array.	If it's not, then convert the entry to an array, add
	 // the original element, and add the new element.
	 ulong index;
	 const jsechar *newName;
	 ulong newLength;
	 ulong didIt = 0;
	
		it = browserCreateElementObject(jsecontext,loop,container_form);
	 newName = jseGetString(jsecontext, VarWrapper(jsecontext,jseMemberEx(jsecontext, it, "name", jseTypeUndefined,jseCreateVar)), &newLength);

	 for (index = 0; index < count; index++) {
//		 jseVariable element;
		 const jsechar *name;
		 ulong length;
		 VarWrapper existingElement(jsecontext,jseIndexMemberEx(jsecontext, where, index, jseTypeObject,jseCreateVar));
		 name = jseGetString(jsecontext, VarWrapper(jsecontext,jseMemberEx(jsecontext, existingElement, "name", jseTypeUndefined,jseCreateVar)), &length);
		 if (strcmp(name, newName) == 0) {
			slong minIndex;
			if (jseGetArrayLength(jsecontext, existingElement, &minIndex) > 0) {
				// We found an existing array, add the new member to it.
				jseAssign(jsecontext, VarWrapper(jsecontext,jseIndexMemberEx(jsecontext, existingElement, jseGetArrayLength(jsecontext, existingElement, &minIndex), jseTypeUndefined,jseCreateVar)), it);
				jseDestroyVariable(jsecontext, it);
				didIt = 1;
			} else {
				// We found the member, but it's not an array.	Create an array, add the original and new members to
				// it, set it up, and put it in place of the original member.
				VarWrapper newArray(jsecontext,(jseVariable)CreateNewObject(jsecontext, ARRAY_PROPERTY));
				jseAssign(jsecontext, VarWrapper(jsecontext,jseIndexMemberEx(jsecontext, newArray, 0, jseTypeUndefined,jseCreateVar)), existingElement);
				jseAssign(jsecontext, VarWrapper(jsecontext,jseIndexMemberEx(jsecontext, newArray, 1, jseTypeUndefined,jseCreateVar)), it);
				jsePutString(jsecontext, VarWrapper(jsecontext,jseMemberEx(jsecontext, newArray, "name", jseTypeUndefined,jseCreateVar)), newName);
				jseAssign(jsecontext, VarWrapper(jsecontext,jseIndexMemberEx(jsecontext, where, index, jseTypeUndefined,jseCreateVar)), newArray);
				jseDestroyVariable(jsecontext, it);
				didIt = 1;
			}
			break;
		 }
	}
	
		if (!didIt) {
		jseAssign(jsecontext,VarWrapper(jsecontext,jseIndexMemberEx(jsecontext,where,count++,jseTypeUndefined,jseCreateVar)),it);
			jseDestroyVariable(jsecontext,it);
	 }
	}

	return where;
}


/* element routines */


static jseLibFunc(Elementblur)
{
	struct BrowserElement *be;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	be = get_my_object(BrowserElement,ELEMENT_PROP);

	browserElementBlur(jsecontext,be);
}


static jseLibFunc(Elementclick)
{
	struct BrowserElement *be;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	be = get_my_object(BrowserElement,ELEMENT_PROP);

	browserElementClick(jsecontext,be);
}


static jseLibFunc(Elementfocus)
{
	struct BrowserElement *be;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	be = get_my_object(BrowserElement,ELEMENT_PROP);

	browserElementFocus(jsecontext,be);
}


static jseLibFunc(Elementselect)
{
	struct BrowserElement *be;

//	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	be = get_my_object(BrowserElement,ELEMENT_PROP);

	browserElementSelect(jsecontext,be);
}


/* ---------------------------------------------------------------------- */
/* option object																			 */
/* ---------------------------------------------------------------------- */


// seb 98.11.12 -- Changed name to SEOption
static void browserFillOptionFields(jseContext jsecontext,
												jseVariable where,struct SEOption *opt)
{
	VarWrapper var;
	// seb 98.11.12 -- Added casts to jseVariable
	// seb 99.01.10 - Changed to Boolean type.
	jsePutBoolean(jsecontext,var.Set(jsecontext,where,"defaultSelected",jseTypeBoolean),
				 opt->defaultSelected);
	jsePutBoolean(jsecontext,var.Set(jsecontext,where,"selected",jseTypeBoolean),
				 opt->selected);
	jsePutLong(jsecontext,var.Set(jsecontext,where,"index",jseTypeNumber),
				 opt->index);
	jsePutString(jsecontext,var.Set(jsecontext,where,"text",jseTypeString),
				 opt->text.String());
	jsePutString(jsecontext,var.Set(jsecontext,where,"value",jseTypeString),
				 opt->value.String());
}


static jseLibFunc(optionUpdate)
{
	struct BrowserElement *be;
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	// seb 98.11.12 -- Changed name to SEOption
	struct SEOption opt;
	ulong index;
	
	be = get_my_object(BrowserElement,ELEMENT_PROP);
	if( be!=NULL )
	{
		VarWrapper var;
		// seb 98.11.12 -- Added cast to jseVariable
		index = jseGetLong(jsecontext,var.Set(jsecontext,thisvar,"index",jseTypeNumber));
		browserGetElementOption(jsecontext,be,index,&opt);
		browserSetUpElementOption(jsecontext,thisvar,be,&opt, NULL);
	}
}


// seb 98.11.12 -- Changed name to SEOption.	Corrected mistyping of "BrowerElement"
// seb 99.1.23 -- Add the parent in browserSetUpElementOption to get around dynamic put.
static jseVariable browserCreateElementOption(jseContext jsecontext,
															 struct BrowserElement *element,
															 struct SEOption *opt,
												jseVariable parent)
{
	jseVariable where;

	
	where = jseCreateVariable(jsecontext,jseTypeObject);
	// seb 99.1.23 -- Add the parent in browserSetUpElementOption to get around dynamic put.
	browserSetUpElementOption(jsecontext,where,element,opt, parent);
	return where;
}


static void browserUpdateElement(jseContext jsecontext,
											jseVariable thisvar,
											struct BrowserElement *be)
{
	// seb 98.11.12 -- Changed name to SEOption
	struct SEOption opt;

	// seb 99.01.10 - Changed to Boolean type.
	opt.defaultSelected = jseGetBoolean(jsecontext,
							VarWrapper(jsecontext,jseMemberEx(jsecontext,thisvar,"defaultSelected",jseTypeBoolean,jseCreateVar)));
	opt.selected = jseGetBoolean(jsecontext,
							VarWrapper(jsecontext,jseMemberEx(jsecontext,thisvar,"selected",jseTypeBoolean,jseCreateVar)));
	opt.index = jseGetLong(jsecontext,
							VarWrapper(jsecontext,jseMemberEx(jsecontext,thisvar,"index",jseTypeNumber,jseCreateVar)));
	opt.text = jseGetString(jsecontext,
							VarWrapper(jsecontext,jseMemberEx(jsecontext,thisvar,"text",jseTypeString,jseCreateVar)),NULL);
	opt.value = jseGetString(jsecontext,
							VarWrapper(jsecontext,jseMemberEx(jsecontext,thisvar,"value",jseTypeString,jseCreateVar)),NULL);

	/* need to pass the element index */
	browserSetElementOption(jsecontext,be,&opt);
}


static jseLibFunc(Optionput)
{
	const jsechar *prop;
	struct BrowserElement *be;

	prop = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	be = get_my_object(BrowserElement,ELEMENT_PROP);

	/* do the regular thing */
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,thisvar,prop,jseTypeUndefined,jseCreateVar)),
				 jseFuncVar(jsecontext,1));

	/* then if we are attached to an element, update it */
	if( be )
	{
		browserUpdateElement(jsecontext,thisvar,be);
	}
}


// seb 98.11.12 -- Changed name to SEOption
// seb 99.1.23 -- Add the parent in browserSetUpElementOption to get around dynamic put.
static void browserSetUpElementOption(jseContext jsecontext,
													jseVariable where,
													struct BrowserElement *elem,
													struct SEOption *opt,
										jseVariable parent)
{
	/* make sure if we are updating, we turn off the dynamic put so we
	 * can just directly store members without wasting the time to tell
	 * the browser to change to the state its already in for everything.
	 */
	jseDeleteMember(jsecontext,where,PUT_PROPERTY);

	
	VarWrapper v(jsecontext,jseMemberEx(jsecontext,where,ELEMENT_PROP,jseTypeNumber,jseCreateVar));
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)elem);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

	/* update all the fields */
	browserFillOptionFields(jsecontext,where,opt);

// seb 99.1.23 -- Add the parent in browserSetUpElementOption to get around dynamic put.
#warning Put_this_back_in
//	if (parent)
//		jseAssign(jsecontext,jseMember(jsecontext,where,PARENT_PROPERTY,jseTypeUndefined),
//					 parent);

	/* add in the browserUpdate() routine. */
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 optionUpdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,optionUpdate);
	
	/* Next, make sure the dynamic put is set. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 Optionput,2,2,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,Optionput);
}


/* Create and return (with NULL BrowserElement) a new option - 0 to 4 params */
static jseLibFunc(Optionconstruct)
{
	// seb 98.11.12 -- Changed name to SEOption
	struct SEOption opt;
	int count;
	jseVariable text, val, ret;

	opt.defaultSelected = False;
	opt.index = 0;
	opt.selected = False;
	opt.text = "";
	opt.value = "";

	count = jseFuncVarCount(jsecontext);
	if( count>0 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(text,jsecontext,0,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(text,jsecontext,0,JSE_VN_STRING);
		opt.text = jseGetString(jsecontext,text,NULL);
	}
	if( count>1 )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(text,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(text,jsecontext,1,JSE_VN_STRING);
		opt.value = jseGetString(jsecontext,text,NULL);
	}
	if( count>2 )
	{
		JSE_FUNC_VAR_NEED(val,jsecontext,2,JSE_VN_BOOLEAN);
		opt.defaultSelected = jseGetLong(jsecontext,val);
	}
	if( count>3 )
	{
		JSE_FUNC_VAR_NEED(val,jsecontext,3,JSE_VN_BOOLEAN);
		opt.selected = jseGetLong(jsecontext,val);
	}

	// seb 99.1.23 -- Add the parent in browserSetUpElementOption to get around dynamic put.
	ret = browserCreateElementOption(jsecontext,NULL,&opt, NULL);
	VarWrapper v(jsecontext,jseMemberEx(jsecontext,ret,ELEMENT_PROP,jseTypeNumber,jseCreateVar));
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,0);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

	jseReturnVar(jsecontext,ret,jseRetTempVar);
}


/* The entire array dynamic put - it only allows
 * putting a new element on the first empty slot, and increments the length
 * appropriately
 */
static jseLibFunc(OptionArrayput)
{
	jseVariable it;
	const jsechar *prop;
	struct BrowserElement *be;
	long index;

	/* The item being put must actually be one of our elements */
	it = jseFuncVar(jsecontext,1);

	prop = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	be = get_my_object(BrowserElement,ELEMENT_PROP);
	
	/* Must be an element */
	if( be )
	{
		/* Ok, next we simply assign this as the given element and increment the
		 * number of elements.
		 */

		/* must be a numeric index */
		if( isdigit(prop[0]) )
		{
			index = atoi(prop);

			VarWrapper v(jsecontext,jseMemberEx(jsecontext,it,ELEMENT_PROP,jseTypeNumber,jseCreateVar));
			jseSetAttributes(jsecontext,v,0);
			jsePutLong(jsecontext,v,(ulong)be);
			jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

			jsePutLong(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,it,"index",jseTypeNumber,jseCreateVar)),index);
			jseAssign(jsecontext,jseIndexMember(jsecontext,thisvar,index,jseTypeUndefined),it);

			VarWrapper len(jsecontext,jseMemberEx(jsecontext,thisvar,LENGTH_PROPERTY,jseTypeNumber,jseCreateVar));
			if( index+1>jseGetLong(jsecontext,len) )
				jsePutLong(jsecontext,len,index+1);

			browserUpdateElement(jsecontext,it,be);
		}
	}
}


static jseVariable browserCreateOptionsArray(jseContext jsecontext,
															struct BrowserElement *elem,
															ulong num_options,
															jseVariable parent)
{
	/* Not only do we have to handle the normal read/write stuff, but we must
	 * be able to assign a new element to the array via ' = new Option();'.
	 *
	 * Each one needs an element/index assigned to it. If doesn't have this,
	 * updates to it are recorded but not passed along.
	 */
	jseVariable where, it;
	ulong x;
	// seb 98.11.12 -- Changed name to SEOption
	struct SEOption opt;

	where = jseCreateVariable(jsecontext,jseTypeObject);

	VarWrapper v(jsecontext, jseMemberEx(jsecontext,where,ELEMENT_PROP,jseTypeNumber,jseCreateVar));
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)elem);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);

	// seb 99.1.2 -- We need a selectedIndex member.	We'll populate it later.
	v = jseMemberEx(jsecontext, where, "selectedIndex", jseTypeNumber,jseCreateVar);
	jsePutLong(jsecontext, v, 0);

	for( x=0;x<num_options;x++ )
	{
		browserGetElementOption(jsecontext,elem,x,&opt);
		/* need to add the element index we are saving */
		// seb 99.1.23 -- Add the parent in browserSetUpElementOption to get around dynamic put.
		it = browserCreateElementOption(jsecontext,elem,&opt, parent);
//		jseAssign(jsecontext,jseMember(jsecontext,it,PARENT_PROPERTY,jseTypeUndefined),
//					 parent);
		jseAssign(jsecontext,jseIndexMember(jsecontext,where,x,jseTypeUndefined),it);
		jseDestroyVariable(jsecontext,it);
	}

	jsePutLong(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,LENGTH_PROPERTY,jseTypeNumber,jseCreateVar)),x);

	/* Set up dynamic put property for the array itself. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 OptionArrayput,2,2,jseDontEnum | jseDontDelete | jseReadOnly,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,OptionArrayput);

	return where;
}


/* ---------------------------------------------------------------------- */
/* image object																				*/
/* ---------------------------------------------------------------------- */


static jseLibFunc(imageUpdate)
{
	struct BrowserImage *bi;
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	
	bi = get_my_object(BrowserImage,IMAGE_PROP);
	browserSetUpImageObject(jsecontext,thisvar,bi);
}


// seb 98.11.12 -- Corrected mistyping of "BrowerImage"
static jseVariable browserCreateImageObject(jseContext jsecontext,
															struct BrowserImage *image)
{
	jseVariable where;

	
	where = jseCreateVariable(jsecontext,jseTypeObject);
	browserSetUpImageObject(jsecontext,where,image);
	return where;
}


static jseLibFunc(Imageput)
{
	const jsechar *prop;
	jseVariable num, txt;
	struct BrowserImage *bi;
// seb 98.11.13 -- Image conflicts, changing name to SEImage
	struct SEImage im;
	BString *part;
	uint *int_part;


	prop = jseGetString(jsecontext,jseFuncVar(jsecontext,0),NULL);
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
	// seb 98.11.12 -- Added cast to BrowserImage
	bi = (struct BrowserImage *)get_my_object(BrowserElement,IMAGE_PROP);

	browserGetImage(jsecontext,bi,&im);

	if( strcmp(prop,"lowsrc")==0 ) part = &(im.lowsrc);
	else if( strcmp(prop,"name")==0 ) part = &(im.name);
	else if( strcmp(prop,"src")==0 ) part = &(im.src);
	else part = NULL;

	if( part )
	{
// seb 99.2.3 - Be more lenient about string conversion
		JSE_FUNC_VAR_NEED(txt,jsecontext,1,CONVERT_ANY_TO_STRING);
//		JSE_FUNC_VAR_NEED(txt,jsecontext,1,JSE_VN_STRING);
		*part = jseGetString(jsecontext,txt,NULL);
		browserSetImage(jsecontext,bi,&im);
	}
	else
	{
		if( strcmp(prop,"border")==0 ) int_part = &(im.border);
		else if( strcmp(prop,"height")==0 ) int_part = &(im.height);
		else if( strcmp(prop,"hspace")==0 ) int_part = &(im.hspace);
		else if( strcmp(prop,"vspace")==0 ) int_part = &(im.vspace);
		else if( strcmp(prop,"width")==0 ) int_part = &(im.width);
		else int_part = NULL;

		if( int_part )
		{
			JSE_FUNC_VAR_NEED(num,jsecontext,1,JSE_VN_NUMBER);
			*int_part = jseGetLong(jsecontext,num);
			browserSetImage(jsecontext,bi,&im);
		}
		else if( strcmp(prop,"complete")==0 )
		{
			JSE_FUNC_VAR_NEED(num,jsecontext,1,JSE_VN_BOOLEAN);
			im.complete = jseGetLong(jsecontext,num);
			browserSetImage(jsecontext,bi,&im);
		}
	}

	
	/* we've notified the browser of any change, make sure to store it in
	 * our own structure tree.
	 */
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,thisvar,prop,jseTypeUndefined,jseCreateVar)),
				 jseFuncVar(jsecontext,1));
}


/* Create and return a new Image */
static jseLibFunc(Imageconstruct)
{
// seb 98.11.13 -- Image conflicts, changing name to SEImage
	struct SEImage im;
	jseVariable v;
	jseVariable where = jseCreateVariable(jsecontext,jseTypeObject);
	
	
	browserGetImage(jsecontext,NULL,&im);

	/* copy the info from the image structure */
	AssignString(jsecontext,where,"lowsrc",im.lowsrc.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"name",im.name.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"src",im.src.String(),jseDontDelete,0);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.border);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"border",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.height);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"height",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.hspace);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"hspace",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.vspace);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"vspace",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.width);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"width",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);


	v = jseCreateVariable(jsecontext,jseTypeBoolean);
	// seb 99.01.10 - Changed to Boolean type.
	jsePutBoolean(jsecontext,v,im.complete);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"complete",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	
	/* add in the browserUpdate() routine. */
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 imageUpdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,imageUpdate);
	

	/* Last, make sure the dynamic put is set. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 Imageput,2,2,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,Imageput);
}


static void browserSetUpImageObject(jseContext jsecontext,
												jseVariable where,
												struct BrowserImage *image)
{
// seb 98.11.13 -- Image conflicts, changing name to SEImage
	struct SEImage im;


	/* make sure if we are updating, we turn off the dynamic put so we
	 * can just directly store members without wasting the time to tell
	 * the browser to change to the state its already in for everything.
	 */
	jseDeleteMember(jsecontext,where,PUT_PROPERTY);

	
	/* save the magic cookie */
	VarWrapper v(jsecontext,jseMemberEx(jsecontext,where,IMAGE_PROP,jseTypeNumber,jseCreateVar));
	jseSetAttributes(jsecontext,v,0);
	jsePutLong(jsecontext,v,(ulong)image);
	jseSetAttributes(jsecontext,v,jseDontEnum | jseDontDelete | jseReadOnly);


	browserGetImage(jsecontext,image,&im);

	/* copy the info from the image structure */
	AssignString(jsecontext,where,"lowsrc",im.lowsrc.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"name",im.name.String(),jseDontDelete,0);
	AssignString(jsecontext,where,"src",im.src.String(),jseDontDelete,0);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.border);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"border",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.height);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"height",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.hspace);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"hspace",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.vspace);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"vspace",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	v = jseCreateVariable(jsecontext,jseTypeNumber);
	jsePutLong(jsecontext,v,im.width);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"width",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);


	/* NOTE: to get this to become 'true', you must make sure to use the
	 *		 browserUpdate() of this image once it finishes loading. This
	 *		 is described at the top of this file.
	 */
	v = jseCreateVariable(jsecontext,jseTypeBoolean);
	// seb 99.01.10 - Changed to Boolean type.
	jsePutBoolean(jsecontext,v,im.complete);
	jseAssign(jsecontext,VarWrapper(jsecontext,jseMemberEx(jsecontext,where,"complete",jseTypeUndefined,jseCreateVar)),v);
	jseSetAttributes(jsecontext,v,jseDontDelete);
	jseDestroyVariable(jsecontext,v);

	
	/* add in the browserUpdate() routine. */
//	jseMemberWrapperFunction(jsecontext,where,BROWSER_UPDATE_ROUTINE,
//									 imageUpdate,0,0,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,BROWSER_UPDATE_ROUTINE,0,imageUpdate);
	

	/* Last, make sure the dynamic put is set. */
//	jseMemberWrapperFunction(jsecontext,where,PUT_PROPERTY,
//									 Imageput,2,2,jseDontEnum | jseDontDelete,
//									 jseFunc_Secure,NULL);
	AssignWrapper(jsecontext,where,PUT_PROPERTY,2,Imageput);
}


static jseVariable browserCreateImageArray(jseContext jsecontext,
														 struct BrowserDocument *document)
{
	// seb 98.11.12 -- Added cast to jseVariable
	jseVariable links = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);

	struct BrowserImage *loc = NULL;
	ulong count = 0;

	while( (loc = browserGetNextImage(jsecontext,document,loc))!=NULL )
	{
		jseVariable it = browserCreateImageObject(jsecontext,loc);
		jseAssign(jsecontext,jseIndexMember(jsecontext,links,count++,jseTypeUndefined),
					 it);
		jseDestroyVariable(jsecontext,it);
	}

	/* NOTE: I don't know if it is legal to assign an image. For instance,
	 *		 can you say "document.images[2] = new Image();"? If this is legal,
	 *		 you'll have to write a dynamic put for this array which notifies the
	 *		 browser of the change. browserCreateOptionsArray() adds a dynamic
	 *		 put, so you can use it as a guide.
	 */
	
	return links;
}


/* ----------------------------------------------------------------------
 * What follows is some routines that the browser should call.
 * ---------------------------------------------------------------------- */


#define JAVA_ENABLED "javaEnabledFlag"


/* This routine is here because many shut-down type activities may eventually
 * be needed. Its best to be prepared.
 *
 * NOTE: Se410 cyclic loop checking is better than se403, so the problem
 *		 should disappear. If it doesn't, this is the place to go through
 *		 each window object in the global object and delete its 'window' and
 *		 'self' properties
 */
void browserCleanup(jseContext jsecontext)
{
}

// seb 98.11.12 -- Added
void browserSetWindow(jseContext jsecontext,struct BrowserWindow *window)
{
}

/* Sets up some general information about your browser in the 'navigator'
 * object. For compatibility, it is suggested you make your 'appCodeName'
 * be "Mozilla".
 */
// seb 98.11.12 -- Added security bits and copyright information.
// also made the strings const char *.
void browserGeneralInfo(jseContext jsecontext,const char *appCodeName,const char *appName,
								const char *appVersion,jsebool javaEnabled, uint securityBits,
						const char *copyrightInfo)
{
	jseVariable nav;
	char *buf;
	VarWrapper var;

	// seb 98.11.12 -- Added casts to jseVariable
	nav = var.Set(jsecontext,NULL,"navigator",jseTypeObject);

	jsePutString(jsecontext,var.Set(jsecontext,nav,"appCodeName",jseTypeString),
					 appCodeName);
	jsePutString(jsecontext,var.Set(jsecontext,nav,"appName",jseTypeString),
					 appName);
	jsePutString(jsecontext,var.Set(jsecontext,nav,"appVersion",jseTypeString),
					 appVersion);
	jsePutBoolean(jsecontext,var.Set(jsecontext,nav,JAVA_ENABLED,jseTypeBoolean),
					javaEnabled);

	// seb 99.01.10 -- Added extra info.
	jsePutLong(jsecontext,var.Set(jsecontext,nav,"appSecurityBits",jseTypeNumber),
			securityBits);
	jsePutString(jsecontext,var.Set(jsecontext,nav,"appCopyrightString",jseTypeString),
			copyrightInfo);

	buf = jseMustMalloc(char,strlen(appCodeName)+strlen(appVersion)+2);
	sprintf(buf,"%s/%s",appCodeName,appVersion);
	jsePutString(jsecontext,var.Set(jsecontext,nav,"userAgent",jseTypeString),
					 buf);
	jseMustFree(buf);
}


/* This routine works just like browserAddMimeTypes(); you can call it as many
 * times as needed, and the entire list is added each time, overwriting any old
 * values.
 */
void browserAddPlugin(jseContext jsecontext,struct Plugin *plugin)
{
	jseVariable nav, list, newlist, pl, newpl, mime, value;
	ulong count = 0;
	struct SEMimeType *loop;
	VarWrapper var;

	// seb 98.11.12 -- Added casts to jseVariable
	nav = var.Set(jsecontext,NULL,"navigator",jseTypeObject);
	list = jseGetMember(jsecontext,nav,"plugins");
	if( list==NULL )
	{
		newlist = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);
		list = jseMember(jsecontext,list,"plugins",jseTypeUndefined);
		jseAssign(jsecontext,list,newlist);
		jseDestroyVariable(jsecontext,newlist);
	}

	while( plugin )
	{
		pl = jseGetMember(jsecontext,list,plugin->name);
		if( pl==NULL )
		{
			/* add a new plugin both as 'name' and as the next array element. */
			newpl = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);
			pl = jseMember(jsecontext,list,plugin->name,jseTypeUndefined);
			jseAssign(jsecontext,pl,newpl);
			pl = jseIndexMember(jsecontext,list,jseGetArrayLength(jsecontext,list,NULL),
										jseTypeUndefined);
			jseAssign(jsecontext,pl,newpl);
			jseDestroyVariable(jsecontext,newpl);
		}

		jseSetArrayLength(jsecontext,pl,0,0);		 /* erase all old mime-type information */

		VarWrapper var;

		jsePutString(jsecontext,(jseVariable)(var.Set(jsecontext,pl,"description",jseTypeString)),
						 plugin->description);
		jseSetAttributes(jsecontext,var,jseReadOnly);
		jsePutString(jsecontext,(jseVariable)(var.Set(jsecontext,pl,"filename",jseTypeString)),
						 plugin->filename);
		jseSetAttributes(jsecontext,var,jseReadOnly);
		jsePutString(jsecontext,(jseVariable)(var.Set(jsecontext,pl,"name",jseTypeString)),
						 plugin->name);
		jseSetAttributes(jsecontext,var,jseReadOnly);

		count = 0;
		loop = plugin->types;
		while( loop )
		{
			mime = jseIndexMember(jsecontext,pl,count++,jseTypeUndefined);

			value = browserCreateMimeObject(jsecontext,loop,pl);
			jseAssign(jsecontext,mime,value);
			jseDestroyVariable(jsecontext,value);

			loop = loop->next;
		}

		plugin = plugin->next;
	}
}


static jseVariable browserCreateMimeObject(jseContext jsecontext,struct SEMimeType *type,
														 jseVariable referer)
{
	jseVariable obj;
	
	VarWrapper var;

	obj = jseCreateVariable(jsecontext,jseTypeObject);

	// seb 98.11.12 -- Added casts to jseVariable
	jsePutString(jsecontext,(jseVariable)(var.Set(jsecontext,obj,"description",jseTypeString)),
					 type->description);
	jseSetAttributes(jsecontext,var,jseReadOnly);
	jsePutString(jsecontext,(jseVariable)(var.Set(jsecontext,obj,"suffixes",jseTypeString)),
					 type->suffixes);
	jseSetAttributes(jsecontext,var,jseReadOnly);
	jsePutString(jsecontext,(jseVariable)(var.Set(jsecontext,obj,"type",jseTypeString)),
					 type->type);
	jseSetAttributes(jsecontext,var,jseReadOnly);
	var.Set(jsecontext,obj,"enabledPlugin",jseTypeNull);
	if( referer ) jseAssign(jsecontext,var,referer);

	return obj;
}


/* This routine can be called any number of times. All types in the linked list
 * started by 'type' are added. If you call it again, the new types are added,
 * and any duplicates overwrite the original value.
 */
void browserAddMimeType(jseContext jsecontext,struct SEMimeType *type)
{
	VarWrapper var;
	bool createdList = false;
	// seb 98.11.12 -- Added casts to jseVariable
	jseVariable nav = var.Set(jsecontext,NULL,"navigator",jseTypeObject);
	jseVariable list = jseGetMember(jsecontext,nav,"mimeTypes");
	if( list==NULL )
	 {
		jseVariable newlist = (jseVariable)CreateNewObject(jsecontext,ARRAY_PROPERTY);
		list = jseMemberEx(jsecontext,list,"mimeTypes",jseTypeUndefined,jseCreateVar);
		createdList = true;
		jseAssign(jsecontext,list,newlist);
		jseDestroyVariable(jsecontext,newlist);
	 }

	while( type )
	 {
		jseVariable pl = jseGetMember(jsecontext,list,type->type);
		jseVariable value = browserCreateMimeObject(jsecontext,type,NULL);

		if( pl==NULL )
			{
			 VarWrapper pl(jsecontext, jseMemberEx(jsecontext,list,type->type,jseTypeUndefined, jseCreateVar));
			 jseAssign(jsecontext,pl,value);
			 pl = jseIndexMemberEx(jsecontext,list,jseGetArrayLength(jsecontext,list,NULL),
										jseTypeUndefined, jseCreateVar);
			}

		jseAssign(jsecontext,pl,value);
		jseDestroyVariable(jsecontext,value);

		type = type->next;
	 }
	if (createdList)
		jseDestroyVariable(jsecontext, list);
}

/* ----------------------------------------------------------------------
 * The one function in 'navigator'
 * ---------------------------------------------------------------------- */


static jseLibFunc(Navigatorjava)
{
	jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);

	jseReturnVar(jsecontext,jseMember(jsecontext,thisvar,JAVA_ENABLED,jseTypeBoolean),
					jseRetCopyToTempVar);
}


/* ---------------------------------------------------------------------- */
/* Set up all the generic prototypes													*/
/* ---------------------------------------------------------------------- */


static CONST_DATA(struct jseFunctionDescription) BrowserLibFunctionList[] =
{
	/* ----------------------------------------------------------------------
	 * Add some browser-specific routines to the string object. Because
	 * strings are constructed via new, it is correct to put these particular
	 * routines in 'prototype', not '_prototype'
	 * ---------------------------------------------------------------------- */

	JSE_LIBMETHOD("String.prototype.anchor", Stringanchor,	1,	 1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.big",	 Stringbig,		0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.blink",	Stringblink,	 0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.bold",	Stringbold,		0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.fixed",	Stringfixed,	 0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.fontcolor", Stringfontcolor,1, 1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.fontsize", Stringfontsize,1,	1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.italics",Stringitalics,	0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.link",	Stringlink,		1,	 1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.small",	Stringsmall,	 0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.strike", Stringstrike,	0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.sub",	 Stringsub,		0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("String.prototype.sup",	 Stringsup,		0,	 0,		jseDontEnum,	 jseFunc_Secure ),


	/* ----------------------------------------------------------------------
	* The single method in navigator
	* ---------------------------------------------------------------------- */

	JSE_LIBMETHOD("navigator.javaEnabled",	Navigatorjava,	0,	 0,		jseDontEnum,	 jseFunc_Secure ),


	/* ----------------------------------------------------------------------
	* Set up the window prototype object
	* ---------------------------------------------------------------------- */

	JSE_LIBMETHOD("_prototype.alert",	Windowalert,	 1,	 1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("_prototype.blur",	Windowblur,		0,	 0,		jseDontEnum,	 jseFunc_Secure ),
// seb 99.2.4 - Changed capitalization
	JSE_LIBMETHOD("_prototype.clearTimeout",Windowcleartimeout,1,1,	jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("_prototype.close",	Windowclose,	 0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("_prototype.confirm",Windowconfirm,	1,	 1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("_prototype.focus",	Windowfocus,	 0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("_prototype.open",	Windowopen,		0,	 4,		jseDontEnum,	 jseFunc_Secure ),
// seb 99.2.3 - Added
	JSE_LIBMETHOD("_prototype.prompt", Windowprompt,	 1,		2,		jseDontEnum,		jseFunc_Secure ),
	JSE_LIBMETHOD("_prototype.scroll", Windowscroll,	2,	 2,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("_prototype.moveBy", Windowmoveby,	2,	 2,		jseDontEnum,	 jseFunc_Secure ),
// seb 98.11.14 - Changed capitalization
	JSE_LIBMETHOD("_prototype.setTimeout",Windowsettimeout,2,2,		jseDontEnum,	 jseFunc_Secure ),


	/* ----------------------------------------------------------------------
	* The location object has a couple of methods we store in
	* 'location.prototype' for convenience
	* ---------------------------------------------------------------------- */

	JSE_LIBMETHOD("location._prototype.reload", Locationreload,0,	 1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("location._prototype.replace",	Locationreplace,	 1,	 1,		jseDontEnum,	 jseFunc_Secure ),

	/* ----------------------------------------------------------------------
	* We do the same for the 'history' object
	* ---------------------------------------------------------------------- */

	JSE_LIBMETHOD("history._prototype.back",	Historyback,	 0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("history._prototype.forward",Historyforward,0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("history._prototype.go",	 Historygo,		1,	 1,		jseDontEnum,	 jseFunc_Secure ),


	/* ----------------------------------------------------------------------
	* document properties
	* ---------------------------------------------------------------------- */

	JSE_LIBMETHOD("document._prototype.close",Documentclose,	0,	 0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("document._prototype.open", Documentopen,	0,	 1,		jseDontEnum,	 jseFunc_Secure ),
	/* clear() is deprecated, and we map it to open() just like IE does */
	JSE_LIBMETHOD("document._prototype.clear",Documentopen,	0,	 1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("document._prototype.write",Documentwrite,	0,	-1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("document._prototype.writeln",Documentwriteln,0, -1,		jseDontEnum,	 jseFunc_Secure ),

	/* ----------------------------------------------------------------------
	* form properties
	* ---------------------------------------------------------------------- */

	JSE_LIBMETHOD("form._prototype.reset", Formreset,		0,		0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("form._prototype.submit",Formsubmit,		0,		0,		jseDontEnum,	 jseFunc_Secure ),

	/* ----------------------------------------------------------------------
	* Element properties
	* ---------------------------------------------------------------------- */

// seb 99.3.16 - Changing Element in the prototype declarations from capitalized to lower case.
	JSE_LIBMETHOD("element._prototype.blur",Elementblur,	0,		0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("element._prototype.click",Elementclick, 0,		0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("element._prototype.focus",Elementfocus, 0,		0,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("element._prototype.select",Elementselect,0,		0,		jseDontEnum,	 jseFunc_Secure ),
	
	/* ----------------------------------------------------------------------
	* Layer properties
	* ---------------------------------------------------------------------- */
	JSE_LIBMETHOD("layer._prototype.captureEvents",Layercaptureevents,	1,		1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.handleEvent",Layerhandleevent,		1,		1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.load",Layerload,					2,		2,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.moveAbove",Layermoveabove,			1,		1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.moveBelow",Layermovebelow,			1,		1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.moveBy",Layermoveby,				2,		2,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.moveTo",Layermoveto,				2,		2,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.moveToAbsolute",Layermovetoabsolute,2,		2,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.releaseEvents",Layerreleaseevents,	1,		1,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.resizeBy",Layerresizeby,			2,		2,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.resizeTo",Layerresizeto,			2,		2,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("layer._prototype.routeEvent",Layerrouteevent,		1,		1,		jseDontEnum,	 jseFunc_Secure ),

	/* ----------------------------------------------------------------------
	* Constructors
	* ---------------------------------------------------------------------- */
	JSE_LIBMETHOD("Option",					Optionconstruct,0,		4,		jseDontEnum,	 jseFunc_Secure ),
	JSE_LIBMETHOD("Image",					 Imageconstruct, 0,		2,		jseDontEnum,	 jseFunc_Secure ),

	JSE_FUNC_END
};

	void
InitializeInternalLib_BrowserLib(jseContext jsecontext)
{
#warning Put_this_back_in
// 	jseAddLibrary(jsecontext,NULL,BrowserLibFunctionList,NULL,NULL,NULL);
// seb 98.11.14 -- Added this here to set up the ECMA objects.	Where should it really go?
	LoadLibrary_All(jsecontext);
}

#endif
