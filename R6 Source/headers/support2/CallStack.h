
#ifndef _SUPPORT2_CALLSTACK_H
#define _SUPPORT2_CALLSTACK_H

#include <support2/ITextStream.h>
#include <support2/SupportDefs.h>
#include <support2/Vector.h>

namespace B {
namespace Support2 {

class BCallTree;

typedef int32 (*b_demangle_func)(	const char *mangled_name,
									char *unmangled_name,
									size_t buffersize);

enum {
	B_CALLSTACK_DEPTH		= 16
};

class BCallStack {
public:
							BCallStack();
							BCallStack(const BCallStack& o);
	virtual					~BCallStack();
	
			BCallStack		&operator=(const BCallStack& o);
			
			void			Update(int32 ignoreDepth=0);
			
			size_t			AddressAt(int32 level) const;
			void			SPrint(char *buffer) const;
			void			Print(ITextOutput::arg io) const;
			void			LongPrint(ITextOutput::arg io, b_demangle_func demangler=NULL) const;
		
			bool			operator==(const BCallStack& o) const;
	inline	bool			operator!=(const BCallStack& o) const	{ return !(*this == o); }
	
private:
			size_t			GetCallerAddress(int32 level) const;

			size_t			m_caller[B_CALLSTACK_DEPTH];
			int32			_reserved[2];
};

inline ITextOutput::arg operator<<(ITextOutput::arg io, const BCallStack& stack)
{
	stack.Print(io);
	return io;
}

class BCallTreeNode {
public:
							BCallTreeNode();
	virtual					~BCallTreeNode();
	
			void			PruneNode();
			void			ShortReport(ITextOutput::arg io);
			void			LongReport(ITextOutput::arg io, b_demangle_func demangler=NULL,
									   char *buffer=NULL, int32 bufferSize=0);
	
private:
							BCallTreeNode(const BCallTreeNode& o);
			BCallTreeNode&	operator=(const BCallTreeNode& o);
	
	friend	class					BCallTree;
			
			size_t					addr;
			int32					count;
			BCallTreeNode *			higher;
			BCallTreeNode *			lower;
			BCallTreeNode *			parent;
			BVector<BCallTreeNode*>	branches;
};

class BCallTree : public BCallTreeNode {
public:
							BCallTree(const char *name);
	virtual					~BCallTree();
	
			void			Prune();
			void			AddToTree(BCallStack *stack, ITextOutput::arg io);
			void			Report(ITextOutput::arg io, int32 count, bool longReport=false);

private:
							BCallTree(const BCallTree& o);
			BCallTree&		operator=(const BCallTree& o);
			
			BCallTreeNode*	highest;
			BCallTreeNode*	lowest;
};

} }	// namespace B::Support2

#endif /* _SUPPORT2_CALLSTACK_H */
