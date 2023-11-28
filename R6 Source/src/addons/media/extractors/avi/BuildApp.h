//---------------------------------------------------------------------
//
//	File:	BuildApp.h
//
//	Author:	Gene Z. Ragan
//
//	Date:	09.10.98
//
//	Desc:	Application defines and exports
//
//	Copyright ©1998 mediapede Software
//
//---------------------------------------------------------------------

#ifndef __BUILDAPP_H__
#define __BUILDAPP_H__

//	Global Includes
#include <File.h>
#include <MediaDefs.h>
#include <support/ByteOrder.h>

#include <stdio.h>
#include <string.h>

#if DEBUG
#define PRINTF printf
#else
#define PRINTF (void)
#endif

#define PROGRESS PRINTF
#define LOOP PRINTF
#define FUNCTION  PRINTF
#define ERROR PRINTF

#endif
