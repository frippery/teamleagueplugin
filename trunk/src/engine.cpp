#include <cassert>
#include <string>
#include <windows.h>
#include "babaschess.h"
#include "dataset.h"
#include "settings.h"
#include "tools.h"

class TeamLeaguePlugin : public BabasChessPlugin {
public:
  TeamLeaguePlugin(HINSTANCE hInstance);
  void RequestData();
  bool IsOnline() const { return online_; }
  void UpdateTabset() {} // TODO(crem): Implement this

private:
  virtual void OnLogOn();
  virtual void OnLogOff(bool reason);
  virtual void OnTimer();
  virtual bool OnQTell(ServerOuputQTellType soq, const std::string& fullline);

  int last_minute_value_;
  Dataset dataset_;
  std::string username_;
  bool online_;
  HINSTANCE hInstance_;
  Settings settings_;
};


TeamLeaguePlugin::TeamLeaguePlugin(HINSTANCE hInstance)
    : hInstance_(hInstance),
      online_(false),
      settings_(hInstance) {
}

void TeamLeaguePlugin::RequestData() {
  if (IsOnline() && !dataset_.is_update_in_progress())	{
#ifdef SHOW_SPECIFIC_SEASON
    SendCommand(TEAMLEAGUE_CMD("botpairings " SHOW_SPECIFIC_SEASON));
#else
    SendCommand(TEAMLEAGUE_CMD("botpairings"));
#endif
    UpdateTabset();
  }
}

void TeamLeaguePlugin::OnTimer() {
  SYSTEMTIME time;
  GetLocalTime(&time);
  if (time.wMinute != last_minute_value_) 	{
    last_minute_value_ = time.wMinute;
    UpdateTabset();
  }
}

void TeamLeaguePlugin::OnLogOn() {
  online_ = true;
  GetFICSUserName(&username_);

  UpdateTabset();

  if (settings_.settings().update_on_logon()) {
    RequestData();
  }
}

void TeamLeaguePlugin::OnLogOff(bool reason) {
  (void)reason;
  online_ = false;
  if (dataset_.is_update_in_progress())
    dataset_.Abort();
}

bool TeamLeaguePlugin::OnQTell(ServerOuputQTellType soq, const std::string& fullline) {
  if (soq == SOQ_Generic && isPrefix(CONFIG_BOT_QTELL_IDENTIFIER, fullline)) {
    Status_t status = dataset_.ProcessDataChunk(fullline);
    if (status == STATUS_OK_GAME_UPDATE_COMPLETED || status == STATUS_OK_EMPTY_DATASET) {  // TODO do something special on empty dataset
      UpdateTabset();
    }
    return false;
  }
  return true;
}

#if 0

_declspec(dllexport) BOOL _cdecl BCP_EnumInfoTabGUIDs(int index, GUID* pGUID, const char** strTitle, HBITMAP* pHBmp) {
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

_declspec(dllexport) HWND _cdecl BCP_CreateInfoTabWindow(const GUID* pGUID, HWND hWndParent) {
	if (memcmp(pGUID, Tabset_GetGUID(), sizeof(GUID)) == 0)	{
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
												   INT chNum, const char* msg, const char* fullLine) {
	(void)titles;
	(void)gameNum;
	(void)msg;
	(void)fullLine;

	if (soc == SOC_ChTell && strcmp(user, "TeamLeague") == 0 && chNum == 101)
	{
		if (Dataset::Get().ProcessHotData(msg))
		{
			Tabset_Event(TABEVENT_DATA_UPDATED);
		}
	}

	return TRUE;
}

_declspec(dllexport) void _cdecl BCP_SetMenu(DWORD* nMenuItems,char*** strItems) {
	static char* menus[]=
	{
		"Update",
		"Settings...",
	};
	*strItems = menus;
	*nMenuItems = sizeof(menus)/sizeof(menus[0]);
}

_declspec(dllexport) BOOL _cdecl BCP_OnUpdateMenuStatus(DWORD nItem, BOOL* bChecked) {
	(void)bChecked;

	switch(nItem)	{
	case 0: // Update
		return (Engine_IsOnline() && !Dataset::Get().is_update_in_progress());
	}
	
	return TRUE;
}

_declspec(dllexport) void _cdecl BCP_OnMenuItem(DWORD nItem) {
	switch(nItem)
	{
	case 0: // Update
		Engine_RequestData();
		break;
	case 1:
		Settings::Get()->DoDialog();
		break;
	}
}
#endif


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
  (void)lpReserved;
  switch( fdwReason ) { 
    case DLL_PROCESS_ATTACH: {
      // TODO(crem) Move creation to BCP_Initialize
      DisableThreadLibraryCalls(hinstDLL);
      assert(!BabasChessPlugin::Get());
      BabasChessPlugin* plugin = new TeamLeaguePlugin(hinstDLL);
      BabasChessPlugin::SetBabasChessPlugin(plugin);
      break;
    }

    case DLL_PROCESS_DETACH: {
      BabasChessPlugin* plugin = BabasChessPlugin::Get();
      if (plugin) {
        delete plugin;
        BabasChessPlugin::SetBabasChessPlugin(0);
      }
      break;
    }
  }
  return TRUE;
}
