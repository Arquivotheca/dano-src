/***************************************************************************
//
//	File:			media2/MediaEndpointGraph.h
//
//	Description:	Represents a connected set of endpoints as a
//					directed graph.  Provides thread-safe access
//					to those endpoints in various orders.
//					NOT A VIRTUAL CLASS
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIAENDPOINTGRAPH_H_
#define _MEDIA2_MEDIAENDPOINTGRAPH_H_

#include <media2/IMediaEndpoint.h>

namespace B {

namespace Media2 {

using namespace B::Support2;

class BMediaEndpointGraph
{
public:
	enum visit_order_t
	{
		UNORDERED, // will list all endpoints, even those unreachable from the root
		BREADTH_FIRST,
		DEPTH_FIRST
	};

	typedef	void (*acquire_hook)(IMediaOutput::arg output, void * context);
	typedef	void (*release_hook)(IMediaOutput::arg output, void * context);
	typedef void (*visit_hook)	(IMediaEndpoint::arg endpoint, void * context);

						BMediaEndpointGraph();
						BMediaEndpointGraph(const BMediaEndpointGraph & clone);
						~BMediaEndpointGraph();
								
	BMediaEndpointGraph & operator=(const BMediaEndpointGraph & clone);

	// returns the root endpoint, or 0 if the graph is empty.
	IMediaEndpoint::ptr	Root() const;

	// allows you to manually set the root; the endpoint must
	// currently exist
	status_t			SetRoot(IMediaEndpoint::arg endpoint);

	// describe an output->input connection
	// if empty, then 'output' will become the new root
	// may result in calls to the acquire hook
	status_t			AddConnection(IMediaOutput::arg output, IMediaInput::arg input);
	
	// +++ always prune unreachable endpoints? +++
	// may result in calls to the release hook
	status_t			RemoveConnection(IMediaOutput::arg output, IMediaInput::arg input);

	// describe a node-internal connection (the input and output must belong to the same node.)
	// may result in calls to the acquire hook
	status_t			AddLink(IMediaInput::arg input, IMediaOutput::arg output);

	// may result in calls to the release hook
	status_t			RemoveLink(IMediaInput::arg input, IMediaOutput::arg output);

	// provide a hook to be called when an output is added to the graph.
	// the BMediaEndpointGraph will be locked when your hook is called.
	status_t			SetAcquireHook(acquire_hook hook,
									   void * context);

	// provide a hook to be called when an output is removed from the graph.
	// the BMediaEndpointGraph will be locked when your hook is called.
	status_t			SetReleaseHook(release_hook hook,
									   void * context);

	// call the given hook for each endpoint reachable from the root.
	// the BMediaEndpointGraph will be locked when your hook is called.
	// returns the number of endpoints visited.
	ssize_t				Visit(visit_hook hook,
							  void * context,
							  visit_order_t order = BREADTH_FIRST);

	status_t			ListEndpoints(BMediaEndpointVector * outEndpoints,
									  visit_order_t order = BREADTH_FIRST);
	
	// remove endpoints that are unreachable from the root
	status_t			PruneUnreachable();
	
	// tabula rasa
	void				MakeEmpty();

	// yes, the lock can be multiply acquired
	lock_status_t		Lock();
	void				Unlock();
	
private:
	class	impl;
			impl *		mImpl;
			
	static	void		do_list_endpoints(IMediaEndpoint::arg endpoint, void * context);
	
			ssize_t		visit_unordered(visit_hook hook, void * context);
			ssize_t		visit_breadth_first(visit_hook hook, void * context);
			ssize_t		visit_depth_first(visit_hook hook, void * context);
			
			impl *		clone() const;
};

} } // B::Media2
#endif //_MEDIA2_MEDIAENDPOINTGRAPH_H_
