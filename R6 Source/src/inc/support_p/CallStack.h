
#ifndef CALL_STACK_H
#define CALL_STACK_H

#define LOG_OUTPUT_QUANTUM	5000
#define REPORT_TOP_COUNT	5
#define CALLSTACK_DEPTH		16

#include <List.h>
#include <StreamIO.h>

namespace BPrivate {
	struct symbol {
		uint32	addr;
		uint32	name;
	};
		
	extern bool load_symbols();
	extern const char *lookup_symbol(uint32 addr, uint32 *offset);

	typedef int32 (*demangle_func)(const char *mangled_name,
								   char *unmangled_name,
								   size_t buffersize);
}
using namespace BPrivate;

class BCallStack {
	public:

		uint32			m_caller[CALLSTACK_DEPTH];

						BCallStack();
						BCallStack(const BCallStack& o);
		unsigned long 	GetCallerAddress(int level) const;
		void 			SPrint(char *buffer) const;
		void 			Print(BDataIO& io) const;
		void 			LongPrint(BDataIO& io, demangle_func demangler=NULL,
								  const char* prefix="") const;
		void 			Update(int32 ignoreDepth=0);

		BCallStack		&operator=(const BCallStack& o);
		bool			operator==(const BCallStack& o) const;
};

class BCallTreeNode {
	public:
	uint32					addr;
	int32					count;
	BCallTreeNode *			higher;
	BCallTreeNode *			lower;
	BCallTreeNode *			parent;
	BList					branches;			// contains BCallTreeNode*
	
							BCallTreeNode();
							~BCallTreeNode();
	void					PruneNode();
	void					ShortReport(BDataIO& io);
	void					LongReport(BDataIO& io, demangle_func demangler=NULL,
									   char *buffer=NULL, int32 bufferSize=0);
};

class BCallTree : public BCallTreeNode {
	public:
	BCallTreeNode *			highest;
	BCallTreeNode *			lowest;

							BCallTree(const char *name);
							~BCallTree();
	void					Prune();
	void					AddToTree(BCallStack *stack, BDataIO& io = BOut);
	void					Report(BDataIO& io, int32 count, bool longReport=false);
};

#endif
