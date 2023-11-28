#if !defined(_GRAPHIC_DRIVER_H_)
#define _GRAPHIC_DRIVER_H_

#include <Drivers.h>

/* The API for driver access is C, not C++ */

#ifdef __cplusplus
extern "C" {
#endif

enum {
	B_GET_ACCELERANT_SIGNATURE = B_GRAPHIC_DRIVER_BASE,
	B_GET_3D_SIGNATURE,
	B_GRAPHIC_DRIVER_PRIVATE_BASE = (B_GRAPHIC_DRIVER_BASE + 50)
};

#ifdef __cplusplus
}
#endif

#endif
