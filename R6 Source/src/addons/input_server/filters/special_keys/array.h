
#define grMalloc(a,b,c)		malloc(a)
#define grFree(a)			free(a)
#define grCalloc(a,b,c,d)	calloc(a,b)
#define grRealloc(a,b,c,d)	realloc(a,b)
#define grStrdup(a,b,c)		strdup(a)

template <class t>
class BArray {

	private:

		t		*items;
		int32	numItems;
		int32	numSlots;
		int32	blockSize;

inline	int32	AssertSize(int size)
{
	if (size > numSlots) {
		if (numSlots<=0) numSlots = blockSize;
		if (numSlots<=0) numSlots = 1;
		while (size > numSlots) numSlots *= 2;
		t *tmp = (t*)grRealloc(items,numSlots*sizeof(t),"AssertSize",MALLOC_CANNOT_FAIL);
		if (!tmp) return -1;
		items = tmp;
	};
	return size;
};

	public:

inline			BArray(int _blockSize=256)
{
	blockSize = _blockSize;
	numItems = numSlots = 0;
	items = NULL;
};

inline			BArray(BArray<t> &copyFrom)
{
	blockSize = copyFrom.blockSize;
	numSlots = 0;
	items = NULL;
	AssertSize(copyFrom.numSlots);
	numItems = copyFrom.numItems;
	memcpy(items,copyFrom.items,numItems*sizeof(t));
};

inline			~BArray()
{
	if (items)
		grFree(items);
};

inline	t*		Items()
{
	return items;
};

inline	void		SetList(t* newList, int32 listSize)
{
	if (items)
		grFree(items);
	items = newList;
	numSlots = listSize;
	numItems = listSize;
};

inline	void		SetSlots(int32 slots)
{
	if (numSlots != slots) {
		numSlots = slots;
		if (numItems > numSlots)
			numItems = numSlots;
		t *tmp = (t*)grRealloc(items,numSlots*sizeof(t),"SetSlots",MALLOC_CANNOT_FAIL);
		if (!tmp) return;
		items = tmp;
	};
};

inline	void		SetItems(int32 count)
{
	if (AssertSize(count) < 0)
		return;
	numItems = count;
};

inline	void		Trim()
{
	SetSlots(numItems);
};

inline	int32	CountItems()
{ return numItems; };

inline	void		RemoveItems(int32 index, int32 len)
{
	memmove(items+index,items+index+len,sizeof(t)*(numItems-index-len));
	numItems-=len;
};

inline	void		RemoveItem(int32 index)
{
	RemoveItems(index,1);
};

inline	int32	AddArray(BArray<t> *a)
{
	if (AssertSize(numItems + a->numItems) < 0)
		return -1;
	memcpy(items+numItems,a->items,a->numItems*sizeof(t));
	numItems = numItems + a->numItems;
	return a->numItems;
};

inline	int32	AddItem(const t &theT)
{
	if (AssertSize(numItems+1) < 0)
		return -1;
	items[numItems] = theT;
	numItems++;
	return numItems-1;
};

inline	void		MakeEmpty()
{
	numSlots = 0;
	numItems = 0;
	if (items!=NULL) {
		grFree(items);
		items = NULL;
	};
};

inline	t&		ItemAt(int index)
				{ return items[index]; };

inline	t&		operator[](int index)
				{ return items[index]; };
};

