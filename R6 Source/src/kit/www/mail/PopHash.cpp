
#include <www/mail/PopHash.h>
#include <www/util.h>

static const uint16 primes[] = {127, 251, 509, 761, 1021, 1279, 1531, 1789, 2039, 2557, 3067, 4093, 5119, 6143, 7159, 8191};
static const uint8 primeCount = 16;


PopHash::PopHash() :
	fTableSize(0),
	fTable(NULL),
	fUsed(0)
{
}


PopHash::~PopHash()
{
	if (fTable)
		delete [] fTable;
}

status_t 
PopHash::ResizeTable(int32 size)
{
//	printf("PopHash::ResizeTable(%ld)\n", size);
	status_t status = B_OK;
	uint32 newTableSize = 0;

	for (uint32 ix = 0; ix < primeCount; ix++) {
		if (primes[ix] > size) {
			newTableSize = primes[ix];
			break;
		}
	}

	if (newTableSize == 0) {
		// too big!
		return B_ERROR;
	}

//	printf("use prime: %ld for size: %ld\n", newTableSize, size);
	
	// see if we can reuse the table - in which case we should clear it
	if (fTable && (newTableSize < fTableSize)) {
		// clear the table
		for (uint ix = 0; ix < fTableSize; ix++) {
			fTable[ix].uid = B_EMPTY_STRING;
		}
		status = B_OK;
	}
	else {
		// either there is no table or we have to resize
		if (fTable) {
			delete [] fTable;
			fTable = NULL;
		}
		
		if (newTableSize > 0) {
			fTable = new id_map[newTableSize];
			fTableSize = newTableSize;
			status = B_OK;
		}
		else
			status = B_ERROR;
	}
	fUsed = 0;
	return status;
}


//#define DoubleHash(key, attempt, size) (uint32)((key % size) + (attempt * (1 + (key % (size - 2))))) % size
#define h1(key, size) (uint32)(key % size)
#define h2(key, size) (uint32)(1 + (key % (size - 2)))

int32 
PopHash::find_slot(const char *uid, bool empty)
{
	// get the hash key
	uint32 key = HashString(uid);
	uint32 attempt = 0;
	int32 slot = -1;
	uint32 startSlot = h1(key, fTableSize);
	uint32 interval = h2(key, fTableSize);
	
	for (; attempt < fTableSize; attempt++) {
		slot = (startSlot + (attempt * interval)) % fTableSize;
//		slot = DoubleHash(key, attempt, fTableSize);

		bool found = (empty) ? (fTable[slot].listnum == -1) : (fTable[slot].uid == uid);
		if (!found)
			slot = -1;
		else
			break;
	}
	return slot;
}

BString *
PopHash::Insert(const char *uid, int32 listNum)
{
//	printf("Insert(%s, %ld): fTable: %p fUsed: %ld fTableSize: %ld\n", uid, listNum);
	
	if (!fTable || fUsed == fTableSize)
		return NULL;
		
	int32 slot = find_slot(uid, true);
	if (slot == -1) {
		printf("our hashing function is not uniform!\n");
		return NULL;
	}

	// insert the data
	fTable[slot].uid = uid;
	fTable[slot].listnum = listNum;
	fUsed++;
	return new BString(uid);
//	return &(fTable[slot].uid);
}

int32 
PopHash::Lookup(const char *uid)
{
	if (!fTable)
		return -1;
	
	int32 slot = find_slot(uid, false);
	if (slot == -1)
		return slot;
	else
		return fTable[slot].listnum;
}

void 
PopHash::PrintToStream(FILE *stream)
{
	if (fTable) {\
		fprintf(stream, "start PopHash: %p ======================\n size: %ld contents:\n", this, fTableSize);
		for (uint ix = 0; ix < fTableSize; ix++) {
			fprintf(stream, "    %4d %4ld %s\n", ix, fTable[ix].listnum, fTable[ix].uid.String());
		}
		fprintf(stream, " end PopHash: %p ======================\n", this);
	}
}

