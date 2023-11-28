
#if !defined(media_reg_h)
#define media_reg_h

#include <SupportDefs.h>
#include <OS.h>

namespace BPrivate {

#define KEY_NAMESPACE_SIZE 64
#define WRITER_ADDON_NAMESPACE "/BPrivate/Media/Add-Ons/Writer"
#define REGISTRY_PORT_NAME "_media_key_port_"
#define KEY_TIMEOUT 10000000LL

typedef uint64 media_key_type;
typedef char media_key_namespace[64];

struct key_registration_request {
	media_key_namespace name;
	media_key_type key;
	port_id client;
	type_code type;
	size_t size;
};
struct key_registration_reply {
	media_key_type key;
	int32 data_code;
};
struct key_removal_request {
	media_key_namespace name;
	media_key_type key;
	port_id client;
	bool remove_all;
};
struct key_removal_reply {
	media_key_type key;
};
struct key_iteration_request {
	media_key_namespace name;
	media_key_type key;
	port_id client;
};
struct key_iteration_reply {
	media_key_type key;
	type_code type;
	size_t size;
};
struct key_retrieval_request {
	media_key_namespace name;
	media_key_type key;
	port_id client;
	type_code type;
	size_t size;
};
struct key_retrieval_error {	//	OK reply is data
	media_key_type key;
};
struct key_signoff_request {
	port_id client;
};

enum {
	keyRegistration = 1,
	keyRemoval,
	keyIteration,
	keyRetrieval,
	keySignoff,
	keyStopServing = 0x1000,
	keyFirstDataCommand = 0x40000000L
};

union key_request {
	key_registration_request registration;
	key_removal_request removal;
	key_iteration_request iteration;
	key_retrieval_request retrieval;
	key_signoff_request signoff;
};
union key_reply {	//	the port code is the error, or B_OK
	key_registration_reply registration;
	key_removal_reply removal;
	key_iteration_reply iteration;
	key_retrieval_error retrieval;
};


enum {
	B_KEY_NOT_FOUND = 0x8fff0000,
	B_KEY_ALREADY_REGISTERED,
	B_KEY_WRONG_TYPE,
	B_KEY_TOO_SMALL,
	B_KEY_SERVER_NOT_RUNNING
};

class BMediaKeyLookup {

public:

		BMediaKeyLookup(
				const char * name);
		~BMediaKeyLookup();

		status_t InitCheck();
		const char * Namespace() const;

		status_t RegisterKey(
				media_key_type key,
				type_code type,
				size_t size,
				const void * data);
		status_t RemoveKey(
				media_key_type key);
		status_t RemoveAllKeys();
		status_t FindKey(
				media_key_type key,
				type_code * out_type,
				size_t * out_size);
		status_t FindNextKey(
				media_key_type min_key,
				media_key_type * out_actual_key,
				type_code * out_type,
				size_t * out_size);
		ssize_t RetrieveKey(
				media_key_type key,
				type_code type,
				size_t in_buf_size,
				void * buffer);

private:

		port_id m_server;
		port_id m_client;
		char * m_namespace;
};

}
using namespace BPrivate;

#endif	//	media_reg_h

