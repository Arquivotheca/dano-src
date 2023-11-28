/* ++++++++++
	FILE:	QueryStack.h
	REVS:	$Revision: 1.2 $
	NAME:	$Author: mani $
	DATE:	$Date: 1997/06/10 18:56:19 $
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _BQUERYSTACK_H
#define _BQUERYSTACK_H

enum {
	ATTR = 1000,
	UINT32,
	INT32,
	UINT64,
	INT64,
	FLOAT,
	DOUBLE,
	STRING,
	QUERY_OP,
	DATE
};


class BQueryElem {
public:
	BQueryElem();
	~BQueryElem();
	
	union {
		char	*attr;				
		uint32	unsigned_int32;		
		int32	signed_int32;
		uint64	unsigned_int64;
		int64	signed_int64;
		float	f;
		double	d;
		char	*s;
		query_op op;				
	} data;
	int		dataType;


	inline bool is_operand() const {
		return (dataType == ATTR ||
				dataType == UINT32 ||
				dataType == INT32 ||
				dataType == INT64 ||
				dataType == FLOAT ||
				dataType == DOUBLE ||
				dataType == DATE ||
				dataType == STRING);
	}

	inline bool is_unary_operator() const {
		return ((dataType == QUERY_OP) &&
				(data.op == B_NOT));
	}

	inline bool is_binary_operator() const {
		return ((dataType == QUERY_OP) &&
				(data.op == B_EQ	||
				 data.op == B_GT	||
				 data.op == B_GE	||
				 data.op == B_LT	||
				 data.op == B_LE	||
				 data.op == B_NE	||
				 data.op == B_CONTAINS		||
				 data.op == B_BEGINS_WITH 	||
				 data.op == B_ENDS_WITH		||
				 data.op == B_AND 	||
				 data.op == B_OR));
	}
};

class BQueryStack {
public:
	BQueryStack();
	~BQueryStack();
	bool 		is_empty() const;
	void		push(BQueryElem *elem);
	BQueryElem	*pop();
	BQueryElem	*top() const;
	char		*convertStackToString();
	void		clear();

private:
	struct stackNode {
		BQueryElem			*elem;
		struct stackNode 	*next;
	};
	typedef struct stackNode stackNode;
	
	stackNode 	*topStackNode;
};


#endif /* _BQUERYSTACK_H */
