#include <windows.h>
#include "engine.h"
#include "status.h"

////////////////////////////////////////////////////////
// DllMain
////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch( fdwReason )
	{ 
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
