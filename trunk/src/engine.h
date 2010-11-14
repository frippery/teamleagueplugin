#ifndef __ENGINE_INCLUDED__
#define __ENGINE_INCLUDED__

#include <windows.h>
#include "status.h"
#include "BabasChessPlugin/BabasChessPlugin.h"

void Engine_SendCommand(const char* command);
BOOL Engine_IsOnline();
void Engine_RequestData();
void Engine_OnTimer();
char* Engine_GetUserName();

#endif