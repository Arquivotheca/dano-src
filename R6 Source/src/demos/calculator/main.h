//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#include "window.h"
#include "view.h"
#include "displayview.h"
#include "application.h"

#define PROGNAME			"Calculator"

#define ALLOW_ZOOM			0			// 1 allows the zooming

extern CalcWindow			*theWindow;
extern CalcView				*theView;
extern DisplayView			*dispView;
extern CalcApplication		*theApp;

extern int32				windowTop;
extern int32				windowLeft;
extern double				initValue;

extern void KeyBeep(void);
