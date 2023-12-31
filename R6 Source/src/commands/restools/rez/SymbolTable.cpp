/*	$Id: SymbolTable.cpp,v 1.3 1999/02/03 08:39:42 maarten Exp $
	
	Copyright 1996, 1997, 1998
	        Hekkelman Programmatuur B.V.  All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.
	3. All advertising materials mentioning features or use of this software
	   must display the following acknowledgement:
	   
	    This product includes software developed by Hekkelman Programmatuur B.V.
	
	4. The name of Hekkelman Programmatuur B.V. may not be used to endorse or
	   promote products derived from this software without specific prior
	   written permission.
	
	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
	FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
	AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
	OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 	

	Created: 12/02/98 15:38:51
*/

#include "SymbolTable.h"
#include <string.h>

struct Node {
	Node *left, *right;
	char *key;
	int value;
	
	Node(const char *key, int value);
};

static Node *root = NULL;
static int nextID = 1;

Node::Node(const char *k, int val)
{
	left = right = NULL;
	key = strdup(k);
	value = val;
} /* Node::Node */

int ST_AddIdent(const char *ident)
{
	int id = 0;
	
	if (!root)
		root = new Node(ident, id = nextID++);
	else
	{
		Node *k = root;
		
		while (id == 0)
		{
			int d = strcasecmp(k->key, ident);
			
			if (d < 0)
			{
				if (k->right)
					k = k->right;
				else
					k->right = new Node(ident, id = nextID++);
			}
			else if (d > 0)
			{
				if (k->left)
					k = k->left;
				else
					k->left = new Node(ident, id = nextID++);
			}
			else
				id = k->value;
		}
	}
	
	return id;
} /* ST_AddIdent */

static Node* ST_FindValue(Node *n, int nr)
{
	Node *r = NULL;
	
	if (n->value == nr)
		return n;
		
	if (n->left)
		r = ST_FindValue(n->left, nr);
	
	if (!r && n->right)
		r = ST_FindValue(n->right, nr);
	
	return r;
} /* ST_FindValue */

char* ST_Ident(int nr)
{
	Node *n = ST_FindValue(root, nr);
	
	return n ? n->key : NULL;
} /* ST_Ident */
