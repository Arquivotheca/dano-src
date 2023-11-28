/* :ts=8 bk=0
 *
 * listnode.h:	Definitions for doubly-linked Lists and Nodes.
 *
 * Leo L. Schwab					1999.01.04
 */
#ifndef _LISTNODE_H
#define	_LISTNODE_H

#ifndef _OS_H
#include <kernel/OS.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Node {
	struct Node	*n_Next;
	struct Node	*n_Prev;
} Node;

typedef struct List {
	union {
		struct {
			struct Node	l_HeadNode;
			struct Node	*__invalid;
		} head;
		struct {
			struct Node	*__invalid;
			struct Node	l_TailNode;
		} tail;
	} u;
} List;

#define	l_Head		u.head.l_HeadNode.n_Next
#define	l_Tail		u.tail.l_TailNode.n_Prev
#define	l__null		u.head.l_HeadNode.n_Prev	/*  Set to NULL  */


#define	FIRSTNODE(l)	(((List *) (l))->l_Head)
#define	LASTNODE(l)	(((List *) (l))->l_Tail)

#define	NEXTNODE(n)	(((Node *) (n))->n_Next)
#define	PREVNODE(n)	(((Node *) (n))->n_Prev)
#define	ISORPHANNODE(n)	(((Node *) (n))->n_Next == ((Node *) (n))->n_Prev)

/*
 * Not strictly necessary if you plan to place the Node in a list immediately.
 * However, if you need to test a Node for list membership before it ends up
 * in a List, then this is useful.
 */
#define	INITNODE(n)	(((Node *) (n))->n_Next = \
			 ((Node *) (n))->n_Prev = (Node *) (n))


extern void InitList (struct List *l);
extern void AddHead (struct Node *n, struct List *l);
extern void AddTail (struct Node *n, struct List *l);
extern void RemNode (struct Node *n);
extern struct Node *RemHead (struct List *l);
extern struct Node *RemTail (struct List *l);
extern void InsertNodeBefore (struct Node *n, struct Node *radix);
extern void InsertNodeAfter (struct Node *n, struct Node *radix);


#ifdef __cplusplus
}
#endif

#endif	/*  _LISTNODE_H  */
