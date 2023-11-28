#include <ResourceParser.h>
#include <ResParserState.h>

#include <StorageKit.h>
#include <Autolock.h>
#include <BufferIO.h>
#include <Debug.h>

#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

extern int yyparse(void* state);
extern void yyrestart( FILE *input_file );
extern FILE *yyin;
extern int yylineno;
extern int yydebug;

namespace BPrivate {
	static BLocker gStateLock;
	static ResourceParserState* gResParserState = NULL;
}

// ----------------------- BResourceContext -----------------------

BResourceContext::BResourceContext()
	: fContext(0)
{
}

BResourceContext::BResourceContext(const BResourceContext& o)
	: fContext(0)
{
	*this = o;
}

BResourceContext::BResourceContext(const ResourceParserContext& o)
	: fContext(0)
{
	*this = o;
}

BResourceContext::~BResourceContext()
{
	delete fContext;
	fContext = 0;
}

BResourceContext& BResourceContext::operator=(const BResourceContext& o)
{
	if( !o.fContext ) {
		delete fContext;
		fContext = 0;
	} else if( fContext ) {
		*fContext = *o.fContext;
	} else {
		fContext = new ResourceParserContext(*o.fContext);
	}
	return *this;
}

BResourceContext& BResourceContext::operator=(const ResourceParserContext& o)
{
	if( fContext ) *fContext = o;
	else fContext = new ResourceParserContext(o);
	return *this;
}

// ----------------------- BResourceParser -----------------------

BResourceParser::BResourceParser()
	: fParser(0), fErrorCount(0), fWriteDest(0), fWriteHeader(0)
{
	Init();
}

BResourceParser::~BResourceParser()
{
	FreeData();
}

status_t BResourceParser::Init()
{
	FreeData();
	
	fParser = new ResourceParserState(this);
	
	Define("__BERES__", 200);
	Define("__BEOS__", 1);
#if __POWERPC__
	Define("__POWERPC__", 1);
#elif __arm__
	Define("__arm__", 1);
#elif __INTEL__
	Define("__INTEL__", 1);
#else
#error Unknown architecture
#endif
#if __BIG_ENDIAN__
	Define("__BIG_ENDIAN__", 1);
#else
	Define("__LITTLE_ENDIAN__", 1);
#endif
	time_t now;
	time(&now);
	Define("__CDATE__", ctime(&now));
	
	return B_OK;
}

status_t BResourceParser::SetTo(const char* file, bool include_from_here)
{
	status_t err = fFile.SetTo(file);
	fParser->Context().SetSourceDir("");
	if( !err && include_from_here ) {
		BPath parent;
		err = fFile.GetParent(&parent);
		if( !err ) fParser->Context().SetSourceDir(parent.Path());
	}
	
	return err;
}

status_t BResourceParser::InitCheck() const
{
	if( !fParser ) return B_NO_INIT;
	if( fError.Code() ) return fError.Code();
	return fFile.InitCheck();
}

status_t BResourceParser::GetContext(BResourceContext* context) const
{
	if( !context ) return B_BAD_VALUE;
	
	if( !fParser ) *context = BResourceContext();
	else *context = fParser->Context();
	
	return B_OK;
}

status_t BResourceParser::SetContext(const BResourceContext* context)
{
	if( !context ) return B_BAD_VALUE;
	
	if( fParser ) {
		if( context->fContext ) fParser->SetContext(*context->fContext);
		else fParser->SetContext(ResourceParserContext());
	}
	
	return B_OK;
}

const char* BResourceParser::Path() const
{
	const char* p = fFile.Path();
	return p ? p : "<init>";
}

status_t BResourceParser::PreInclude(const char* dir)
{
	return fParser->Context().PreInclude(dir);
}

status_t BResourceParser::PostInclude(const char* dir)
{
	return fParser->Context().PostInclude(dir);
}

static bool free_func(void* item)
{
	delete reinterpret_cast<BResourceItem*>(item);
	return true;
}

void BResourceParser::FreeData()
{
	StopWriting();
	
	delete fParser;
	fParser = 0;
	
	fFile.Unset();
	
	fItems.DoForEach(free_func);
	fItems.MakeEmpty();
}

status_t BResourceParser::Define(const char* name)
{
	return Define(name, true);
}

status_t BResourceParser::Define(const char* name, const char* val)
{
	if( !fParser ) return B_NO_INIT;
	BRef<RSymbol> sym = fParser->NewStringSymbol(val);
	fParser->AddSymbol(name, sym);
	PRINT(("Defined symbol %s = %s\n", name, val));
	return B_OK;
}

status_t BResourceParser::Define(const char* name, int val)
{
	if( !fParser ) return B_NO_INIT;
	BRef<RSymbol> sym = fParser->NewIntegerSymbol(val);
	fParser->AddSymbol(name, sym);
	PRINT(("Defined symbol %s = %d\n", name, val));
	return B_OK;
}

status_t BResourceParser::Run()
{
	BAutolock l(gStateLock);
	
	fError.Init();
	fErrorCount = 0;
	
	if( fFile.InitCheck() ) return fFile.InitCheck();
	
	yyrestart(fopen(fFile.Path(), "r"));
	//yyin = fopen(fFile.Path(), "r");
	yylineno = 1;
	
	#if DEBUG
	//yydebug = 1;
	#endif
	
	if (!yyin) {
		fError.SetToV(B_ENTRY_NOT_FOUND, Path(), 0, "Cannot open file");
		fErrorCount++;
		return B_ENTRY_NOT_FOUND;
	}
	
	gResParserState = fParser;
	
	try {
		while (yyparse(fParser) == 0 && !fErrorCount)
			;
		
		fclose(yyin);
		yyin = 0;
	} catch(...) {
		fError.SetTo(B_ERROR, "Unexpected exception occurred");
	}
	
	gResParserState = NULL;
	
	return InitCheck();
}

status_t BResourceParser::ReadItem(BResourceItem* item)
{
	return AddItem(item);
}

void BResourceParser::Error(const ErrorInfo& info)
{
	AddError(info);
}

void BResourceParser::Warn(const ErrorInfo& info)
{
	fprintf(stderr, "### BeRes Warning\n# %s\n", info.Message());
	fprintf(stderr, "\n#------------\nFile \"%s\"; Line %ld\n#-------------\n",
					info.File(), info.Line());
}

status_t BResourceParser::StartWriting(BDataIO* resource, BDataIO* header,
									   bool buffered, bool owns_inputs)
{
	StopWriting();
	fFoundSymbols = false;
	
	if( buffered ) {
		fOwnWriteIO = true;
		BPositionIO* io = dynamic_cast<BPositionIO*>(resource);
		if( !io ) return B_BAD_VALUE;
		fWriteDest = new BBufferIO(io, BBufferIO::DEFAULT_BUF_SIZE, owns_inputs);
		if( header ) {
			io = dynamic_cast<BPositionIO*>(header);
			if( !io ) return B_BAD_VALUE;
			fWriteHeader = new BBufferIO(io, BBufferIO::DEFAULT_BUF_SIZE, owns_inputs);
		}
	
	} else {
		fOwnWriteIO = owns_inputs;
		fWriteDest = resource;
		fWriteHeader = header;
		
	}
	
	return B_OK;
}

status_t BResourceParser::WriteHeader(const char* fileName,
									  const char* headerName)
{
	if( !fWriteDest ) return B_NO_INIT;
	
	ssize_t err = WriteCommentBlock(fWriteDest, fileName);
	
	if( err >= B_OK && fWriteHeader ) {
		BString newName;
		int32 s = 0;
		if( !headerName ) {
			newName = fileName;
			int32 p = newName.FindLast('.');
			s = newName.FindLast('/');
			if( p != B_ERROR && p > s ) {
				newName.Truncate(p);
			}
			newName.Append(".h");
			if( s == B_ERROR ) s = 0;
			else s = s+1;
		} else {
			newName = headerName;
			int32 i = 0;
			while( i<newName.Length() && fileName[i]
				   && fileName[i] == newName[i] ) i++;
			i = newName.FindLast('/', i);
			if( i == B_ERROR ) s = 0;
			else s = i + 1;
		}
		const char* name = newName.String() + s;
		
		err = WriteCommentBlock(fWriteHeader, newName.String());
		
		if( err >= B_OK ) {
			BString outp;
			outp = "#include \"";
			outp += name;
			outp += "\"\n\n";
			err = fWriteDest->Write(outp.String(), outp.Length());
		}
	}
	
	if( err < B_OK ) {
		ErrorInfo info;
		info.SetTo(err, "Error writing header: %s", strerror(err));
		Error(info);
		return err;
	}
	return B_OK;
}

status_t BResourceParser::StartWritingHeader(const char* file)
{
	BFile* writeDest = new BFile();
	BFile* writeHeader = new BFile();
	
	status_t err;
	err = writeDest->SetTo(file, B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	if( err ) {
		ErrorInfo info;
		info.SetTo(err, "Unable to open write file %s: %s",
					file, strerror(err));
		Error(info);
		delete writeDest;
		delete writeHeader;
		return err;
	}
	BNodeInfo	ni;
	if(ni.SetTo(writeDest) == B_OK) ni.SetType("text/x-vnd.Be.ResourceDef");
	
	BString header_name = file;
	int32 p = header_name.FindLast('.');
	int32 s = header_name.FindLast('/');
	if( p != B_ERROR && p > s ) {
		header_name.Truncate(p);
	}
	header_name.Append(".h");
	err = writeHeader->SetTo(header_name.String(),
							 B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	if( err ) {
		ErrorInfo info;
		info.SetTo(err, "Unable to open write file %s: %s",
					header_name.String(), strerror(err));
		Error(info);
		delete writeDest;
		delete writeHeader;
		return err;
	}
	if(ni.SetTo(writeHeader) == B_OK) ni.SetType("text/x-source-code");
	
	err = StartWriting(writeDest, writeHeader);
	if( !err ) err = WriteHeader(file, header_name.String());
	return err;
}

status_t BResourceParser::WriteCommentBlock(BDataIO* to,
											const char* fileName) const
{
	if( !to ) return B_OK;
	
	BString outp;
	outp = "/*\n** ";
	outp += fileName ? fileName : "<unknown>";
	outp += "\n**\n** Automatically generated by BResourceParser on\n** ";
	
	time_t now;
	time(&now);
	struct tm *now_t = localtime(&now);
	char buffer[256];
	strftime(buffer, 255, "%A, %B %d, %Y at %H:%M:%S", now_t);
	outp += &buffer[0];
	
	outp += ".\n**\n*/\n\n";
	
	ssize_t err = to->Write(outp.String(), outp.Length());
	if( err < B_OK ) return (status_t)err;
	return B_OK;
}

const char* BResourceParser::EscapeForString(const char* str, BString* out)
{
	if( !str || !*str ) {
		(*out) = "";
		return out->String();
	}
	
	out->CharacterEscape(str, "\"\\\t\n", '\\');
	const int32 len = out->Length();
	char* p = out->LockBuffer(len);
	while( p && *p ) {
		switch( *p ) {
			case '\t':		*p = 't';		break;
			case '\n':		*p = 'n';		break;
			case 0:			*p = '@';		break;
		}
		p++;
	}
	out->UnlockBuffer(len);
	
	return out->String();
}

const char* BResourceParser::EscapeForString(const char* str)
{
	return EscapeForString(str, &fEscapeBuffer);
}

status_t BResourceParser::WriteItem(const BResourceItem* item)
{
	if( !fWriteDest ) return B_NO_INIT;
	
	BString outp;
	outp << "resource(";
	if( fWriteHeader && item->HasSymbol() ) {
		outp << item->Symbol();
	} else {
		outp << item->ID();
	}
	
	if( item->HasName() ) {
		outp << ", \"" << EscapeForString(item->Name()) << "\"";
	}
	outp << ") ";
	ssize_t err = fWriteDest->Write(outp.String(), outp.Length());
	
	if( err >= B_OK ) {
		err = WriteValue(fWriteDest, item->Type(),
						 item->Data(), item->Size(), 0);
	}
	
	if( err >= B_OK ) {
		outp = ";\n\n";
		err = fWriteDest->Write(outp.String(), outp.Length());
	}
	
	if( err >= B_OK && fWriteHeader && item->HasSymbol() ) {
		BString outp;
		if( !fFoundSymbols ) {
			outp << "enum {\n\t";
			fFoundSymbols = true;
		} else {
			outp << ",\n\t";
		}
		outp << item->Symbol() << " = " << item->ID();
		err = fWriteHeader->Write(outp.String(), outp.Length());
	}
	
	if( err < B_OK ) {
		ErrorInfo info;
		BString buf;
		info.SetTo(err, "Error writing resource %s: %s",
					TypeIDToString(item->Type(), item->ID(), &buf),
					strerror(err));
		Error(info);
		return err;
	}
	return B_OK;
}

status_t BResourceParser::StopWriting()
{
	ssize_t err = B_OK;
	
	if( fWriteHeader && fFoundSymbols ) {
		BString outp("\n};\n");
		err = fWriteHeader->Write(outp.String(), outp.Length());
	}
	
	if( fOwnWriteIO ) {
		delete fWriteDest;
		delete fWriteHeader;
	}
	
	fWriteDest = fWriteHeader = 0;
	fOwnWriteIO = false;
	
	if( err < B_OK ) {
		ErrorInfo info;
		BString buf;
		info.SetTo(err, "Error finishing write: %s",
					strerror(err));
		Error(info);
		return err;
	}
	return B_OK;
}

const char* BResourceParser::LevelIndent(int32 level) const
{
	static const char space[] =
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	int32 off = sizeof(space) - level - 1;
	if( off < 0 ) off = 0;
	return &space[off];
}

static void append_float(BString& into, double value)
{
	char buffer[64];
	sprintf(buffer, "%g", value);
	if( !strchr(buffer, '.') && !strchr(buffer, 'e') &&
		!strchr(buffer, 'E') ) {
		strncat(buffer, ".0", sizeof(buffer)-1);
	}
	into << buffer;
}

status_t BResourceParser::WriteValue(BDataIO* to, type_code type,
									 const void* data, size_t size,
									 int32 level) const
{
	BString outp;
	BString buf;
	
	ssize_t err = B_OK;
	
	if( (type == B_STRING_TYPE || type == B_MIME_TYPE ||
		 type == B_ASCII_TYPE || type == B_MIME_STRING_TYPE) &&
			size > 0 && ((const char*)data)[size-1] == 0 ) {
		WriteStringValue(to, type, (const char*)data, size, level+1);
	} else if( type == B_INT8_TYPE && size == sizeof(int8) ) {
		outp << "(int8)" << (*((const int8*)data));
	} else if( type == B_INT16_TYPE && size == sizeof(int16) ) {
		outp << "(int16)" << (*((const int16*)data));
	} else if( type == B_INT32_TYPE && size == sizeof(int32) ) {
		outp << (*((const int32*)data));
	} else if( type == B_INT64_TYPE && size == sizeof(int64) ) {
		outp << "(int64)" << (*((const int64*)data));
	} else if( type == B_UINT8_TYPE && size == sizeof(uint8) ) {
		outp << "(uint8)" << (uint32)(*((const uint8*)data));
	} else if( type == B_UINT16_TYPE && size == sizeof(uint16) ) {
		outp << "(uint16)" << (uint32)(*((const uint16*)data));
	} else if( type == B_UINT32_TYPE && size == sizeof(uint32) ) {
		outp << "(uint32)" << (*((const uint32*)data));
	} else if( type == B_UINT64_TYPE && size == sizeof(uint64) ) {
		outp << "(uint64)" << (*((const uint64*)data));
	} else if( type == B_OFF_T_TYPE && size == sizeof(off_t) ) {
		outp << "(off_t)" << (*((const off_t*)data));
	} else if( type == B_SIZE_T_TYPE && size == sizeof(size_t) ) {
		outp << "(size_t)" << (*((const size_t*)data));
	} else if( type == B_SSIZE_T_TYPE && size == sizeof(ssize_t) ) {
		outp << "(ssize_t)" << (*((const ssize_t*)data));
	} else if( type == B_TIME_TYPE && size == sizeof(time_t) ) {
		outp << "(time_t)" << (*((const time_t*)data));
	} else if( type == B_FLOAT_TYPE && size == sizeof(float) ) {
		append_float(outp, *(const float*)data);
	} else if( type == B_DOUBLE_TYPE && size == sizeof(double) ) {
		outp << "(double)";
		append_float(outp, *(const double*)data);
	} else if( type == B_BOOL_TYPE && size == sizeof(bool) ) {
		outp << ( (*((const bool*)data)) ? "true" : "false" );
	} else if( type == B_RECT_TYPE && size == sizeof(BRect) ) {
		const BRect& d = *(const BRect*)data;
		outp << "rect { ";
		append_float(outp, d.left);
		outp << ", ";
		append_float(outp, d.top);
		outp << ", ";
		append_float(outp, d.right);
		outp << ", ";
		append_float(outp, d.bottom);
		outp << " }";
	} else if( type == B_POINT_TYPE && size == sizeof(BPoint)) {
		const BPoint& d = *(const BPoint*)data;
		outp << "point { ";
		append_float(outp, d.x);
		outp << ", ";
		append_float(outp, d.y);
		outp << " }";
	} else if( type == B_RGB_COLOR_TYPE && size == sizeof(rgb_color)) {
		const rgb_color& d = *(const rgb_color*)data;
		outp << "rgb_color { 0x";
		char buffer[16];
		sprintf(buffer, "%02x", d.red);
		outp << buffer;
		outp << ", 0x";
		sprintf(buffer, "%02x", d.green);
		outp << buffer;
		outp << ", 0x";
		sprintf(buffer, "%02x", d.blue);
		outp << buffer;
		outp << ", 0x";
		sprintf(buffer, "%02x", d.alpha);
		outp << buffer;
		outp << " }";
	} else {
		BMessage msg;
		BMemoryIO mem(data, size);
		err = msg.Unflatten(&mem);
		bool isArray = false;
		#if 0
		if( err == B_OK && mem.Position() != size ) {
			fprintf(stderr, "Looks like message, but size=%ld and pos=%Ld\n",
					size, mem.Position());
		} else if( err != B_OK ) {
			fprintf(stderr, "Error unflattening message: %s\n", strerror(err));
		}
		#endif
		if( err == B_OK && mem.Position() == size ) {
			err = WriteMessageValue(to, type, &msg, level);
			if( err < B_OK ) return (status_t)err;
		} else if( LooksLikeString(data, size, &isArray) ) {
			err = WriteStringValue(to, type, (const char*)data, size, level, isArray);
			if( err < B_OK ) return (status_t)err;
		} else {
			err = WriteHexValue(to, type, data, size, level);
			if( err < B_OK ) return (status_t)err;
		}
	}
	
	if( outp.Length() > 0 ) {
		err = to->Write(outp.String(), outp.Length());
		if( err < B_OK ) return (status_t)err;
	}
	
	return (status_t)err;
}

bool BResourceParser::LooksLikeString(const void* data, size_t size, bool* out_isArray)
{
	if( size <= 0 ) return false;
	
	// Does this look like a string?
	const uint8* c = (const uint8*)data;
	const uint8* e = c+size;
	bool isString = c[size-1] == 0 ? true : false;
	bool isArray = false;
	int32 quality = 1;
	while( c < e && isString ) {
		if( *c == 0 ) {
			if( c < (e-1) ) {
				// embedded '\0'.
				isArray = true;
				quality--;
			}
		} else if( *c >= ' ' && *c < 127 ) {
			quality++;
		} else if( ((*c)&0xE0) == 0xC0 ) {
			c++;
			if( ((*c)&0xC0) == 0x80 ) {
				c++;
				quality += 2;
			}
		} else if( ((*c)&0xF0) == 0xE0 ) {
			c++;
			if( ((*c)&0xC0) == 0x80 ) {
				c++;
				if( ((*c)&0xC0) == 0x80 ) {
					c++;
					quality += 3;
				}
			}
		} else if( *c != '\n' && *c != '\t' ) {
			isString = false;
		}
		c++;
	}
	
	if( out_isArray ) *out_isArray = isArray;
	
	if( quality < 0 ) return false;
	return isString;
}

const char* BResourceParser::TypeToString(type_code type, BString* out, uint32 flags)
{
	return BResourceItem::TypeToString(type, out, flags);
}

const char* BResourceParser::TypeIDToString(type_code type, int32 id, BString* out)
{
	return BResourceItem::TypeIDToString(type, id, out);
}

type_code BResourceParser::StringToType(const char* str, uint32 flags)
{
	return BResourceItem::StringToType(str, flags);
}

const char* BResourceParser::TypeToString(type_code type, uint32 flags)
{
	return TypeToString(type, &fTypeBuffer, flags);
}

const char* BResourceParser::TypeIDToString(type_code type, int32 id)
{
	return TypeIDToString(type, id, &fTypeBuffer);
}

status_t BResourceParser::WriteMessageValue(BDataIO* to, type_code type,
											const BMessage* msg, int32 level) const
{
	BString outp;
	BString buf;
	ssize_t err;
	
	BString classStr, addonStr;
	if( msg->FindString("class", &classStr) == B_OK ) {
		msg->FindString("add_on", &addonStr);
	}
	
	if( type != B_MESSAGE_TYPE ) {
		outp << "#" << TypeToString(type, &buf) << " ";
	}
	if( classStr.Length() > 0 ) {
		outp << "archive";
		if( addonStr.Length() > 0 || msg->what != B_ARCHIVED_OBJECT ) {
			outp << "(";
			if (addonStr.Length() > 0) outp << "\"" << addonStr << "\"";
			if( msg->what != B_ARCHIVED_OBJECT )
				outp << ", " << TypeToString(msg->what, &buf);
			outp << ")";
		}
		outp << " " << classStr;
	} else {
		outp << "message";
		if( msg->what != 0 ) outp << "(" << TypeToString(msg->what, &buf) << ")";
	}
	err = to->Write(outp.String(), outp.Length());
	if( err < B_OK ) return (status_t)err;

#if B_BEOS_VERSION_DANO
	const char* fieldName;
#else
	char* fieldName;
#endif
	type_code fieldType;
	int32 fieldCount;
	int32 myLevel = level;
	int32 i;
	for( i=0;
		 msg->GetInfo(B_ANY_TYPE,i,&fieldName,&fieldType,&fieldCount) == B_OK;
		 i++ ) {
		for( int32 j=0; j<fieldCount; j++ ) {
			if( j == 0 && classStr.Length() > 0 &&
					( strcmp(fieldName, "class") == 0 ||
					  strcmp(fieldName, "add_on") == 0 ) ) {
				continue;
			}
			
			const void* fieldData=NULL;
			ssize_t fieldSize=0;
			err = msg->FindData(fieldName, fieldType, j,
								&fieldData, &fieldSize);
			if( err != B_OK ) return err;
			
			if( myLevel == level ) {
				// start writing array.
				myLevel = level+1;
				outp = " {\n";
			} else {
				// move to next field.
				outp = ",\n";
			}
			outp << LevelIndent(myLevel)
				 << "\"" << EscapeForString(fieldName, &buf)
				 << "\" = ";
			err = to->Write(outp.String(), outp.Length());
			if( err < B_OK ) return (status_t)err;
			err = WriteValue(to, fieldType, fieldData, fieldSize, myLevel);
			if( err < B_OK ) return (status_t)err;
		}
	}
	
	if( i == 0 ) {
		to->Write(" { }", 4);
	}
	
	if( myLevel != level ) {
		outp = "\n";
		outp << LevelIndent(level) << "}";
		err = to->Write(outp.String(), outp.Length());
		if( err < B_OK ) return (status_t)err;
	}
	
	return B_OK;
}

status_t BResourceParser::WriteStringValue(BDataIO* to, type_code type,
											const char* data, size_t size,
											int32 level, bool force_array) const
{
	BString outp;
	BString buf;
	
	ssize_t err = B_OK;
	
	if( !force_array ) if( size > 64 ) force_array = true;
	
	if( !force_array ) {
		if( type != B_STRING_TYPE ) {
			outp << "(#" << TypeToString(type, &buf) << ") ";
			err = to->Write(outp.String(), outp.Length());
		}
		if( err < B_OK ) return (status_t)err;
		err = WriteStringData(to, data, size, level+1, force_array);
		if( err < B_OK ) return (status_t)err;
	} else {
		// TO DO: Add "array string { }" syntax.
		if( type != B_RAW_TYPE ) {
			outp << "#" << TypeToString(type, &buf);
		}
		outp << " array {\n" << LevelIndent(level+1);
		err = to->Write(outp.String(), outp.Length());
		if( err < B_OK ) return (status_t)err;
		err = WriteStringData(to, data, size, level+1, force_array);
		if( err < B_OK ) return (status_t)err;
		outp = "\n";
		outp << LevelIndent(level) << "}";
		err = to->Write(outp.String(), outp.Length());
		if( err < B_OK ) return (status_t)err;
	}
	
	return (status_t)err;
}

status_t BResourceParser::WriteStringData(BDataIO* to,
										  const char* data, size_t size,
										  int32 level, bool as_array) const
{
	BString prefix("\n");
	prefix << LevelIndent(level) << "\"";
	
	const char* pos = (const char*)data;
	const char* end = pos + size;
	
	ssize_t err = B_OK;
	char* buffer = (char*)malloc(prefix.Length() + 64 + 6);
	BString result;
	bool first = true;
	while( pos < end && err >= B_OK ) {
		// Extract out next string.
		EscapeForString(pos, &result);
		while( *pos ) pos++;
		pos++;
		
		const char* p = result.String();
		do {
			char* b = buffer;
			if( first ) {
				*b++ = '"';
				first = false;
			} else {
				memcpy(b, prefix.String(), prefix.Length());
				b += prefix.Length();
			}
			
			int32 i = 0;
			while( *p && i < 64 ) {
				*b++ = *p;
				if( *p == '\\' ) {
					p++;
					i++;
					*b++ = *p;
					if( *p == 'n' && *(p+1) != 0 ) {
						if( *(p+1) != '\\' || *(p+2) != 'n' ) {
							p++;
							break;
						}
					}
				}
				p++;
				i++;
			}
			bool embedded_null = ( !*p && pos < end );
			if( embedded_null && !as_array ) {
				*b++ = '\\';
				*b++ = '@';
			}
			*b++ = '"';
			if( embedded_null && as_array ) *b++ = ',';
			err = to->Write(buffer, (size_t)(b-&buffer[0]));
		} while( *p );
	}
	free(buffer);
	
	return err;
}

status_t BResourceParser::WriteHexValue(BDataIO* to, type_code type,
									   const void* data, size_t size,
									   int32 level, int32 hint_line_length) const
{
	BString outp;
	BString buf;
	
	ssize_t err = B_OK;
	
	if( hint_line_length == 0 ) {
		switch( type ) {
			case 'ICON':		hint_line_length = 32;		break;
			case 'MICN':		hint_line_length = 16;		break;
			default:			hint_line_length = 32;		break;
		}
	}
	
	if( (int32)size <= hint_line_length ) {
		if( type != B_RAW_TYPE ) {
			outp << "(#" << TypeToString(type, &buf) << ") ";
			err = to->Write(outp.String(), outp.Length());
		}
		if( err < B_OK ) return (status_t)err;
		err = WriteHexData(to, data, size, level+1, hint_line_length);
		if( err < B_OK ) return (status_t)err;
	} else {
		if( type != B_RAW_TYPE ) {
			outp << "#" << TypeToString(type, &buf);
		}
		outp << " array {\n" << LevelIndent(level+1);
		err = to->Write(outp.String(), outp.Length());
		if( err < B_OK ) return (status_t)err;
		err = WriteHexData(to, data, size, level+1, hint_line_length);
		if( err < B_OK ) return (status_t)err;
		outp = "\n";
		outp << LevelIndent(level) << "}";
		err = to->Write(outp.String(), outp.Length());
		if( err < B_OK ) return (status_t)err;
	}
	
	return (status_t)err;
}

status_t BResourceParser::WriteHexData(BDataIO* to,
									   const void* data, size_t size,
									   int32 level, int32 line_length) const
{
	if( size == 0 ) {
		// special case -- empty hex data.
		return to->Write("$\"\"", 3);
	}
	
	BString prefix("\n");
	prefix << LevelIndent(level) << "$\"";
	const uint8* pos = (const uint8*)data;
	const uint8* end = pos + size;
	
	ssize_t err = B_OK;
	char* buffer = (char*)malloc(prefix.Length() + (line_length*2) + 1);
	while( pos < end && err >= B_OK ) {
		char* b = buffer;
		if( pos == data ) {
			*b++ = '$';
			*b++ = '"';
		} else {
			memcpy(b, prefix.String(), prefix.Length());
			b += prefix.Length();
		}
		int32 i = 0;
		while( pos < end && i < line_length ) {
			*b++ = makehexdigit( (*pos) >> 4 );
			*b++ = makehexdigit( (*pos) );
			pos++;
			i++;
		}
		*b++ = '"';
		err = to->Write(buffer, (size_t)(b-&buffer[0]));
	}
	free(buffer);
	
	return err;
}

void BResourceParser::AddError(const ErrorInfo& info)
{
	if( info.Code() != B_OK ) {
		if( fErrorCount == 0 ) fError = info;
		fErrorCount++;
	}
}
	
size_t BResourceParser::CountErrors() const
{
	return fErrorCount;
}

const ErrorInfo& BResourceParser::ErrorAt(size_t idx) const
{
	return idx == 0 ? fError : fNoError;
}

status_t BResourceParser::AddItem(BResourceItem* item)
{
	fItems.AddItem(item);
	return B_OK;
}

const BResourceItem* BResourceParser::ItemAt(int32 index) const
{
	return reinterpret_cast<BResourceItem*>(fItems.ItemAt(index));
}

BPath BResourceParser::FindResourceFile(const char* name) const
{
	return fParser->FindResourceFile(name);
}

BResourceItem* BResourceParser::NewItem(type_code type, int32 id,
										const char* name, const char* sym) const
{
	return new BResourceItem(type, id, name, sym);
}
