#include "Object2.h"
#include "Atomizer.h"
#include <string.h>
#include "pdf_doc.h"
#include <math.h>
#include <limits.h>
#include "PDFKeywords.h"
#include <OS.h>
#include <alloc.h>
#include <DataIO.h>

using namespace BPrivate;

#if 1
#include <hash_map>
#if 0
struct str_equal_to {
	bool operator()(const char *a, const char *b) {	return (::strcmp(a, b) == 0); };
};

typedef hash_map<const char *, PDFObject *, hash<const char *>, str_equal_to> object_dictionary_base;
#else
typedef hash_map<const char *, PDFObject *> object_dictionary_base;
#endif
#else
#include <map>
typedef map<const char *, PDFObject *> object_dictionary_base;
#endif

class object_dictionary : public object_dictionary_base {
public:
	~object_dictionary();
};

object_dictionary::~object_dictionary()
{
	// walk the map, releasing any owned objects
	iterator i = begin();
	iterator e = end();
	while (i != e) (*i++).second->Release();
}

void 
object_array::Release()
{
	// walk the array, releasing any owned objects
	iterator i = begin();
	iterator e = end();
	while (i != e) (*i++)->Release();
}


object_array::~object_array()
{
	Release();
}


PDFOpaque::~PDFOpaque()
{
}


PDFOpaquePointer::PDFOpaquePointer(void *p)
	: fPointer(p)
{
}


PDFOpaquePointer::~PDFOpaquePointer()
{
}

OpaqueBPositionIO::OpaqueBPositionIO(BPositionIO *io)
	: PDFOpaque(), m_io(io)
{
}


OpaqueBPositionIO::~OpaqueBPositionIO()
{
	delete m_io;
}

#define MALLOC_STATS 1
#if MALLOC_STATS
size_t 	c_total_alloced = 0;
size_t 	c_curent_alloced = 0;
size_t 	c_max_alloced = 0;
size_t	c_max_size_alloced = 0;
size_t	c_total_allocations = 0;
size_t	c_outstanding_allocations = 0;
size_t	c_max_outstanding_allocs = 0;
#endif

#if !USE_REAL_NEW_DELETE
const size_t kPageSize = 4096;

struct mem_list {
	mem_list	*next;
};

//static
mem_list *s_free_list = 0;
//static
mem_list *s_page_list = 0;

struct init_mem_list {
		init_mem_list();
		~init_mem_list();
void	Lock() { if ((atomic_add(&ben, 1)) >= 1) acquire_sem(sem); };
void	Unlock() { if ((atomic_add(&ben, -1)) > 1) release_sem(sem); };
private:
int32	ben;
sem_id	sem;
};

/*
	NOTE: the ordering of the statics and static member variables matters!
*/
PDFObject PDFObject::__NILL__(NILL);
PDFObject PDFObject::__TRUE__(true);
PDFObject PDFObject::__FALSE__(false);
PDFObject PDFObject::__ZERO__((double)0.0);

static BAtomizer atomizer;
static init_mem_list iml;
static object_dictionary name_dict;
static object_dictionary keyword_dict;


init_mem_list::init_mem_list()
{
	ben = 0;
	sem = create_sem(0, "pdf_object_mem");
}

init_mem_list::~init_mem_list()
{
#ifndef NDEBUG
	// walk the free list, marking those entries invalid
	while (s_free_list)
	{
		mem_list *nuke = s_free_list;
		s_free_list = s_free_list->next;
		memset(nuke, 0, sizeof(PDFObject));
	}
	// now, walk all the pages, searching for in-use items
	s_free_list = s_page_list;
	while (s_free_list)
	{
		mem_list *item = s_free_list + 1;
		printf("Items on page %p\n", s_free_list);
		for (size_t i = (kPageSize - sizeof(void *)) / sizeof (PDFObject); i > 1; i--)
		{
			PDFObject *o = (PDFObject *)item;
			item += 3;
			if (!o->IsInvalid())
			{
				printf("%p -> ", o); o->PrintToStream(0); printf("\n");
			}
		}
		s_free_list = s_free_list->next;
	}
#endif
	// throw away our pages
	while (s_page_list)
	{
		s_free_list = s_page_list->next;
		free(s_page_list);
		s_page_list = s_free_list;
	}
	s_page_list = 0;
	// delete the semaphore
	delete_sem(sem);
	ben = 0;
	sem = 0;
}

static void *
new_object(void)
{
	iml.Lock();
	// no space in the list?
	if (!s_free_list)
	{
		// make a new page
		mem_list *new_page = (mem_list *)malloc(kPageSize);
		if (!new_page)
		{
			iml.Unlock();
			return 0;
		}
		// link the new page into the list
		new_page->next = s_page_list;
		s_page_list = new_page;
		// init the free list
		s_free_list = s_page_list + 1;
		s_free_list->next = 0;
		mem_list *last = s_free_list;
		// prepare the free list on this page
		for (size_t i = (kPageSize - sizeof(void *)) / sizeof (PDFObject); i > 1; i--)
		{
			// next object
			s_free_list = s_free_list + 3;
			// link into the list
			s_free_list->next = last;
			// remember the last one
			last = s_free_list;
		}
	}
	// now there's space
	void *p = s_free_list;
	s_free_list = s_free_list->next;
	iml.Unlock();
#if 1
	memset(p, 0, sizeof(PDFObject));
#endif
	return p;
}

static void
delete_object(void *p)
{
	iml.Lock();
#ifndef NDEBUG
	// FIXME: turn off when not debugging
	// walk the free list, and see if it's already there
	{
		mem_list *fl = s_free_list;
		while (fl && fl != p)
			fl = fl->next;
		if (fl == p)
		{
			debugger("double free!");
			return;
		}
	}
#endif
	// put this object at the head of the free list
	mem_list *prev = (mem_list *)p;
	prev->next = s_free_list;
	s_free_list = prev;
	iml.Unlock();
}
#endif

PDFObject::PDFObject(pdf_class c)
{
	memset(this, 0, sizeof(PDFObject));
	_t._tag = c;
}

PDFObject::PDFObject(bool b)
	: _ref_count(0)
{
	_t._tag = BOOLEAN;
	_bool = b;
}


PDFObject::PDFObject(double d)
	: _ref_count(0)
{
	if (d == floor(d))
	{
		// integer
		if (fabs(d) > ((double)(1 << (FLT_MANT_DIG+1))-1))
		{
			_t._tag = NUMBER48;
			int64 n = (int64)d;
			_int = n & 0xffffffff;
			_size = (n >> 32) & 0xffff;
		}
		else
		{
			_t._tag = NUMBER;
			_int = (int32)d;
		}
	}
	else
	{
		_t._tag = REAL;
		_float = (float)d;
	}
}

PDFObject::PDFObject(uint32 obj, uint16 gen, PDFDocument *doc)
	: _ref_count(0), _generation(gen)
{
	_t._tag = REFERENCE;
	SetXrefEntry(obj);
	_doc = doc;
#if 0
	if (obj == 18625) DEBUGGER("18625 0 R created");
#endif
}

PDFObject::PDFObject(size_t len, const void *data)
	: _ref_count(0), _size((uint16)len)
{
	_uint8 = new uint8[len];
	if (_uint8) memcpy(_uint8, data, len);
	else _size = 0;
	_t._tag = STRING;
}


PDFObject::PDFObject(const char *atom)
	: _ref_count(0)
{
	_t._tag = NAME;
	_name = atom;
}

PDFObject::PDFObject(const PDFObject &)
{
	// no initialization allowed!
	ASSERT(0 == 1);
}


PDFObject::~PDFObject()
{
	// usefull to catch when one deletes a particular object
	//if (GetXrefEntry() == 283) debugger("deleting object 283");
	if (IsArray() || IsDictionary())
	{
		delete _array;
	}
	else if (IsString())
	{
		delete [] _uint8;
	}
	else if (IsOpaque())
	{
		delete _opaque;
	}
}

PDFObject &
PDFObject::operator=(const PDFObject &)
{
	// no assignment allowed!
	ASSERT(0 == 1);
	return *this;
}

void *
PDFObject::operator new(size_t size)
{
	ASSERT(size == sizeof(PDFObject));
#if MALLOC_STATS
	c_total_allocations++;
	c_outstanding_allocations++;
	if (c_outstanding_allocations > c_max_outstanding_allocs)
		c_max_outstanding_allocs = c_outstanding_allocations;
#if 0
	if (c_total_allocations == 8497) debugger("allocation 8497");
	if (c_total_allocations == 8519) debugger("allocation 8519");
#endif
	c_total_alloced += size;
	c_curent_alloced += size;
	if (c_curent_alloced > c_max_alloced)
		c_max_alloced = c_curent_alloced;
	if (size > c_max_size_alloced)
		c_max_size_alloced = size;
#endif
#if USE_REAL_NEW_DELETE
	void *p = ::new char[size];
#else
	void *p = new_object();
#endif
	//printf("%p %.08lx ::a\n", p, c_total_allocations);
	return p;
}

void 
PDFObject::operator delete(void *p, size_t size)
{
	ASSERT(size == sizeof(PDFObject));

	if (!p) return;
	if (p == &__NILL__) return;
	if (p == &__TRUE__) return;
	if (p == &__FALSE__) return;
	if (p == &__ZERO__) return;
	
#if MALLOC_STATS
	c_curent_alloced -= size;
	c_outstanding_allocations--;
#endif
	//printf("%p %.08lx ::b\n", p, c_total_allocations);
#if 0
	{
		PDFObject *o = (PDFObject *)p;
		uint32 entry = o->GetXrefEntry();
		if ((o->_t._tag != REFERENCE) && entry)
		{
			printf("%lu %lu deleted\n", entry, (uint32)(o->_generation));
			debugger("Who deleted me?");
			o->_entry = 0;
		}
	}
#endif

#if USE_REAL_NEW_DELETE
	::delete [] p;
#else
	delete_object(p);
#endif
}

void *
PDFObject::operator new[] (size_t)
{
	debugger("PDFObject::operator new[] called!\n");
	return 0;
}

void 
PDFObject::operator delete[] (void *, size_t)
{
	debugger("PDFObject::operator delete[] called!\n");
}

void 
PDFObject::Acquire()
{
	// avoid nulls
	if (!this) return;
	//if (GetXrefEntry() == 38) debugger("acquire'd entry 38");
	++_ref_count;
}

void 
PDFObject::Release()
{
	// avoid nulls
	if (!this) return;
	// more refs? bail now
	if (--_ref_count > 0) return;
	// over-release?
	ASSERT(_ref_count == 0);
	// no longer needed
	delete this;
}

PDFObject *
PDFObject::makeNULL(void)
{
	__NILL__.Acquire();
	return &__NILL__;
}

PDFObject *
PDFObject::makeArray(void)
{
	PDFObject *array = new PDFObject(ARRAY);
	array->Acquire();
	array->_array = new object_array();
	return array;
}

PDFObject *
PDFObject::makeArray(object_array::iterator first, object_array::iterator last)
{
	PDFObject *array = new PDFObject(ARRAY);
	array->Acquire();
	object_array *_array = array->_array = new object_array(last - first);
	uint32 i = 0;
	while (first != last)
		(*_array)[i++] = *first++;
	return array;
}


PDFObject *
PDFObject::makeBoolean(bool t)
{
	PDFObject *a_bool = t ? &__TRUE__ : &__FALSE__;
	a_bool->Acquire();
	return a_bool;
}

PDFObject *
PDFObject::makeDictionary()
{
	PDFObject *dict = makeArray();
	dict->_t._tag = DICTIONARY;
	return dict;
}

#if MALLOC_STATS
uint32 g_made_keywords = 0;
#endif

PDFObject *
PDFObject::makeKeyword(const char *k)
{
	const char *atom = atomizer.Atom(k);
	PDFObject *a_keyword = 0;

	// find n in name_dict;
	object_dictionary::iterator i = keyword_dict.find(atom);
	if (i == keyword_dict.end())
	{
#if MALLOC_STATS
		g_made_keywords++;
#endif
		// make the new name
		a_keyword = new PDFObject(atom);
		a_keyword->_t._tag = KEYWORD;
		a_keyword->_size = a_keyword->keyword_number();
		// up refcount for map
		a_keyword->Acquire();
		// add to map
		object_dictionary::value_type vt(atom, a_keyword);
		keyword_dict.insert(vt);
	}
	else a_keyword = (*i).second;
	// up the reference count
	a_keyword->Acquire();
	return a_keyword;
}

#if MALLOC_STATS
uint32 g_made_names = 0;
#endif

PDFObject *
PDFObject::makeName(const char *n)
{
	const char *atom = atomizer.Atom(n);
	PDFObject *a_name = 0;
	// find n in name_dict;
	object_dictionary::iterator i = name_dict.find(atom);
	if (i == name_dict.end())
	{
#if MALLOC_STATS
		g_made_names++;
#endif
		// make the new name
		a_name = new PDFObject(atom);
		// up refcount for map
		a_name->Acquire();
		// add to map
		object_dictionary::value_type vt(atom, a_name);
		name_dict.insert(vt);
	}
	else a_name = (*i).second;
	// up the reference count
	a_name->Acquire();
	return a_name;
}

PDFObject *
PDFObject::makeNumber(double n)
{
	PDFObject *a_number = &__ZERO__;
	if (n != 0.0) a_number = new PDFObject(n);
	a_number->Acquire();
	return a_number;
}

PDFObject *
PDFObject::makeNumber(const char *s)
{
	return makeNumber(strtod(s, 0));
}

PDFObject *
PDFObject::makeOpaque(PDFOpaque *o)
{
	PDFObject *an_opaque = new PDFObject(OPAQUE);
	an_opaque->_opaque = o;
	an_opaque->Acquire();
	return an_opaque;
}


PDFObject *
PDFObject::makeReference(uint32 obj, uint16 gen, PDFDocument *doc)
{
	PDFObject *a_reference = new PDFObject(obj, gen, doc);
	a_reference->Acquire();
	return a_reference;
}

PDFObject *
PDFObject::makeString(size_t len, const void *data)
{
	PDFObject *a_string = new PDFObject(len, data);
	a_string->Acquire();
	return a_string;
}

void 
PDFObject::Assign(PDFObject *key, PDFObject *value)
{
	ASSERT(_t._tag == DICTIONARY);
	ASSERT(key->_t._tag == NAME);
	ASSERT(value);

	object_array::iterator i;

	for (i = _array->begin(); i != _array->end(); i += 2)
	{
		ASSERT((*i)->_t._tag == NAME);
		// we can compare this way because names are unique
		if ((*i) == key) break;
	}
	// not found?
	if (i == _array->end())
	{
		// make new entry
		_array->push_back(key);
		_array->push_back(value);
	}
	else
	{
		// advance to value item
		i++;
		// don't need the value item any longer
		(*i)->Release();
		// store the new value
		*i = value;
		// drop the extra reference to name
		key->Release();
	}
}

void 
PDFObject::Erase(const char *key)
{
	ASSERT(_t._tag == DICTIONARY);
	ASSERT(key == Atom(key));

	object_array::iterator i;

	for (i = _array->begin(); i != _array->end(); i += 2)
	{
		ASSERT((*i)->_t._tag == NAME);
		// we can compare this way because names are unique
		if ((*i)->GetCharPtr() == key)
		{
			// dispose of key
			(*i)->Release();
			// dispose of value
			(*(i+1))->Release();
			// remove from data structure
			_array->erase(i, i+2);
			// all done
			break;
		}
	}
}


void 
PDFObject::Erase(PDFObject *key)
{
	ASSERT(key->_t._tag == NAME);
	Erase(key->GetCharPtr());
}

PDFObject *
PDFObject::Find(PDFObject *key)
{
	ASSERT(key->_t._tag == NAME);
	return Find(key->GetCharPtr());
}

PDFObject *
PDFObject::Find(const char *key)
{
	if (!this) return 0;
	ASSERT(_t._tag == DICTIONARY);
	// the following prevents code like: Find("somename")
	ASSERT(key == Atom(key));
	PDFObject *value = 0;

	object_array::iterator i;

	for (i = _array->begin(); i != _array->end(); i += 2)
	{
		ASSERT((*i)->_t._tag == NAME);
		// we can compare this way because names are unique
		if ((*i)->GetCharPtr() == key)
		{
			// advance to value
			i++;
			value = *i;
			break;
		}
	}
	// give up the goods
	return value;
}

void 
PDFObject::Merge(PDFObject *source)
{
	ASSERT(_t._tag == DICTIONARY);
	ASSERT(source->_t._tag == DICTIONARY);

	object_array::iterator i;
	object_array::iterator j;

	// for each key/value pair in source
	for (i = source->_array->begin(); i < source->_array->end(); i++)
	{
		// find this key in the target
		for (j = _array->begin(); j < _array->end(); j += 2)
			if ((*j) == (*i)) break;
		// if this key is NOT already in the target
		if (j == _array->end())
		{
			// add the key
			_array->push_back(*i);
			(*i++)->Acquire();
			
			// add the value
			_array->push_back(*i);
			(*i)->Acquire();
		}
		// skip value
		else i++;
	}
}

PDFObject *
PDFObject::Resolve()
{
	if (!this) return 0;
#if 0
	if (GetXrefEntry() == 18625) DEBUGGER("18625 0 R resolved");
#endif
	if (_t._tag == REFERENCE) return _doc->GetObject(GetXrefEntry(), _generation);
	Acquire();
	return this;
}

bool
PDFObject::ResolveArrayOrDictionary(void)
{
	bool result = false;
	//ASSERT((_t._tag == DICTIONARY) || (_t._tag == ARRAY));
	if (!IsArray() && !IsDictionary()) return result;

	object_array::iterator i;

	for (i = _array->begin(); i != _array->end(); i++)
	{
		if ((*i)->IsReference())
		{
			PDFObject *old = *i;
			*i = old->Resolve();
			old->Release();
			result = true;
		}
	}
	return result;
}

#if 1
#include "PushContents.h"
#include "DataIOSink.h"

BPositionIO *
PDFObject::RealizeStream(void)
{
	ASSERT(_t._tag == DICTIONARY);
	BPositionIO *mio = 0;
	// have we resolved this stream yet?
	PDFObject *sd = Find(PDFAtom.__stream_data__);
	if (!sd)
	{
		// build a filter
		PDFObject *doc_obj = Find(PDFAtom.__document__);
		if (doc_obj)
		{
			mio = new BMallocIO();
			DataIOSink *dsio = new DataIOSink(mio);
			if (PushStream(this, dsio) == B_OK)
				Assign(PDFObject::makeName(PDFAtom.__stream_data__), PDFObject::makeOpaque(new OpaqueBPositionIO(mio)));
			else
			{
				delete mio;
				mio = 0;
			}
		}
	}
	else if (sd->IsOpaque())
	{
		// extract the BPositionIO from the stream dictionary
		mio = (BPositionIO *)(((OpaqueBPositionIO *)sd->GetOpaquePtr())->m_io);
	}
	return mio;
}
#endif



#ifndef NDEBUG
void 
PDFObject::PrintToStream(int depth, FILE *fd)
{
	fprintf(fd, "%*s", depth, "");
	if (!this)
	{
		printf("NULL");
		return;
	}
	switch (_t._tag)
	{
		case ARRAY:
		{
			fprintf(fd, "[\n");
			for (object_array::iterator i = _array->begin(); i != _array->end(); i++)
			{
				(*i)->PrintToStream(depth + 1, fd);
				fprintf(fd, "\n");
			}
			fprintf(fd, "%*s]", depth, "");
		} break;
		case BOOLEAN:
		{
			fprintf(fd, "%s", _bool ? "true" : "false");
		} break;
		case DICTIONARY:
		{
			fprintf(fd, "{\n");
			for (object_array::iterator i = _array->begin(); i != _array->end(); i++)
			{
				// print the key
				(*i)->PrintToStream(depth, fd);
				fprintf(fd, " ->");
				i++;
				if ((*i)->IsDictionary() || (*i)->IsArray()) {
					fprintf(fd, "\n");
					(*i)->PrintToStream(depth + 1, fd);
				} else (*i)->PrintToStream(1, fd);
				fprintf(fd, "\n");
			}
			fprintf(fd, "%*s}", depth, "");
		} break;
		case KEYWORD:
		{
			fprintf(fd, "keyword: %s", _name);
		} break;
		case NAME:
		{
			fprintf(fd, "/%s", _name);
		} break;
		case NUMBER:
		{
			fprintf(fd, "%ld", _int);
		} break;
		case NUMBER48:
		{
			fprintf(fd, "%Ld", this->GetInt64());
		} break;
		case OPAQUE:
		{
			fprintf(fd, "(opaque data at %p)", _opaque);
		} break;
		case REAL:
		{
			fprintf(fd, "%f", _float);
		} break;
		case REFERENCE:
		{
			fprintf(fd, "%ld %d R", GetXrefEntry(), _generation);
		} break;
		case STRING:
		{
			fprintf(fd, "(length %d) ->%.*s<-", _size, (int)(_size > 20 ? 20 : _size), (const char *)_uint8);
		} break;
		case NILL:
		{
			fprintf(fd, "null");
		} break;
		default:
			fprintf(fd, "UNKNOWN _tag: %d for object %p", _t._tag, this);
			break;
	}
	fprintf(fd, " (%d)", _ref_count);
}
#endif

PDFObject *
PDFObject::AsArray(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsArray()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsBool(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsBool()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsDictionary(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsDictionary()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsKeyword(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsKeyword()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsName(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsName()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsNull(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsNull()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsNumber(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsNumber()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsNumber48(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsNumber48()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsOpaque(void)
{
	if (IsOpaque())
	{
		Acquire();
		return this;
	}
	return 0;
}

PDFObject *
PDFObject::AsReal(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsReal()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsReference(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsReference()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

PDFObject *
PDFObject::AsString(void)
{
	PDFObject *obj = Resolve();
	if (!(obj->IsString()))
	{
		obj->Release();
		obj = 0;
	}
	return obj;
}

static int64
StringAsInteger(uint8 *bytes, uint32 size)
{
	int64 num = 0;
	while (size--)
	{
		num <<= 8;
		num += (int64)*bytes++;
	}
	return num;
}

int32
PDFObject::GetInt32 () const
{
	int32 num = 0;
	// if it's a keyword, return the keyword number for it
	// if it's some kind of number, return it as an int32
	if (this)
		switch (_t._tag)
		{
			case KEYWORD:
				num = _size;
				break;
			case NUMBER:
				num = _int;
				break;
			case NUMBER48:
				{
				int64 i64 = (0xffffffffLL & (uint64)_int) | (((int64)_size << 32) & 0xffffffff00000000LL);
				num = (int32)i64;
				}
				break;
			case REAL:
				num = (int32)_float;
				break;
			case STRING:
				num = (int32)StringAsInteger(_uint8, _size);
				break;
			default:
				num = 0;
				break;
		}
	return num;
}

int64
PDFObject::GetInt64 () const
{
	int64 num = 0;
	// if it's some kind of number, return it as an int64
	if (this)
		switch (_t._tag)
		{
			case KEYWORD:
				num = (int64)_size;
				break;
			case NUMBER:
				num = (int64)_int;
				break;
			case NUMBER48:
				num = (0xffffffffLL & (uint64)_int) | (((int64)_size << 32) & 0xffffffff00000000LL);
				break;
			case REAL:
				num = (int64)_float;
				break;
			case STRING:
				num = (int64)StringAsInteger(_uint8, _size);
				break;
			default:
				num = 0;
				break;
		}
	return num;
}

float
PDFObject::GetFloat () const
{
	float num = 0.0;
	// if it's some kind of number, return it as an int64
	if (this)
		switch (_t._tag)
		{
			case KEYWORD:
				num = (float)_size;
				break;
			case NUMBER:
				num = (float)_int;
				break;
			case NUMBER48:
				{
				int64 i64 = (0xffffffffLL & (uint64)_int) | (((int64)_size << 32) & 0xffffffff00000000LL);
				num = (float)i64;
				}
				break;
			case REAL:
				num = _float;
				break;
			case STRING:
				num = (float)StringAsInteger(_uint8, _size);
				break;
			default:
				num = 0.0;
				break;
		}
	return num;
}

struct keyword_mapping {
	const char *text;
	int32		key;
};

// these entries must be sorted according to strcmp(a->text, b->text)
static
keyword_mapping keywords[] = {
	{"\"",	PDF_ticktick},
	{"'",	PDF_tick},
	{"<<",	PDF_startdict},
	{">>",	PDF_enddict},

	{"B",	PDF_B},
	{"B*",	PDF_Bstar},
	{"BDC",	PDF_BDC},
	{"BI",	PDF_BI},
	{"BMC",	PDF_BMC},
	{"BT",	PDF_BT},
	{"BX",	PDF_BX},
	{"CS",	PDF_CS},
	{"DP",	PDF_DP},
	{"Do",	PDF_Do},
	{"EI",	PDF_EI},
	{"EMC",	PDF_EMC},
	{"ET",	PDF_ET},
	{"EX",	PDF_EX},
	{"F",	PDF_F},
	{"G",	PDF_G},
	{"ID",	PDF_ID},
	{"J",	PDF_J},
	{"K",	PDF_K},
	{"M",	PDF_M},
	{"MP",	PDF_MP},
	{"Q",	PDF_Q},
	{"R",	PDF_R},
	{"RG",	PDF_RG},
	{"S",	PDF_S},
	{"SC",	PDF_SC},
	{"SCN",	PDF_SCN},
	{"T*",	PDF_Tstar},
	{"TD",	PDF_TD},
	{"TJ",	PDF_TJ},
	{"TL",	PDF_TL},
	{"Tc",	PDF_Tc},
	{"Td",	PDF_Td},
	{"Tf",	PDF_Tf},
	{"Tj",	PDF_Tj},
	{"Tm",	PDF_Tm},
	{"Tr",	PDF_Tr},
	{"Ts",	PDF_Ts},
	{"Tw",	PDF_Tw},
	{"Tz",	PDF_Tz},
	{"W",	PDF_W},
	{"W*",	PDF_Wstar},

	{"[",	PDF_startarray},
	{"]",	PDF_endarray},

	{"b",	PDF_b},
	{"b*",	PDF_bstar},
	{"c",	PDF_c},
	{"cm",	PDF_cm},
	{"cs",	PDF_cs},
	{"d",	PDF_d},
	{"d0",	PDF_d0},
	{"d1",	PDF_d1},
	{"endobj", PDF_endobj},
	{"endstream", PDF_endstream},
	{"f",	PDF_f},
	{"f*",	PDF_fstar},
	{"g",	PDF_g},
	{"gs",	PDF_gs},
	{"h",	PDF_h},
	{"i",	PDF_i},
	{"j",	PDF_j},
	{"k",	PDF_k},
	{"l",	PDF_l},
	{"m",	PDF_m},
	{"n",	PDF_n},
	{"obj",	PDF_obj},
	{"q",	PDF_q},
	{"re",	PDF_re},
	{"rg",	PDF_rg},
	{"ri",	PDF_ri},
	{"s",	PDF_s},
	{"sc",	PDF_sc},
	{"scn",	PDF_scn},
	{"sh",	PDF_sh},
	{"startxref", PDF_startxref},
	{"stream", PDF_stream},
	{"trailer", PDF_trailer},
	{"v",	PDF_v},
	{"w",	PDF_w},
	{"xref", PDF_xref},
	{"y",	PDF_y},
};
#define KEYWORDS (sizeof(keywords) / sizeof(keywords[0]))

static int compare_keywords(const void *a, const void *b)
{
	return strcmp(((keyword_mapping *)a)->text, ((keyword_mapping *)b)->text);
}

int32 
PDFObject::keyword_number(void) const
{
	keyword_mapping kw;
	kw.text = _name;
	kw.key = PDF_UNKNOWN_KEY;
	//printf("pdf_make_keyword(%ld, %s)\n", length, token);
	keyword_mapping *k = (keyword_mapping *)bsearch(&kw, keywords, KEYWORDS, sizeof(keyword_mapping), compare_keywords);
	if (!k) k = &kw;
	return k->key;
}

const char *
PDFObject::Atom(const char *name)
{
	return atomizer.Atom(name);
}

const char *
PDFObject::StaticAtom(const char *name)
{
	return atomizer.StaticAtom(name);
}


PDFAtoms::PDFAtoms()
{
#define intern(x)	x = PDFObject::StaticAtom(#x)
	intern(A);
	intern(A85);
	intern(AbsoluteColorimetric);
	intern(AHx);
	intern(All);
	intern(Alternate);
	intern(Annots);
	intern(ASCIIHexDecode);
	intern(ASCII85Decode);
	intern(BaseEncoding);
	intern(BaseFont);
	intern(BitsPerComponent);
	intern(BlackIs1);
	intern(BPC);
	intern(__bogus__);
	intern(CCF);
	intern(CCITTFaxDecode);
	intern(CalCMYK);
	intern(CalGray);
	intern(CalRGB);
	intern(Catalog);
	intern(CMYK);
	intern(ColorSpace);
	intern(Colors);
	intern(Columns);
	intern(Contents);
	intern(Count);
	intern(Courier);
	Courier_Bold = PDFObject::StaticAtom("Courier-Bold");
	Courier_BoldOblique = PDFObject::StaticAtom("Courier-BoldOblique");
	Courier_Oblique = PDFObject::StaticAtom("Courier-Oblique");
	intern(CropBox);
	intern(CS);
	intern(D);
	intern(DamagedRowsBeforeError);
	intern(DCT);
	intern(DCTDecode);
	intern(Decode);
	intern(DecodeParms);
	intern(DefaultGray);
	intern(DefaultRGB);
	intern(Dest);
	intern(Dests);
	intern(DeviceCMYK);
	intern(DeviceGray);
	intern(DeviceN);
	intern(DeviceRGB);
	intern(Differences);
	intern(__document__);
	intern(DP);
	intern(EarlyChange);
	intern(EncodedByteAlign);
	intern(Encoding);
	intern(Encrypt);
	intern(EndOfBlock);
	intern(EndOfLine);
	intern(ExtGState);
	intern(F);
	intern(Filter);
	intern(First);
	intern(FirstChar);
	intern(Fl);
	intern(Flags);
	intern(FlateDecode);
	intern(Font);
	intern(FontBBox);
	intern(FontDescriptor);
	intern(__font_engine__);
	intern(FontFile);
	intern(FontFile2);
	intern(FontFile3);
	intern(G);
	intern(Gamma);
	intern(H);
	intern(Height);
	intern(Helvetica);
	Helvetica_Bold = PDFObject::StaticAtom("Helvetica-Bold");
	Helvetica_BoldOblique = PDFObject::StaticAtom("Helvetica-BoldOblique");
	Helvetica_Oblique = PDFObject::StaticAtom("Helvetica-Oblique");
	intern(I);
	intern(ICCBased);
	intern(ID);
	intern(IM);
	intern(Image);
	intern(ImageMask);
	intern(Indexed);
	intern(Info);
	intern(Interpolate);
	intern(K);
	intern(Kids);
	intern(Lab);
	intern(Last);
	intern(LastChar);
	intern(Length);
	intern(Limits);
	intern(Link);
	intern(LZW);
	intern(LZWDecode);
	intern(Mask);
	intern(MacExpertEncoding);
	intern(MacRomanEncoding);
	intern(MediaBox);
	intern(MissingWidth);
	intern(N);
	intern(Names);
	intern(Next);
	intern(None);
	intern(__stream_off_t__);
	intern(Outlines);
	intern(Page);
	intern(Pages);
	intern(Parent);
	intern(Pattern);
	intern(Perceptual);
	intern(__PixelFunctor__);
	intern(Predictor);
	intern(Prev);
	intern(Range);
	intern(Rect);
	intern(RelativeColorimetric);
	intern(Resources);
	intern(RL);
	intern(RGB);
	intern(Root);
	intern(Rows);
	intern(RunLengthDecode);
	intern(Saturation);
	intern(Separation);
	intern(Size);
	intern(space);
	intern(__stream_data__);
	intern(__stream_filter__);
	intern(__stream_key__);
	intern(Subtype);
	intern(Symbol);
	Times_Roman = PDFObject::StaticAtom("Times-Roman");
	Times_Bold = PDFObject::StaticAtom("Times-Bold");
	Times_BoldItalic = PDFObject::StaticAtom("Times-BoldItalic");
	Times_Italic = PDFObject::StaticAtom("Times-Italic");
	intern(Title);
	intern(ToUnicode);
	intern(Type);
	intern(Type0);
	intern(Type1);
	intern(Type3);
	intern(W);
	intern(WhitePoint);
	intern(WinAnsiEncoding);
	intern(Width);
	intern(Widths);
	intern(XObject);
	intern(ZapfDingbats);

#undef intern(x)
}

PDFAtoms BPrivate::PDFAtom;

#ifdef TEST_OBJECT2
int
main(int argc, char **argv)
{
	printf("sizeof(PDFObject): %lu\n", sizeof(PDFObject));
}
#endif
