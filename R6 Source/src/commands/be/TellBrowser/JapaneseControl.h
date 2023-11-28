
#ifndef _JAPANESE_CONTROL_H
#define	_JAPANESE_CONTROL_H

class BMessage;

void ReadJapaneseSettings(BMessage *message);
void WriteJapaneseSettings(BMessage *message);
void AddToJapaneseDictionary(BMessage *message);
static char**build_argv(char *str, int *argc);
static void SendToJIM(BMessage *msg);

#endif

