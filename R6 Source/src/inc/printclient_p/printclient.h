/*
 * printclient.h
 * Copyright (c) 1997 Be, Inc.	All Rights Reserved 
 *
 * Private API (for now) to deal with network printing (generically)
 *
 */
#ifndef _PRINTCLIENT_H
#define _PRINTCLIENT_H
#include <BeBuild.h>

typedef enum _NodeType {
	NODETYPE_ROOT,
	NODETYPE_CLASS,
	NODETYPE_DIRECTORY,
	NODETYPE_ENDPOINT
} NodeType;

typedef bool (*print_callback_t)(void *data, unsigned bytes);

class _PrinterNode {
public:
	virtual NodeType Type(void) = 0;
	virtual int List(_PrinterNode **nodes, unsigned maxnodes) = 0;
	virtual int Name(char *base, unsigned maxlen) = 0;
	virtual void SetParent(_PrinterNode *node) = 0;
	virtual int Print(int fd, print_callback_t callback, void *data) = 0;
	virtual void CancelPrint(void) = 0;
	virtual int Lookup(const char *name, _PrinterNode **node) = 0;
	virtual void Reference(void) = 0;
	virtual void Close(void) = 0;
	virtual _PrinterNode *Parent(void) = 0;
};
#define PrinterNode _PrinterNode

extern PrinterNode *OpenRoot(void);
extern PrinterNode *OpenByName(const char *name);
extern void GetFQName(PrinterNode *node, char *base, unsigned maxlen);	


#endif /* _PRINTCLIENT_H */
