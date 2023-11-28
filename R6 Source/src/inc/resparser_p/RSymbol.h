#ifndef _R_SYMBOL_H
#define _R_SYMBOL_H

#include <Ref.h>

#include <String.h>
#include <DataIO.h>
#include <Flattenable.h>

namespace BPrivate {

class ResourceParserState;
class RSymbol;
class RFieldSymbol;

void PrintSymbol(ResourceParserState* parser, RSymbol* sym, int32 level=0);

#if DEBUG
#define PRINT_SYMBOL(x, y) PrintSymbol(x, y)
#else
#define PRINT_SYMBOL(x, y)
#endif

enum symbol_code {
	B_EMPTY_SYMBOL,				// Should this be here?
	B_INTEGER_SYMBOL,
	B_REAL_SYMBOL,
	B_STRING_SYMBOL,
	B_BUFFER_SYMBOL,
	B_FIELD_SYMBOL,
	B_COMPOUND_SYMBOL
};

class RSymbol : public BRefable, public BFlattenable
{
public:
	RSymbol(RSymbol* type, type_code typeCode = 0);
	RSymbol(int32 ident, RSymbol* type, type_code typeCode = 0);
	~RSymbol();
	
	// Get and set the identifier for the symbol.  If set, this a
	// named symbol in the symbol table.
	void SetIdentifier(int32 ident);
	int32 Identifier() const;
	
	// Return the most descriptive identifier for this symbol.
	int32 BestIdentifier() const;
	
	// Return the type of symbol this is.
	symbol_code Symbol() const;
	
	// If this is a B_FIELD_SYMBOL, return the information about
	// its associated value.  Otherwise, the field symbole is simply
	// this very object.
	virtual symbol_code FieldSymbol() const;
	virtual RSymbol* FieldValue();
	
	// Get and set the type of this symbol.  If this is a base type,
	// Type() is NULL.
	void SetType(RSymbol* type);
	const RSymbol* Type() const;
	RSymbol* Type();
	
	// Get and set the type code for this symbol.  If the type code
	// is 0, the returned type code is taken from Type().
	void SetTypeCode(type_code code);
	// TypeCode() is in the BFlattenable interface.
	
	// Get the base type information for this symbol.  This is the
	// fundamental type -- i.e., a type that was manually added to
	// the symbol table before the parser was run and is not derived
	// from any other type.
	RSymbol* BaseType();
	const RSymbol* BaseType() const;
	type_code BaseTypeCode() const;
	
	// Returns true if this symbol has a fully-defined value.
	// Otherwise, some values within it have not yet been
	// specified -- i.e., it is a type.
	virtual bool HasValue() const;
	
	// Remove any value attached to this symbol.
	virtual void ClearThisValue();
	
	// Attempt to convert this symbol to another type.
	//	'type' is the new type to convert to.
	//	'force' indicates whether this is an explicit conversion -- if
	//		true, any possible conversion is allowed; otherwise, only
	//		conversions that don't drop precision are allowed.
	//		If a conversion is not possible, NULL is returned.
	//	'new_result' indicates whether the returned object should always
	//		be a new value -- if false, the passed in object will be
	//		returned if no change is needed or the reference count on
	//		this object is 1.  Otherwise, a new copy will always be
	//		returned.
	RSymbol* CastTo(RSymbol* type,
					bool force=false, bool new_result=false);
	
	// Create a copy of this symbol.  If 'always' is false, the same
	// symbol will be returned if there is currently only one reference
	// on it.  The returned copy is exactly the same as the original,
	// except if same_ident is false, in which case the new symbol's
	// Identifier() is cleared.
	//
	// Note that if you are cloning a base type, the returned object
	// is NOT a new base type -- its type points to the original base.
	// If you actually want to make a new base type, you must do this
	// by explicitly constructing such an object.
	virtual RSymbol* Clone(bool always=true, bool same_ident=false);
	
	// Return true if this is a compound symbol -- i.e., it contains
	// child symbols.
	virtual bool IsCompound() const;
	
	// Add a field to this object.  This is actually handled by
	// the object's type, enforcing correct semantics.  Returns
	// B_OK if all is okay, else an error code and (if parser is
	// supplied) reports an error.
	virtual status_t ApplyField(RFieldSymbol* value,
								ResourceParserState* parser = 0);
	
	// Move through symbols at a single level.
	RSymbol* Next();
	RSymbol* Prev();
	
	// Step through all symbols in a tree.
	RSymbol* StepNext(int32* level_change = 0);
	
	// BFlattenable interface
	virtual	bool		IsFixedSize() const;
	virtual	type_code	TypeCode() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual	status_t	Flatten(void *buffer, ssize_t size) const;
	virtual	bool		AllowsTypeCode(type_code code) const;
	virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);
	
	// A better flattenable interface
	virtual ssize_t		FlattenStream(BDataIO* stream) const;
	virtual ssize_t		UnflattenStream(type_code c, BDataIO* stream);
	
	virtual void SetParser(ResourceParserState* parser);
	
protected:
	RSymbol(int32 ident, RSymbol* type,
			symbol_code symbol = B_EMPTY_SYMBOL, type_code typeCode = 0);
	RSymbol(const RSymbol&);
	void InitObject(int32 ident, RSymbol* type,
					symbol_code symbol, type_code typeCode);
	
	virtual RSymbol* PerformStepNext(bool returning, int32* level_change = 0);
	
	// Perform a cast.  You should transform 'value' into a new symbol of
	// your type, if possible.  See CastTo() for the various options.
	virtual RSymbol* Transform(RSymbol* value,
							   bool force=false, bool new_result=false);
	
	friend class RCompoundSymbol;
	void SetNext(RSymbol* sym)				{ fNext = sym; }
	void SetPrev(RSymbol* sym)				{ fPrev = sym; }
	void SetParent(RSymbol* sym)			{ fParent = sym; }
	RSymbol* Parent()						{ return fParent; }
	
	ResourceParserState* Parser() const		{ return fParser; }
	
private:
	RSymbol& operator=(const RSymbol&);
	
	int32 fIdentifier;
	BRef<RSymbol> fType;
	BRef<RSymbol> fBaseType;
	symbol_code fSymbol;
	type_code fTypeCode;
	
	ResourceParserState* fParser;
	
	BRef<RSymbol> fNext;
	RSymbol* fPrev;
	RSymbol* fParent;
};

/*
 * Implements these basic types:
 * bool		(B_BOOL_TYPE)
 * int8		(B_INT8_TYPE)
 * uint8	(B_UINT8_TYPE)
 * int16	(B_INT16_TYPE)
 * uint16	(B_UINT16_TYPE)
 * int32	(B_INT32_TYPE)
 * uint32	(B_UINT32_TYPE)
 * int64	(B_INT64_TYPE)
 * uint64	(B_UINT64_TYPE)
 * ssize_T	(B_SSIZE_T_TYPE)
 * size_t	(B_SIZE_T_TYPE)
 * off_t	(B_OFF_T_TYPE)
 * time_t	(B_TIME_TYPE)
 */

class RIntegerSymbol : public RSymbol
{
public:
	RIntegerSymbol(int32 ident, int64 val, RSymbol* type);
	RIntegerSymbol(int32 ident, RSymbol* type);
	
	// For creation of integer types.
	RIntegerSymbol(int32 ident, type_code type);
	
	~RIntegerSymbol();
	
	virtual bool HasValue() const		{ return fHasValue; }
	virtual void ClearThisValue()		{ fHasValue = false; }
	
	virtual RSymbol* Clone(bool always=true, bool same_ident=false);
	
	void SetValue(int64 value);
	const int64& Value() const;
	int64& Value();

	virtual	bool		IsFixedSize() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual ssize_t		FlattenStream(BDataIO* stream) const;
	virtual ssize_t		UnflattenStream(type_code c, BDataIO* stream);
	
protected:
	RIntegerSymbol(const RIntegerSymbol&);
	virtual RSymbol* Transform(RSymbol* value,
							   bool force=false, bool new_result=false);

private:
	RIntegerSymbol& operator=(const RIntegerSymbol&);
	
	int64 fValue;
	bool fHasValue;
};

/*
 * Implements these basic types:
 * float	(B_FLOAT_TYPE)
 * double	(B_DOUBLE_TYPE)
 */

class RRealSymbol : public RSymbol
{
public:
	RRealSymbol(int32 ident, double val, RSymbol* type);
	RRealSymbol(int32 ident, RSymbol* type);
	
	// For creation of real types.
	RRealSymbol(int32 ident, type_code type);
	
	~RRealSymbol();
	
	virtual bool HasValue() const		{ return fHasValue; }
	virtual void ClearThisValue()		{ fHasValue = false; }
	
	virtual RSymbol* Clone(bool always=true, bool same_ident=false);
	
	void SetValue(double value);
	const double& Value() const;
	double& Value();

	virtual	bool		IsFixedSize() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual ssize_t		FlattenStream(BDataIO* stream) const;
	virtual ssize_t		UnflattenStream(type_code c, BDataIO* stream);
	
protected:
	RRealSymbol(const RRealSymbol&);
	virtual RSymbol* Transform(RSymbol* value,
							   bool force=false, bool new_result=false);

private:
	RRealSymbol& operator=(const RRealSymbol&);
	
	double fValue;
	bool fHasValue;
};

class RStringSymbol : public RSymbol
{
public:
	RStringSymbol(int32 ident, const char* val, RSymbol* type);
	RStringSymbol(int32 ident, RSymbol* type);
	
	// For creation of string types.
	RStringSymbol(int32 ident, type_code type);
	
	~RStringSymbol();
	
	virtual bool HasValue() const		{ return fHasValue; }
	virtual void ClearThisValue()		{ fHasValue = false; fValue = ""; }
	
	virtual RSymbol* Clone(bool always=true, bool same_ident=false);
	
	void SetValue(const char* value);
	const BString& Value() const;
	BString& Value();
	
	virtual	bool		IsFixedSize() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual ssize_t		FlattenStream(BDataIO* stream) const;
	virtual ssize_t		UnflattenStream(type_code c, BDataIO* stream);
	
protected:
	RStringSymbol(const RStringSymbol&);
	virtual RSymbol* Transform(RSymbol* value,
							   bool force=false, bool new_result=false);

private:
	RStringSymbol& operator=(const RStringSymbol&);
	
	BString fValue;
	bool fHasValue;
};

class RBufferSymbol : public RSymbol
{
public:
	RBufferSymbol(int32 ident, const void* val, size_t length, RSymbol* type);
	RBufferSymbol(int32 ident, const BMallocIO& value, RSymbol* type);
	RBufferSymbol(int32 ident, RSymbol* type);
	
	// For creation of buffer types.
	RBufferSymbol(int32 ident, type_code type);
	~RBufferSymbol();
	
	virtual bool HasValue() const		{ return fHasValue; }
	virtual void ClearThisValue()		{ fHasValue = false; fValue.SetSize(0); }
	
	virtual RSymbol* Clone(bool always=true, bool same_ident=false);
	
	void SetValue(const void* value, size_t length);
	const BMallocIO& Value() const;
	BMallocIO& Value();
	
	void AddLineLength(size_t len);
	size_t LineLength() const;
	
	void				SetFixedSize(bool isFixed);
	virtual	bool		IsFixedSize() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual ssize_t		FlattenStream(BDataIO* stream) const;
	virtual ssize_t		UnflattenStream(type_code c, BDataIO* stream);
	
protected:
	RBufferSymbol(const RBufferSymbol&);
	virtual RSymbol* Transform(RSymbol* value,
							   bool force=false, bool new_result=false);

private:
	RBufferSymbol& operator=(const RBufferSymbol&);
	
	BMallocIO fValue;
	bool fHasValue;
	bool fFixedSize;
	size_t fLineLength;
	size_t fLastLength;
};

class RFieldSymbol : public RSymbol
{
public:
	// NOTE: No check is made that 'type' is a valid type for 'value'.
	RFieldSymbol(int32 fieldid, RSymbol* value, RSymbol* type);
	RFieldSymbol(int32 fieldid, RSymbol* value, type_code type);
	~RFieldSymbol();
	
	virtual symbol_code FieldSymbol() const	{ return fValue->Symbol(); }
	virtual RSymbol* FieldValue()			{ return fValue; }

	virtual bool HasValue() const		{ return fValue != 0 ? true : false; }
	virtual void ClearThisValue()		{ fValue = 0; }
	
	virtual RSymbol* Clone(bool always=true, bool same_ident=false);
	
	void SetValue(RSymbol* value);
	const RSymbol* Value() const;
	RSymbol* Value();
	
	virtual	bool		IsFixedSize() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual ssize_t		FlattenStream(BDataIO* stream) const;
	virtual ssize_t		UnflattenStream(type_code c, BDataIO* stream);
	
	virtual void SetParser(ResourceParserState* parser);
	
protected:
	RFieldSymbol(const RFieldSymbol&);
	virtual RSymbol* Transform(RSymbol* value,
							   bool force=false, bool new_result=false);

private:
	RFieldSymbol& operator=(const RFieldSymbol&);
	
	BRef<RSymbol> fValue;
};

class RCompoundSymbol : public RSymbol
{
public:
	RCompoundSymbol(int32 ident, RSymbol* type);
	RCompoundSymbol(int32 ident, type_code type);
	~RCompoundSymbol();
	
	virtual bool HasValue() const;
	virtual void ClearThisValue();
	
	virtual RSymbol* Clone(bool always=true, bool same_ident=false);
	
	virtual bool IsCompound() const;
	
	virtual status_t ApplyField(RFieldSymbol* value,
								ResourceParserState* parser = 0);
	
	// Low-level adding and removing of fields in the array.  These
	// directly change the array, not allowing the type to handle
	// semantics.
	void AddField(RFieldSymbol* value, RSymbol* before = 0);
	void RemField(RFieldSymbol* value);
	
	void CopyFields(const RCompoundSymbol* from, bool append=false);
	
	// Retrieve the fields in the array.
	RFieldSymbol* FirstField() const;
	RFieldSymbol* LastField() const;
	RFieldSymbol* FieldFor(int32 ident) const;

	virtual	bool		IsFixedSize() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual ssize_t		FlattenStream(BDataIO* stream) const;
	virtual ssize_t		UnflattenStream(type_code c, BDataIO* stream);
	
	virtual void SetParser(ResourceParserState* parser);
	
protected:
	RCompoundSymbol(const RCompoundSymbol&);
	virtual RSymbol* PerformStepNext(bool returning, int32* level_change = 0);
	
	virtual RSymbol* Transform(RSymbol* value,
							   bool force=false, bool new_result=false);

	// Subclasses can hook this to plug in their own class type when
	// transforming a value, rather than having to completely re-implement
	// Transform().
	virtual RSymbol* CloneValue(RCompoundSymbol* value, bool new_result);

	// Similarily, subclass this to control what casts are allowed.
	// By default, a cast must be forced if the typeid of the value
	// is not RCompoundSymbol.
	virtual bool AllowCast(RCompoundSymbol* value, bool force) const;
	
private:
	RCompoundSymbol& operator=(const RCompoundSymbol&);
	
	BRef<RFieldSymbol> fHead;
	BRef<RFieldSymbol> fTail;
};

class RMessageSymbol : public RCompoundSymbol
{
public:
	RMessageSymbol(int32 ident, RSymbol* type, uint32 what = 0);
	RMessageSymbol(int32 ident, type_code type, uint32 what = 0);
	~RMessageSymbol();
	
	virtual bool HasValue() const		{ return false; }
	virtual void ClearThisValue()		{ }
	
	virtual RSymbol* Clone(bool always=true, bool same_ident=false);
	
	void SetWhat(uint32 what)			{ fWhat = what; }
	uint32 What() const					{ return fWhat; }
	
	virtual	bool		IsFixedSize() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual ssize_t		FlattenStream(BDataIO* stream) const;
	virtual ssize_t		UnflattenStream(type_code c, BDataIO* stream);
	
protected:
	RMessageSymbol(const RMessageSymbol&);
	
	virtual RSymbol* CloneValue(RCompoundSymbol* value, bool new_result);
	virtual bool AllowCast(RCompoundSymbol* value, bool force) const;

	virtual status_t	AddMessageFields(BMessage* msg) const;
	
private:
	RMessageSymbol& operator=(const RMessageSymbol&);
	
	status_t BuildMessage();
	void ForgetMessage();
	
	uint32 fWhat;
	BMessage* fMessage;
};

class RArchiveSymbol : public RMessageSymbol
{
public:
	RArchiveSymbol(int32 ident, RSymbol* type,
				   const char* addon = "", uint32 what = 0);
	RArchiveSymbol(int32 ident, type_code type,
				   const char* addon = "", uint32 what = 0);
	~RArchiveSymbol();

	virtual RSymbol* Clone(bool always=true, bool same_ident=false);
	
	void SetClass(const char* cls)			{ fClass = cls; }
	const char* Class() const				{ return fClass.String(); }
	
	void SetAddon(const char* add)			{ fAddon = add; }
	const char* Addon() const				{ return fAddon.String(); }
	
protected:
	RArchiveSymbol(const RArchiveSymbol&);

	virtual RSymbol* CloneValue(RCompoundSymbol* value, bool new_result);
	virtual bool AllowCast(RCompoundSymbol* value, bool force) const;

	virtual status_t	AddMessageFields(BMessage* msg) const;
	
private:
	RArchiveSymbol& operator=(const RArchiveSymbol&);
	
	BString fAddon;
	BString fClass;
};

}	// namespace BPrivate

using namespace BPrivate;

#endif
