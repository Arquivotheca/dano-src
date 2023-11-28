#include <stdlib.h>
#include <Node.h>

node_ref::node_ref()
{
	device = -1;
	node = -1;
}

node_ref::node_ref(const node_ref &ref)
{
	device = ref.device;
	node = ref.node;
}

bool		node_ref::operator==(const node_ref &ref) const
{
	return ((node == ref.node) &&
			(device == ref.device));
}

bool		node_ref::operator!=(const node_ref &ref) const
{
	return ((node != ref.node) ||
			(device != ref.device));

}

node_ref &	node_ref::operator=(const node_ref &ref)
{
	if (&ref == this)
	  return *this;
	device = ref.device;
	node = ref.node;
	return *this;
}


