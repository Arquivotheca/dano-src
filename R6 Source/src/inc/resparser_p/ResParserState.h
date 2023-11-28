#ifndef _RES_PARSER_STATE_H
#define _RES_PARSER_STATE_H

#include <ResourceParser.h>
#include <RSymbol.h>
#include <Ref.h>

#include <Path.h>
#include <String.h>

#include <stdio.h>
#include <vector>

#if __GNUC__ >= 3
#include <map>
#endif

namespace BPrivate {

	// if this include is outside the namespace, gcc throws up
	// on the two friend operator== and operator<
	// there is very likely a better way to do this.

#if __GNUC__ < 3
#include <map>
#endif

class RResourceData
{
public:
	RResourceData(RIntegerSymbol* id = 0, const char* label = 0)
		: fID(id), fLabel(label), fData(0), fTypeCode(0)
	{
	}
	~RResourceData()
	{
	}
	
	RIntegerSymbol* ID() const			{ return fID; }
	const char* Label() const			{ return fLabel.String(); }
	RSymbol* Data() const				{ return fData; }
	
	void SetData(RSymbol* data)			{ fData = data; }
	
	void SetTypeCode(type_code type)	{ fTypeCode = type; }
	type_code TypeCode() const			{ return fTypeCode ? fTypeCode : fData->TypeCode(); }
	
private:
	BRef<RIntegerSymbol> fID;
	BString fLabel;
	BRef<RSymbol> fData;
	type_code fTypeCode;
};

class ResourceParserContext
{
public:
	ResourceParserContext(ResourceParserState* state = 0);
	ResourceParserContext(const ResourceParserContext& o);
	~ResourceParserContext();

	ResourceParserContext& operator=(const ResourceParserContext& o);
	
	void SetState(ResourceParserState* parser);
	ResourceParserState* State() const;
	
	status_t PreInclude(const char* dir);
	status_t PostInclude(const char* dir);
	void SetSourceDir(const char* dir);
	
	BPath FindResourceFile(const char* name) const;
	
	int32 DefineBasicType(const char* name, symbol_code sym,
						  type_code type);
	int32 DefineCompoundType(const char* name, type_code type,
							 /*RSymbol* ftype, const char* fname,*/ ...);
	int32 DefineTypeAlias(const char* origType, const char* newType);
	
	// Symbol table.
	int32 AddIdentifier(const char* name);
	int32 AddSymbol(const char* name, RSymbol* symbol);
	void SetSymbol(int32 id, RSymbol* symbol);
	const char* IdentifierName(int32 id) const;
	RSymbol* IdentifierSymbol(int32 id) const;
	
	// Some common types.
	RIntegerSymbol*		Int32Type() const;
	RIntegerSymbol*		BoolType() const;
	RRealSymbol*		FloatType() const;
	RRealSymbol*		DoubleType() const;
	RStringSymbol*		StringType() const;
	RBufferSymbol*		BufferType() const;
	RCompoundSymbol*	CompoundType() const;
	RMessageSymbol*		MessageType() const;
	RArchiveSymbol*		ArchiveType() const;
	
private:
	ResourceParserState*	fState;
	
	std::vector<BString> fIncludePaths;
	BString fSourceIncludePath;
	
	std::map< BString, int32 > fSymbolTable;
	std::vector< BString > fIdentifiers;
	std::vector< BRef<RSymbol> > fSymbols;
	
	BRef<RIntegerSymbol>	fInt32Type;
	BRef<RIntegerSymbol>	fBoolType;
	BRef<RRealSymbol>		fFloatType;
	BRef<RRealSymbol>		fDoubleType;
	BRef<RStringSymbol>		fStringType;
	BRef<RBufferSymbol>		fBufferType;
	BRef<RCompoundSymbol>	fCompoundType;
	BRef<RMessageSymbol>	fMessageType;
	BRef<RArchiveSymbol>	fArchiveType;
};

class ResourceParserState
{
public:
	ResourceParserState(BResourceParser* parser);
	~ResourceParserState();
	
	// Parser context.
	void SetContext(const ResourceParserContext& o);
	const ResourceParserContext& Context() const	{ return fContext; }
	ResourceParserContext& Context()				{ return fContext; }
	
	// Current parser state.
	const char* CurrentFile() const;
	int32 CurrentLine() const;
	
	// Error reporting.
	void Error(status_t code, const char *e, ...) const;
	void Error(const char *e, ...) const;
	void Warn(const char *e, ...) const;
	
	// Resource generation.
	void WriteResource(RResourceData* res);
	void WriteResource(const char *file, int type, int id, const char *name);
	void Import(const char *file);

	// Filename lookup.
	BPath FindResourceFile(const char* name) const;

	// Symbol table.
	inline int32 AddIdentifier(const char* name)				{ return Context().AddIdentifier(name); }
	inline int32 AddSymbol(const char* name, RSymbol* symbol)	{ return Context().AddSymbol(name, symbol); }
	inline void SetSymbol(int32 id, RSymbol* symbol)			{ return Context().SetSymbol(id, symbol); }
	inline const char* IdentifierName(int32 id) const			{ return Context().IdentifierName(id); }
	inline RSymbol* IdentifierSymbol(int32 id) const			{ return Context().IdentifierSymbol(id); }

	// Generation of new symbols.
	RSymbol* NewSymbol(RSymbol* type = 0, type_code code = B_RAW_TYPE) const;
	RIntegerSymbol* NewIntegerSymbol(int64 value, RSymbol* type = 0) const;
	RRealSymbol* NewRealSymbol(double value, RSymbol* type = 0) const;
	RStringSymbol* NewStringSymbol(const char* value, RSymbol* type = 0) const;
	RBufferSymbol* NewBufferSymbol(const void* data, size_t size, RSymbol* type = 0) const;
	RBufferSymbol* NewBufferSymbol(const char* file, RSymbol* type = 0) const;
	RBufferSymbol* NewBufferSymbol(const char* resfile,
								   type_code restype, int resid, const char* resname,
								   RSymbol* type = 0) const;
	RFieldSymbol* NewFieldSymbol(int32 fieldid, RSymbol* value, RSymbol* type = 0) const;
	RCompoundSymbol* NewCompoundSymbol(RSymbol* type = 0) const;
	RMessageSymbol* NewMessageSymbol(uint32 what = 0, RSymbol* type = 0) const;
	RArchiveSymbol* NewArchiveSymbol(const char* addon = "",
									 uint32 what = B_ARCHIVED_OBJECT,
									 RSymbol* type = 0) const;
	
	void DeleteSymbol(RSymbol* symbol) const;

	// Some common types.
	inline RIntegerSymbol*	Int32Type() const		{ return Context().Int32Type(); }
	inline RIntegerSymbol*	BoolType() const		{ return Context().BoolType(); }
	inline RRealSymbol*		FloatType() const		{ return Context().FloatType(); }
	inline RRealSymbol*		DoubleType() const		{ return Context().DoubleType(); }
	inline RStringSymbol*	StringType() const		{ return Context().StringType(); }
	inline RBufferSymbol*	BufferType() const		{ return Context().BufferType(); }
	inline RCompoundSymbol*	CompoundType() const	{ return Context().CompoundType(); }
	inline RMessageSymbol*	MessageType() const		{ return Context().MessageType(); }
	inline RArchiveSymbol*	ArchiveType() const		{ return Context().ArchiveType(); }

	// Resource definition location tracking.
	void StartResource();
	
	// Value generation stack.  Push a type on to the stack to
	// start creating a value of that type, pop when done.
	// When calling PopType(), you should pass in what you think
	// was the last Push() to help validate the stack.
	int32 PushType(RSymbol* type);
	int32 PopType(RSymbol* type);
	RSymbol* CurrentType() const;
	
	// Include file stack.
	bool CanIncludeFile() const;
	status_t PushIncludeFile(const char* name, void* data);
	void* PopIncludeFile();

	// Macro storage and expansion stack.
	status_t AddMacro(char *s);
	const char* StartMacro(const char* sym, void* data);
	void* EndMacro();

	// Quick access to some methods in BResourceParser.
	static const char* TypeToString(type_code type, BString* out,
									bool fullContext = true)
		{ return BResourceParser::TypeToString(type, out, fullContext); }
	static const char* TypeIDToString(type_code type, int32 id, BString* out)
		{ return BResourceParser::TypeIDToString(type, id, out); }
	static type_code StringToType(const char* str)
		{ return BResourceParser::StringToType(str); }

	const char* TypeToString(type_code type, bool fullContext = true) const
		{ return fParser->TypeToString(type, fullContext); }
	const char* TypeIDToString(type_code type, int32 id) const
		{ return fParser->TypeIDToString(type, id); }
	
	
private:
	bool operator<(const ResourceParserState& other)
	{ return this < &other; }
	
	BResourceParser* fParser;
	
	ResourceParserContext fContext;
	
	std::vector< BRef<RSymbol> > fTypeStack;
	
	// Location where current resource definition started.
	BString fResourceFile;
	int32 fResourceLine;
	
	enum { MAX_INCLUDE_DEPTH = 10 };
	void* include_stack[MAX_INCLUDE_DEPTH];
	int include_lineno_stack[MAX_INCLUDE_DEPTH];
	BString fFileName[MAX_INCLUDE_DEPTH];
	int include_stack_ptr;
	
	enum { MAX_MACRO_DEPTH = 10 };
	void* macro_stack[MAX_MACRO_DEPTH];
	int macro_stack_ptr;
	
	std::map< int32, BString > fMacroTable;
};

} // namespace BPrivate

using namespace BPrivate;

#endif
