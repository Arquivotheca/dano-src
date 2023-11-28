/*
	PResources
*/

#ifndef HAPPRESFILE_H
#define HAPPRESFILE_H

#include <Rect.h>
#include <MenuBar.h>

namespace HResources {

#if __BEOS__
BMenuBar* GetMenuBar(BRect frame, int id);
BMenu* GetMenu(int id, bool popup = false);
#endif

const void *GetResource(unsigned long type, int id);
const void *GetResource(unsigned long type, int id, size_t& size);
const void *GetResource(unsigned long type, int id, size_t& size, const char **name);

const void *GetNamedResource(unsigned long type, const char *name);

}

#endif
