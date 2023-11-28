/* :ts=8 bk=0
 *
 * listnode_rp.c:	Routines for manipulating doubly-linked lists, using
 *			relative pointers, rather than absolute pointers.
 *
 * Leo L. Schwab					2000.04.07
 */
#include <dinky/listnode_rp.h>


#define	RELATIVE2NODE(base,node)	((Node_RP *) RELATIVE2ADDR ((base), \
								    (node)))


void
InitList_RP (struct List_RP *l)
{
	l->lrp_Head = ADDR2RELATIVE (&l->u.head.lrp_HeadNode,
				     &l->u.tail.lrp_TailNode);
	l->lrp_Tail = ADDR2RELATIVE (&l->u.tail.lrp_TailNode,
				     &l->u.head.lrp_HeadNode);
	l->lrp__null = 0;
}

void
AddHead_RP (struct Node_RP *new, struct List_RP *l)
{
	Node_RP	*n;
	int32	next, prev;	/*  Measured from 'new'  */

	next = ADDR2RELATIVE (new, n = FIRSTNODE_RP (l));
	prev = ADDR2RELATIVE (new, &l->u.head.lrp_HeadNode);

	new->n_Next = next;
	new->n_Prev = prev;
	n->n_Prev = -next;
	l->lrp_Head = -prev;
}

void
AddTail_RP (struct Node_RP *new, struct List_RP *l)
{
	Node_RP	*n;
	int32	next, prev;	/*  Measured from 'new'  */

	next = ADDR2RELATIVE (new, &l->u.tail.lrp_TailNode);
	prev = ADDR2RELATIVE (new, n = LASTNODE_RP (l));

	new->n_Next = next;
	new->n_Prev = prev;
	l->lrp_Tail = -next;
	n->n_Next = -prev;
}

void
RemNode_RP (struct Node_RP *n)
{
	Node_RP	*nxt, *prv;
	int32	dist;

	nxt = RELATIVE2NODE (n, n->n_Next);
	prv = RELATIVE2NODE (n, n->n_Prev);
	dist = ADDR2RELATIVE (prv, nxt);
	prv->n_Next = dist;
	nxt->n_Prev = -dist;
	n->n_Next = n->n_Prev = 0;	/*  Thanks, Talin...  */
}

struct Node_RP *
RemHead_RP (struct List_RP *l)
{
	Node_RP	*n, *nxt;

	if (nxt = NEXTNODE_RP (n = FIRSTNODE_RP (l))) {
		int32	dist;

		dist = ADDR2RELATIVE (&l->u.head.lrp_HeadNode, nxt);
		l->lrp_Head = dist;
		nxt->n_Prev = -dist;
		n->n_Next = n->n_Prev = 0;
		nxt = n;
	}
	return (nxt);
}

struct Node_RP *
RemTail_RP (struct List_RP *l)
{
	Node_RP	*n, *prv;

	if (prv = PREVNODE_RP (n = LASTNODE_RP (l))) {
		int32	dist;

		dist = ADDR2RELATIVE (&l->u.tail.lrp_TailNode, prv);
		l->lrp_Tail = dist;
		prv->n_Next = -dist;
		n->n_Next = n->n_Prev = 0;
		prv = n;
	}
	return (prv);
}

void
InsertNodeBefore_RP (struct Node_RP *new, struct Node_RP *radix)
{
	Node_RP	*n;
	int32	next, prev;	/*  Measured from 'new'  */

	next = ADDR2RELATIVE (new, radix);
	prev = ADDR2RELATIVE (new, n = PREVNODE_RP (radix));

	new->n_Next = next;
	new->n_Prev = prev;
	radix->n_Prev = -next;
	n->n_Next = -prev;
}

void
InsertNodeAfter_RP (struct Node_RP *new, struct Node_RP *radix)
{
	Node_RP	*n;
	int32	next, prev;	/*  Measured from 'new'  */

	next = ADDR2RELATIVE (new, n = NEXTNODE_RP (radix));
	prev = ADDR2RELATIVE (new, radix);

	new->n_Next = next;
	new->n_Prev = prev;
	n->n_Prev = -next;
	radix->n_Next = -prev;
}
