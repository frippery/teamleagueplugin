#ifndef __BABASCHESS_INCLUDED__
#define __BABASCHESS_INCLUDED__

#include <string>
#include <vector>
#include "BabasChessPlugin/BabasChessPlugin.h"

#pragma warning(push)
#pragma warning(disable: 4100)

class BabasChessPlugin {
public:
  static void SetBabasChessPlugin(BabasChessPlugin* plugin) { babaschess_plugin_ = plugin; }
  static BabasChessPlugin* Get() { return babaschess_plugin_; }

  typedef unsigned int TabID;
  virtual ~BabasChessPlugin() {}
  bool AttemptLogon();
  bool IsLoggedOn();
  bool GetFICSUserName(std::string* user_name);
  bool GetServerName(std::string* server_name);
  bool GetShortServerName(std::string* server_name);
  bool SendCommand(const std::string& command);
  bool WriteInConsole(const std::string& str);
  bool SetTimer(unsigned int seconds);
  HWND GetMainWindow();
  bool AddCustomChatTab(bool chat_or_channel, const std::string& title, TabID* tab_id);
  bool WriteInCustomTab(bool chat_or_channel, TabID tab_id, const std::string& text);
  bool RemoveCustomChatTab(bool chat_or_channel, TabID tab_id);
  void EnableMovesInObsBoards(bool enable);
  void GetInfoWindowColors(COLORREF* background_color, COLORREF* text_color);

public:  // :-(  Unable to have a friend which is extern "C" function
//private:
  typedef int MenuItem;

  virtual void OnGetMenu(std::vector<std::string>* menus) {}
  virtual bool OnUpdateMenuStatus(MenuItem item, bool* checked) { return true; }
  virtual void OnMenuItem(MenuItem) {}
  virtual void OnLogOn() {}
  virtual void OnLogOff(bool reason) {}
  virtual void OnTimer() {}
  virtual bool OnChatMessage(ServerOuputChatType soc, const std::string& user_name, const std::string& title,
                     int game_num, int chan_num, const std::string& message, const std::string& full_line) { return true; }
  virtual bool OnQTell(ServerOuputQTellType soc, const std::string& fullline) { return true; }
  virtual bool OnNotification(ServerOutputNotificationType soc, const std::string& msg, const std::string& user_name, int game, const std::string& fullline) {
    return true;
  }
  virtual bool OnRequest(ServerOutputRequestType sor, const std::string& msg, const std::string& fullline) { return true; }
  virtual void OnObservedBoardMove(int game, int half_move, const std::string& san, const std::string& lan) {}
  virtual void OnPlayedGameResult(const std::string& game_type, bool rated,
                                  const std::string& white, const std::string& black, const std::string& res) {}
  virtual void OnRatingChange(const std::string& rating_type, int old_rating, int new_rating) {}
  virtual void OnCustomChatCommand(bool chat_or_channel, TabID tab_id, const std::string& command) {}
  virtual void OnCustomChatRemoved(bool chat_or_channel, TabID tab_id) {}

  virtual bool OnEnumInfoTabGUIDs(int index, GUID* guid, const std::string* title, HBITMAP* bitmap) { return false; }
  virtual HWND OnCreateInfoTabWindow(const GUID* guid, HWND parent) { return 0; }

  void SetPluginId(DWORD plugin_id) { plugin_id_ = plugin_id; }
  void SetDispatchTable(BABASCHESS_DISPATCH_V_1_4* dispatch_table) { dispatch_table_ = dispatch_table; }

private:
  static BabasChessPlugin* babaschess_plugin_;
  DWORD plugin_id_;
  BABASCHESS_DISPATCH_V_1_4* dispatch_table_;
};

#pragma warning(pop)

#endif  /* __BABASCHESS_INCLUDED__ */