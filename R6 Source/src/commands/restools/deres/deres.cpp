#ifndef _APPLICATION_H
#include <Application.h>
#endif

#include <File.h>
#include <NodeInfo.h>
#include <Resources.h>
#include <Autolock.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <Locker.h>
#include <String.h>

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <vector.h>

#include <ResourceParser.h>
#include <experimental/Order.h>

// Mime string of our application's signature.
static const char* AppSig = "application/x-vnd.Be.deres";

static int CompareValues(const BResourceItem& i1, const BResourceItem& i2)
{
	if( i1.Type() < i2.Type() ) return -1;
	if( i1.Type() > i2.Type() ) return 1;
	if( i1.ID() < i2.ID() ) return -1;
	if( i1.ID() > i2.ID() ) return 1;
	return 0;
}

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
"Usage: %s [-h] [-r] [-o <file>] [-w] <file> ...\n",
			argv[0]);

	fputs(
"\nOptions:\n"
"    --help (-h):          Show this message.\n"
"    --add-extension (-r): Add '.rdef' extension to output file name.\n"
"    --output <file> (-o): Set output file name.  If not set, the\n"
"                          default is the first input file name with\n"
"                          the extension set to '.rdef'.\n"
"    --no-warnings (-w):   Do not display any warning messages.\n"
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
	MyParser()
		: fNoWarnings(false), fExplicitNames(false), fNoNames(false)
	{
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
	bool fNoWarnings, fExplicitNames, fNoNames;
};

enum {
	EXPLICIT_NAMES = 256,
	NO_NAMES
};

void MainApp::ArgvReceived(int32 argc, char** argv)
{
	BString output_name;
	bool add_extension = false;
	bool merge = false;
	
	BResources resources;
	vector<BResourceItem> items;
	
	MyParser parser;
	
	int c;
	
	// Parse options
	while(1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"help",			no_argument,		0,	'h'},
			{"add-extension",	no_argument,		0,	'r'},
			{"output",			required_argument,	0,	'o'},
			{"merge",			no_argument,		0,	'm'},
			{"no-warnings",		no_argument,		0,	'w'},
			{0,0,0,0}
		};
		c = getopt_long (argc, argv, "h?romw", long_options, &option_index);
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
				if( add_extension ) output_name += ".rdef";
				break;
			case 'm':
				merge = true;
				break;
			case 'w':
				parser.SetNoWarnings(true);
				break;
			default:
				fprintf (stderr, "?? getopt returned character code 0%o ??\n", c);
		}
	}
	
	if( optind >= argc ) {
		SetResult(5);
		return;
	}
	
	if( output_name.Length() <= 0 ) {
		output_name = argv[optind];
		int32 p = output_name.FindLast('.');
		int32 s = output_name.FindLast('/');
		if( p != B_ERROR && p > s ) {
			output_name.Truncate(p);
		}
		output_name.Append(".rdef");
	}
	
	while( optind < argc ) {
		status_t err;
		
		BFile file;
		err = file.SetTo(argv[optind], B_READ_ONLY);
		if( err ) {
			fprintf(stderr, "*** Unable to open input file %s: %s\n",
					output_name.String(), strerror(err));
			SetResult(10, false);
			return;
		}
		err = resources.SetTo(&file, false);
		if( err ) {
			fprintf(stderr, "*** Unable to open resource file %s: %s\n",
					output_name.String(), strerror(err));
			SetResult(10, false);
			return;
		}
			
		resources.PreloadResourceType();
		type_code	type;
		int32		id;
		size_t		length;
		const char	*name;
		const void	*ptr;
		for(int32 i = 0;
				!err && resources.GetResourceInfo(i, &type, &id, &name, &length);
				i++) {
			if((ptr = resources.LoadResource(type, id, &length)) != 0) {
				items.push_back(BResourceItem());
				BResourceItem& it = items.back();
				it.SetType(type);
				it.SetID(id);
				it.ApplyLabel(name);
				it.SetData(ptr, length);
			}
		}
		
		optind++;
	}
	
	TArrayOrder<vector<BResourceItem>, BResourceItem> order(items,
															items.size(),
															CompareValues);
	
	status_t err = parser.StartWritingHeader(output_name.String());
	if( err ) {
		SetResult(10, false);
		return;
	}
	
	for( size_t i=0; i<items.size(); i++ ) {
		const BResourceItem& it = order.ValueAt(i);
		err = parser.WriteItem(&it);
		if( err ) {
			SetResult(10, false);
			return;
		}
	}
	
	err = parser.StopWriting();
	if( err ) {
		SetResult(10, false);
		return;
	}
	
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
