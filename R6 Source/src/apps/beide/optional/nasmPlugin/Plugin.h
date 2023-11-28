#ifndef _PLUGIN_
#define _PLUGIN_

extern const char* kNASMOptionsMessageName;
extern ulong kNASMMessageType;

// NULL only works for pointers in gcc
// but I just can't bring myself to do a char = 0...
#define NIL (0)

#endif
