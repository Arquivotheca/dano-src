#ifndef _APPLICATION_H
#include <Application.h>
#endif

#include <File.h>
#include <NodeInfo.h>
#include <Resources.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <String.h>

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include <ResourceParser.h>

// Mime string of our application's signature.
static const char* AppSig = "application/x-vnd.Be.beres";

class MainApp : public BApplication
{
private:
	typedef BApplication inherited;
	
public:
	MainApp();
	~MainApp();
	
	virtual bool QuitRequested(void);
	virtual void AboutRequested(void);
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void RefsReceived(BMessage* message);
	virtual void ReadyToRun(void);
	
	void SetResult(int result, bool showUsage=true, bool quit=true);
	int Result() const					{ return fResult; }
	bool ShowUsage() const				{ return fShowUsage; }
	bool HaveResult() const				{ return fHaveResult; }
	
private:
	int fResult;
	bool fShowUsage;
	bool fHaveResult;
};

static void Usage(int /*argc*/, char **argv)
{
	fprintf(stderr,
"Usage: %s [-h] [-r] [-o <file>] [-m] [-D <symbol>[=<value>]]\n"
"       [-I <dir>] [-w] <file> ...\n",
			argv[0]);

	fputs(
"\nOptions:\n"
"    --help (-h):          Show this message.\n"
"    --add-extension (-r): Add '.rsrc' extension to output file name.\n"
"    --output <file> (-o): Set output file name.  If not set, the\n"
"                          default is the first input file name with\n"
"                          the extension set to '.rsrc'.\n"
"    --merge (-m):         Do not overwrite any existing resources in\n"
"                          output file.\n"
"    --define <symbol>[=<value>] (-D):\n"
"                          Set the value of <symbol> to <value>.  If\n"
"                          <value> is not supplied, set it to 1.\n"
"    --include <dir> (-I): Add <dir> to the list of include paths.\n"
"    --no-warnings (-w):   Do not display any warning messages.\n"
"    --explicit-names:     Only write resource names for resources in\n"
"                          source file that have an explicitly\n"
"                          specified name.  Otherwise, resource names\n"
"                          may be constructed from the ID symbol.\n"
"    --no-names:           Do not write any names in resource file.\n"
		, stderr);
}

int main(int argc, char **argv)
{
	MainApp myApplication;
	myApplication.Run();
	if( myApplication.ShowUsage() ) Usage(argc, argv);
	return(myApplication.Result());
}

MainApp::MainApp()
	: BApplication(AppSig),
	  fResult(10), fShowUsage(true), fHaveResult(false)
{
}

MainApp::~MainApp()
{
}

void MainApp::SetResult(int result, bool showUsage, bool quit)
{
	fResult = result;
	fShowUsage = showUsage;
	fHaveResult = true;
	if( quit ) PostMessage(B_QUIT_REQUESTED);
}

bool MainApp::QuitRequested(void)
{
	return true;
}

void MainApp::AboutRequested(void)
{
#if 0
	if( !mAboutWin.IsValid() ) {
		ArpAboutWindow* win = 
			new ArpAboutWindow(0, "printmsg", 0, __DATE__,
					"printmsg version " PROGRAM_VERSION
					" / " __DATE__ "\n"
					B_UTF8_COPYRIGHT "1999 Angry Red Planet Software.\n\n"
					
					"Print BMessage objects.  Wow.\n\n"
					
					"For more info and the latest version, see\n"
					"<URL:http://www.angryredplanet.com/>.\n\n"
					
					"Written by Dianne Hackborn\n"
					"(email: hackbod@angryredplanet.com)\n\n");

		mAboutWin = BMessenger(win);
		win->Show();
	}
#endif
}

class MyParser : public BResourceParser
{
public:
	MyParser(BResources& output)
		: fOutput(output),
		  fNoWarnings(false), fExplicitNames(false), fNoNames(false)
	{
	}

	virtual status_t ReadItem(BResourceItem* item)
	{
		WriteItem(item);
		
		if( item->Size() == 0 ) {
			ErrorInfo info;
			info.SetTo(0, "Skipping empty resource %s",
						TypeIDToString(item->Type(), item->ID()));
			Warn(info);
			delete item;
			return B_OK;
		}
		
		if( fOutput.HasResource(item->Type(), item->ID()) ) {
			ErrorInfo info;
			info.SetTo(0, "Resource %s already exists, ignoring new definition",
						TypeIDToString(item->Type(), item->ID()));
			Warn(info);
			delete item;
			return B_OK;
		}
		
		BString label;
		if( !fNoNames ) {
			if( fExplicitNames ) label = item->Name();
			else item->CreateLabel(&label);
		}
		status_t err = fOutput.AddResource(item->Type(), item->ID(),
										   item->Data(), item->Size(),
                                           label.Length() > 0 ? label.String() : 0);
		if( err ) {
			ErrorInfo info;
			info.SetTo(err, "Unable to add resource %s:\n%s",
					TypeIDToString(item->Type(), item->ID()), strerror(err));
			Error(info);
			return err;
		}
		
		delete item;
		
		return B_OK;
	}
	
	virtual void Error(const ErrorInfo& info)
	{
		fprintf(stderr, "### Error:\n");
		print_error(info);
		AddError(info);
	}
	
	virtual void Warn(const ErrorInfo& info)
	{
		if( !fNoWarnings ) {
			fprintf(stderr, "### Warning:\n");
			print_error(info);
		}
	}
	
	void print_error(const ErrorInfo& info)
	{
		BString msg(info.Message());
		int32 base = 0;
		do {
			int32 next = msg.FindFirst('\n', base);
			if( next < 0 ) next = msg.Length();
			fprintf(stderr, "%s:%ld: ", info.File(), info.Line());
			if( next > base ) {
				fwrite(msg.String()+base, 1, (size_t)(next-base), stderr);
			}
			fprintf(stderr, "\n");
			base = next+1;
		} while( base < msg.Length() );
	}
	
	void SetNoWarnings(bool state)		{ fNoWarnings = state; }
	void SetExplicitNames(bool state)	{ fExplicitNames = state; }
	void SetNoNames(bool state)			{ fNoNames = state; }
	
private:
	BResources& fOutput;
	bool fNoWarnings, fExplicitNames, fNoNames;
};

enum {
	EXPLICIT_NAMES = 256,
	NO_NAMES,
	DUMP_OPT
};

void MainApp::ArgvReceived(int32 argc, char** argv)
{
	BString output_name;
	BString dump_name;
	bool add_extension = false;
	bool merge = false;
	
	BResources resources;
	bool openedResources = false;
	
	MyParser parser(resources);
	BResourceContext context;
	
	int c;
	
	// Parse options
	while(1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"help",			no_argument,		0,	'h'},
			{"add-extension",	no_argument,		0,	'r'},
			{"output",			required_argument,	0,	'o'},
			{"merge",			no_argument,		0,	'm'},
			{"define",			required_argument,	0,	'D'},
			{"include",			required_argument,	0,	'I'},
			{"no-warnings",		no_argument,		0,	'w'},
			{"explicit-names",	no_argument,		0,	EXPLICIT_NAMES},
			{"no-names",		no_argument,		0,	NO_NAMES},
			{"dump",			required_argument,	0,	DUMP_OPT},
			{0,0,0,0}
		};
		c = getopt_long (argc, argv, "h?romDIw", long_options, &option_index);
		if (c == -1)
			break;
		switch(c) {
			case 'h':
			case '?':
				SetResult(0);
				return;
			case 'r':
				add_extension = true;
				break;
			case 'o':
				if( !optarg && optind < argc ) {
					optarg = argv[optind];
					optind++;
				}
				if( !optarg ) {
					SetResult(5);
					return;
				}
				output_name = optarg;
				if( add_extension ) output_name += ".rsrc";
				break;
			case 'm':
				merge = true;
				break;
			case 'D': {
				if( !optarg && optind < argc ) {
					optarg = argv[optind];
					optind++;
				}
				if( !optarg ) {
					SetResult(5);
					return;
				}
				char * eq = strchr(optarg, '=');
				if (eq) {
					BString name(optarg, (int32)(eq-optarg));
					eq++;
					char * out;
					int l = strtol(eq, &out, 10);
					if (out && !*out) parser.Define(name.String(), l);
					else parser.Define(name.String(), eq);
				}
				else {
					parser.Define(optarg);
				}
			} break;
			case 'I':
				if( !optarg && optind < argc ) {
					optarg = argv[optind];
					optind++;
				}
				if( !optarg ) {
					SetResult(5);
					return;
				}
				parser.PostInclude(optarg);
				break;
			case 'w':
				parser.SetNoWarnings(true);
				break;
			case DUMP_OPT:
				if( !optarg ) {
					SetResult(5);
					return;
				}
				dump_name = optarg;
				break;
			case EXPLICIT_NAMES:
				parser.SetExplicitNames(true);
				break;
			case NO_NAMES:
				parser.SetNoNames(true);
				break;
			default:
				fprintf (stderr, "?? getopt returned character code 0%o ??\n", c);
		}
	}
	
	if( optind >= argc ) {
		SetResult(5);
		return;
	}
	
	parser.GetContext(&context);
	
	if( dump_name.Length() > 0 ) {
		status_t err = parser.StartWritingHeader(dump_name.String());
		if( err ) {
			SetResult(10, false);
			return;
		}
	}
	
	while( optind < argc ) {
		status_t err;
		
		if( output_name.Length() <= 0 ) {
			output_name = argv[optind];
			int32 p = output_name.FindLast('.');
			int32 s = output_name.FindLast('/');
			if( p != B_ERROR && p > s ) {
				output_name.Truncate(p);
			}
			output_name.Append(".rsrc");
		}
		
		if( !openedResources ) {
			BFile file;
			err = file.SetTo(output_name.String(), B_READ_WRITE|B_CREATE_FILE);
			if( err ) {
				fprintf(stderr, "*** Unable to open ouput file %s: %s\n",
						output_name.String(), strerror(err));
				SetResult(10, false);
				return;
			}
			err = resources.SetTo(&file, !merge);
			if( err ) {
				fprintf(stderr, "*** Unable to open resource file %s: %s\n",
						output_name.String(), strerror(err));
				SetResult(10, false);
				return;
			}
			BNodeInfo	ni;
			if(ni.SetTo(&file) == B_OK)
				ni.SetType("application/x-be-resource");
			
			openedResources = true;
			
		} else {
			PRINT(("Resetting parser context...\n"));
			parser.Init();
			parser.SetContext(&context);
		}
		
		err = parser.SetTo(argv[optind]);
		if( err ) {
			fprintf(stderr, "*** Unable to open source file %s: %s\n",
					argv[optind], strerror(err));
			SetResult(10, false);
			return;
		}
		
		err = parser.Run();
		if( err ) {
			SetResult(10, false);
			return;
		}
		
		optind++;
	}
	
	parser.StopWriting();
	SetResult(0, false);
	
	return;
}

void MainApp::RefsReceived(BMessage* message)
{
	inherited::RefsReceived(message);
}

void MainApp::ReadyToRun()
{
	if( !HaveResult() ) {
		SetResult(5);
	}
}
