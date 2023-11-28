/*	ResourceStrings.cpp	*/

#include <BeBuild.h>
#include <Debug.h>
#include <ResourceStrings.h>
#include <String.h>
#include <Resources.h>
#include <File.h>
#include <Autolock.h>
#include <Application.h>

#include <string.h>
#include <stdlib.h>
#include <new.h>


BResourceStrings::BResourceStrings() :
	_string_lock("String Table Lock")
{
	_init_error = 0;
	_string_file = 0;
	_string_hash = NULL;
	_string_hash_size = 0;
	_num_hashed_strings = 0;
}

BResourceStrings::BResourceStrings(const entry_ref & ref) :
	_string_lock("String Table Lock")
{
	_string_file = 0;
	_string_hash = NULL;
	_string_hash_size = 0;
	_num_hashed_strings = 0;
	_init_error = _set_string_file(&ref);
}

BResourceStrings::~BResourceStrings()
{
	delete _string_file;
//	_clear_strings();
}

status_t BResourceStrings::InitCheck()
{
	return _init_error;
}

BString * BResourceStrings::NewString(int32 id)
{
	if (_init_error) return NULL;
	_string_id_hash * h = _find_string_id(id);
	if (!h) {
		return NULL;
	}
	return new BString(h->data);
}

const char * BResourceStrings::FindString(int32 id)
{
	if (_init_error) return NULL;
	_string_id_hash * h = _find_string_id(id);
	if (!h) {
		return NULL;
	}
	return h->data;
}

void BResourceStrings::_clear_strings()
{
	BAutolock lock(_string_lock);
	for (int ix=0; ix<_string_hash_size; ix++) {
		for (_string_id_hash * h = _string_hash[ix]; h;) {
			_string_id_hash * d = h;
			h = h->next;
			delete d;
		}
	}
	delete[] _string_hash;
	_string_hash = NULL;
	_string_hash_size = 0;
	_num_hashed_strings = 0;
}

BResourceStrings::_string_id_hash ** BResourceStrings::_rehash_strings(BResourceStrings::_string_id_hash ** old, int old_size, int new_size)
{
	_string_id_hash **ptr = new _string_id_hash*[new_size];
	if (!ptr) {
		return NULL;
	}
	memset(ptr, 0, sizeof(*ptr)*new_size);
	/* re-hash each item */
	for (int ix=0; ix<old_size; ix++) {
		_string_id_hash * h = old[ix];
		while (h != NULL) {
			_string_id_hash * n = h;
			h = h->next;
			n->next = NULL;
			int ix = n->id % new_size;
			if (ix < 0) {
				ix += new_size;
			}
			_string_id_hash **p = &ptr[ix];
			while (*p) {
				p = &(*p)->next;
			}
			*p = n;
		}
	}
//	delete[] old;	//	double-delete (caller deletes)
	return ptr;
}

BResourceStrings::_string_id_hash * BResourceStrings::_find_string_id(int id)
{
	BAutolock lock(_string_lock);
	if (_string_hash_size) {
		int ix = id % _string_hash_size;
		if (ix < 0) {
			ix = ix + _string_hash_size;
		}
		_string_id_hash * p = _string_hash[ix];
		while (p != NULL) {
			if (p->id == id) {
				return p;
			}
			p = p->next;
		}
	}
	BResources * res = _string_file;
	if (!res) {
		res = BApplication::AppResources();
	}
	if (!res) {
		_init_error = ENOENT;
bad_str:
		char msg[100];
		sprintf(msg, "_find_string_id() called for missing id %d\n", id);
		PRINT((msg));
		return NULL;
	}
	size_t size;
	char * str = (char *)res->LoadResource(RESOURCE_TYPE, id, &size);
	if (str == NULL) {
		goto bad_str;
	}
	return _add_string(str, id, false);
}

BResourceStrings::_string_id_hash * BResourceStrings::_add_string(char * str, int id, bool was_malloced)
{
	_string_id_hash * h = new (nothrow) _string_id_hash;
	if (!h) {
		return NULL;	/* out of memory */
	}
	h->next = NULL;
	h->id = id;
	h->data = str;
	h->data_alloced = was_malloced;
	BAutolock lock(_string_lock);
	int mult = (_string_hash_size + 128) / 128;	/* at 128 strings, we allow 2 deep on average */
	if (mult > 10) {
		mult = 10;	/* never allow more than 10 deep on average */
	}
	if (_num_hashed_strings >= _string_hash_size*mult) {
		/* realloc hash table -- carefully crafted to work with 0 size tables, too! */
		int new_count = _string_hash_size * 2;
		if (!new_count) {
			new_count = 64;
		}
		_string_id_hash ** new_hash = _rehash_strings(_string_hash, _string_hash_size, new_count);
		if (new_hash) {
			delete[] _string_hash;
			_string_hash = new_hash;
			_string_hash_size = new_count;
		}
		/* else keep chugging along with current table */
		if (_string_hash_size < 1) {	/* bad! */
			return NULL;
		}
	}
	int ix = id % _string_hash_size;
	if (ix < 0) {
		ix += _string_hash_size;
	}
	_string_id_hash ** p = &_string_hash[ix];
	while (*p != NULL) {
		p = &(*p)->next;
	}
	*p = h;
	_num_hashed_strings += 1;
	return h;
}

status_t BResourceStrings::SetStringFile(const entry_ref * ref)
{
	return _set_string_file(ref);
}

status_t BResourceStrings::_set_string_file(const entry_ref * ref)
{
	BResources * new_file = NULL;
	if (ref) {
		BFile file;
		status_t error = file.SetTo(ref, O_RDONLY);
		if (error < B_OK) {
			if (!_string_file) _init_error = error;
			return error;
		}
		new_file = new BResources;
		error = new_file->SetTo(&file, false);
		if (error < B_OK) {
			delete new_file;
			if (!_string_file) _init_error = error;
			return error;
		}
	}
	else {
		_init_error = (BApplication::AppResources() == NULL) ? B_ERROR : B_OK;
		if (_init_error < 0) return _init_error;
	}
	BAutolock lock(_string_lock);
	if (_string_file) {
		delete _string_file;
	}
	_clear_strings();
	_string_file = new_file;
	if (ref) {
		_cur_string_ref = *ref;
	}
	_init_error = B_OK;
	return _init_error;
}

status_t BResourceStrings::GetStringFile(entry_ref * out_ref)
{
	if (_init_error) return _init_error;
	BAutolock lock(_string_lock);
	*out_ref = _cur_string_ref;
	if (!_string_file) {
		return ENOENT;	/* if the string file is the app file, this is returned, too */
	}
	return B_OK;
}

BResourceStrings::_string_id_hash::_string_id_hash()
{
	next = NULL;
	id = 0;
	data = NULL;
	data_alloced = false;
}

BResourceStrings::_string_id_hash::~_string_id_hash()
{
	if (data_alloced) {
		free(data);
	}
}

void BResourceStrings::_string_id_hash::assign_string(const char * str, bool make_copy)
{
	if (data_alloced) {
		free(data);
	}
	if (make_copy) {
		data = strdup(str);
	}
	else {
		data = (char *)str;	/* skanky cast */
	}
	data_alloced = make_copy;
}


status_t BResourceStrings::_Reserved_ResourceStrings_0(void *) { return B_ERROR; }
status_t BResourceStrings::_Reserved_ResourceStrings_1(void *) { return B_ERROR; }
status_t BResourceStrings::_Reserved_ResourceStrings_2(void *) { return B_ERROR; }
status_t BResourceStrings::_Reserved_ResourceStrings_3(void *) { return B_ERROR; }
status_t BResourceStrings::_Reserved_ResourceStrings_4(void *) { return B_ERROR; }
status_t BResourceStrings::_Reserved_ResourceStrings_5(void *) { return B_ERROR; }

