/* ++++++++++
	FILE:	QueryStack.cpp
	REVS:	$Revision$
	NAME:	$Author$
	DATE:	$Date$
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <SupportDefs.h>
#include <Query.h>

#include "QueryStack.h"
#include "QueryTree.h"

BQueryStack::BQueryStack()
{
	topStackNode = NULL;
}

BQueryStack::~BQueryStack()
{
	clear();
}

void
BQueryStack::clear()
{
	while (pop() != NULL)
		;
}

bool
BQueryStack::is_empty() const
{
	return (topStackNode == NULL);
}

void
BQueryStack::push(BQueryElem *elem)
{
	stackNode *newNode;

	newNode = new stackNode;

	newNode->elem = elem;
	newNode->next = topStackNode;
	topStackNode = newNode;
}


BQueryElem *
BQueryStack::pop()
{
	stackNode *node;
	BQueryElem *elem;

	if (topStackNode == NULL)
		return NULL;
	
	node = topStackNode;
	topStackNode = topStackNode->next;
	elem = node->elem;
	delete node;
	return elem;
}

BQueryElem *
BQueryStack::top() const
{
	if (topStackNode == NULL) {
		return NULL;
	}

	return topStackNode->elem;
}


char *
BQueryStack::convertStackToString()
{
	BQueryTree tree;

	tree.ConvertStackToTree(this);
	tree.ConvertToQueryString();

	if (tree.query_error) {
		free(tree.queryString);
		return NULL;
	}

	return tree.queryString;
}


BQueryElem::BQueryElem()
{
	memset(&data, 0, sizeof(data));
	dataType = 0;
}

BQueryElem::~BQueryElem()
{
	if (dataType == ATTR) {
		free(data.attr);
	} else if (dataType == STRING || dataType == DATE) {
		free(data.s);
	}
}
