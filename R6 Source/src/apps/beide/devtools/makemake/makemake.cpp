// ---------------------------------------------------------------------------
/*
	makemake.cpp
		
	...original makemake by Jon Watte

	Create a textual make file from a currently active project in the BeIDE

*/
// ---------------------------------------------------------------------------

#include <Application.h>
#include <Entry.h>
#include <Window.h>
#include <StringView.h>
#include <Messenger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Message.h>
#include <Messenger.h>
#include <Beep.h>
#include <Path.h>

#include "MScripting.h"


// ---------------------------------------------------------------------------
//	static strings
// ---------------------------------------------------------------------------

const char kBeginComment = '#';

const char* kTemplateFileName = "/boot/develop/etc/makefile";

const char* kNameKeyword = "NAME";
const char* kTypeKeyword = "TYPE";
const char* kSourcesKeyword = "SRCS";
const char* kResourcesLine = "RSRCS";
const char* kLibrariesLine = "LIBS";
const char* kIncludeLine = "LOCAL_INCLUDE_PATHS";

// ---------------------------------------------------------------------------
//	makefile template matches
// ---------------------------------------------------------------------------

enum EKeywordKind {kNone = 0, kAppName, kType, kSources, kResources, kLibraries, kInclude};

class KeywordMatch 
{
public:
	const char* 	fKeyword;
	EKeywordKind	fKind;
};

KeywordMatch gMatchList[] =
{
	{kNameKeyword, 		kAppName	},
	{kTypeKeyword,		kType		},
	{kSourcesKeyword, 	kSources	},
	{kResourcesLine, 	kResources	},
	{kLibrariesLine, 	kLibraries	},
	{kIncludeLine, 		kInclude	},
	{NULL,				kNone		}
};
	

// ---------------------------------------------------------------------------
//	global variables
// ---------------------------------------------------------------------------

BWindow *gWindow;

BStringView *gStringView;

FILE* gTemplatefile = NULL;
FILE *gMakefile = NULL;

bool gDone = false;

char gAppName[256];

// ---------------------------------------------------------------------------

static void
ShowProgress(const char* data)
{
	fprintf(stdout, "%s\n", data);
	if (!gWindow) {
		gWindow = new BWindow(
			BRect(200,100,500,200),
			"Make makefile progress",
			B_TITLED_WINDOW,
			B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
		gWindow->Lock();
		gStringView = new BStringView(
			gWindow->Bounds(),
			"progress",
			"");
		gWindow->AddChild(gStringView);
		gWindow->Unlock();
		gWindow->Show();
	}
	gWindow->Lock();
	gStringView->SetText(data);
	gWindow->Unlock();
}

// ---------------------------------------------------------------------------

static char *
strqdup(
	char *	string)
{
	char *ptr = string;
	int size = 0;
	while (*ptr)
	{
		if ((*ptr == ' ') || (*ptr == '\'') || (*ptr == '\"') ||
			(*ptr == '?') || (*ptr == '$') || (*ptr == '*') ||
			(*ptr == '\\'))
		{
			size++;
		}
		size++;
		ptr++;
	}
	char *ret = (char *)malloc(size+1);
	ptr = string;
	char *dst = ret;
	while (*ptr)
	{
		if ((*ptr == ' ') || (*ptr == '\'') || (*ptr == '\"') ||
			(*ptr == '?') || (*ptr == '$') || (*ptr == '*') ||
			(*ptr == '\\'))
		{
			*(dst++) = '\\';
		}
		*(dst++) = *(ptr++);
	}
	*dst = 0;
	return ret;
}

// ---------------------------------------------------------------------------
//	class app_file
//	one of these is created for each file in the project
//	they are linked together in a linked list as they are created
// ---------------------------------------------------------------------------


class app_file {
public:
	app_file() { name = lib = resname = NULL; deps = next = NULL; }
	~app_file() { if (name) free(name); if (lib) free (lib); if (resname) free(resname); }
	char *				name;
	char *				lib;
	char *				resname;
	struct app_file *	deps;
	struct app_file *	next;
};

app_file *gAppFiles = NULL;
app_file *gIncludes = NULL;

// ---------------------------------------------------------------------------

static void
get_dependencies(BMessenger & app, const char* curpath, int32 ix, app_file* af)
{
	// Find dependencies
	BMessage dependencies;
	PropertyItem item;
	unsigned long type;

	BMessage* msg = new BMessage(kGetVerb);
	strcpy(item.property, "dependencies");
	item.form = formDirect;
	msg->AddData("target", PROPERTY_TYPE, &item, sizeof(item));
	strcpy(item.property, "file");
	item.form = formIndex;
	item.data.index = ix+1;
	msg->AddData("target", PROPERTY_TYPE, &item, sizeof(item));
	strcpy(item.property, "project");
	item.form = formDirect;
	msg->AddData("target", PROPERTY_TYPE, &item, sizeof(item));
	if (!app.SendMessage(msg, &dependencies))
	{
		delete msg;
		/*	Iterate over dependencies	*/
		long num_deps = 0;
		if (dependencies.GetInfo("data", &type, &num_deps) < 0)
		{
			ShowProgress("Dependencies not returned!");
			return;
		}
		if (num_deps && (type != B_REF_TYPE))
		{
			ShowProgress("Dependencies are not entry_ref!");
			return;
		}
		for (int iy=0; iy<num_deps; iy++)
		{
			entry_ref dep_ref;
			if (dependencies.FindRef("data", iy, &dep_ref))
			{
				ShowProgress("Could not find dependency");
				return;
			}
			BEntry file(&dep_ref);
			BPath temppath;
			if (file.GetPath(&temppath))
			{
				ShowProgress("Could not get dependency path");
				return;
			}
			char deppath[1024];
			strcpy(deppath, temppath.Path());
			fprintf(stdout, "got dependency %d %s\n", iy, deppath);
			/*	look for system headers	*/
			char *dsl = strchr(&deppath[1], '/');
			if (!strncmp(dsl, "/develop", 8) && strncmp(deppath, curpath, strlen(curpath)))
			{
				dsl = strrchr(deppath, '/')+1;
//				fprintf(stdout, "ignoring system header: %s\n", dsl);
			}
			else
			{
				/*	Find relative path for dependencies	*/
				if (!strncmp(deppath, curpath, strlen(curpath)))
				{
					dsl = &deppath[strlen(curpath)];
				}
				else
				{
					dsl = deppath;
				}
				/*	record dependency	*/
				app_file *hed = new app_file;
				hed->name = strqdup(dsl);
				hed->next = af->deps;
				af->deps = hed;
	
				/*	record header path	*/
				char *end = strrchr(dsl, '/');
				if (end)
				{
					end[1] = 0;
					bool prev = false;
					/*	test for previous path	*/
					for (app_file *xip = gIncludes; xip != NULL; xip = xip->next)
					{
						if (!strcmp(xip->name, dsl))
						{
							prev = true;
							break;
						}
					}
					if (!prev)
					{
						app_file *ipa = new app_file;
						ipa->name = strqdup(dsl);
						ipa->next = gIncludes;
						gIncludes = ipa;
					}
				}
			}
		}
	}
	else
	{
		delete msg;
		ShowProgress("Could not get dependencies!");
	}
}
	
	
	// ---------------------------------------------------------------------------

static void
get_all_files(BMessenger & app)
{
	//	We need to know where we are, so we can ignore that prefix
	char curpath[1024];
	getcwd(curpath, 1024);
	char *lastPtr = &curpath[strlen(curpath)];
	if (lastPtr[-1] != '/')
	{
		*lastPtr++ = '/';
		*lastPtr = 0;
	}

	// Find the application files and their dependencies
	BMessage *msg = new BMessage(kGetVerb);
	PropertyItem item;
	strcpy(item.property, "files");
	item.form = formDirect;
	msg->AddData("target", PROPERTY_TYPE, &item, sizeof(item));
	strcpy(item.property, "project");
	item.form = formDirect;
	msg->AddData("target", PROPERTY_TYPE, &item, sizeof(item));
	BMessage file_list;
	if (app.SendMessage(msg, &file_list))
	{
		delete msg;
		ShowProgress("Cannot get files from BeIDE project");
		return;
	}
	delete msg;

	unsigned long type;
	long num_files;
	file_list.GetInfo("data", &type, &num_files);
	if (type != B_REF_TYPE)
	{
		ShowProgress("Files not returned as entry_ref!");
		return;
	}
	
	//
	// iterate through the entry_refs and build the list of all files
	//
	for (int ix=0; ix<num_files; ix++)
	{
		entry_ref ref;
		if (file_list.FindRef("data", ix, &ref))
		{
			ShowProgress("Returned file not found?");
			continue;
		}
		BEntry file(&ref);
		BPath path;
		if (file.GetPath(&path))
		{
			ShowProgress("Returned file has no path?");
			continue;
		}
		
		// Get the relative path name to current directory
		char* relfilepath;
		char filepath[1024];
		strcpy(filepath, path.Path());
		if (!strncmp(curpath, filepath, strlen(curpath)))
		{
			relfilepath = &filepath[strlen(curpath)];
		}
		else
		{
			relfilepath = filepath;
		}
				
		// Get the leaf name of the file
		char leafName[256];
		strcpy(leafName, strrchr(filepath, '/')+1);
		char* periodPtr = strrchr(leafName, '.');
		
		// Create our file information based on file name

		app_file *af = new app_file;
		if (periodPtr == NULL)
		{
			// ignore this file name (no extension)
			fprintf(stdout, "ignoring: %s\n", leafName);
			delete af;
			af = NULL;
		}
		else if (!strcmp(periodPtr, ".lib") || !strcmp(periodPtr, ".o"))
		{
			// file is user library - add to list of libs
			fprintf(stdout, "user lib/object: %s\n", leafName);
			af->lib = strqdup(relfilepath);
		}
		else if (!strcmp(periodPtr, ".so") || !strcmp(periodPtr, ".a"))
		{
			if (strncmp(leafName, "lib", 3) == 0) {
				// library is libXXX.so or libXXX.a so add XXX
				fprintf(stdout, "system lib: %s\n", leafName);
				char rootName[256];
				strcpy(rootName, &leafName[3]);
				char* ptr = strrchr(rootName, '.');
				*ptr = 0;
				af->lib = strqdup(rootName);
			}
			else {
				// not a libXXX.so or libXXX.a so add path name
				fprintf(stdout, "user lib: %s\n", leafName);
				af->lib = strqdup(relfilepath);
			}
		}
		else if (!strncmp(periodPtr, ".pch", 4))
		{
			fprintf(stdout, "ignoring: %s\n", leafName);
		}
		else if (!strncmp(periodPtr, ".rsrc", 5))
		{
			fprintf(stdout, "resource file: %s\n", leafName);
			af->resname = strqdup(relfilepath);
		}
		else if (!strncmp(periodPtr, ".sh", 3))
		{
			fprintf(stdout, "ignoring: %s\n", leafName);
		}
		else if (!strncmp(periodPtr, ".h", 2))
		{
			fprintf(stdout, "ignoring: %s\n", leafName);
		}
		else
		{
			// file is regular file
			fprintf(stdout, "source file:%s\n", leafName);
			af->name = strqdup(relfilepath);
		
			get_dependencies(app, curpath, ix, af);	
		}
		if (af != NULL)
		{
			af->next = gAppFiles;
			gAppFiles = af;
		}
	}
	if (!gAppFiles)
	{
		ShowProgress("There are no files!");
		return;
	}
}

// ---------------------------------------------------------------------------

static EKeywordKind
do_match(const char* string)
{
	EKeywordKind which = kNone;
	
	// check for comment lines quickly...
	if (string[0] == kBeginComment) {
		return which;
	}
	
	for (int32 i = 0; gMatchList[i].fKeyword != NULL; i++) {
		int32 matchLength = strlen(gMatchList[i].fKeyword);
		if (strncmp(gMatchList[i].fKeyword, string, matchLength) == 0) {
			which = gMatchList[i].fKind;
			break;
		}
	}
	
	return which;
}

// ---------------------------------------------------------------------------

static void
do_substitution(const char* currentLine, EKeywordKind which)
{
	// take off the newline from our currentline, and then let
	// each case decide how they want to end the line
	
	char* newString = new char[strlen(currentLine) + 1];
	strcpy(newString, currentLine);
	char* ptr = strrchr(newString, '\n');
	if (ptr) {
		*ptr = 0;
	}
	fprintf(gMakefile, "%s", newString);
	
	
	// the VAR= is written, now add whatever is necessary for that variable
	switch (which) {
		case kAppName:
			fprintf(gMakefile, gAppName);
			break;
		
		case kType:
			fprintf(gMakefile, "APP\n## (makemake defaults to APP type) ##");
			break;
			
		case kSources:
			for (app_file *fl = gAppFiles; fl != NULL; fl = fl->next) {
				if (fl->name) {
					fprintf(gMakefile, " \\\n\t%s", fl->name);
				}
			}
			break;
			
		case kResources:
			for (app_file *fl = gAppFiles; fl != NULL; fl = fl->next) {
				if (fl->resname) {
					fprintf(gMakefile, " \\\n\t%s", fl->resname);
				}
			}
			break;
			
		case kLibraries:
			for (app_file *fl = gAppFiles; fl != NULL; fl = fl->next) {
				if (fl->lib) {
					fprintf(gMakefile, " \\\n\t%s", fl->lib);
				}
			}
			break;
			
		case kInclude:
			for (app_file *ipa = gIncludes; ipa != NULL; ipa = ipa->next) {
				fprintf(gMakefile, "\\\n\t%s ", ipa->name);
			}
			break;
			
		case kNone:
			break;
	}

	fprintf(gMakefile, "\n");
	delete newString;
}

// ---------------------------------------------------------------------------

const int32 kMaxLineLength = 2048;

static void
write_make_file()
{
	// Read each line of the template file
	// If we find a line that we know about, then stuff with our file names
	// Then continue on with the template
	
	fprintf(gMakefile, "## makefile generated by makemake ##\n");
	char currentLine[kMaxLineLength];
	while (fgets(currentLine, kMaxLineLength, gTemplatefile) != NULL) {
		EKeywordKind which = do_match(currentLine);
		if (which != kNone) {
			do_substitution(currentLine, which);
		}
		else {
			fputs(currentLine, gMakefile);
		}
	}
}
	

// ---------------------------------------------------------------------------

static void
get_project_name(
	BMessenger &	ide,
	char *			name)
{
	BMessage msg(kGetVerb);
	PropertyItem item;
	strcpy(item.property, "name");
	item.form = formDirect;
	msg.AddData("target", PROPERTY_TYPE, &item, sizeof(item));
	strcpy(item.property, "project");
	item.form = formDirect;
	msg.AddData("target", PROPERTY_TYPE, &item, sizeof(item));
	BMessage proj_name;
	int didit = 0;
	if (!ide.SendMessage(&msg, &proj_name))
	{
		uint32 type;
		int32 num_names = 0;
		if (!proj_name.GetInfo("data", &type, &num_names))
		{
			char *str = NULL;
			if (!proj_name.FindData("data", type, (const void**) &str, &num_names) && (str != NULL))
			{
				didit = 1;
				strncpy(name, str, num_names);
				name[num_names] = 0;
				str = strrchr(name, '.');
				if (str)
					*str = 0;
			}
			else
			{
				fprintf(stdout, "get_project_name::FindString, %.4s\n", (char*) &type);
			}
		}
		else
		{
			fprintf(stdout, "get_project_name::GetInfo\n");
		}
	}
	else
	{
		fprintf(stdout, "get_project_name::SendMessage\n");
	}
	if (!didit)
		strcpy(name, "Application");
}

// ---------------------------------------------------------------------------

static void
DoMakeMake()
{
	BMessenger msg("application/x-mw-BeIDE", -1, NULL);
	if (!msg.IsValid()) {
		throw "Can't create messenger";
	}

	// open template file
	gTemplatefile = fopen(kTemplateFileName, "r");
	if (gTemplatefile == NULL) {
		char buffer[256];
		sprintf(buffer, "Can't open file: %s", kTemplateFileName);
		throw buffer;
	}
	
	// open output make file
	gMakefile = fopen("makefile", "w");
	if (gMakefile == NULL) {
		throw "Can't create 'makefile'";
	}
	
	// get the project name
	get_project_name(msg, gAppName);
	
	// iterate through all the files
	get_all_files(msg);
	
	// now write the final makefile
	write_make_file();
	
	// clean up
	fflush(gMakefile);
	fclose(gTemplatefile);
	fclose(gMakefile);
	ShowProgress("I'm all done!");
}

// ---------------------------------------------------------------------------

static long
DoMakeMakeThread(void* /* data */)
{
	try {
		DoMakeMake();
	}
	catch (const char* message) {
		ShowProgress(message);
	}
	catch (...) {
		ShowProgress("Unknown error during makemake");
	}
	gDone = true;
	return 0;
}

// ---------------------------------------------------------------------------

class MakeApp :
	public BApplication
{
public:
								MakeApp(
									const char* signature) :
									BApplication(signature)
								{
								}
		void					ReadyToRun()
								{
									BApplication::ReadyToRun();
									resume_thread(
										spawn_thread(
											DoMakeMakeThread,
											"DoMakeMake",
											B_NORMAL_PRIORITY,
											NULL));
								}
		bool					QuitRequested()
								{
									return gDone;
								}
};


int
main(
	int argc,
	char * argv[])
{
	if (argv[1] && strcmp(argv[1], "--help") == 0) {
		fprintf(stdout, "usage: makemake\n");
		fprintf(stdout, "generates make file for the currently active BeIDE project.\n");
		fprintf(stdout, "the currently active BeIDE project should be correctly built before makemake is run.\n");
		fprintf(stdout, "use Alt-Q to quit\n");
		exit(1);
	}
	MakeApp app("application/x-vnd.mw.makemake");
	ShowProgress("Creating makefile");
	app.Run();
}
