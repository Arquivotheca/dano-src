/* :ts=8 bk=0
 *
 * listnode_rp.h:	Definitions for doubly-linked Lists and Nodes, using
 *			"relative pointers".
 *
 *			These are, in all semantic respects, identical to
 *			Nodes and Lists as defined in 'listnode.h'.  However,
 *			the link fields pointing to the next and previous
 *			nodes are maintained using "relative pointers";
 *			signed integers measuring the offset from the current
 *			Node to the next node in sequence.  This allows linked
 *			lists to be maintained in a shared memory area whose
 *			base address may not be the same across all execution
 *			contexts.  It also avoids passing around the base
 *			address all the time, as well as the NULL problem with
 *			"offset pointers"
 *
 * Leo L. Schwab					2000.04.07
 */
#ifndef _LISTNODERP_H
#define	_LISTNODERP_H

#ifndef _OS_H
#include <kernel/OS.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Node_RP {
	int32	n_Next;
	int32	n_Prev;
} Node_RP;

typedef struct List_RP {
	union {
		struct {
			struct Node_RP	lrp_HeadNode;
			int32		__invalid;
		} head;
		struct {
			int32		__invalid;
			struct Node_RP	lrp_TailNode;
		} tail;
	} u;
} List_RP;

#define	lrp_Head	u.head.lrp_HeadNode.n_Next
#define	lrp_Tail	u.tail.lrp_TailNode.n_Prev
#define	lrp__null	u.head.lrp_HeadNode.n_Prev	/*  Set to NULL  */

/*  THESE TWO MACROS MUST BE IMMUNE TO SIDE-EFFECTS  */
#define	RELATIVE2ADDR(base,offset)	((void *) ((int32) (base) + \
						   (int32) (offset)))
#define	ADDR2RELATIVE(from,to)		((int32) ((int32) (to) - \
						  (int32) (from)))

#define	FIRSTNODE_RP(l)	RELATIVE2ADDR (&((List_RP *) (l))->u.head.lrp_HeadNode, \
				       ((List_RP *) (l))->lrp_Head)
#define	LASTNODE_RP(l)	RELATIVE2ADDR (&((List_RP *) (l))->u.tail.lrp_TailNode, \
				       ((List_RP *) (l))->lrp_Tail)

#define	NEXTNODE_RP(n)	RELATIVE2ADDR ((n), ((Node_RP *) (n))->n_Next)
#define	PREVNODE_RP(n)	RELATIVE2ADDR ((n), ((Node_RP *) (n))->n_Prev)
#define	ISORPHANNODE_RP(n)	(((Node_RP *) (n))->n_Next == \
				 ((Node_RP *) (n))->n_Prev)

/*
 * Not strictly necessary if you plan to place the Node in a list immediately.
 * However, if you need to test a Node for list membership before it ends up
 * in a List, then this is useful.
 */
#define	INITNODE(n)	(((Node *) (n))->n_Next = ((Node *) (n))->n_Prev = 0)


extern void InitList_RP (struct List_RP *l);
extern void AddHead_RP (struct Node_RP *n, struct List_RP *l);
extern void AddTail_RP (struct Node_RP *n, struct List_RP *l);
extern void RemNode_RP (struct Node_RP *n);
extern struct Node_RP *RemHead_RP (struct List_RP *l);
extern struct Node_RP *RemTail_RP (struct List_RP *l);
extern void InsertNodeBefore_RP (struct Node_RP *n, struct Node_RP *radix);
extern void InsertNodeAfter_RP (struct Node_RP *n, struct Node_RP *radix);


#ifdef __cplusplus
}
#endif

#endif	/*  _LISTNODERP_H  */
