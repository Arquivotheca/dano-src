
#if !defined(port_sync_h)
#define port_sync_h

#include <OS.h>

extern status_t _write_port_sync(port_id port, int32 msg_code, const void *msg_buffer, size_t buffer_size);
extern ssize_t _read_port_etc_sync(port_id port, int32 *msg_code, void *msg_buffer, size_t buffer_size, uint32 flags, bigtime_t timeout, sem_id *toDelete);
extern status_t _write_port_etc_sync(port_id port, int32 msg_code, const void *msg_buffer, size_t buffer_size, uint32 flags, bigtime_t timeout);

#endif	//	port_sync_h

