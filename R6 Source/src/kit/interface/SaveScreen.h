#ifndef SAVESCREEN_H
#define SAVESCREEN_H

#if _SUPPORTS_FEATURE_SCREEN_DUMP

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
extern "C" void		_save_screen_to_file_();

#endif

#endif

