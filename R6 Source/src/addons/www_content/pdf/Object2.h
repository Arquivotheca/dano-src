#if !defined (_OBJECT2_H_)
#define _OBJECT2_H_

#include <ByteOrder.h>
#include <SupportDefs.h>
#include <vector>
#include <Debug.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

class BPositionIO;
class BAtomizer;

namespace BPrivate {

class PDFDocument;

class PDFObject;
typedef vector<PDFObject *> object_array_base;
class object_array : public object_array_base {
public:
		object_array() : object_array_base() {};
		object_array(uint32 size) : object_array_base(size) {};
		~object_array();
void	Release();
};

class PDFOpaque {			// base class for Opaque objects.  Forces virtual desctuctors.
public:
		PDFOpaque() {};
virtual	~PDFOpaque();
};

struct PDFOpaquePointer : public PDFOpaque {	// Opaque storage for unmanaged pointers.
		PDFOpaquePointer(void *p);
virtual	~PDFOpaquePointer();
void	*fPointer;
};

struct OpaqueBPositionIO : public PDFOpaque {
		OpaqueBPositionIO(BPositionIO *io);
virtual	~OpaqueBPositionIO();

		BPositionIO *m_io;
};


class PDFObject {
public:
	typedef enum {
		INVALID,
		ARRAY,
		BOOLEAN,
		DICTIONARY,
		KEYWORD,
		NAME,
		NILL,
		NUMBER,
		NUMBER48,
		OPAQUE,
		REAL,
		REFERENCE,
		STRING
	} pdf_class;

private:

	union {
#if B_HOST_IS_LENDIAN
		struct {
			uint8	__dummy0;
			uint8	__dummy1;
			uint8	__dummy2;
			uint8	_tag;	// object type: null, boolean, name, etc.
		} _t;
#else
		struct {
			uint8	_tag;
			uint8	__dummy0;
			uint8	__dummy1;
			uint8	__dummy2;
		} _t;
#endif
		uint32		_entry;	// index into xref table for this objects, zero if not xref'd
		PDFObject	*_next;	// free list management
	};
	int16	_ref_count; // reference count
	union {
		uint16	_size;
		uint16	_generation;
	};
	union {
		int32		_int; //
		float		_float;	// numbers
		uint8		*_uint8; // strings
		const char	*_name;	// names, keywords
		PDFOpaque	*_opaque; // opaque data
		PDFDocument	*_doc;
		bool		_bool;
		object_array *_array;
	};	// payload

					PDFObject(pdf_class c);
					PDFObject(bool b);
					PDFObject(double d);
					PDFObject(uint32 obj, uint16 gen, PDFDocument *doc);
					PDFObject(size_t len, const void *data);
					PDFObject(const char *atom);

					PDFObject(const PDFObject &rhs);
public:
					~PDFObject();
private:
PDFObject &			operator=(const PDFObject &rhs);
void *				operator new [] (size_t size);
void				operator delete [] (void *p, size_t size);
void *				operator new(size_t size);
void *				operator new(size_t , void *ptr) { return ptr; };
void				operator delete(void *p, size_t size);

int32				keyword_number(void) const;

static PDFObject	__NILL__;
static PDFObject	__TRUE__;
static PDFObject	__FALSE__;
static PDFObject	__ZERO__;

public:

uint32				GetXrefEntry(void)	{ return _entry & 0x00ffffff; };
void				SetXrefEntry(uint32 entry) { uint8 tag = _t._tag; _entry = entry; _t._tag = tag; };
int16				GetRefCount(void) { return _ref_count; };

bool				IsInvalid(void) { return this && (_t._tag == INVALID); };
bool				IsArray(void) { return this && (_t._tag == ARRAY); };
bool				IsBool(void) { return this && (_t._tag == BOOLEAN); };
bool				IsDictionary(void) { return this && (_t._tag == DICTIONARY); };
bool				IsKeyword(void) { return this && (_t._tag == KEYWORD); };
bool				IsName(void) { return this && (_t._tag == NAME); };
bool				IsNull(void) { return this && (_t._tag == NILL); };
bool				IsNumber(void) { return this && (_t._tag == NUMBER || _t._tag == NUMBER48 || _t._tag == REAL); };
bool				IsNumber48(void) { return this && (_t._tag == NUMBER48); };
bool				IsOpaque(void) { return this && (_t._tag == OPAQUE); };
bool				IsReal(void) { return this && (_t._tag == REAL); };
bool				IsReference(void) { return this && (_t._tag == REFERENCE); };
bool				IsString(void) { return this && (_t._tag == STRING); };

PDFObject *			AsArray(void);
PDFObject *			AsBool(void);
PDFObject *			AsDictionary(void);
PDFObject *			AsKeyword(void);
PDFObject *			AsName(void);
PDFObject *			AsNull(void);
PDFObject *			AsNumber(void);
PDFObject *			AsNumber48(void);
PDFObject *			AsOpaque(void);
PDFObject *			AsReal(void);
PDFObject *			AsReference(void);
PDFObject *			AsString(void);

size_t				Length(void) const { return _size; };
const uint8 *		Contents(void) const { return _uint8; };
uint32				Object(void) { return GetXrefEntry(); };
uint16				Generation(void) { return _generation; };

void				Acquire(void);
void				Release(void);

PDFObject *			Resolve(void);
bool				ResolveArrayOrDictionary(void);

#if 1
BPositionIO *		RealizeStream(void);
#endif

#ifndef NDEBUG
void				PrintToStream(int depth = 0, FILE *fd = stdout);
#endif

const char *		GetCharPtr() const { return this ? _name : 0; };
PDFOpaque *			GetOpaquePtr() const { return this ? _opaque : 0; };
int32				GetInt32() const;
int64				GetInt64() const;
float				GetFloat() const;
bool				GetBool() const { return this ? _bool : false; };

#if 0
operator const char *() const { return _name; };
operator PDFOpaque *() const { return _opaque; };
operator int32 () const;
operator int64 () const;
operator float () const;
#endif

void				PromoteToDictionary(void) { ASSERT(_t._tag == ARRAY); _t._tag = DICTIONARY; };
					// associate key/value pair.  Both objects' current references become
					// property of the dictionary
void				Assign(PDFObject *key, PDFObject *value);
					// drop key/value pair.  key remains the property of the caller.
void				Erase(PDFObject *key);
					// key must be an atom. That is: key == PDFObject::Atom(key)
void				Erase(const char *key);
PDFObject *			Find(PDFObject *key);
					// key must be an atom. That is: key == PDFObject::Atom(key)
PDFObject *			Find(const char *key);
					// add all key/value pairs in source NOT ALREADY in the target
					// acquires references to names and values added to target
void				Merge(PDFObject *source);

object_array *		Array(void) { ASSERT((_t._tag == ARRAY) || (_t._tag == DICTIONARY)); return _array; };

void				push_back(PDFObject *o) { return Array()->push_back(o); };
void				pop_back(void) { return Array()->pop_back(); };
PDFObject *			back(void) { return Array()->back(); };
size_t				size(void) { return Array()->size(); };
object_array::iterator
					begin() { return Array()->begin(); };

object_array::iterator
					end() { return Array()->end(); };

static
const char *		Atom(const char *name);
static
const char *		StaticAtom(const char *name);

static PDFObject	*makeNULL(void);
static PDFObject	*makeArray(void);
static PDFObject	*makeArray(object_array::iterator first, object_array::iterator last);
static PDFObject	*makeBoolean(bool t);
static PDFObject	*makeDictionary();
static PDFObject	*makeKeyword(const char *k);
static PDFObject	*makeName(const char *n);
static PDFObject	*makeNumber(double n);
static PDFObject	*makeNumber(const char *s);
static PDFObject	*makeOpaque(PDFOpaque *o);
static PDFObject	*makeReference(uint32 obj, uint16 gen, PDFDocument *doc);
static PDFObject	*makeString(size_t len, const void *data);
};

extern struct PDFAtoms {
typedef const char * ccharp;
ccharp
	A,
	A85,
	AbsoluteColorimetric,
	AHx,
	All,
	Alternate,
	Annots,
	ASCIIHexDecode,
	ASCII85Decode,
	BaseEncoding,
	BaseFont,
	BitsPerComponent,
	BlackIs1,
	BPC,
	__bogus__,
	CCF,
	CCITTFaxDecode,
	CalCMYK,
	CalGray,
	CalRGB,
	Catalog,
	CMYK,
	ColorSpace,
	Colors,
	Columns,
	Contents,
	Count,
	Courier,
	Courier_Bold,
	Courier_BoldOblique,
	Courier_Oblique,
	CropBox,
	CS,
	D,
	DamagedRowsBeforeError,
	DCT,
	DCTDecode,
	Decode,
	DecodeParms,
	DefaultGray,
	DefaultRGB,
	Dest,
	Dests,
	DeviceCMYK,
	DeviceGray,
	DeviceN,
	DeviceRGB,
	Differences,
	__document__,
	DP,
	EarlyChange,
	EncodedByteAlign,
	Encoding,
	Encrypt,
	EndOfBlock,
	EndOfLine,
	ExtGState,
	F,
	Filter,
	First,
	FirstChar,
	Fl,
	Flags,
	FlateDecode,
	Font,
	FontBBox,
	FontDescriptor, 
	__font_engine__,
	FontFile,
	FontFile2,
	FontFile3,
	G,
	Gamma,
	H,
	Height,
	Helvetica,
	Helvetica_Bold,
	Helvetica_BoldOblique,
	Helvetica_Oblique,
	I,
	ICCBased,
	ID,
	IM,
	Image,
	ImageMask,
	Indexed,
	Info,
	Interpolate,
	K,
	Kids,
	Lab,
	Last,
	LastChar, 
	Length,
	Limits,
	Link,
	LZW,
	LZWDecode,
	MacExpertEncoding,
	MacRomanEncoding,
	Mask,
	MediaBox,
	MissingWidth,
	N,
	Names,
	Next,
	None,
	__stream_off_t__,
	Outlines,
	Page,
	Pages,
	Parent,
	Pattern,
	Perceptual,
	__PixelFunctor__,
	Predictor,
	Prev,
	Range,
	Rect,
	RelativeColorimetric,
	Resources,
	RL,
	RGB,
	Root,
	Rows,
	RunLengthDecode,
	Saturation,
	Separation,
	Size,
	space, 
	__stream_data__,
	__stream_filter__,
	__stream_key__,
	Subtype,
	Symbol,
	Times_Roman,
	Times_Bold,
	Times_BoldItalic,
	Times_Italic,
	Title,
	ToUnicode,
	Type,
	Type0,
	Type1,
	Type3,
	W,
	WhitePoint,
	Width,
	Widths,
	WinAnsiEncoding,
	XObject,
	ZapfDingbats;
	PDFAtoms();
} PDFAtom;		// some common atoms


}; // namespace BPrivate

using namespace BPrivate;
#endif
