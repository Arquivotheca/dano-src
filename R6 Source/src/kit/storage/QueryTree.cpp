/* ++++++++++
	FILE:	QueryTree.cpp
	REVS:	$Revision: 1.2 $
	NAME:	$Author: steve $
	DATE:	$Date: 1997/06/11 00:32:21 $
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <Debug.h>
#include <String.h>
#include <SupportDefs.h>
#include <Query.h>

#include "QueryTree.h"
#include "QueryStack.h"


static inline const char 	*queryOpToString(query_op op);

BQueryTree::BQueryTree()
{
	root = NULL;
	query_error = 0;
	queryString = NULL;
	queryStrlen = 0;
}

BQueryTree::~BQueryTree()
{
	deleteTree(root);
}

BQueryTree::treeNode::treeNode()
{
	elem = 		NULL;
	left = 		NULL;
	right = 	NULL;
}

BQueryTree::treeNode::~treeNode()
{
	delete elem;
}

/*
 * delete the tree rooted at node
 */
void
BQueryTree::deleteTree(treeNode *node)
{
	if (node == NULL)
		return;

	deleteTree(node->left);
	deleteTree(node->right);
	delete node;
}


void
BQueryTree::ConvertStackToTree(BQueryStack *s)
{
	root = toTree(s);
	if (!s->is_empty()) {
		query_error = PARSE_ERROR;
	}
}


BQueryTree::treeNode *
BQueryTree::toTree(BQueryStack *s)
{
	treeNode	*node;

	if (s->is_empty()) {
		query_error = PARSE_ERROR;
		return NULL;
	}
	
	node = new treeNode;
	node->elem = s->pop();
	if (node->elem->is_binary_operator()) {
		node->right = toTree(s);
		node->left = toTree(s);
		return node;
	} else if (node->elem->is_unary_operator()) {
		node->right = toTree(s);
		return node;
	} else if (node->elem->is_operand()) {
		return node;
	} 

	TRESPASS();
	return NULL;
}

/*
 * inorder traversal, inserting parentheses as necessary
 */
void
BQueryTree::ConvertToQueryString()
{
	queryString = (char *) calloc(sizeof(char), MAX_QUERY_STRLEN + 1);
	inorderTraverse(root);
}


inline int
BQueryTree::addToQueryString(const char c)
{
	if (queryStrlen+1 > MAX_QUERY_STRLEN) {
		query_error = LENGTH_ERROR;
		return -1;
	}

	queryString[queryStrlen++] = c;
	return 0;
}

int
BQueryTree::addToQueryString(const char *str)
{
	int len = strlen(str);

	if (queryStrlen + len > MAX_QUERY_STRLEN) {
		query_error = LENGTH_ERROR;
		return -1;
	}

	strcat(queryString, str);
	queryStrlen += len;
	return 0;
}

int
BQueryTree::addToQueryString(const BQueryElem *elem)
{
	char tempstr[50];
	int ret;
	
	switch (elem->dataType) {
	case ATTR:
		return addToQueryString(elem->data.attr);

	case UINT32:
		sprintf(tempstr, "%lu", elem->data.unsigned_int32);
		return addToQueryString(tempstr);
		
	case INT32:
		sprintf(tempstr, "%ld", elem->data.signed_int32);
		return addToQueryString(tempstr);

	case UINT64:
		sprintf(tempstr, "%Lu", elem->data.unsigned_int64);
		return addToQueryString(tempstr);

	case INT64:
		sprintf(tempstr, "%Ld", elem->data.unsigned_int64);
		return addToQueryString(tempstr);

	/*
	 * Pass floats and doubles using their binary representation
	 * because otherwise we introduce error by converting them
	 * to decimal strings and then back to float.
	 */
	case FLOAT:
		sprintf(tempstr, "0x%lx", *(uint32 *) &elem->data.f);
		return addToQueryString(tempstr);

	case DOUBLE:
		sprintf(tempstr, "0x%Lx", *(uint64 *) &elem->data.d);
		return addToQueryString(tempstr);

	case STRING:
		{
			BString tmp;
			ret = addToQueryString('"');
			tmp.CharacterEscape(elem->data.s, "\"\\\'", '\\');
			ret = addToQueryString(tmp.String());
			ret = addToQueryString('"');
		}
		return ret;
		
	case DATE:
		{
			BString tmp;
			tmp.CharacterEscape(elem->data.s, "%\"\\\'", '\\');
			ret = addToQueryString('%');
			ret = addToQueryString(tmp.String());
			ret = addToQueryString('%');
		}
		return ret;
		
	case QUERY_OP:
		return addToQueryString(queryOpToString(elem->data.op));

	default:
		TRESPASS();
		return -1;
	}
}

void
BQueryTree::inorderTraverse(treeNode *node)
{
	if (node == NULL)
		return;

	if (node->left || node->right) {
		if (addToQueryString('(') < 0)
			return;

		inorderTraverse(node->left);

		if (addToQueryString(node->elem) < 0)
			return;
		
		inorderTraverse(node->right);

		if (addToQueryString(')') < 0)
			return;
	} else {
		addToQueryString(node->elem);
	}
}

static inline const char *
queryOpToString(query_op op)
{
	switch (op) {
	case B_EQ:
	case B_CONTAINS:
	case B_ENDS_WITH:
	case B_BEGINS_WITH:
		return "==";
	case B_GT:
		return ">";
	case B_GE:
		return ">=";
	case B_LT:
		return "<";
	case B_LE:
		return "<=";
	case B_NE:
		return "!=";
	case B_AND:
		return "&&";
	case B_OR:
		return "||";
	case B_NOT:
		return "!";
	default:
		return NULL;
	}
}


