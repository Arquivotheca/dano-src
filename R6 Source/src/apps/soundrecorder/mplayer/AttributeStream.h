//
//	Copyright 1997, 1998, Be Incorporated, All Rights Reserved.
//
//	attribute streams allow copying/filtering/transformation of attributes
//	between file and/or memory nodes
//
//	for example one can use constructs of nodes like:
//
// 	destinationNode << transformer << buffer << filter << sourceNode
//
//	transformer may for instance perform endian-swapping or offsetting of a B_RECT attribute
//	filter may withold certain attributes
//	buffer is a memory allocated snapshot of attributes, may be repeatedly streamed into
//	other files, buffers
//
//	In addition to the whacky (but usefull) << syntax, calls like Read, Write are also
//	available


#ifndef __ATTRIBUTE_STREAM__
#define __ATTRIBUTE_STREAM__

#include <Node.h>
#include <String.h>

#include <fs_attr.h>

#include "ObjectList.h"

struct AttributeTemplate {
	// used for read-only attribute source
	const char *attributeName;
	uint32 attributeType;
	off_t size;
	const char *bits;
};


class AttributeInfo {
	// utility class for internal attribute description
public:
	AttributeInfo()
		{}
	AttributeInfo(const AttributeInfo &);
	AttributeInfo(const char *, attr_info);
	AttributeInfo(const char *, uint32, off_t);
	
	void SetTo(const AttributeInfo &);
	void SetTo(const char *, attr_info);
	void SetTo(const char *, uint32, off_t);
	const char *Name() const;
	uint32 Type() const;
	off_t Size() const;

private:
	BString name;
	attr_info info;
};	
	
class AttributeStreamNode {
public:
	AttributeStreamNode();
	virtual ~AttributeStreamNode();
	
	AttributeStreamNode &operator<<(AttributeStreamNode &source);
		// workhorse call
		// to the outside makes this node a part of the stream, passing on
		// any data it has, gets, transforms, doesn't filter out
		//
		// under the hood sets up streaming into the next node; hooking
		// up source and destination, forces the stream head to start streaming
	
	virtual void Rewind();
		// get ready to start all over again
	virtual void MakeEmpty() {}
		// remove any attributes the node may have
	
	virtual off_t Contains(const char *, uint32);
		// returns size of attribute if found

	virtual off_t Read(const char *name, const char *foreignName, uint32 type, off_t size,
		void *buffer, void (*swapFunc)(void *) = 0);
		// read from this node
	virtual off_t Write(const char *name, const char *foreignName, uint32 type, off_t size,
		const void *buffer);
		// write to this node


	// work calls
	
	virtual bool Drive();
		// node at the head of the stream makes the entire stream
		// feed it
	virtual const AttributeInfo *Next();
		// give me the next attribute in the stream
	virtual const char *Get();
		// give me the data of the attribute in the stream that was just returned
		// by Next
		// assumes there is a buffering node somewhere on the way to
		// the source, from which the resulting buffer is borrowed
	virtual bool Fill(char *buffer) const;
		// fill the buffer with data of the attribute in the stream that was just returned
		// by next
		// <buffer> is big enough to hold the entire attribute data

	virtual bool CanFeed() const { return false; }
		// return true if can work as a source for the entire stream

private:
	bool Start();
		// utility call, used to start up the stream by finding the ultimate
		// target of the stream and calling Drive on it

	void Detach();

protected:
	AttributeStreamNode *readFrom;
	AttributeStreamNode *writeTo;
};

class AttributeStreamFileNode : public AttributeStreamNode {
	// handles reading and writing attributes to and from the
	// stream
public:
	AttributeStreamFileNode();
	AttributeStreamFileNode(BNode *);
	
	virtual void MakeEmpty();
	virtual void Rewind();
	virtual off_t Contains(const char *name, uint32 type);
	virtual off_t Read(const char *name, const char *foreignName, uint32 type, off_t size,
		void *buffer, void (*swapFunc)(void *) = 0);
	virtual off_t Write(const char *name, const char *foreignName, uint32 type, off_t size,
		const void *buffer);

	void SetTo(BNode *);
	
protected:
	virtual bool CanFeed() const { return true; }

	virtual bool Drive();
		// give me all the attributes, I'll write them into myself
	virtual const AttributeInfo *Next();
		// return the info for the next attribute I can read for you
	virtual const char *Get();
	virtual bool Fill(char *buffer) const;

private:
	AttributeInfo currentAttr;

	typedef AttributeStreamNode _inherited;
	BNode *node;
};

class AttributeStreamMemoryNode : public AttributeStreamNode {
	// in memory attribute buffer; can be both target of writing and source
	// of reading at the same time
public:
	AttributeStreamMemoryNode();

	virtual void MakeEmpty();
	virtual off_t Contains(const char *name, uint32 type);
	virtual off_t Read(const char *name, const char *foreignName, uint32 type, off_t size,
		void *buffer, void (*swapFunc)(void *) = 0);
	virtual off_t Write(const char *name, const char *foreignName, uint32 type, off_t size,
		const void *buffer);
		
protected:

	virtual bool CanFeed() const { return true; }
	virtual void Rewind();
	virtual bool Drive();
	virtual const AttributeInfo *Next();
	virtual const char *Get();
	virtual bool Fill(char *buffer) const;

	class AttrNode {
	public:
		AttrNode(const char *name, uint32 type, off_t size, char *data)
			:	attr(name, type, size),
				data(data)
			{
			}

		~AttrNode()
			{
				delete data;
			}
			
		AttributeInfo attr;
		char *data;
	};
	
		// utility calls
	virtual AttrNode *BufferingGet();
	virtual AttrNode *BufferingGet(const char *name, uint32 type, off_t size);
	int32 Find(const char *name, uint32 type) const;

private:

	BObjectList<AttrNode> attributes;
	int32 currentIndex;

	typedef AttributeStreamNode _inherited;
};

class AttributeStreamTemplateNode : public AttributeStreamNode {
	// in read-only memory attribute source
	// can only be used as a source for Next and Get
public:
	AttributeStreamTemplateNode(const AttributeTemplate *, int32 count);

	virtual off_t Contains(const char *name, uint32 type);
		
protected:

	virtual bool CanFeed() const { return true; }
	virtual void Rewind();
	virtual const AttributeInfo *Next();
	virtual const char *Get();
	virtual bool Fill(char *buffer) const;

	int32 Find(const char *name, uint32 type) const;

private:
	AttributeInfo currentAttr;
	const AttributeTemplate *attributes;
	int32 currentIndex;
	int32 count;

	typedef AttributeStreamNode _inherited;
};

class AttributeStreamFilterNode : public AttributeStreamNode {
	// filter node may not pass thru specified attributes
public:
	AttributeStreamFilterNode()
		{}
	virtual off_t Contains(const char *name, uint32 type);
	virtual off_t Read(const char *name, const char *foreignName, uint32 type, off_t size,
		void *buffer, void (*swapFunc)(void *) = 0);
	virtual off_t Write(const char *name, const char *foreignName, uint32 type, off_t size,
		const void *buffer);

protected:
	virtual bool Reject(const char *name, uint32 type, off_t size);
		// override to implement filtering
	virtual const AttributeInfo *Next();
	
private:
	typedef AttributeStreamNode _inherited;
};

class NamesToAcceptAttrFilter : public AttributeStreamFilterNode {
	// filter node that only passes thru attributes that match
	// a list of names
public:
	NamesToAcceptAttrFilter(const char **);
protected:
	virtual bool Reject(const char *name, uint32 type, off_t size);
private:
	const char **nameList;
};

class SelectiveAttributeTransformer : public AttributeStreamNode {
	// node applies a transformation on specified attributes
public:
	SelectiveAttributeTransformer(char *attributeName, bool (*)(const char *,
		uint32 , off_t , void *, void *), void *params);
	virtual ~SelectiveAttributeTransformer();

	virtual off_t Read(const char *name, const char *foreignName, uint32 type, off_t size,
		void *buffer, void (*swapFunc)(void *) = 0);

	virtual void Rewind();

protected:
	virtual bool WillTransform(const char *name, uint32 type, off_t size, const char *data) const;
		// override to implement filtering; should only return true if transformation will
		// occur
	virtual char *CopyAndApplyTransformer(const char *name, uint32 type, off_t size, const char *data);
		// makes a copy of data
	virtual bool ApplyTransformer(const char *name, uint32 type, off_t size, char *data);
		// transforms in place
	virtual const AttributeInfo *Next();
	virtual const char *Get();
	
private:
	AttributeInfo currentAttr;
	const char *attributeNameToTransform;
	bool (*transformFunc)(const char *, uint32 , off_t , void *, void *);
	void *transformParams;

	BObjectList<char> transformedBuffers;

	typedef AttributeStreamNode _inherited;
};

#endif
