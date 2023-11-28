
#include <support2/Value.h>

#include <kernel/OS.h>
#include <support2/KeyedVector.h>
#include <support2/Atom.h>
#include <support2/Debug.h>
#include <support2/Flattenable.h>
#include <support2/ITextStream.h>
#include <support2/String.h>
#include <support2/TypeConstants.h>
#include <support2/Binder.h>
#include <support2/Parcel.h>
#include <support2/Looper.h>
#include <support2/Team.h>
#include <support2/StdIO.h>
#include <support2/StringIO.h>

#include <support2_p/BinderIPC.h>
#include <support2_p/SupportMisc.h>
#include <support2_p/ValueMap.h>

#include <ctype.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <typeinfo>

#define DB_INTEGRITY_CHECKS 1
#define DB_CORRECTNESS_CHECKS 1

namespace B {
namespace Support2 {

using namespace B::Private;

enum {
	OBJECT_MAGIC_LENGTH		= B_ERRORS_END
};

#if DB_INTEGRITY_CHECKS
#define CHECK_INTEGRITY(v) (v).check_integrity()
#else
#define CHECK_INTEGRITY(v)
#endif

#if DB_CORRECTNESS_CHECKS
static void check_correctness(const BValue& mine, const BValue& correct,
	const BValue& opLeft, const char* opName, const BValue& opRight)
{
	if (mine != correct) {
		BStringIO::ptr sio(new BStringIO);
		sio << "Correctness check failed!\nWhile computing:\n"
			<< indent << opLeft << dedent << endl
			<< opName << endl
			<< indent << opRight << dedent << endl
			<< "Result:\n"
			<< indent << (mine) << dedent << endl
			<< "Should have been:\n"
			<< indent << correct << dedent << endl;
		debugger(sio->String());
	}
}
#define INIT_CORRECTNESS(ops) ops
#define CHECK_CORRECTNESS(mine, correct, left, op, right) check_correctness((mine), (correct), (left), (op), (right))
#else
#define INIT_CORRECTNESS(ops)
#define CHECK_CORRECTNESS(mine, correct, left, op, right)
#endif

const value_ref value_ref::undefined(0, NULL, 0);
const value_ref value_ref::null(B_NULL_TYPE, &undefined, 0);

struct printer_registry {
	BLocker lock;
	BKeyedVector<type_code, value_ref::print_func> funcs;
	
	printer_registry()	: lock("value_ref printer registry"), funcs(NULL) { }
};

static int32 g_havePrinters = 0;
static printer_registry* g_printers = NULL;

static printer_registry* Printers()
{
	if (g_havePrinters&2) return g_printers;
	if (atomic_or(&g_havePrinters, 1) == 0) {
		g_printers = new(std::nothrow) printer_registry;
		atomic_or(&g_havePrinters, 2);
	} else {
		if ((g_havePrinters&2) == 0) snooze(10000);
	}
	return g_printers;
}

value_ref::value_ref(const BValue& o)
{
	type = o.m_type;
	if (o.m_length >= 0) {
		length = o.m_length;
		data = (length <= static_cast<ssize_t>(sizeof(o.m_data)))
				? (static_cast<const void*>(o.m_data.local))
				: (static_cast<const void*>(o.m_data.heap.flat->data()));
	} else {
		// XXX Should value_ref be able to contain errors?  Probably yes,
		// so that we can propagate memory allocation errors.
		data = NULL;
		length = 0;
	}
}

#define TYPED_DATA_COMPARE(d1, d2, type)													\
	( (*static_cast<const type*>(d1)) < (*static_cast<const type*>(d2))			\
		? -1																	\
		: (	(*static_cast<const type*>(d1)) > (*static_cast<const type*>(d2))	\
			? 1 : 0 )															\
	)

int32 value_ref::compare(const value_ref& o) const
{
	if (type != o.type) {
		return type < o.type ? -1 : 1;
	}
	
	// Handle standard types, if possible.
	if (length == o.length) {
		switch (type) {
			case 0:
			case B_NULL_TYPE:
				if (length == 0) return 0;
			case B_BOOL_TYPE:
			case B_INT8_TYPE:
				if (length == 1) return TYPED_DATA_COMPARE(data, o.data, int8);
				break;
			case B_UINT8_TYPE:
				if (length == 1) return TYPED_DATA_COMPARE(data, o.data, uint8);
				break;
			case B_INT16_TYPE:
				if (length == 2) return TYPED_DATA_COMPARE(data, o.data, int16);
				break;
			case B_UINT16_TYPE:
				if (length == 2) return TYPED_DATA_COMPARE(data, o.data, uint16);
				break;
			case B_INT32_TYPE:
				if (length == 4) return TYPED_DATA_COMPARE(data, o.data, int32);
				break;
			case B_UINT32_TYPE:
				if (length == 4) return TYPED_DATA_COMPARE(data, o.data, uint32);
				break;
			case B_INT64_TYPE:
				if (length == 8) return TYPED_DATA_COMPARE(data, o.data, int64);
				break;
			case B_UINT64_TYPE:
				if (length == 8) return TYPED_DATA_COMPARE(data, o.data, uint64);
				break;
			case B_FLOAT_TYPE:
				if (length == 4) return TYPED_DATA_COMPARE(data, o.data, float);
				break;
			case B_DOUBLE_TYPE:
				if (length == 8) return TYPED_DATA_COMPARE(data, o.data, double);
				break;
		}
	}
	
	int32 result = memcmp(data, o.data, length < o.length ? length : o.length);
	if (result == 0 && length != o.length) {
		return length < o.length ? -1 : 1;
	}
	return result;
}

#undef TYPED_DATA_COMPARE

int32 value_ref::fast_compare(const value_ref& o) const
{
	return B::Support2::fast_compare(*this, o);
}

static const char* UIntToHex(uint64 val, char* out)
{
	sprintf(out, "0x%Lx", val);
	return out;
}

void value_ref::print_to_stream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "value_ref(";
	
	if (length > 0 && data == NULL) {
		io << "NULL data of value_ref length " << length;
		if (flags&B_PRINT_STREAM_HEADER) io << ")";
		return;
	}
	
	bool handled = false;
	char buf[32];
	
	switch (type) {
	case 0: {
		if (length == 0) {
			io << "undefined";
			handled = true;
		}
	} break;
	case B_NULL_TYPE: {
		if (length == 0) {
			io << "null";
			handled = true;
		}
	} break;
	case B_CHAR_TYPE: {
		if (length == sizeof(char)) {
			char c[2];
			c[0] = *static_cast<const char*>(data);
			c[1] = 0;
			io << "char(" << (int32)c[0]
			   << " or " << UIntToHex((uint8)c[0], buf);
			if (isprint(c[0])) io << " or '" << c << "'";
			io << ")";
			handled = true;
		}
	} break;
	case B_INT8_TYPE: {
		if (length == sizeof(int8)) {
			int8 c[2];
			c[0] = *static_cast<const int8*>(data);
			c[1] = 0;
			io << "int8(" << (int32)c[0]
			   << " or " << UIntToHex((uint8)c[0], buf);
			if (isprint(c[0])) io << " or '" << (char*)&c[0] << "'";
			io << ")";
			handled = true;
		}
	} break;
	case B_INT16_TYPE: {
		if (length == sizeof(int16)) {
			io << "int16(" << *static_cast<const int16*>(data)
			   << " or " << UIntToHex(*(uint16*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_INT32_TYPE: {
		if (length == sizeof(int32)) {
			io << "int32(" << *static_cast<const int32*>(data)
			   << " or " << UIntToHex(*(uint32*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_INT64_TYPE: {
		if (length == sizeof(int64)) {
			io << "int64(" << *static_cast<const int64*>(data)
			   << " or " << UIntToHex(*(uint64*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_UINT8_TYPE: {
		if (length == sizeof(uint8)) {
			io << "uint8(" << (uint32)*static_cast<const uint8*>(data)
			   << " or " << UIntToHex(*(uint8*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_UINT16_TYPE: {
		if (length == sizeof(uint16)) {
			io << "uint16(" << (uint32)*static_cast<const uint16*>(data)
			   << " or " << UIntToHex(*(uint16*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_UINT32_TYPE: {
		if (length == sizeof(uint32)) {
			io << "uint32(" << *static_cast<const uint32*>(data)
			   << " or " << UIntToHex(*(uint32*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_UINT64_TYPE: {
		if (length == sizeof(uint64)) {
			io << "uint64(" << *static_cast<const uint64*>(data)
			   << " or " << UIntToHex(*(uint64*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_FLOAT_TYPE: {
		if (length == sizeof(float)) {
			io << "float(" << *static_cast<const float*>(data) << ")";
			handled = true;
		}
	} break;
	case B_DOUBLE_TYPE: {
		if (length == sizeof(double)) {
			io << "double(" << *static_cast<const double*>(data) << ")";
			handled = true;
		}
	} break;
	case B_BOOL_TYPE: {
		if (length == sizeof(bool)) {
			if (flags&B_PRINT_VALUE_TYPES) io << "bool(";
			io << ((*static_cast<const bool*>(data)) ? "true" : "false");
			if (flags&B_PRINT_VALUE_TYPES) io << ")";
			handled = true;
		}
	} break;
	case B_BIGTIME_TYPE: {
		if (length == sizeof(off_t)) {
			io << "bigtime_t(" << *static_cast<const bigtime_t*>(data)
			   << " or " << UIntToHex(*(bigtime_t*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_OFF_T_TYPE: {
		if (length == sizeof(off_t)) {
			io << "off_t(" << *static_cast<const off_t*>(data)
			   << " or " << UIntToHex(*(off_t*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_SIZE_T_TYPE: {
		if (length == sizeof(size_t)) {
			io << "size_t(" << *static_cast<const size_t*>(data)
			   << " or " << UIntToHex(*(size_t*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_SSIZE_T_TYPE: {
		if (length == sizeof(ssize_t)) {
			io << "ssize_t(" << *static_cast<const ssize_t*>(data)
			   << " or " << UIntToHex(*(size_t*)data, buf) << ")";
			handled = true;
		}
	} break;
	case B_POINTER_TYPE: {
		if (length == sizeof(void*)) {
			io << "pointer(" << UIntToHex(*static_cast<const size_t*>(data), buf) << ")";
			handled = true;
		}
	} break;
	case B_ATOM_TYPE: {
		if (length == sizeof(BAtom*)) {
			const BAtom* a = *static_cast<BAtom* const *>(data);
			io << "atom_ptr(" << a;
			if (a) io << " " << typeid(*a).name();
			io << ")";
			handled = true;
		}
	} break;
	case B_ATOMREF_TYPE: {
		if (length == sizeof(BAtom*)) {
			const BAtom* a = *static_cast<BAtom* const *>(data);
			io << "atom_ref(" << a;
			if (a) io << " " << typeid(*a).name();
			io << ")";
			handled = true;
		}
	} break;
	case B_BINDER_TYPE: {
		if (length == sizeof(BBinder*)) {
			const BBinder* b = *static_cast<BBinder* const *>(data);
			io << "BBinder(" << b;
			if (b) io << " " << typeid(*b).name();
			io << ")";
			handled = true;
		}
	} break;
	case B_BINDER_HANDLE_TYPE: {
		if (length == sizeof(int32)) {
			io << "binder_handle(" << UIntToHex(*static_cast<const int32*>(data), buf) << ")";
			handled = true;
		}
	} break;
	case B_SYSTEM_TYPE:
	case B_STRING_TYPE: {
		bool valid = true;
		if (length > 0) {
			if (static_cast<const char*>(data)[length-1] != 0) valid = false;
			for (size_t i=0; valid && i<length-1; i++) {
				if (static_cast<const char*>(data)[i] == 0) valid = false;
			}
		} else {
			valid = false;
		}
		if (valid) {
			if (type == B_STRING_TYPE) {
				if (flags&B_PRINT_VALUE_TYPES) io << "string(\"";
				else io << "\"";
				io << static_cast<const char*>(data);
				if (flags&B_PRINT_VALUE_TYPES) io << "\", " << length << " bytes)";
				else io << "\"";
			} else {
				io << "system(\"" << static_cast<const char*>(data) << "\")";
			}
			handled = true;
		} else if (type == B_STRING_TYPE) {
			io->BumpIndentLevel(1);
			io << "string " << length << " bytes: "
			   << (BHexDump(data, length).SetSingleLineCutoff(16));
			io->BumpIndentLevel(-1);
			handled = true;
		}
	} break;
	case B_VALUE_MAP_TYPE: {
		#if 0
		BValueMap map;
		status_t err = map.Unflatten(B_VALUE_MAP_TYPE, data, length);
		if (err >= B_OK) {
			io << map;
		} else {
			io << "BValueMap(" << strerror(err) << ")";
		}
		handled = true;
		#endif
	} break;
	default: {
		printer_registry* reg = Printers();
		if (reg && reg->lock.Lock() == B_OK) {
			print_func f = reg->funcs.ValueFor(type);
			reg->lock.Unlock();
			if (f) handled = ( (*f)(io, *this, flags|B_PRINT_STREAM_HEADER) == B_OK );
		}
	} break;
	}
	
	if (!handled) {
		io->BumpIndentLevel(1);
		io << BTypeCode(type) << " " << length << " bytes:"
		   << (BHexDump(data, length).SetSingleLineCutoff(16));
		io->BumpIndentLevel(-1);
	}
	
	if (flags&B_PRINT_STREAM_HEADER) io << ")";
}

void value_ref::register_print_func(type_code type, print_func func)
{
	printer_registry* reg = Printers();
	if (reg && reg->lock.Lock() == B_OK) {
		reg->funcs.AddItem(type, func);
		reg->lock.Unlock();
	}
}

void value_ref::unregister_print_func(type_code type)
{
	printer_registry* reg = Printers();
	if (reg && reg->lock.Lock() == B_OK) {
		reg->funcs.RemoveItemFor(type);
		reg->lock.Unlock();
	}
}

ITextOutput::arg operator<<(ITextOutput::arg io, const value_ref& value)
{
	value.print_to_stream(io, B_PRINT_STREAM_HEADER);
	return io;
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

const BValue BValue::undefined(0, NULL, 0);
const BValue BValue::null(B_NULL_TYPE, NULL, 0);

BValue::BValue()
	: m_type(0), m_length(0)
{
	//printf("*** Creating BValue %p Undefined\n", this);
	CHECK_INTEGRITY(*this);
}

BValue::BValue(const value_ref& o)
	: m_length(0)
{
	//printf("*** Creating BValue %p from value_ref\n", this);
	Assign(o);
}

BValue::BValue(const BValue& o)
	: m_type(o.m_type), m_length(o.m_length), m_data(o.m_data)
{
	//printf("*** Creating BValue %p from BValue\n", this);
	if (m_length <= static_cast<ssize_t>(sizeof(m_data))) {
		if (m_length == sizeof(void*)) {
			acquire_objects();
		} else if (m_length == OBJECT_MAGIC_LENGTH) {
			if (m_type == B_VALUE_MAP_TYPE) {
				m_data.heap.map->IncUsers();
			}
		}
	} else {
		m_data.heap.flat->inc_users();
	}
	CHECK_INTEGRITY(*this);
}

BValue::BValue(const BValue& key, const BValue& value)
{
	//printf("*** Creating BValue %p from BValue->BValue\n", this);
	init_as_map(key, value);
}

BValue::BValue(const value_ref& key, const BValue& value)
{
	//printf("*** Creating BValue %p from value_ref->BValue\n", this);
	init_as_map(key, value);
}

BValue::BValue(const BValue& key, const value_ref& value)
{
	//printf("*** Creating BValue %p from BValue->value_ref\n", this);
	init_as_map(key, value);
}

BValue::BValue(const value_ref& key, const value_ref& value)
{
	//printf("*** Creating BValue %p from value_ref->value_ref\n", this);
	init_as_map(key, value);
}

void BValue::init_as_map(const BValue& key, const BValue& value)
{
	if (key.IsDefined() && value.IsDefined()) {
		if (!key.IsNull()) {
			m_type = B_VALUE_MAP_TYPE;
			m_length = OBJECT_MAGIC_LENGTH;
			BValueMap* map = new(std::nothrow) BValueMap;
			m_data.heap.map = map;
			if (map) {
				const status_t err = map->SetFirstMap(key, value);
				if (err < B_OK) set_error(err);
			} else {
				set_error(B_NO_MEMORY);
			}
		} else {
			m_length = 0;
			Assign(value);
		}
	} else {
		m_length = 0;
		Undefine();
	}
	
	CHECK_INTEGRITY(*this);
}

BValue::BValue(type_code type, const void* data, size_t len)
	: m_length(0)
{
	//printf("*** Creating BValue %p from type,data,len\n", this);
	Assign(type, data, len);
}

BValue::BValue(const BString& str)
	: m_length(0)
{
	//printf("*** Creating BValue %p from BString\n", this);
	Assign(B_STRING_TYPE, str.String(), str.Length()+1);
}

BValue::BValue(IBinder::arg binder)
	: m_length(0)
{
	//printf("*** Creating BValue %p from IBinder\n", this);
	BBinder *local = binder->LocalBinder();
	if (!local) {
		RBinder *proxy = binder->RemoteBinder();
		ASSERT(proxy);
		int32 handle = proxy->Handle();
		Assign(B_BINDER_HANDLE_TYPE,&handle,sizeof(int32));
	} else
		Assign(B_BINDER_TYPE,&local,sizeof(BBinder*));
}

BValue::~BValue()
{
	//printf("*** Destroying BValue %p\n", this);
	Undefine();
}

inline void BValue::free_data()
{
	if (m_length > static_cast<ssize_t>(sizeof(m_data))) {
		m_data.heap.flat->dec_users();
	} else if (m_length == sizeof(void*)) {
		release_objects();
	} else if (m_length == OBJECT_MAGIC_LENGTH) {
		if (m_type == B_VALUE_MAP_TYPE)
			m_data.heap.map->DecUsers();
	}
}

inline void* BValue::alloc_data(size_t len)
{
	if (m_length != 0) free_data();
	
	m_length = len;
	if (len <= sizeof(m_data)) {
		return m_data.local;
	}
	
	if ((m_data.heap.flat=shared_buffer::create(len)) == NULL) {
		m_type = 0;
		m_length = B_NO_MEMORY;
		return NULL;
	}
	return const_cast<void*>(m_data.heap.flat->data());
}

BValue BValue::Binder(IBinder::arg obj)
{
	return BValue(obj);
}

BValue BValue::String(const char* value)
{
	return BValue(B_STRING_TYPE, value, strlen(value)+1);
}

BValue BValue::String(const BString& value)
{
	return BValue(B_STRING_TYPE, value.String(), value.Length()+1);
}

BValue BValue::Int8(int8 value)
{
	return BValue(B_INT8_TYPE, &value, sizeof(value));
}

BValue BValue::Int16(int16 value)
{
	return BValue(B_INT16_TYPE, &value, sizeof(value));
}

BValue BValue::Int32(int32 value)
{
	return BValue(B_INT32_TYPE, &value, sizeof(value));
}

BValue BValue::Int64(int64 value)
{
	return BValue(B_INT64_TYPE, &value, sizeof(value));
}

BValue BValue::Time(bigtime_t value)
{
	return BValue(B_BIGTIME_TYPE, &value, sizeof(value));
}

BValue BValue::Bool(bool value)
{
	return BValue(B_BOOL_TYPE, &value, sizeof(value));
}

BValue BValue::Float(float value)
{
	return BValue(B_FLOAT_TYPE, &value, sizeof(value));
}

BValue BValue::Double(double value)
{
	return BValue(B_DOUBLE_TYPE, &value, sizeof(value));
}

BValue BValue::Atom(const atom_ptr<BAtom>& value)
{
	return BValue(B_ATOM_TYPE, &value, sizeof(value));
}

BValue BValue::AtomRef(const atom_ref<BAtom>& value)
{
	return BValue(B_ATOMREF_TYPE, &value, sizeof(value));
}

BValue BValue::Flat(const BFlattenable& flat)
	DECLARE_RETURN(value)
{
	BValue value;
	
	const type_code type = flat.TypeCode();
	const ssize_t len = flat.FlattenedSize();
	void* buf = value.BeginEditBytes(type, len);
	if (buf) {
		status_t err = flat.Flatten(buf, len);
		value.EndEditBytes();
		if (err < B_OK) value.set_error(err);
	}
	CHECK_INTEGRITY(value);
	
	return value;
}

static int32 read_num(const char** p)
{
	int32 num=0;
	while(**p && isdigit(**p)) {
		num = (num*10) + (**p - '0');
		(*p)++;
	}
	return num;
}

BValue BValue::TypeInfo(const std::type_info& value)
{
	//return BValue(B_STRING_TYPE, value.name(), strlen(value.name())+1);
#if __GNUC__ <= 2
	const char *original = value.name();
	const char *name = original;
	BString buf;
	
	if (*name == 'Q') {
		// This class is in a namespace; need to decode.
		name++;
		if (!isdigit(*name)) debugger("Invalid namespace classname found");
		int32 count = *name - '0';
		name++;
		while (count > 0 && *name) {
			int32 len = read_num(&name);
			buf.Append(name, len);
			if (count > 1) buf.Append(":");
			while (len > 0 && *name) {
				len--;
				name++;
			}
			count--;
		}
		name = buf.String();
	} else {
		read_num(&name);
	}
	
	return BValue(name);
#else
	#error need to implement BValue::TypeInfo parsing for this platform
#endif
}

status_t BValue::Status() const
{
	return (m_length >= 0 || m_length == OBJECT_MAGIC_LENGTH) ? B_OK : m_length;
}

BValue& BValue::Assign(const BValue& o)
{
	if (this != &o) {
		if (m_length != 0) free_data();
		
		m_type = o.m_type;
		m_length = o.m_length;
		m_data = o.m_data;
		if (m_length <= static_cast<ssize_t>(sizeof(m_data))) {
			if (m_length == sizeof(void*)) {
				acquire_objects();
			} else if (m_length == OBJECT_MAGIC_LENGTH) {
				if (m_type == B_VALUE_MAP_TYPE) {
					m_data.heap.map->IncUsers();
				}
			}
		} else {
			m_data.heap.flat->inc_users();
		}
	}
	
	CHECK_INTEGRITY(*this);
	
	return *this;
}

BValue& BValue::Assign(const value_ref& value)
{
	return Assign(value.type, value.data, value.length);
}

BValue& BValue::Assign(type_code type, const void* data, size_t len)
{
	DataUnflatten(type, data, len);
	return *this;
}

typedef uint8 dummy_value[sizeof(BValue)];

void BValue::Swap(BValue& with)
{
	dummy_value buffer;
	buffer = *reinterpret_cast<dummy_value*>(this);
	*reinterpret_cast<dummy_value*>(&with) = *reinterpret_cast<dummy_value*>(this);
	*reinterpret_cast<dummy_value*>(this) = buffer;
}

void BValue::Undefine()
{
	if (m_length != 0) free_data();
	m_type = 0;
	m_length = 0;
	CHECK_INTEGRITY(*this);
}

bool BValue::IsDefined() const
{
	return m_type != 0 || m_length != 0;
}

BValue::operator const void*() const
{
	return (m_type != 0 || m_length != 0) ? this : NULL;
}

bool BValue::IsNull() const
{
	return m_type == B_NULL_TYPE && m_length >= 0;
}

bool BValue::IsSpecified() const
{
	return m_type != 0 && m_type != B_NULL_TYPE
		&& (m_length >= 0 || m_length == OBJECT_MAGIC_LENGTH);
}

inline bool BValue::is_final() const
{
	return (m_type == B_NULL_TYPE || (m_length < B_OK && m_length != OBJECT_MAGIC_LENGTH));
}

inline bool BValue::is_map() const
{
	return m_type == B_VALUE_MAP_TYPE && m_length == OBJECT_MAGIC_LENGTH;
}

bool BValue::IsSimple() const
{
	return !is_map();
}

status_t BValue::CanByteSwap() const
{
	return is_type_swapped(m_type);
}

status_t BValue::ByteSwap(swap_action action)
{
#if B_HOST_IS_LENDIAN
	if ((action == B_SWAP_HOST_TO_LENDIAN) || (action == B_SWAP_LENDIAN_TO_HOST))
		return B_OK;
#endif
#if B_HOST_IS_BENDIAN
	if ((action == B_SWAP_HOST_TO_BENDIAN) || (action == B_SWAP_BENDIAN_TO_HOST))
		return B_OK;
#endif
	
	if (m_length > 0) {
		void* data = edit_data(m_length);
		CHECK_INTEGRITY(*this);
		if (!data) return m_length;
		return swap_data(m_type, data, m_length, action);
	}
	return B_BAD_VALUE;
}

BValue& BValue::Overlay(const BValue& from, uint32 flags)
{
	if (from.is_final()) {
		*this = from;
		
	} else if (IsSpecified()) {
		size_t i=0;
		BValue k, v;
		while (from.GetNextItem(reinterpret_cast<void**>(&i), &k, &v) >= B_OK) {
			if (!(flags&B_NO_VALUE_RECURSION) && v.is_map()) {
				BValue orig(ValueFor(k));
				if (orig.IsDefined()) RemoveItem(value_ref(k), value_ref(orig));
				orig.Overlay(v);
				Overlay(k, orig);
			} else {
				Overlay(k, v);
			}
		}
	
	} else if (!IsDefined()) {
		*this = from;
	
	}
	
	CHECK_INTEGRITY(*this);
	return *this;
}

BValue & BValue::Overlay(const BValue& key, const BValue& value)
{
	if (!is_final() && key.IsDefined() && value.IsDefined()) {
		// Most of these checks are to ensure that the value stays
		// normalized, by not creating a BValueMap unless really needed.
		const bool nullkey = key.IsNull();
		if (nullkey && (!IsDefined() || value.is_final())) {
			// Handle (null -> A) special cases.
			Assign(value);
		} else if (nullkey && value.is_map()) {
			// Handle (null -> (B -> ...)) special case.
			Overlay(value);
		} else if (!nullkey || !IsSimple() || *this != value) {
			// Generic case.
			BValueMap* map = edit_map();
			if (map) {
				ssize_t result = map->OverlayMap(key, value);
				if (result < B_OK && result != B_BAD_VALUE) {
					set_error(result);
				}
			}
		}
	}
	
	CHECK_INTEGRITY(*this);
	return *this;
}

BValue & BValue::Overlay(const value_ref& key, const BValue& value)
{
	if (!is_final() && key != value_ref::undefined && value.IsDefined()) {
		// Most of these checks are to ensure that the value stays
		// normalized, by not creating a BValueMap unless really needed.
		const bool nullkey = key == value_ref::null;
		if (nullkey && (!IsDefined() || value.is_final())) {
			// Handle (null -> A) special cases.
			Assign(value);
		} else if (nullkey && value.is_map()) {
			// Handle (null -> (B -> ...)) special case.
			Overlay(value);
		} else if (!nullkey || !IsSimple() || *this != value) {
			// Generic case.
			BValueMap* map = edit_map();
			if (map) {
				ssize_t result = map->OverlayMap(key, value);
				if (result < B_OK && result != B_BAD_VALUE) {
					set_error(result);
				}
			}
		}
	}
	
	CHECK_INTEGRITY(*this);
	return *this;
}

const BValue BValue::OverlayCopy(const BValue& from, uint32 flags) const
	DECLARE_RETURN(out)
{
	// XXX this could be optimized.
	BValue out(*this);
	out.Overlay(from, flags);
	return out;
}

BValue& BValue::Inherit(const BValue& from, uint32 flags)
{
	if (from.Status() < B_OK) {
		set_error(from.m_length);
		return *this;
	}
	
	INIT_CORRECTNESS(BValue correct(from.OverlayCopy(*this)); BValue orig(*this));
	
	if (IsSpecified()) {
		size_t i1=0, i2=0;
		BValue k1, v1, k2, v2;
		bool first = true, finish = false;
		int32 cmp;
		while (from.GetNextItem(reinterpret_cast<void**>(&i2), &k2, &v2) >= B_OK) {
			cmp = -1;
			while (!finish && (first || (cmp=CompareMap(k1,v1,k2,v2)) < 0)) {
				if (GetNextItem(reinterpret_cast<void**>(&i1), &k1, &v1) < B_OK)
					finish = true;
				first = false;
			}
			
			if (cmp != 0) {
				Overlay(k2, v2);
			} else if (cmp == 0 && !(flags&B_NO_VALUE_RECURSION)
							&& v1.is_map() && v2.is_map()) {
				v1.Inherit(v2, true);
				if (v1.Status() >= B_OK) {
					Overlay(k1, v2);
				} else {
					set_error(v1.Status());
				}
			}
			
			// Skip to next item in 'this' -- either because we added an item
			// in 'from' in front of it, or because the items in the two values
			// matched so we can start with the following one next time around.
			i1++;
		}
	} else if (!IsDefined()) {
		*this = from;
	}
	
	CHECK_INTEGRITY(*this);
	CHECK_CORRECTNESS(*this, correct, orig, "Inherit", from);
	
	return *this;
}

BValue & BValue::Inherit(const BValue& key, const BValue& value)
{
	if (!is_final() && key.IsDefined() && value.IsDefined()) {
		// Most of these checks are to ensure that the value stays
		// normalized, by not creating a BValueMap unless really needed.
		const bool nullkey = key.IsNull();
		if (nullkey && (!IsDefined() || value.is_final())) {
			// Handle (null -> A) special cases.
			Assign(value);
		} else if (nullkey && value.is_map()) {
			// Handle (null -> (B -> ...)) special case.
			Inherit(value);
		} else if (!nullkey || !IsSimple() || *this != value) {
			// Generic case.
			BValueMap* map = edit_map();
			if (map) {
				ssize_t result = map->InheritMap(key, value);
				if (result < B_OK && result != B_BAD_VALUE) {
					set_error(result);
				}
			}
		}
	}
	
	CHECK_INTEGRITY(*this);
	return *this;
}

BValue & BValue::Inherit(const value_ref& key, const BValue& value)
{
	if (!is_final() && key != value_ref::undefined && value.IsDefined()) {
		// Most of these checks are to ensure that the value stays
		// normalized, by not creating a BValueMap unless really needed.
		const bool nullkey = key == value_ref::null;
		if (nullkey && (!IsDefined() || value.is_final())) {
			// Handle (null -> A) special cases.
			Assign(value);
		} else if (nullkey && value.is_map()) {
			// Handle (null -> (B -> ...)) special case.
			Inherit(value);
		} else if (!nullkey || !IsSimple() || *this != value) {
			// Generic case.
			BValueMap* map = edit_map();
			if (map) {
				ssize_t result = map->InheritMap(key, value);
				if (result < B_OK && result != B_BAD_VALUE) {
					set_error(result);
				}
			}
		}
	}
	
	CHECK_INTEGRITY(*this);
	return *this;
}

BValue & BValue::Inherit(const value_ref& key, const value_ref& value)
{
	if (!is_final() && key != value_ref::undefined && value != value_ref::undefined) {
		// Most of these checks are to ensure that the value stays
		// normalized, by not creating a BValueMap unless really needed.
		const bool nullkey = key == value_ref::null;
		if (nullkey && (!IsDefined() || value == value_ref::null)) {
			// Handle (null -> A) special cases.
			Assign(value);
		} else if (!nullkey || !IsSimple() || value_ref(*this) != value) {
			// Generic case.
			BValueMap* map = edit_map();
			if (map) {
				ssize_t result = map->InheritMap(key, value);
				if (result < B_OK && result != B_BAD_VALUE) {
					set_error(result);
				}
			}
		}
	}
	
	CHECK_INTEGRITY(*this);
	return *this;
}

const BValue BValue::InheritCopy(const BValue& from, uint32 flags) const
	DECLARE_RETURN(out)
{
	// XXX this could be optimized.
	BValue out(*this);
	out.Inherit(from, flags);
	return out;
}

BValue& BValue::Cross(const BValue& from, uint32 flags)
{
	// XXX this could be optimized.
	Assign(CrossCopy(from, flags));
	return *this;
}

const BValue BValue::CrossCopy(const BValue& from, uint32 flags) const
	DECLARE_RETURN(out)
{
	BValue out;
	
	#if DB_CORRECTNESS_CHECKS
		// This is the formal definition of Cross().
		BValue correct;
		void* i=NULL;
		BValue key, value;
		while (GetNextItem(&i, &key, &value) >= B_OK) {
			correct.Inherit(key, from.ValueFor(value, flags));
		}
	#endif
	
	if (Status() >= B_OK) {
		if (!from.is_final()) {
			void* i=NULL;
			BValue key1, value1, value2;
			while (GetNextItem(&i, &key1, &value1) >= B_OK) {
				value2 = from.ValueFor(value1, flags);
				if (value2.IsDefined()) out.Inherit(key1, value2);
			}
		} else if (from.IsNull() || is_final()) {
			out = *this;
		} else {
			out = from;
		}
	} else {
		out = *this;
	}
	
	CHECK_INTEGRITY(*this);
	CHECK_CORRECTNESS(out, correct, *this, "Cross", from);
	
	return out;
}

BValue& BValue::Remove(const BValue& from, uint32 flags)
{
	#if DB_CORRECTNESS_CHECKS
		// This is the formal definition of Remove().
		BValue orig(*this);
		BValue correct(*this);
		void* i=NULL;
		BValue key, value;
		while (from.GetNextItem(&i, &key, &value) >= B_OK) {
			if (!(flags&B_NO_VALUE_RECURSION) && value.is_map()) {
				BValue orig(ValueFor(key));
				orig.Remove(value);
				if (value.IsDefined()) correct.Overlay(key, value);
				else correct.RemoveItem(value_ref(key));
			} else {
				correct.RemoveItem(value_ref(key), value_ref(value));
			}
		}
	#endif
	
	if (from.Status() < B_OK) {
		*this = from;
	
	} else if (from.IsNull()) {
		Undefine();
		
	} else if (IsSpecified()) {
		size_t i1=0, i2=0;
		BValue k1, v1, k2, v2;
		bool first = true, finish = false;
		int32 cmp;
		while (from.GetNextItem(reinterpret_cast<void**>(&i2), &k2, &v2) >= B_OK) {
			if (!(flags&B_NO_VALUE_RECURSION) && v2.is_map()) {
				BValue orig(ValueFor(k2));
				if (orig.IsDefined()) {
					RemoveItem(value_ref(k2));
					orig.Remove(v2);
					Overlay(k2, orig);
				}
			} else {
				cmp = -1;
				while (!finish && (first || (cmp=CompareMap(k1,v1,k2,v2)) < 0)) {
					if (GetNextItem(reinterpret_cast<void**>(&i1), &k1, &v1) < B_OK)
						finish = true;
					first = false;
				}
				if (cmp == 0) {
					remove_item_index(i1-1);
					i1--;
				}
			}
		}
	
		if (is_map()) shrink_map(edit_map());
	}
	
	CHECK_INTEGRITY(*this);
	CHECK_CORRECTNESS(*this, correct, orig, "Remove", from);
	
	return *this;
}

const BValue BValue::RemoveCopy(const BValue& from, uint32 flags) const
	DECLARE_RETURN(out)
{
	// XXX this could be optimized.
	BValue out(*this);
	out.Remove(from, flags);
	return out;
}

status_t BValue::RemoveItem(const value_ref& key, const value_ref& value)
{
	status_t result;
	
	if (Status() >= B_OK && key != value_ref::undefined &&
				value != value_ref::undefined) {
		if (m_type == B_VALUE_MAP_TYPE) {
			BValueMap* map = edit_map();
			if (map) {
				result = map->RemoveMap(key, value);
				if (result >= B_OK) shrink_map(map);
			} else {
				result = B_NAME_NOT_FOUND;
			}
		} else if (key == value_ref::null &&
						(*this == value || value == value_ref::null)) {
			Undefine();
			result = B_OK;
		} else {
			result = B_NAME_NOT_FOUND;
		}
	} else {
		result = Status();
	}
	
	CHECK_INTEGRITY(*this);
	return result;
}

BValue& BValue::Retain(const BValue& from, uint32 flags)
{
	#if DB_CORRECTNESS_CHECKS
		// This is the formal definition of Retain().
		BValue orig(*this);
		BValue correct;
		void* i=NULL;
		BValue key, value;
		while (GetNextItem(&i, &key, &value) >= B_OK) {
			if (!(flags&B_NO_VALUE_RECURSION) && value.is_map()) {
				correct.Overlay(key, value.Retain(from.ValueFor(key), flags));
			} else if (key.IsNull() && value.IsNull()) {
				// XXX How to get rid of this??
				correct.Overlay(from);
			} else if (from.HasItem(value_ref(key), value_ref(value))) {
				correct.Overlay(key, value);
			}
		}
	#endif
	
	if (from.Status() < B_OK) {
		*this = from;
	
	} else if (IsNull()) {
		Assign(from);
		
	} else if (!from.IsDefined()) {
		Undefine();
		
	} else if (!from.IsNull()) {
		size_t i1=0, i2=0;
		BValue k1, v1, k2, v2;
		bool first = true, finish = false;
		int32 cmp;
		while (GetNextItem(reinterpret_cast<void**>(&i1), &k1, &v1) >= B_OK) {
			if (!(flags&B_NO_VALUE_RECURSION) && v1.is_map()) {
				BValue mod(from.ValueFor(k1));
				RemoveItem(value_ref(k1));
				if (mod.IsDefined()) {
					v1.Retain(mod);
					Overlay(k1, v1);
				}
			} else {
				cmp = -1;
				while (!finish && (first || (cmp=CompareMap(k1,v1,k2,v2)) < 0)) {
					if (from.GetNextItem(reinterpret_cast<void**>(&i2), &k2, &v2) < B_OK) {
						size_t n = CountItems();
						while (n >= i1) {
							remove_item_index(i1-1);
							n--;
						}
						finish = true;
					}
					first = false;
				}
				if (cmp != 0) {
					remove_item_index(i1-1);
					i1--;
				}
			}
		}
	
		if (is_map()) shrink_map(edit_map());
	}
	
	CHECK_INTEGRITY(*this);
	CHECK_CORRECTNESS(*this, correct, orig, "Retain", from);
	
	return *this;
}

const BValue BValue::RetainCopy(const BValue& from, uint32 flags) const
	DECLARE_RETURN(out)
{
	// XXX this could be optimized.
	BValue out(*this);
	out.Retain(from, flags);
	return out;
}

status_t BValue::RenameItem(const value_ref& old_key,
							const BValue& new_key)
{
	status_t result;
	
	if (Status() >= B_OK) {
		if (m_type == B_VALUE_MAP_TYPE) {
			BValueMap* map = edit_map();
			if (map) {
				result = map->RenameMap(old_key, new_key);
				if (result != B_OK && result != B_NAME_IN_USE
						&& result != B_NAME_NOT_FOUND && result != B_BAD_VALUE) {
					set_error(result);
				}
			} else {
				result = m_length < B_OK ? m_length : B_NAME_NOT_FOUND;
			}
		} else {
			result = B_NAME_NOT_FOUND;
		}
	} else {
		result = Status();
	}
	
	CHECK_INTEGRITY(*this);
	return result;
}

bool BValue::HasItem(const value_ref& key, const value_ref& value) const
{
	if (Status() >= B_OK) {
		if (IsNull()) {
			return true;
		} else if (key == value_ref::null) {
			if (value == value_ref::null) {
				return IsDefined();
			} else if (is_map()) {
				return m_data.heap.map->IndexFor(key, value) >= 0;
			} else {
				return *this == value;
			}
		} else {
			if (is_map()) {
				return m_data.heap.map->IndexFor(key, value) >= 0;
			}
		}
	}
	return false;
}

const BValue BValue::ValueFor(const BValue& key, uint32 flags) const
	DECLARE_RETURN(val)
{
	BValue val;
	if (IsNull()) {
		val = key;
	} else if (key == null) {
		val = *this;
	} else if (!(flags&B_NO_VALUE_RECURSION) && key.is_map()) {
		// Index by entire hierarchy, for each map in the key.
		void* i=NULL;
		BValue key1, value1, value2;
		while (key.GetNextItem(&i, &key1, &value1) >= B_OK) {
			value2 = ValueFor(key1);
			value2 = value2.ValueFor(value1);
			if (value2.IsDefined()) val.Inherit(value2);
		}
	} else if (is_map()) {
		ssize_t index = m_data.heap.map->IndexFor(value_ref(key));
		if (index >= B_OK) val = m_data.heap.map->MapAt(index).value;
	} else if ((m_type == B_BINDER_TYPE || m_type == B_BINDER_HANDLE_TYPE)
					&& !(flags&B_NO_BINDER_RECURSION)) {
		IBinder::ptr b(AsBinder());
		if (b != NULL) val = b->Get(key);
	} else if (key == *this) {
		val = *this;
	}
	return val;
}
	
const BValue BValue::ValueFor(const value_ref& key, uint32 flags) const
	DECLARE_RETURN(val)
{
	BValue val;
	if (IsNull()) {
		val = key;
	} else if (key == value_ref::null) {
		val = *this;
	} else if (is_map()) {
		ssize_t index = m_data.heap.map->IndexFor(key);
		if (index >= B_OK) val = m_data.heap.map->MapAt(index).value;
	} else if ((m_type == B_BINDER_TYPE || m_type == B_BINDER_HANDLE_TYPE)
					&& !(flags&B_NO_BINDER_RECURSION)) {
		IBinder::ptr b(AsBinder());
		if (b != NULL) val = b->Get(key);
	} else if (key == ((value_ref)*this)) {
		val = *this;
	}
	return val;
}

int32 BValue::CountItems() const
{
	return is_map() ? m_data.heap.map->CountMaps() : ( IsDefined() ? 1 : 0 );
}

status_t BValue::GetNextItem(void** cookie, BValue* out_key, BValue* out_value) const
{
	status_t result = find_item_index(*reinterpret_cast<size_t*>(cookie), out_key, out_value);
	if (result == B_OK) {
		*cookie = reinterpret_cast<void*>((*reinterpret_cast<size_t*>(cookie)) + 1);
	} else if (result == B_BAD_INDEX) {
		result = B_ENTRY_NOT_FOUND;
	}
	return result;
}

BValue *BValue::BeginEditItem(const BValue& key)
	DECLARE_RETURN(val)
{		
	BValue *val;
	if (key == null) {
		val = NULL;
	} else if (is_map()) {
		ssize_t index = m_data.heap.map->IndexFor(value_ref(key));
		if (index >= B_OK) {
			BValueMap* map = edit_map();
			val = map ? map->BeginEditMapAt(index) : NULL;
		} else {
			val = NULL;
		}
	} else {
		val = NULL;
	}
	
	return val;
}

BValue *BValue::BeginEditItem(const value_ref& key)
	DECLARE_RETURN(val)
{		
	BValue *val;
	if (key == value_ref::null) {
		val = NULL;
	} else if (is_map()) {
		ssize_t index = m_data.heap.map->IndexFor(key);
		if (index >= B_OK) {
			BValueMap* map = edit_map();
			val = map ? map->BeginEditMapAt(index) : NULL;
		} else {
			val = NULL;
		}
	} else {
		val = NULL;
	}
	
	return val;
}

void BValue::EndEditItem(BValue*)
{
	if (is_map()) {
		edit_map()->EndEditMapAt();
	} else {
		debugger("BeginEditItem has not been called");
	}
}

status_t BValue::remove_item_index(size_t index)
{
	status_t result;
	
	if (Status() >= B_OK) {
		if (is_map()) {
			const size_t N = m_data.heap.map->CountMaps();
			if (index == 0 && N <= 1) {
				Undefine();
				result = B_OK;
			
			} else if (index < m_data.heap.map->CountMaps()) {
				BValueMap* map = edit_map();
				if (map) {
					map->RemoveMapAt(index);
					shrink_map(map);
					result = B_OK;
				} else {
					result = B_NO_MEMORY;
				}
			} else {
				result = B_BAD_INDEX;
			}
		} else if (index == 0) {
			Undefine();
			result = B_OK;
		} else {
			result = B_BAD_INDEX;
		}
	} else {
		result = Status();
	}
	
	CHECK_INTEGRITY(*this);
	return result;
}

status_t BValue::find_item_index(size_t index, BValue* out_key, BValue* out_value) const
{
	status_t result;
	
	if (Status() >= B_OK) {
		if (is_map()) {
			if (index < m_data.heap.map->CountMaps()) {
				const BValueMap::pair& p = m_data.heap.map->MapAt(index);
				if (out_key) *out_key = p.key;
				if (out_value) *out_value = p.value;
				result = B_OK;
			} else {
				result = B_BAD_INDEX;
			}
		} else if (index == 0 && IsDefined()) {
			if (out_key) *out_key = null;
			if (out_value) *out_value = *this;
			result = B_OK;
		} else {
			result = B_BAD_INDEX;
		}
	} else {
		result = Status();
	}
	
	CHECK_INTEGRITY(*this);
	return result;
}

ssize_t BValue::FlattenedSize() const
{
	if (m_length >= B_OK) return sizeof(int32)*2 + m_length;
	else if (is_map()) return sizeof(int32)*2 + m_data.heap.map->FlattenedSize();
	return m_length;
}

ssize_t BValue::Flatten(void *buffer, ssize_t size, BParcel* offsets) const
{
	ssize_t err;
	int32* out = static_cast<int32*>(buffer);
	if (m_length >= B_OK && size >= static_cast<ssize_t>(sizeof(int32)*2+m_length)) {
		if (m_type == B_BINDER_TYPE || m_type == B_BINDER_HANDLE_TYPE) {
			if (offsets) {
				if ((err=offsets->AddBinder(0, sizeof(int32)*2)) < B_OK)
					return err;
			} else {
				debugger("Can't flatten BValue containing IBinder objects in to raw data.");
			}
		} else if (m_type == B_ATOM_TYPE || m_type == B_ATOMREF_TYPE) {
			debugger(	"Can't flatten BValue containing BAtom pointers "
						"(maybe you should have added binder objects instead).");
		}
		*out++ = m_type;
		*out++ = m_length;
		memcpy(out, Data(), m_length);
		return m_length + sizeof(int32)*2;
		
	} else if (is_map()) {
		*out++ = m_type;
		*out++ = m_data.heap.map->FlattenedSize();
		const size_t base = offsets ? offsets->MoveBase(sizeof(int32)*2) : 0;
		err = m_data.heap.map->Flatten(out, size-sizeof(int32)*2, offsets)
				+ sizeof(int32)*2;
		if (offsets) offsets->SetBase(base);
		return err;
		
	} else if (m_length < B_OK) return m_length;
	
	return B_MISMATCHED_VALUES;
}

ssize_t BValue::Flatten(IByteOutput::arg stream, BParcel* offsets) const
{
	ssize_t err;
	ssize_t written;
	
	if (m_length >= B_OK) {
		if (m_type == B_BINDER_TYPE || m_type == B_BINDER_HANDLE_TYPE) {
			if (offsets) {
				if ((err=offsets->AddBinder(0, sizeof(int32)*2)) < B_OK)
					return err;
			} else {
				debugger("Can't flatten BValue containing IBinder objects in to raw data.");
			}
		} else if (m_type == B_ATOM_TYPE || m_type == B_ATOMREF_TYPE) {
			debugger(	"Can't flatten BValue containing BAtom pointers "
						"(maybe you should have added binder objects instead).");
		}
		if (m_length > static_cast<ssize_t>(sizeof(m_data))) {
			written = stream->Write(&m_type, sizeof(int32)*2);
			if (written == sizeof(int32)*2) {
				written = stream->Write(m_data.heap.flat->data(), m_length);
				if (written >= B_OK) written += sizeof(int32)*2;
			}
		} else {
			written = stream->Write(&m_type, sizeof(int32)*2 + m_length);
		}
		
	} else if (is_map()) {
		written = stream->Write(&m_type, sizeof(int32));
		if (written == sizeof(int32)) {
			written = m_data.heap.map->FlattenedSize();
			if (written >= 0)
				written = stream->Write(&written, sizeof(int32));
		}
		if (written == sizeof(int32)) {
			const size_t base = offsets ? offsets->MoveBase(sizeof(int32)*2) : 0;
			written = m_data.heap.map->Flatten(stream, offsets);
			if (written >= B_OK) written += sizeof(int32)*2;
			if (offsets) offsets->SetBase(base);
		}
		
	} else {
		written = (m_length < B_OK) ? m_length : B_MISMATCHED_VALUES;
	}
	
	return written;
}

ssize_t BValue::Unflatten(const void *buffer, size_t avail)
{
	const int32* in = static_cast<const int32*>(buffer);
	type_code intype = *in++;
	size_t insize = *in++;
	if (avail >= (sizeof(int32)*2 + insize)) {
		const ssize_t amount = DataUnflatten(intype, in, insize);
		return amount >= B_OK ? (amount+sizeof(int32)*2) : amount;
	}
	
	Undefine();
	return B_BAD_VALUE;
}

ssize_t BValue::Unflatten(IByteInput::arg stream)
{
	int32 head[2];
	ssize_t res = stream->Read(head, sizeof(head));
	if (res == sizeof(head)) {
		if (head[0] == B_VALUE_MAP_TYPE) {
			BValueMap* obj = new(std::nothrow) BValueMap;
			if (obj) {
				if ((res=obj->Unflatten(stream)) >= B_OK) {
					free_data();
					m_type = B_VALUE_MAP_TYPE;
					m_data.heap.flat = NULL;
					m_data.heap.map = obj;
					m_length = OBJECT_MAGIC_LENGTH;
					CHECK_INTEGRITY(*this);
					return res + sizeof(head);
				}
				obj->DecUsers();
			} else {
				res = B_NO_MEMORY;
			}
		} else {
			void* buf = alloc_data(head[1]);
			if (buf) {
				m_type = head[0];
				res = stream->Read(buf, m_length);
				if (res == m_length) {
					if (m_length == sizeof(void*)) acquire_objects();
					CHECK_INTEGRITY(*this);
					return res + sizeof(head);
				}
			} else {
				// retrieve error from alloc_data().
				res = m_length;
			}
		}
	}
	
	set_error(res >= 0 ? B_BAD_VALUE : res);
	return m_length;
}

ssize_t BValue::DataFlattenedSize() const
{
	if (m_length >= B_OK) return m_length;
	else if (is_map()) return m_data.heap.map->FlattenedSize();
	return m_length;
}

ssize_t BValue::DataFlatten(void *buffer, ssize_t size, BParcel* offsets) const
{
	int32* out = static_cast<int32*>(buffer);
	if (m_length >= B_OK && size >= m_length) {
		memcpy(out, Data(), m_length);
		return m_length;
		
	} else if (is_map()) {
		return m_data.heap.map->Flatten(out, size, offsets);
		
	} else if (m_length < B_OK) return m_length;
	
	return B_MISMATCHED_VALUES;
}

ssize_t BValue::DataFlatten(IByteOutput::arg stream, BParcel* offsets) const
{
	if (m_length >= B_OK) {
		return stream->Write(Data(), m_length);
		
	} else if (is_map()) {
		return m_data.heap.map->Flatten(stream, offsets);
		
	} else if (m_length < B_OK) return m_length;
	
	return B_MISMATCHED_VALUES;
}

ssize_t BValue::DataUnflatten(type_code type, const void *buffer, size_t length)
{
	if (type == B_VALUE_MAP_TYPE) {
		BValueMap* obj = new(std::nothrow) BValueMap;
		if (obj == NULL) return set_error(B_NO_MEMORY);
		
		ssize_t amount = obj->Unflatten(buffer, length);
		if (amount < B_OK) {
			obj->DecUsers();
			return set_error(amount);
		}
		
		free_data();
		m_type = B_VALUE_MAP_TYPE;
		m_data.heap.flat = NULL;
		m_data.heap.map = obj;
		m_length = OBJECT_MAGIC_LENGTH;
		CHECK_INTEGRITY(*this);
		return amount;
	}
	void* buf = alloc_data(length);
	if (buf) {
		m_type = type;
		memcpy(buf, buffer, length);
		if (m_length == sizeof(void*)) acquire_objects();
	}
	CHECK_INTEGRITY(*this);
	return m_length;
}

ssize_t BValue::DataUnflatten(type_code type, IByteInput::arg stream, size_t length)
{
	if (type == B_VALUE_MAP_TYPE) {
		BValueMap* obj = new(std::nothrow) BValueMap;
		if (obj == NULL) return set_error(B_NO_MEMORY);
		
		const ssize_t amount = obj->Unflatten(stream);
		if (amount < B_OK || (length > 0 && amount != static_cast<ssize_t>(length))) {
			obj->DecUsers();
			return set_error(amount);
		}
		
		free_data();
		m_type = B_VALUE_MAP_TYPE;
		m_data.heap.flat = NULL;
		m_data.heap.map = obj;
		m_length = OBJECT_MAGIC_LENGTH;
		CHECK_INTEGRITY(*this);
		return amount;
	}
	void* buf = alloc_data(length);
	if (buf) {
		m_type = type;
		ssize_t amount = stream->Read(buf, length);
		if (amount < B_OK || (amount != static_cast<ssize_t>(length))) {
			return set_error(amount);
		}
		
		if (m_length == sizeof(void*)) acquire_objects();
	}
	CHECK_INTEGRITY(*this);
	return m_length;
}

int32 BValue::Compare(const BValue& o) const
{
	if (m_length != OBJECT_MAGIC_LENGTH && o.m_length != OBJECT_MAGIC_LENGTH)
		return value_ref(*this).compare(value_ref(o));
	if (m_type != o.m_type)
		return m_type < o.m_type ? -1 : 1;
	if (is_map() && o.is_map())
		return m_data.heap.map->Compare(*o.m_data.heap.map);
	debugger("BValue has mistaken magic length!");
	return 0;
}

int32 BValue::FastCompare(const BValue& o) const
{
	if (m_length != OBJECT_MAGIC_LENGTH && o.m_length != OBJECT_MAGIC_LENGTH)
		return value_ref(*this).fast_compare(value_ref(o));
	if (m_type != o.m_type)
		return m_type < o.m_type ? -1 : 1;
	if (is_map() && o.is_map())
		return m_data.heap.map->FastCompare(*o.m_data.heap.map);
	debugger("BValue has mistaken magic length!");
	return 0;
}

inline void BValue::shrink_map(BValueMap* map)
{
	switch (map->CountMaps()) {
		case 0: {
			Undefine();
		} break;
		case 1: {
			const BValueMap::pair& p = map->MapAt(0);
			if (p.key.IsNull()) {
				*this = p.value;
			}
		} break;
	}
}

status_t BValue::GetBinder(IBinder:: ptr *obj) const
{
	BBinder *binder;
	status_t r = copy_data(B_BINDER_TYPE, &binder, sizeof(binder));
	if (r == B_BAD_TYPE) {
		int32 handle;
		r = copy_data(B_BINDER_HANDLE_TYPE, &handle, sizeof(handle));
		if (!r) *obj = BLooper::Team()->GetProxyForHandle(handle);
	} else
		*obj = binder;
	return r;
}

status_t BValue::GetString(const char** a_string) const
{
	if (m_length < B_OK) return m_length;
	if (m_type != B_STRING_TYPE) return B_BAD_TYPE;
	*a_string = static_cast<const char*>(Data());
	return B_OK;
}

status_t BValue::GetString(BString* a_string) const
{
	if (m_length < B_OK) return m_length;
	if (m_type != B_STRING_TYPE) return B_BAD_TYPE;
	
	const size_t len = Length();
	const void* data = Data();
	
	if (len > 1) {
		char* c = a_string->LockBuffer(len-1);
		if (!c) {
			a_string->UnlockBuffer();
			return B_NO_MEMORY;
		}
		memcpy(c, data, len-1);
		a_string->UnlockBuffer(len-1);
	} else {
		*a_string = "";
	}
	return B_OK;
}

status_t BValue::GetBool(bool* val) const
{
	return copy_data(B_BOOL_TYPE, val, sizeof(*val));
}

status_t BValue::GetInt8(int8* val) const
{
	return copy_data(B_INT8_TYPE, val, sizeof(*val));
}

status_t BValue::GetInt16(int16* val) const
{
	return copy_data(B_INT16_TYPE, val, sizeof(*val));
}

status_t BValue::GetInt32(int32* val) const
{
	return copy_data(B_INT32_TYPE, val, sizeof(*val));
}

status_t BValue::GetInt64(int64* val) const
{
	return copy_data(B_INT64_TYPE, val, sizeof(*val));
}

status_t BValue::GetTime(bigtime_t* val) const
{
	return copy_data(B_BIGTIME_TYPE, val, sizeof(*val));
}

status_t BValue::GetFloat(float* a_float) const
{
	return copy_data(B_FLOAT_TYPE, a_float, sizeof(*a_float));
}

status_t BValue::GetDouble(double* a_double) const
{
	return copy_data(B_DOUBLE_TYPE, a_double, sizeof(*a_double));
}

status_t BValue::GetFlat(BFlattenable* obj) const
{
	if (m_length < B_OK) return m_length;
	if (!obj->AllowsTypeCode(m_type)) return B_BAD_TYPE;
	return obj->Unflatten(m_type, Data(), Length());
}

status_t BValue::GetAtom(atom_ptr<BAtom>* atom) const
	DECLARE_RETURN(err)
{
	BAtom* a;
	status_t err = copy_data(B_ATOM_TYPE, &a, sizeof(a));
	if (err == B_OK) *atom = a;
	return err;
}

status_t BValue::GetAtomRef(atom_ref<BAtom>* atom) const
	DECLARE_RETURN(err)
{
	BAtom* a;
	status_t err = copy_data(B_ATOMREF_TYPE, &a, sizeof(a));
	if (err != B_OK) err = copy_data(B_ATOM_TYPE, &a, sizeof(a));
	if (err == B_OK) *atom = a;
	return err;
}

IBinder::ptr
BValue::AsBinder(status_t *result) const DECLARE_RETURN(val)
{
	IBinder::ptr val;
	const status_t r = GetBinder(&val);
	if (result) *result = r;
	return val;
}

BString BValue::AsString(status_t* result) const DECLARE_RETURN(val)
{
	BString val;
	status_t r;
	switch (m_type) {
		case B_STRING_TYPE: {
			r = GetString(&val);
		} break;
		case B_DOUBLE_TYPE: {
			double ex;
			if ((r=GetDouble(&ex)) == B_OK) {
				char buffer[64];
				sprintf(buffer, "%g", ex);
				if( !strchr(buffer, '.') && !strchr(buffer, 'e') &&
					!strchr(buffer, 'E') ) {
					strncat(buffer, ".0", sizeof(buffer)-1);
				}
				val = buffer;
			}
		} break;
		case B_FLOAT_TYPE: {
			float ex;
			if ((r=GetFloat(&ex)) == B_OK) {
				char buffer[64];
				sprintf(buffer, "%g", (double)ex);
				if( !strchr(buffer, '.') && !strchr(buffer, 'e') &&
					!strchr(buffer, 'E') ) {
					strncat(buffer, ".0", sizeof(buffer)-1);
				}
				val = buffer;
			}
		} break;
		case B_INT64_TYPE: {
			int64 ex;
			if ((r=GetInt64(&ex)) == B_OK) {
				char buffer[64];
				sprintf(buffer, "%Ld", ex);
				val = buffer;
			}
		} break;
		case B_BIGTIME_TYPE: {
			bigtime_t ex;
			if ((r=GetTime(&ex)) == B_OK) {
				char buffer[64];
				sprintf(buffer, "%Ld", ex);
				val = buffer;
			}
		} break;
		case B_INT32_TYPE: {
			int32 ex;
			if ((r=GetInt32(&ex)) == B_OK) {
				char buffer[64];
				sprintf(buffer, "%ld", ex);
				val = buffer;
			}
		} break;
		case B_INT16_TYPE: {
			int16 ex;
			if ((r=GetInt16(&ex)) == B_OK) {
				char buffer[64];
				sprintf(buffer, "%d", ex);
				val = buffer;
			}
		} break;
		case B_INT8_TYPE: {
			int8 ex;
			if ((r=GetInt8(&ex)) == B_OK) {
				char buffer[64];
				sprintf(buffer, "%d", ex);
				val = buffer;
			}
		} break;
		case B_BOOL_TYPE: {
			bool ex;
			if ((r=GetBool(&ex)) == B_OK) {
				val = ex ? "true" : "false";
			}
		} break;
		case B_ATOM_TYPE: {
			atom_ptr<BAtom> ex;
			if ((r=GetAtom(&ex)) == B_OK) {
				char buffer[64];
				sprintf(buffer, "<atom_ptr %p>", ex.ptr());
				val = buffer;
			}
		} break;
		case B_ATOMREF_TYPE: {
			atom_ref<BAtom> ex;
			if ((r=GetAtomRef(&ex)) == B_OK) {
				char buffer[64];
				sprintf(buffer, "<atom_ref %p>", *reinterpret_cast<BAtom**>(&ex));
				val = buffer;
			}
		} break;
		case B_NULL_TYPE: {
			if (Length() == 0) {
				val = "<null>";
				r = B_OK;
			} else r = B_BAD_VALUE;
		} break;
		case 0: {
			if (Length() == 0) {
				val = "<undefined>";
				r = B_OK;
			} else r = B_BAD_VALUE;
		} break;
		default:
			r = B_BAD_TYPE;
	}
	if (result) *result = r;
	return val;
}

bool BValue::AsBool(status_t* result) const DECLARE_RETURN(val)
{
	bool val = false;
	status_t r;
	switch (m_type) {
		case B_STRING_TYPE: {
			const char* ex;
			if ((r=GetString(&ex)) == B_OK) {
				if (strcasecmp(ex, "true") == 0) val = true;
				else val = strtol(ex, const_cast<char**>(&ex), 10) != 0 ? true : false;
			}
		} break;
		case B_DOUBLE_TYPE: {
			double ex;
			if ((r=GetDouble(&ex)) == B_OK) {
				val = (ex > DBL_EPSILON || ex < (-DBL_EPSILON)) ? true : false;
			}
		} break;
		case B_FLOAT_TYPE: {
			float ex;
			if ((r=GetFloat(&ex)) == B_OK) {
				val = (ex > FLT_EPSILON || ex < (-FLT_EPSILON)) ? true : false;
			}
		} break;
		case B_INT64_TYPE: {
			int64 ex;
			if ((r=GetInt64(&ex)) == B_OK) {
				val = ex != 0 ? true : false;
			}
		} break;
		case B_BIGTIME_TYPE: {
			bigtime_t ex;
			if ((r=GetTime(&ex)) == B_OK) {
				val = ex != 0 ? true : false;
			}
		} break;
		case B_INT32_TYPE: {
			int32 ex;
			if ((r=GetInt32(&ex)) == B_OK) {
				val = ex != 0 ? true : false;
			}
		} break;
		case B_INT16_TYPE: {
			int16 ex;
			if ((r=GetInt16(&ex)) == B_OK) {
				val = ex != 0 ? true : false;
			}
		} break;
		case B_INT8_TYPE: {
			int8 ex;
			if ((r=GetInt8(&ex)) == B_OK) {
				val = ex != 0 ? true : false;
			}
		} break;
		case B_BOOL_TYPE: {
			r = GetBool(&val);
		} break;
		case B_ATOM_TYPE: {
			atom_ptr<BAtom> ex;
			if ((r=GetAtom(&ex)) == B_OK) {
				val = ex != NULL ? true : false;
			}
		} break;
		case B_ATOMREF_TYPE: {
			atom_ref<BAtom> ex;
			if ((r=GetAtomRef(&ex)) == B_OK) {
				val = ex != NULL ? true : false;
			}
		} break;
		default:
			r = B_BAD_TYPE;
	}
	if (result) *result = r;
	return val;
}

int32 BValue::AsInt32(status_t* result) const DECLARE_RETURN(val)
{
	int32 val = 0;
	status_t r;
	switch (m_type) {
		case B_STRING_TYPE: {
			const char* ex;
			if ((r=GetString(&ex)) == B_OK) {
				val = strtol(ex, const_cast<char**>(&ex), 10);
			}
		} break;
		case B_DOUBLE_TYPE: {
			double ex;
			if ((r=GetDouble(&ex)) == B_OK) {
				val = static_cast<int32>((ex > 0.0 ? (ex+.5) : (ex-.5)));
			}
		} break;
		case B_FLOAT_TYPE: {
			float ex;
			if ((r=GetFloat(&ex)) == B_OK) {
				val = static_cast<int32>((ex > 0.0 ? (ex+.5) : (ex-.5)));
			}
		} break;
		case B_INT64_TYPE: {
			int64 ex;
			if ((r=GetInt64(&ex)) == B_OK) {
				val = static_cast<int32>(ex);
			}
		} break;
		case B_BIGTIME_TYPE: {
			bigtime_t ex;
			if ((r=GetTime(&ex)) == B_OK) {
				val = static_cast<int32>(ex);
			}
		} break;
		case B_INT32_TYPE: {
			r = GetInt32(&val);
		} break;
		case B_INT16_TYPE: {
			int16 ex;
			if ((r=GetInt16(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_INT8_TYPE: {
			int8 ex;
			if ((r=GetInt8(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_BOOL_TYPE: {
			bool ex;
			if ((r=GetBool(&ex)) == B_OK) {
				val = ex ? 1 : 0;
			}
		} break;
		case B_ATOM_TYPE: {
			atom_ptr<BAtom> ex;
			if ((r=GetAtom(&ex)) == B_OK) {
				val = reinterpret_cast<int32>(ex.ptr());
			}
		} break;
		case B_ATOMREF_TYPE: {
			atom_ref<BAtom> ex;
			if ((r=GetAtomRef(&ex)) == B_OK) {
				val = reinterpret_cast<int32>(reinterpret_cast<atom_ptr<BAtom>*>(&ex)->ptr());
			}
		} break;
		default:
			r = B_BAD_TYPE;
	}
	if (result) *result = r;
	return val;
}

int64 BValue::AsInt64(status_t* result) const DECLARE_RETURN(val)
{
	int64 val = 0;
	status_t r;
	switch (m_type) {
		case B_STRING_TYPE: {
			const char* ex;
			if ((r=GetString(&ex)) == B_OK) {
				val = strtoll(ex, const_cast<char**>(&ex), 10);
			}
		} break;
		case B_DOUBLE_TYPE: {
			double ex;
			if ((r=GetDouble(&ex)) == B_OK) {
				val = static_cast<int64>((ex > 0.0 ? (ex+.5) : (ex-.5)));
			}
		} break;
		case B_FLOAT_TYPE: {
			float ex;
			if ((r=GetFloat(&ex)) == B_OK) {
				val = static_cast<int64>((ex > 0.0 ? (ex+.5) : (ex-.5)));
			}
		} break;
		case B_INT32_TYPE: {
			int32 ex;
			if ((r=GetInt32(&ex)) == B_OK) {
				val = static_cast<int64>(ex);
			}
		} break;
		case B_BIGTIME_TYPE: {
			bigtime_t ex;
			if ((r=GetTime(&ex)) == B_OK) {
				val = static_cast<int64>(ex);
			}
		} break;
		case B_INT64_TYPE: {
			r = GetInt64(&val);
		} break;
		case B_INT16_TYPE: {
			int16 ex;
			if ((r=GetInt16(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_INT8_TYPE: {
			int8 ex;
			if ((r=GetInt8(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_BOOL_TYPE: {
			bool ex;
			if ((r=GetBool(&ex)) == B_OK) {
				val = ex ? 1 : 0;
			}
		} break;
		case B_ATOM_TYPE: {
			atom_ptr<BAtom> ex;
			if ((r=GetAtom(&ex)) == B_OK) {
				val = reinterpret_cast<int64>(ex.ptr());
			}
		} break;
		case B_ATOMREF_TYPE: {
			atom_ref<BAtom> ex;
			if ((r=GetAtomRef(&ex)) == B_OK) {
				val = reinterpret_cast<int64>(reinterpret_cast<atom_ptr<BAtom>*>(&ex)->ptr());
			}
		} break;
		default:
			r = B_BAD_TYPE;
	}
	if (result) *result = r;
	return val;
}

bigtime_t BValue::AsTime(status_t* result) const DECLARE_RETURN(val)
{
	bigtime_t val = 0;
	status_t r;
	switch (m_type) {
		case B_STRING_TYPE: {
			const char* ex;
			if ((r=GetString(&ex)) == B_OK) {
				val = strtoll(ex, const_cast<char**>(&ex), 10);
			}
		} break;
		case B_DOUBLE_TYPE: {
			double ex;
			if ((r=GetDouble(&ex)) == B_OK) {
				val = static_cast<bigtime_t>((ex > 0.0 ? (ex+.5) : (ex-.5)));
			}
		} break;
		case B_FLOAT_TYPE: {
			float ex;
			if ((r=GetFloat(&ex)) == B_OK) {
				val = static_cast<bigtime_t>((ex > 0.0 ? (ex+.5) : (ex-.5)));
			}
		} break;
		case B_INT64_TYPE: {
			int64 ex;
			if ((r=GetInt64(&ex)) == B_OK) {
				val = static_cast<bigtime_t>(ex);
			}
		} break;
		case B_BIGTIME_TYPE: {
			r = GetTime(&val);
		} break;
		case B_INT32_TYPE: {
			int32 ex;
			if ((r=GetInt32(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_INT16_TYPE: {
			int16 ex;
			if ((r=GetInt16(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_INT8_TYPE: {
			int8 ex;
			if ((r=GetInt8(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_BOOL_TYPE: {
			bool ex;
			if ((r=GetBool(&ex)) == B_OK) {
				val = ex ? 1 : 0;
			}
		} break;
		default:
			r = B_BAD_TYPE;
	}
	if (result) *result = r;
	return val;
}

float BValue::AsFloat(status_t* result) const DECLARE_RETURN(val)
{
	float val = 0;
	status_t r;
	switch (m_type) {
		case B_STRING_TYPE: {
			const char* ex;
			if ((r=GetString(&ex)) == B_OK) {
				val = static_cast<float>(strtod(ex, const_cast<char**>(&ex)));
			}
		} break;
		case B_DOUBLE_TYPE: {
			double ex;
			if ((r=GetDouble(&ex)) == B_OK) {
				val = static_cast<float>(ex);
			}
		} break;
		case B_FLOAT_TYPE: {
			r = GetFloat(&val);
		} break;
		case B_INT64_TYPE: {
			int64 ex;
			if ((r=GetInt64(&ex)) == B_OK) {
				val = static_cast<float>(ex);
			}
		} break;
		case B_BIGTIME_TYPE: {
			bigtime_t ex;
			if ((r=GetTime(&ex)) == B_OK) {
				val = static_cast<float>(ex);
			}
		} break;
		case B_INT32_TYPE: {
			int32 ex;
			if ((r=GetInt32(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_INT16_TYPE: {
			int16 ex;
			if ((r=GetInt16(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_INT8_TYPE: {
			int8 ex;
			if ((r=GetInt8(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_BOOL_TYPE: {
			bool ex;
			if ((r=GetBool(&ex)) == B_OK) {
				val = ex ? 1 : 0;
			}
		} break;
		default:
			r = B_BAD_TYPE;
	}
	if (result) *result = r;
	return val;
}

double BValue::AsDouble(status_t* result) const DECLARE_RETURN(val)
{
	double val = 0;
	status_t r;
	switch (m_type) {
		case B_STRING_TYPE: {
			const char* ex;
			if ((r=GetString(&ex)) == B_OK) {
				val = strtod(ex, const_cast<char**>(&ex));
			}
		} break;
		case B_DOUBLE_TYPE: {
			r = GetDouble(&val);
		} break;
		case B_FLOAT_TYPE: {
			float ex;
			if ((r=GetFloat(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_INT64_TYPE: {
			int64 ex;
			if ((r=GetInt64(&ex)) == B_OK) {
				val = static_cast<double>(ex);
			}
		} break;
		case B_BIGTIME_TYPE: {
			bigtime_t ex;
			if ((r=GetTime(&ex)) == B_OK) {
				val = static_cast<double>(ex);
			}
		} break;
		case B_INT32_TYPE: {
			int32 ex;
			if ((r=GetInt32(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_INT16_TYPE: {
			int16 ex;
			if ((r=GetInt16(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_INT8_TYPE: {
			int8 ex;
			if ((r=GetInt8(&ex)) == B_OK) {
				val = ex;
			}
		} break;
		case B_BOOL_TYPE: {
			bool ex;
			if ((r=GetBool(&ex)) == B_OK) {
				val = ex ? 1 : 0;
			}
		} break;
		default:
			r = B_BAD_TYPE;
	}
	if (result) *result = r;
	return val;
}

atom_ptr<BAtom> BValue::AsAtom(status_t* result) const DECLARE_RETURN(val)
{
	atom_ptr<BAtom> val;
	status_t r = GetAtom(&val);
	if (r < B_OK) {
		atom_ref<BAtom> ex;
		if ((r=GetAtomRef(&ex)) == B_OK) {
			val = ex.promote();
		}
	}
	if (result) *result = r;
	return val;
}

atom_ref<BAtom> BValue::AsAtomRef(status_t* result) const DECLARE_RETURN(val)
{
	atom_ref<BAtom> val;
	const status_t r = GetAtomRef(&val);
	if (result) *result = r;
	return val;
}

type_code BValue::Type() const
{
	return m_type;
}

const void* BValue::Data() const
{
	if (m_length >= 0) {
		return (m_length <= static_cast<ssize_t>(sizeof(m_data)))
				? (static_cast<const void*>(m_data.local))
				: (static_cast<const void*>(m_data.heap.flat->data()));
	}
	return NULL;
}

size_t BValue::Length() const
{
	return m_length >= 0 ? m_length : 0;
}

void* BValue::BeginEditBytes(type_code type, size_t length, uint32 flags)
{
	void* data;
	if (type != B_VALUE_MAP_TYPE) {
		if (!(flags&B_EDIT_VALUE_DATA)) {
			if (m_length >= 0 || m_length == OBJECT_MAGIC_LENGTH) {
				data = alloc_data(length);
			} else {
				data = NULL;
			}
		} else {
			data = edit_data(length);
		}
	} else {
		Undefine();
		data = NULL;
	}
	if (data) m_type = type;
	return data;
}

status_t BValue::EndEditBytes(ssize_t final_length)
{
	if (m_length >= 0) {
		if (final_length < 0) return B_OK;
		if (final_length <= m_length) {
			m_length = final_length;
			return B_OK;
		}
		return B_BAD_VALUE;
	}
	return m_length;
}

void BValue::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BValue(";
	
	if (Status() >= B_OK) {
		const size_t N = CountItems();
		if (N <= 1 && m_length >= 0) {
			value_ref(*this).print_to_stream(io);
		} else {
			BValue key, value;
			if (N > 1) {
				io << "{" << endl;
				io->BumpIndentLevel(1);
			}
			for (size_t i=0; i<N; i++) {
				find_item_index(i, &key, &value);
				if (!key.IsNull()) {
					key.PrintToStream(io);
					io << " -> ";
				}
				value.PrintToStream(io);
				if (N > 1) {
					if (i < (N-1)) io << "," << endl;
					else io << endl;
				}
			}
			if (N > 1) {
				io->BumpIndentLevel(-1);
				io << "}";
			}
		}
	} else {
		io << strerror(Status());
	}
	
	if (flags&B_PRINT_STREAM_HEADER) io << ")";
}

ITextOutput::arg operator<<(ITextOutput::arg io, const BValue& value)
{
	value.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

void BMoveBefore(BValue* to, BValue* from, size_t count)
{
	memcpy(to, from, sizeof(BValue)*count);
}

void BMoveAfter(BValue* to, BValue* from, size_t count)
{
	memmove(to, from, sizeof(BValue)*count);
}

void* BValue::edit_data(size_t len)
{
	if (m_length < B_OK) {
		return NULL;
	
	} else if (m_length <= static_cast<ssize_t>(sizeof(m_data))) {
		if (len <= sizeof(m_data)) {
			m_length = len;
			return m_data.local;
		}
		shared_buffer* buf = shared_buffer::create(len);
		if (buf) {
			memcpy(buf->data(), m_data.local, m_length);
			m_length = len;
			m_data.heap.flat = buf;
			return buf->data();
		}
		
		set_error(B_NO_MEMORY);
		return NULL;
	}
	
	shared_buffer* buf = m_data.heap.flat->edit(len);
	if (buf) {
		m_length = len;
		m_data.heap.flat = buf;
		return buf->data();
	}
	
	set_error(B_NO_MEMORY);
	return NULL;
}

status_t BValue::copy_data(type_code type, void* buf, size_t len) const
{
	if (m_length < B_OK) return m_length;
	if (m_type != type) return B_BAD_TYPE;
	if (m_length != static_cast<ssize_t>(len)) return B_MISMATCHED_VALUES;
	memcpy(buf, Data(), len);
	return B_OK;
}

BValueMap* BValue::edit_map()
	DECLARE_RETURN(map)
{
	BValueMap* map;
	if (is_map()) {
		if ((map=const_cast<BValueMap*>(m_data.heap.map))->IsShared()) {
			if ((map=new(std::nothrow) BValueMap(*map)) != NULL) {
				m_data.heap.map->DecUsers();
				m_data.heap.map = map;
			} else {
				set_error(B_NO_MEMORY);
			}
		}
	} else if (m_length >= 0) {
		map = new(std::nothrow) BValueMap;
		if (map) {
			ssize_t err = B_OK;
			if (IsSpecified()) {
				err = map->SetFirstMap(BValue::null, *this);
			}
			if (m_length != 0) free_data();
			if (err >= B_OK) {
				m_type = B_VALUE_MAP_TYPE;
				m_length = OBJECT_MAGIC_LENGTH;
				m_data.heap.flat = NULL;
				m_data.heap.map = map;
			} else {
				m_length = err;
				map->DecUsers();
				map = NULL;
			}
		} else {
			set_error(B_NO_MEMORY);
			map = NULL;
		}
	}
	return map;
}

status_t BValue::set_error(ssize_t code)
{
	free_data();
	m_type = 0;
	m_length = (code >= B_OK || code == OBJECT_MAGIC_LENGTH) ? B_ERROR : code;
	m_data.heap.flat = NULL;
	m_data.heap.map = NULL;
	CHECK_INTEGRITY(*this);
	return m_length;
}

void BValue::acquire_objects()
{
	switch (m_type) {
		case B_ATOM_TYPE: {
			BAtom* a = *reinterpret_cast<BAtom**>(m_data.local);
			if (a) a->Acquire(this);
		} break;
		case B_ATOMREF_TYPE: {
			BAtom* a = *reinterpret_cast<BAtom**>(m_data.local);
			if (a) a->IncRefs(this);
		} break;
		case B_BINDER_TYPE: {
			BBinder* b = *reinterpret_cast<BBinder**>(m_data.local);
			if (b) b->Acquire(this);
		} break;
		case B_BINDER_HANDLE_TYPE: {
			int32 handle = *reinterpret_cast<int32*>(m_data.local);
			IBinder::ptr b = BLooper::GetProxyForHandle(handle);
			if (b != NULL) b->Acquire(this);
		} break;
	}
}

void BValue::release_objects()
{
	switch (m_type) {
		case B_ATOM_TYPE: {
			BAtom* a = *reinterpret_cast<BAtom**>(m_data.local);
			if (a) a->Release(this);
		} break;
		case B_ATOMREF_TYPE: {
			BAtom* a = *reinterpret_cast<BAtom**>(m_data.local);
			if (a) a->DecRefs(this);
		} break;
		case B_BINDER_TYPE: {
			BBinder* b = *reinterpret_cast<BBinder**>(m_data.local);
			if (b) b->Release(this);
		} break;
		case B_BINDER_HANDLE_TYPE: {
			int32 handle = *reinterpret_cast<int32*>(m_data.local);
			IBinder::ptr b = BLooper::GetProxyForHandle(handle);
			if (b != NULL) b->Release(this);
		} break;
	}
}

bool BValue::check_integrity() const
{
	bool passed = true;
	
	if (m_length >= 0) {
		if (m_type == B_VALUE_MAP_TYPE) {
			debugger("BValue: m_length is >= 0, and m_type is B_VALUE_MAP_TYPE");
			passed = false;
		}
		if (m_length > static_cast<ssize_t>(sizeof(m_data)) && m_data.heap.flat == NULL) {
			debugger("BValue: m_length is >= sizeof(m_data), but m_data.heap.flat is NULL");
			passed = false;
		}
	} else if (m_length == OBJECT_MAGIC_LENGTH) {
		if (m_type != B_VALUE_MAP_TYPE) {
			debugger("BValue: m_length is OBJECT_MAGIC_LENGTH, but m_type not B_VALUE_MAP_TYPE");
			passed = false;
		}
		if (m_data.heap.map == NULL) {
			debugger("BValue: m_length is OBJECT_MAGIC_LENGTH, but m_data.heap.map is NULL");
			passed = false;
		} else {
			if (m_data.heap.map->CountMaps() <= 0) {
				debugger("BValue: contains a BValueMap of no items");
				passed = false;
			} else if (m_data.heap.map->CountMaps() == 1) {
				const BValueMap::pair& p = m_data.heap.map->MapAt(0);
				if (p.key == null) {
					debugger("BValue: contains a redundant BValueMap");
					passed = false;
				}
			}
		}
	} else {
		if (m_type != 0) {
			debugger("BValue: error occurred but m_type != 0");
			passed = false;
		}
	}
	
	return passed;
}

} }	// namespace B::Support2
