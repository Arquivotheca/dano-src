
#include <xml2/BStringMap.h>
#include <ctype.h>
#include <stdio.h>

namespace B {
namespace XML {


// =====================================================================
BStringMap::BStringMap()
	:_names(),
	 _values()
{
	
}


// =====================================================================
BStringMap::BStringMap(const BStringMap & copy)
	:_names(),
	 _values()
{
	BString * name;
	BString * value;
	int32 count = copy._names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		name = new BString(*(BString *) copy._names.ItemAt(i));
		value = new BString(*(BString *) copy._names.ItemAt(i));
		_names.AddItem(name);
		_values.AddItem(value);
	}
}


// =====================================================================
BStringMap &
BStringMap::operator=(const BStringMap & copy)
{
	if (&copy == this)
		return *this;
	
	MakeEmpty();
	BString * name;
	BString * value;
	int32 count = copy._names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		name = new BString(*(BString *) copy._names.ItemAt(i));
		value = new BString(*(BString *) copy._values.ItemAt(i));
		_names.AddItem(name);
		_values.AddItem(value);
	}
	return *this;
}


// =====================================================================
BStringMap::~BStringMap()
{
	MakeEmpty();
}


// Adopts, and leaves the arguments empty
// =====================================================================
void
BStringMap::AddAdopt(BString & key, BString & val)
{
	int32 index = FindIndex(key);
	if (index == -1)
	{
		// Add it fresh
		BString * name = new BString;
		BString * value = new BString;
		name->Adopt(key);
		value->Adopt(val);
		_names.AddItem(name);
		_values.AddItem(value);
	}
	else
	{
		// Replace
		BString * str = (BString *) _values.ItemAt(index);
		str->Adopt(val);
	}
}


// =====================================================================
void
BStringMap::Add(const BString & key, const BString & val)
{
	int32 index = FindIndex(key);
	if (index == -1)
	{
		// Add it fresh
		BString * name = new BString(key);
		BString * value = new BString(val);
		_names.AddItem(name);
		_values.AddItem(value);
	}
	else
	{
		// Replace
		BString * str = (BString *) _values.ItemAt(index);
		str->SetTo(val);
	}
}

void 
BStringMap::Add(const BString &key, int val)
{
	BString tmp;
	tmp << val;
	Add(key, tmp);
}

void 
BStringMap::Add(const BString &key, unsigned int val)
{
	BString tmp;
	tmp << val;
	Add(key, tmp);
}

void 
BStringMap::Add(const BString &key, uint32 val)
{
	BString tmp;
	tmp << val;
	Add(key, tmp);
}

void 
BStringMap::Add(const BString &key, int32 val)
{
	BString tmp;
	tmp << val;
	Add(key, tmp);
}

void 
BStringMap::Add(const BString &key, uint64 val)
{
	BString tmp;
	tmp << val;
	Add(key, tmp);
}

void 
BStringMap::Add(const BString &key, int64 val)
{
	BString tmp;
	tmp << val;
	Add(key, tmp);
}

void 
BStringMap::Add(const BString &key, float val)
{
	BString tmp;
	tmp << val;
	Add(key, tmp);
}


// =====================================================================
int32
BStringMap::FindIndex(const BString & key) const
{
	int32 count = _names.CountItems();
	for (int32 i=0; i<count; ++i)
		if (*((BString *) _names.ItemAt(i)) == key)
			return i;
	return -1;
}


// =====================================================================
status_t
BStringMap::PairAt(int32 index, BString & key, BString & val) const
{
	if (index < 0 || index >= (int32)_names.CountItems())
		return B_BAD_INDEX;
	BString * name = (BString *) _names.ItemAt(index);
	BString * value = (BString *) _values.ItemAt(index);
	key.SetTo(*name);
	val.SetTo(*value);
	return B_OK;
}


// =====================================================================
status_t
BStringMap::PairAt(int32 index, BString ** key, BString ** val)
{
	if (index < 0 || index >= (int32)_names.CountItems())
		return B_BAD_INDEX;
	*key = (BString *) _names.ItemAt(index);
	*val = (BString *) _values.ItemAt(index);
	return B_OK;
}


// =====================================================================
status_t
BStringMap::PairAt(int32 index, const BString ** key, const BString ** val) const
{
	if (index < 0 || index >= (int32)_names.CountItems())
		return B_BAD_INDEX;
	*key = (const BString *) _names.ItemAt(index);
	*val = (const BString *) _values.ItemAt(index);
	return B_OK;
}


// =====================================================================
status_t
BStringMap::Find(const BString & key, BString & val) const
{
	BString * str;
	int32 count = _names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		str = (BString *) _names.ItemAt(i);
		if (*(str) == key)
		{
			val.SetTo(*((BString *) _values.ItemAt(i)));
			return B_OK;
		}
	}
	return B_NAME_NOT_FOUND;
}


// =====================================================================
BString *
BStringMap::Find(const BString & key) const
{
	BString * str;
	int32 count = _names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		str = (BString *) _names.ItemAt(i);
		if (*(str) == key)
			return (BString *) _values.ItemAt(i);
	}
	return NULL;
}


// =====================================================================
void
BStringMap::MakeEmpty()
{
	int32 count = _names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		delete (BString *) _names.ItemAt(i);
		delete (BString *) _values.ItemAt(i);
	}
	_names.MakeEmpty();
	_values.MakeEmpty();
}


// =====================================================================
int32
BStringMap::CountItems() const
{
	return _names.CountItems();
}


// =====================================================================
void
MushString(BString & str)
{
	if (str.Length() == 0)
		return;
	char* buf = str.LockBuffer(str.Length());
	char* begin = buf;
	char* pos = buf;
	bool lastSpace = false;
	while (*buf) {
		if (pos == begin && isspace(*buf)) {
			buf++;
			continue;
		}
		if (pos < buf) *pos = *buf;
		if (*pos == '\n' || *pos == '\r' || *pos == '\t')
			*pos = ' ';
		if (isspace(*buf)) {
			if (!lastSpace) {
				pos++;
				lastSpace = true;
			}
		} else {
			lastSpace = false;
			pos++;
		}
		buf++;
	}
	if (pos > begin && isspace(*(pos-1)))
		*(pos-1) = '\0';
	else
		*pos = '\0';
	str.UnlockBuffer(pos-begin);
}


// =====================================================================
void
StripWhitespace(BString & str)
{
	char* buf = str.LockBuffer(str.Length());
	char* begin = buf;
	char* pos = buf;
	while (*buf) {
		if (isspace(*buf)) {
			buf++;
			continue;
		}
		*pos++ = *buf++;
	}
	*pos = '\0';
	str.UnlockBuffer(pos-begin);
}


// =====================================================================
// STRING SET
// =====================================================================

// =====================================================================
BStringSet::BStringSet()
	:_values()
{
	
}


// =====================================================================
BStringSet::BStringSet(const BStringSet & copy)
	:_values()
{
	int32 count = copy._values.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BString * str = new BString(*(BString *) copy._values.ItemAt(i));
		_values.AddItem(str);
	}
}


// =====================================================================
BStringSet &
BStringSet::operator=(const BStringSet & copy)
{
	MakeEmpty();
	
	int32 count = copy._values.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BString * str = new BString(*(BString *) copy._values.ItemAt(i));
		_values.AddItem(str);
	}
	
	return *this;
}


// =====================================================================
BStringSet::~BStringSet()
{
	MakeEmpty();
}


// =====================================================================
status_t
BStringSet::AddAdopt(BString & val)
{
	if (Exists(val))
		return B_NAME_IN_USE;
	
	BString * str = new BString();
	str->Adopt(val);
	_values.AddItem(str);
	return B_OK;
}


// =====================================================================
status_t
BStringSet::Add(const BString & val)
{
	if (Exists(val))
		return B_NAME_IN_USE;
	
	BString * str = new BString(val);
	_values.AddItem(str);
	return B_OK;
}


// =====================================================================
status_t
BStringSet::Remove(const BString & val)
{
	int32 count = _values.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BString * str = (BString *) _values.ItemAt(i);
		if (val == *str)
		{
			_values.RemoveItemsAt(i);
			delete str;
			return B_OK;
		}
	}
	return B_NAME_NOT_FOUND;
}


// =====================================================================
bool
BStringSet::Exists(const BString & val) const
{
	int32 count = _values.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BString * str = (BString *) _values.ItemAt(i);
		if (val == *str)
			return true;
	}
	return false;
}


// =====================================================================
int32
BStringSet::CountItems() const
{
	return _values.CountItems();
}


// =====================================================================
const BString &
BStringSet::ItemAt(int32 index) const
{
	return *(BString *) _values.ItemAt(index);
}


// =====================================================================
void
BStringSet::MakeEmpty()
{
	int32 count = _values.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		delete (BString *) _values.ItemAt(i);
	}
	_values.MakeEmpty();
}


// Return the next string in str, starting after pos, in split.
// split will start in the next character after any whitespace happening
// at pos, and continue up until, but not including any whitespace.
// =====================================================================
bool
SplitStringOnWhitespace(const BString & str, BString & split, int32 * pos)
{
	if (*pos >= str.Length())
		return false;
	
	int count = 0;
	const char * p = str.String() + *pos;
	
	// Use up any preceeding whitespace
	while (*p && isspace(*p))
		p++;
	if (*p == '\0')
		return false;
		
	const char * s = p;
	
	// Go until the end
	while (*p && !isspace(*p))
	{
		count++;
		p++;
	}
	
	split.SetTo(s, count);
	*pos = p - str.String();
	return true;
}


}; // namespace XML
}; // namespace B


