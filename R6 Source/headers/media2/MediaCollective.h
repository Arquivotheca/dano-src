/***************************************************************************
//
//	File:			media2/MediaCollective.h
//
//	Description:	Presents a group of BMediaNodes as a single node,
//					exposing selected endpoints.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIACOLLECTIVE_H_
#define _MEDIA2_MEDIACOLLECTIVE_H_

#include <support2/String.h>

#include <media2/MediaNode.h>
#include <media2/MediaDefs.h>

#include <media2/IMediaCollective.h>

namespace B {
namespace Media2 {

using namespace Support2;

class LMediaCollective : public LInterface<IMediaCollective>
{
public:
	virtual	status_t				Called(BValue &in, const BValue &outBindings, BValue &out);
};

class BMediaCollective : public LMediaCollective, public LMediaNode
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BMediaCollective)

									BMediaCollective();
									BMediaCollective(const char * name);

	virtual	BValue					Inspect(const BValue &which, uint32 flags = 0);

			void					SetName(const char * name);

	virtual	status_t				AddNode(IMediaNode::arg node);
	virtual	status_t				RemoveNode(IMediaNode::arg node);
			IMediaNode::ptr			NodeAt(int32 index) const;
			int32					IndexOf(IMediaNode::arg node);
			int32					CountNodes() const;

	// you can only expose endpoints belonging to current child nodes
	virtual	status_t				ShowEndpoint(IMediaEndpoint::arg endpoint);
	virtual	status_t				HideEndpoint(IMediaEndpoint::arg endpoint);
	virtual	bool					IsVisible(IMediaEndpoint::arg endpoint);

	// ** IMediaCollective

	virtual	ssize_t					ListNodes(BMediaNodeVector * outNodes) const;
	
	// ** IMediaNode

	virtual	BString					Name() const;
	virtual	IMediaCollective::ptr	Parent() const;
	virtual	ssize_t					ListEndpoints(
										BMediaEndpointVector * outEndpoints,
										int32 type = B_ANY_ENDPOINT_TYPE,
										int32 state = B_ANY_ENDPOINT_STATE) const;
	virtual	ssize_t					ListLinkedEndpoints(
										IMediaEndpoint::arg fromEndpoint,
										BMediaEndpointVector * outEndpoints,
										int32 state = B_ANY_ENDPOINT_STATE) const;

protected:
	virtual							~BMediaCollective();

	virtual	status_t				Acquired(const void* id);
	virtual	status_t				Released(const void* id);

private:

	virtual	status_t				SetParent(IMediaCollective::arg parent);

			int32					_FindNode(IMediaNode::arg node);
			int32					_FindVisible(IMediaEndpoint::arg endpoint);

	mutable	BLocker					mLock;
			BString					mName;
			IMediaCollective::ref	mParent;
			BMediaNodeVector		mNodes;
			BMediaEndpointVector	mVisible;
};

}; }; // namespace B::Media2
#endif //_MEDIA2_MEDIACOLLECTIVE_H_
