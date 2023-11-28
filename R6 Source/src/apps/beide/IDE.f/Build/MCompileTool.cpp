//========================================================================
//	MCompileTool.cpp
//	Copyright 1995 - 96  Metrowerks Corporation. All rights reserved.
//========================================================================	
//	Jon Watte, BS

#include <OS.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>

#include "MCompileTool.h"
#include "MCompileGenerator.h"
#include "MFileUtils.h"
#include "MFileCache.h"
#include "MAlert.h"
#include "BeIDEComm.h"
#include "CString.h"
#include "Utils.h"
#include "MMessageWindow.h"

#include <File.h>
#include <Debug.h>

static void print_argv(BList& inArgvList);
static void print_argv(const char ** argv);


MCompileTool::MCompileTool(
	const char * 	compiler,	// path to the tool
	BList& 			args,		// other compiler args
	bool 			IDEAware,	// does this tool communicate with the IDE?
	int32			threadPriority)
	: MCompile()
{
	fCompilerThread = -1;
	fServeThread = -1;
	fWaitThread = -1;
	fFromCompiler = -1;
	fToCompiler = -1;
	fIDEAware = IDEAware;
	fThreadPriority = threadPriority;
	
	// Check that the compiler is really there
	entry_ref		ref;

	fCompletionStatus = get_ref_for_path(compiler, &ref);
	if (fCompletionStatus != B_NO_ERROR)
	{
		String		text = "Couldn't find tool:\n";
		text += compiler;
		
		MAlert		alert(text);
		alert.Go();
		return;
	}
	
	String			commandline;
	/*	Create the arguments */

	/*	Create IDE communications for IDE aware tools */
	if (IDEAware) {
		fToCompiler = create_port(1, "To Compiler");
		if (fToCompiler < B_NO_ERROR) {
			fCompletionStatus = fToCompiler;
			fCompilerDone = TRUE;
			DeleteArgv(args);
			return;
		}
		fFromCompiler = create_port(4, "From Compiler");
		if (fFromCompiler < B_NO_ERROR) {
			delete_port(fToCompiler);
			fToCompiler = -1;
			fCompletionStatus = fFromCompiler;
			fCompilerDone = TRUE;
			DeleteArgv(args);
			return;
		}
		char ideStr[50];
		sprintf(ideStr, "%ld,%ld", (int32)fFromCompiler, (int32)fToCompiler);
		
		args.AddItem(strdup(compiler), 0L);
		args.AddItem(strdup("-beide"), 1L);
		args.AddItem(strdup(ideStr), 2L);
	}
	else
	{
		// Every arg needs to be quoted
		commandline = "\"";
		commandline += compiler;
		commandline += "\" ";
		for (int32 i = 0; i < args.CountItems(); i++)
		{
			char*	arg = (char*) args.ItemAt(i);
			if (arg != nil)
			{
				commandline += " \"";
				commandline += arg;
				commandline += "\"";
			}
		}

		// redirect output to a temp file
		commandline += "&>";
		commandline += MFileUtils::BootTmp();
		commandline += "/mwtmp";
		BEntry file;
		fIndex = MFileUtils::GetTempFile(file, nil, false);
		commandline += fIndex;
		
		DeleteArgv(args);
		args.AddItem(strdup("-noprofile"), 0L);
		args.AddItem(strdup("-c"), 1L);
		args.AddItem(strdup(commandline), 2L);
		
		// could use getenv and SHELL environment variable here ????
		commandline = MFileUtils::BinPath();
		commandline += "/sh";
		args.AddItem(strdup(commandline), 0L);
	}

#ifdef DEBUG
	print_argv(args);
#endif

	/*	Launch the program */
#if __POWERPC__
// this is already declared in the intel headers but not the ppc headers
	extern char ** environ;
#endif

#if DEBUG
//	print_argv(environ);
#endif

	PRINT(("About to load_image: args = %ld\n", args.CountItems()));
	fCompilerThread = load_image(args.CountItems(), (const char**) args.Items(), (const char**) environ);
	PRINT(("Launch, Compiler thread ID: %ld\n", fCompilerThread));

	DeleteArgv(args);
	
	if (fCompilerThread < B_NO_ERROR)
	{
		fCompletionStatus = fCompilerThread;
		fCompilerDone = TRUE;
		PRINT(("Coudn't start compiler. %ld\n", fCompilerThread));

		String		text = "Couldn't start tool:\n";
		text += compiler;
		text += "\nError = ";
		text += fCompilerThread;
		text += ".";
	
		MAlert		alert(text);
		alert.Go();

		return;
	}

	if (IDEAware)
	{
		/*	Compile needs a server to handle headers and status */
		fServeThread = spawn_thread(CompileServe, "CompileServe", B_NORMAL_PRIORITY, (void *)this);
		if (fServeThread < B_NO_ERROR)
		{
			kill_thread(fCompilerThread);
			fCompilerThread = -1;
			fCompletionStatus = fServeThread;
			fCompilerDone = FALSE;
			return;
		}
	}

	if (!IDEAware)
	{
		/*	We also wait for the tool, since it may not send a status message */
		fWaitThread = spawn_thread(CompileWait, "CompileWait", B_NORMAL_PRIORITY, (void *)this);
		if (fWaitThread < B_NO_ERROR)
		{
			kill_thread(fCompilerThread);
			fCompilerThread = -1;
			if (fServeThread >= B_NO_ERROR)
				kill_thread(fServeThread);
			fServeThread = -1;
			fCompletionStatus = fWaitThread;
			fCompilerDone = FALSE;
			return;
		}
	}
}

// ---------------------------------------------------------------------------
//		~MCompileTool
// ---------------------------------------------------------------------------

MCompileTool::~MCompileTool()
{
	if (Lock())
	{		
		if (! fCompilerDone)
		{
			Kill();
		}
		if (fToCompiler >= B_NO_ERROR)
			delete_port(fToCompiler);
		if (fFromCompiler >= B_NO_ERROR)
			delete_port(fFromCompiler);
	
		Unlock();
	}
}

// ---------------------------------------------------------------------------
//		Run
// ---------------------------------------------------------------------------

status_t
MCompileTool::Run()
{
	status_t 	err;

	if (fServeThread >= B_NO_ERROR)			// ide aware
		err = resume_thread(fServeThread);
	else
	if (fWaitThread >= B_NO_ERROR)			// non ide aware
		err = resume_thread(fWaitThread);	// Calling wait_for_thread will start the thread(!)
	else
		err = fServeThread;

	if (err == B_NO_ERROR) {
		set_thread_priority(fCompilerThread, fThreadPriority);
		err = resume_thread(fCompilerThread);
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		Kill
// ---------------------------------------------------------------------------

status_t
MCompileTool::Kill()
{
	if (Lock())
	{
		if (fCompilerThread >= B_NO_ERROR)
		{
			KillTeam(fCompilerThread);
			fCompilerThread = -1;
		}
	
		if (fWaitThread >= B_NO_ERROR)
		{
			kill_thread(fWaitThread);
			fWaitThread = -1;
		}
		if (fServeThread >= B_NO_ERROR)
		{
			kill_thread(fServeThread);
			fServeThread = -1;
		}
	
		MCompile::Kill();
		Unlock();
	}

	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		KillTeam
// ---------------------------------------------------------------------------
//	Static function called to asyncronously kill a team.

void
MCompileTool::KillTeam(
	thread_id	tid)
{
	if (tid >= B_NO_ERROR)
	{
		thread_info		info;

		if (B_NO_ERROR == get_thread_info(tid, &info))
		{
			status_t	err = send_signal(tid, SIGINT);
		}
	}
}

// ---------------------------------------------------------------------------
//		FindHeader
// ---------------------------------------------------------------------------

status_t
MCompileTool::FindHeader(
	const HeaderQuery& 	/*inQuery*/,
	HeaderReply& 		/*outReply*/)
{
	ASSERT(false);	// never happens on a link
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		DoPreprocesResult
// ---------------------------------------------------------------------------

void
MCompileTool::DoPreprocesResult(
	const SendTextMessage& /*textMessage*/)
{
}

// ---------------------------------------------------------------------------
//		CodeDataSize
// ---------------------------------------------------------------------------

void
MCompileTool::CodeDataSize(
	int32&	outCodeSize,
	int32&	outDataSize)
{
	outCodeSize = -1;
	outDataSize = -1;
}

// ---------------------------------------------------------------------------
//		GenerateDependencies
// ---------------------------------------------------------------------------

void
MCompileTool::GenerateDependencies()
{
}

// ---------------------------------------------------------------------------
//		GetArea
// ---------------------------------------------------------------------------

void
MCompileTool::GetArea(
	GetAreaReply& areaMessage)
{
	areaMessage.area = -1;
}

// ---------------------------------------------------------------------------
//		ParseMessageText
// ---------------------------------------------------------------------------
//	return B_ERROR if there are errors in the text and
//	B_NO_ERROR if there are only warnings.

status_t
MCompileTool::ParseMessageText(
	const char*	/*inText*/)
{
	ASSERT(false);
	
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		CompileWait
// ---------------------------------------------------------------------------
//	Handle compiles by non-ideaware tools.

status_t
MCompileTool::CompileWait(
	void * arg)
{
	MCompileTool * 	compile = (MCompileTool *)arg;
	status_t	status = B_NO_ERROR;
	status_t	ret = wait_for_thread(compile->fCompilerThread, &status);
	if (ret == B_NO_ERROR)
		compile->fCompilerThread = -1;
	// Could post a message if status is not zero ????
	// Status is almost always zero
	// We can only detect errors by the presence of the temp file

	// Get the output from stderr and stdout if it exists
	entry_ref		ref;
	String			filePath = MFileUtils::BootTmp();
	filePath +=  "/mwtmp";
	filePath += compile->fIndex;
	
	if (B_NO_ERROR == get_ref_for_path(filePath, &ref))
	{
		BEntry		entry(&ref);
		off_t		size;
		status_t	err;
		
		err = entry.GetSize(&size);

		if (err == B_NO_ERROR && size > 0)
		{
			BFile		file(&ref, B_READ_ONLY);

			if (B_OK == file.InitCheck() && file.IsReadable())
			{
				char*		text = new char[size+1];
				size = file.Read(text, size);
				if (size > 0)
				{
					text[size] = 0;
					err = compile->ParseMessageText(text);	// Send to the builder
					// handle case where there were no errors, but the tool
					// had an error exit status (especially with gcc where we treat
					// unknown messages as warnings, since we can't tell)
					if (err == B_OK && status != B_OK) {
						ErrorNotificationMessageShort message;
						strcpy(message.errorMessage, "Errors and/or warnings resulted in tool termination");
						message.hasErrorRef = false;
						message.isWarning = false;
						compile->DoMessage(message);
						err = B_ERROR;
					}
				}

				delete [] text;
			}

			// Set the status
			if (status == B_NO_ERROR && err != B_NO_ERROR)
				status = B_ERROR;
		}

		// Delete the temp file
		entry.Remove();
	}

	// Notify the compile object that the compile is completed
	CompilerStatusNotification	statusRec;
	
	statusRec.errorCode = status;
	statusRec.browseErrorCode = -1;
	statusRec.codeSize = -1;
	statusRec.dataSize = -1;
	statusRec.count = 0;
	
	if (status == B_NO_ERROR)
	{
		compile->CodeDataSize(statusRec.codeSize, statusRec.dataSize);	// Send to the builder
		compile->GenerateDependencies();
	}

	compile->DoStatus(statusRec);
	compile->DoStatus(status == B_NO_ERROR, status);

	// don't allow a kill to come in while we are marking the thread
	// as done and while we are deleting the compile object 
	// (the destructor will Unlock)
	compile->Lock();
	compile->fWaitThread = -1;
	delete compile;

	return status;
}

// ---------------------------------------------------------------------------
//		CompileServe
// ---------------------------------------------------------------------------

status_t
MCompileTool::CompileServe(
	void * arg)
{
	MCompileTool * compile = (MCompileTool *) arg;

	union {
		HeaderQuery					header;		// request for info on a header file
		ErrorNotificationMessage	message;	// description of an error
		StatusNotification			status;		// sent at the end of all source files in a compile
		CompilerStatusNotification	cstatus;	// sent after each source file in a compile
		SendTextMessage				text;		// used to transfer text
		SendTextReply				textReply;	// reply to SendTextMessage
		GetAreaReply				areaReply;	// return an area to the compiler
		SourceQuery					sourceQuery;// request from compiler for info about a source file
		SourceReply					sourceReply;// reply to source request
	} data;

	HeaderReply		reply;
	int32 			type;
	status_t		err = B_NO_ERROR;
	
	while (! compile->fCompilerDone)
	{
		err = read_port(compile->fFromCompiler, &type, &data, sizeof(data));
		
		if (err >= B_NO_ERROR)
		{
			compile->Lock();		// Don't let us be killed in this critical section

			switch (type)
			{
				case kNullQuery:	//	Ignore
					break;
	
				case kHeaderQuery:
					err = compile->FindHeader(data.header, reply);
					if (err != B_NO_ERROR)
						reply.errorCode = err;
					err = write_port(compile->fToCompiler, kHeaderReply, &reply, sizeof(reply));
					if (err != B_NO_ERROR)
						err = reply.errorCode;
					break;
		
				case kMessageNotification:
					err = compile->DoMessage(data.message);
					break;
	
				case kPreprocessResult:
					compile->DoPreprocesResult(data.text);
					data.textReply.doneWithArea = true;
					err = write_port(compile->fToCompiler, kPreprocessReply, &data, sizeof(data.textReply));
					break;
	
				case kCompilerStatusNotification:
					compile->DoStatus(data.cstatus);
					break;
	
				case kStatusNotification:
					compile->DoStatus(data.status.objProduced, data.status.errorCode);
					break;
	
				case kGetArea:
					compile->GetArea(data.areaReply);
					err = write_port(compile->fToCompiler, kAreaReply, &data, sizeof(data.areaReply));
					break;
	
				case kSourceQuery:
					data.sourceReply.errorCode = B_NO_ERROR;
					data.sourceReply.recordbrowseinfo = true;
					data.sourceReply.browserfileID = 1;
					data.sourceReply.browseoptions.recordClasses = false;
					data.sourceReply.browseoptions.recordEnums = false;
					data.sourceReply.browseoptions.recordMacros = false;
					data.sourceReply.browseoptions.recordTypedefs = false;
					data.sourceReply.browseoptions.recordConstants = false;
					data.sourceReply.browseoptions.recordTemplates = false;
					data.sourceReply.browseoptions.recordUndefinedFunctions = false;
					data.sourceReply.browseoptions.reserved1 = 0L;
					data.sourceReply.browseoptions.reserved2 = 0L;
					err = write_port(compile->fToCompiler, kSourceReply, &data, sizeof(data.sourceReply));
					break;
	
				default:
					ASSERT(!"Unknown message type in CompileServe!");
					break;
				}
				
				compile->Unlock();
			}

		if (err < B_NO_ERROR) 
		{
			compile->DoStatus(FALSE, err);
			break;
		}
	}

	compile->Lock();

	compile->fServeThread = -1;
	compile->fCompilerThread = -1;

	delete compile;

	return err;
}

static void
print_argv(
	BList&		inArgvList)
{
	for (int32 i = 0; i < inArgvList.CountItems(); i++)
	{
		char*	item = (char*) inArgvList.ItemAt(i);
		if (item)
			puts(item);
	}
}

static void
print_argv(
	const char ** argv)
{
	int cnt = 0;
	while (*argv) {
		puts(*argv++);
		cnt++;
	}
}
