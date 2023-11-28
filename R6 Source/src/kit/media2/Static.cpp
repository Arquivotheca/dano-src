
#include <media2/IMediaCollective.h>
#include <media2/IMediaControllable.h>
#include <media2/IMediaEndpoint.h>
#include <media2/IMediaNode.h>
#include <media2/IMediaProducer.h>
#include "BufferOutlet.h"

#include "shared_properties.h"

namespace B {
namespace Media2 {

// +++ short forms

// interface descriptors
const BValue IBufferSource::descriptor(BValue::TypeInfo(typeid(IBufferSource)));
const BValue IMediaCollective::descriptor(BValue::TypeInfo(typeid(IMediaCollective)));
const BValue IMediaControllable::descriptor(BValue::TypeInfo(typeid(IMediaControllable)));
const BValue IMediaEndpoint::descriptor(BValue::TypeInfo(typeid(IMediaEndpoint)));
const BValue IMediaOutput::descriptor(BValue::TypeInfo(typeid(IMediaOutput)));
const BValue IMediaInput::descriptor(BValue::TypeInfo(typeid(IMediaInput)));
const BValue IMediaNode::descriptor(BValue::TypeInfo(typeid(IMediaNode)));
const BValue IMediaProducer::descriptor(BValue::TypeInfo(typeid(IMediaProducer)));

// interface bindings
const BValue IMediaEndpoint::attached_to_node("attached_to_node");
const BValue IMediaEndpoint::detached_from_node("detached_from_node");
const BValue IMediaEndpoint::connected("connected");
const BValue IMediaEndpoint::disconnected("disconnected");
const BValue IMediaEndpoint::allocate_buffers("allocate_buffers");

// format keys
const BValue B_FORMATKEY_MEDIA_TYPE("media_type");
const BValue B_FORMATKEY_BUFFER_TRANSPORT("buffer_transport_type");
const BValue B_FORMATKEY_CHANNEL_COUNT("channel_count");
const BValue B_FORMATKEY_FRAME_RATE("frame_rate");
const BValue B_FORMATKEY_BUFFER_FRAMES("buffer_frames");
const BValue B_FORMATKEY_BYTE_ORDER("byte_order");
const BValue B_FORMATKEY_RAW_AUDIO_TYPE("raw_audio_type");
const BValue B_FORMATKEY_RAW_AUDIO_BITS("raw_audio_bits");
const BValue B_FORMATKEY_WIDTH("width");
const BValue B_FORMATKEY_HEIGHT("height");
const BValue B_FORMATKEY_BYTES_PER_ROW("bytes_per_row");
const BValue B_FORMATKEY_COLORSPACE("color_space");
const BValue B_FORMATKEY_ENCODING("encoding");
const BValue B_FORMATKEY_INFO("info");
const BValue B_FORMATKEY_DECODED_BUFFER_SIZE("dec_buffer_size");

// propagate values
const BValue B_ENDPOINTKEY_FLUSH("flush");
const BValue B_ENDPOINTKEY_LATE("late");
const BValue B_ENDPOINTKEY_REPEAT_LAST_FRAME("repeat_last_frame");
const BValue B_ENDPOINTKEY_RELEASE_ALL_BUFFERS("release_all_buffers");

} // Media2

namespace Private {

// private interface descriptors
const BValue IBufferOutlet::descriptor(BValue::TypeInfo(typeid(IBufferOutlet)));

// shared binder property keys
const BValue PARG_NODE(".node");
const BValue PARG_ENDPOINT(".endpoint");
const BValue PARG_INPUT(".input");
const BValue PARG_OUTPUT(".output");
const BValue PARG_TRANSPORT(".transport");
const BValue PARG_CONSTRAINT(".constraint");
const BValue PARG_FORMAT(".format");
const BValue PARG_PREFERENCE(".preference");
const BValue PARG_BUFFER(".buffer");
const BValue PARG_BUFFER_ID(".buffer_id");
const BValue PARG_ENDPOINT_TYPE(".endpoint_type");
const BValue PARG_ENDPOINT_STATE(".endpoint_state");
const BValue PARG_KEY(".key");
const BValue PARG_VALUE(".value");
const BValue PARG_VISITED_ENDPOINTS(".visited_endpoints");
const BValue PARG_INDEX(".index");
const BValue PARG_TIMEOUT(".timeout");
const BValue PARG_BUFFER_COUNT(".buffer_count");
const BValue PARG_BUFFER_CAPACITY(".buffer_capacity");

const BValue PMETHOD_LIST_ENDPOINTS("list_endpoints");
const BValue PMETHOD_LIST_LINKED_ENDPOINTS("list_linked_endpoints");
const BValue PMETHOD_LIST_NODES("list_nodes");
const BValue PMETHOD_LIST_BUFFERS("list_buffers");

const BValue PMETHOD_CONTROL("control");
const BValue PMETHOD_CONTROL_INFO("control_info");
const BValue PMETHOD_SET_CONTROL("set_control");

const BValue PMETHOD_NAME("name");
const BValue PMETHOD_PARENT("parent");
const BValue PMETHOD_NODE("node");
const BValue PMETHOD_PARTNER("partner");
const BValue PMETHOD_BUFFER_SOURCE("buffer_source");

const BValue PMETHOD_CONSTRAINT("constraint");
const BValue PMETHOD_PREFERENCE("preference");
const BValue PMETHOD_FORMAT("format");
const BValue PMETHOD_NODE_CHAIN("node_chain");
const BValue PMETHOD_ACQUIRE_BUFFER("acquire_buffer");
const BValue PMETHOD_ALLOCATE_BUFFERS("allocate_buffers");
const BValue PMETHOD_PROPAGATE("propagate");
const BValue PMETHOD_GET_BUFFER_CONSTRAINTS("get_buffer_constraints");
const BValue PMETHOD_MAKE_BUFFER_SOURCE("make_buffer_source");
const BValue PMETHOD_COMMIT_DEPENDANT_TRANSACTION("commit_dependant_transaction");
const BValue PMETHOD_CANCEL_DEPENDANT_TRANSACTION("cancel_dependant_transaction");
const BValue PMETHOD_ADDED_TO_CONTEXT("added_to_graph");
const BValue PMETHOD_REMOVED_FROM_CONTEXT("removed_from_graph");

const BValue PMETHOD_ENDPOINT_TYPE("type");
const BValue PMETHOD_ENDPOINT_STATE("state");

const BValue PMETHOD_RESERVE("reserve");
const BValue PMETHOD_CONNECT("connect");
const BValue PMETHOD_DISCONNECT("disconnect");
const BValue PMETHOD_ACCEPT_FORMAT("accept_format");
const BValue PMETHOD_DEPENDANT_CONNECT("dependant_connect");

const BValue PMETHOD_ACCEPT_BUFFER_COUNT("accept_buffer_count");
const BValue PMETHOD_HANDLE_BUFFER("handle_buffer");

const BValue PMETHOD_ACCEPT_RESERVE("accept_reserve");
const BValue PMETHOD_ACCEPT_CONNECT("accept_connect");
const BValue PMETHOD_ACCEPT_DEPENDANT_CONNECT("accept_dependant_connect");
const BValue PMETHOD_ACCEPT_DISCONNECT("accept_disconnect");

} // Private

namespace Media2 {

//****************************************************************************//
// legacy/codec stuff

const type_code B_CODEC_TYPE_INFO = 0x040807b2;

/* format wildcards */
media_multi_audio_format media_raw_audio_format::wildcard;
media_multi_audio_format media_multi_audio_format::wildcard;
media_encoded_audio_format media_encoded_audio_format::wildcard;
media_multistream_format media_multistream_format::wildcard = { 0.0, 0.0, 0UL, 0UL, 0UL, media_multistream_format::B_ANY, { 0UL, 0UL }, { { 0.0, 0U, 0U, B_NO_COLOR_SPACE, 0.0, 0UL, 0, 0 } } };
media_video_display_info media_video_display_info::wildcard = { B_NO_COLOR_SPACE, 0, 0, 0, 0, 0, 0, { 0, 0, 0 } };
media_raw_video_format media_raw_video_format::wildcard = { 0.0, 0, 0, 0, 0, 0, 0, media_video_display_info::wildcard };
media_encoded_video_format media_encoded_video_format::wildcard = { media_raw_video_format::wildcard, 0.0, 0.0, media_encoded_video_format::B_ANY, 0, 0, 0, { 0, 0, 0 } };

} } // B::Media2

