/* :ts=8 bk=0
 *
 * listnode_op.c:	Routines for manipulating doubly-linked lists, using
 *			"offset pointers" rather than absolute pointers.
 *
 * Leo L. Schwab					1999.08.11
 */
#include <dinky/listnode_op.h>


#define	OFFSET2NODE(base,node)	((Node_OP *) OFFSET2ADDR ((base), (node)))


void
InitList_OP (void *base, struct List_OP *l)
{
	l->lop_Head = ADDR2OFFSET (base, &l->u.tail.lop_TailNode);
	l->lop_Tail = ADDR2OFFSET (base, &l->u.head.lop_HeadNode);
	l->lop__null = 0;
}

void
AddHead_OP (void *base, struct Node_OP *n, struct List_OP *l)
{
	uint32	nodeoff;

	nodeoff = ADDR2OFFSET (base, n);

	n->n_Prev = ADDR2OFFSET (base, &l->u.head.lop_HeadNode);
	OFFSET2NODE (base, (n->n_Next = l->lop_Head))->n_Prev = nodeoff;
	l->lop_Head = nodeoff;
}

void
AddTail_OP (void *base, struct Node_OP *n, struct List_OP *l)
{
	uint32	nodeoff;

	nodeoff = ADDR2OFFSET (base, n);

	n->n_Next = ADDR2OFFSET (base, &l->u.tail.lop_TailNode);
	OFFSET2NODE (base, (n->n_Prev = l->lop_Tail))->n_Next = nodeoff;
	l->lop_Tail = nodeoff;
}

void
RemNode_OP (void *base, struct Node_OP *n)
{
	OFFSET2NODE (base, (OFFSET2NODE (base,
					 n->n_Next)->
			    n_Prev = n->n_Prev))->
	 n_Next = n->n_Next;
	n->n_Next = n->n_Prev = ADDR2OFFSET (base, n);	/* Thanks, Talin... */
}

struct Node_OP *
RemHead_OP (void * base, struct List_OP *l)
{
	Node_OP	*n, *nxt;

	if (nxt = NEXTNODE_OP (base, n = FIRSTNODE_OP (base, l))) {
		l->lop_Head = ADDR2OFFSET (base, nxt);
		nxt->n_Prev = ADDR2OFFSET (base, &l->u.head.lop_HeadNode);
		n->n_Next = n->n_Prev = ADDR2OFFSET (base, n);
		nxt = n;
	}
	return (nxt);
}

struct Node_OP *
RemTail_OP (void *base, struct List_OP *l)
{
	Node_OP	*n, *prv;

	if (prv = PREVNODE_OP (base, n = LASTNODE_OP (base, l))) {
		l->lop_Tail = ADDR2OFFSET (base, prv);
		prv->n_Next = ADDR2OFFSET (base, &l->u.tail.lop_TailNode);
		n->n_Next = n->n_Prev = ADDR2OFFSET (base, n);
		prv = n;
	}
	return (prv);
}

void
InsertNodeBefore_OP (void *base, struct Node_OP *n, struct Node_OP *radix)
{
	uint32	noff;

	noff = ADDR2OFFSET (base, n);

	n->n_Next = ADDR2OFFSET (base, radix);
	OFFSET2NODE (base, n->n_Prev = radix->n_Prev)->n_Next = noff;
	radix->n_Prev = noff;
}

void
InsertNodeAfter_OP (void *base, struct Node_OP *n, struct Node_OP *radix)
{
	uint32	noff;

	noff = ADDR2OFFSET (base, n);

	n->n_Prev = ADDR2OFFSET (base, radix);
	OFFSET2NODE (base, n->n_Next = radix->n_Next)->n_Prev = noff;
	radix->n_Next = noff;
}

/*
 * Alas, these must be implemented as functions, as the macro equivalent would
 * have side-effects.
 */
struct Node_OP *
NextNode_OP (void *base, struct Node_OP *n)
{
	uint32	nxtoff;
	
	if (nxtoff = n->n_Next)
		return (OFFSET2ADDR (base, nxtoff));
	else
		return (NULL);
}

struct Node_OP *
PrevNode_OP (void *base, struct Node_OP *n)
{
	uint32	nxtoff;
	
	if (nxtoff = n->n_Prev)
		return (OFFSET2ADDR (base, nxtoff));
	else
		return (NULL);
}
