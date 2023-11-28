// private header for the UDPMessenger/UDPLooper implementation

#ifndef UDPMessage_p_H
#define UDPMessage_p_H 1

// max payload size that will be sent in a single UDP datagram.  datagrams other
// than the last one MUST contain exactly this many bytes of payload.
#define UDPM_MAX_PAYLOAD (8192U)

#endif
