#include <vector>

#include <SmartCardDefs.h>

using std::vector;

// Explicit instanciation of needed classes
template class pcsc_list<STR>;
template class pcsc_list<GUID_t>;


// To access our vector pointer
#define VECTOR	(static_cast<vector<T> *>(fPrivateData))

// *****************************************************
template <class T>
pcsc_list<T>::pcsc_list(uint32 n)
// *****************************************************
{
	fPrivateData = (void *)(new vector<T>(n));
}

// *****************************************************
template <class T>
pcsc_list<T>::pcsc_list()
// *****************************************************
{
	fPrivateData = (void *)(new vector<T>);
}

// *****************************************************
template <class T>
pcsc_list<T>::pcsc_list(const pcsc_list<T>& copy)
// *****************************************************
{
	const vector<T>& c = *(static_cast<vector<T> *>(copy.fPrivateData));
	fPrivateData = (void *)(new vector<T>(c));
}

// *****************************************************
template <class T>
pcsc_list<T>& pcsc_list<T>::operator=(const pcsc_list<T>& copy)
// *****************************************************
{
	this->~pcsc_list();
	const vector<T>& c = *(static_cast<vector<T> *>(copy.fPrivateData));
	fPrivateData = (void *)(new vector<T>(c));
	return *this;
}

#pragma mark -

// *****************************************************
template <class T>
pcsc_list<T>::~pcsc_list()
// *****************************************************
{
	delete VECTOR;
	fPrivateData = NULL;
}

#pragma mark -

// *****************************************************
template <class T>
void pcsc_list<T>::AddItem(const T& item)
// *****************************************************
{
	VECTOR->push_back(item);
}

// *****************************************************
template <class T>
void pcsc_list<T>::AddItem(const T& item, uint32 index)
// *****************************************************
{
	VECTOR->insert(VECTOR->begin() + index, item);
}

#pragma mark -

// *****************************************************
template <class T>
uint32 pcsc_list<T>::CountItems(void) const
// *****************************************************
{
	return VECTOR->size();
}

// *****************************************************
template <class T>
bool pcsc_list<T>::IsEmpty(void) const
// *****************************************************
{
	return VECTOR->empty();
}

#pragma mark -

// *****************************************************
template <class T>
T& pcsc_list<T>::FirstItem(uint32 index) const
// *****************************************************
{
	return VECTOR->front();
}

// *****************************************************
template <class T>
T& pcsc_list<T>::LastItem(uint32 index) const
// *****************************************************
{
	return VECTOR->back();
}

// *****************************************************
template <class T>
T& pcsc_list<T>::ItemAt(uint32 index) const
// *****************************************************
{
	return VECTOR->operator[](index);
}

// *****************************************************
template <class T>
T& pcsc_list<T>::operator [] (int index) const
// *****************************************************
{
	return VECTOR->operator[](index);
}

#pragma mark -

// *****************************************************
template <class T>
void pcsc_list<T>::MakeEmpty(void)
// *****************************************************
{
	VECTOR->clear();
}

// *****************************************************
template <class T>
void pcsc_list<T>::RemoveItem(uint32 index)
// *****************************************************
{
	VECTOR->erase(VECTOR->begin() + index);
}

// *****************************************************
template <class T>
void pcsc_list<T>::RemoveItems(uint32 index, uint32 count)
// *****************************************************
{
	VECTOR->erase(VECTOR->begin() + index, VECTOR->begin() + index + count);
}
