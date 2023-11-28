#include <Font.h>
#include <Window.h>

void What(const char* what);
void WhatWithTimeout(const char* what, int32 seconds=3);
void CenterWindowOnScreen(BWindow* w);
float FontHeight(const BFont* font, bool full);
