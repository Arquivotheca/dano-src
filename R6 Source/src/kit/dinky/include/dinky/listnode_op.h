/* :ts=8 bk=0
 *
 * listnode_op.h:	Definitions for doubly-linked Lists and Nodes, using
 *			"offset pointers".
 *
 *			These are, in all semantic respects, identical to
 *			Nodes and Lists as defined in 'listnode.h'.  However,
 *			the link fields pointing to the next and previous
 *			nodes are maintained using "offset pointers";
 *			integers measuring the offset of the Node from a base
 *			location.  This allows linked lists to be maintained
 *			in a shared memory area whose base address may not be
 *			the same across all execution contexts.
 *
 *			WARNING: Make sure you have something already
 *			consuming space that isn't a Node_OP at offset 0.  0
 *			is interpreted as a NULL pointer; allowing an offset
 *			pointer to offset 0 would appear as end-of-list.
 *			B_DONT_DO_THAT.
 *
 * Leo L. Schwab					1999.08.11
 */
#ifndef _LISTNODEOP_H
#define	_LISTNODEOP_H

#ifndef _OS_H
#include <kernel/OS.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Node_OP {
	uint32	n_Next;
	uint32	n_Prev;
} Node_OP;

typedef struct List_OP {
	union {
		struct {
			struct Node_OP	lop_HeadNode;
			uint32		__invalid;
		} head;
		struct {
			uint32		__invalid;
			struct Node_OP	lop_TailNode;
		} tail;
	} u;
} List_OP;

#define	lop_Head	u.head.lop_HeadNode.n_Next
#define	lop_Tail	u.tail.lop_TailNode.n_Prev
#define	lop__null	u.head.lop_HeadNode.n_Prev	/*  Set to NULL  */

/*  THESE TWO MACROS MUST BE IMMUNE TO SIDE-EFFECTS  */
#define	OFFSET2ADDR(base,offset)	((void *) ((uint32) (base) + \
						   (uint32) (offset)))
#define	ADDR2OFFSET(base,addr)		((uint32) ((uint32) (addr) - \
						   (uint32) (base)))

#define	FIRSTNODE_OP(base,l)	OFFSET2ADDR (base, ((List_OP *) (l))->lop_Head)
#define	LASTNODE_OP(base,l)	OFFSET2ADDR (base, ((List_OP *) (l))->lop_Tail)

#define	NEXTNODE_OP(base,n)	NextNode_OP ((void *) (base), (Node_OP *) (n))
#define	PREVNODE_OP(base,n)	PrevNode_OP ((void *) (base), (Node_OP *) (n))
#define	ISORPHANNODE_OP(n)	(((Node_OP *) (n))->n_Next == \
				 ((Node_OP *) (n))->n_Prev)

/*
 * Not strictly necessary if you plan to place the Node in a list immediately.
 * However, if you need to test a Node for list membership before it ends up
 * in a List, then this is useful.  Obviously, 'n' can't have any side-
 * effects.
 */
#define	INITNODE_OP(base,n)	(((Node_OP *) (n))->n_Next = \
				 ((Node_OP *) (n))->n_Prev = \
				  ADDR2OFFSET (base, (n)))


extern void InitList_OP (void *base, struct List_OP *l);
extern void AddHead_OP (void *base, struct Node_OP *n, struct List_OP *l);
extern void AddTail_OP (void *base, struct Node_OP *n, struct List_OP *l);
extern void RemNode_OP (void *base, struct Node_OP *n);
extern struct Node_OP *RemHead_OP (void * base, struct List_OP *l);
extern struct Node_OP *RemTail_OP (void * base, struct List_OP *l);
extern void InsertNodeBefore_OP (void *base,
				 struct Node_OP *n, struct Node_OP *radix);
extern void InsertNodeAfter_OP (void *base,
				struct Node_OP *n, struct Node_OP *radix);
extern struct Node_OP *NextNode_OP (void *base, struct Node_OP *n);
extern struct Node_OP *PrevNode_OP (void *base, struct Node_OP *n);


#ifdef __cplusplus
}
#endif

#endif	/*  _LISTNODE_H  */
