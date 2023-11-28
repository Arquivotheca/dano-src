//========================================================================
//	MKeyIcons.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MKeyIcons.h"
#include "MKeyBindingManager.h"
#include "Utils.h"

#include <Debug.h>

#define L 253
#define G 18
#define d 24
#define B 0
#define w 63
#define t 255

const char Altdata[] = {
	L,L,L,L,L,L,L,L,L,L,L,L,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,d,d,d,d,B,d,d,d,d,d,G,
	L,d,d,d,B,B,B,d,d,d,d,G,
	L,d,d,d,B,d,B,d,d,d,d,G,
	L,d,d,B,B,B,B,B,d,d,d,G,
	L,d,d,B,d,d,d,B,d,d,d,G,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,G,G,G,G,G,G,G,G,G,G,G
};
const char Optdata[] = {
	L,L,L,L,L,L,L,L,L,L,L,L,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,d,d,d,d,B,B,d,d,d,d,G,
	L,d,d,d,B,d,d,B,d,d,d,G,
	L,d,d,d,B,d,d,B,d,d,d,G,
	L,d,d,d,B,d,d,B,d,d,d,G,
	L,d,d,d,d,B,B,d,d,d,d,G,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,G,G,G,G,G,G,G,G,G,G,G
};
const char Controldata[] = {
	L,L,L,L,L,L,L,L,L,L,L,L,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,d,d,d,d,B,B,d,d,d,d,G,
	L,d,d,d,B,d,d,B,d,d,d,G,
	L,d,d,d,B,d,d,d,d,d,d,G,
	L,d,d,d,B,d,d,B,d,d,d,G,
	L,d,d,d,d,B,B,d,d,d,d,G,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,G,G,G,G,G,G,G,G,G,G,G
};
const char Shiftdata[] = {
	L,L,L,L,L,L,L,L,L,L,L,L,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,d,d,B,B,B,B,B,d,d,d,G,
	L,d,d,B,d,d,d,d,d,d,d,G,
	L,d,d,d,B,B,B,d,d,d,d,G,
	L,d,d,d,d,d,d,B,d,d,d,G,
	L,d,d,B,B,B,B,B,d,d,d,G,
	L,d,d,d,d,d,d,d,d,d,d,G,
	L,G,G,G,G,G,G,G,G,G,G,G
};

const char UpArrowdata[] = {
	t,t,t,B,t,t,t,
	t,t,B,B,B,t,t,
	t,B,t,B,t,B,t,
	t,t,t,B,t,t,t,
	t,t,t,B,t,t,t,
	t,t,t,B,t,t,t,
	t,t,t,B,t,t,t,
};
const char DownArrowdata[] = {
	t,t,t,B,t,t,t,
	t,t,t,B,t,t,t,
	t,t,t,B,t,t,t,
	t,t,t,B,t,t,t,
	t,B,t,B,t,B,t,
	t,t,B,B,B,t,t,
	t,t,t,B,t,t,t,
};
const char LeftArrowdata[] = {
	t,t,t,t,t,t,t,
	t,t,B,t,t,t,t,
	t,B,t,t,t,t,t,
	B,B,B,B,B,B,B,
	t,B,t,t,t,t,t,
	t,t,B,t,t,t,t,
	t,t,t,t,t,t,t,
};
const char RightArrowdata[] = {
	t,t,t,t,t,t,t,
	t,t,t,t,B,t,t,
	t,t,t,t,t,B,t,
	B,B,B,B,B,B,B,
	t,t,t,t,t,B,t,
	t,t,t,t,B,t,t,
	t,t,t,t,t,t,t,
};
const char F1data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,B,B,B,t,t,t,B,t,t,t,B,
	B,t,t,B,t,t,t,t,t,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,t,t,B,t,t,t,B,
	B,t,t,B,B,B,t,t,t,t,B,t,t,t,B,
	B,t,t,B,t,t,t,t,t,t,B,t,t,t,B,
	B,t,t,B,t,t,t,t,t,t,B,t,t,t,B,
	B,t,t,B,t,t,t,t,t,t,B,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F2data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,B,B,B,t,t,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,B,B,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,t,t,t,B,
	B,t,t,B,t,t,t,t,B,B,B,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F3data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,B,B,B,t,B,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,B,B,t,t,t,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,B,B,B,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F4data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,B,B,B,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,B,B,t,t,B,B,B,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F5data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,B,B,B,t,B,B,B,B,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,t,t,t,B,
	B,t,t,B,B,B,t,t,t,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,B,B,B,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F6data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,B,B,B,t,t,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,t,t,t,B,
	B,t,t,B,B,B,t,t,B,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,B,B,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F7data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,B,B,B,t,B,B,B,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,B,B,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,B,t,t,t,B,
	B,t,t,B,t,t,t,t,t,B,t,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F8data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,B,B,B,t,t,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,B,B,t,t,t,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,B,B,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F9data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,B,B,B,t,t,B,B,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,B,B,B,t,t,t,B,B,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,B,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F10data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,B,B,B,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,B,B,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,B,t,B,t,B,
	B,t,B,B,B,t,t,t,B,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F11data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,B,B,B,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,B,B,t,B,B,t,t,B,
	B,t,B,t,t,t,t,t,B,t,t,B,t,t,B,
	B,t,B,B,B,t,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F12data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,B,B,B,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,B,B,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,t,B,
	B,t,B,B,B,t,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,t,B,t,B,t,t,t,B,
	B,t,B,t,t,t,t,t,B,t,B,B,B,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F13data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,B,B,B,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,B,B,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,t,B,
	B,t,B,B,B,t,t,t,B,t,t,B,t,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F14data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,B,B,B,t,t,B,t,B,t,B,t,B,
	B,t,B,t,t,t,t,B,B,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,B,t,B,t,B,
	B,t,B,B,B,t,t,t,B,t,B,B,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char F15data[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,B,B,B,t,t,B,t,B,B,B,t,B,
	B,t,B,t,t,t,t,B,B,t,B,t,t,t,B,
	B,t,B,t,t,t,t,t,B,t,B,B,t,t,B,
	B,t,B,B,B,t,t,t,B,t,t,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,B,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char Homedata[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,B,t,t,t,B,t,t,B,B,t,B,t,t,t,B,t,t,B,
	B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,
	B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,B,B,t,B,
	B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,t,t,t,B,
	B,t,B,t,B,t,t,B,t,t,B,t,B,t,B,t,t,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t
};
const char Enddata[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,B,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,B,t,B,
	B,t,t,B,t,t,B,B,t,t,t,t,B,t,B,
	B,t,B,t,B,t,B,t,B,t,t,B,B,t,B,
	B,t,B,B,B,t,B,t,B,t,B,t,B,t,B,
	B,t,B,t,t,t,B,t,B,t,B,t,B,t,B,
	B,t,t,B,t,t,B,t,B,t,t,B,B,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char ForwardDeletedata[] = {
	B,B,B,B,B,B,B,B,B,B,t,t,t,t,t,
	B,t,t,t,t,t,t,t,t,t,B,t,t,t,t,
	B,t,B,t,t,t,t,t,B,t,t,B,t,t,t,
	B,t,t,B,t,t,t,B,t,t,t,t,B,t,t,
	B,t,t,t,B,t,B,t,t,t,t,t,t,B,t,
	B,t,t,t,t,B,t,t,t,t,t,t,t,t,B,
	B,t,t,t,B,t,B,t,t,t,t,t,t,B,t,
	B,t,t,B,t,t,t,B,t,t,t,t,B,t,t,
	B,t,B,t,t,t,t,t,B,t,t,B,t,t,t,
	B,t,t,t,t,t,t,t,t,t,B,t,t,t,t,
	B,B,B,B,B,B,B,B,B,B,t,t,t,t,t,
};
const char Backspacedata[] = {
	t,t,t,t,t,B,B,B,B,B,B,B,B,B,B,
	t,t,t,t,B,t,t,t,t,t,t,t,t,t,B,
	t,t,t,B,t,t,B,t,t,t,t,t,B,t,B,
	t,t,B,t,t,t,t,B,t,t,t,B,t,t,B,
	t,B,t,t,t,t,t,t,B,t,B,t,t,t,B,
	B,t,t,t,t,t,t,t,t,B,t,t,t,t,B,
	t,B,t,t,t,t,t,t,B,t,B,t,t,t,B,
	t,t,B,t,t,t,t,B,t,t,t,B,t,t,B,
	t,t,t,B,t,t,B,t,t,t,t,t,B,t,B,
	t,t,t,t,B,t,t,t,t,t,t,t,t,t,B,
	t,t,t,t,t,B,B,B,B,B,B,B,B,B,B,
};
const char Tabdata[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,t,t,t,t,t,t,B,t,t,t,B,
	B,t,B,B,B,t,B,B,t,t,B,t,t,t,B,
	B,t,t,B,t,t,t,t,B,t,B,t,t,t,B,
	B,t,t,B,t,t,t,B,B,t,B,B,t,t,B,
	B,t,t,B,t,t,B,t,B,t,B,t,B,t,B,
	B,t,t,B,t,t,B,t,B,t,B,t,B,t,B,
	B,t,t,t,B,t,B,B,B,t,B,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char PageUpdata[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,B,t,t,B,B,B,t,t,B,t,B,t,B,B,t,t,B,
	B,t,B,t,B,t,B,t,B,t,t,B,t,B,t,B,t,B,t,B,
	B,t,B,t,B,t,B,t,B,t,t,B,t,B,t,B,t,B,t,B,
	B,t,B,B,t,t,t,B,B,t,t,B,t,B,t,B,B,t,t,B,
	B,t,B,t,t,t,t,t,B,t,t,B,t,B,t,B,t,t,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,B,t,B,t,t,t,B,
	B,t,B,t,t,t,B,B,t,t,t,t,t,t,t,B,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t
};
const char PageDowndata[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,B,t,t,B,B,B,t,t,t,t,B,t,t,B,t,t,B,t,t,t,B,t,B,B,t,t,B,
	B,t,B,t,B,t,B,t,B,t,t,t,t,B,t,B,t,B,t,B,t,t,t,B,t,B,t,B,t,B,
	B,t,B,t,B,t,B,t,B,t,t,t,B,B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,t,B,
	B,t,B,B,t,t,t,B,B,t,t,B,t,B,t,B,t,B,t,t,B,t,B,t,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,B,t,B,t,B,t,B,t,t,B,t,B,t,t,B,t,B,t,B,
	B,t,B,t,t,t,t,t,B,t,t,t,B,B,t,t,B,t,t,t,B,t,B,t,t,B,t,B,t,B,
	B,t,B,t,t,t,B,B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char Enterdata[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,B,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,t,t,B,B,t,t,B,B,B,t,t,B,t,t,t,B,B,t,B,
	B,t,B,t,B,t,B,t,B,t,t,B,t,t,B,t,B,t,B,t,B,t,B,
	B,t,B,B,B,t,B,t,B,t,t,B,t,t,B,B,B,t,B,t,t,t,B,
	B,t,B,t,t,t,B,t,B,t,t,B,t,t,B,t,t,t,B,t,t,t,B,
	B,t,B,t,t,t,B,t,B,t,t,B,t,t,B,t,t,t,B,t,t,t,B,
	B,t,t,B,t,t,B,t,B,t,t,t,B,t,t,B,t,t,B,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char Insertdata[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,t,t,B,
	B,t,t,t,B,B,t,t,t,B,B,t,t,B,t,t,t,B,B,t,B,B,B,t,B,
	B,t,B,t,B,t,B,t,B,t,t,t,B,t,B,t,B,t,B,t,t,B,t,t,B,
	B,t,B,t,B,t,B,t,B,B,t,t,B,B,B,t,B,t,t,t,t,B,t,t,B,
	B,t,B,t,B,t,B,t,t,B,B,t,B,t,t,t,B,t,t,t,t,B,t,t,B,
	B,t,B,t,B,t,B,t,t,t,B,t,B,t,B,t,B,t,t,t,t,B,t,t,B,
	B,t,B,t,B,t,B,t,B,B,t,t,t,B,t,t,B,t,t,t,t,t,B,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};
const char Escapedata[] = {
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	B,t,t,B,t,t,t,B,B,t,t,B,t,t,B,
	B,t,B,t,B,t,B,t,t,t,B,t,B,t,B,
	B,t,B,B,B,t,B,B,t,t,B,t,t,t,B,
	B,t,B,t,t,t,t,B,B,t,B,t,t,t,B,
	B,t,B,t,t,t,t,t,B,t,B,t,B,t,B,
	B,t,t,B,t,t,B,B,t,t,t,B,t,t,B,
	B,t,t,t,t,t,t,t,t,t,t,t,t,t,B,
	t,B,B,B,B,B,B,B,B,B,B,B,B,B,t,
};


#undef L
#undef G
#undef d
#undef B
#undef w
#undef t

const uchar kModWidth = 12;
const uchar kModHeight = 10;
const uchar kArrowWidth = 7;
const uchar kArrowHeight = 7;
const uchar kVKeyWidth = 15;
const uchar kVKeyHeight = 11;
const uchar kTextKeyHeight = 11;

const uchar kBSWidth = 15;
const uchar kFWDDeleteWidth = 15;
const uchar kTabWidth = 15;
const uchar kHomeWidth = 21;
const uchar kEndWidth = 15;
const uchar kPageUpWidth = 20;
const uchar kPageDownWidth = 30;
const uchar kEnterWidth = 23;
const uchar kInsertWidth = 25;
const uchar kEscapeWidth = 15;

enum KeyIndexes
{
	altindex,
	optindex,
	controlindex,
	shiftindex,
	uparrowindex,
	downarrowindex,
	leftarrowindex,
	rightarrowindex,
	F1index,
	F2index,
	F3index,
	F4index,
	F5index,
	F6index,
	F7index,
	F8index,
	F9index,
	F10index,
	F11index,
	F12index,
	F13index,
	F14index,
	F15index,
	backspaceindex,
	forwarddeleteindex,
	tabindex,
	homeindex,
	endindex,
	pageupindex,
	pagedownindex,
	enterindex,
	insertindex,
	escapeindex
};

MList<BBitmap*>		MKeyIcons::sBitmapList;
bool				MKeyIcons::sBitmapInited;

// Lookup table to map keycode to index in the list
static uchar keyLookup[] = {
0, homeindex, 0, 0, endindex, insertindex, 0, 0, backspaceindex, tabindex, enterindex, pageupindex, pagedownindex, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, escapeindex, leftarrowindex, rightarrowindex, uparrowindex, downarrowindex,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, forwarddeleteindex,
};

// Lookup table to map width of icon to index in the list
static uchar WidthLookup[] = {
0, kHomeWidth, 0, 0, kEndWidth, kInsertWidth, 0, 0, 
kBSWidth, kTabWidth, kEnterWidth, kPageUpWidth, kPageDownWidth, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, kEscapeWidth, kArrowWidth, kArrowWidth, kArrowWidth, kArrowWidth,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, kFWDDeleteWidth,
};

// Lookup table to map virtual keycode to index in the list
static uchar VKeyLookup[] = {
0, 0, F1index, F2index, F3index, F4index, F5index, F6index, F7index, F8index, 
F9index, F10index, F11index, F12index, F13index, F14index, F15index
};

// ---------------------------------------------------------------------------
//		InitBitmaps
// ---------------------------------------------------------------------------

void
MKeyIcons::InitBitmaps()
{
	ASSERT(! sBitmapInited);
	if (! sBitmapInited)
	{
		sBitmapList.AddItem(LoadBitmap(Altdata, kModWidth, kModHeight));
		sBitmapList.AddItem(LoadBitmap(Optdata, kModWidth, kModHeight));
		sBitmapList.AddItem(LoadBitmap(Controldata, kModWidth, kModHeight));
		sBitmapList.AddItem(LoadBitmap(Shiftdata, kModWidth, kModHeight));
		sBitmapList.AddItem(LoadBitmap(UpArrowdata, kArrowWidth, kArrowHeight));
		sBitmapList.AddItem(LoadBitmap(DownArrowdata, kArrowWidth, kArrowHeight));
		sBitmapList.AddItem(LoadBitmap(LeftArrowdata, kArrowWidth, kArrowHeight));
		sBitmapList.AddItem(LoadBitmap(RightArrowdata, kArrowWidth, kArrowHeight));

		sBitmapList.AddItem(LoadBitmap(F1data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F2data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F3data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F4data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F5data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F6data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F7data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F8data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F9data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F10data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F11data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F12data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F13data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F14data, kVKeyWidth, kVKeyHeight));
		sBitmapList.AddItem(LoadBitmap(F15data, kVKeyWidth, kVKeyHeight));
		
		sBitmapList.AddItem(LoadBitmap(Backspacedata, kBSWidth, kTextKeyHeight));
		sBitmapList.AddItem(LoadBitmap(ForwardDeletedata, kFWDDeleteWidth, kTextKeyHeight));
		sBitmapList.AddItem(LoadBitmap(Tabdata, kTabWidth, kTextKeyHeight));
		sBitmapList.AddItem(LoadBitmap(Homedata, kHomeWidth, kTextKeyHeight));
		sBitmapList.AddItem(LoadBitmap(Enddata, kEndWidth, kTextKeyHeight));
		sBitmapList.AddItem(LoadBitmap(PageUpdata, kPageUpWidth, kTextKeyHeight));
		sBitmapList.AddItem(LoadBitmap(PageDowndata, kPageDownWidth, kTextKeyHeight));
		sBitmapList.AddItem(LoadBitmap(Enterdata, kEnterWidth, kTextKeyHeight));
		sBitmapList.AddItem(LoadBitmap(Insertdata, kInsertWidth, kTextKeyHeight));
		sBitmapList.AddItem(LoadBitmap(Escapedata, kEscapeWidth, kTextKeyHeight));

		sBitmapInited = true;
	}
}

// ---------------------------------------------------------------------------
//		DrawKeyBinding
// ---------------------------------------------------------------------------
//	The key binding is drawn in the view, aligned as specified, in the area rect.
//	This version draws the prefix key binding if there is one.

void
MKeyIcons::DrawKeyBinding(
	BView*						inView,
	BRect						inArea,
	KeyBinding&					inBinding,
	const MKeyBindingManager&	inManager,
	alignment					inAlignment)
{
	float		width;
	KeyBinding	prefixBinding;
	bool		hasPrefix = inBinding.prefixIndex != 0;
	
	if (hasPrefix)
		inManager.GetPrefixBinding(inBinding.prefixIndex, prefixBinding);

	switch (inAlignment)
	{
		case B_ALIGN_LEFT:
			if (hasPrefix)
			{
				width = DrawKeyBinding(inView, inArea, prefixBinding, B_ALIGN_LEFT);
				inArea.left += width;
			}
			DrawKeyBinding(inView, inArea, inBinding, B_ALIGN_LEFT);
			break;

		case B_ALIGN_RIGHT:
			width = DrawKeyBinding(inView, inArea, inBinding, B_ALIGN_RIGHT);
			if (hasPrefix)
			{
				inArea.right -= width;
				DrawKeyBinding(inView, inArea, prefixBinding, B_ALIGN_RIGHT);
			}
			break;

		case B_ALIGN_CENTER:		// ugh! we don't use this alignment
			ASSERT(false);
			break;
	}
}

// ---------------------------------------------------------------------------
//		DrawKeyBinding
// ---------------------------------------------------------------------------
//	The key binding is drawn in the view, aligned as specified, in the area rect.

float
MKeyIcons::DrawKeyBinding(
	BView*		inView,
	BRect		inArea,
	KeyBinding&	inBinding,
	alignment	inAlignment)
{
	if (! sBitmapInited)
		InitBitmaps();

	bool		drawControl = (inBinding.modifiers & B_CONTROL_KEY) != 0;
	bool		drawOption = (inBinding.modifiers & B_OPTION_KEY) != 0;
	bool		drawShift = (inBinding.modifiers & B_SHIFT_KEY) != 0;
	bool		drawCommand = (inBinding.modifiers & B_COMMAND_KEY) != 0;
	float		width = 0;
	float		charWidth;
	float		left;
	float		bottom = inArea.bottom - 2.0f;
	char		theChar = inBinding.keyCode;
	bool		isAsciiChar = theChar > 0x20 && theChar < 0x7f && ! inBinding.isVKey;

	// Calculate the width
	if (drawControl)
		width += kModWidth + 1.0f;
	if (drawOption)
		width += kModWidth + 1.0f;
	if (drawShift)
		width += kModWidth + 1.0f;
	if (drawCommand)
		width += kModWidth + 1.0f;

	if (isAsciiChar)
	{
		be_bold_font->GetEscapements(&theChar, 1, &charWidth);
		width += charWidth * be_bold_font->Size();
	}
	else
	{
		if (inBinding.isVKey)
			width += kVKeyWidth + 1.0f;
		else
			width += WidthLookup[theChar] + 1.0f;	
	}

	switch (inAlignment)
	{
		case B_ALIGN_LEFT:
			left = inArea.left + 2.0f;
			break;
		case B_ALIGN_RIGHT:
			left = inArea.right - width;
			break;
		case B_ALIGN_CENTER:
			left = inArea.left + inArea.Width() / 2.0f - width / 2.0f;
			break;
	}

	// Draw any modifier keys
	BPoint		where(left, bottom - kModHeight);

	if (drawControl)
	{
		inView->DrawBitmapAsync(sBitmapList.ItemAt(controlindex), where);
		left += kModWidth + 1.0f;
		where.x = left;
	}
	if (drawOption)
	{
		inView->DrawBitmapAsync(sBitmapList.ItemAt(optindex), where);
		left += kModWidth + 1.0f;
		where.x = left;
	}
	if (drawShift)
	{
		inView->DrawBitmapAsync(sBitmapList.ItemAt(shiftindex), where);
		left += kModWidth + 1.0f;
		where.x = left;
	}
	if (drawCommand)
	{
		inView->DrawBitmapAsync(sBitmapList.ItemAt(altindex), where);
		left += kModWidth + 1.0f;
		where.x = left;
	}

	// Draw the character
	if (isAsciiChar)
	{
		// Draw an ascii character normally
		bottom -= 1.0f;
		left += 1.0f;
		inView->MovePenTo(left, bottom);
		inView->DrawChar(theChar);
	}
	else
	{
		// If it's a special character we use a bitmap
		int32	index;
		if (inBinding.isVKey)
			index = VKeyLookup[theChar];
		else
			index = keyLookup[theChar];
		ASSERT(index >= 0 && index < sBitmapList.CountItems());
		
		where.y--;

		if (index != 0)
			inView->DrawBitmapAsync(sBitmapList.ItemAt(index), where);
	}
	
	return width;
}
