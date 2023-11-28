#include <ResParserState.h>

#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <time.h>
#include <sys/stat.h>
#include <File.h>
#include <Resources.h>
#include <Autolock.h>
#include <Debug.h>
#include <ByteOrder.h>

extern int yyparse();
extern FILE *yyin;
extern int yylineno;
extern int yydebug;

// Shamelessly stolen from tracker/Utilites.cpp
#if DEBUG
static void
HexDump(const void *buf, long length)
{
	const int32 kBytesPerLine = 16;
	long offset;
	unsigned char *buffer = (unsigned char *)buf;

	for (offset = 0; ; offset += kBytesPerLine, buffer += kBytesPerLine) {
		long remain = length;
		int index;

		printf( "\t0x%06x: ", (int)offset);

		for (index = 0; index < kBytesPerLine; index++) {

			if (remain-- > 0)
				printf("%02x%c", buffer[index], remain > 0 ? ',' : ' ');
			else
				printf("   ");
		}

		remain = length;
		printf(" \'");
		for (index = 0; index < kBytesPerLine; index++) {

			if (remain-- > 0)
				printf("%c", buffer[index] > ' ' ? buffer[index] : '.');
			else
				printf(" ");
		}
		printf("\'\n");

		length -= kBytesPerLine;
		if (length <= 0)
			break;

	}
	fflush(stdout);
}
#endif

// ---------------------- ResourceParserContext ----------------------

ResourceParserContext::ResourceParserContext(ResourceParserState* state)
	: fState(state)
{
	fIncludePaths.resize(0);
	PostInclude(".");
	
	int32 ident;
	ident = DefineBasicType("int32", B_INTEGER_SYMBOL, B_INT32_TYPE);
	fInt32Type = dynamic_cast<RIntegerSymbol*>(IdentifierSymbol(ident));
	ident = DefineBasicType("bool", B_INTEGER_SYMBOL, B_BOOL_TYPE);
	fBoolType = dynamic_cast<RIntegerSymbol*>(IdentifierSymbol(ident));
	
	BRef<RSymbol> s_uint8 = 0;
	
	DefineBasicType("int8", B_INTEGER_SYMBOL, B_INT8_TYPE);
	DefineBasicType("int16", B_INTEGER_SYMBOL, B_INT16_TYPE);
	DefineBasicType("int64", B_INTEGER_SYMBOL, B_INT64_TYPE);
	ident = DefineBasicType("uint8", B_INTEGER_SYMBOL, B_UINT8_TYPE);
	s_uint8 = IdentifierSymbol(ident);
	DefineBasicType("uint16", B_INTEGER_SYMBOL, B_UINT16_TYPE);
	DefineBasicType("uint32", B_INTEGER_SYMBOL, B_UINT32_TYPE);
	DefineBasicType("uint64", B_INTEGER_SYMBOL, B_UINT64_TYPE);
	DefineBasicType("ssize_t", B_INTEGER_SYMBOL, B_SSIZE_T_TYPE);
	DefineBasicType("size_t", B_INTEGER_SYMBOL, B_SIZE_T_TYPE);
	DefineBasicType("off_t", B_INTEGER_SYMBOL, B_OFF_T_TYPE);
	DefineBasicType("time_t", B_INTEGER_SYMBOL, B_TIME_TYPE);
	
	ident = DefineBasicType("float", B_REAL_SYMBOL, B_FLOAT_TYPE);
	fFloatType = dynamic_cast<RRealSymbol*>(IdentifierSymbol(ident));
	ident = DefineBasicType("double", B_REAL_SYMBOL, B_DOUBLE_TYPE);
	fDoubleType = dynamic_cast<RRealSymbol*>(IdentifierSymbol(ident));
	
	ident = DefineBasicType("string", B_STRING_SYMBOL, B_STRING_TYPE);
	fStringType = dynamic_cast<RStringSymbol*>(IdentifierSymbol(ident));
	ident = DefineBasicType("buffer", B_BUFFER_SYMBOL, B_RAW_TYPE);
	fBufferType = dynamic_cast<RBufferSymbol*>(IdentifierSymbol(ident));
	
	ident = DefineBasicType("array", B_COMPOUND_SYMBOL, B_RAW_TYPE);
	fCompoundType = dynamic_cast<RCompoundSymbol*>(IdentifierSymbol(ident));
	ident = DefineBasicType("message", B_COMPOUND_SYMBOL, B_MESSAGE_TYPE);
	fMessageType = dynamic_cast<RMessageSymbol*>(IdentifierSymbol(ident));
	ident = DefineBasicType("archive", B_COMPOUND_SYMBOL, B_MESSAGE_TYPE);
	fArchiveType = dynamic_cast<RArchiveSymbol*>(IdentifierSymbol(ident));
	
	ASSERT( fInt32Type != 0 && fBoolType != 0 && fStringType != 0 &&
			fBufferType != 0 && fCompoundType != 0 && fMessageType != 0 &&
			fArchiveType != 0 );

	DefineCompoundType("rgb_color", B_RGB_COLOR_TYPE,
					   (RSymbol*)s_uint8, "red",
					   (RSymbol*)s_uint8, "green",
					   (RSymbol*)s_uint8, "blue",
					   (RSymbol*)s_uint8, "alpha",
					   (RSymbol*)0, 0);
	
	DefineCompoundType("BPoint", B_POINT_TYPE,
					   FloatType(), "x",
                       FloatType(), "y",
                       (RSymbol*)0, 0);
	DefineTypeAlias("BPoint", "point");
	
	DefineCompoundType("BRect", B_RECT_TYPE,
					   FloatType(), "left",
                       FloatType(), "top",
					   FloatType(), "right",
                       FloatType(), "bottom",
                       (RSymbol*)0, 0);
	DefineTypeAlias("BRect", "rect");
}

ResourceParserContext::ResourceParserContext(const ResourceParserContext& o)
	: fState(0)
{
	*this = o;
}

ResourceParserContext::~ResourceParserContext()
{
	// NOTE:
	// The include and macro stacks will leak their passed in data if
	// this class is destroyed before they are empty!
#if DEBUG
	PRINT(("Cleaning up symbols...\n"));
	for( size_t i=0; i<fSymbols.size(); i++ ) {
		if( fSymbols[i] != 0 ) {
			PRINT(("Symbol #%ld (%s) at %p being cleaned...\n",
					i, fIdentifiers[i].String(), (RSymbol*)fSymbols[i]));
			fSymbols[i] = 0;
		} else {
			PRINT(("Symbol #%ld (%s) is empty.\n",
					i, fIdentifiers[i].String()));
		}
	}
#endif
}

ResourceParserContext& ResourceParserContext::operator=(const ResourceParserContext& o)
{
	fIncludePaths = o.fIncludePaths;
	fSourceIncludePath = o.fSourceIncludePath;
	fSymbolTable = o.fSymbolTable;
	fIdentifiers = o.fIdentifiers;
	fSymbols = o.fSymbols;
	fInt32Type = o.fInt32Type;
	fBoolType = o.fBoolType;
	fFloatType = o.fFloatType;
	fDoubleType = o.fDoubleType;
	fStringType = o.fStringType;
	fBufferType = o.fBufferType;
	fCompoundType = o.fCompoundType;
	fMessageType = o.fMessageType;
	fArchiveType = o.fArchiveType;
	
	// The only thing that isn't copied is the parser state we are
	// associated with.  Make sure all of the symbols we were given are
	// now associated with this state.
	ResourceParserState* myState = fState;
	fState = 0;
	SetState(myState);

#if DEBUG
	PRINT(("Assigned ResourceParserContext, new symbols are:\n"));
	for( size_t i=0; i<fSymbols.size(); i++ ) {
		PRINT(("Symbol #%ld (%s) at %p.\n",
				i, fIdentifiers[i].String(), (RSymbol*)fSymbols[i]));
	}
#endif

	return *this;
};

void ResourceParserContext::SetState(ResourceParserState* state)
{
	if( fState != state ) {
		for( size_t i=0; i<fSymbols.size(); i++ ) {
			if( fSymbols[i] != 0 ) fSymbols[i]->SetParser(state);
		}
		if( fInt32Type != 0 ) fInt32Type->SetParser(state);
		if( fBoolType != 0 ) fBoolType->SetParser(state);
		if( fFloatType != 0 ) fFloatType->SetParser(state);
		if( fDoubleType != 0 ) fDoubleType->SetParser(state);
		if( fStringType != 0 ) fStringType->SetParser(state);
		if( fBufferType != 0 ) fBufferType->SetParser(state);
		if( fCompoundType != 0 ) fCompoundType->SetParser(state);
		if( fMessageType != 0 ) fMessageType->SetParser(state);
		if( fArchiveType != 0 ) fArchiveType->SetParser(state);
	}
}

ResourceParserState* ResourceParserContext::State() const
{
	return fState;
}
	
status_t ResourceParserContext::PreInclude(const char* dir)
{
	for( size_t i=fIncludePaths.size(); i>0; i-- ) {
		fIncludePaths[i] = fIncludePaths[i-1];
	}
	fIncludePaths[0] = dir;
	
	return B_OK;
}

status_t ResourceParserContext::PostInclude(const char* dir)
{
	fIncludePaths.push_back(dir);
	
	return B_OK;
}

void ResourceParserContext::SetSourceDir(const char* dir)
{
	fSourceIncludePath = dir;
}

BPath ResourceParserContext::FindResourceFile(const char* name) const
{
	PRINT(("Looking for include file: %s\n", name));
	
	BPath path;
	
	for( size_t i = 0; path.InitCheck() != B_OK && i <= fIncludePaths.size(); i++ ) {
		const BString& dir = (i > 0)
						   ? (fIncludePaths[i-1])
						   : fSourceIncludePath;
		if( dir.Length() > 0 ) {
			path.Unset();
			if( dir != "." ) {
				PRINT(("Checking path: %s\n", dir.String()));
				path.SetTo(dir.String(), name);
			} else {
				PRINT(("Checking current directory.\n"));
				path.SetTo(name);
			}
			
			if( path.InitCheck() == B_OK ) {
				PRINT(("Made path: %s\n", path.Path()));
				struct stat s;
				if( stat(path.Path(), &s) != B_OK ) {
					PRINT(("Not found!\n"));
					path.Unset();
				}
			}
		}
	}
	
	return path;
}

int32 ResourceParserContext::DefineBasicType(const char* name,
											 symbol_code symbol, type_code type)
{
	BRef<RSymbol> sym = 0;
	
	switch(symbol) {
		case B_INTEGER_SYMBOL:
			sym = new RIntegerSymbol(0, type);
			break;
		case B_REAL_SYMBOL:
			sym = new RRealSymbol(0, type);
			break;
		case B_STRING_SYMBOL:
			sym = new RStringSymbol(0, type);
			break;
		case B_BUFFER_SYMBOL:
			sym = new RBufferSymbol(0, type);
			break;
		case B_COMPOUND_SYMBOL:
			if( strcmp(name, "message") == 0 ) {		// GAG!!!
				sym = new RMessageSymbol(0, type);
			} else if( strcmp(name, "archive") == 0 ) {
				sym = new RArchiveSymbol(0, type);
			} else {
				sym = new RCompoundSymbol(0, type);
			}
			break;
		default:
			debugger("Not a basic type.");
	}
	
	if( !sym ) return 0;
	return AddSymbol(name, sym);
}

int32 ResourceParserContext::DefineCompoundType(const char* name, type_code type, ...)
{
	BRef<RCompoundSymbol> sym = new RCompoundSymbol(0, type);
	
	va_list vl;
	va_start(vl, type);
	
	RSymbol* curf;
	while( (curf = va_arg(vl, RSymbol*)) != 0 ) {
		const char* curn = va_arg(vl, const char*);
		int32 iden = curn ? AddIdentifier(curn) : 0;
		sym->AddField(new RFieldSymbol(iden, NULL, curf));
	}
	
	va_end(vl);
	
	return AddSymbol(name, sym);
}

int32 ResourceParserContext::DefineTypeAlias(const char* origType, const char* newType)
{
	int32 origIdent = AddIdentifier(origType);
	if( origIdent <= 0 ) return 0;
	BRef<RSymbol> origSym = IdentifierSymbol(origIdent);
	if( origSym == 0 ) return 0;
	
	BRef<RSymbol> newSym = origSym->Clone();
	newSym->SetType(origSym);
	return AddSymbol(newType, newSym);
}

int32 ResourceParserContext::AddIdentifier(const char* name)
{
	return AddSymbol(name, NULL);
}

const char* ResourceParserContext::IdentifierName(int32 id) const
{
	if( id < 1 || (size_t)id > fIdentifiers.size() ) return "<anonymous>";
	return fIdentifiers[id-1].String();
}

int32 ResourceParserContext::AddSymbol(const char* name, RSymbol* symbol)
{
	std::map<BString, int32>::const_iterator i = fSymbolTable.find(name);
	if( i != fSymbolTable.end() ) {
		const int32 id = i->second;
		if( symbol ) {
			ASSERT(id >= 1 && (size_t)id <= fSymbols.size());
			if( id >= 1 && (size_t)id <= fSymbols.size() ) {
				BRef<RSymbol>* slot = &(fSymbols[id-1]);
				if( *slot ) {
					if( State() ) State()->Warn("Symbol %s redefined", name);
					*slot = 0;
				}
				*slot = symbol;
				symbol->SetIdentifier(id);
				symbol->SetParser(State());
			}
		}
		return id;
	}
	
	int32 id = fIdentifiers.size()+1;
	fSymbolTable[name] = id;
	fIdentifiers.push_back(name);
	fSymbols.push_back(symbol);
	if( symbol ) {
		symbol->SetIdentifier(id);
		symbol->SetParser(State());
	}
	
	ASSERT(fIdentifiers.size() == fSymbols.size());
	
	return id;
}

void ResourceParserContext::SetSymbol(int32 id, RSymbol* symbol)
{
	if( id < 1 || (size_t)id > fSymbols.size() ) {
		debugger("SetSymbol() -- identifier out of range.");
		return;
	}
	
	if( fSymbols[id-1] && fSymbols[id-1]->Symbol() != B_EMPTY_SYMBOL ) {
		if( State() ) State()->Warn("Symbol %s redefined", IdentifierName(id));
	}
	
	fSymbols[id-1] = symbol;
	if( symbol ) {
		symbol->SetIdentifier(id);
		symbol->SetParser(State());
	}
}

RSymbol* ResourceParserContext::IdentifierSymbol(int32 id) const
{
	if( id < 1 || (size_t)id > fSymbols.size() ) return NULL;
	return fSymbols[id-1];
}

RIntegerSymbol* ResourceParserContext::Int32Type() const
{
	return fInt32Type;
}

RIntegerSymbol* ResourceParserContext::BoolType() const
{
	return fBoolType;
}

RRealSymbol* ResourceParserContext::FloatType() const
{
	return fFloatType;
}

RRealSymbol* ResourceParserContext::DoubleType() const
{
	return fDoubleType;
}

RStringSymbol* ResourceParserContext::StringType() const
{
	return fStringType;
}

RBufferSymbol* ResourceParserContext::BufferType() const
{
	return fBufferType;
}

RCompoundSymbol* ResourceParserContext::CompoundType() const
{
	return fCompoundType;
}

RMessageSymbol* ResourceParserContext::MessageType() const
{
	return fMessageType;
}

RArchiveSymbol* ResourceParserContext::ArchiveType() const
{
	return fArchiveType;
}

// ---------------------- ResourceParserState ----------------------

ResourceParserState::ResourceParserState(BResourceParser* parser)
	: fParser(parser), fContext(this), include_stack_ptr(0), macro_stack_ptr(0)
{
	fResourceLine = -1;
}

ResourceParserState::~ResourceParserState()
{
}

void ResourceParserState::SetContext(const ResourceParserContext& o)
{
	fContext = o;
	fContext.SetState(this);
}

const char* ResourceParserState::CurrentFile() const
{
	if( include_stack_ptr > 0 )
		return fFileName[include_stack_ptr-1].String();
	if( fParser ) return fParser->Path();
	return "";
}

int32 ResourceParserState::CurrentLine() const
{
	return yylineno;
}

void ResourceParserState::Error(status_t code, const char *e, ...) const
{
	va_list vl;
	va_start(vl, e);
	
	ErrorInfo error;
	error.SetToV(code, CurrentFile(), CurrentLine(), e, vl);
	if( fParser ) fParser->Error(error);
	
	va_end(vl);
} /* error */

void ResourceParserState::Error(const char *e, ...) const
{
	va_list vl;
	va_start(vl, e);
	
	ErrorInfo error;
	error.SetToV(B_ERROR, CurrentFile(), CurrentLine(), e, vl);
	if( fParser ) fParser->Error(error);
	
	va_end(vl);
} /* error */

void ResourceParserState::Warn(const char *e, ...) const
{
	va_list vl;
	va_start(vl, e);
	
	ErrorInfo error;
	error.SetToV(0, CurrentFile(), CurrentLine(), e, vl);
	if( fParser ) fParser->Warn(error);
	
	va_end(vl);
} /* warn */

BPath ResourceParserState::FindResourceFile(const char* name) const
{
	return Context().FindResourceFile(name);
}

void ResourceParserState::WriteResource(RResourceData* res)
{
	PRINT(("Writing resource: %s, label=%s\n",
			TypeIDToString(res->TypeCode(), res->ID()->Value()), res->Label()));
	PRINT_SYMBOL(this, res->Data());
	
	const char* sym = res->ID()->Identifier() > 0
					? IdentifierName(res->ID()->Identifier())
					: NULL;
	BResourceItem* item = fParser->NewItem(res->TypeCode(),
											res->ID()->Value(),
											res->Label(),
											sym);
	ssize_t err = item->SetSize(res->Data()->FlattenedSize());
	if( err >= B_OK ) {
		item->Seek(0, SEEK_SET);
		err = res->Data()->FlattenStream(item);
		PRINT(("Data is %ld bytes, error=%lx:\n", item->Size(), err));
#if DEBUG
		HexDump(item->Data(), item->Size());
#endif
	}
	
	item->SetFile(fResourceFile.String());
	item->SetLine(fResourceLine);
	
	delete res;
	
	if( err < B_OK ) {
		Error(err, "Unable to create data for resource %s (%s)",
				TypeIDToString(item->Type(), item->ID()), item->Name());
		delete item;
		return;
	}
	
	fParser->ReadItem(item);
	
	fResourceFile = "";
	fResourceLine = -1;
} /* WriteResource */

void ResourceParserState::WriteResource(const char *file, int type, int id, const char *name)
{
	void *p;
	size_t s;
	
	BPath path = FindResourceFile(name);
	FILE* f = NULL;
	if( path.InitCheck() == B_OK ) f = fopen(path.Path(), "rb");
	
	if (!f) Error("Error opening file %s: %s", file, strerror(errno));
	
	fseek(f, 0, SEEK_END);
	s = ftell(f);
	fseek(f, 0, SEEK_SET);
	p = malloc(s);
	if (!p) {
		Error(B_NO_MEMORY, "Insufficient memory");
		return;
	}
	
	fread(p, s, 1, f);
	fclose(f);
	
	BResourceItem* item = fParser->NewItem(type, id, name);
	item->SetFile(file);
	status_t err = item->SetData(p, s);
	free(p);
	
	if( err != B_OK ) {
		Error(err, "Unable to create data for resource %s (%s)",
				TypeIDToString(item->Type(), item->ID()), item->Name());
		delete item;
		return;
	}
	
	fParser->ReadItem(item);
	
	fResourceFile = "";
	fResourceLine = -1;
} /* WriteResource */

void ResourceParserState::Import(const char *n)
{
	BPath path = FindResourceFile(n);
	BFile file;
	status_t err = path.InitCheck();
	if( !err ) err = file.SetTo(path.Path(), B_READ_ONLY);
	
	if (err != B_OK) {
		Error("Error opening file %s: %s", n, strerror(err));
		return;
	}
	
	BResources res;
	if( (err=res.SetTo(&file)) != B_OK ) {
		Error("Error reading resources %s: %s", path.Path(), strerror(err));
		return;
	}
	
	if( (err=res.PreloadResourceType()) != B_OK ) {
		Error("Error loading resources %s: %s", path.Path(), strerror(err));
		return;
	}
	
	int32 index = 0;
	type_code type;
	int32 id;
	const char* name;
	while( res.GetResourceInfo(index, &type, &id, &name, NULL) ) {
		size_t size;
		const void* data = res.LoadResource(type, id, &size);
		if( data ) {
			BResourceItem* item = fParser->NewItem(type, id, name);
			item->SetFile(path.Path());
			status_t err = item->SetData(data, size);
			
			if( err != B_OK ) {
				Error(err, "Unable to create data for resource %s (%s)",
						TypeIDToString(item->Type(), item->ID()), item->Name());
				delete item;
				return;
			}
			
			fParser->ReadItem(item);
		}
		
		index++;
	}
	
	fResourceFile = "";
	fResourceLine = -1;
} // Include

RSymbol* ResourceParserState::NewSymbol(RSymbol* type, type_code code) const
{
	RSymbol* sym = new RSymbol(type, code);
	sym->SetParser(const_cast<ResourceParserState*>(this));
	return sym;
}

RIntegerSymbol* ResourceParserState::NewIntegerSymbol(int64 value, RSymbol* type) const
{
	if( !type ) type = Int32Type();
	RIntegerSymbol* sym = new RIntegerSymbol(0, value, type);
	sym->SetParser(const_cast<ResourceParserState*>(this));
	return sym;
}

RRealSymbol* ResourceParserState::NewRealSymbol(double value, RSymbol* type) const
{
	if( !type ) type = FloatType();
	RRealSymbol* sym = new RRealSymbol(0, value, type);
	sym->SetParser(const_cast<ResourceParserState*>(this));
	return sym;
}

RStringSymbol* ResourceParserState::NewStringSymbol(const char* value, RSymbol* type) const
{
	if( !type ) type = StringType();
	RStringSymbol* sym = new RStringSymbol(0, value, type);
	sym->SetParser(const_cast<ResourceParserState*>(this));
	return sym;
}

RBufferSymbol* ResourceParserState::NewBufferSymbol(const void* value, size_t length,
											  RSymbol* type) const
{
	if( !type ) type = BufferType();
	RBufferSymbol* sym = new RBufferSymbol(0, value, length, type);
	sym->SetParser(const_cast<ResourceParserState*>(this));
	return sym;
}

RBufferSymbol* ResourceParserState::NewBufferSymbol(const char* f,
													RSymbol* type) const
{
	BPath path = FindResourceFile(f);
	ssize_t err = path.InitCheck();
	if( err != B_OK ) {
		Error("Error finding file %s: %s", f, strerror(err));
		return 0;
	}
	
	BFile file;
	err = file.SetTo(path.Path(), B_READ_ONLY);
	if (err != B_OK) {
		Error("Error opening file %s: %s", path.Path(), strerror(err));
		return 0;
	}
	
	size_t s = (size_t)file.Seek(0, SEEK_END);
	void* m = malloc(s);
	err = file.ReadAt(0, m, s);
	if( err < B_OK ) {
		free(m);
		Error("Error reading file %s: %s", path.Path(), strerror(err));
		return 0;
	}
	
	RBufferSymbol* b = NewBufferSymbol(m, s, type);
	free(m);
	return b;
}

RBufferSymbol* ResourceParserState::NewBufferSymbol(const char* resfile,
													type_code type, int id, const char* name,
													RSymbol* typeSym) const
{
	BPath path = FindResourceFile(resfile);
	ssize_t err = path.InitCheck();
	if( err != B_OK ) {
		Error("Error finding file %s: %s", resfile, strerror(err));
		return 0;
	}
	
	BFile file;
	err = file.SetTo(path.Path(), B_READ_ONLY);
	if (err != B_OK) {
		Error("Error opening file %s: %s", path.Path(), strerror(err));
		return 0;
	}
	
	BResources res;
	if( (err=res.SetTo(&file)) != B_OK ) {
		Error("Error reading resources %s: %s", path.Path(), strerror(err));
		return 0;
	}
	
	const void* data = 0;
	size_t size = 0;
	if( name && *name ) data = res.LoadResource(type, name, &size);
	if( !data ) data = res.LoadResource(type, id, &size);
	if( !data ) {
		Error("In file %s:\nUnable to find resource %s %s%s",
			  path.Path(), TypeIDToString(type, id),
			  name ? ", name " : "", name ? name : "");
		return 0;
	}
	
	return NewBufferSymbol(data, size, typeSym);
}

RFieldSymbol* ResourceParserState::NewFieldSymbol(int32 fieldid,
												  RSymbol* value,
												  RSymbol* type) const
{
	RFieldSymbol* sym = new RFieldSymbol(fieldid, value, type);
	sym->SetParser(const_cast<ResourceParserState*>(this));
	return sym;
}

RCompoundSymbol* ResourceParserState::NewCompoundSymbol(RSymbol* type) const
{
	if( !type ) type = CompoundType();
	RCompoundSymbol* sym = new RCompoundSymbol(0, type);
	sym->SetParser(const_cast<ResourceParserState*>(this));
	return sym;
}

RMessageSymbol* ResourceParserState::NewMessageSymbol(uint32 what, RSymbol* type) const
{
	if( !type ) type = MessageType();
	RMessageSymbol* sym = new RMessageSymbol(0, type, what);
	sym->SetParser(const_cast<ResourceParserState*>(this));
	return sym;
}

RArchiveSymbol* ResourceParserState::NewArchiveSymbol(const char* addon, uint32 what,
											   RSymbol* type) const
{
	if( !type ) type = ArchiveType();
	RArchiveSymbol* sym = new RArchiveSymbol(0, type, addon, what);
	sym->SetParser(const_cast<ResourceParserState*>(this));
	return sym;
}

void ResourceParserState::DeleteSymbol(RSymbol* symbol) const
{
	debugger("Noooooooo...  don't go there!!!");
	delete symbol;
}

void ResourceParserState::StartResource()
{
	fResourceFile = CurrentFile();
	fResourceLine = CurrentLine();
}

int32 ResourceParserState::PushType(RSymbol* type)
{
	fTypeStack.push_back(type);
	return fTypeStack.size()-1;
}

int32 ResourceParserState::PopType(RSymbol* type)
{
	if( fTypeStack.size() <= 0 ) {
		debugger("Type stack error: tried to pop from empty stack.");
		return 0;
	}
	
	if( type && fTypeStack.back() != type ) {
		debugger("Type stack error: expected pop type does not match stack.");
		return 0;
	}
	
	fTypeStack.pop_back();
	return fTypeStack.size();
}

RSymbol* ResourceParserState::CurrentType() const
{
	if( fTypeStack.size() <= 0 ) return NULL;
	return fTypeStack.back();
}
	
bool ResourceParserState::CanIncludeFile() const
{
	return include_stack_ptr < MAX_INCLUDE_DEPTH;
}

status_t ResourceParserState::PushIncludeFile(const char* name, void* data)
{
	if( !CanIncludeFile() ) {
		Error(B_BAD_INDEX, "Includes nested too deeply");
		return B_BAD_INDEX;
	}
	
	include_lineno_stack[include_stack_ptr] = yylineno;
	fFileName[include_stack_ptr] = name;
	include_stack[include_stack_ptr++] = data;
	
	return B_OK;
}

void* ResourceParserState::PopIncludeFile()
{
	if (include_stack_ptr <= 0) return NULL;
		
	include_stack_ptr--;
	yylineno = include_lineno_stack[include_stack_ptr];
	fFileName[include_stack_ptr] = "";
	return include_stack[include_stack_ptr];
}

status_t ResourceParserState::AddMacro(char* s)
{
	char *m = s + strlen("#define");
	while (isspace(*m)) m++;
	
	char *c = m;
	while (!isspace(*c)) c++;
	*c++ = 0;
	while (isspace(*c)) c++;
	
	int32 mn = AddIdentifier(m);
	
	fMacroTable[mn] = c;
	
	return B_OK;
}

const char* ResourceParserState::StartMacro(const char* sym, void* data)
{
	int ident = AddIdentifier(sym);
	
	if (fMacroTable.find(ident) != fMacroTable.end())
	{
		if (macro_stack_ptr >= MAX_MACRO_DEPTH) {
			Error(B_BAD_INDEX, "Macro's nested too deeply");
			return NULL;
		} else {
			macro_stack[macro_stack_ptr++] = data;
			return fMacroTable[ident].String();
		}
	}
	
	return NULL;
}

void* ResourceParserState::EndMacro()
{
	if( macro_stack_ptr <= 0 ) return NULL;
	
	macro_stack_ptr--;
	return macro_stack[macro_stack_ptr];
}
