#ifndef __MDADDON_DEBUG_H__
#define __MDADDON_DEBUG_H__

#ifdef MDADDON_DEBUG
#define MDADDON_PRINT(f...) 			printf("MDAddon : " f)
#else
#define MDADDON_PRINT(f...)				0
#endif

#endif
