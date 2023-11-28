#ifndef _MEDIA2_SHARED_PROPERTIES_PRIVATE_
#define _MEDIA2_SHARED_PROPERTIES_PRIVATE_

#include <support2/Value.h>

namespace B {
namespace Private {

using namespace Support2;

extern const BValue PARG_NODE;
extern const BValue PARG_ENDPOINT;
extern const BValue PARG_INPUT;
extern const BValue PARG_OUTPUT;
extern const BValue PARG_TRANSPORT;
extern const BValue PARG_CONSTRAINT;
extern const BValue PARG_FORMAT;
extern const BValue PARG_PREFERENCE;
extern const BValue PARG_BUFFER;
extern const BValue PARG_BUFFER_ID;
extern const BValue PARG_ENDPOINT_TYPE;
extern const BValue PARG_ENDPOINT_STATE;
extern const BValue PARG_KEY;
extern const BValue PARG_VALUE;
extern const BValue PARG_VISITED_ENDPOINTS;
extern const BValue PARG_INDEX;
extern const BValue PARG_TIMEOUT;
extern const BValue PARG_BUFFER_COUNT;
extern const BValue PARG_BUFFER_CAPACITY;

extern const BValue PMETHOD_LIST_ENDPOINTS;
extern const BValue PMETHOD_LIST_LINKED_ENDPOINTS;
extern const BValue PMETHOD_LIST_NODES;
extern const BValue PMETHOD_LIST_BUFFERS;

extern const BValue PMETHOD_CONTROL;
extern const BValue PMETHOD_CONTROL_INFO;
extern const BValue PMETHOD_SET_CONTROL;

extern const BValue PMETHOD_NAME;
extern const BValue PMETHOD_PARENT;
extern const BValue PMETHOD_NODE;
extern const BValue PMETHOD_PARTNER;
extern const BValue PMETHOD_BUFFER_SOURCE;

extern const BValue PMETHOD_CONSTRAINT;
extern const BValue PMETHOD_PREFERENCE;
extern const BValue PMETHOD_FORMAT;
extern const BValue PMETHOD_NODE_CHAIN;
extern const BValue PMETHOD_ACQUIRE_BUFFER;
extern const BValue PMETHOD_ALLOCATE_BUFFERS;
extern const BValue PMETHOD_PROPAGATE;
extern const BValue PMETHOD_GET_BUFFER_CONSTRAINTS;
extern const BValue PMETHOD_MAKE_BUFFER_SOURCE;
extern const BValue PMETHOD_COMMIT_DEPENDANT_TRANSACTION;
extern const BValue PMETHOD_CANCEL_DEPENDANT_TRANSACTION;
extern const BValue PMETHOD_ADDED_TO_CONTEXT;
extern const BValue PMETHOD_REMOVED_FROM_CONTEXT;

extern const BValue PMETHOD_ENDPOINT_TYPE;
extern const BValue PMETHOD_ENDPOINT_STATE;

extern const BValue PMETHOD_RESERVE;
extern const BValue PMETHOD_CONNECT;
extern const BValue PMETHOD_DISCONNECT;
extern const BValue PMETHOD_ACCEPT_FORMAT;
extern const BValue PMETHOD_DEPENDANT_CONNECT;

extern const BValue PMETHOD_ACCEPT_BUFFER_COUNT;
extern const BValue PMETHOD_HANDLE_BUFFER;

extern const BValue PMETHOD_ACCEPT_RESERVE;
extern const BValue PMETHOD_ACCEPT_CONNECT;
extern const BValue PMETHOD_ACCEPT_DEPENDANT_CONNECT;
extern const BValue PMETHOD_ACCEPT_DISCONNECT;

} } // B::Private
#endif //_MEDIA2_SHARED_PROPERTIES_PRIVATE_
