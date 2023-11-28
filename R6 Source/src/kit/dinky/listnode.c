/* :ts=8 bk=0
 *
 * listnode.c:	Routines for manipulating doubly-linked lists.
 *
 * Leo L. Schwab					1999.01.04
 */
#include <dinky/listnode.h>


void
InitList (struct List *l)
{
	l->l_Head = &l->u.tail.l_TailNode;
	l->l_Tail = &l->u.head.l_HeadNode;
	l->l__null = NULL;
}

void
AddHead (struct Node *n, struct List *l)
{
	n->n_Prev = &l->u.head.l_HeadNode;
	(n->n_Next = l->l_Head)->n_Prev = n;
	l->l_Head = n;
}

void
AddTail (struct Node *n, struct List *l)
{
	n->n_Next = &l->u.tail.l_TailNode;
	(n->n_Prev = l->l_Tail)->n_Next = n;
	l->l_Tail = n;
}

void
RemNode (struct Node *n)
{
	(n->n_Next->n_Prev = n->n_Prev)->n_Next = n->n_Next;
	n->n_Next = n->n_Prev = n;	/*  Thanks, Talin...  */
}

struct Node *
RemHead (struct List *l)
{
	Node	*n, *nxt;

	if (nxt = NEXTNODE (n = FIRSTNODE (l))) {
		(l->l_Head = nxt)->n_Prev = &l->u.head.l_HeadNode;
		n->n_Next = n->n_Prev = n;
		nxt = n;
	}
	return (nxt);
}

struct Node *
RemTail (struct List *l)
{
	Node	*n, *prv;

	if (prv = PREVNODE (n = LASTNODE (l))) {
		(l->l_Tail = prv)->n_Next = &l->u.tail.l_TailNode;
		n->n_Next = n->n_Prev = n;
		prv = n;
	}
	return (prv);
}

void
InsertNodeBefore (struct Node *n, struct Node *radix)
{
	n->n_Next = radix;
	(n->n_Prev = radix->n_Prev)->n_Next = n;
	radix->n_Prev = n;
}

void
InsertNodeAfter (struct Node *n, struct Node *radix)
{
	n->n_Prev = radix;
	(n->n_Next = radix->n_Next)->n_Prev = n;
	radix->n_Next = n;
}
