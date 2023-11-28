/*******************************************************************************
/
/	File:		StorageDefs.h
/
/	Description:	Storage Kit miscellany.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _STORAGE2_DEFS_H
#define	_STORAGE2_DEFS_H

#include <support2/SupportDefs.h>

#include <fcntl.h>
#include <sys/param.h>
#include <limits.h>

namespace B {
namespace Storage2 {

using namespace Support2;

/*----Forward declarations--------------------------------*/

struct entry_ref;
struct node_ref;

class BDirectory;
class BEntry;
class BStatable;
class BPath;
class BVolume;
class BFile;
class BSymLink;

/*----NODE FLAVORS--------------------------------*/

enum node_flavor {
  B_FILE_NODE=0x01,
  B_SYMLINK_NODE=0x02,
  B_DIRECTORY_NODE=0x04,
  B_ANY_NODE= 0x07
};

} } // namespace B::Storage2

#endif	// _STORAGE2_DEFS_H
