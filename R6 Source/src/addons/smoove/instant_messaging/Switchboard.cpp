/*
	Switchboard.cpp
*/
#include "IMManager.h"
#include <GHandler.h>

extern "C" _EXPORT GHandler *return_handler(GDispatcher * /* dispatcher */)
{
	return IMManager::Manager();
}
