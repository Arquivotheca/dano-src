/*
 * Copyright 1997, Be, Inc.
 * All Rights Reserved 
 */
#if __POWERPC__

/*
 * Compatibility routines, since _swap* functions were moved to libroot and
 * there is code that still expects them to be in libbe.
 */
#include <byteorder.h>

extern uint32 _swap_int32(uint32);
extern uint64 _swap_int64(uint64);
extern double _swap_double(double);
extern float _swap_float(float);

extern 
uint32 _swap_int32(uint32 uarg) {
	return __swap_int32(uarg);
}

extern 
uint64 _swap_int64(uint64 uarg) {
	return __swap_int64(uarg);
}

extern 
double _swap_double(double arg) {
	return __swap_double(arg);
}

extern 
float _swap_float(float arg) {
	return __swap_float(arg);
}

#endif /* __POWERPC__ */
