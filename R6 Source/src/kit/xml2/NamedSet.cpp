
#include <xml2/BContent.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

namespace B {
namespace XML {

// =====================================================================
BNamedSet::BNamedSet()
{
	// nothing
}


// =====================================================================
BNamedSet::~BNamedSet()
{
	// Items are NOT deleted
}


// =====================================================================
status_t
BNamedSet::Add(BNamed * named)
{
	if (!named)
		return B_OK;
	BNamespace * ns = named->Namespace();
	int32 i, cmp;
	int32 count = _data.CountItems();
	for (i=0; i<count; i++)
	{
		cmp = strcmp(_data.ItemAt(i)->Name(), named->Name());
		if (cmp == 0)
		{
			// Names are same, check namespace
			BNamespace * ns2 = _data.ItemAt(i)->Namespace();
			if ((!ns && !ns2) || ((ns && ns2) && (0 == strcmp(ns->Value(), ns2->Value()))))
				return B_NAME_IN_USE;
		}
		else if (cmp > 0)
		{
			_data.AddItemAt(named, i);
			return B_OK;
		}
	}
	_data.AddItem(named);
	return B_OK;
}


// =====================================================================
void
BNamedSet::Remove(BNamed * named)
{
	int32 count = _data.CountItems();
	for (int32 i=0; i<count; i++)
	{
		if (_data.ItemAt(i) == named)
			_data.RemoveItemsAt(i);
	}
}


// =====================================================================
void
BNamedSet::Remove(int32 index)
{
	_data.RemoveItemsAt(index);
}


// =====================================================================
BNamed *
BNamedSet::Find(const char * name)
{
	BNamed * named;
	int32 i, cmp;
	int32 count = _data.CountItems();
	for (i=0; i<count; i++)
	{
		named = _data.ItemAt(i);
		cmp = strcmp(named->Name(), name);
		if (cmp == 0)
		{
			// Names match, check namespace -- only accept null namespace
			if (!named->Namespace())
				return named;
		}
	}
	return NULL;
}

const BNamed *
BNamedSet::Find(const char * name) const
{
	const BNamed * named;
	int32 i, cmp;
	int32 count = _data.CountItems();
	for (i=0; i<count; i++)
	{
		named = _data.ItemAt(i);
		cmp = strcmp(named->Name(), name);
		if (cmp == 0)
		{
			// Names match, check namespace -- only accept null namespace
			if (!named->Namespace())
				return named;
		}
	}
	return NULL;
}


// =====================================================================
int32
BNamedSet::CountItems() const
{
	return _data.CountItems();
}


// =====================================================================
BNamed *
BNamedSet::ItemAt(int32 index)
{
	return _data.ItemAt(index);
}

const BNamed *
BNamedSet::ItemAt(int32 index) const
{
	return _data.ItemAt(index);
}


// =====================================================================
void
BNamedSet::MakeEmpty(bool deleteData)
{
	if (deleteData) {
		int32 count = _data.CountItems();
		for (int32 i=0; i<count; i++) {
			delete _data.ItemAt(i);
		}
	}
	_data.MakeEmpty();
}


}; // namespace XML
}; // namespace B

