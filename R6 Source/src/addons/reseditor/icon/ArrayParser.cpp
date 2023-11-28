#include "ArrayParserState.h"
#include "array_scanner.h"

#include <StorageKit.h>
#include <Autolock.h>
#include <Debug.h>

extern void yyrestart( FILE *input_file );
extern FILE *yyin;
extern int yylineno;
extern int yydebug;

namespace BPrivate {
	static BLocker gStateLock;
	static ArrayParserState* gArrayParserState = NULL;
}

// ----------------------- BArrayParser -----------------------

BArrayParser::BArrayParser()
	: fParser(0), fErrorCount(0)
{
	Init();
}

BArrayParser::~BArrayParser()
{
	FreeData();
}

status_t BArrayParser::Init()
{
	FreeData();
	
	fParser = new ArrayParserState(this);
	
	return B_OK;
}

status_t BArrayParser::Run(const char* data, size_t size)
{
	BAutolock l(gStateLock);
	
	fError.Init();
	fErrorCount = 0;
	
	status_t err = StartScanningString(data, size);
	if( err != B_OK ) {
		printf("Error parsing: %s\n", strerror(err));
		return err;
	}
	
	yylineno = 1;
	
	#if DEBUG
	yydebug = 1;
	#endif
	
	gArrayParserState = fParser;
	
	try {
		do {
			BMallocIO data;
			size_t bytes_per_entry = 0;
			BString identifier;
			BMessage meta_data;
			err = fParser->ReadArray(&data, &bytes_per_entry, &identifier, &meta_data);
			if( err == B_OK && data.BufferLength() > 0 ) {
				ReadArray(data.Buffer(), data.BufferLength(),
						  bytes_per_entry, identifier.String(), meta_data);
			}
		} while( err == B_OK );
	} catch(...) {
		fError.SetTo(B_ERROR, "Unexpected exception occurred");
	}
	
	StopScanningString();
	
	gArrayParserState = NULL;
	
	return B_OK;
}

status_t BArrayParser::ReadArray(const void* data, size_t size,
								 size_t bytes_per_entry,
								 const char* identifier,
								 const BMessage& meta_data)
{
	(void)data;
	(void)size;
	(void)bytes_per_entry;
	(void)identifier;
	(void)meta_data;
	return B_OK;
}

void BArrayParser::Error(const ErrorInfo& info)
{
	AddError(info);
}

void BArrayParser::Warn(const ErrorInfo& info)
{
	fprintf(stderr, "### Warning\n# %s\n", info.Message());
	fprintf(stderr, "\n#------------\nFile \"%s\"; Line %ld\n#-------------\n",
					info.File(), info.Line());
}

void BArrayParser::AddError(const ErrorInfo& info)
{
	if( info.Code() != B_OK ) {
		if( fErrorCount == 0 ) fError = info;
		fErrorCount++;
	}
}
	
size_t BArrayParser::CountErrors() const
{
	return fErrorCount;
}

const ErrorInfo& BArrayParser::ErrorAt(size_t idx) const
{
	return idx == 0 ? fError : fNoError;
}

void BArrayParser::FreeData()
{
	delete fParser;
	fParser = 0;
}
