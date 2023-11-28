#ifndef _3D_DEFS_H
#define _3D_DEFS_H

#include <SupportDefs.h>
#include <InterfaceDefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _3D_VECTOR_H
#include "3dVector.h"
#endif

enum {
	B_3D_NAME_LENGTH = 16
};
	
typedef struct {
	float       radius;
	B3dVector   min;
	B3dVector   max;
} B3dSteric;

#endif










