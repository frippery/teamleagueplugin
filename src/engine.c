#include "engine.h"
#include "BabasChessPlugin/BabasChessPlugin.h"
#include "tabset.h"
#include <string.h>
#include "status.h"
#include "dataset.h"
#include "settings.h"

typedef struct
{
	HINSTANCE hInstance;
	DWORD pluginId;
	BABASCHESS_DISPATCH_V_1_4 *dispatchTable;
	BOOL isOnline;
	char onlineUserName[CONFIG_MAX_USERNAME_LENGTH];
	int lastMinuteValue;
} Engine_t;

static Engine_t g_engine;

void Engine_OnTimer()
{
	SYSTEMTIME time;
	GetLocalTime(&time);
	if (time.wMinute != g_engine.lastMinuteValue)
	{
		g_engine.lastMinuteValue = time.wMinute;
		Tabset_Event(TABEVENT_MINUTE_CHANGED);
	}
}

BOOL Engine_IsOnline()
{
	return g_engine.isOnline;
}

char* Engine_GetUserName()
{
	return g_engine.onlineUserName;
}

void Engine_SendCommand(const char* command)
{
	g_engine.dispatchTable->BCAPI_SendCommand(g_engine.pluginId, command);
}

void Engine_RequestData()
{
	if (Engine_IsOnline() && !Dataset_GetDataset()->isUpdateInProgress);
	{
#ifdef SHOW_SPECIFIC_SEASON
		Engine_SendCommand(TEAMLEAGUE_CMD("botpairings " SHOW_SPECIFIC_SEASON));
#else
		Engine_SendCommand(TEAMLEAGUE_CMD("botpairings"));
#endif
		Tabset_Event(TABEVENT_DATA_UPDATE_REQUESTED);
	}
}

Status_t Engine_Init()
{
	Status_t status = Settings_Init(g_engine.hInstance);
	status = Dataset_init();
	g_engine.isOnline = FALSE;
	*g_engine.onlineUserName = '\0';
	return status;
}

void Engine_Destroy()
{
	Tabset_Destroy();
}

void Engine_SetDispatchTable(BABASCHESS_DISPATCH_V_1_4 * pTable)
{
	g_engine.dispatchTable = pTable;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	(void)lpReserved;
	switch( fdwReason )
	{ 
	case DLL_PROCESS_ATTACH:
		g_engine.hInstance = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

////////////////////////////////////////////////////////
// Plugin handlers
////////////////////////////////////////////////////////

_declspec(dllexport) BOOL _cdecl BCP_Initialize(DWORD dwBabasChessVersion, DWORD dwPluginID, DWORD *pDispatchversionRequired)
{
	Status_t status;

	if (dwBabasChessVersion<BCP_MAKEVERSION(3,9))
	{
		return FALSE;
	}

	*pDispatchversionRequired = BCP_MAKEVERSION(1,4);
	g_engine.pluginId = dwPluginID;

	status = Engine_Init();

	if (status != STATUS_OK)
	{
		return FALSE;
	}

	return TRUE;
}

_declspec(dllexport) void _cdecl BCP_DeInitialize()
{
	Engine_Destroy();
}

_declspec(dllexport) void _cdecl BCP_GetDispatchTable(void *pTable)
{
	Engine_SetDispatchTable(pTable);
}

_declspec(dllexport) void _cdecl BCP_OnLogOn()
{
	g_engine.isOnline = TRUE;
	g_engine.dispatchTable->BCAPI_GetUserName(g_engine.onlineUserName, CONFIG_MAX_USERNAME_LENGTH);

	Tabset_Event(TABEVENT_ONLINE);

	if (Settings_GetSettings()->updateOnLogon)
	{
		Engine_RequestData();
	}
}

_declspec(dllexport) void _cdecl BCP_OnLogOff(BOOL bReason)
{ 
	(void)bReason;
	g_engine.isOnline = FALSE;
	Tabset_Event(TABEVENT_OFFLINE);
	if (Dataset_GetDataset()->isUpdateInProgress)
	{
		Dataset_Abort();
	}
}

_declspec(dllexport) void _cdecl BCP_OnTimer() {}
_declspec(dllexport) BOOL _cdecl BCP_OnQTell(ServerOuputQTellType soq, const char *fullLine)
{
	if (soq == SOQ_Generic && strncmp(CONFIG_BOT_QTELL_IDENTIFIER, fullLine, sizeof(CONFIG_BOT_QTELL_IDENTIFIER)-1 ) == 0)
	{
		Status_t status = Dataset_ProcessDataChunk(fullLine);
		if (status == STATUS_OK_GAME_UPDATE_STARTED)
		{
			Tabset_Event(TABEVENT_DATA_UPDATING);
		}
		else if (status == STATUS_OK_GAME_UPDATE_COMPLETED || status == STATUS_OK_EMPTY_DATASET)  // TODO do something special on empty dataset
		{
			Tabset_Event(TABEVENT_DATA_UPDATED);
		}
		else if (status >= STATUS_ERR)
		{
			Tabset_Event(TABEVENT_DATA_UPDATE_ERROR);
		}
		return FALSE;
	}
	return TRUE;
}

_declspec(dllexport) BOOL _cdecl BCP_OnNotification(ServerOutputNotificationType son, const char *msg, const char *user, INT nGame, const char *fullLine)
{
	(void)son;
	(void)msg;
	(void)user;
	(void)nGame;
	(void)fullLine;

	return TRUE;
}

_declspec(dllexport) BOOL _cdecl BCP_OnRequest(ServerOutputRequestType sor, const char *msg, const char *fullLine)
{
	(void)sor;
	(void)msg;
	(void)fullLine;
	return TRUE;
}

_declspec(dllexport) BOOL _cdecl BCP_EnumInfoTabGUIDs(int index, GUID* pGUID, const char** strTitle, HBITMAP* pHBmp)
{
	switch(index)
	{
	case 0:
		*pGUID = *Tabset_GetGUID();
		*strTitle = Tabset_GetTitle();
		*pHBmp = Tabset_GetBitmap();
		return TRUE;
	default:
		return FALSE;
	}
}

_declspec(dllexport) HWND _cdecl BCP_CreateInfoTabWindow(const GUID* pGUID, HWND hWndParent)
{
	if (memcmp(pGUID, Tabset_GetGUID(), sizeof(GUID)) == 0)
	{
		COLORREF bgColor;
		COLORREF textColor;
		Status_t status;

		g_engine.dispatchTable->BCAPI_GetInfoWindowColors(g_engine.pluginId, &bgColor, &textColor);
		status = Tabset_init(hWndParent, g_engine.hInstance, bgColor, textColor);
		if (status != STATUS_OK)
		{
			return 0;
		}
		return Tabset_GetHWND();
	}
	return 0;
}

_declspec(dllexport) BOOL _cdecl BCP_OnChatMessage(ServerOuputChatType soc, const char* user, const char* titles, INT gameNum,
												   INT chNum, const char* msg, const char* fullLine)
{
	(void)titles;
	(void)gameNum;
	(void)msg;
	(void)fullLine;

	if (soc == SOC_ChTell && strcmp(user, "TeamLeague") == 0 && chNum == 101)
	{
		if (Dataset_ProcessHotData(msg))
		{
			Tabset_Event(TABEVENT_DATA_UPDATED);
		}
	}

	return TRUE;
}

_declspec(dllexport) void _cdecl BCP_SetMenu(DWORD* nMenuItems,char*** strItems)
{
	static char* menus[]=
	{
		"Update",
		"Settings...",
	};
	*strItems = menus;
	*nMenuItems = sizeof(menus)/sizeof(menus[0]);
}

_declspec(dllexport) BOOL _cdecl BCP_OnUpdateMenuStatus(DWORD nItem, BOOL* bChecked)
{
	(void)bChecked;

	switch(nItem)
	{
	case 0: // Update
		return (Engine_IsOnline() && !Dataset_GetDataset()->isUpdateInProgress);
	}
	
	return TRUE;
}

_declspec(dllexport) void _cdecl BCP_OnMenuItem(DWORD nItem)
{
	switch(nItem)
	{
	case 0: // Update
		Engine_RequestData();
		break;
	case 1:
		Settings_DoDialog();
		break;
	}
}

_declspec(dllexport) void _cdecl BCP_OnPlayedGameResult(const char* strGameType, BOOL bRated, const char* strWhite, const char* strBlack,
														const char* strResult)
{
	(void)strGameType;
	(void)bRated;
	(void)strWhite;
	(void)strBlack;
	(void)strResult;
}

_declspec(dllexport) void _cdecl BCP_OnRatingChange(const char* strRatingType, DWORD nOldRating, DWORD nNewRating)
{
	(void)strRatingType;
	(void)nOldRating;
	(void)nNewRating;
}
