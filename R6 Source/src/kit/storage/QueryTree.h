/* ++++++++++
	FILE:	QueryTree.h
	REVS:	$Revision$
	NAME:	$Author$
	DATE:	$Date$
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _BQUERYTREE_H
#define _BQUERYTREE_H

#define MAX_QUERY_STRLEN	1024

#define PARSE_ERROR		-10
#define LENGTH_ERROR	-20

class BQueryStack;
class BQueryElem;

class BQueryTree {
public:
	BQueryTree();
	~BQueryTree();
	int		query_error;
	char	*queryString;
	void	ConvertToQueryString();
	void	ConvertStackToTree(BQueryStack *s);

private:

	class treeNode {
	public:
		treeNode();
		~treeNode();

		BQueryElem	*elem;
		treeNode	*left;
		treeNode	*right;
	};

	void deleteTree(treeNode *node);

	treeNode	*root;
	treeNode	*toTree(BQueryStack *s);

	void		inorderTraverse(treeNode *node);

	int			queryStrlen;
	inline int	addToQueryString(const char c);
	int			addToQueryString(const char *str);
	int			addToQueryString(const BQueryElem *elem);
};


#endif /* _BQUERYTREE_H */
