// --------------------------------------------------------------------------------------------------------
// BabasChess plugin interface header
// Dispatch table v1.4 (BabasChess v3.9 or later)
//
// Jan 2007 - by RRaf of FICS
// --------------------------------------------------------------------------------------------------------



#ifndef __BABASCHESSPLUGIN_H
#define __BABASCHESSPLUGIN_H

#ifndef _WINDOWS_
#include <windows.h>
#endif


// --------------------------------------------------------------------------------------------------------
// Definitions
// --------------------------------------------------------------------------------------------------------

#define BCP_MAX_STRING_LEN 4096
#define BCP_MAKEVERSION(major,minor) (((DWORD)major<<8)|minor)

// --------------------------------------------------------------------------------------------------------
// Functions that BabasChess makes available to the Plugin
// --------------------------------------------------------------------------------------------------------

typedef BOOL (__cdecl *TBCAPI_AttemptLogon)();
typedef BOOL (__cdecl *TBCAPI_IsLoggedOn)();
typedef BOOL (__cdecl *TBCAPI_GetUserName)(char *strUser,DWORD nMaxUserNameLen);
typedef BOOL (__cdecl *TBCAPI_GetServerName)(char *strServerName,DWORD nMaxServerNameLen);
typedef BOOL (__cdecl *TBCAPI_GetShortServerName)(char *strServerName,DWORD nMaxServerNameLen);
typedef BOOL (__cdecl *TBCAPI_SendCommand)(DWORD dwPluginID,const char *strCommand);
typedef BOOL (__cdecl *TBCAPI_WriteInConsole)(DWORD dwPluginID,const char *str);
typedef BOOL (__cdecl *TBCAPI_SetTimer)(DWORD dwPluginID,DWORD nSecInterval);	// nSecInterval==0 to remove the timer
typedef HWND (__cdecl *TBCAPI_GetMainWnd)();
typedef BOOL (__cdecl *TBCAPI_AddCustomChatTab)(DWORD dwPluginID,BOOL bChatOrChannel,const char *strTitle,DWORD *pTabID);	// 0 is never used as a Tab ID
typedef BOOL (__cdecl *TBCAPI_WriteInCustomChatTab)(DWORD dwPluginID,BOOL bChatOrChannel,DWORD dwTabID,const char *strText);
typedef BOOL (__cdecl *TBCAPI_RemoveCustomChatTab)(DWORD dwPluginID,BOOL bChatOrChannel,DWORD dwTabID);
typedef void (__cdecl *TBCAPI_EnableMovesInObsBoards)(DWORD dwPluginID,BOOL bEnable);
typedef void (__cdecl *TBCAPI_GetInfoWindowColors)(DWORD dwPluginID,COLORREF *pBackColor,COLORREF *pTextColor);

// Dispatch table
typedef struct _BABASCHESS_DISPATCH_V_1_4 {
	TBCAPI_AttemptLogon BCAPI_AttemptLogon;
	TBCAPI_IsLoggedOn BCAPI_IsLoggedOn;
	TBCAPI_GetUserName BCAPI_GetUserName;
	TBCAPI_GetServerName BCAPI_GetServerName;
	TBCAPI_GetShortServerName BCAPI_GetShortServerName;
	TBCAPI_SendCommand BCAPI_SendCommand;
	TBCAPI_WriteInConsole BCAPI_WriteInConsole;
	TBCAPI_SetTimer BCAPI_SetTimer;
	TBCAPI_GetMainWnd BCAPI_GetMainWnd;
	TBCAPI_AddCustomChatTab BCAPI_AddCustomChatTab;
	TBCAPI_WriteInCustomChatTab BCAPI_WriteInCustomChatTab;
	TBCAPI_RemoveCustomChatTab BCAPI_RemoveCustomChatTab;
	TBCAPI_EnableMovesInObsBoards BCAPI_EnableMovesInObsBoards;
	TBCAPI_GetInfoWindowColors BCAPI_GetInfoWindowColors;
} BABASCHESS_DISPATCH_V_1_4;



// Old dispatch tables that are still supported by BabasChess
typedef struct _BABASCHESS_DISPATCH_V_1_2 {
	TBCAPI_AttemptLogon BCAPI_AttemptLogon;
	TBCAPI_IsLoggedOn BCAPI_IsLoggedOn;
	TBCAPI_GetUserName BCAPI_GetUserName;
	TBCAPI_SendCommand BCAPI_SendCommand;
	TBCAPI_WriteInConsole BCAPI_WriteInConsole;
	TBCAPI_SetTimer BCAPI_SetTimer;
	TBCAPI_GetMainWnd BCAPI_GetMainWnd;
	TBCAPI_AddCustomChatTab BCAPI_AddCustomChatTab;
	TBCAPI_WriteInCustomChatTab BCAPI_WriteInCustomChatTab;
	TBCAPI_RemoveCustomChatTab BCAPI_RemoveCustomChatTab;
	TBCAPI_EnableMovesInObsBoards BCAPI_EnableMovesInObsBoards;
} BABASCHESS_DISPATCH_V_1_2;

typedef struct _BABASCHESS_DISPATCH_V_1_1 {
	TBCAPI_AttemptLogon BCAPI_AttemptLogon;
	TBCAPI_IsLoggedOn BCAPI_IsLoggedOn;
	TBCAPI_GetUserName BCAPI_GetUserName;
	TBCAPI_SendCommand BCAPI_SendCommand;
	TBCAPI_WriteInConsole BCAPI_WriteInConsole;
	TBCAPI_SetTimer BCAPI_SetTimer;
	TBCAPI_GetMainWnd BCAPI_GetMainWnd;
} BABASCHESS_DISPATCH_V_1_1;



// --------------------------------------------------------------------------------------------------------

// Enums (new values may be appended in the future)
typedef enum { SOC_Say, SOC_Tell, SOC_PTell, SOC_ChTell, SOC_Kibitz, SOC_Whisper, SOC_Shout, SOC_CShout, SOC_It } ServerOuputChatType;
typedef enum { SON_Generic, SON_FriendArrived, SON_FriendNotByArrived, SON_FriendDeparted, SON_FriendNotByDeparted, SON_GameNotify } ServerOutputNotificationType;
typedef enum { SOQ_Generic } ServerOuputQTellType;
typedef enum { SOR_Challenge, SOR_ChallengeSecondLine, SOR_Abort, SOR_Draw, SOR_Adjourn, SOR_BugPartner, SOR_Takeback, SOR_Pause, SOR_Unpause, SOR_Simul, SOR_Switch } ServerOutputRequestType;


// --------------------------------------------------------------------------------------------------------
// Functions that the Plugin has to implement and export
// --------------------------------------------------------------------------------------------------------

typedef BOOL (__cdecl *TBCP_Initialize)(DWORD dwBabasChessVersion,DWORD dwPluginID,DWORD *pDispatchversionRequired);
typedef void (__cdecl *TBCP_DeInitialize)();
typedef void (__cdecl *TBCP_GetDispatchTable)(void *pTable);
typedef void (__cdecl *TBCP_SetMenu)(DWORD *nMenuItems,const char ***strItems);

typedef BOOL (__cdecl *TBCP_OnUpdateMenuStatus)(DWORD nItem,BOOL *bChecked);
typedef void (__cdecl *TBCP_OnMenuItem)(DWORD nItem);

typedef void (__cdecl *TBCP_OnLogOn)();
typedef void (__cdecl *TBCP_OnLogOff)(BOOL bReason);
typedef void (__cdecl *TBCP_OnTimer)();
typedef BOOL (__cdecl *TBCP_OnUpdateMenuStatus)(DWORD nItem,BOOL *bChecked);
typedef void (__cdecl *TBCP_OnMenuItem)(DWORD nItem);

typedef BOOL (__cdecl *TBCP_OnChatMessage)(ServerOuputChatType soc,const char *user,const char *titles, INT32 gameNum,INT32 chNum,const char *msg, const char *fullLine);
typedef BOOL (__cdecl *TBCP_OnQTell)(ServerOuputQTellType soq, const char *fullLine);
typedef BOOL (__cdecl *TBCP_OnNotification)(ServerOutputNotificationType son,const char *msg, const char *user, INT32 nGame, const char *fullLine);
typedef BOOL (__cdecl *TBCP_OnRequest)(ServerOutputRequestType sor,const char *msg, const char *fullLine);

typedef void (__cdecl *TBCP_OnObservedBoardMove)(int nGame,DWORD nHalfMove,const char *strSAN,const char *strLAN);
typedef void (__cdecl *TBCP_OnPlayedGameResult)(const char *strGameType,BOOL bRated,const char *strWhite,const char *strBlack,const char *strResult);
typedef void (__cdecl *TBCP_OnRatingChange)(const char *strRatingType,DWORD nOldRating,DWORD nNewRating);
typedef void (__cdecl *TBCP_OnCustomChatCommand)(BOOL bChatOrChannelnGame,DWORD dwTabID,const char *strCommand);
typedef void (__cdecl *TBCP_OnCustomChatRemoved)(BOOL bChatOrChannelnGame,DWORD dwTabID);

// Info tabs
typedef BOOL (__cdecl *TBCP_EnumInfoTabGUIDs)(int index,GUID *pGUID,const char **strTitle,HBITMAP *pHBmp);
typedef HWND (__cdecl *TBCP_CreateInfoTabWindow)(const GUID *pGUID,HWND hWndParent);


#endif // __BABASCHESSPLUGIN_H
