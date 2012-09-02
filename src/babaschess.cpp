#include <cassert>
#include "config.h"
#include "babaschess.h"

BabasChessPlugin* BabasChessPlugin::babaschess_plugin_  = 0;

bool BabasChessPlugin::GetFICSUserName(std::string* user_name) {
  assert(dispatch_table_);
  char buf[CONFIG_MAX_USERNAME_LENGTH];
  bool res = FALSE != dispatch_table_->BCAPI_GetUserName(buf, CONFIG_MAX_USERNAME_LENGTH);
  if (res)
    user_name->assign(buf);
  return res;
}

bool BabasChessPlugin::SendCommand(const std::string& command) {
  assert(dispatch_table_);
  return FALSE != dispatch_table_->BCAPI_SendCommand(plugin_id_, command.c_str());
}

extern "C" {
_declspec(dllexport) BOOL _cdecl BCP_Initialize(DWORD dwBabasChessVersion, DWORD dwPluginID, DWORD *pDispatchversionRequired) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  if (!plugin)
    return false;
  if (dwBabasChessVersion < BCP_MAKEVERSION(3,9)) {
    return false;
  }
  *pDispatchversionRequired = BCP_MAKEVERSION(1,4);
  if (plugin) {
    plugin->SetPluginId(dwPluginID);
  }
  return true;
}

_declspec(dllexport) void _cdecl BCP_DeInitialize() {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  if (plugin) {
    delete plugin;
    BabasChessPlugin::SetBabasChessPlugin(0);
  }
}

_declspec(dllexport) void _cdecl BCP_GetDispatchTable(void *pTable) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  plugin->SetDispatchTable((BABASCHESS_DISPATCH_V_1_4 *) pTable);
}

_declspec(dllexport) void _cdecl BCP_OnLogOn() {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  plugin->OnLogOn();
}

_declspec(dllexport) void _cdecl BCP_OnLogOff(BOOL bReason) { 
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  plugin->OnLogOff(bReason != FALSE);
}

_declspec(dllexport) void _cdecl BCP_OnTimer() {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  plugin->OnTimer();
}

_declspec(dllexport) BOOL _cdecl BCP_OnQTell(ServerOuputQTellType soq, const char *fullLine) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  return plugin->OnQTell(soq, fullLine);
}

_declspec(dllexport) BOOL _cdecl BCP_OnNotification(ServerOutputNotificationType son, const char *msg, const char *user, INT nGame, const char *fullLine) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  return plugin->OnNotification(son, msg, user, nGame, fullLine);
}

_declspec(dllexport) BOOL _cdecl BCP_OnRequest(ServerOutputRequestType sor, const char *msg, const char *fullLine) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  return plugin->OnRequest(sor, msg, fullLine);
}

_declspec(dllexport) BOOL _cdecl BCP_EnumInfoTabGUIDs(int index, GUID* pGUID, const char** strTitle, HBITMAP* pHBmp) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  static std::string str;
  bool res = plugin->OnEnumInfoTabGUIDs(index, pGUID, &str, pHBmp);
  *strTitle = str.c_str();
  return res;
}

_declspec(dllexport) HWND _cdecl BCP_CreateInfoTabWindow(const GUID* pGUID, HWND hWndParent) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  return plugin->OnCreateInfoTabWindow(pGUID, hWndParent);
}

_declspec(dllexport) BOOL _cdecl BCP_OnChatMessage(ServerOuputChatType soc, const char* user, const char* titles, INT gameNum,
                                                   INT chNum, const char* msg, const char* fullLine) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  return plugin->OnChatMessage(soc, user, titles, gameNum, chNum, msg, fullLine);
}

_declspec(dllexport) void _cdecl BCP_SetMenu(DWORD* nMenuItems, char*** strItems) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  static std::vector<std::string> menus;
  menus.clear();
  plugin->OnGetMenu(&menus);
  *nMenuItems = menus.size();
  static std::vector<const char*> menu_c_strs;
  menu_c_strs.clear();
  for (size_t i = 0; i < menus.size(); ++i) {
    menu_c_strs.push_back(menus[i].c_str());
  }
  if (!menus.empty()) {
    *strItems = const_cast<char**>(&menu_c_strs.front());
  }
}

_declspec(dllexport) BOOL _cdecl BCP_OnUpdateMenuStatus(DWORD nItem, BOOL* bChecked) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  bool val = false;
  bool res = plugin->OnUpdateMenuStatus(nItem, &val);
  *bChecked = val;
  return res;
}

_declspec(dllexport) void _cdecl BCP_OnMenuItem(DWORD nItem) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  plugin->OnMenuItem(nItem);
}

_declspec(dllexport) void _cdecl BCP_OnPlayedGameResult(const char* strGameType, BOOL bRated, const char* strWhite, const char* strBlack,
  const char* strResult) {
    BabasChessPlugin* plugin = BabasChessPlugin::Get();
    assert(plugin);
    plugin->OnPlayedGameResult(strGameType, bRated != FALSE, strWhite, strBlack, strResult);
}

_declspec(dllexport) void _cdecl BCP_OnRatingChange(const char* strRatingType, DWORD nOldRating, DWORD nNewRating) {
  BabasChessPlugin* plugin = BabasChessPlugin::Get();
  assert(plugin);
  plugin->OnRatingChange(strRatingType, nOldRating, nNewRating);
}
}  // extern "C"
