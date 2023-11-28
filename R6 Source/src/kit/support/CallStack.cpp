
#include <CallStack.h>

#include <Autolock.h>
#include <Debug.h>
#include <Locker.h>
#include <OS.h>

#include <image.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

BCallStack::BCallStack()
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) m_caller[i] = 0;
};

BCallStack::BCallStack(const BCallStack& from)
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) m_caller[i] = from.m_caller[i];
}

#if __POWERPC__
static __asm unsigned long * get_caller_frame();

static __asm unsigned long *
get_caller_frame ()
{
	lwz     r3, 0 (r1)
	blr
}

#endif

#define bogus_image_addr(x)  ((x) < 0x80000000 || (x) >= 0xfc000000)
#define bogus_stack_addr(x)  ((x) < 0xfc000000)

void BCallStack::Update(int32 ignoreDepth)
{
	for (int32 i = 1; i <= CALLSTACK_DEPTH; i++) {
		m_caller[i - 1] = GetCallerAddress(i+ignoreDepth);
	};
};

unsigned long BCallStack::GetCallerAddress(int level) const
{
#if __INTEL__
	uint32 fp = 0, nfp, ret=0;

	level += 2;
	
	fp = (uint32)get_stack_frame();
	if (bogus_stack_addr(fp))
		return 0;
	nfp = *(ulong *)fp;
	while (nfp && --level > 0) {
		if (bogus_stack_addr(fp))
			return 0;
		nfp = *(ulong *)fp;
		ret = *(ulong *)(fp + 4);
		if (bogus_image_addr(ret))
			return 0;
		fp = nfp;
	}

	return ret;
#else
	unsigned long *cf = get_caller_frame();
	unsigned long ret = 0;
	
	level += 1;
	
	while (cf && --level > 0) {
		ret = cf[2];
		if (ret < 0x80000000) break;
		cf = (unsigned long *)*cf;
	}

	return ret;
#endif
}

void BCallStack::SPrint(char *buffer) const
{
	char tmp[32];
	buffer[0] = 0;
	for (int32 i = 0; i < CALLSTACK_DEPTH; i++) {
		if (!m_caller[i]) break;
		sprintf(tmp, " 0x%lx", m_caller[i]);
		strcat(buffer, tmp);
	}
}

void BCallStack::Print(BDataIO& io) const
{
	char tmp[32];
	for (int32 i = 0; i < CALLSTACK_DEPTH; i++) {
		if (!m_caller[i]) break;
		sprintf(tmp, " 0x%lx", m_caller[i]);
		io << tmp;
	}
}

void BCallStack::LongPrint(BDataIO& io, demangle_func demangler, const char* prefix) const
{
	char tmp[256];
	char tmp1[32];
	char tmp2[32];
	uint32 offs;
	for (int32 i = 0; i < CALLSTACK_DEPTH; i++) {
		if (!m_caller[i]) break;
		const char* name = lookup_symbol(m_caller[i], &offs);
		if (!name) continue;
		if (demangler && (*demangler)(name, tmp, 256) != 0) {
			name = tmp;
		}
		sprintf(tmp1, "0x%08lx: <", m_caller[i]);
		sprintf(tmp2, ">+0x%08lx", offs);
		io << prefix << tmp1 << name << tmp2 << endl;
	}
}

BCallStack &BCallStack::operator=(const BCallStack& from)
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) m_caller[i] = from.m_caller[i];
	return *this;
}

bool BCallStack::operator==(const BCallStack& s) const
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) if (m_caller[i] != s.m_caller[i]) return false;
	return true;
}

BCallTreeNode::BCallTreeNode()
{
}

BCallTreeNode::~BCallTreeNode()
{
	PruneNode();
};

void BCallTreeNode::PruneNode()
{
	for (int32 i=0;i<branches.CountItems();i++)
		delete (BCallTreeNode*)(branches.ItemAt(i));
	branches.MakeEmpty();
};

void BCallTreeNode::ShortReport(BDataIO& io)
{
	char tmp[32];
	
	if (!parent) return;
	parent->ShortReport(io);
	
	if (parent->parent)
		sprintf(tmp, ", %08lx", addr);
	else
		sprintf(tmp, "%08lx", addr);
		
	io << tmp;
};

void BCallTreeNode::LongReport(BDataIO& io, demangle_func demangler,
							   char *buffer, int32 bufferSize)
{
	char tmp1[32];
	char tmp2[32];
	const char* name;
	
	uint32 offs;
	if (!parent) return;
	
	parent->LongReport(io, demangler, buffer, bufferSize);
	name = lookup_symbol(addr, &offs);
	if (!name) name = "";
	if (demangler && buffer && (*demangler)(name, buffer, bufferSize) != 0) {
		name = buffer;
	}
	
	sprintf(tmp1, "  0x%08lx: <", addr);
	sprintf(tmp2, ">+0x%08lx", offs);
	
	io << tmp1 << name << tmp2 << endl;
};

BCallTree::BCallTree(const char */*name*/)
{
	addr = 0;
	count = 0;
	higher = lower = highest = lowest = parent = NULL;
};

BCallTree::~BCallTree()
{
	Prune();
};

void BCallTree::Prune()
{
	PruneNode();
	highest = lowest = higher = lower = NULL;
	count = 0;
};

void BCallTree::Report(BDataIO& io, int32 rcount, bool longReport)
{
	BCallTreeNode *n = highest;
	io << count << " total hits, reporting top " << rcount << endl;
	io << "-------------------------------------------------" << endl;
	while (n && rcount--) {
		if (longReport) {
			io << n->count << " hits-------------------------------" << endl;
			n->LongReport(io);
		} else {
			io << n->count << " hits --> ";
			n->ShortReport(io);
			io << endl;
		};
		n = n->lower;
	};
};

void BCallTree::AddToTree(BCallStack *stack, BDataIO& io)
{
	BCallTreeNode *n,*next,*replace;
	int32 i,index = 0;

	if (!stack->m_caller[0]) return;

	n = this;
	while (stack->m_caller[index]) {
		for (i=0;i<branches.CountItems();i++) {
			next = (BCallTreeNode*)(branches.ItemAt(i));
			if (next->addr == stack->m_caller[index]) goto gotIt;
		};
		next = new BCallTreeNode;
		next->addr = stack->m_caller[index];
		next->higher = NULL;
		next->lower = NULL;
		next->count = 0;
		next->parent = n;
		branches.AddItem(next);
		gotIt:
		n = next;
		index++;
		if (index == CALLSTACK_DEPTH) break;
	};
	if (n->count == 0) {
		n->higher = lowest;
		if (lowest) lowest->lower = n;
		else highest = n;
		lowest = n;
	};
	count++;
	n->count++;
	while (n->higher && (n->count > n->higher->count)) {
		replace = n->higher;
		replace->lower = n->lower;
		if (replace->lower == NULL) lowest = replace;
		else replace->lower->higher = replace;
		n->lower = replace;
		n->higher = replace->higher;
		if (n->higher == NULL) highest = n;
		else n->higher->lower = n;
		replace->higher = n;
	};
	
	if (!(count % LOG_OUTPUT_QUANTUM)) {
		Report(io,REPORT_TOP_COUNT,true);
	};
};


namespace BPrivate {

static BLocker			symbolAccess;
static char *			symbolNames = NULL;
static int32			symbolNamesSize = 0;
static int32			symbolNamesPtr = 0;
static symbol*			symbolTable = NULL;
static int32			symbolTableSize = 0;
static int32			symbolTableCount = 0;

static void free_symbol_memory()
{
	if (symbolTable) free(symbolTable);
	symbolTable = NULL;
	symbolNamesSize = symbolNamesPtr = 0;
	if (symbolNames) free(symbolNames);
	symbolNames = NULL;
	symbolTableSize = symbolTableCount = 0;
}

class SymbolTableCleanup
{
public:
	~SymbolTableCleanup() {
		free_symbol_memory();
	}
};

static SymbolTableCleanup symbolTableCleanup;

static int symbol_cmp(const void *p1, const void *p2)
{
	const symbol *sym1 = (const symbol*)p1;
	const symbol *sym2 = (const symbol*)p2;
	return (sym1->addr == sym2->addr)?0:
			((sym1->addr > sym2->addr)?1:-1);
};

bool load_symbols()
{
	if (symbolNames != NULL) return true;
	
	BAutolock _l(symbolAccess);
	if (symbolNames != NULL) return true;
	
	_sPrintf("*** Loading application symbols...\n");
	
	image_info info;
	int32 cookie=0;
	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		const image_id id = info.id;
		int32 n = 0;
		char name[256];
		int32 symType = B_SYMBOL_TYPE_ANY;
		int32 nameLen = sizeof(name);
		void *location;
		while (get_nth_image_symbol(id,n,name,&nameLen,&symType,&location) == B_OK) {
			
			// resize string block, if needed.
			while ((symbolNamesPtr + nameLen) > symbolNamesSize) {
				if (symbolNamesSize > 0) symbolNamesSize *= 2;
				else symbolNamesSize = 1024;
				symbolNames = (char*)realloc(symbolNames,symbolNamesSize);
				if (!symbolNames) {
					free_symbol_memory();
					return false;
				}
			};
			
			// resize symbol block, if needed.
			if ((sizeof(symbol)*symbolTableCount) >= (size_t)symbolTableSize) {
				if (symbolTableSize > 0) symbolTableSize *= 2;
				else symbolTableSize = 1024;
				symbolTable = (symbol*)realloc(symbolTable, symbolTableSize);
				if (!symbolTable) {
					free_symbol_memory();
					return false;
				}
			}
			
			// set up symbol.
			symbol& sym = symbolTable[symbolTableCount++];
			sym.addr = (uint32)location;
			sym.name = symbolNamesPtr;
			
			// set up name.
			strcpy(symbolNames+symbolNamesPtr,name);
			symbolNamesPtr += nameLen;
			
			n++;
			symType = B_SYMBOL_TYPE_ANY;
			nameLen = sizeof(name);
		};
	};
	qsort(symbolTable,symbolTableCount,sizeof(symbol),&symbol_cmp);
	return true;
};

const char * lookup_symbol(uint32 addr, uint32 *offset)
{
	int32 i;
	if (!load_symbols()) return "";
	const int32 count = symbolTableCount-1;
	for (i=0;i<count;i++) {
		if ((addr >= symbolTable[i].addr) && (addr < symbolTable[i+1].addr)) break;
	};
	if (i >= count) return NULL;
	*offset = addr - symbolTable[i].addr;
	return symbolNames + symbolTable[i].name;
};

}
