/*
	This implements the pattern matching described in Modern Compiler
	Design, by Grune, Bal, Jacobs and Langendoen (ISBN 0-471-97697-0),
	Section 2.1.  Specifically, we use their terminology.  Otherwise,
	this textbook sucks, but it's what we used in Compilers class
	in College, so I've read it, and the algorithm works.
	
	-joe
*/

/*
	Item is defined as an ElementDecl (which must have mode CHILDREN),
	and an index, which tells you how far into the pattern the "dot" is.
*/

#define DEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <OS.h>

#include <support2/SupportDefs.h>
#include <support2/Debug.h>
#include <xml2/BContent.h>
#include <validate_children.h>

namespace B {
namespace XML {

#define DUPLICATE_SHIFT_ERROR_STR_1 "WARNING: Ambiguous Pattern!  Keeping first the one I found.\n"
#define DUPLICATE_SHIFT_ERROR_STR_2 DUPLICATE_SHIFT_ERROR_STR_1
const char * kStringIntCrashMessage = "StringIntList must have a StringStore associated with it.";


class BIntegerList;
class StringStore;
class StringIntList;
class TransitionInfo;

class BIntegerList
{
public:
				BIntegerList(int32 initialAllocSize = 0);
	virtual		~BIntegerList();
	
	bool		AddItem(int32 n);
	int32		ItemAt(int32 index) const;
	bool		FindValue(int32 n);
	bool		RemoveItem(int32 n);
	int32		RemoveItemAt(int32 index);
	int32 		CountItems() const;
	
private:
	BVector<void*>	_list;
};

// Add another level of indirection, which will allow us to only 
class StringStore
{
public:
	~StringStore();			// << --- Note no virtual dtor
	
	int32		AddString(const char * s);				// Returns an id for the string
	const char	* FindStringByID(int32 id) const;		// Return a string (or NULL) searching by ID
	int32		FindIDByString(const char * s) const;	// Look for it, return the id
	
	int32		CountItems() const;
	const char	* ItemAt(int32 i) const;
	
	void		UseMe(TransitionInfo * array, int32 count);
	
private:
	BVector<void*> _list;
};

class StringIntList
{
public:
	StringIntList();
	~StringIntList();
	void AddItem(const char * s, int32 n);
	int32 CountItems() const;
	const char * StringAt(int32 index) const;
	int32 IntAt(int32 index) const;
	int32 FindStringIndex(const char * s) const;
	void ItemAt(int32 index, uint16 * stringID, uint16 * nextState) const;
	
private:
	BVector<void*>	_strings;
	BVector<void*>	_ints;
	StringStore * _store;
	friend class StringStore;
};

class TransitionInfo
{
public:
	TransitionInfo();
	TransitionInfo(const BIntegerList & unexpanded);
	
	void AddUnexpandedItem(int32 item);
	int32 CountUnexpandedItems() const;
	int32 UnexpandedItemAt(int32 index) const;
	
	void AddExpandedItem(int32 item);
	int32 CountExpandedItems() const;
	int32 ExpandedItemAt(int32 index) const;
	
	void AddShiftState(const char * element, int32 nextIndex);
	int32 CountShiftStates() const;
	void ShiftStateAt(int32 index, uint16 * stringID, uint16 * nextState) const;
	
	bool AllExpanded() const;
	
	bool IsSuccess() const;
	
	bool closureFinished;
	enum {EPSILON, SHIFT, SUCCESS} type;
	
	void Print() const;
	
	void DoClosure(TransitionInfo * transitions);
	
private:
	bool					_transitiveSuccess;
	BIntegerList			_unexpandedItems;
	BIntegerList			_expandedItems;
	StringIntList			_shiftStates;
	friend class StringStore;
};


BIntegerList::BIntegerList(int32 )
	:_list()
{
	
}

BIntegerList::~BIntegerList()
{
	
}

bool
BIntegerList::AddItem(int32 n)
{
	if (FindValue(n))
		return true;
	return _list.AddItem((void *) n);
}

int32
BIntegerList::ItemAt(int32 index) const
{
	return (int32) _list.ItemAt(index);
}

bool
BIntegerList::FindValue(int32 n)
{
	int32 count = _list.CountItems();
	for (int32 i=0; i<count; i++)
		if (ItemAt(i) == n)
			return true;
	return false;
}

bool
BIntegerList::RemoveItem(int32 n)
{
	int32 count = _list.CountItems();
	for (int32 i=0; i<count; i++)
	{
		if (ItemAt(i) == n)
		{
			_list.RemoveItemsAt(i);
			return true;
		}
	}
	return false;
}

int32
BIntegerList::RemoveItemAt(int32 index)
{
	_list.RemoveItemsAt((int32) index);
	return index;
}

int32
BIntegerList::CountItems() const
{
	return _list.CountItems();
}


StringStore::~StringStore()
{
	int32 count = _list.CountItems();
	for (int32 i=0; i<count; i++)
		free(_list.ItemAt(i));
}

int32
StringStore::AddString(const char * s)
{
	int32 id;
	id = FindIDByString(s);
	if (id >= 0)
		return id;
	_list.AddItem(strdup(s));
	return _list.CountItems()-1;
}

const char *
StringStore::FindStringByID(int32 id) const
{
	return reinterpret_cast<const char *>(_list.ItemAt(id)); // ItemAt(int32) does the check and returns NULL
}

int32
StringStore::FindIDByString(const char * s) const
{
	int32 count = _list.CountItems();
	for (int32 i=0; i<count; i++)
		if (0 == strcmp(s, reinterpret_cast<const char *>(_list.ItemAt(i))))
			return i;
	return -1;
}

int32
StringStore::CountItems() const
{
	return _list.CountItems();
}

const char *
StringStore::ItemAt(int32 i) const
{
	return (const char *) _list.ItemAt(i);
}

void
StringStore::UseMe(TransitionInfo * array, int32 count)
{
	// Holy friend access Batman!
	for (int32 i=0; i<count; i++)
		array[i]._shiftStates._store = this;
}


StringIntList::StringIntList()
	:_strings(), _ints(), _store(NULL)
{
	
}

StringIntList::~StringIntList()
{

}

void
StringIntList::AddItem(const char * s, int32 n)
{
	ASSERT(_strings.CountItems() == _ints.CountItems());
	if (!_store)
		debugger(kStringIntCrashMessage);
	_strings.AddItem((void *) _store->AddString(s));
	_ints.AddItem((void *) n);
}

int32
StringIntList::CountItems() const
{
	ASSERT(_strings.CountItems() == _ints.CountItems());
	if (!_store)
		debugger(kStringIntCrashMessage);
	return _strings.CountItems();
}

const char *
StringIntList::StringAt(int32 index) const
{
	ASSERT(_strings.CountItems() == _ints.CountItems());
	if (!_store)
		debugger(kStringIntCrashMessage);
	if (index >= (int32)_strings.CountItems())
		return NULL;
	int32 stringID = (int32) _strings.ItemAt(index);
	return _store->FindStringByID(stringID);
}

int32
StringIntList::IntAt(int32 index) const
{
	ASSERT(_strings.CountItems() == _ints.CountItems());
	if (!_store)
		debugger(kStringIntCrashMessage);
	if (index >= (int32)_ints.CountItems())
		return -1;
	return (int32) _ints.ItemAt(index);
}

int32
StringIntList::FindStringIndex(const char * s) const
{
	ASSERT(_strings.CountItems() == _ints.CountItems());
	if (!_store)
		debugger(kStringIntCrashMessage);
	int32 count = _strings.CountItems();
	for (int32 i=0; i<count; i++)
	{
		int32 stringID = (int32) _strings.ItemAt(i);
		const char * str = _store->FindStringByID(stringID);
		if (0 == strcmp(s, str))
			return i;
	}
	return -1;
}

void
StringIntList::ItemAt(int32 index, uint16 * stringID, uint16 * nextState) const
{
	ASSERT(_strings.CountItems() == _ints.CountItems());
	*stringID = (int32) _strings.ItemAt(index);
	*nextState = (int32) _ints.ItemAt(index);
}


TransitionInfo::TransitionInfo()
	:_transitiveSuccess(false)
{
	
}

TransitionInfo::TransitionInfo(const BIntegerList & unexpanded)
	:closureFinished(false),
	 type(SUCCESS),
	 _transitiveSuccess(false),
	 _unexpandedItems(unexpanded.CountItems())
{
	int32 count = unexpanded.CountItems();
	for (int32 i=0; i<count; i++)
		_unexpandedItems.AddItem(unexpanded.ItemAt(i));
}

void
TransitionInfo::AddUnexpandedItem(int32 item)
{
	if (!_expandedItems.FindValue(item))
		_unexpandedItems.AddItem(item);
}

int32
TransitionInfo::CountUnexpandedItems() const
{
	return _unexpandedItems.CountItems();
}

int32
TransitionInfo::UnexpandedItemAt(int32 index) const
{
	return _unexpandedItems.ItemAt(index);
}

void
TransitionInfo::AddExpandedItem(int32 item)
{
	_unexpandedItems.RemoveItem(item);
	_expandedItems.AddItem(item);
}

int32
TransitionInfo::CountExpandedItems() const
{
	return _expandedItems.CountItems();
}

int32
TransitionInfo::ExpandedItemAt(int32 index) const
{
	return _expandedItems.ItemAt(index);
}

void
TransitionInfo::AddShiftState(const char * element, int32 nextIndex)
{
	int32 index = _shiftStates.FindStringIndex(element);
	//printf("TransitionInfo::AddShiftState element: %s  nextIndex: %ld  index: %ld\n", element, nextIndex, index); 
	if (index >= 0)
	{
		;
		//if (_shiftStates.IntAt(index) == nextIndex)
		//	printf(DUPLICATE_SHIFT_ERROR_STR_1);
	}
	else
		_shiftStates.AddItem(element, nextIndex);
}

int32
TransitionInfo::CountShiftStates() const
{
	return _shiftStates.CountItems();
}

void
TransitionInfo::ShiftStateAt(int32 index, uint16 * stringID, uint16 * nextState) const
{
	_shiftStates.ItemAt(index, stringID, nextState);
}

bool
TransitionInfo::AllExpanded() const
{
	return _unexpandedItems.CountItems() == 0;
}

bool
TransitionInfo::IsSuccess() const
{
	return _transitiveSuccess || type == SUCCESS;
}

void
TransitionInfo::Print() const
{
	if (type == EPSILON)
		printf("(EPSILON)");
	else if (type == SHIFT)
		printf("(SHIFT)");
	else if (type == SUCCESS)
		printf("(SUCCESS)");
	for (int32 j=0; j<_expandedItems.CountItems(); j++)
		printf(" (%ld)", _expandedItems.ItemAt(j));
	for (int32 j=0; j<_unexpandedItems.CountItems(); j++)
		printf(" %ld", _unexpandedItems.ItemAt(j));
	for (int32 j=0; j<_shiftStates.CountItems(); j++)
		printf(" %s->%ld", _shiftStates.StringAt(j), _shiftStates.IntAt(j));
	if (IsSuccess())
		printf(" (possible success)");
}

void
TransitionInfo::DoClosure(TransitionInfo * transitions)
{
	int32 myCount = _unexpandedItems.CountItems();
	for (int32 i=myCount-1; i>=0; i--)
	{
		int32 otherCount, j;
		int32 item = _unexpandedItems.RemoveItemAt(i);
		AddExpandedItem(item);
		TransitionInfo & other = transitions[item];
		
		//printf("expanding item: %ld --- ", item);
		//other.Print();
		//printf("\n");
		
		otherCount = other.CountUnexpandedItems();
		for (j=0; j<otherCount; j++)
			AddUnexpandedItem(other.UnexpandedItemAt(j));
		otherCount = other.CountExpandedItems();
		for (j=0; j<otherCount; j++)
			AddExpandedItem(other.ExpandedItemAt(j));
		otherCount = other._shiftStates.CountItems();
		for (j=0; j<otherCount; j++)
		{
			const char * s = other._shiftStates.StringAt(j);
			int32 n = other._shiftStates.IntAt(j);
			//printf("transferring transition: %s->%ld\n", s, n);
			int32 next = _shiftStates.FindStringIndex(s);
			if (next >= 0)
			{
				//if (_shiftStates.IntAt(next) != n)
				//	printf(DUPLICATE_SHIFT_ERROR_STR_2);
			}
			else
				_shiftStates.AddItem(s, n);
		}
		_transitiveSuccess |= other.IsSuccess();
		
		//printf("Current Value: ");
		//Print();
		//printf("\n");
	}
}



static void
print_cardinality(BElementDecl::node_cardinality cardin)
{
	switch (cardin)
	{
		case BElementDecl::ONE:
			break;
		case BElementDecl::ONE_OR_MORE:
			printf("+ ");
			break;
		case BElementDecl::ZERO_OR_MORE:
			printf("* ");
			break;
		case BElementDecl::ONE_OR_ZERO:
			printf("? ");
			break;
		default:
			break;
	}
}

static void
maybe_print_dot(int32 & currentIndex, int32 index, int32 debug)
{
	if (currentIndex == index)
//		printf(". ");
		printf(".%ld ", debug);
//	printf("[%d] ", currentIndex);
	currentIndex++;
}

static void
print_item_recursive(BElementDecl::ListNode * node, int32 & currentIndex, int32 index)
{
	if (!node)
		exit(1);
	if (node->type == BElementDecl::ListNode::STRING)
	{
		exit(1);
	}
	else
	{
		BElementDecl::List * list = node->list;
		if (!list)
			exit(1);
		
		maybe_print_dot(currentIndex, index, 0);
		if (node->cardinality != BElementDecl::ONE)
		{
			printf("( ");				// outer '(' always has cardinality
			maybe_print_dot(currentIndex, index, 1);
		}
		printf("( "); 					// never has cardinality
		int32 count = list->CountItems();
		for (int32 i=0; i<count; i++)
		{
			BElementDecl::ListNode * nextNode = list->NodeAt(i);
			if (i!=0 && list->type == BElementDecl::List::CHOICE)
				printf("| ");
			if (nextNode->type == BElementDecl::ListNode::STRING)
			{	
				if (nextNode->cardinality != BElementDecl::ONE)
				{
					maybe_print_dot(currentIndex, index, 4);
					printf("( ");		// inner '(' always has cardinality (string only)
				}
				maybe_print_dot(currentIndex, index, 5);
				printf("%s ", nextNode->element.String());
				if (nextNode->cardinality != BElementDecl::ONE)
				{
					maybe_print_dot(currentIndex, index, 6);
					printf(")");		// inner ')' always has cardinality (string only)
				}
				print_cardinality(nextNode->cardinality);
			}
			else
			{
				print_item_recursive(nextNode, currentIndex, index);
			}
			if (list->type == BElementDecl::List::CHOICE)
			{
				// Just matched this node in a choice list
				maybe_print_dot(currentIndex, index, 8);
			}
			else if (i == count-1 && list->type == BElementDecl::List::SEQ)
			{							// reduce item after the last one in a seq
				maybe_print_dot(currentIndex, index, 9);
			}
		}
		printf(") "); 					// never has cardinality
		
		if (node->cardinality != BElementDecl::ONE)
		{								// outer ')' always has cardinality
			maybe_print_dot(currentIndex, index, 10);
			printf(")");
		}
		print_cardinality(node->cardinality);
	}
}
/*
static void
print_item(BElementDecl * decl, int32 index)
{
	int32 currentIndex = 0;
	BElementDecl::ListNode * node = decl->GetPattern();
	print_item_recursive(node, currentIndex, index);
	maybe_print_dot(currentIndex, index, 11);
}
*/

static void
count_total_items_recursive(const BElementDecl::ListNode * node, int32 & runningSum)
{
	if (!node)
		exit(1);
	if (node->type == BElementDecl::ListNode::STRING)
	{
		exit(1);
	}
	else
	{
		BElementDecl::List * list = node->list;
		if (!list)
			exit(1);
		
		runningSum++;
		if (node->cardinality != BElementDecl::ONE)
			runningSum++;
		int32 count = list->CountItems();
		for (int32 i=0; i<count; i++)
		{
			BElementDecl::ListNode * nextNode = list->NodeAt(i);
			if (nextNode->type == BElementDecl::ListNode::STRING)
			{	
				if (nextNode->cardinality != BElementDecl::ONE)
					runningSum++;		// inner '(' always has cardinality (string only)
				runningSum++;
				if (nextNode->cardinality != BElementDecl::ONE)
					runningSum++;		// inner ')' always has cardinality (string only)
			}
			else
			{
				count_total_items_recursive(nextNode, runningSum);
			}
			if (list->type == BElementDecl::List::CHOICE)
				runningSum++;
			else if (i == count-1 && list->type == BElementDecl::List::SEQ)
				runningSum++;			// reduce item after the last one in a seq
		}
		if (node->cardinality != BElementDecl::ONE)
		{
			runningSum++;				// outer ')' always has cardinality
		}
	}
}

static int32
count_total_items(const BElementDecl * decl)
{
	int32 runningSum = 0;
	const BElementDecl::ListNode * node = decl->GetPattern();
	count_total_items_recursive(node, runningSum);
	return runningSum + 1; // + 1 is the final one outside -- meaning we're done
}


/*
	Counts the number of items INSIDE the list given by node
*/
static int32
count_list_items(const BElementDecl::ListNode * node)
{
	int32 runningSum = 0;
	
	if (node->type != BElementDecl::ListNode::LIST)
		return 0;
	
	BElementDecl::List * list = node->list;
	if (!list)
		return 0;
	
	// These two on the ends are the two on the outside of the parens
	// all the others occur inside the parens, so count them.  In other
	// words, we're replacing the first recursion of the
	// count_total_items_recursive function
	// runningSum++;
	if (node->cardinality != BElementDecl::ONE)
		runningSum++;
	int32 count = list->CountItems();
	for (int32 i=0; i<count; i++)
	{
		BElementDecl::ListNode * nextNode = list->NodeAt(i);
		if (nextNode->type == BElementDecl::ListNode::STRING)
		{	
			if (nextNode->cardinality != BElementDecl::ONE)
				runningSum++;		// inner '(' always has cardinality (string only)
			runningSum++;
			if (nextNode->cardinality != BElementDecl::ONE)
				runningSum++;		// inner ')' always has cardinality (string only)
		}
		else
		{
			// Want all of the children
			count_total_items_recursive(nextNode, runningSum);
		}
		if (list->type == BElementDecl::List::CHOICE)
			runningSum++;
		else if (i == count-1 && list->type == BElementDecl::List::SEQ)
			runningSum++;			// reduce item after the last one in a seq
	}
	if (node->cardinality != BElementDecl::ONE)
	{
		runningSum++;				// outer ')' always has cardinality
	}

	return runningSum;
}


static int32
count_string_node_items(BElementDecl::ListNode * node)
{
	if (node->cardinality != BElementDecl::ONE)
		return 2; // The '(' ')' that appear if there's cardinality
	return 1;
}

static bool
maybe_find_transitions(int32 & currentIndex, int32 index, int32 debug)
{
	(void) debug;
	
	if (currentIndex == index)
	{
		//printf(".%d ", debug);
		return true;
	}
	currentIndex++;
	return false;
}

/*
Transition Type
Numbers			This Node				Next Node
	0			T --> a.Rb				T --> aR.b
	1			T --> a.(R)*b			T --> a(R)*.b
										T --> a(.R)*b
	2			T --> a(R.)*b			T --> a(R)*.b
										T --> a(.R)*b
	3			T --> a(R.)+b			T --> a(.R)+b
	4			T --> a(R.)+b			T --> a(R)+.b
										T --> a(.R)+b
	5			T --> a.(R)?b			T --> a(R)?.b
										T --> a(.R)?b
	6			T --> a(R.)?b			T --> a(R)?.b
	7			T --> a.(R1|R2|etc)b	T --> a(.R1|R2|etc)b
										T --> a(R1|.R2|etc)b
	8			T --> a(R1.|R2|etc)b	T --> a(R1|R2|etc).b
	9			T --> a.(R1 R2 etc)b	T --> a(.R1 R2 etc)b
	10			T --> a(etc R.)b		T --> a(etc R).b

Note that 0 is a shift item, and the rest are epsilon items
*/



/*
	Note, list must be a choice list
*/
static void
get_all_choices_in_list(BElementDecl::List * list, int32 currentIndex, TransitionInfo & transition)
{
	if (list->type != BElementDecl::List::CHOICE)
		debugger("get_all_choices_in_list: file a bug");
	int32 runningIndex = currentIndex + 1;
	int32 count = list->CountItems();
	for (int32 i=0; i<count; i++)
	{
		BElementDecl::ListNode * nextNode = list->NodeAt(i);
		if (nextNode->type == BElementDecl::ListNode::STRING)
		{
			// printf("\tNext Item: %d\n", runningIndex);
			transition.AddUnexpandedItem(runningIndex);
			if (nextNode->cardinality != BElementDecl::ONE)
				runningIndex += 2; // The '(' ')' that appear if there's cardinality
			runningIndex++;
		}
		else
		{
			// printf("\tNext Item: %d\n", runningIndex);
			transition.AddUnexpandedItem(runningIndex);
			runningIndex += count_list_items(nextNode);
		}
		runningIndex++; // The item that goes after each one in a choice
	}
}

static bool
get_next_items_recursive(const BElementDecl::ListNode * node, int32 & currentIndex, int32 index, TransitionInfo & transition)
{
	if (!node)
		debugger("get_next_items_recursive error (no node).  File a bug with a stack crawl.");
	if (node->type == BElementDecl::ListNode::STRING)
	{
		debugger("get_next_items_recursive error (string node).  File a bug with a stack crawl.");
	}
	else
	{
		BElementDecl::List * list = node->list;
		if (!list)
			exit(1);
		int32 itemCount = count_list_items(node);
		int32 thisListStart = currentIndex;
		
		if (maybe_find_transitions(currentIndex, index, 0))
		{
			switch (node->cardinality)
			{
				// If it's ONE, we have something like this:
				//      .0 ( E1 E2 )
				// Otherwise, we have something like this:
				//		.0 ( ( E1 E2 ) )*
				case BElementDecl::ONE:
					if (list->type == BElementDecl::List::CHOICE)
					{
						//printf("\nTransition Type: 7 (epsilon)\n");
						get_all_choices_in_list(list, currentIndex, transition);
					}
					else
					{
						//printf("\nTransition Type: 9 (epsilon)\n");
						//printf("\tNext Item: %d\n", currentIndex + 1);
						transition.AddUnexpandedItem(currentIndex + 1);
					}
					break;
				case BElementDecl::ONE_OR_MORE:
					//printf("\nTransition Type: 3 (epsilon)\n");
					//printf("\tNext Item: %d\n", currentIndex + 1);
					transition.AddUnexpandedItem(currentIndex + 1);
					break;
				case BElementDecl::ZERO_OR_MORE:
					//printf("\nTransition Type: 1 (epsilon)\n");
					//printf("\tNext Item: %d\n", currentIndex + 1);
					//printf("\tNext Item: %d\n", currentIndex + itemCount + 1);
					transition.AddUnexpandedItem(currentIndex + 1);
					transition.AddUnexpandedItem(currentIndex + itemCount + 1);
					break;
				case BElementDecl::ONE_OR_ZERO:
					//printf("\nTransition Type: 5 (epsilon)\n");
					//printf("\tNext Item: %d\n", currentIndex + 1);
					//printf("\tNext Item: %d\n", currentIndex + itemCount + 1);
					transition.AddUnexpandedItem(currentIndex + 1);
					transition.AddUnexpandedItem(currentIndex + itemCount + 1);
					break;
				default:
					break;
			}
			return true;
		}
		if (node->cardinality != BElementDecl::ONE)
		{
			//printf("( ");				// outer '(' always has cardinality
			if (maybe_find_transitions(currentIndex, index, 1))
			{
				// We're here:
				//		( .1 ( E1 E2 ) )*
				if (list->type == BElementDecl::List::CHOICE)
				{
					//printf("\nTransition Type: 7 (epsilon)\n");
					get_all_choices_in_list(list, currentIndex, transition);
				}
				else
				{
					//printf("\nTransition Type: 9 (epsilon)\n");
					//printf("\tNext Item: %d\n", currentIndex + 1);
					transition.AddUnexpandedItem(currentIndex + 1);
				}
				return true;
			}
		}
		//printf("( "); 					// never has cardinality
		int32 count = list->CountItems();
		for (int32 i=0; i<count; i++)
		{
			BElementDecl::ListNode * nextNode = list->NodeAt(i);
			//if (i!=0 && list->type == BElementDecl::List::CHOICE)
			//	printf("| ");
			if (nextNode->type == BElementDecl::ListNode::STRING)
			{	
				if (nextNode->cardinality != BElementDecl::ONE)
				{
					if (maybe_find_transitions(currentIndex, index, 4))
					{
						switch (nextNode->cardinality)
						{
							// We have something like this:
							//		( .4 ( E1 )+ E2 )
							case BElementDecl::ONE:
								// not possible
								break;
							case BElementDecl::ONE_OR_MORE:
								//printf("\nTransition Type: 3 (epsilon)\n");
								//printf("\tNext Item: %d\n", currentIndex + 1);
								transition.AddUnexpandedItem(currentIndex + 1);
								break;
							case BElementDecl::ZERO_OR_MORE:
								//printf("\nTransition Type: 1 (epsilon)\n");
								//printf("\tNext Item: %d\n", currentIndex + 1);
								//printf("\tNext Item: %d\n", currentIndex + count_string_node_items(nextNode) + 1);
								transition.AddUnexpandedItem(currentIndex + 1);
								transition.AddUnexpandedItem(currentIndex + count_string_node_items(nextNode) + 1);
								break;
							case BElementDecl::ONE_OR_ZERO:
								//printf("\nTransition Type: 5 (epsilon)\n");
								//printf("\tNext Item: %d\n", currentIndex + 1);
								//printf("\tNext Item: %d\n", currentIndex + count_string_node_items(nextNode) + 1);
								transition.AddUnexpandedItem(currentIndex + 1);
								transition.AddUnexpandedItem(currentIndex + count_string_node_items(nextNode) + 1);
								break;
							default:
								break;
						}
						return true;
					}
					//printf("( ");		// inner '(' always has cardinality (string only)
				}
				if (maybe_find_transitions(currentIndex, index, 5))
				{
					// We have something like this:
					//		( .5 E1 )?
					// or
					//		.5 E1
					//printf("\nTransition Type: 0 (shift)\n");
					//printf("\tElement to Find: %s\n", nextNode->element.String());
					transition.type = transition.SHIFT;
					transition.AddShiftState(nextNode->element.String(), index+1);
					//printf("FOUND SHIFT: ");
					//transition.Print();
					//printf("\n");
					return true;
				}
				//printf("%s ", nextNode->element.String());
				if (nextNode->cardinality != BElementDecl::ONE)
				{
					if (maybe_find_transitions(currentIndex, index, 6))
					{
						switch (nextNode->cardinality)
						{
							// We have something like this:
							//		( E1 .6 )?
							case BElementDecl::ONE:
								// not possible
								break;
							case BElementDecl::ONE_OR_MORE:
								//printf("\nTransition Type: 4 (epsilon)\n");
								//printf("\tNext Item: %d\n", currentIndex - count_string_node_items(nextNode) + 1);
								//printf("\tNext Item: %d\n", currentIndex + 1);
								transition.AddUnexpandedItem(currentIndex - count_string_node_items(nextNode) + 1);
								transition.AddUnexpandedItem(currentIndex + 1);
								break;
							case BElementDecl::ZERO_OR_MORE:
								//printf("\nTransition Type: 2 (epsilon)\n");
								//printf("\tNext Item: %d\n", currentIndex - count_string_node_items(nextNode) + 1);
								//printf("\tNext Item: %d\n", currentIndex + 1);
								transition.AddUnexpandedItem(currentIndex - count_string_node_items(nextNode) + 1);
								transition.AddUnexpandedItem(currentIndex + 1);
								break;
							case BElementDecl::ONE_OR_ZERO:
								//printf("\nTransition Type: 6 (epsilon)\n");
								//printf("\tNext Item: %d\n", currentIndex + 1);
								transition.AddUnexpandedItem(currentIndex + 1);
								break;
							default:
								break;
						}
						return true;
					}
					//printf(")");		// inner ')' always has cardinality (string only)
				}
				//print_cardinality(nextNode->cardinality);
			}
			else
			{
				if (get_next_items_recursive(nextNode, currentIndex, index, transition))
					return true;
			}
			if (list->type == BElementDecl::List::CHOICE)
			{
				// Just matched this node in a choice list
				if (maybe_find_transitions(currentIndex, index, 8))
				{
					//printf("\nTransition Type: 8 (epsilon)\n");
					//printf("\tNext Item: %d\n", thisListStart + itemCount);
					transition.AddUnexpandedItem(thisListStart + itemCount);
					return true;
				}
			}
			else if (i == count-1 && list->type == BElementDecl::List::SEQ)
			{							// reduce item after the last one in a seq
				if (maybe_find_transitions(currentIndex, index, 9))
				{
					// We have something like this:
					//		( E1 E2 .7 ) <-- Note never a *+? here
					//printf("\nTransition Type: 10 (epsilon)\n");
					//printf("\tNext Item: %d\n", currentIndex + 1);
					transition.AddUnexpandedItem(currentIndex + 1);
					return true;
				}
			}
		}
		//printf(") "); 					// never has cardinality
		
		if (node->cardinality != BElementDecl::ONE)
		{
			if (maybe_find_transitions(currentIndex, index, 10))
			{
				switch (node->cardinality)
				{
					// We have something like this:
					//		( E1 E2 .9 )? <-- Note always a *+? here
					case BElementDecl::ONE:
						// not possible
						break;
					case BElementDecl::ONE_OR_MORE:
						//printf("\nTransition Type: 4 (epsilon)\n");
											// + 1 is to put it right inside the first '('
						//printf("\tNext Item: %d\n", thisListStart + 1);
						//printf("\tNext Item: %d\n", currentIndex + 1);
						transition.AddUnexpandedItem(thisListStart + 1);
						transition.AddUnexpandedItem(currentIndex + 1);
						break;
					case BElementDecl::ZERO_OR_MORE:
						//printf("\nTransition Type: 2 (epsilon)\n");
											// + 1 is to put it right inside the first '('
						//printf("\tNext Item: %d\n", thisListStart + 1);
						//printf("\tNext Item: %d\n", currentIndex + 1);
						transition.AddUnexpandedItem(thisListStart + 1);
						transition.AddUnexpandedItem(currentIndex + 1);
						break;
					case BElementDecl::ONE_OR_ZERO:
						//printf("\nTransition Type: 6 (epsilon)\n");
						//printf("\tNext Item: %d\n", currentIndex + 1);
						transition.AddUnexpandedItem(currentIndex + 1);
						break;
					default:
						break;
				}
				return true;
			}
			//printf(")");				// outer ')' always has cardinality
		}
		//print_cardinality(node->cardinality);
	}
	return false;
}

static void
get_next_items(const BElementDecl * decl, int32 index, TransitionInfo & transition)
{
	int32 currentIndex = 0;
	const BElementDecl::ListNode * node = decl->GetPattern();
	transition.type = transition.EPSILON;
	if (!get_next_items_recursive(node, currentIndex, index, transition))
	{
		if (maybe_find_transitions(currentIndex, index, 11))
		{
			transition.type = transition.SUCCESS;
			// We have something like this:
			//		( E1 E2 .7 ) <-- Note never a *+? here
			//printf("\nTransition Type: 10 (epsilon)\n");
			//printf("\tSuccess!\n");
		}
	}
}

void
do_transition_closure(TransitionInfo * transitions, int32 count)
{
	bool done = false;
	while (!done)
	{
		done = true;
		for (int32 i=0; i<count; i++)
		{
			transitions[i].DoClosure(transitions);
			if (!transitions[i].AllExpanded())
				done = false;
		}
	}
}


#define B_XML_TRANSITON_TABLE_MAGIC_NUMBER 0xBE05BE05

static void
make_transition_table(TransitionInfo * transitions, uint16 stateCount, StringStore * store, const void ** tableData)
{
	int32 i, j;
	uint16 stringCount = store->CountItems();
	uint32 tableSize = 0;

	// Calculate The Size of Memory to Allocate
	tableSize += sizeof(uint32);	// magic number
	tableSize += sizeof(uint16) * 2; // size of stringCount and stateCount
	tableSize += sizeof(uint32) * stringCount;  // string array
	tableSize += sizeof(uint32) * stateCount;   // state array
	for (i=0; i<stringCount; i++)				 // the string data
		tableSize += strlen(store->ItemAt(i)) + 1;
	for (i=0; i<stateCount; i++)				 // the state transition data
		tableSize += (2 * sizeof(uint16)) + (sizeof(uint16) * 2 * transitions[i].CountShiftStates());
	
	// Allocate the table
	//printf("Creating Transition Table.  Size is %ld bytes.\n", tableSize);
	char * table = (char *) malloc(tableSize);

// Memory Layout of the table
//   uint32 magicNumber					// B_XML_TRANSITON_TABLE_MAGIC_NUMBER (0xBE05BE05)
//   uint16 stringCount					// Number of strings
//   uint16 stateCount					// Number of states
//   uint32 stringOffsets[stringCount]	// Offsets from beginning to the start of a string
//   uint32 stateOffsets[stateCount]	// Offsets from beginning to the start of a state
//   char  stringData[total size of the strings]  // A bunch of null terminated strings,
//                                                   pointed to by the stringOffsets
//   stateData -->
//			uint16 columnCount			// How many transitions there are here
//          uint16 success (boolean)	// Is this possibly a success state?
//			One for each columnCount -->
//				uint16 stringIndex		// Index into stringOffsets of the string to match
//				uint16 nextState		// Next state to go to if we find the string

	// Where does the data go
	uint32 * magicNumberPtr = (uint32 *) table;
	uint16 * stringCountPtr = (uint16 *) (magicNumberPtr + 1);
	uint16 * stateCountPtr = (uint16 *) (stringCountPtr + 1);
	uint32 * stringOffsetsPtr = (uint32 *) (stateCountPtr + 1);
	uint32 * stateOffsetsPtr = (uint32 *) (stringOffsetsPtr + stringCount);
	char * stringDataPtr = (char *) (stateOffsetsPtr + stateCount);
	
	char * p;
	uint16 * p16;
	const char * s;
	
	// Magic Number
	*magicNumberPtr = B_XML_TRANSITON_TABLE_MAGIC_NUMBER;
	
	// stringCount and stateCount
	*stringCountPtr = stringCount;
	*stateCountPtr = stateCount;
	
	// stringData and stringOffsets
	p = stringDataPtr;
	for (i=0; i<stringCount; i++)
	{
		stringOffsetsPtr[i] = p - table;
		s = store->ItemAt(i);
		strcpy(p, s);
		p += strlen(s) + 1;
	}
	
	// stateData and stateOffsets
	p16 = (uint16 *) p;
	for (i=0; i<stateCount; i++)
	{
		uint16 columnCount, stringID, nextState;
		p = (char *) p16;
		stateOffsetsPtr[i] = p - table;
		columnCount = transitions[i].CountShiftStates();
		*p16++ = columnCount;
		*p16++ = transitions[i].IsSuccess() ? 1 : 0;
		for (j=0; j<columnCount; j++)
		{
			transitions[i].ShiftStateAt(j, &stringID, &nextState);
			*p16++ = stringID;
			*p16++ = nextState;
		}
	}
	
	// Return the table
	*tableData = table;
}

void
_init_validate_table_(const BElementDecl * decl, const void ** tablePtr)
{
	int32 count = count_total_items(decl);
	TransitionInfo * transitions = new TransitionInfo[count];
	StringStore store;
	store.UseMe(transitions, count);
	for (int32 i=0; i<count; i++)
		get_next_items(decl, i, transitions[i]);
	do_transition_closure(transitions, count);
	make_transition_table(transitions, count, &store, tablePtr);
	delete[] transitions;
}

void
_init_validate_state_(uint16 * state)
{
	*state = 0;
}

status_t
_check_next_element_(const char * element, uint16 * state, const void * tableData)
{
	// See the comment in make_transition_table about memory layout to see what's going on here
	int32 i;
	const char * table = (const char *) tableData;
	const uint32 * magicNumberPtr = (const uint32 *) table;
	const uint16 * stringCountPtr = (const uint16 *) (magicNumberPtr + 1);
	const uint16 * stateCountPtr = (const uint16 *) (stringCountPtr + 1);
	const uint32 * stringOffsetsPtr = (const uint32 *) (stateCountPtr + 1);
	const uint32 * stateOffsetsPtr = (const uint32 *) (stringOffsetsPtr + *stringCountPtr);
	
	// The beginning of this state info
	const uint16 * p = (const uint16 *) (table + stateOffsetsPtr[*state]);
	uint16 columnCount = *p;
	p += 2;					// skip over the success info
	for (i=0; i<columnCount; i++)
	{
		uint16 stringID = *p++;
		uint16 nextState = *p++;
		if (0 == strcmp(element, table + stringOffsetsPtr[stringID]))
		{
			*state = nextState;
			return B_OK;
		}
	}
	return B_XML_CHILD_ELEMENT_NOT_ALLOWED;
}

status_t
_check_end_of_element_(uint16 state, const void * tableData)
{
	// See the comment in make_transition_table about memory layout to see what's going on here
	const char * table = (const char *) tableData;
	const uint32 * magicNumberPtr = (const uint32 *) table;
	const uint16 * stringCountPtr = (const uint16 *) (magicNumberPtr + 1);
	const uint16 * stateCountPtr = (const uint16 *) (stringCountPtr + 1);
	const uint32 * stringOffsetsPtr = (const uint32 *) (stateCountPtr + 1);
	const uint32 * stateOffsetsPtr = (const uint32 *) (stringOffsetsPtr + *stringCountPtr);
	
	// The beginning of this state info
	const uint16 * p = (const uint16 *) (table + stateOffsetsPtr[state]);
	p++;					// Skip over the columnCount
	uint16 successHere = *p;
	if (successHere)
		return B_OK;
	return B_XML_CHILD_PATTERN_NOT_FINISHED;	
}

void
_free_validate_table_(const void * tableData)
{
	free((void *) tableData);
}

}; // namespace XML
}; // namespace B

