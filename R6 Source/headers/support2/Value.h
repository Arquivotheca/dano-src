/***************************************************************************
//
//	File:			support2/BValue.h
//
//	Description:	A general-purpose data container.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT2_VALUE_H_
#define _SUPPORT2_VALUE_H_

#include <support2/SupportDefs.h>
#include <support2/IByteStream.h>
#include <support2/ITextStream.h>
#include <support2/IBinder.h>
#include <support2/ByteOrder.h>
#include <support2/TypeConstants.h>

#include <string.h>

namespace B { namespace Private { struct shared_buffer; } }

namespace B {
namespace Support2 {

class BParcel;
class BFlattenable;
class BString;
class BValueMap;
class BValue;

//!	Flags for BValue operations.
enum {
	//!	don't traverse into nested values?
	B_NO_VALUE_RECURSION	= 0x00000001,
	//!	don't traverse into nested binders?
	B_NO_BINDER_RECURSION	= 0x00000002
};

//!	Flags for BValue::LockData().
enum {
	//!	keep existing data as-is.
	B_EDIT_VALUE_DATA		= 0x00000001
};

//!	Additional flags for PrintToStream().
enum {
	//!	print full type information?
	B_PRINT_VALUE_TYPES		= 0x00010000
};

//	--------------------------------------------------------------------

//!	A value_ref represents a single, type pile of bits.
/*!
	An instance of a value_ref does not own the data to which it refers --
	you must guarantee that the data exists for the lifetime of any value_ref
	pointing to it.  Generally a value_ref is only used as a transient
	object, such as to convert a string in to something that can be
	used with various BValue methods.  See BValue for more information
	on undefined and null values, comparison, etc.
	\sa BValue
*/
struct value_ref
{
			type_code			type;
			size_t				length;
			const void*			data;
	
			//!	Note that the default constructor \e DOES \e NOT
			//!	initialize the value_ref's fields.
								value_ref();
								
			//!	Create value_ref with an explicit initial value.
								value_ref(const value_ref& o);
								value_ref(type_code t, const void* d, size_t l);
								
			//! Create a value_ref for standard types.
			/*! Doing a \c value_ref(1) will work if you are careful, as
				a temporary integer will be created which exists for the
				rest of the scope the value_ref is in.
			*/
								value_ref(const char* string);
								value_ref(const int32& integer);
								
			//!	Create a value_ref from a BValue().
			/*!	This constructor is explicit because information may be lost --
				not all BValue data can be turned in to a value_ref.
			*/
	explicit					value_ref(const BValue& o);
	
								~value_ref();
	
			//!	Not-a-value.
	static	const value_ref		undefined;
			//!	The all-encompassing value.
	static	const value_ref		null;
			
			value_ref&			operator=(const value_ref& o);
	
			//!	Comparison that creates a "nice" (but undefined) order.
			int32				compare(const value_ref& o) const;
			//!	Comparison that is fast is well-defined.
			int32				fast_compare(const value_ref& o) const;
			
			//!	Synonyms for compare().
			//!	\sa compare
	inline	bool				operator==(const value_ref& o) const	{ return compare(o) == 0; }
	inline	bool				operator!=(const value_ref& o) const	{ return compare(o) != 0; }
	inline	bool				operator<(const value_ref& o) const		{ return compare(o) < 0; }
	inline	bool				operator<=(const value_ref& o) const	{ return compare(o) <= 0; }
	inline	bool				operator>=(const value_ref& o) const	{ return compare(o) >= 0; }
	inline	bool				operator>(const value_ref& o) const		{ return compare(o) > 0; }
			
			//!	Write a textual description of the value_ref
			//!	to the text stream \a io.
			void				print_to_stream(ITextOutput::arg io, uint32 flags = 0) const;

			//!	value_ref pretty printer function.
			/*!	\param	io		The stream to print in.
				\param	val		The value to print.
				\param	flags	Additional formatting information.
			*/
	typedef	status_t			(*print_func)(	ITextOutput::arg io,
												const value_ref& val,
												uint32 flags);
	
			//!	Add a new printer function for a type code.
			/*!	When value_ref::print_to_stream() or BValue::PrintToStream() is
				called, values of the given \a type will be printed by \a func.
			*/
	static	void				register_print_func(type_code type, print_func func);
			//!	Remove printer function for type code.
	static	void				unregister_print_func(type_code type);
	
private:
	friend	class				BValue;

			const void*			object;
};

ITextOutput::arg operator<<(ITextOutput::arg io, const value_ref& value);

// --------------------------------------------------------------------

//!	The BValue is a general-purpose data container.
/*!	BValue is formally an abstract group of (key->value) mappings, where
	each key and value can be any typed blob of data.  Standard data types
	are defined for strings, integers, Binder objects, and other common data
	types.  In addition, there are two special values: "undefined" is a
	BValue that is not set; "null" is a special value that can be thought of
	as being every possible value all at the same time.

	Note that the definition of BValue is very recursive -- it is a
	collection of key/value mappings, where both the key and value are
	themselves a BValue.  As such, a BValue containing a single data
	item is conceptually actually the mapping \c (null->value); because of
	the properties of null, a null key can appear more than one time
	so that a set of the values \c { A, B } can be constructed in a BValue
	as the mapping \c { (null->A), (null->B) }.  When iterating over and
	thinking of operations on a value, it is always viewed as a map in
	this way -- however, there are many convenience functions for working
	with a value in its common form of a single \c (null->A) item.
*/
class BValue
{
public:
							BValue();
							BValue(const value_ref& o);
							BValue(const BValue& o);
							
			//!	Create a value for a single explicit mapping.
							BValue(const BValue& key, const BValue& value);
							BValue(const value_ref& key, const BValue& value);
							BValue(const BValue& key, const value_ref& value);
							BValue(const value_ref& key, const value_ref& value);
							
			//!	Create specific value types, implicitly (null->A)
							BValue(type_code type, const void* data, size_t len);
							BValue(const BString& str);
							BValue(IBinder::arg binder);
							
							~BValue();
	
	/*	The two "special" values */
	
			//! not-a-value
	static	const BValue 	undefined;
			//!	the all-encompassing value
	static	const BValue 	null;
	
	/*	Generating standard values */
	
	static	const BValue&	Undefined();
	static	const BValue&	Null();
	static	BValue			Binder(IBinder::arg value);
	static	BValue			String(const char *value);
	static	BValue			String(const BString& value);
	static	BValue			Int8(int8 value);
	static	BValue			Int16(int16 value);
	static	BValue			Int32(int32 value);
	static	BValue			Int64(int64 value);
	static	BValue			Time(bigtime_t value);
	static	BValue			Bool(bool value);
	static	BValue			Float(float value);
	static	BValue			Double(double value);
	static	BValue			Flat(const BFlattenable& flat);
	static	BValue			Atom(const atom_ptr<BAtom>& value);
	static	BValue			AtomRef(const atom_ref<BAtom>& value);
	static	BValue			TypeInfo(const std::type_info& value);

	/* Assignment, status, existance */
	
			status_t		Status() const;
			
			BValue&			Assign(const BValue& o);
			BValue&			Assign(const value_ref& o);
			BValue&			Assign(type_code type, const void* data, size_t len);
	inline	BValue&			operator=(const BValue& o)			{ return Assign(o); }
	inline	BValue&			operator=(const value_ref& o)		{ return Assign(o); }

			//!	Exchange contents of 'this' and 'with'.
			void			Swap(BValue& with);
			
			//!	Set value to 'undefined'.
			void			Undefine();
			//!	Check whether the value is not 'undefined'.
			bool			IsDefined() const;
			/*!	\note This is really IsDefined() -- it returns a
				non-NULL value if the value is defined.
				\sa IsDefined
			*/
							operator const void*() const;
	
			//!	Checking whether this value is 'null'.
			bool			IsNull() const;
			
			//!	Check whether this value is neither 'undefined' nor 'null'.
			bool			IsSpecified() const;
			
			//!	Check whether this value is a single item of the form (null->A).
			/*!	A value_ref can only represent this kind of value.
			*/
			bool			IsSimple() const;
			
			status_t		CanByteSwap() const;
			status_t		ByteSwap(swap_action action);
			
	/* Mapping operations */
	
			//!	Apply mappings on top of this value.
			/*!	Overwrites any existing mappings in the value that are the
				same as the new mappings.
				
				For example:
					{ (A->B), (D->E) }.Overlay( { (A->G), (B->I) } )
				
				Results in:
					{ (A->G), (B->I), (D->E) }
			*/
			BValue&			Overlay(const BValue& from, uint32 flags = 0);
			const BValue	OverlayCopy(const BValue& from, uint32 flags = 0) const;
			
			//!	Convenience functions for overlaying a single map item.
			/*!	\note Overlay(null, A) is the same as Overlay(A).
			*/
			BValue&			Overlay(const BValue& key, const BValue& value);
			BValue&			Overlay(const value_ref& key, const BValue& value);
			BValue&			Overlay(const BValue& key, const value_ref& value);
			BValue&			Overlay(const value_ref& key, const value_ref& value);
			
			//!	Apply mappings underneath this value.
			/*!	Does not modify any existing mappings in the value.
			
				For example:
					{ (A->B), (D->E) }.Inherit( { (A->G), (B->I) } )
				
				Results in:
					{ (A->B), (B->I), (D->E) }
			*/
			BValue&			Inherit(const BValue& from, uint32 flags = 0);
			const BValue	InheritCopy(const BValue& from, uint32 flags = 0) const;
			
			//!	Convenience functions for inheriting a single map item.
			/*!	\note Inherit(null, A) is the same as Inherit(A).
			*/
			BValue&			Inherit(const BValue& key, const BValue& value);
			BValue&			Inherit(const value_ref& key, const BValue& value);
			BValue&			Inherit(const BValue& key, const value_ref& value);
			BValue&			Inherit(const value_ref& key, const value_ref& value);
			
			//!	Perform a cross product operation.
			/*!	This operation is defined as
				(A->B).Cross((D->E)) == (A->((D->E)[B])).  This is performed for
				every mapping in 'this', the results of which are aggregated
				using Inherit().
				
				For example:
					{ (A->B), (D->E) }.Cross( { (A->G), (B->I) } )
				
				Results in:
					{ (A->I) }
			*/
			BValue&			Cross(const BValue& from, uint32 flags = 0);
			const BValue	CrossCopy(const BValue& from, uint32 flags = 0) const;
	
			//!	Remove mappings from this value.
			/*!	For every mapping in 'from', if that \e key appears in
				'this' then it is removed.
			*/
			BValue&			Remove(const BValue& from, uint32 flags = 0);
			const BValue	RemoveCopy(const BValue& from, uint32 flags = 0) const;
			status_t		RemoveItem(	const value_ref& key,
										const value_ref& value = value_ref::undefined);
			
			//! Keep mappings in this value.
			/*!	For every mapping in 'this', if that key \e doesn't appear
				in 'from' then it is kept.  This is basically an intersect
				operation, with the additional definition that we use each
				item's value in 'this' and ignore the corresponding items
				in 'from'.
			*/
			BValue&			Retain(const BValue& from, uint32 flags = 0);
			const BValue	RetainCopy(const BValue& from, uint32 flags = 0) const;
			
			//!	Change the name of a key in the value.
			/*!	\note The old and new keys must be a specified value --
				you can not rename using null values.
				\sa IsSpecified
			*/
			status_t		RenameItem(	const value_ref& old_key,
										const BValue& new_key);
			
			//!	Check whether this value contains the given mapping.
			bool			HasItem(const value_ref& key,
									const value_ref& value = value_ref::undefined) const;
			
			//!	Look up a key in this value.
			/*!	Looking up a null key returns the entire value as-is.
				Looking up with a key that is a map will match all items
				of the map.
				
				For example:
					{ (A->(B->(D->E))) }.ValueFor( { (A->(B->(D))) } )
				
				Results in:
					{ E }
			*/
			const BValue	ValueFor(const BValue& key, uint32 flags = 0) const;
			const BValue	ValueFor(const value_ref& key, uint32 flags = 0) const;
	
			//!	Count the number of mappings in the value.
			int32			CountItems() const;
			//!	Iterate over the mappings in the value.
			status_t		GetNextItem(void** cookie, BValue* out_key, BValue* out_value) const;
			
			//! Direct editing of sub-items in the value.
			/*!	Only valid for a value that contains full mappings (A->B);
				\a key must be a simple value of one of these mappings.
			*/
			BValue*			BeginEditItem(const BValue &key);
			BValue*			BeginEditItem(const value_ref &key);
			void			EndEditItem(BValue* item);
			
			/*!	\sa ValueFor */
	inline	const BValue	operator[](const value_ref& key) const	{ return ValueFor(key); }
	inline	const BValue	operator[](const BValue& key) const		{ return ValueFor(key); }
	inline	const BValue	operator[](const char* key) const		{ return ValueFor(key); }
	
			/*!	\sa OverlayCopy */
	inline	const BValue	operator+(const BValue& o) const		{ return OverlayCopy(o); }
			/*!	\sa RemoveCopy */
	inline	const BValue	operator-(const BValue& o) const		{ return RemoveCopy(o); }
			/*!	\sa CrossCopy */
	inline	const BValue	operator*(const BValue& o) const		{ return CrossCopy(o); }

			/*!	\sa Overlay */
	inline	BValue&			operator+=(const BValue& o)				{ return Overlay(o); }
			/*!	\sa Remove */
	inline	BValue&			operator-=(const BValue& o)				{ return Remove(o); }
			/*!	\sa Cross */
	inline	BValue&			operator*=(const BValue& o)				{ return Cross(o); }
			
	/* Flattening */
	
			ssize_t			FlattenedSize() const;
			ssize_t			Flatten(void *buffer,
									ssize_t avail,
									BParcel* offsets = NULL) const;
			ssize_t			Flatten(IByteOutput::arg stream,
									BParcel* offsets = NULL) const;
	
			ssize_t			Unflatten(const void *buffer, size_t avail);
			ssize_t			Unflatten(IByteInput::arg stream);
	
			//!	These methods flatten and unflatten only the value's data.
			/*!	\note type and length are not included in the flattened buffer.
			*/
			ssize_t			DataFlattenedSize() const;
			ssize_t			DataFlatten(void *buffer,
										ssize_t avail,
										BParcel* offsets = NULL) const;
			ssize_t			DataFlatten(IByteOutput::arg stream,
										BParcel* offsets = NULL) const;
			ssize_t			DataUnflatten(	type_code type,
											const void *buffer,
											size_t length);
			ssize_t			DataUnflatten(	type_code type,
											IByteInput::arg stream,
											size_t length);
			
	/* Comparison */
	
			//!	Perform a comparison between two values.
			/*!	This comparison is smart about types, trying to get standard
				types in their natural order.  Because of this, note that the
				results of Compare() WILL CHANGE between operating system releases.
			*/
			int32			Compare(const BValue& o) const;
			int32			Compare(const value_ref& o) const;
			
			//!	Perform a quick comparison of values.
			/*!	Not only is this fast, but the way values are compared is
				type-independent and thus will never change.
			*/
			int32			FastCompare(const BValue& o) const;
			int32			FastCompare(const value_ref& o) const;
			
	inline	bool			operator==(const BValue& o) const		{ return Compare(o) == 0; }
	inline	bool			operator!=(const BValue& o) const		{ return Compare(o) != 0; }
	inline	bool			operator<(const BValue& o) const		{ return Compare(o) < 0; }
	inline	bool			operator<=(const BValue& o) const		{ return Compare(o) <= 0; }
	inline	bool			operator>=(const BValue& o) const		{ return Compare(o) >= 0; }
	inline	bool			operator>(const BValue& o) const		{ return Compare(o) > 0; }
	
	inline	bool			operator==(const value_ref& o) const	{ return Compare(o) == 0; }
	inline	bool			operator!=(const value_ref& o) const	{ return Compare(o) != 0; }
	inline	bool			operator<(const value_ref& o) const		{ return Compare(o) < 0; }
	inline	bool			operator<=(const value_ref& o) const	{ return Compare(o) <= 0; }
	inline	bool			operator>=(const value_ref& o) const	{ return Compare(o) >= 0; }
	inline	bool			operator>(const value_ref& o) const		{ return Compare(o) > 0; }
			
	/* Viewing the value as a standard type, potentially performing a conversion */
	
			IBinder::ptr	AsBinder(status_t* result = NULL) const;
			BString			AsString(status_t* result = NULL) const;
			bool			AsBool(status_t* result = NULL) const;
			int32			AsInt32(status_t* result = NULL) const;
			int64			AsInt64(status_t* result = NULL) const;
			bigtime_t		AsTime(status_t* result = NULL) const;
			float			AsFloat(status_t* result = NULL) const;
			double			AsDouble(status_t* result = NULL) const;
			atom_ptr<BAtom>	AsAtom(status_t* result = NULL) const;
			atom_ref<BAtom>	AsAtomRef(status_t* result = NULL) const;
	
			/*! \deprecated Use AsInt32() instead. */
	inline	int32			AsInteger(status_t* result = NULL) const { return AsInt32(result); }
			
	/* Retrieving basic value as an explicit standard type */
	
			status_t		GetBinder(IBinder::ptr *obj) const;
			status_t		GetString(const char** a_string) const;
			status_t		GetString(BString* a_string) const;
			status_t		GetBool(bool* val) const;
			status_t		GetInt8(int8* val) const;
			status_t		GetInt16(int16* val) const;
			status_t		GetInt32(int32* val) const;
			status_t		GetInt64(int64* val) const;
			status_t		GetTime(bigtime_t* val) const;
			status_t		GetFloat(float* a_float) const;
			status_t		GetDouble(double* a_double) const;
			status_t		GetFlat(BFlattenable* obj) const;
			status_t		GetAtom(atom_ptr<BAtom>* atom) const;
			status_t		GetAtomRef(atom_ref<BAtom>* atom) const;

	/* Raw data characteristics. */
	
			//!	Raw access.
			/*!	\note Data() and Length() are not valid if this object contains
				anything besides a simple values (i.e., two values in a set or
				one or more (key->value) mappings).
			*/
			type_code		Type() const;
			const void*		Data() const;
			size_t			Length() const;
			
			//!	Raw editing of data in value.
			/*!	Only valid on values for which IsSimple() is true.
				\sa IsSimple
			*/
			void*			BeginEditBytes(type_code type, size_t length, uint32 flags=0);
			status_t		EndEditBytes(ssize_t final_length=-1);
			
			void			PrintToStream(ITextOutput::arg io, uint32 flags = 0) const;
			
private:
	
	friend	class			value_ref;
	
			void			free_data();
			void*			alloc_data(size_t len);
			void			init_as_map(const BValue& key, const BValue& value);
			bool			is_map() const;
			bool			is_final() const;	// null or error
			status_t		remove_item_index(size_t index);
			status_t		find_item_index(size_t index,
											BValue* out_key, BValue* out_value) const;
			void			shrink_map(BValueMap* map);
			void*			edit_data(size_t len);
			status_t		copy_data(type_code type, void* buf, size_t len) const;
			BValueMap*		edit_map();
			status_t		set_error(ssize_t code);
			void			acquire_objects();
			void			release_objects();
			bool			check_integrity() const;
			
			struct value_ptr {
				// if m_length >= B_OK, this is set to the raw flattened data.
				const B::Private::shared_buffer* flat;
				
				// if m_length == OBJECT_MAGIC_LENGTH, this is set to the object.
				const BValueMap* map;
			};
			
			type_code		m_type;
			ssize_t			m_length;
			union {
				value_ptr	heap;
				uint8		local[8];
			}				m_data;
};

/*----- Type and STL utilities --------------------------------------*/
void			BMoveBefore(BValue* to, BValue* from, size_t count = 1);
void			BMoveAfter(BValue* to, BValue* from, size_t count = 1);
void			BSwap(BValue& v1, BValue& v2);
int32			BCompare(const BValue& v1, const BValue& v2);
void			swap(BValue& x, BValue& y);

ITextOutput::arg	operator<<(ITextOutput::arg io, const BValue& value);

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

inline value_ref::value_ref()
	{ }
inline value_ref::value_ref(const value_ref& o)
	: type(o.type), length(o.length), data(o.data), object(o.object) { }
inline value_ref::value_ref(type_code t, const void* d, size_t l)
	: type(t), length(l), data(d), object(NULL) { }
inline value_ref::value_ref(const char* string)
	: type(B_STRING_TYPE), length(strlen(string)+1), data(string), object(NULL) { }
inline value_ref::value_ref(const int32& integer)
	: type(B_INT32_TYPE), length(sizeof(integer)), data(&integer), object(NULL) { }
inline value_ref::~value_ref()
	{ }

inline value_ref& value_ref::operator=(const value_ref& o)
{
	type = o.type;
	length = o.length;
	data = o.data;
	object = o.object;
	return *this;
}

inline const BValue& BValue::Undefined()				{ return undefined; }
inline const BValue& BValue::Null()						{ return null; }
	
inline int32 BValue::Compare(const value_ref& o) const
{
	return value_ref(*this).compare(o);
}

inline int32 BValue::FastCompare(const value_ref& o) const
{
	return value_ref(*this).fast_compare(o);
}

inline BValue & BValue::Overlay(const value_ref& key, const value_ref& value)
{
	return Overlay(key, BValue(value));
}

inline BValue & BValue::Overlay(const BValue& key, const value_ref& value)
{
	return Overlay(key, BValue(value));
}

inline BValue & BValue::Inherit(const BValue& key, const value_ref& value)
{
	return Overlay(key, BValue(value));
}

inline void BSwap(BValue& v1, BValue& v2)
{
	v1.Swap(v2);
}

inline int32 BCompare(const BValue& v1, const BValue& v2)
{
	return v1.Compare(v2);
}

inline void swap(BValue& x, BValue& y)
{
	x.Swap(y);
}

} }	// namespace B::Support2

#endif	/* _SUPPORT2_VALUE_H_ */
