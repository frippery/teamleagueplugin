#include "settings.h"
#include <Windows.h>
#include "../resource.h"
#include "dataset.h"
#include <stdio.h>

namespace {

std::wstring GetConfigFileName(HINSTANCE instance) {
  wchar_t buffer[2048];
  wchar_t* pos;
  GetModuleFileName(instance, buffer, (DWORD)(sizeof(buffer)/sizeof(wchar_t) - wcslen(CONFIG_CONFIG_FILENAME) - 2));
  pos = wcsrchr(buffer, L'\\');
  if (pos == NULL)
    pos = buffer + wcslen(buffer);
  *pos = L'\0';
  wcscpy_s(pos, sizeof(buffer)/sizeof(wchar_t)-(pos-buffer) , L"\\" CONFIG_CONFIG_FILENAME);
  return buffer;
}

typedef struct {
	char header[16]; // "moOOOoOoOoOoOo\0"
	int size;
} Settings_header_t;
}  // Anonymous namespace

Status_t Settings::ChangeTeamFavoriteStatus(const std::string& team, bool favorite) {
	for(int i = 0; i < data_.favorite_teams_size(); ++i) {
    if (data_.favorite_teams(i) == team) {
      if (favorite)
        return STATUS_ERR_TEAM_ALREADY_EXISTS;
      data_.set_favorite_teams(i, data_.favorite_teams(data_.favorite_teams_size()-1));
      data_.mutable_favorite_teams()->RemoveLast();
      return STATUS_OK;
    }
  }
  if (!favorite)
    return STATUS_ERR_NO_SUCH_TEAM;
  data_.add_favorite_teams(team);
  return STATUS_OK;
}

void Settings::PopulateDefaults() {
  if (!data_.has_time_mode())        data_.set_time_mode(SETTINGS_TIME_LOCAL);
  if (!data_.has_update_on_logon())  data_.set_update_on_logon(true);
  if (!data_.has_update_regularly()) data_.set_update_regularly(false);
  if (!data_.has_update_period())    data_.set_update_period(14);

  if (!data_.has_showfavoritesonly()) data_.set_showfavoritesonly(false);
  if (!data_.has_playsoundwhengamestarts())  data_.set_playsoundwhengamestarts(false);
  if (!data_.has_playsoundonlyforfavorites()) data_.set_playsoundonlyforfavorites(true);
}

Settings::Settings(HINSTANCE instance)
  : config_filename_(GetConfigFileName(instance)) {
    Load();
    PopulateDefaults();
}

Status_t Settings::Load() {
	FILE* file;
	Settings_header_t header;
	int err = _wfopen_s(&file, config_filename_.c_str(), L"rb");
	if (err != 0)	{
		return STATUS_ERR_FILE_CANNOT_OPEN;
	}

	if (fread(&header, sizeof(header), 1, file) != 1) {
		fclose(file);
		return STATUS_ERR_FILE_CANNOT_READ;
	}

	if (strcmp(header.header, "moOOOoOoOoOoOo\0") != 0) {
		return STATUS_ERR_FILE_WRONG_FORMAT;
	}

  std::vector<unsigned char> buffer(header.size);

	if (fread(&buffer.front(), header.size, 1, file) != 1) {
		fclose(file);
		return STATUS_ERR_FILE_CANNOT_READ;
	}

  fclose(file);
  if (!data_.ParseFromArray(&buffer.front(), buffer.size())) {
    return STATUS_ERR_FILE_WRONG_FORMAT;
  }

	return STATUS_OK;
}

Status_t Settings::Store() {
	FILE* file;
	int err = _wfopen_s(&file, config_filename_.c_str(), L"wb");
  if (err != 0)	{
    return STATUS_ERR_FILE_CANNOT_OPEN;
  }

  std::string str;
  data_.AppendToString(&str);
	Settings_header_t header = { "moOOOoOoOoOoOo", str.size()	};

	fwrite(&header, sizeof(header), 1, file);
	fwrite(str.data(), str.size(), 1, file);
	fclose(file);

  return STATUS_OK;
}

bool Settings::IsTeamFavorite(const std::string& team) const {
	for(int i = 0; i < data_.favorite_teams_size(); ++i)
	{
		if (data_.favorite_teams(i) == team) {
			return true;
		}
	}
	return false;
}

void Settings::DlgInsertTeam(HWND hwnd, const std::string& team, const std::string& section) {
  if (dialog_data_.teams_.find(team) != dialog_data_.teams_.end())
    return;

  if (dialog_data_.sections_.find(section) == dialog_data_.sections_.end()) {
		TVINSERTSTRUCTA tvi;
		tvi.hParent = 0;
		tvi.hInsertAfter = TVI_LAST;
		tvi.itemex.mask = TVIF_TEXT;
		tvi.itemex.pszText = const_cast<char*>(section.c_str());
    dialog_data_.sections_[section] = (HTREEITEM)SendMessageA((hwnd), TVM_INSERTITEMA, 0, (LPARAM)&tvi);
	}

  HTREEITEM parent = dialog_data_.sections_[section];

	{
		TVINSERTSTRUCTA tvi;
		tvi.hParent = parent;
		tvi.hInsertAfter = TVI_LAST;
		tvi.itemex.mask = TVIF_TEXT  | TVIF_STATE;
		tvi.itemex.state =  INDEXTOSTATEIMAGEMASK((IsTeamFavorite(team) ? 2 : 1));
		tvi.itemex.stateMask = TVIS_STATEIMAGEMASK;
		tvi.itemex.pszText = const_cast<char*>(team.c_str());
    dialog_data_.teams_[team] = (HTREEITEM)SendMessageA((hwnd), TVM_INSERTITEMA, 0, (LPARAM)&tvi);
		TreeView_Expand(hwnd, parent, TVE_EXPAND);
	}
}
/*
void Settings::UnpopulateDlg(HWND hwnd) {
	HWND favoritesHwnd = GetDlgItem(hwnd, IDC_TREE_FAVORITES);

	data_.set_time_mode(SETTINGS_TIME_LOCAL);
	if (IsDlgButtonChecked(hwnd, IDC_RADIO_TIME_FICS))
    data_.set_time_mode(SETTINGS_TIME_FICS);
	if (IsDlgButtonChecked(hwnd, IDC_RADIO_TIME_RELATIVE))
    data_.set_time_mode(SETTINGS_TIME_RELATIVE);

	data_.set_update_on_logon(IsDlgButtonChecked(hwnd, IDC_CHECK_UPDATE_LOGON) != 0);
  data_.set_update_period(GetDlgItemInt(hwnd, IDC_EDIT_UPDATE_INTERVAL, 0, FALSE));

  char buf[4096];
  WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, dialog_data_.playSoundFileName_.c_str(),
                      dialog_data_.playSoundFileName_.size(), &buf[0], sizeof(buf), NULL, NULL);
  data_.set_playsoundfilename(std::string(buf));

  data_.set_playsoundwhengamestarts(IsDlgButtonChecked(hwnd, IDC_CHECK_PLAY_SOUND) != 0);
  data_.set_playsoundonlyforfavorites(IsDlgButtonChecked(hwnd, IDC_CHECK_PLAY_ONLY_FAVORITES) != 0);

	if (dialog_data_.was_valid)	{
    data_.mutable_favorite_teams()->Clear();
    for(std::map<std::string, HTREEITEM>::const_iterator iter = dialog_data_.teams_.begin(), end = dialog_data_.teams_.end(); iter != end; ++iter) {
			if (TreeView_GetCheckState(favoritesHwnd, iter->second)) {
        data_.add_favorite_teams(iter->first);
			}
		}
	}
}

void Settings::PopulateDlg(HWND hwnd) {
	int timeButton = IDC_RADIO_TIME_LOCAL;
	HWND favoritesHwnd = GetDlgItem(hwnd, IDC_TREE_FAVORITES);
	SetWindowLong(favoritesHwnd, GWL_STYLE, GetWindowLong(favoritesHwnd, GWL_STYLE) | TVS_CHECKBOXES);

  if (data_.has_time_mode()) {
	  switch(data_.time_mode())	{
	  case SETTINGS_TIME_LOCAL:
		  timeButton = IDC_RADIO_TIME_LOCAL;
		  break;
	  case SETTINGS_TIME_FICS:
		  timeButton = IDC_RADIO_TIME_FICS;
		  break;
	  case SETTINGS_TIME_RELATIVE:
		  timeButton = IDC_RADIO_TIME_RELATIVE;
		  break;
	  }
  }

	CheckRadioButton(hwnd, IDC_RADIO_TIME_LOCAL, IDC_RADIO_TIME_RELATIVE, timeButton);
	CheckDlgButton(hwnd, IDC_CHECK_UPDATE_LOGON, data_.update_on_logon());
	CheckDlgButton(hwnd, IDC_CHECK_UPDATE_EVERY, data_.update_period());
	SendDlgItemMessage(hwnd, IDC_EDIT_UPDATE_INTERVAL, WM_ENABLE, (WPARAM) (data_.update_regularly()), 0);
  SetDlgItemInt(hwnd, IDC_EDIT_UPDATE_INTERVAL, data_.update_period(), FALSE);

  if (data_.has_playsoundfilename()) {
    wchar_t buf[2048];
    MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, data_.playsoundfilename().c_str(), data_.playsoundfilename().size(),
                        buf, sizeof(buf) / sizeof(wchar_t));
    dialog_data_.playSoundFileName_ = buf;
  } else {
    dialog_data_.playSoundFileName_.clear();
  }
  CheckDlgButton(hwnd, IDC_CHECK_PLAY_SOUND, data_.has_playsoundwhengamestarts() && data_.playsoundwhengamestarts());
  CheckDlgButton(hwnd, IDC_CHECK_PLAY_ONLY_FAVORITES, data_.has_playsoundonlyforfavorites() && data_.playsoundonlyforfavorites());
  
	dialog_data_.was_valid = Dataset::Get().is_data_valid();
  dialog_data_.teams_.clear();
  dialog_data_.sections_.clear();
  const Season_t& season = Dataset::Get().get_season();

	if (dialog_data_.was_valid)	{
		for(int i = 0; i < season.team_games_size(); ++i) {
      DlgInsertTeam(favoritesHwnd, season.team_games(i).team1(), season.team_games(i).section());
      DlgInsertTeam(favoritesHwnd, season.team_games(i).team1(), season.team_games(i).section());
		}
	}

	EnableWindow(GetDlgItem(hwnd, IDC_CHECK_PLAY_ONLY_FAVORITES), IsDlgButtonChecked(hwnd, IDC_CHECK_PLAY_SOUND));
	EnableWindow(GetDlgItem(hwnd, IDC_TESTSOUND),
    IsDlgButtonChecked(hwnd, IDC_CHECK_PLAY_SOUND) && !dialog_data_.playSoundFileName_.empty());
}

INT_PTR CALLBACK Settings_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	(void)lParam;
	(void)wParam;
	(void)uMsg;
	(void)hwndDlg;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		Settings::Get()->PopulateDlg(hwndDlg);
		return TRUE;
	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			switch(id)
			{
			case IDOK:
				Settings::Get()->UnpopulateDlg(hwndDlg);
			case IDCANCEL:   // Fall through
				EndDialog(hwndDlg, id);
				return TRUE;
			case IDC_CHECK_UPDATE_EVERY:
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_UPDATE_INTERVAL), IsDlgButtonChecked(hwndDlg, IDC_CHECK_UPDATE_EVERY));
				return TRUE;
			case IDC_BUTTON_BROWSESOUND:
				{
					OPENFILENAME openFileName;
					wchar_t fileName[CONFIG_MAX_CONFIG_PATHNAME_LENGTH] = {L'\0'};
					openFileName.lStructSize = sizeof(openFileName);
					openFileName.hwndOwner = hwndDlg;
					openFileName.hInstance = NULL;
					openFileName.lpstrFilter = L"Sound Files (*.wav)\0*.WAV\0All Files (*.*)\0*.*\0";
					openFileName.lpstrCustomFilter = NULL;
					openFileName.nMaxCustFilter = 0;
					openFileName.nFilterIndex = 0;
					openFileName.lpstrFile = fileName;
					openFileName.nMaxFile = CONFIG_MAX_CONFIG_PATHNAME_LENGTH;
					openFileName.lpstrFileTitle = NULL;
					openFileName.nMaxFileTitle = 0;
					openFileName.lpstrInitialDir = NULL;
					openFileName.lpstrTitle = NULL;
					openFileName.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_PATHMUSTEXIST;
					openFileName.nFileOffset = 0;
					openFileName.nFileExtension = 0;
					openFileName.lpstrDefExt = NULL;
					openFileName.lCustData = 0;
					if (GetOpenFileName(&openFileName))
					{
            Settings::Get()->GetDialogData().playSoundFileName_ = fileName;
						CheckDlgButton(hwndDlg, IDC_CHECK_PLAY_SOUND, TRUE);
					}
					else
					{
						return TRUE;
					}
				}
			case IDC_CHECK_PLAY_SOUND: // Fall through
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_PLAY_ONLY_FAVORITES), IsDlgButtonChecked(hwndDlg, IDC_CHECK_PLAY_SOUND));
				EnableWindow(GetDlgItem(hwndDlg, IDC_TESTSOUND), IsDlgButtonChecked(hwndDlg, IDC_CHECK_PLAY_SOUND) && 
                     !Settings::Get()->GetDialogData().playSoundFileName_.empty());
				return TRUE;
			case IDC_TESTSOUND:
				PlaySound(Settings::Get()->GetDialogData().playSoundFileName_.c_str(), NULL, SND_ASYNC | SND_FILENAME);
				return TRUE;
			}
			return FALSE;
		}
	}

	return FALSE;
}
*/
void Settings::DoDialog() {
/*	if (IDOK == DialogBox(Tabset_GetHInstance(), MAKEINTRESOURCE(IDD_DIALOG1), Tabset_GetHWND(), Settings_DialogProc)) {
		Store();
		Tabset_Event(TABEVENT_CONFIG_CHANGED);
	} */
	PlaySound(NULL, NULL, SND_ASYNC);
}