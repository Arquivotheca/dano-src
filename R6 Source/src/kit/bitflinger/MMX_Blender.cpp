/********************************************************************
* Integer blender
* MMX internal format is:
* r = 15 - 0
* g = 31 - 16
* b = 47 - 32
* a = 64 - 48
*
* Format is 4.12
* 
/********************************************************************/

#include <stdio.h>
#include <debugger.h>
#include <string.h>

#include "Bitflinger.h"
#include "asm.h"

