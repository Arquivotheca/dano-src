#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/Locker.h>
#include <support2/StdIO.h>
#include <support2/String.h>
#include <media2/MediaEndpointGraph.h>
#include <media2/IMediaNode.h>
#include "DumbVector.h"

#include <cstdlib>

#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Media2 {

using namespace B::Support2;
using B::Private::BDumbVector;

struct BMediaEndpointGraph::impl
{
	enum visit_state_t
	{
		VIRGIN,
		VISITED,
		EXHAUSTED
	};
	struct entry
	{
		IMediaEndpoint::ptr		endpoint;
		IMediaOutput::ptr		output;
		visit_state_t			visit_state;
		BDumbVector 			adjacent; // entry *

		entry(IMediaEndpoint::arg _endpoint, IMediaOutput::arg _output = 0)
		:	endpoint(_endpoint),
			output(_output),
			visit_state(VIRGIN)
		{
			ASSERT(endpoint != 0);
		}
	};
	enum link_type_t
	{
		EXTERNAL_LINK,
		INTERNAL_LINK
	};
	struct link
	{
		// +++ 
	};

	void make_empty()
	{
		for (int32 n = (int32)entries.CountItems()-1; n >= 0; n--)
		{
			entry * e = static_cast<entry*>(entries.EditItemAt(n));
			if (e && e->output != 0)
			{
				release_fn(e->output, release_context);
			}
			delete e;
		}
		entries.MakeEmpty();
	}
	status_t find_entry(IBinder::arg endpoint, entry ** out_entry, size_t * out_index = 0)
	{
		for (int32 n = (int32)entries.CountItems()-1; n >= 0; n--)
		{
			if (((entry*)entries[n])->endpoint->AsBinder() == endpoint)
			{
				*out_entry = static_cast<entry*>(entries.EditItemAt(n));
				if (out_index) *out_index = n;
				return B_OK;
			}
		}
		return B_BAD_VALUE;
	}
	status_t remove_entry(entry * ex)
	{
		for (int32 n = entries.CountItems()-1; n >= 0; n--)
		{
			const entry * _e = static_cast<const entry*>(entries.ItemAt(n));
			if (_e == ex)
			{
				// remove entry
				entries.RemoveItemsAt(n, 1);
				if (root == ex) root = 0;

				// remove all adjacency entries
				for (int32 n = ex->adjacent.CountItems()-1; n >= 0; n--)
				{
					entry * e = static_cast<entry*>(ex->adjacent.EditItemAt(n));
					for (int32 na = e->adjacent.CountItems()-1; na >= 0; na--)
					{
						entry * ea = static_cast<entry*>(e->adjacent.EditItemAt(na));
						if (ea == ex)
						{
							e->adjacent.RemoveItemsAt(na, 1);
							break;
						}
					}
				}
				
				// done
				return B_OK;
			}
		}
		// not found
		return B_BAD_VALUE;
	}
	status_t connect_entries(entry * output_e, entry * input_e)
	{
		// mark adjacent
		IBinder::ptr output_binder = output_e->endpoint->AsBinder();
		IBinder::ptr input_binder = input_e->endpoint->AsBinder();
		for (int32 n = output_e->adjacent.CountItems()-1; n >= 0; n--)
		{
			if (static_cast<const entry*>(output_e->adjacent[n])->endpoint->AsBinder() == input_binder)
			{
				// we already have an output->input mapping, thanks
				return B_NOT_ALLOWED;
			}
		}
		output_e->adjacent.AddItem(input_e);

		for (int32 n = input_e->adjacent.CountItems()-1; n >= 0; n--)
		{
			if (static_cast<const entry*>(output_e->adjacent[n])->endpoint->AsBinder() == output_binder)
			{
				debugger("connect_entries: partial (output->input) mapping");
				output_e->adjacent.RemoveItemsAt(input_e->adjacent.CountItems()-1, 1);
				return B_NOT_ALLOWED;
			}
		}
		input_e->adjacent.AddItem(output_e);
		return B_OK;
	}
	status_t disconnect_entries(entry * output_e, bool * out_output_empty, entry * input_e, bool * out_input_empty)
	{
checkpoint
		// remove adjacency mappings and empty entries
		IBinder::ptr output_binder = output_e->endpoint->AsBinder();
		IBinder::ptr input_binder = input_e->endpoint->AsBinder();
		*out_output_empty = false;
		*out_input_empty = false;
		impl::entry * input_e2 = 0;
		for (int32 n = output_e->adjacent.CountItems()-1; n >= 0; n--)
		{
			if (static_cast<const impl::entry*>(output_e->adjacent[n])->endpoint->AsBinder() == input_binder)
			{
				input_e2 = static_cast<impl::entry*>(output_e->adjacent.EditItemAt(n));
				if (input_e2 != input_e)
				{
					debugger("disconnect_entries: duplicate output->input mapping");
					return B_ERROR;
				}
				output_e->adjacent.RemoveItemsAt(n, 1);
				if (!output_e->adjacent.CountItems())
				{
					*out_output_empty = true;
				}
				break;
			}
		}
		if (!input_e2)
		{
			// no output->input mapping exists
checkpoint
			return B_BAD_VALUE;
		}

		for (int32 n = input_e->adjacent.CountItems()-1; n >= 0; n--)
		{
			if (static_cast<const entry*>(input_e->adjacent[n])->endpoint->AsBinder() == output_binder)
			{
				input_e->adjacent.RemoveItemsAt(n, 1);
				if (!input_e->adjacent.CountItems())
				{
					*out_input_empty = true;
				}
				break;
			}
		}
		return B_OK;
	}
	
	impl()
	:	lock("BMediaEndpointGraph::impl::lock"),
		root(0),
		acquire_fn(0),
		acquire_context(0),
		release_fn(0),
		release_context(0)
	{
	}
	~impl()
	{
		make_empty();
	}

	BNestedLocker					lock;
	BDumbVector						entries; // entry *
	
	entry *							root;
	acquire_hook					acquire_fn;
	void *							acquire_context;
	release_hook					release_fn;
	void *							release_context;
};

BMediaEndpointGraph::BMediaEndpointGraph()
	: mImpl(new impl)
{
}

BMediaEndpointGraph::BMediaEndpointGraph(const BMediaEndpointGraph & clone)
{
	mImpl = clone.clone();
}

BMediaEndpointGraph &
BMediaEndpointGraph::operator=(const BMediaEndpointGraph & clone)
{
	delete mImpl;
	mImpl = clone.clone();
	return *this;
}

BMediaEndpointGraph::~BMediaEndpointGraph()
{
	delete mImpl;
}

IMediaEndpoint::ptr 
BMediaEndpointGraph::Root() const
{
	BAutolock _l(mImpl->lock.Lock());
	return (mImpl->root) ? mImpl->root->endpoint : 0;
}

status_t 
BMediaEndpointGraph::SetRoot(IMediaEndpoint::arg endpoint)
{
	BAutolock _l(mImpl->lock.Lock());
	impl::entry * e = 0;
	status_t err = mImpl->find_entry(endpoint->AsBinder(), &e);
	if (err < B_OK) return B_BAD_VALUE;
	mImpl->root = e;
	return B_OK;
}

status_t 
BMediaEndpointGraph::AddConnection(IMediaOutput::arg output, IMediaInput::arg input)
{
	status_t err;

	IMediaEndpoint::ptr output_endpoint = IMediaEndpoint::AsInterface(output->AsBinder());
	IMediaEndpoint::ptr input_endpoint = IMediaEndpoint::AsInterface(input->AsBinder());
	IBinder::ptr output_binder = output_endpoint->AsBinder();
	IBinder::ptr input_binder = input_endpoint->AsBinder();

	bool output_added = false;
	acquire_hook acquire_fn = 0;
	void * acquire_context = 0;

	{
		BAutolock _l(mImpl->lock.Lock());
	
		// find/create entries
		impl::entry * output_e = 0;
		err = mImpl->find_entry(output_binder, &output_e);
		if (err < B_OK)
		{
			output_e = new impl::entry(output_endpoint, output);
			mImpl->entries.AddItem(output_e);
			if (!mImpl->root) mImpl->root = output_e;
			acquire_fn = mImpl->acquire_fn;
			acquire_context = mImpl->acquire_context;
			output_added = true;
		}

		impl::entry * input_e = 0;
		err = mImpl->find_entry(input_binder, &input_e);
		if (err < B_OK)
		{
			input_e = new impl::entry(input_endpoint);
			mImpl->entries.AddItem(input_e);
			if (!mImpl->root) mImpl->root = input_e;
		}

		err = mImpl->connect_entries(output_e, input_e);
		if (err < B_OK)
		{
berr << "BMediaEndpointGraph::AddConnection(): mImpl->connect_entries: " << strerror(err) << endl;
			return err;
		}

	}// BAutolock _l(mImpl->lock.Lock());
	
	if (output_added && acquire_fn)
	{
		// notify
		acquire_fn(output, acquire_context);
	}

	return B_OK;
}

status_t 
BMediaEndpointGraph::RemoveConnection(IMediaOutput::arg output, IMediaInput::arg input)
{
	status_t err;

	IMediaEndpoint::ptr output_endpoint = IMediaEndpoint::AsInterface(output->AsBinder());
	IMediaEndpoint::ptr input_endpoint = IMediaEndpoint::AsInterface(input->AsBinder());
	IBinder::ptr output_binder = output_endpoint->AsBinder();
	IBinder::ptr input_binder = input_endpoint->AsBinder();

	acquire_hook release_fn = 0;
	void * release_context = 0;

	impl::entry * output_e = 0;
	impl::entry * input_e = 0;

	{
		BAutolock _l(mImpl->lock.Lock());

		// look up entries
	
		err = mImpl->find_entry(output_binder, &output_e);
		if (err < B_OK)
		{
			// no such output
			return B_BAD_VALUE;
		}

		err = mImpl->find_entry(input_binder, &input_e);
		if (err < B_OK)
		{
			// no such input
			return B_BAD_VALUE;
		}

		bool remove_output, remove_input;
		err = mImpl->disconnect_entries(output_e, &remove_output, input_e, &remove_input);
		if (err < B_OK)
		{
berr << "BMediaEndpointGraph::RemoveConnection: disconnect_entries: " << strerror(err) << endl;
			return err;
		}
		
		if (remove_output)
		{
			err = mImpl->remove_entry(output_e);
			if (err < B_OK)
			{
berr << "BMediaEndpointGraph::RemoveConnection: remove_entry(output_e): " << strerror(err) << endl;
				return err;
			}
			delete output_e;
			output_e = 0;
			release_fn = mImpl->release_fn;
			release_context = mImpl->release_context;
		}

		if (remove_input)
		{
			err = mImpl->remove_entry(input_e);
			if (err < B_OK)
			{
berr << "BMediaEndpointGraph::RemoveConnection: remove_entry(input_e): " << strerror(err) << endl;
				return err;
			}
			delete input_e;
			input_e = 0;
		}
		
	}//BAutolock _l(mImpl->lock.Lock());

	if (!output_e && release_fn)
	{
		// notify
		release_fn(output, release_context);
	}
	return B_OK;
}

status_t 
BMediaEndpointGraph::AddLink(IMediaInput::arg input, IMediaOutput::arg output)
{
	status_t err;

	IMediaEndpoint::ptr output_endpoint = IMediaEndpoint::AsInterface(output->AsBinder());
	IMediaEndpoint::ptr input_endpoint = IMediaEndpoint::AsInterface(input->AsBinder());
	
	if (input_endpoint->Node()->AsBinder() != output_endpoint->Node()->AsBinder())
	{
		// links can only represent intranode connections
		return B_NOT_ALLOWED;
	}
	
	IBinder::ptr output_binder = output_endpoint->AsBinder();
	IBinder::ptr input_binder = input_endpoint->AsBinder();

	bool output_added = false;
	acquire_hook acquire_fn = 0;
	void * acquire_context = 0;

	{
		BAutolock _l(mImpl->lock.Lock());
	
		// find/create entries
		impl::entry * output_e = 0;
		err = mImpl->find_entry(output_binder, &output_e);
		if (err < B_OK)
		{
			output_e = new impl::entry(output_endpoint, output);
			mImpl->entries.AddItem(output_e);
			if (!mImpl->root) mImpl->root = output_e;
			acquire_fn = mImpl->acquire_fn;
			acquire_context = mImpl->acquire_context;
			output_added = true;
		}

		impl::entry * input_e = 0;
		err = mImpl->find_entry(input_binder, &input_e);
		if (err < B_OK)
		{
			input_e = new impl::entry(input_endpoint);
			mImpl->entries.AddItem(input_e);
			if (!mImpl->root) mImpl->root = input_e;
		}

		err = mImpl->connect_entries(output_e, input_e);
		if (err < B_OK)
		{
berr << "BMediaEndpointGraph::AddConnection(): mImpl->connect_entries: " << strerror(err) << endl;
			return err;
		}

	}// BAutolock _l(mImpl->lock.Lock());
	
	if (output_added && acquire_fn)
	{
		// notify
		acquire_fn(output, acquire_context);
	}

	return B_OK;
}

status_t 
BMediaEndpointGraph::RemoveLink(IMediaInput:: arg input, IMediaOutput:: arg output)
{
	status_t err;

	IMediaEndpoint::ptr output_endpoint = IMediaEndpoint::AsInterface(output->AsBinder());
	IMediaEndpoint::ptr input_endpoint = IMediaEndpoint::AsInterface(input->AsBinder());

	if (input_endpoint->Node()->AsBinder() != output_endpoint->Node()->AsBinder())
	{
		// links can only represent intranode connections
		return B_NOT_ALLOWED;
	}
	
	IBinder::ptr output_binder = output_endpoint->AsBinder();
	IBinder::ptr input_binder = input_endpoint->AsBinder();

	acquire_hook release_fn = 0;
	void * release_context = 0;

	impl::entry * output_e = 0;
	impl::entry * input_e = 0;

	{
		BAutolock _l(mImpl->lock.Lock());

		// look up entries
	
		err = mImpl->find_entry(output_binder, &output_e);
		if (err < B_OK)
		{
			// no such output
			return B_BAD_VALUE;
		}

		err = mImpl->find_entry(input_binder, &input_e);
		if (err < B_OK)
		{
			// no such input
			return B_BAD_VALUE;
		}

		bool remove_output, remove_input;
		err = mImpl->disconnect_entries(output_e, &remove_output, input_e, &remove_input);
		if (err < B_OK)
		{
berr << "BMediaEndpointGraph::RemoveLink: disconnect_entries: " << strerror(err) << endl;
			return err;
		}

		if (remove_output)
		{
			err = mImpl->remove_entry(output_e);
			if (err < B_OK)
			{
berr << "BMediaEndpointGraph::RemoveLink: remove_entry(output_e): " << strerror(err) << endl;
				return err;
			}
			delete output_e;
			output_e = 0;
			release_fn = mImpl->release_fn;
			release_context = mImpl->release_context;
		}

		if (remove_input)
		{
			err = mImpl->remove_entry(input_e);
			if (err < B_OK)
			{
berr << "BMediaEndpointGraph::RemoveLink: remove_entry(input_e): " << strerror(err) << endl;
				return err;
			}
			delete input_e;
			input_e = 0;
		}
		
	}//BAutolock _l(mImpl->lock.Lock());

	if (!output_e && release_fn)
	{
		// notify
		release_fn(output, release_context);
	}
	return B_OK;
}

status_t 
BMediaEndpointGraph::SetAcquireHook(acquire_hook hook, void * context)
{
	BAutolock _l(mImpl->lock.Lock());
	mImpl->acquire_fn = hook;
	mImpl->acquire_context = context;
	return B_OK;
}

status_t 
BMediaEndpointGraph::SetReleaseHook(release_hook hook, void * context)
{
	BAutolock _l(mImpl->lock.Lock());
	mImpl->release_fn = hook;
	mImpl->release_context = context;
	return B_OK;
}

ssize_t 
BMediaEndpointGraph::Visit(visit_hook hook, void * context, visit_order_t order)
{
	BAutolock _l(mImpl->lock.Lock());
	switch (order)
	{
		case UNORDERED:
			return visit_unordered(hook, context);
			
		case BREADTH_FIRST:
			return visit_breadth_first(hook, context);

		case DEPTH_FIRST:
			return visit_depth_first(hook, context);
	}
}

status_t 
BMediaEndpointGraph::ListEndpoints(BMediaEndpointVector *outEndpoints, visit_order_t order)
{
	if (!outEndpoints) return B_BAD_VALUE;
	return Visit(&do_list_endpoints, static_cast<void*>(outEndpoints), order);
}

status_t 
BMediaEndpointGraph::PruneUnreachable()
{
	status_t err;

	ssize_t unreachable = 0;
	impl::entry ** unreachable_entries = 0;
	release_hook release_fn;
	void * release_context;
	
	// remove unreachable entries from the graph, keeping references to them on
	// a stack-allocated list.
	{
		BAutolock _l(mImpl->lock.Lock());

		release_fn = mImpl->release_fn;
		release_context = mImpl->release_context;

		const ssize_t reachable = visit_breadth_first(0, 0);
		if (reachable < B_OK) return reachable;
	
		const size_t total = mImpl->entries.CountItems();
		unreachable = total - reachable;
	
		unreachable_entries = static_cast<impl::entry**>(alloca(sizeof(impl::entry*) * unreachable));
	
		size_t cur_unreachable = 0;
		size_t removed = 0;
		for (size_t n = 0; n < total; n++)
		{
			impl::entry * e = static_cast<impl::entry*>(mImpl->entries.EditItemAt(n - removed));
			if (e->visit_state != impl::VIRGIN) continue;
#if DEBUG
			// consistency check: we shouldn't find more unvisited entries than
			// visit_breadth_first() told us to expect
			if (cur_unreachable >= (size_t)unreachable)
			{
				debugger("BMediaEndpointGraph::PruneUnreachable()");
			}
#endif
			unreachable_entries[cur_unreachable] = e;
			cur_unreachable++;
			
			err = mImpl->remove_entry(e);
			if (err < B_OK)
			{
				debugger("BMediaEndpointGraph::PruneUnreachable(): mImpl->remove_entry(e)");
				return err;
			}
			removed++;
		}
	}	
	
	// now that we're no longer holding the lock, notify any outputs that we've removed,
	// then delete the unreachable entries.
	if (release_fn)
	{
		for (ssize_t n = 0; n < unreachable; n++)
		{
			if (unreachable_entries[n]->output != 0)
			{
				release_fn(unreachable_entries[n]->output, release_context);
			}
			delete unreachable_entries[n];
		}
	}
	
	return B_OK;
}

void 
BMediaEndpointGraph::MakeEmpty()
{
	BAutolock _l(mImpl->lock.Lock());
	mImpl->make_empty();
}

lock_status_t 
BMediaEndpointGraph::Lock()
{
	return mImpl->lock.Lock();
}

void 
BMediaEndpointGraph::Unlock()
{
	mImpl->lock.Unlock();
}

void 
BMediaEndpointGraph::do_list_endpoints(IMediaEndpoint::arg endpoint, void *context)
{
	BMediaEndpointVector * v = static_cast<BMediaEndpointVector*>(context);
	v->AddItem(endpoint);
}

ssize_t 
BMediaEndpointGraph::visit_unordered(visit_hook hook, void *context)
{
	// * we assume mImpl->lock has been acquired
	
	for (int32 n = mImpl->entries.CountItems()-1; n >= 0; n--)
	{
		const impl::entry * e = static_cast<impl::entry *>(mImpl->entries.EditItemAt(n));
		hook(e->endpoint, context);
#if DEBUG
berr << indent << "* " << indent;
for (int32 nn = e->adjacent.CountItems()-1; nn >= 0; nn--)
{
	berr << static_cast<const impl::entry*>(e->adjacent[nn])->endpoint->Name();
	if (nn > 0) berr << ", ";
}
berr << dedent << dedent << endl;
#endif
	}
	return mImpl->entries.CountItems();
}

ssize_t 
BMediaEndpointGraph::visit_breadth_first(visit_hook hook, void *context)
{
	// * we assume mImpl->lock has been acquired

	const size_t endpoint_count = mImpl->entries.CountItems();
	if (endpoint_count == 0) return 0;
	impl::entry ** visited_queue = static_cast<impl::entry**>(alloca(sizeof(impl::entry*) * endpoint_count));
	if (!visited_queue) return B_NO_MEMORY;
	size_t head = 0;
	size_t tail = 0;
	size_t enqueued = 0;
	ssize_t visited = 0;

	// initialize root-entry state (pick a root if none was set)			
	impl::entry * root = mImpl->root;
	if (!root) root = static_cast<impl::entry*>(mImpl->entries.EditItemAt(0));
	root->visit_state = impl::VISITED;
	visited_queue[tail] = root;
	if (++tail == endpoint_count) tail = 0;
	enqueued++;
	visited++;
	if (hook) hook(root->endpoint, context);
	
	// initialize the remaining entries
	for (int32 n = mImpl->entries.CountItems()-1; n >= 0; n--)
	{
		impl::entry * e = static_cast<impl::entry *>(mImpl->entries.EditItemAt(n));
		if (e == root) continue;
		e->visit_state = impl::VIRGIN;
	}

	while (enqueued)
	{
		impl::entry * e = visited_queue[head];
		const size_t adjacent_count = e->adjacent.CountItems();
		for (size_t n = 0; n < adjacent_count; n++)
		{
			impl::entry * ae = static_cast<impl::entry *>(e->adjacent.EditItemAt(n));
			if (ae->visit_state == impl::VIRGIN)
			{
				ae->visit_state = impl::VISITED;
				visited_queue[tail] = ae;
				if (++tail == endpoint_count) tail = 0;
				enqueued++;
				visited++;
				if (hook) hook(ae->endpoint, context);
			}
		}
		e->visit_state = impl::EXHAUSTED;
		if (++head == endpoint_count) head = 0;
		enqueued--;
	}

	return visited;
}

ssize_t 
BMediaEndpointGraph::visit_depth_first(visit_hook hook, void *context)
{
	// * we assume mImpl->lock has been acquired

#warning * IMPLEMENT BMediaEndpointGraph::visit_depth_first
	return B_UNSUPPORTED;
}

BMediaEndpointGraph::impl *
BMediaEndpointGraph::clone() const
{
	status_t err;
	impl * c = new impl;

	mImpl->lock.Lock();

	c->acquire_fn = mImpl->acquire_fn;
	c->acquire_context = mImpl->acquire_context;
	c->release_fn = mImpl->release_fn;
	c->release_context = mImpl->release_context;
	
	// pass 1: create entries
	size_t endpoint_count = mImpl->entries.CountItems();

	for (size_t n = 0; n < endpoint_count; n++)
	{
		const impl::entry * source_e = static_cast<const impl::entry *>(mImpl->entries[n]);
		c->entries.AddItem(new impl::entry(source_e->endpoint, source_e->output));
	}

	// pass 2: build adjacency lists in the +++ dumbest imaginable +++ manner
	for (size_t n = 0; n < endpoint_count; n++)
	{
		const impl::entry * source_e = static_cast<const impl::entry *>(mImpl->entries[n]);
		impl::entry * dest_e = static_cast<impl::entry *>(c->entries.EditItemAt(n));
		for (size_t nn = 0; nn < source_e->adjacent.CountItems(); nn++)
		{
			const impl::entry * source_e_adjacent = static_cast<const impl::entry *>(source_e->adjacent[nn]);
			impl::entry * dest_e_adjacent = 0;
			err = c->find_entry(source_e_adjacent->endpoint->AsBinder(), &dest_e_adjacent);
			ASSERT(err >= B_OK);
			dest_e->adjacent.AddItem(dest_e_adjacent);
		}
	}

	mImpl->lock.Unlock();
	
	for (size_t n = 0; n < endpoint_count; n++)
	{
		const impl::entry * e = static_cast<const impl::entry *>(c->entries[n]);
		if (e->output != 0)
		{
			c->acquire_fn(e->output, c->acquire_context);
		}
	}
	
	return c;
}


} } // B::Media2
