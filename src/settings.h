#ifndef __SETTINGS_INCLUDED__
#define __SETTINGS_INCLUDED__

#include <string>
#include <vector>
#include <map>
#include "config.h"
#include <windows.h>
#include <CommCtrl.h>
#include "status.h"
#pragma warning( push, 3 )
#include "settings.pb.h"
#pragma warning( pop )

class Settings {
public:
  Settings(const std::wstring& filename);
  void DoDialog();
  Status_t Store();
  bool IsTeamFavorite(const std::string& team);
  SettingsFile_t& settings() { return data_; }
  Status_t ChangeTeamFavoriteStatus(const std::string& team, bool favorite);
  static Settings* Get();

private:
  struct DialogData {
    bool was_valid;  // True if teamleague data was valid when dialog appears
    std::wstring playSoundFileName_;
    std::map<std::string, HTREEITEM> sections_;
    std::map<std::string, HTREEITEM> teams_;
  };

  Status_t Load();
  void PopulateDefaults();
  void DlgInsertTeam(HWND hwnd, const std::string& team, const std::string& section);
  void UnpopulateDlg(HWND hwnd);
  void PopulateDlg(HWND hwnd);
  DialogData& GetDialogData() { return dialog_data_; }

  SettingsFile_t data_;
  std::wstring config_filename_;

  DialogData dialog_data_;
  friend INT_PTR CALLBACK Settings_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif