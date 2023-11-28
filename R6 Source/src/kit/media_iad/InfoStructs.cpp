/*	InfoStructs.cpp	*/

#include "trinity_p.h"
#include "MediaAddOn.h"
#include "Buffer.h"

/* null values */
media_destination media_destination::null(-1, -1);
media_source media_source::null(-1, -1);
media_node media_node::null;	/* default constructor does it */

/* format wildcards */
media_multi_audio_format media_raw_audio_format::wildcard;
media_multi_audio_format media_multi_audio_format::wildcard;
media_encoded_audio_format media_encoded_audio_format::wildcard;
media_multistream_format media_multistream_format::wildcard = { 0.0, 0.0, 0, 0, 0, media_multistream_format::B_ANY };
media_video_display_info media_video_display_info::wildcard = { B_NO_COLOR_SPACE, 0, 0, 0, 0, 0 };
media_raw_video_format media_raw_video_format::wildcard = { 0.0, 0, 0, 0, 0, 0, 0, media_video_display_info::wildcard };
media_encoded_video_format media_encoded_video_format::wildcard = { media_raw_video_format::wildcard, 0.0, 0.0, media_encoded_video_format::B_ANY, 0, 0, 0 };



media_source::media_source(
	port_id i_port,
	int32 i_id)
{
	port = i_port;
	id = i_id;
}

media_source::media_source(
	const media_source & clone)
{
	port = clone.port;
	id = clone.id;
}

media_source &
media_source::operator=(
	const media_source & clone)
{
	port = clone.port;
	id = clone.id;
	return *this;
}

media_source::media_source()
{
	*this = null;
}

media_source::~media_source()
{
}


media_destination::media_destination(
	port_id i_port,
	int32 i_id)
{
	port = i_port;
	id = i_id;
}

media_destination::media_destination(
	const media_destination & clone)
{
	port = clone.port;
	id = clone.id;
}

media_destination &
media_destination::operator=(
	const media_destination & clone)
{
	port = clone.port;
	id = clone.id;
	return *this;
}

media_destination::media_destination()
{
	*this = null;
}

media_destination::~media_destination()
{
}


bool
operator==(
	const media_destination & a,
	const media_destination & b)
{
	return (a.id == b.id) && (a.port == b.port);
}


bool
operator!=(
	const media_destination & a,
	const media_destination & b)
{
	return (a.id != b.id) || (a.port != b.port);
}


bool
operator<(
	const media_destination & a, 
	const media_destination & b)
{
	if (a.port < b.port) return true;
	if (a.port > b.port) return false;
	if (a.id < b.id) return true;
	return false;
}


bool
operator==(
	const media_source & a,
	const media_source & b)
{
	return (a.id == b.id) && (a.port == b.port);
}


bool
operator!=(
	const media_source & a,
	const media_source & b)
{
	return (a.id != b.id) || (a.port != b.port);
}


bool
operator<(
	const media_source & a, 
	const media_source & b)
{
	if (a.port < b.port) return true;
	if (a.port > b.port) return false;
	if (a.id < b.id) return true;
	return false;
}


bool
operator==(
	const media_node & a,
	const media_node & b)
{
	//	all bad nodes are the same
	if ((a.node < 0) && (b.node < 0)) return true;
#if !NDEBUG
	if (a.node == b.node) {
		if ((a.kind != b.kind) || (a.port != b.port)) {
			fprintf(stderr, "a.node %d; a.kind 0x%x b.kind 0x%x a.port %d b.port %d\n", a.node, a.kind, b.kind, a.port, b.port);
		}
		ASSERT((a.kind == b.kind) && (a.port == b.port));
	}
#endif
	return (a.node == b.node);
}


bool
operator!=(
	const media_node & a,
	const media_node & b)
{
	//	all bad nodes are the same
	if ((a.node < 0) && (b.node < 0)) return false;
#if !NDEBUG
	if (a.node == b.node) {
		if ((a.kind != b.kind) || (a.port != b.port)) {
			fprintf(stderr, "a.node %d; a.kind 0x%x b.kind 0x%x a.port %d b.port %d\n", a.node, a.kind, b.kind, a.port, b.port);
		}
		ASSERT((a.kind == b.kind) && (a.port == b.port));
	}
#endif
	return (a.node != b.node);// || (a.kind != b.kind) || (a.port != b.port);
}


bool
operator<(
	const media_node & a,
	const media_node & b)
{
	//	all bad nodes are the same
	if ((a.node < 0) && (b.node < 0)) return false;
#if !NDEBUG
	if (a.node == b.node) {
		if ((a.kind != b.kind) || (a.port != b.port)) {
			fprintf(stderr, "a.node %d; a.kind 0x%x b.kind 0x%x a.port %d b.port %d\n", a.node, a.kind, b.kind, a.port, b.port);
		}
		ASSERT((a.kind == b.kind) && (a.port == b.port));
	}
#endif
	if (a.node < b.node) return true;
//	if (a.node > b.node) return false;
//	if (a.kind < b.kind) return true;
//	if (a.kind > b.kind) return false;
//	if (a.port < b.port) return true;
	return false;
}



bool operator==(const media_file_format_id & a, const media_file_format_id & b)
{
	return (a.node == b.node) && (a.device == b.device);
}

bool operator<(const media_file_format_id & a, const media_file_format_id & b)
{
	if (a.node < b.node) return true;
	if (a.node > b.node) return false;
	if (a.device < b.device) return true;
	return false;
}







/***************************** INFO STRUCTS *****************************/
#pragma mark --- Info Structs ---





media_node::media_node()
{
	node = -1;
	port = -1;
	kind = 0;
	_reserved_[0] = 0;
	_reserved_[1] = 0;
	_reserved_[2] = 0;
}

media_node::~media_node()
{
}



media_input::media_input()
{
	source = media_source::null;
	destination = media_destination::null;
	format.type = B_MEDIA_NO_TYPE;
	name[0] = 0;
//	source_node = 0;
}

media_input::~media_input()
{
}


media_output::media_output()
{
	source = media_source::null;
	destination = media_destination::null;
	format.type = B_MEDIA_NO_TYPE;
	name[0] = 0;
//	destination_node = 0;
}

media_output::~media_output()
{
}


live_node_info::live_node_info()
{
	name[0] = 0;
}

live_node_info::~live_node_info()
{
}

buffer_clone_info::buffer_clone_info()
{
	buffer = 0;
	area = offset = size = flags = 0;
}

buffer_clone_info::~buffer_clone_info()
{
}


dormant_node_info::dormant_node_info()
{
	addon = -1;
	flavor_id = -1;
	name[0] = 0;
}


dormant_node_info::~dormant_node_info()
{
}


