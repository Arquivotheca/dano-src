
#ifndef _SUPPORT2_BINDERKEYS_H_
#define _SUPPORT2_BINDERKEYS_H_

#include <support2/Value.h>

namespace B {
namespace Private {

using namespace Support2;

// Standard binder keys.
extern const BValue g_keyRead;
extern const BValue g_keyWrite;
extern const BValue g_keyEnd;
extern const BValue g_keySync;
extern const BValue g_keyPosition;
extern const BValue g_keySeek;
extern const BValue g_keyMode;
extern const BValue g_keyPost;
extern const BValue g_keyRedirectMessagesTo;
extern const BValue g_keyPrint;
extern const BValue g_keyBumpIndentLevel;
extern const BValue g_keyStatus;
extern const BValue g_keyValue;
extern const BValue g_keyWhich;
extern const BValue g_keyFlags;
//extern const BValue g_key;

// System binder keys.
extern const BValue g_keySysInspect;
extern const BValue g_keySysInherit;
extern const BValue g_keySysBequeath;

// IRender keys
extern const BValue g_keyIsOpaque;
extern const BValue g_keyDoesClipParent;
extern const BValue g_keyBranch;
extern const BValue g_keyDisplay;
extern const BValue g_keyBounds;
extern const BValue g_keyDraw;
extern const BValue g_keyInvalidate;

} }	// namespace B::Private

#endif	/* _SUPPORT2_BINDERKEYS_H_ */
